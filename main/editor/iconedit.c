#include <WINDOWS.H>  
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#include "iconedit.h"
#include "canvas.h"
#include "toolbar.h"
#include "colorbox.h"
#include "log.h"

static char szBuffer[80];
// static char szLogMessage[80];
static char szLogLines[10][80];
static logWriteIndex, logReadIndex;
static short nLength;
static HWND debugWindow;
#define CALL_DEPTH_LIMIT 1024
static int callDepthCurrent, callDepthMax;
extern int STKHQQ;
static WORD stackPointerMin, stackPointerCurrent, stackPointerStart;
static short pixW, pixH;
static HWND floodHWND;
static HDC floodHDC;
static CanvasHistoryEntry_s* canvasHistory[CANVAS_HISTORY_LEN];
static int canvasHistoryWriteIndex;
static int canvasHistoryReadIndex;
static BYTE restoreData[] = {1, 1, 1, 1, 1,
                             1, 4, 5, 4, 1,
                             1, 5, 5, 5, 1,
                             1, 4, 5, 4, 1,
                             1, 1, 1, 1, 1};


#define FLOOD_VER 7

void free_history_entry(int index){
    if(canvasHistory[index] != NULL){
        free(canvasHistory[index]->nextAction->args);
        free(canvasHistory[index]->prevAction->args);    
        free(canvasHistory[index]);  
        canvasHistory[index] = NULL;
    }
}

BOOL create_history_entry(CanvasTool_e tool){
    void* doArgs = NULL; 
    void* undoArgs = NULL;
    CanvasAction_s* doAction = NULL;
    CanvasAction_s* undoAction = NULL;
    CanvasHistoryEntry_s* newEntry = NULL;
    

    newEntry = (CanvasHistoryEntry_s*) malloc(sizeof(CanvasHistoryEntry_s));
    if(newEntry == NULL){
        goto FAIL;
    }

    doAction = (CanvasAction_s*) malloc(sizeof(CanvasAction_s));
    if(doAction == NULL){
        goto FAIL;
    }
    
    undoAction = (CanvasAction_s*) malloc(sizeof(CanvasAction_s));                    
    if(undoAction == NULL){
        goto FAIL;
    }

    switch(tool){
        case CanvasToolBrush:
            doAction->tool = CanvasToolBrush;
            doArgs = malloc(sizeof(CanvasBrushArgs_s));
 
            undoAction->tool = CanvasToolBrush;
            undoArgs = malloc(sizeof(CanvasBrushArgs_s)); 
            break;

        case CanvasToolLine:
            doAction->tool = CanvasToolLine;
            doArgs = malloc(sizeof(CanvasLineArgs_s));

            undoAction->tool = CanvasToolRestore; 
            undoArgs = malloc(sizeof(CanvasRestoreArgs_s)); 
            break;    

        case CanvasToolRect:
            doAction->tool = CanvasToolRect;
            doArgs = malloc(sizeof(CanvasRectArgs_s));

            undoAction->tool = CanvasToolRestore; 
            undoArgs = malloc(sizeof(CanvasRestoreArgs_s)); 
            break;       

        case CanvasToolFlood:
            doAction->tool = CanvasToolFlood;
            doArgs = malloc(sizeof(CanvasFloodArgs_s));

            undoAction->tool = CanvasToolRestore; 
            undoArgs = malloc(sizeof(CanvasRestoreArgs_s)); 
            break;   

        default:
            goto FAIL;
    }

    // check here instead of after each malloc for readibility
    if((doArgs == NULL) || (undoArgs == NULL)){
        goto FAIL;
    }


    free_history_entry(canvasHistoryWriteIndex);
    
    // make actions
    doAction->args = doArgs;
    undoAction->args = undoArgs;

    // make entry
    newEntry->valid = TRUE;
    newEntry->nextAction = doAction;
    newEntry->prevAction = undoAction;

    canvasHistory[canvasHistoryWriteIndex] = newEntry;
    canvasHistoryReadIndex = canvasHistoryWriteIndex;
    canvasHistoryWriteIndex = (canvasHistoryWriteIndex + 1) % CANVAS_HISTORY_LEN;
    if(canvasHistory[canvasHistoryWriteIndex] != NULL){
        canvasHistory[canvasHistoryWriteIndex]->valid = FALSE;
    }
    return TRUE;


    FAIL:
    free(newEntry);
    free(doAction);
    free(undoAction);
    free(doArgs);
    free(undoArgs);
    return FALSE;

}

// As-implemented, the cursor is not bounded here
// This is intentional since eventually the canvas selector will be shown w/ a rectangle instead of the cursor
void move_cursor(short xAmount, short yAmount){
    POINT lpCursorPoint;

    GetCursorPos(&lpCursorPoint);
    // nLength = wsprintf(szBuffer, "x %d, y %d", lpCursorPoint.x, lpCursorPoint.y);
    // MessageBox(debugWindow, szBuffer, "Before", MB_OK);
    lpCursorPoint.x += (WORD)xAmount;
    lpCursorPoint.y += (WORD)yAmount;
    SetCursorPos(lpCursorPoint.x, lpCursorPoint.y);

    // nLength = wsprintf(szBuffer, "x %d, y %d", lpCursorPoint.x, lpCursorPoint.y);
    // MessageBox(debugWindow, szBuffer, "After", MB_OK);
    return;
}                            

BOOL map_toolbar_tool_to_canvas(CanvasTool_e* canvasTool, ToolbarTool_e toolbarTool){
    *canvasTool = (CanvasTool_e)toolbarTool;
    return TRUE;
}                        

BOOL pixel_color_code_to_rgb(WORD code, COLORREF* color){
    if(code >= 16){
        return FALSE;
    }
    *color = pixelColorList[code];
    return TRUE;
}

// BOOL canvas_act(HWND hwnd, HDC* hdc, BYTE* pixelFrame, CanvasTool_e tool, WORD x, WORD y, short size){
//     switch (tool){
//         case CanvasToolBrush:{
//             canvas_draw_brush(hwnd, hdc, pixelFrame, )
//             break;
//         }
//     }
// }

BYTE canvas_draw_brush(HDC* hdc, BYTE* pixelFrame, CanvasBrushArgs_s* args){
    COLORREF newColor;
    HBRUSH hBrush;
    RECT rect;
    short pixelCol, pixelRow;
    BYTE oldColorCode;

    pixelCol = PIXEL_1D_2_COL(args->pixel);
    pixelRow = PIXEL_1D_2_ROW(args->pixel);

    oldColorCode = pixelFrame[args->pixel];
    pixelFrame[args->pixel] = (BYTE)(args->newColorCode);  

    pixel_color_code_to_rgb(args->newColorCode, &newColor);
    hBrush = CreateSolidBrush(newColor);

    rect.left = (pixelCol*args->size)+1;
    rect.top = (pixelRow*args->size)+1;
    rect.right = rect.left + args->size - 2 ;
    rect.bottom = rect.top + args->size - 2;

    FillRect(*hdc, &rect, hBrush);
    DeleteObject(hBrush);

    return oldColorCode;
}


// Bresenham's line algo
// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
// Specifically, the versions tracking error for X and Y
BOOL canvas_draw_line(HDC* hdc, BYTE* pixelFrame, CanvasLineArgs_s* args, int* restoreLength, BYTE** restoreData){
    short pixelX, pixelY;
    short deltaX, deltaY, errorD, signX, signY, error2;
    short bailCounter = CANVAS_DIM*2;
    CanvasBrushArgs_s brushArgs;

    // abs of difference
    deltaX = args->pt2.x - args->pt1.x;
    deltaX = deltaX < 0 ? -1*deltaX : deltaX;

    // get sign
    signX = args->pt1.x < args->pt2.x ? 1 : -1;

    //-1*abs of difference
    deltaY = args->pt2.y - args->pt1.y;
    deltaY = deltaY < 0 ? deltaY : -1*deltaY;

    // get sign
    signY = args->pt1.y < args->pt2.y ? 1 : -1;    

    errorD = deltaX + deltaY;
    pixelX = args->pt1.x;
    pixelY = args->pt1.y;    

    if((restoreLength != NULL) && (*restoreLength > 0) && (*restoreLength <= PIXEL_COUNT)){
        int leftX, rightX, topY, botY, x, y, counter;

        leftX = min(args->pt1.x, args->pt2.x);
        rightX = max(args->pt1.x, args->pt2.x);
        topY = min(args->pt1.y, args->pt2.y);
        botY = max(args->pt1.y, args->pt2.y);

        *restoreLength = (rightX - leftX + 1)*(botY - topY + 1);

        *restoreData = (BYTE*)malloc(sizeof(BYTE)*(*restoreLength));
        if(*restoreData == NULL){
            MessageBox(NULL, "Unable to malloc restore data", "Line", MB_OK);
            *restoreLength = 0;
            return FALSE;
        }

        counter = 0;
        for(x = leftX; x <= rightX; x++){
            for(y = topY; y <= botY; y++){
                (*restoreData)[counter] = pixelFrame[PIXEL_2D_2_1D(x, y)];
                counter++;
            }
        }
    }

    while(bailCounter-- > 0){
        brushArgs.pixel = PIXEL_2D_2_1D(pixelX, pixelY);
        brushArgs.size = args->size;
        brushArgs.newColorCode = args->newColorCode;
        canvas_draw_brush(hdc, pixelFrame, &brushArgs);

        if((pixelX == args->pt2.x) && (pixelY == args->pt2.y)){
            break; //handle double-click on same spot
            // MessageBox(hwnd, "First break", "Line", MB_OK);
        }
        error2 = 2*errorD;
        if(error2 >= deltaY){
            if(pixelX == args->pt2.x){
                break;
                // MessageBox(hwnd, "2nd break", "Line", MB_OK);
            }
            errorD += deltaY;
            pixelX += signX;
        }
        if(error2 <= deltaX){
            if(pixelY == args->pt2.y){
                break;
                // MessageBox(hwnd, "3rd break", "Line", MB_OK);
            }
            errorD += deltaX;
            pixelY += signY;
        }
    }
    if(bailCounter <= 0){
        MessageBox(NULL, "Error", "Line", MB_OK);
    }
    return TRUE;
}


BOOL canvas_draw_rect(HDC* hdc, BYTE* pixelFrame, CanvasRectArgs_s* args, int* restoreLength, BYTE** restoreData){
    short pixelX, pixelY, pxLeftCol, pxRightCol, pxTopRow, pxBotRow;
    CanvasBrushArgs_s brushArgs;

    pxLeftCol = min(args->pt1.x, args->pt2.x);
    pxRightCol = max(args->pt1.x, args->pt2.x);    
    pxTopRow = min(args->pt1.y, args->pt2.y);
    pxBotRow = max(args->pt1.y, args->pt2.y);   

    if((restoreLength != NULL) && (*restoreLength > 0) && (*restoreLength <= PIXEL_COUNT)){
        int counter;
        *restoreLength = (pxRightCol - pxLeftCol + 1)*(pxBotRow - pxTopRow + 1);

        *restoreData = (BYTE*)malloc(sizeof(BYTE)*(*restoreLength));
        if(*restoreData == NULL){
            MessageBox(NULL, "Unable to malloc restore data", "Rect", MB_OK);
            *restoreLength = 0;
            return FALSE;
        }

        counter = 0;
        for(pixelX = pxLeftCol; pixelX <= pxRightCol; pixelX++){
            for(pixelY = pxTopRow; pixelY <= pxBotRow; pixelY++){
                (*restoreData)[counter] = pixelFrame[PIXEL_2D_2_1D(pixelX, pixelY)]; //TODO replace w/ linear copy
                counter++;
            }
        }
    } 

    for(pixelX = pxLeftCol; pixelX <= pxRightCol; pixelX++){
        for(pixelY = pxTopRow; pixelY <= pxBotRow; pixelY++){
            brushArgs.pixel = PIXEL_2D_2_1D(pixelX, pixelY);
            brushArgs.size = args->size;
            brushArgs.newColorCode = args->newColorCode;

            canvas_draw_brush(hdc, pixelFrame, &brushArgs);
        }
    }

    return TRUE;
}


BOOL canvas_restore_rect(HDC* hdc, BYTE* pixelFrame, CanvasRestoreArgs_s* args){
    short pixelX, pixelY, pxLeftCol, pxRightCol, pxTopRow, pxBotRow;
    short counter;
    CanvasBrushArgs_s brushArgs;

    pxLeftCol = args->ptNW.x;
    pxRightCol = args->ptSE.x;   
    pxTopRow = args->ptNW.y;
    pxBotRow = args->ptSE.y;  

    if((args->dataLength) != ((pxRightCol - pxLeftCol + 1)*(pxBotRow - pxTopRow + 1))){
        nLength = wsprintf (szBuffer, "Data length error. %d vs %d.", args->dataLength, (pxRightCol - pxLeftCol + 1)*(pxBotRow - pxTopRow + 1));
        MessageBox(NULL, szBuffer, "Restore", MB_OK);
        return FALSE;
    }

    brushArgs.size = args->size;
    counter = 0;
    for(pixelX = pxLeftCol; pixelX <= pxRightCol; pixelX++){
        for(pixelY = pxTopRow; pixelY <= pxBotRow; pixelY++){
            brushArgs.pixel = PIXEL_2D_2_1D(pixelX, pixelY);

            // oldColorCode = pixelFrame[brushArgs.pixel];
            // brushArgs.newColorCode = (oldColorCode + 8)%16;

            brushArgs.newColorCode = (args->colorData)[counter];
            canvas_draw_brush(hdc, pixelFrame, &brushArgs);

            counter++;
        }
    }

    return TRUE;
}


#if FLOOD_VER == 0
BOOL canvas_draw_flood_v0(HWND hwnd, HDC* hdc, BYTE* pixelFrame, int pixel, POINT* ptClick, short width, short height, PixelColorCode_e targetColorCode){
    short pixelRow, pixelCol, nextRow, nextCol;
    POINT clickPoint;
    PixelColorCode_e currentColorCode;

    callDepthCurrent++;

    pixelRow = PIXEL_1D_2_ROW(pixel);
    pixelCol = PIXEL_1D_2_COL(pixel);    
    clickPoint.x = pixelCol*width;
    clickPoint.y = pixelRow*height;

    currentColorCode = pixelFrame[pixel];
    if (currentColorCode != targetColorCode){
        goto FLOOD_EXIT;
    }else{
        canvas_draw_brush(hwnd, hdc, pixelFrame, pixel, &clickPoint, width, height);
        // nLength = wsprintf(szBuffer, "Depth %d, Row %d, Col %d.", callDepthCurrent, pixelRow, pixelCol);
        // MessageBox(hwnd, szBuffer, "Fill", MB_OK);
    }

    if(callDepthCurrent >= CALL_DEPTH_LIMIT){
        goto FLOOD_EXIT;
    }
    

    if(pixelCol > 0){
        canvas_draw_flood_v0(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol-1, pixelRow), &clickPoint, width, height, targetColorCode);
    }
    if(pixelCol < CANVAS_DIM-1){
        canvas_draw_flood_v0(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol+1, pixelRow), &clickPoint, width, height, targetColorCode);
    }

    if(pixelRow > 0){
        canvas_draw_flood_v0(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow-1), &clickPoint, width, height, targetColorCode);    
    }

    if(pixelRow < CANVAS_DIM-1){
        canvas_draw_flood_v0(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow+1), &clickPoint, width, height, targetColorCode);    
    }

    FLOOD_EXIT:
    // Capture SP depth
    __asm mov stackPointerCurrent, sp;
    stackPointerMin = min(stackPointerCurrent, stackPointerMin);

    // Capture call depth
    callDepthMax = max(callDepthCurrent, callDepthMax);
    callDepthCurrent--;
    return TRUE;
}
#endif



void pixel_num_to_bitfield(int pixel, short* byteNum, short* byteOffset){
    *byteNum = pixel / 8;
    *byteOffset = pixel % 8;
}


#if FLOOD_VER == 7
// Fixed length queue of max length without adding duplicates to queue
// SP about 3k (recheck after adding restore code)
BOOL canvas_draw_flood_v7(HDC* hdc, BYTE* pixelFrame, CanvasFloodArgs_s* args, int* restoreLength, BYTE** restoreData, POINT* ptNW, POINT* ptSE){
    int pixelQueue[PIXEL_COUNT];
    BYTE pixelAdded[PIXEL_COUNT];
    short pixelRow, pixelCol;
    short rowMin, rowMax, colMin, colMax;
    short i;
    short readIndex, writeIndex;
    int checkPixel;
    CanvasBrushArgs_s brushArgs;

    if (pixelFrame[args->pixel] != args->targetColorCode){
        //bail early for flooding shape with same color
        goto FLOOD_EXIT; 
    }

    for(i=0; i<PIXEL_COUNT; i++){
        pixelAdded[i] = 0;
    }
    
    readIndex = writeIndex = 0;

    // Set up first entry
    pixelQueue[writeIndex++] = args->pixel;
    pixelAdded[args->pixel] = 1;

    rowMin = colMin = CANVAS_DIM;
    rowMax = colMax = 0;
    do{
        // for breaking infinite loop error
        callDepthCurrent++;
        // if(callDepthCurrent > CALL_DEPTH_LIMIT){
        //     MessageBox(NULL, "Depth", "Flood v6", MB_OK);
        //     goto FLOOD_EXIT;
        // }

        if(writeIndex > 1024){
            MessageBox(NULL, "Write max", "Flood v7", MB_OK);
            goto FLOOD_EXIT;
        }

        // Get next pixel
        args->pixel = pixelQueue[readIndex++];

        // Compute row and column
        pixelRow = PIXEL_1D_2_ROW(args->pixel);
        pixelCol = PIXEL_1D_2_COL(args->pixel);    

        // Check 4-way adjacent pixels and set flag if need to flood them
        // Left
        checkPixel = PIXEL_2D_2_1D(pixelCol-1, pixelRow);
        if((pixelCol > 0) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){            
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;
            rowMin = min(rowMin, pixelRow);
            rowMax = max(rowMax, pixelRow);
            colMin = min(colMin, pixelCol-1);
            colMax = max(colMax, pixelCol-1);            
        }

        // Up
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow-1);        
        if((pixelRow > 0) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;  
            rowMin = min(rowMin, pixelRow-1);
            rowMax = max(rowMax, pixelRow-1);
            colMin = min(colMin, pixelCol);
            colMax = max(colMax, pixelCol);          
        }

        // Right
        checkPixel = PIXEL_2D_2_1D(pixelCol+1, pixelRow);
        if((pixelCol < CANVAS_DIM-1) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){            
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;  
            rowMin = min(rowMin, pixelRow);
            rowMax = max(rowMax, pixelRow);
            colMin = min(colMin, pixelCol+1);
            colMax = max(colMax, pixelCol+1);
        }

        // Down
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow+1);        
        if((pixelRow < CANVAS_DIM-1) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;  
            rowMin = min(rowMin, pixelRow+1);
            rowMax = max(rowMax, pixelRow+1);
            colMin = min(colMin, pixelCol);
            colMax = max(colMax, pixelCol);
        }

    // Check if pixels left to check
    }while(readIndex < writeIndex);
    
    // Capture current state of region
    if((restoreLength != NULL) && (*restoreLength > 0) && (*restoreLength <= PIXEL_COUNT)){
        int x, y, counter;

        *restoreLength = (colMax - colMin + 1)*(rowMax - rowMin + 1);

        *restoreData = (BYTE*)malloc(sizeof(BYTE)*(*restoreLength));
        if(*restoreData == NULL){
            MessageBox(NULL, "Unable to malloc restore data", "Flood", MB_OK);
            *restoreLength = 0;
            return FALSE;
        }

        counter = 0;
        for(x = colMin; x <= colMax; x++){
            for(y = rowMin; y <= rowMax; y++){
                (*restoreData)[counter] = pixelFrame[PIXEL_2D_2_1D(x, y)];
                counter++;
            }
        }
        (*ptNW).x = colMin;
        (*ptNW).y = rowMin;

        (*ptSE).x = colMax;
        (*ptSE).y = rowMax;

    }



    // Draw all pixels
    for (i=0; i<PIXEL_COUNT; i++){
        if(pixelAdded[i] == 1){
            brushArgs.pixel = i;
            brushArgs.size = args->size;
            brushArgs.newColorCode = args->newColorCode;
            canvas_draw_brush(hdc, pixelFrame, &brushArgs);
        }
    }

    FLOOD_EXIT:

    return TRUE;

    
}
#endif
#define LOG_LINE_MAX 4
void log_message(char* message){
// static char szLogLines[LOG_LINE_MAX][80];
// static logWriteIndex, logReadIndex;

strcpy(szLogLines[logWriteIndex], message);

logWriteIndex = (logWriteIndex+1)%LOG_LINE_MAX;

if(logWriteIndex == logReadIndex){
    logReadIndex = (logReadIndex + 1)%LOG_LINE_MAX;
}

}

int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow){
    static char szNameApp[] = "CharacterCreator";
    HWND hwnd;
    MSG msg;
    WNDCLASS wndclass;

    if(!hPrevInstance){
        // Main window 
        wndclass.style = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc = WndProcMain;
        wndclass.cbClsExtra = 0;
        wndclass.cbWndExtra = 0;
        wndclass.hInstance = hInstance;
        wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hCursor = LoadCursor(NULL, IDC_CROSS);
        wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName = NULL;
        wndclass.lpszClassName = szNameApp;
        RegisterClass(&wndclass);

        // Child window - toolbar
        wndclass.lpfnWndProc = WndProcToolbar;
        wndclass.cbWndExtra = sizeof(WORD)*(TOOLBAR_EXTRA_WORDS);
        wndclass.hIcon = NULL;
        wndclass.lpszClassName = szNameToolbar;
        RegisterClass(&wndclass);

        // Child window - color pane
        wndclass.lpfnWndProc = WndProcColorBox;
        wndclass.cbWndExtra = sizeof(WORD)*(COLORBOX_EXTRA_WORDS);;
        wndclass.hIcon = NULL;
        wndclass.lpszClassName = szNameColorBox;
        RegisterClass(&wndclass);
        
        // Child window - canvas
        wndclass.lpfnWndProc = WndProcCanvas;
        wndclass.cbWndExtra = sizeof(WORD)*(CANVAS_EXTRA_WORDS);
        wndclass.hIcon = NULL;
        wndclass.lpszClassName = szNameCanvas;
        RegisterClass(&wndclass);
        
        // Child window - log
        wndclass.lpfnWndProc = WndProcLog;
        wndclass.cbWndExtra = sizeof(WORD)*(LOG_EXTRA_WORDS);;
        wndclass.hIcon = NULL;
        wndclass.lpszClassName = szNameLog;
        RegisterClass(&wndclass);
    }

    hwnd = CreateWindow(
        szNameApp,
        "Character Creator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while(GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}



long FAR PASCAL _export WndProcMain(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    static HWND hwndToolbar, hwndColorBox;
    static HWND hwndCanvas;
    static HWND hwndLog;
    static short cxBlock, cyBlock;
    static short canvasSize;
    static short counter;
    short x, y;
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    static short cxChar, cxCaps, cyChar;
    TEXTMETRIC tm;
    POINT lpMousePoint;
    //HBRUSH hBrush;
    static BYTE CTRL_WASD = CTRL_GROUP_CURSOR;
    static BYTE CTRL_HJKL = CTRL_GROUP_CURSOR;
    static BYTE CTRL_ARROWS = CTRL_GROUP_CURSOR;
    ToolbarTool_e toolbarTool;
    CanvasTool_e canvasTool;

    switch(message){
        case WM_CREATE:{
            counter = 0;
            hdc = GetDC(hwnd);
            GetTextMetrics(hdc, &tm);
            cxChar = tm.tmAveCharWidth;
            cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;
            cyChar = tm.tmHeight + tm.tmExternalLeading;
            ReleaseDC(hwnd, hdc);

            // wsprintf(szLogMessage, "Hello world");
            logWriteIndex = logReadIndex = 0;
            nLength = wsprintf (szBuffer, "Brush entry: %d.", sizeof(CanvasHistoryEntry_s) + 2*sizeof(CanvasAction_s) + sizeof(CanvasBrushArgs_s));
            log_message(szBuffer);

            hwndToolbar = CreateWindow(szNameToolbar, NULL, WS_CHILDWINDOW | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        hwnd, CHILD_ID_TOOLBAR, GetWindowWord(hwnd, GWW_HINSTANCE), NULL);

            hwndColorBox = CreateWindow(szNameColorBox, NULL, WS_CHILD | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        hwnd, CHILD_ID_COLORBOX, GetWindowWord(hwnd, GWW_HINSTANCE), NULL);


            hwndCanvas = CreateWindow(szNameCanvas, NULL, WS_CHILDWINDOW | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        hwnd, CHILD_ID_CANVAS, GetWindowWord(hwnd, GWW_HINSTANCE), NULL);

            hwndLog = CreateWindow(szNameLog, NULL, WS_CHILDWINDOW | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        hwnd, CHILD_ID_LOG, GetWindowWord(hwnd, GWW_HINSTANCE), NULL);

            debugWindow = hwndLog;
            
            return 0;
            }
        case WM_SIZE:{
            cxBlock = LOWORD(lParam) / 6;
            cyBlock = HIWORD(lParam) / 6;
            canvasSize = min( ALIGN(4*cxBlock, 32), ALIGN(5*cyBlock, 32));

            MoveWindow(hwndColorBox, 5*cxBlock, 0, cxBlock, cyBlock, TRUE);
            MoveWindow(hwndToolbar, 0, 0, cxBlock, 3*cyBlock, TRUE);
            MoveWindow(hwndCanvas, 
                    cxBlock + max(0, (4*cxBlock - canvasSize)/2), 
                    max(0, (5*cyBlock - canvasSize)/2), 
                    canvasSize, canvasSize, TRUE);
            MoveWindow(hwndLog, cxBlock, 5*cyBlock, 4*cxBlock, cyBlock, TRUE);

            lpMousePoint.x = cxBlock;
            lpMousePoint.y = 0;
            ClientToScreen(hwnd, &lpMousePoint);

            SetCursorPos(lpMousePoint.x, lpMousePoint.y);

            // nLength = wsprintf(szBuffer, "Handle %d", hwnd);
            // MessageBox(hwnd, szBuffer, "Main", MB_OK);
            return 0;
        }  
        case WM_KEYDOWN:{    
       		 debugWindow = hwnd;
            switch(wParam){
                case VK_SPACE:
                    GetCursorPos(&lpMousePoint);
                    // nLength = wsprintf(szBuffer, "x %d, y %d", lpMousePoint.x, lpMousePoint.y);
                    // MessageBox(debugWindow, szBuffer, "Current", MB_OK);
                    ScreenToClient(hwndCanvas, &lpMousePoint);
                    SendMessage(hwndCanvas, WM_LBUTTONDOWN, 0, MAKELONG(lpMousePoint.x, lpMousePoint.y));
                    break;

                case VK_LEFT:
                    
                    if((WORD)CTRL_ARROWS == (WORD)CTRL_GROUP_CURSOR){
                        move_cursor(-1 * canvasSize/CANVAS_DIM, 0);
                    }
                    break;

                case 'A':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor(-1 * canvasSize/CANVAS_DIM, 0);
                    }
                    break;

                case 'H':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor(-1*canvasSize/CANVAS_DIM, 0);
                    }
                    break; 


                case VK_RIGHT:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        move_cursor(canvasSize/CANVAS_DIM, 0);  
                    }
                    break;
                
                case 'D':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor(canvasSize/CANVAS_DIM, 0); 
                    }
                    break; 

                case 'L':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor(canvasSize/CANVAS_DIM, 0);
                    }
                    break; 


                case VK_UP:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        move_cursor(0, -1*canvasSize/CANVAS_DIM); 
                    }
                    break;
                
                case 'W':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor(0, -1*canvasSize/CANVAS_DIM);
                    }
                    break; 

                case 'K':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor(0, -1*canvasSize/CANVAS_DIM);
                    }
                    break; 


                case VK_DOWN:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        move_cursor(0, canvasSize/CANVAS_DIM);
                    }
                    break;

                case 'S':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor(0, canvasSize/CANVAS_DIM);                    
                    }
                    break;

                case 'J':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor(0, canvasSize/CANVAS_DIM);                    
                    }
                    break; 
            }
            return 0;
        }

        case WM_PAINT:{
            // nLength = wsprintf(szBuffer, "cxBlock %d, cyBlock %d, canvasSize %d", cxBlock, cyBlock, canvasSize);
            hdc = BeginPaint(hwnd, &ps);
            // TextOut(hdc, 10, cyBlock<<2, szBuffer, nLength);
            GetClientRect(hwnd, &rect);   

            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

            for(x=0; x<6; x++){
                for(y=0; y<6; y++){
                    Rectangle(hdc, rect.left + x*cxBlock, rect.top + y*cyBlock, 
                                    rect.left + x*cxBlock +cxBlock, rect.top + y*cyBlock + cyBlock);
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_COMMAND:{
            // nLength = wsprintf (szBuffer, "wParam: %d, lParam %ld.", wParam, lParam);

            // MessageBox(hwnd, szBuffer, "IconEdit", MB_OK);
            if(wParam == CHILD_ID_COLORBOX){
                SetWindowWord(hwndCanvas, CanvasWordForeColor, LOWORD(lParam));
                SetWindowWord(hwndCanvas, CanvasWordBackColor, HIWORD(lParam));
            }else if(wParam == CHILD_ID_TOOLBAR){
                toolbarTool = (ToolbarTool_e) LOWORD(lParam);
                switch(toolbarTool){
                    case ToolbarToolUndo:
                        if((canvasHistory[canvasHistoryReadIndex] != NULL) && (canvasHistory[canvasHistoryReadIndex]->valid == TRUE)){                            
                            SendMessage(hwndCanvas, WM_COMMAND, 0, MAKELONG(canvasHistoryReadIndex, 0));
                            canvasHistoryWriteIndex = canvasHistoryReadIndex;
                            canvasHistoryReadIndex = canvasHistoryReadIndex > 0 ? (canvasHistoryReadIndex - 1) : (CANVAS_HISTORY_LEN-1);

                        }
                        break;
                    case ToolbarToolRedo:
                        if((canvasHistory[canvasHistoryWriteIndex] != NULL) && (canvasHistory[canvasHistoryWriteIndex]->valid == TRUE)){
                            SendMessage(hwndCanvas, WM_COMMAND, 0, MAKELONG(canvasHistoryWriteIndex, 1));
                            canvasHistoryReadIndex = canvasHistoryWriteIndex;
                            canvasHistoryWriteIndex = (canvasHistoryWriteIndex + 1)%CANVAS_HISTORY_LEN;                           
                        }
                        
                        break;
                    default:
                        map_toolbar_tool_to_canvas(&canvasTool, toolbarTool);
                        SetWindowWord(hwndCanvas, CanvasWordTool, (WORD)canvasTool);
                        break;
                }
                

            }else if(wParam == CHILD_ID_CANVAS){
                InvalidateRect(hwndLog, NULL, FALSE);
            }
            return 0;
        }

        case WM_SETFOCUS:
            ShowCursor(TRUE);
            return 0;

        case WM_KILLFOCUS:
            ShowCursor(FALSE);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


long FAR PASCAL _export WndProcToolbar(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect, rectText;
    static short cxBlock, cyBlock;
    short x, y, col, row;
    char cBuffer[16] ;
    short nLength ;
    static WORD selectedTool, activeTool;
    HWND hwndParent;

    switch(message){
        case WM_CREATE:
            SetWindowWord(hwnd, ToolbarWordTool, ToolbarToolBrush);
            selectedTool = ToolbarToolBrush;  
            activeTool = selectedTool;     
            return 0;
        
        case WM_SIZE:
            cxBlock = LOWORD(lParam) / TOOLBAR_COLS;
            cyBlock = HIWORD(lParam) / TOOLBAR_ROWS;
            return 0;

        case WM_LBUTTONDOWN:{
            x = LOWORD(lParam);
            y = HIWORD(lParam);

            col = x/cxBlock;
            row = y/cyBlock;
            selectedTool = col + row*TOOLBAR_COLS;

            if( (col >= TOOLBAR_COLS) || (col < 0) || (row >= TOOLBAR_ROWS) || (row < 0)){
                return 0;
            }

            hwndParent = GetParent(hwnd);

            // nLength = wsprintf(szBuffer, "ID %d, code %d, to %d", CHILD_ID_COLORBOX, activeColorCode, hwndParent);
            // MessageBox(hwnd, szBuffer, "ColorBox", MB_OK);

            SendMessage(hwndParent, WM_COMMAND, CHILD_ID_TOOLBAR, selectedTool);
            // MessageBeep(1);
            InvalidateRect(hwnd, NULL, FALSE);

            return 0;
        }

        case WM_PAINT:{
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);   

            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);


            for(x=0; x<TOOLBAR_COLS; x++){
                for(y=0; y<TOOLBAR_ROWS; y++){
                    nLength = wsprintf (cBuffer, "%s", toolbarToolNames[min(x + y*TOOLBAR_COLS, ToolbarToolMAX)]);

                    rectText.left = rect.left + x*cxBlock;
                    rectText.top = rect.top + y*cyBlock;
                    rectText.right = rect.left + x*cxBlock +cxBlock;
                    rectText.bottom = rect.top + y*cyBlock + cyBlock;


                    Rectangle(hdc, rectText.left, rectText.top, 
                                    rectText.right, rectText.bottom);
                    DrawText (hdc, cBuffer, -1, &rectText, DT_CENTER | DT_NOCLIP) ;
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


long FAR PASCAL _export WndProcColorBox(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect, clientRect;
    static short cxBlock, cyBlock;
    short x, y;
    short colorRow, colorCol;
    int colorCode;
    HWND hwndParent;
    HBRUSH hBrush;
    COLORREF colorRef;
    static WORD activeColorCode;

    switch(message){
        case WM_CREATE:
            SetWindowWord(hwnd, ColorBoxWordForeColor, PixelColorCodeBlack); 
            SetWindowWord(hwnd, ColorBoxWordBackColor, PixelColorCodeWhite);            
            return 0;
        
        case WM_SIZE:
            cxBlock = LOWORD(lParam) / COLORBOX_COLS;
            cyBlock = HIWORD(lParam) / COLORBOX_ROWS;
            return 0;

        case WM_PAINT:{
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &clientRect);   

            // Outer border
            Rectangle(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

            for(x=0; x<COLORBOX_COLS; x++){
                for(y=0; y<COLORBOX_ROWS; y++){

                    colorCode = x + y*COLORBOX_COLS;
                    pixel_color_code_to_rgb(colorCode, &colorRef);
                    hBrush = CreateSolidBrush(colorRef);

                    rect.left = clientRect.left + x*cxBlock + 1;
                    rect.top  = clientRect.top + y*cyBlock + 1;
                    rect.right = clientRect.left + x*cxBlock + cxBlock - 2; 
                    rect.bottom = clientRect.top + y*cyBlock + cyBlock - 2;
                    FillRect(hdc, &rect, hBrush);

                    if((WORD)colorCode == activeColorCode){
                        rect.left -= 1;
                        rect.top  -= 1;
                        rect.right += 1; 
                        rect.bottom += 1;
                        // SelectObject(hdc, GetStockObject(BLACK_BRUSH));
                        // Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
                        // SelectObject(hdc, GetStockObject(WHITE_BRUSH));
                        DrawFocusRect(hdc, &rect);
                    }

                    
                    DeleteObject(hBrush);
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:{
            x = LOWORD(lParam);
            y = HIWORD(lParam);

            colorCol = x/cxBlock;
            colorRow = y/cyBlock;
            activeColorCode = colorCol + colorRow*COLORBOX_COLS;

            if( (colorCol >= COLORBOX_COLS) || (colorCol < 0) || (colorRow >= COLORBOX_ROWS) || (colorRow < 0)){
                return 0;
            }

            hwndParent = GetParent(hwnd);

            // nLength = wsprintf(szBuffer, "ID %d, code %d, to %d", CHILD_ID_COLORBOX, activeColorCode, hwndParent);
            // MessageBox(hwnd, szBuffer, "ColorBox", MB_OK);

            SendMessage(hwndParent, WM_COMMAND, CHILD_ID_COLORBOX, activeColorCode);
            // MessageBeep(1);
            InvalidateRect(hwnd, NULL, FALSE);

            return 0;
        }
        
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


long FAR PASCAL _export WndProcCanvas(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    static short cxBlock, cyBlock;
    int x, y, pixel, pixRow, pixCol;
    static HPEN hPen;
    static short xSel, ySel;
    HBRUSH hBrush;
    POINT lpPoint;
    BYTE colorCode;
    // WORD activeColorForeground;
    COLORREF newColor;
    static BYTE pixelFrame[PIXEL_COUNT];
    static WORD drawState;
    static POINT ptPixelDraw1, ptPixelDraw2;
    HWND hwndParent;


    switch(message){
        case WM_CREATE:{
            drawState = DRAW_STATE_START;
            SetWindowWord(hwnd, CanvasWordForeColor, PixelColorCodeBlack);
            SetWindowWord(hwnd, CanvasWordBackColor, PixelColorCodeWhite); 
            SetWindowWord(hwnd, CanvasWordTool, CanvasToolBrush); 

            for(x=0; x<PIXEL_COUNT; x++){ 
                pixelFrame[x] = (BYTE)PixelColorCodeWhite;
            }
            hPen = CreatePen(PS_SOLID, 1, COLOR_SILVER);

            canvasHistoryWriteIndex = canvasHistoryReadIndex = 0;
            return 0;
        }
        case WM_SIZE:{
            cxBlock = LOWORD(lParam) / CANVAS_DIM;
            cyBlock = HIWORD(lParam) / CANVAS_DIM;
            pixW = cxBlock;
            pixH = cyBlock;
            return 0;
        }
        
        case WM_COMMAND:{
            CanvasAction_s* action;

            hdc = GetDC(hwnd);
            switch(HIWORD(lParam)){
                case 0: //undo
                    action = canvasHistory[LOWORD(lParam)]->prevAction;
                    break;
                case 1: //redo
                    action = canvasHistory[LOWORD(lParam)]->nextAction;
                    break;
                default:
                    break;
            }
            switch(action->tool){
                case CanvasToolBrush:
                    canvas_draw_brush(&hdc, pixelFrame, action->args);
                    ValidateRect(hwnd, NULL);
                    break;
                case CanvasToolLine:
                    canvas_draw_line(&hdc, pixelFrame, action->args, NULL, NULL);
                    ValidateRect(hwnd, NULL);
                    break;
                case CanvasToolRect:
                    canvas_draw_rect(&hdc, pixelFrame, action->args, NULL, NULL);
                    ValidateRect(hwnd, NULL);
                    break;
                case CanvasToolFlood:
                    canvas_draw_flood_v7(&hdc, pixelFrame, action->args, NULL, NULL, NULL, NULL);
                    ValidateRect(hwnd, NULL);
                    break;
                // case CanvasToolErase:{//TODO make function for this
                //     int x;
                //     for(x=0; x<PIXEL_COUNT; x++){
                //         pixelFrame[x] = (BYTE)PixelColorCodeWhite;
                //     }
                //     InvalidateRect(hwnd, NULL, FALSE);
                //     break;
                // }
                case CanvasToolRestore:
                    canvas_restore_rect(&hdc, pixelFrame, action->args);
                    ValidateRect(hwnd, NULL);
                    break;
                default:
                    MessageBox(NULL, "Unsupported history tool", "Canvas", MB_OK);
                    break;
            }
            ReleaseDC(hwnd, hdc); 
            break;
        }

        case WM_LBUTTONDOWN:{
            x = LOWORD(lParam);
            y = HIWORD(lParam);

            pixCol = x/cxBlock;
            pixRow = y/cyBlock;
            pixel = pixCol + pixRow*CANVAS_DIM;

            if( (pixCol >= CANVAS_DIM) || (pixCol < 0) || (pixRow >= CANVAS_DIM) || (pixRow < 0)){
                return 0;
            }

            hdc = GetDC(hwnd);

            switch ((BYTE)GetWindowWord(hwnd, CanvasWordTool)){
                case CanvasToolBrush:{
                    CanvasBrushArgs_s* brushDoArgs = NULL;
                    CanvasBrushArgs_s* brushUndoArgs = NULL;

                    if(FALSE == create_history_entry(CanvasToolBrush)){
                        MessageBox(NULL, "Unable to create history entry", "Brush", MB_OK);
                        break;
                    }   

                    brushDoArgs = (CanvasBrushArgs_s*)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
                    brushUndoArgs = (CanvasBrushArgs_s*)canvasHistory[canvasHistoryReadIndex]->prevAction->args;                    

                    brushDoArgs->pixel = pixel;
                    brushDoArgs->size = cxBlock;
                    brushDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);

                    brushUndoArgs->pixel = brushDoArgs->pixel;
                    brushUndoArgs->size = brushDoArgs->size;                    
                    brushUndoArgs->newColorCode = canvas_draw_brush(&hdc, pixelFrame, brushDoArgs);
                    ValidateRect(hwnd, NULL);

                    break;
                }

                case CanvasToolLine:{
                    switch (drawState){
                        case DRAW_STATE_START:
                            drawState = DRAW_LINE_FIRST;
                            ptPixelDraw1.x = pixCol;
                            ptPixelDraw1.y = pixRow;
                            break;
                        case DRAW_LINE_FIRST:{
                            CanvasLineArgs_s* lineDoArgs;
                            CanvasRestoreArgs_s* lineUndoArgs;


                            ptPixelDraw2.x = pixCol;
                            ptPixelDraw2.y = pixRow;

                            if(FALSE == create_history_entry(CanvasToolLine)){
                                MessageBox(NULL, "Unable to create history entry", "Line", MB_OK);
                                break;
                            }  
                            
                            lineDoArgs = (CanvasLineArgs_s*)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
                            lineUndoArgs = (CanvasRestoreArgs_s*)canvasHistory[canvasHistoryReadIndex]->prevAction->args; 

                            lineDoArgs->size = cxBlock;
                            lineDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                            lineDoArgs->pt1 = ptPixelDraw1;
                            lineDoArgs->pt2 = ptPixelDraw2;   

                            lineUndoArgs->dataLength = PIXEL_COUNT;
                            if(FALSE == canvas_draw_line(&hdc, pixelFrame, lineDoArgs, &(lineUndoArgs->dataLength), &(lineUndoArgs->colorData))){
                                free_history_entry(canvasHistoryReadIndex);
                                drawState = DRAW_STATE_START;
                                MessageBox(NULL, "Draw line failed", "Line", MB_OK);
                                break;
                            }

                            lineUndoArgs->size = cxBlock;
                            lineUndoArgs->ptNW.x = min(lineDoArgs->pt1.x, lineDoArgs->pt2.x);
                            lineUndoArgs->ptSE.x = max(lineDoArgs->pt1.x, lineDoArgs->pt2.x);
                            lineUndoArgs->ptNW.y = min(lineDoArgs->pt1.y, lineDoArgs->pt2.y);
                            lineUndoArgs->ptSE.y = max(lineDoArgs->pt1.y, lineDoArgs->pt2.y);

                            
                            drawState = DRAW_STATE_START;
                            break;   
                        }
                    }                         
                    
                    ValidateRect(hwnd, NULL);
                    break;
                    }

                case CanvasToolRect:{
                    switch (drawState){
                        case DRAW_STATE_START:
                            drawState = DRAW_LINE_FIRST;
                            ptPixelDraw1.x = pixCol;
                            ptPixelDraw1.y = pixRow;
                            break;
                        case DRAW_LINE_FIRST:{
                            CanvasRectArgs_s* rectDoArgs;
                            CanvasRestoreArgs_s* rectUndoArgs;

                            ptPixelDraw2.x = pixCol;
                            ptPixelDraw2.y = pixRow;

                            if(FALSE == create_history_entry(CanvasToolRect)){
                                MessageBox(NULL, "Unable to create history entry", "Rect", MB_OK);
                                break;
                            } 

                            rectDoArgs = (CanvasRectArgs_s*)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
                            rectUndoArgs = (CanvasRestoreArgs_s*)canvasHistory[canvasHistoryReadIndex]->prevAction->args; 

                            rectDoArgs->size = cxBlock;
                            rectDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                            rectDoArgs->pt1 = ptPixelDraw1;
                            rectDoArgs->pt2 = ptPixelDraw2;

                            rectUndoArgs->dataLength = PIXEL_COUNT;
                            if(FALSE == canvas_draw_rect(&hdc, pixelFrame, rectDoArgs, &(rectUndoArgs->dataLength), &(rectUndoArgs->colorData))){
                                free_history_entry(canvasHistoryReadIndex);
                                drawState = DRAW_STATE_START;
                                MessageBox(NULL, "Draw rect failed", "Rect", MB_OK);
                                break;
                            }

                            rectUndoArgs->size = cxBlock;
                            rectUndoArgs->ptNW.x = min(rectDoArgs->pt1.x, rectDoArgs->pt2.x);
                            rectUndoArgs->ptSE.x = max(rectDoArgs->pt1.x, rectDoArgs->pt2.x);
                            rectUndoArgs->ptNW.y = min(rectDoArgs->pt1.y, rectDoArgs->pt2.y);
                            rectUndoArgs->ptSE.y = max(rectDoArgs->pt1.y, rectDoArgs->pt2.y);

                            drawState = DRAW_STATE_START;
                            break;   
                        }                         
                    }
                    ValidateRect(hwnd, NULL);
                    break;
                }

                case CanvasToolFlood:{
                    CanvasFloodArgs_s* floodDoArgs;
                    CanvasRestoreArgs_s* floodUndoArgs;

                    if(pixelFrame[pixel] == (BYTE)GetWindowWord(hwnd, CanvasWordForeColor)){
                        break; //skip, already equal to target color
                    }

                    if(FALSE == create_history_entry(CanvasToolFlood)){
                        MessageBox(NULL, "Unable to create history entry", "Flood", MB_OK);
                        break;
                    }  

                    floodDoArgs = (CanvasFloodArgs_s*)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
                    floodUndoArgs = (CanvasRestoreArgs_s*)canvasHistory[canvasHistoryReadIndex]->prevAction->args; 
                    
                    lpPoint.x = pixCol;
                    lpPoint.y = pixRow;

                    floodDoArgs->pixel = pixel;
                    floodDoArgs->size = cxBlock;
                    floodDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                    floodDoArgs->targetColorCode = pixelFrame[pixel];

                    floodUndoArgs->dataLength = PIXEL_COUNT;
                    if(FALSE == canvas_draw_flood_v7(&hdc, pixelFrame, floodDoArgs, &(floodUndoArgs->dataLength), &(floodUndoArgs->colorData), &(floodUndoArgs->ptNW), &(floodUndoArgs->ptSE))){
                        free_history_entry(canvasHistoryReadIndex);
                        MessageBox(NULL, "Flood failed", "Flood", MB_OK);
                        break;
                    }

                    floodUndoArgs->size = cxBlock;
                    // nLength = wsprintf (szBuffer, "Flood undo args. Length %d.", floodUndoArgs->dataLength);
                    // MessageBox(NULL, szBuffer, "Flood", MB_OK);


                    ValidateRect(hwnd, NULL);
                    break;    
                }                        
                    
                case CanvasToolErase:{
                    CanvasRectArgs_s* eraseDoArgs;
                    CanvasRestoreArgs_s* eraseUndoArgs;
                    //Treating erase as a rectangle draw so it will work better if/when I add editing only a section of the canvas
                    
                    if(FALSE == create_history_entry(CanvasToolRect)){
                        MessageBox(NULL, "Unable to create history entry", "Erase", MB_OK);
                        break;
                    }

                    eraseDoArgs = (CanvasRectArgs_s*)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
                    eraseUndoArgs = (CanvasRestoreArgs_s*)canvasHistory[canvasHistoryReadIndex]->prevAction->args; 

                    eraseDoArgs->size = cxBlock;
                    eraseDoArgs->newColorCode = PixelColorCodeWhite;
                    eraseDoArgs->pt1.x = 0;
                    eraseDoArgs->pt1.y = 0;
                    eraseDoArgs->pt2.x = CANVAS_DIM-1;
                    eraseDoArgs->pt2.y = CANVAS_DIM-1;

                    eraseUndoArgs->dataLength = PIXEL_COUNT;
                    if(FALSE == canvas_draw_rect(&hdc, pixelFrame, eraseDoArgs, &(eraseUndoArgs->dataLength), &(eraseUndoArgs->colorData))){
                                free_history_entry(canvasHistoryReadIndex);
                                drawState = DRAW_STATE_START;
                                MessageBox(NULL, "Erase failed", "Erase", MB_OK);
                                break;
                    }

                    eraseUndoArgs->size = cxBlock;
                    eraseUndoArgs->ptNW.x = min(eraseDoArgs->pt1.x, eraseDoArgs->pt2.x);
                    eraseUndoArgs->ptSE.x = max(eraseDoArgs->pt1.x, eraseDoArgs->pt2.x);
                    eraseUndoArgs->ptNW.y = min(eraseDoArgs->pt1.y, eraseDoArgs->pt2.y);
                    eraseUndoArgs->ptSE.y = max(eraseDoArgs->pt1.y, eraseDoArgs->pt2.y);
                    
                    ValidateRect(hwnd, NULL);
                    // InvalidateRect(hwnd, NULL, FALSE);
                    break;
                }

                default:
                    MessageBeep(0);
                    ValidateRect(hwnd, NULL);
                    break; //TODO signal error here
            }
            
            wsprintf(szBuffer, "Write %d, read %d", canvasHistoryWriteIndex, canvasHistoryReadIndex);
            log_message(szBuffer);
            hwndParent = GetParent(hwnd);
            SendMessage(hwndParent, WM_COMMAND, CHILD_ID_CANVAS, 0);

            // Cleanup
            ReleaseDC(hwnd, hdc);    
            return 0;
        }

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            SelectObject(hdc, hPen);

            for(x=0; x<CANVAS_DIM; x++){
                for(y=0; y<CANVAS_DIM; y++){
                    Rectangle(hdc, rect.left + x*cxBlock, rect.top + y*cyBlock, 
                                    rect.left + x*cxBlock +cxBlock, rect.top + y*cyBlock + cyBlock);
                }
            }

            for(pixel=0; pixel<PIXEL_COUNT; pixel++){
                x = pixel % 32;
                y = pixel / 32;

                colorCode = pixelFrame[pixel];

                if(FALSE == pixel_color_code_to_rgb(colorCode, &newColor)){
                    MessageBeep(1);
                    newColor = COLOR_BLACK; //TODO signal error w/ dialog box w/ option to quit
                }

                hBrush = CreateSolidBrush(newColor);

                rect.left = x*cxBlock + 1;
                rect.top = y*cyBlock + 1;
                rect.right = rect.left + cxBlock - 2 ;
                rect.bottom = rect.top + cyBlock - 2;

                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);
            }

            EndPaint(hwnd, &ps);
            return 0;

        case WM_DESTROY:{
            int x;
            for(x=0; x<CANVAS_HISTORY_LEN; x++){
                free_history_entry(x);
            }
            DeleteObject(hPen);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Much of the scroll text window code is from Charles Petzold's SYSMETS3.C (pg 79)
long FAR PASCAL _export WndProcLog(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    HDC hdc;
    PAINTSTRUCT ps;
    static short cxChar, cxCaps, cyChar, cxClient, cyClient, nMaxWidth, nVscrollPos, nVscrollMax, nHscrollPos, nHscrollMax;
    short i, x, y, nVscrollInc, nHscrollInc;
    TEXTMETRIC tm;

    switch(message){
        case WM_CREATE:
            for (x=0; x<LOG_EXTRA_WORDS; x+=2){
                SetWindowWord(hwnd, x, 0);
            }

            hdc = GetDC(hwnd);

            GetTextMetrics(hdc, &tm);
            cxChar = tm.tmAveCharWidth;
            cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2)*cxChar/2;
            cyChar = tm.tmHeight + tm.tmExternalLeading;

            ReleaseDC(hwnd, hdc);

            nMaxWidth = 80 * cxChar;
            return 0;

        case WM_SIZE:
            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);

            nVscrollMax = max(0, cyClient/cyChar);
            nVscrollPos = min(nVscrollPos, nVscrollMax);

            // SetScrollRange(hwnd, SB_VERT, 0, nVscrollMax, FALSE);
            // SetScrollPos(hwnd, SB_VERT, nVscrollPos, TRUE);

            nHscrollMax = max(0, 2 + (nMaxWidth - cxClient) / cxChar);
            nHscrollPos = min(nHscrollPos, nHscrollMax);

            // SetScrollRange(hwnd, SB_HORZ, 0, nHscrollMax, FALSE);
            // SetScrollPos(hwnd, SB_HORZ, nHscrollPos, TRUE);

            return 0;

        case WM_VSCROLL:
            switch(wParam){
                case SB_TOP:
                    nVscrollInc = -nVscrollPos;
                    break;

                case SB_BOTTOM:
                    nVscrollInc = nVscrollMax - nVscrollPos;
                    break;

                case SB_LINEUP:
                    nVscrollInc = -1;
                    break;

                case SB_LINEDOWN:
                    nVscrollInc = 1;
                    break;

                case SB_PAGEUP:
                    nVscrollInc = min(-1, -cyClient/cyChar);
                    break;

                case SB_PAGEDOWN:
                    nVscrollInc = max(1, cyClient/cyChar);
                    break;

                case SB_THUMBTRACK:
                    nVscrollInc = LOWORD(lParam) - nVscrollPos;
                    break;

                default:
                    nVscrollInc = 0;
                    break;
            }
            nVscrollInc = max(-nVscrollPos, min(nVscrollInc, nVscrollMax - nVscrollPos));
            if(nVscrollInc != 0){
                nVscrollPos += nVscrollInc;
                ScrollWindow(hwnd, 0, -cyChar*nVscrollInc, NULL, NULL);
                SetScrollPos(hwnd, SB_VERT, nVscrollPos, TRUE);
                UpdateWindow(hwnd);
            }
            return 0;

        case WM_HSCROLL:
            switch(wParam){
                case SB_LINEUP:
                    nHscrollInc = -1;
                    break;

                case SB_LINEDOWN:
                    nHscrollInc = 1;
                    break;

                case SB_PAGEUP:
                    nHscrollInc = -8;
                    break;

                case SB_PAGEDOWN:
                    nHscrollInc = 8;
                    break;

                case SB_THUMBPOSITION:
                    nHscrollInc = LOWORD(lParam) - nHscrollPos;
                    break;

                default:
                    nHscrollInc = 0;
                    break;
            }
            nHscrollInc = max(-nHscrollPos, min(nHscrollInc, nHscrollMax - nHscrollPos));
            if(nHscrollInc != 0){
                nHscrollPos += nHscrollInc;
                ScrollWindow(hwnd, -cxChar*nHscrollInc, 0, NULL, NULL);
                SetScrollPos(hwnd, SB_HORZ, nHscrollPos, TRUE);
                UpdateWindow(hwnd);
            }
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            // nPaintBegin = max(0, nVscrollPos);// + ps.rcPaint.top / cyChar - 1);
            // nPaintEnd = min(LOG_LINE_MAX, nVscrollPos+1);// + ps.rcPaint.bottom / cyChar);

            for(i=logReadIndex; ((i%LOG_LINE_MAX)!=logWriteIndex) && ((i-logReadIndex + nVscrollPos) <  nVscrollMax); i++){
                x = cxChar * (1-nHscrollPos);
                y = cyChar * (1-nVscrollPos+i-logReadIndex);

                TextOut(hdc, x, y, szLogLines[i%LOG_LINE_MAX], lstrlen(szLogLines[i%LOG_LINE_MAX]));
            }
            
            // Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
            // MoveTo(hdc, rect.left, rect.top);
            // LineTo(hdc, rect.right, rect.bottom);
            // MoveTo(hdc, rect.left, rect.bottom);
            // LineTo(hdc, rect.right, rect.top);

            
            // DrawText(hdc, szLogMessage, -1, &rect, 
            //             DT_SINGLELINE);
            EndPaint(hwnd, &ps);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}