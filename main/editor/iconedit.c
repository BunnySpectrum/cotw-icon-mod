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

            undoAction->tool = CanvasToolLine; //change to rect bit replace
            undoArgs = malloc(sizeof(CanvasLineArgs_s)); 
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
BOOL canvas_draw_line(HDC* hdc, BYTE* pixelFrame, CanvasLineArgs_s* args){
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


BOOL canvas_draw_rect(HDC* hdc, BYTE* pixelFrame, CanvasRectArgs_s* args){
    short pixelX, pixelY, pxLeftCol, pxRightCol, pxTopRow, pxBotRow;
    CanvasBrushArgs_s brushArgs;

    pxLeftCol = min(args->pt1.x, args->pt2.x);
    pxRightCol = max(args->pt1.x, args->pt2.x);    
    pxTopRow = min(args->pt1.y, args->pt2.y);
    pxBotRow = max(args->pt1.y, args->pt2.y);    

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
    BYTE oldColorCode;
    short counter;
    CanvasBrushArgs_s brushArgs;

    pxLeftCol = min(args->pt1.x, args->pt2.x);
    pxRightCol = max(args->pt1.x, args->pt2.x);    
    pxTopRow = min(args->pt1.y, args->pt2.y);
    pxBotRow = max(args->pt1.y, args->pt2.y);    

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

#if FLOOD_VER == 1
BOOL canvas_draw_flood_v1(HWND hwnd, HDC* hdc, BYTE* pixelFrame, int pixel, POINT* ptClick, short width, short height, PixelColorCode_e targetColorCode){
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
    

    if((pixelCol > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol-1, pixelRow)])){
        canvas_draw_flood_v1(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol-1, pixelRow), &clickPoint, width, height, targetColorCode);
    }
    if((pixelCol < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol+1, pixelRow)])){
        canvas_draw_flood_v1(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol+1, pixelRow), &clickPoint, width, height, targetColorCode);
    }

    if((pixelRow > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow-1)])){
        canvas_draw_flood_v1(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow-1), &clickPoint, width, height, targetColorCode);    
    }

    if((pixelRow < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow+1)])){
        canvas_draw_flood_v1(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow+1), &clickPoint, width, height, targetColorCode);    
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

#if FLOOD_VER == 2
BOOL canvas_draw_flood_v2(HWND hwnd, HDC* hdc, BYTE* pixelFrame, int pixel, POINT* ptClick, short width, short height, PixelColorCode_e targetColorCode){
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
    
    // Left
    if((pixelCol > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol-1, pixelRow)])){
        canvas_draw_flood_v2(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol-1, pixelRow), &clickPoint, width, height, targetColorCode);
    }

    // Up
    if((pixelRow > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow-1)])){
        canvas_draw_flood_v2(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow-1), &clickPoint, width, height, targetColorCode);    
    }

    // Right
    if((pixelCol < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol+1, pixelRow)])){
        canvas_draw_flood_v2(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol+1, pixelRow), &clickPoint, width, height, targetColorCode);
    }

    // Down
    if((pixelRow < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow+1)])){
        canvas_draw_flood_v2(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow+1), &clickPoint, width, height, targetColorCode);    
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

#if FLOOD_VER == 3
// SP start 0x86A6
// #pragma check_stack(off)
BOOL canvas_draw_flood_v3(BYTE* pixelFrame, int pixel, PixelColorCode_e targetColorCode){
    // SP-2: push BP
    // SP-6: chkstk moved by  to account for local variables
    // SP-2: push SI
    // SP-2: push DI
    short pixelRow, pixelCol;

    callDepthCurrent++;

    pixelRow = PIXEL_1D_2_ROW(pixel);
    pixelCol = PIXEL_1D_2_COL(pixel);    

    if (pixelFrame[pixel] != targetColorCode){
        goto FLOOD_EXIT;
    }else{
        canvas_draw_brush(floodHWND, &floodHDC, pixelFrame, pixel, pixW, pixH);
        // nLength = wsprintf(szBuffer, "Depth %d, Row %d, Col %d.", callDepthCurrent, pixelRow, pixelCol);
        // MessageBox(hwnd, szBuffer, "Fill", MB_OK);
    }

    if(callDepthCurrent >= CALL_DEPTH_LIMIT){
        goto FLOOD_EXIT;
    }
    
    // Left
    if((pixelCol > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol-1, pixelRow)])){
        canvas_draw_flood_v3(pixelFrame, PIXEL_2D_2_1D(pixelCol-1, pixelRow), targetColorCode);
    }

    // Up
    if((pixelRow > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow-1)])){
        canvas_draw_flood_v3(pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow-1), targetColorCode);    
    }

    // Right
    if((pixelCol < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol+1, pixelRow)])){
        canvas_draw_flood_v3(pixelFrame, PIXEL_2D_2_1D(pixelCol+1, pixelRow), targetColorCode);
    }

    // Down
    if((pixelRow < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow+1)])){
        canvas_draw_flood_v3(pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow+1), targetColorCode);    
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
// #pragma check_stack(on)
#endif

void pixel_num_to_bitfield(int pixel, short* byteNum, short* byteOffset){
    *byteNum = pixel / 8;
    *byteOffset = pixel % 8;
}

#if FLOOD_VER == 4
//Bitfield
BOOL canvas_draw_flood_v4(BYTE* pixelFrame, int pixel, PixelColorCode_e targetColorCode){
    BYTE pixelFlags[PIXEL_BQUEUE_LEN];
    short pixelRow, pixelCol;
    short i, j;
    short pxQByte, pxQOffset, pxQSet;
    int checkPixel;

    if (pixelFrame[pixel] != targetColorCode){
        //bail early for flooding shape with same color
        goto FLOOD_EXIT; 
    }

    // memset(pixelQueue, 0, sizeof(pixelQueue));
    for(i=0; i<PIXEL_BQUEUE_LEN; i++){
        pixelFlags[i] = 0;
    }


    //set bit for starting pixel
    // pixel_num_to_bitfield(pixel, &pxQByte, &pxQOffset);
    // pixelQueue[pxQByte] |= (1 << pxQOffset);


    do{
        // for breaking infinite loop error
        callDepthCurrent++;
        if(callDepthCurrent > CALL_DEPTH_LIMIT){
            goto FLOOD_EXIT;
        }

        // Fill pixel
        pixelRow = PIXEL_1D_2_ROW(pixel);
        pixelCol = PIXEL_1D_2_COL(pixel);    
        canvas_draw_brush(floodHWND, &floodHDC, pixelFrame, pixel, pixW, pixH);

        // Clear bit in queue
        pixel_num_to_bitfield(pixel, &pxQByte, &pxQOffset);
        pixelFlags[pxQByte] &= ~(1 << pxQOffset);

        // Check 4-way adjacent pixels and set flag if need to flood them
        // Left
        if((pixelCol > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol-1, pixelRow)])){
            checkPixel = PIXEL_2D_2_1D(pixelCol-1, pixelRow);

            // Set bit in queue
            pixel_num_to_bitfield(checkPixel, &pxQByte, &pxQOffset);
            pixelFlags[pxQByte] |= (1 << pxQOffset);
        }

        // Up
        if((pixelRow > 0) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow-1)])){
            checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow-1);
            
            // Set bit in queue
            pixel_num_to_bitfield(checkPixel, &pxQByte, &pxQOffset);
            pixelFlags[pxQByte] |= (1 << pxQOffset);
        }

        // Right
        if((pixelCol < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol+1, pixelRow)])){
            checkPixel = PIXEL_2D_2_1D(pixelCol+1, pixelRow);
            
            // Set bit in queue
            pixel_num_to_bitfield(checkPixel, &pxQByte, &pxQOffset);
            pixelFlags[pxQByte] |= (1 << pxQOffset);
        }

        // Down
        if((pixelRow < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow+1)])){
            checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow+1);
            
            // Set bit in queue
            pixel_num_to_bitfield(checkPixel, &pxQByte, &pxQOffset);
            pixelFlags[pxQByte] |= (1 << pxQOffset);
        }


        // Check if pixels left to check
        pixel = PIXEL_COUNT;
        for(i=0; i<PIXEL_COUNT; i++){
            pixel_num_to_bitfield(i, &pxQByte, &pxQOffset);
            if((pixelFlags[pxQByte] & (1<<pxQOffset)) != 0){
                pixel = i;
                break;
            }
        }

    }while(pixel < PIXEL_COUNT);
    
    

    FLOOD_EXIT:
    // Capture SP depth
    __asm mov stackPointerCurrent, sp;
    stackPointerMin = min(stackPointerCurrent, stackPointerMin);

    // Capture call depth
    callDepthMax = max(callDepthCurrent, callDepthMax);
    callDepthCurrent--;
    return TRUE;
}
// #pragma check_stack(on)
#endif

#if FLOOD_VER == 5
// Like v4 but fewer divisions
// SP about 1k
BOOL canvas_draw_flood_v5(BYTE* pixelFrame, int pixel, PixelColorCode_e targetColorCode){
    BYTE pixelList[PIXEL_COUNT];
    short pixelRow, pixelCol;
    short i, j;
    short pxQByte, pxQOffset, pxQSet;
    int checkPixel;

    if (pixelFrame[pixel] != targetColorCode){
        //bail early for flooding shape with same color
        goto FLOOD_EXIT; 
    }

    // memset(pixelQueue, 0, sizeof(pixelQueue));
    for(i=0; i<PIXEL_COUNT; i++){
        pixelList[i] = 0;
    }

    do{
        // for breaking infinite loop error
        callDepthCurrent++;
        if(callDepthCurrent > CALL_DEPTH_LIMIT){
            goto FLOOD_EXIT;
        }

        // Fill pixel
        pixelRow = PIXEL_1D_2_ROW(pixel);
        pixelCol = PIXEL_1D_2_COL(pixel);    
        canvas_draw_brush(floodHWND, &floodHDC, pixelFrame, pixel, pixW, pixH);

        // Clear bit in queue
        pixelList[pixel] = 0;

        // Check 4-way adjacent pixels and set flag if need to flood them
        // Left
        checkPixel = PIXEL_2D_2_1D(pixelCol-1, pixelRow);
        if((pixelCol > 0) && (targetColorCode == pixelFrame[checkPixel])){            
            // Set bit in queue
            pixelList[checkPixel] = 1;
        }

        // Up
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow-1);        
        if((pixelRow > 0) && (targetColorCode == pixelFrame[checkPixel])){
            // Set bit in queue
            pixelList[checkPixel] = 1;
        }

        // Right
        checkPixel = PIXEL_2D_2_1D(pixelCol+1, pixelRow);
        if((pixelCol < CANVAS_DIM-1) && (targetColorCode == pixelFrame[checkPixel])){            
            // Set bit in queue
            pixelList[checkPixel] = 1;
        }

        // Down
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow+1);        
        if((pixelRow < CANVAS_DIM-1) && (targetColorCode == pixelFrame[PIXEL_2D_2_1D(pixelCol, pixelRow+1)])){
            // Set bit in queue
            pixelList[checkPixel] = 1;
        }


        // Check if pixels left to check
        pixel = PIXEL_COUNT;
        for(i=0; i<PIXEL_COUNT; i++){
            if(pixelList[i] != 0){
                pixel = i;
                break;
            }
        }

    }while(pixel < PIXEL_COUNT);
    
    

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

#if FLOOD_VER == 6
// Fixed length queue of max length
BOOL canvas_draw_flood_v6(BYTE* pixelFrame, int pixel, PixelColorCode_e targetColorCode){
    int pixelQueue[PIXEL_COUNT*5];
    short pixelRow, pixelCol;
    short i, j;
    short readIndex, writeIndex;
    int checkPixel;

    if (pixelFrame[pixel] != targetColorCode){
        //bail early for flooding shape with same color
        goto FLOOD_EXIT; 
    }
    
    readIndex = writeIndex = 0;

    // Set up first entry
    pixelQueue[writeIndex++] = pixel;

    
    do{
        // for breaking infinite loop error
        callDepthCurrent++;
        // if(callDepthCurrent > CALL_DEPTH_LIMIT){
        //     MessageBox(NULL, "Depth", "Flood v6", MB_OK);
        //     goto FLOOD_EXIT;
        // }

        if(writeIndex > 1024*5){
            MessageBox(NULL, "Write max", "Flood v6", MB_OK);
            goto FLOOD_EXIT;
        }

        // Get next pixel
        pixel = pixelQueue[readIndex++];

        // Fill pixel
        pixelRow = PIXEL_1D_2_ROW(pixel);
        pixelCol = PIXEL_1D_2_COL(pixel);    
        canvas_draw_brush(floodHWND, &floodHDC, pixelFrame, pixel, pixW, pixH);

        // Check 4-way adjacent pixels and set flag if need to flood them
        // Left
        checkPixel = PIXEL_2D_2_1D(pixelCol-1, pixelRow);
        if((pixelCol > 0) && (targetColorCode == pixelFrame[checkPixel])){            
            // Add
            pixelQueue[writeIndex++] = checkPixel;
        }

        // Up
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow-1);        
        if((pixelRow > 0) && (targetColorCode == pixelFrame[checkPixel])){
            // Add
            pixelQueue[writeIndex++] = checkPixel;
        }

        // Right
        checkPixel = PIXEL_2D_2_1D(pixelCol+1, pixelRow);
        if((pixelCol < CANVAS_DIM-1) && (targetColorCode == pixelFrame[checkPixel])){            
            // Add
            pixelQueue[writeIndex++] = checkPixel;
        }

        // Down
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow+1);        
        if((pixelRow < CANVAS_DIM-1) && (targetColorCode == pixelFrame[checkPixel])){
            // Add
            pixelQueue[writeIndex++] = checkPixel;
        }

    // Check if pixels left to check
    }while(readIndex < writeIndex);
    
    

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


#if FLOOD_VER == 7
// Fixed length queue of max length without adding duplicates to queue
// SP about 3k
BOOL canvas_draw_flood_v7(HDC* hdc, BYTE* pixelFrame, CanvasFloodArgs_s* args){
    int pixelQueue[PIXEL_COUNT];
    BYTE pixelAdded[PIXEL_COUNT];
    short pixelRow, pixelCol;
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

    
    do{
        // for breaking infinite loop error
        callDepthCurrent++;
        // if(callDepthCurrent > CALL_DEPTH_LIMIT){
        //     MessageBox(NULL, "Depth", "Flood v6", MB_OK);
        //     goto FLOOD_EXIT;
        // }

        if(writeIndex > 1024){
            MessageBox(NULL, "Write max", "Flood v6", MB_OK);
            goto FLOOD_EXIT;
        }

        // Get next pixel
        args->pixel = pixelQueue[readIndex++];

        // Fill pixel
        pixelRow = PIXEL_1D_2_ROW(args->pixel);
        pixelCol = PIXEL_1D_2_COL(args->pixel);    
        brushArgs.pixel = args->pixel;
        brushArgs.size = args->size;
        brushArgs.newColorCode = args->newColorCode;
        canvas_draw_brush(hdc, pixelFrame, &brushArgs);

        // Check 4-way adjacent pixels and set flag if need to flood them
        // Left
        checkPixel = PIXEL_2D_2_1D(pixelCol-1, pixelRow);
        if((pixelCol > 0) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){            
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;
        }

        // Up
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow-1);        
        if((pixelRow > 0) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;            
        }

        // Right
        checkPixel = PIXEL_2D_2_1D(pixelCol+1, pixelRow);
        if((pixelCol < CANVAS_DIM-1) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){            
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;  
        }

        // Down
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow+1);        
        if((pixelRow < CANVAS_DIM-1) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0)){
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;  
        }

    // Check if pixels left to check
    }while(readIndex < writeIndex);
    
    

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
    CanvasHistoryEntry_s* newHistoryEntry;


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
            hdc = GetDC(hwnd);
            switch(HIWORD(lParam)){
                case 0: //undo
                    // assume only brush tool for now
                    canvas_draw_brush(&hdc, pixelFrame, canvasHistory[LOWORD(lParam)]->prevAction->args);
                    ValidateRect(hwnd, NULL);
                    break;
                case 1: //redo
                    canvas_draw_brush(&hdc, pixelFrame, canvasHistory[LOWORD(lParam)]->nextAction->args);
                    break;
                default:
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
                    CanvasLineArgs_s lineArgs;
                    switch (drawState){
                        case DRAW_STATE_START:
                            drawState = DRAW_LINE_FIRST;
                            ptPixelDraw1.x = pixCol;
                            ptPixelDraw1.y = pixRow;
                            break;
                        case DRAW_LINE_FIRST:
                            ptPixelDraw2.x = pixCol;
                            ptPixelDraw2.y = pixRow;

                            lineArgs.pixel = pixel;
                            lineArgs.size = cxBlock;
                            lineArgs.newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                            lineArgs.pt1 = ptPixelDraw1;
                            lineArgs.pt2 = ptPixelDraw2;                            
                            canvas_draw_line(&hdc, pixelFrame, &lineArgs);
                            drawState = DRAW_STATE_START;
                            break;   
                    }                         
                    
                    ValidateRect(hwnd, NULL);
                    break;
                    }

                case CanvasToolRect:{
                    CanvasRectArgs_s rectArgs;

                    switch (drawState){
                        case DRAW_STATE_START:
                            drawState = DRAW_LINE_FIRST;
                            ptPixelDraw1.x = pixCol;
                            ptPixelDraw1.y = pixRow;
                            break;
                        case DRAW_LINE_FIRST:
                            ptPixelDraw2.x = pixCol;
                            ptPixelDraw2.y = pixRow;

                            rectArgs.pixel = pixel;
                            rectArgs.size = cxBlock;
                            rectArgs.newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                            rectArgs.pt1 = ptPixelDraw1;
                            rectArgs.pt2 = ptPixelDraw2;
                            canvas_draw_rect(&hdc, pixelFrame, &rectArgs);

                            drawState = DRAW_STATE_START;
                            break;                            
                    }
                    ValidateRect(hwnd, NULL);
                    break;
                }

                case CanvasToolFlood:{
                    CanvasFloodArgs_s floodArgs;
                    if(pixelFrame[pixel] == (BYTE)GetWindowWord(hwnd, CanvasWordForeColor)){
                        
                        // MessageBox(hwnd, "Skip", "Flood", MB_OK);
                        break;
                    }
                    __asm mov stackPointerStart, sp;
                    stackPointerMin = stackPointerStart;
                    stackPointerCurrent = stackPointerStart;

                    callDepthCurrent = 0;
                    callDepthMax = 0;
                    lpPoint.x = pixCol;
                    lpPoint.y = pixRow;

                    #if FLOOD_VER == 0
                        canvas_draw_flood_v0(hwnd, &hdc, pixelFrame, pixel, &lpPoint, cxBlock, cyBlock, pixelFrame[pixel]);
                    #elif FLOOD_VER == 1
                        canvas_draw_flood_v1(hwnd, &hdc, pixelFrame, pixel, &lpPoint, cxBlock, cyBlock, pixelFrame[pixel]);
                    #elif FLOOD_VER == 2
                        canvas_draw_flood_v2(hwnd, &hdc, pixelFrame, pixel, &lpPoint, cxBlock, cyBlock, pixelFrame[pixel]);
                    #elif FLOOD_VER == 3
                        // total usage 22 bytes
                        // uses 10 bytes for args and to call function
                        // uses 6 bytes for BP, SI, and DI on stack
                        // uses 6 bytes for local variables
                        

                        // SP-2: push 3rd arg
                        // SP-2: push 2nd arg
                        // SP-2: push 1st arg
                        // SP-4: CALL function
                        // SP at -10
                        canvas_draw_flood_v3(pixelFrame, pixel, pixelFrame[pixel]);
                    #elif FLOOD_VER == 4
                        canvas_draw_flood_v4(pixelFrame, pixel, pixelFrame[pixel]);
                    #elif FLOOD_VER == 5
                        canvas_draw_flood_v5(pixelFrame, pixel, pixelFrame[pixel]);
                    #elif FLOOD_VER == 6
                        canvas_draw_flood_v6(pixelFrame, pixel, pixelFrame[pixel]);
                    #elif FLOOD_VER == 7
                        floodArgs.pixel = pixel;
                        floodArgs.size = cxBlock;
                        floodArgs.newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                        floodArgs.targetColorCode = pixelFrame[pixel];
                        canvas_draw_flood_v7(&hdc, pixelFrame, &floodArgs);
                    #else
                        callDepthMax = -1;
                    #endif

                    nLength = wsprintf(szBuffer, "V%d: Call %d, SP %d.", FLOOD_VER, callDepthMax, stackPointerStart - stackPointerMin);
                    MessageBox(hwnd, szBuffer, "Flood", MB_OK);
                    ValidateRect(hwnd, NULL);
                    break;    
                }                        
                    
                case CanvasToolErase:{
                    for(x=0; x<PIXEL_COUNT; x++){ 
                       pixelFrame[x] = (BYTE)PixelColorCodeWhite;
                    }
                    
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                }

                case CanvasToolRestore:{
                    CanvasRestoreArgs_s restoreArgs;

                    ptPixelDraw1.x = pixCol;
                    ptPixelDraw1.y = pixRow;
                    ptPixelDraw2.x = min(pixCol+4, CANVAS_DIM-1);
                    ptPixelDraw2.y = min(pixRow+4, CANVAS_DIM-1);

                    restoreArgs.size = cxBlock;
                    restoreArgs.pt1 = ptPixelDraw1;
                    restoreArgs.pt2 = ptPixelDraw2;
                    restoreArgs.colorData = restoreData;
                    canvas_restore_rect(&hdc, pixelFrame, &restoreArgs);

                    ValidateRect(hwnd, NULL);
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
    RECT rect;
    static short cxChar, cxCaps, cyChar, cxClient, cyClient, nMaxWidth, nVscrollPos, nVscrollMax, nHscrollPos, nHscrollMax;
    short i, x, y, nPaintBegin, nPaintEnd, nVscrollInc, nHscrollInc;
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