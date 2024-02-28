#include <WINDOWS.H>  
#include <stdio.h>

#include "iconedit.h"
#include "canvas.h"
#include "toolbar.h"
#include "colorbox.h"
#include "log.h"

// As-implemented, the cursor is not bounded here
// This is intentional since eventually the canvas selector will be shown w/ a rectangle instead of the cursor
void move_cursor(short xAmount, short yAmount){
    POINT lpCursorPoint;
    GetCursorPos(&lpCursorPoint);
    lpCursorPoint.x += xAmount;
    lpCursorPoint.y += yAmount;
    SetCursorPos(lpCursorPoint.x, lpCursorPoint.y);
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

BOOL canvas_draw_brush(HWND hwnd, HDC* hdc, BYTE* pixelFrame, int pixel, POINT* ptClick, short width, short height){
    COLORREF newColor;
    HBRUSH hBrush;
    RECT rect;
    BYTE newColorCode;
    short pixelCol, pixelRow;

    pixelCol = PIXEL_1D_2_COL(pixel);
    pixelRow = PIXEL_1D_2_ROW(pixel);

    newColorCode = (BYTE)GetWindowWord(hwnd, CanvasWordForeColor);
    pixelFrame[pixel] = newColorCode;  

    pixel_color_code_to_rgb(newColorCode, &newColor);
    hBrush = CreateSolidBrush(newColor);

    // rect.left = (pixelCol*width)+1;
    // rect.top = (pixelRow*height)+1;
    rect.left = (ptClick->x)+1;
    rect.top = (ptClick->y)+1;
    rect.right = rect.left + width - 2 ;
    rect.bottom = rect.top + height - 2;

    FillRect(*hdc, &rect, hBrush);
    DeleteObject(hBrush);

    return TRUE;
}


BOOL canvas_draw_line(HWND hwnd, HDC* hdc, BYTE* pixelFrame, int pixel, POINT* ptClick1, POINT* ptClick2, short width, short height){
    short pixelX, pixelY, pxLeftCol, pxRightCol, pxTopRow, pxBotRow;
    short slope, inc;
    POINT clickPoint;

    //Quick fix for vertical lines
    if(ptClick1->x == ptClick2->x){
        pixelX = ptClick1->x;
        for(pixelY = min(ptClick1->y, ptClick2->y); pixelY <= max(ptClick1->y, ptClick2->y); pixelY++){
            clickPoint.x = pixelX * width;
            clickPoint.y = pixelY * height;

            canvas_draw_brush(hwnd, hdc, pixelFrame, 
                PIXEL_2D_2_1D(pixelX, pixelY), 
                &clickPoint, 
                width, height);
        }
        return TRUE;
    }

    // Have to rework this section anyways for the line to be connected and to work for slope < 1
    if(ptClick1->x <=  ptClick2->x){
        inc = 1;
    }else{
        inc = -1;
    }
       
    slope = ((ptClick2->y - ptClick1->y)/(ptClick2->x - ptClick1->x));

    for(pixelX = ptClick1->x; inc*(pixelX - ptClick2->x) <= 0; pixelX += inc){
        pixelY = (pixelX - ptClick1->x)*slope + (ptClick1->y);
            clickPoint.x = pixelX * width;
            clickPoint.y = pixelY * height;

            canvas_draw_brush(hwnd, hdc, pixelFrame, 
                PIXEL_2D_2_1D(pixelX, pixelY), 
                &clickPoint, 
                width, height);
    }

    return TRUE;
}


BOOL canvas_draw_rect(HWND hwnd, HDC* hdc, BYTE* pixelFrame, int pixel, POINT* ptClick1, POINT* ptClick2, short width, short height){
    short pixelX, pixelY, pxLeftCol, pxRightCol, pxTopRow, pxBotRow;
    POINT clickPoint, ptLeftTopClick, ptRightBotClick;


    pxLeftCol = min(ptClick1->x, ptClick2->x);
    pxRightCol = max(ptClick1->x, ptClick2->x);    
    pxTopRow = min(ptClick1->y, ptClick2->y);
    pxBotRow = max(ptClick1->y, ptClick2->y);    

    for(pixelX = pxLeftCol; pixelX <= pxRightCol; pixelX++){
        for(pixelY = pxTopRow; pixelY <= pxBotRow; pixelY++){
            clickPoint.x = pixelX * width;
            clickPoint.y = pixelY * height;

            canvas_draw_brush(hwnd, hdc, pixelFrame, 
                PIXEL_2D_2_1D(pixelX, pixelY), 
                &clickPoint, 
                width, height);
        }
    }

    return TRUE;
}


BOOL canvas_draw_flood(HWND hwnd, HDC* hdc, BYTE* pixelFrame, int pixel, POINT* ptClick, short width, short height, PixelColorCode_e targetColorCode){
    short pixelRow, pixelCol, nextRow, nextCol;
    POINT clickPoint;
    PixelColorCode_e currentColorCode;

    pixelRow = PIXEL_1D_2_ROW(pixel);
    pixelCol = PIXEL_1D_2_COL(pixel);    
    clickPoint.x = pixelCol*width;
    clickPoint.y = pixelRow*height;

    currentColorCode = pixelFrame[pixel];
    if (currentColorCode != targetColorCode){
        return TRUE;
    }else{
        canvas_draw_brush(hwnd, hdc, pixelFrame, pixel, &clickPoint, width, height);
    }

    

    if(pixelCol > 0){
        canvas_draw_flood(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol-1, pixelRow), &clickPoint, width, height, targetColorCode);
    }
    if(pixelCol < CANVAS_DIM-1){
        canvas_draw_flood(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol+1, pixelRow), &clickPoint, width, height, targetColorCode);
    }

    if(pixelRow > 0){
        canvas_draw_flood(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow-1), &clickPoint, width, height, targetColorCode);    
    }

    if(pixelRow < CANVAS_DIM-1){
        canvas_draw_flood(hwnd, hdc, pixelFrame, PIXEL_2D_2_1D(pixelCol, pixelRow+1), &clickPoint, width, height, targetColorCode);    
    }

    return TRUE;
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
    char szBuffer[80];
    short nLength;
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
            switch(wParam){
                case VK_SPACE:
                    GetCursorPos(&lpMousePoint);
                    ScreenToClient(hwndCanvas, &lpMousePoint);
                    SendMessage(hwndCanvas, WM_LBUTTONDOWN, 0, MAKELONG(lpMousePoint.x, lpMousePoint.y));
                    break;

                case VK_LEFT:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        move_cursor
                    (-1 * canvasSize/CANVAS_DIM, 0);
                    }
                    break;

                case 'A':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor
                    (-1 * canvasSize/CANVAS_DIM, 0);
                    }
                    break;

                case 'H':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor
                    (-1*canvasSize/CANVAS_DIM, 0);
                    }
                    break; 


                case VK_RIGHT:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        move_cursor
                    (canvasSize/CANVAS_DIM, 0);  
                    }
                    break;
                
                case 'D':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor
                    (canvasSize/CANVAS_DIM, 0); 
                    }
                    break; 

                case 'L':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor
                    (canvasSize/CANVAS_DIM, 0);
                    }
                    break; 


                case VK_UP:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        move_cursor
                    (0, -1*canvasSize/CANVAS_DIM); 
                    }
                    break;
                
                case 'W':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor
                    (0, -1*canvasSize/CANVAS_DIM);
                    }
                    break; 

                case 'K':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor
                    (0, -1*canvasSize/CANVAS_DIM);
                    }
                    break; 


                case VK_DOWN:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        move_cursor
                    (0, canvasSize/CANVAS_DIM);
                    }
                    break;

                case 'S':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        move_cursor
                    (0, canvasSize/CANVAS_DIM);                    
                    }
                    break;

                case 'J':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        move_cursor
                    (0, canvasSize/CANVAS_DIM);                    
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
            nLength = wsprintf (szBuffer, "wParam: %d, lParam %ld.", wParam, lParam);

            // MessageBox(hwnd, szBuffer, "IconEdit", MB_OK);
            if(wParam == CHILD_ID_COLORBOX){
                SetWindowWord(hwndCanvas, CanvasWordForeColor, LOWORD(lParam));
                SetWindowWord(hwndCanvas, CanvasWordBackColor, HIWORD(lParam));
            }else if(wParam == CHILD_ID_TOOLBAR){
                toolbarTool = (ToolbarTool_e) LOWORD(lParam);
                map_toolbar_tool_to_canvas(&canvasTool, toolbarTool);

                SetWindowWord(hwndCanvas, CanvasWordTool, (WORD)canvasTool);
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
    char szBuffer[80];
    short nLength;

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
    BYTE colorCode, newColorCode;
    // WORD activeColorForeground;
    COLORREF newColor;
    //char szBuffer[40];
    //short nLength;
    static BYTE pixelFrame[PIXEL_COUNT];
    static WORD drawState;
    static POINT ptPixelDraw1, ptPixelDraw2;


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
            return 0;
        }
        case WM_SIZE:{
            cxBlock = LOWORD(lParam) / CANVAS_DIM;
            cyBlock = HIWORD(lParam) / CANVAS_DIM;
            return 0;
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
                case CanvasToolBrush:
                    lpPoint.x = ALIGN(x, cxBlock);
                    lpPoint.y = ALIGN(y, cyBlock);
                    canvas_draw_brush(hwnd, &hdc, pixelFrame, pixel, &lpPoint, cxBlock, cyBlock);
                    break;
                case CanvasToolLine:
                    switch (drawState){
                        case DRAW_STATE_START:
                            drawState = DRAW_LINE_FIRST;
                            ptPixelDraw1.x = pixCol;
                            ptPixelDraw1.y = pixRow;
                            break;
                        case DRAW_LINE_FIRST:
                            ptPixelDraw2.x = pixCol;
                            ptPixelDraw2.y = pixRow;
                            canvas_draw_line(hwnd, &hdc, pixelFrame, pixel, &ptPixelDraw1, &ptPixelDraw2, cxBlock, cyBlock);
                            drawState = DRAW_STATE_START;
                            break;                            
                    }

                    break;
                case CanvasToolRect:
                    switch (drawState){
                        case DRAW_STATE_START:
                            drawState = DRAW_LINE_FIRST;
                            ptPixelDraw1.x = pixCol;
                            ptPixelDraw1.y = pixRow;
                            break;
                        case DRAW_LINE_FIRST:
                            ptPixelDraw2.x = pixCol;
                            ptPixelDraw2.y = pixRow;
                            canvas_draw_rect(hwnd, &hdc, pixelFrame, pixel, &ptPixelDraw1, &ptPixelDraw2, cxBlock, cyBlock);
                            drawState = DRAW_STATE_START;
                            break;                            
                    }

                    break;

                case CanvasToolFlood:
                    lpPoint.x = pixCol;
                    lpPoint.y = pixRow;
                    canvas_draw_flood(hwnd, &hdc, pixelFrame, pixel, &lpPoint, cxBlock, cyBlock, pixelFrame[pixel]);
                    break;                            
                    

                default:
                    MessageBeep(0);
                    break; //TODO signal error here
            }
            
            ReleaseDC(hwnd, hdc);            
            ValidateRect(hwnd, NULL);

            // If I re-paint every time, the canvas blips.
            // So only draw the pixels I modified
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
            DeleteObject(hPen);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


long FAR PASCAL _export WndProcLog(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    short x;

    switch(message){
        case WM_CREATE:
            for (x=0; x<LOG_EXTRA_WORDS; x+=2){
                SetWindowWord(hwnd, x, 0);
            } 
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
            MoveTo(hdc, rect.left, rect.top);
            LineTo(hdc, rect.right, rect.bottom);
            MoveTo(hdc, rect.left, rect.bottom);
            LineTo(hdc, rect.right, rect.top);

            
            DrawText(hdc, "Log", -1, &rect, 
                        DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}