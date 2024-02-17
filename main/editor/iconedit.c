#include <WINDOWS.H>  
#include <stdio.h>


#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#ifndef WIN31                       
#define FAR  
#define PASCAL  
#define _export
typedef char* LPSTR;
typedef const char* LPCSTR;
#endif                                  
                                    
long FAR PASCAL _export WndProcMain(HWND, UINT, UINT, LONG);
long FAR PASCAL _export WndProcToolbar(HWND, UINT, UINT, LONG);
long FAR PASCAL _export WndProcCanvas(HWND, UINT, UINT, LONG);
long FAR PASCAL _export WndProcLog(HWND, UINT, UINT, LONG);

char szNameToolbar[] = "Toolbar";
char szNameCanvas[] = "Canvas";
char szNameLog[] = "Log";
LPSTR buffer = "1234567890";

#define TOOLBAR_ROWS 10
#define TOOLBAR_COLS 2
#define CANVAS_DIM 32
#define PIXEL_COUNT CANVAS_DIM * CANVAS_DIM
#define PIXEL_CHANNEL_COUNT 3 //red, green, blue
#define PIXEL_CHANNEL_BYTES 1 //0-255
#define FRAME_BUFFER_BYTE_COUNT PIXEL_COUNT * PIXEL_CHANNEL_COUNT * PIXEL_CHANNEL_BYTES


#define COLOR_BLACK (RGB(0, 0, 0))
#define COLOR_GRAY (RGB(128, 128, 128))

#define COLOR_RED (RGB(255, 0, 0))
#define COLOR_MAROON (RGB(128, 0, 0))

#define COLOR_LIME (RGB(0, 255, 0))
#define COLOR_GREEN (RGB(0, 128, 0))

#define COLOR_YELLOW (RGB(255, 255, 0))
#define COLOR_OLIVE (RGB(128, 128, 0))

#define COLOR_BLUE (RGB(0, 0, 255))
#define COLOR_NAVY (RGB(0, 0, 128))

#define COLOR_FUCHIA (RGB(255, 0, 255))
#define COLOR_PURPLE (RGB(128, 0, 128))

#define COLOR_AQUA (RGB(0, 255, 255))
#define COLOR_TEAL (RGB(0, 128, 128))

#define COLOR_WHITE (RGB(255, 255, 255))
#define COLOR_SILVER (RGB(192, 192, 192))

typedef enum ControlKeyGroup{
    CTRL_GROUP_CURSOR = 0,
    CTRL_GROUP_LH_TOOLBOX = 1,
    CTRL_GROUP_RH_TOOLBOX = 2
} ControlKeyGroup_e;

typedef enum MovementDirection{
    MOVEMENT_LEFT = 0,
    MOVEMENT_RIGHT = 1,
    MOVEMENT_UP = 2,
    MOVEMENT_DOWN = 3,
    MOVEMENT_UPRIGHT = 4,
    MOVEMENT_RIGHTUP = 4,
    MOVEMENT_UPLEFT = 5,
    MOVEMENT_LEFTUP = 5,
    MOVEMENT_DOWNRIGHT = 6,
    MOVEMENT_RIGHTDOWN = 6,
    MOVEMENT_DOWNLEFT = 7,
    MOVEMENT_LEFTDOWN = 7
} MovementDirection_e;

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
        wndclass.cbWndExtra = sizeof(WORD);
        wndclass.hIcon = NULL;
        wndclass.lpszClassName = szNameToolbar;
        RegisterClass(&wndclass);
        
        // Child window - canvas
        wndclass.lpfnWndProc = WndProcCanvas;
        wndclass.cbWndExtra = sizeof(BYTE)*FRAME_BUFFER_BYTE_COUNT;
        wndclass.hIcon = NULL;
        wndclass.lpszClassName = szNameCanvas;
        RegisterClass(&wndclass);
        
        // Child window - log
        wndclass.lpfnWndProc = WndProcLog;
        wndclass.cbWndExtra = sizeof(WORD);
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

// As-implemented, the cursor is not bounded here
// This is intentional since eventually the canvas selector will be shown w/ a rectangle instead of the cursor
void MoveCursor(short xAmount, short yAmount){
    POINT lpCursorPoint;
    GetCursorPos(&lpCursorPoint);
    lpCursorPoint.x += xAmount;
    lpCursorPoint.y += yAmount;
    SetCursorPos(lpCursorPoint.x, lpCursorPoint.y);
    return;
}

long FAR PASCAL _export WndProcMain(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    static HWND hwndToolbar;
    static HWND hwndCanvas;
    static HWND hwndLog;
    static short cxBlock, cyBlock;
    static short canvasSize;
    static short counter;
    char szBuffer[80];
    short nLength;
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    static short cxChar, cxCaps, cyChar;
    TEXTMETRIC tm;
    POINT lpMousePoint;
    HBRUSH hBrush;
    static BYTE CTRL_WASD = CTRL_GROUP_CURSOR;
    static BYTE CTRL_HJKL = CTRL_GROUP_CURSOR;
    static BYTE CTRL_ARROWS = CTRL_GROUP_CURSOR;

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
                                        hwnd, 1, GetWindowWord(hwnd, GWW_HINSTANCE), NULL);

            hwndCanvas = CreateWindow(szNameCanvas, NULL, WS_CHILDWINDOW | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        hwnd, 1, GetWindowWord(hwnd, GWW_HINSTANCE), NULL);

            hwndLog = CreateWindow(szNameLog, NULL, WS_CHILDWINDOW | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        hwnd, 1, GetWindowWord(hwnd, GWW_HINSTANCE), NULL);
            
            return 0;
            }
        case WM_SIZE:{
            cxBlock = LOWORD(lParam) / 5;
            cyBlock = HIWORD(lParam) / 5;
            canvasSize = min( (cxBlock<<2) - (cxBlock<<2)%32, ((cyBlock<<2) - (cyBlock<<2)%32));

            // MoveWindow(hwndToolbar, 0, cyBlock, cxBlock, cyBlock*3, TRUE);
            MoveWindow(hwndToolbar, 0, 0, cxBlock, cyBlock<<2, TRUE);
            MoveWindow(hwndCanvas, cxBlock, 0, canvasSize, canvasSize, TRUE);
            MoveWindow(hwndLog, 0, cyBlock<<2, LOWORD(lParam), cyBlock, TRUE);

            lpMousePoint.x = cxBlock;
            lpMousePoint.y = 0;
            ClientToScreen(hwnd, &lpMousePoint);

            SetCursorPos(lpMousePoint.x, lpMousePoint.y);
            MessageBeep(0);  
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
                        MoveCursor(-1 * canvasSize/CANVAS_DIM, 0);
                    }
                    break;

                case 'A':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        MoveCursor(-1 * canvasSize/CANVAS_DIM, 0);
                    }
                    break;

                case 'H':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        MoveCursor(-1*canvasSize/CANVAS_DIM, 0);
                    }
                    break; 


                case VK_RIGHT:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        MoveCursor(canvasSize/CANVAS_DIM, 0);  
                    }
                    break;
                
                case 'D':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        MoveCursor(canvasSize/CANVAS_DIM, 0); 
                    }
                    break; 

                case 'L':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        MoveCursor(canvasSize/CANVAS_DIM, 0);
                    }
                    break; 


                case VK_UP:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        MoveCursor(0, -1*canvasSize/CANVAS_DIM); 
                    }
                    break;
                
                case 'W':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        MoveCursor(0, -1*canvasSize/CANVAS_DIM);
                    }
                    break; 

                case 'K':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        MoveCursor(0, -1*canvasSize/CANVAS_DIM);
                    }
                    break; 


                case VK_DOWN:
                    if(CTRL_ARROWS == CTRL_GROUP_CURSOR){
                        MoveCursor(0, canvasSize/CANVAS_DIM);
                    }
                    break;

                case 'S':
                    if(CTRL_WASD == CTRL_GROUP_CURSOR){
                        MoveCursor(0, canvasSize/CANVAS_DIM);                    
                    }
                    break;

                case 'J':
                    if(CTRL_HJKL == CTRL_GROUP_CURSOR){
                        MoveCursor(0, canvasSize/CANVAS_DIM);                    
                    }
                    break; 
            }
            return 0;
        }

        case WM_PAINT:
            // nLength = wsprintf(szBuffer, "cxBlock %d, cyBlock %d, canvasSize %d", cxBlock, cyBlock, canvasSize);
            hdc = BeginPaint(hwnd, &ps);
            // TextOut(hdc, 10, cyBlock<<2, szBuffer, nLength);
            EndPaint(hwnd, &ps);
            return 0;

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
    RECT rect;
    static short cxBlock, cyBlock;
    short x, y;

    switch(message){
        case WM_CREATE:
            SetWindowWord(hwnd, 0, 0);
            return 0;
        
        case WM_SIZE:
            cxBlock = LOWORD(lParam) / TOOLBAR_COLS;
            cyBlock = HIWORD(lParam) / TOOLBAR_ROWS;
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);   

            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

            for(x=0; x<TOOLBAR_COLS; x++){
                for(y=0; y<TOOLBAR_ROWS; y++){
                    Rectangle(hdc, rect.left + x*cxBlock, rect.top + y*cyBlock, 
                                    rect.left + x*cxBlock +cxBlock, rect.top + y*cyBlock + cyBlock);
                }
            }

            // Draw X
            // MoveTo(hdc, rect.left, rect.top);
            // LineTo(hdc, rect.right, rect.bottom);
            // MoveTo(hdc, rect.left, rect.bottom);
            // LineTo(hdc, rect.right, rect.top);
            

            // DrawText(hdc, "Toolbar", -1, &rect, 
            //             DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
            return 0;
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
    POINT lpMousePoint, lpPoint;
    WORD pixelColor, colorRed, colorBlue, colorGreen;
    WORD newColorR, newColorG, newColorB;
    COLORREF newColor;
    WORD wordAddress, wordMask;
    BYTE dataShift;
    char szBuffer[40];
    short nLength;


    switch(message){
        case WM_CREATE:{
            for(x=0; x<PIXEL_COUNT; x+=2){ //
                // using GetR/G/BValue causes truncation warning, so I'm doing this manually
                newColorR = (WORD)((COLOR_WHITE & 0x000000FF)>>0);
                newColorG = (WORD)((COLOR_WHITE & 0x0000FF00)>>8);                
                newColorB = (WORD)((COLOR_WHITE & 0x00FF0000)>>16);

                SetWindowWord(hwnd, x,                 (newColorR<<8) | (newColorR) );
                SetWindowWord(hwnd, x + PIXEL_COUNT,   (newColorG<<8) | (newColorG) );                
                SetWindowWord(hwnd, x + 2*PIXEL_COUNT, (newColorB<<8) | (newColorB) );
            }
            hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            return 0;
        }
        case WM_SIZE:{
            cxBlock = LOWORD(lParam) / CANVAS_DIM;
            cyBlock = HIWORD(lParam) / CANVAS_DIM;
            return 0;
        }
        case WM_LBUTTONDOWN:{
            MessageBeep(0);
            x = LOWORD(lParam);
            y = HIWORD(lParam);

            pixCol = x/cxBlock;
            pixRow = y/cyBlock;
            pixel = pixCol + pixRow*CANVAS_DIM;

            if( (pixCol >= CANVAS_DIM) || (pixCol < 0) || (pixRow >= CANVAS_DIM) || (pixRow < 0)){
                return 0;
            }

            wordAddress = pixel & 0xFFFE;            

            if(pixel % 2 == 0){
                wordMask = 0xFF00;
                dataShift = 0;
                newColor = COLOR_GREEN;
            }else{
                wordMask = 0x00FF;
                dataShift = 8;
                newColor = COLOR_BLUE;
            }

            colorRed = GetWindowWord(hwnd, wordAddress + 0);
            colorGreen = GetWindowWord(hwnd, wordAddress + PIXEL_COUNT);            
            colorBlue = GetWindowWord(hwnd, wordAddress + 2*PIXEL_COUNT);

            SetWindowWord(hwnd, wordAddress + 0,             (colorRed & wordMask) | (GetRValue(newColor) << dataShift));
            SetWindowWord(hwnd, wordAddress + PIXEL_COUNT,   (colorGreen & wordMask) | (GetGValue(newColor) << dataShift));
            SetWindowWord(hwnd, wordAddress + 2*PIXEL_COUNT, (colorBlue & wordMask) | (GetBValue(newColor) << dataShift));

            hdc = GetDC(hwnd);
            hBrush = CreateSolidBrush(RGB(0xFF, 0x00, 0xFF));
            
            lpPoint.x = x - x%cxBlock;
            lpPoint.y = y - y%cyBlock;

            rect.left = lpPoint.x+1;
            rect.top = lpPoint.y+1;
            rect.right = rect.left + cxBlock - 2 ;
            rect.bottom = rect.top + cyBlock - 2;

            // Rectangle(hdc, rect.left-1, rect.top-1, rect.right+1, rect.bottom+1);
            FillRect(hdc, &rect, hBrush);
            ReleaseDC(hwnd, hdc);
            DeleteObject(hBrush);
            ValidateRect(hwnd, NULL);

            // If I re-paint every time, the canvas blips.
            // So only draw the pixel I modified
            // InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            // nLength = wsprintf(szBuffer, "cxBlock %d, size %d", cxBlock, rect.right - rect.left);
            
            // Rectangle(hdc, rect.left, rect.top, rect.left + 32*cxBlock, rect.top + 32*cyBlock);
            // Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);            

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

                wordAddress = pixel & 0xFFFE;
                
                if(pixel % 2 == 0){
                    wordMask = 0x00FF;
                    dataShift = 0;
                }else{
                    wordMask = 0xFF00;
                    dataShift = 8;
                }

                colorRed = (BYTE)((GetWindowWord(hwnd, wordAddress + 0) & wordMask) >> dataShift);
                colorGreen = (BYTE)((GetWindowWord(hwnd, wordAddress + PIXEL_COUNT) & wordMask) >> dataShift);
                colorBlue = (BYTE)((GetWindowWord(hwnd, wordAddress + 2*PIXEL_COUNT) & wordMask) >> dataShift);

                hBrush = CreateSolidBrush(RGB(colorRed, colorGreen, colorBlue));

                rect.left = x*cxBlock + 1;
                rect.top = y*cyBlock + 1;
                rect.right = rect.left + cxBlock - 2 ;
                rect.bottom = rect.top + cyBlock - 2;

                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);
            }

            // rect.left = 4*cxBlock;
            // rect.right = 28*cxBlock;
            // rect.top = 4*cyBlock;
            // rect.bottom = 28*cyBlock;


            
            // TextOut(hdc, rect.left, rect.top, szBuffer, nLength);
            // DrawText(hdc, "Test", -1, &rect, 
                        // DT_SINGLELINE | DT_CENTER | DT_VCENTER);
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

    switch(message){
        case WM_CREATE:
            SetWindowWord(hwnd, 0, 0);
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