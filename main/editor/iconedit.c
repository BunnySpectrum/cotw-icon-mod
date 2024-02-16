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
#define CANVAS_DIV 32
#define EXTRA_WORDS 32*16

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
        wndclass.cbWndExtra = sizeof(WORD)*EXTRA_WORDS;
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

long FAR PASCAL _export WndProcMain(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    static HWND hwndToolbar;
    static HWND hwndCanvas;
    static HWND hwndLog;
    short cxBlock, cyBlock;
    static short canvasSize;
    static short counter;
    char szBuffer[40];
    short nLength;
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    static short cxChar, cxCaps, cyChar;
    TEXTMETRIC tm;
    POINT lpMousePoint;
    HBRUSH hBrush;

    switch(message){
        case WM_CREATE:
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

        case WM_SIZE:
            cxBlock = LOWORD(lParam) / 5;
            cyBlock = HIWORD(lParam) / 5;
            canvasSize = min(cxBlock<<2, cyBlock<<2);

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

        case WM_SETFOCUS:
            ShowCursor(TRUE);
            return 0;

        case WM_KILLFOCUS:
            ShowCursor(FALSE);
            return 0;



        case WM_LBUTTONDOWN:
            MessageBeep(MB_OK);

            return 0;
        
        case WM_KEYDOWN:
            switch(wParam){
                case VK_SPACE:
                    GetCursorPos(&lpMousePoint);
                    ScreenToClient(hwndCanvas, &lpMousePoint);
                    SendMessage(hwndCanvas, WM_LBUTTONDOWN, 0, MAKELONG(lpMousePoint.x, lpMousePoint.y));
                    break;

                case VK_LEFT:
                    GetCursorPos(&lpMousePoint);
                    lpMousePoint.x -= canvasSize/CANVAS_DIV;
                    SetCursorPos(lpMousePoint.x, lpMousePoint.y);
                    break;

                case VK_RIGHT:
                    GetCursorPos(&lpMousePoint);
                    lpMousePoint.x += canvasSize/CANVAS_DIV;
                    SetCursorPos(lpMousePoint.x, lpMousePoint.y);
                    break;

                case VK_UP:
                    GetCursorPos(&lpMousePoint);
                    lpMousePoint.y -= canvasSize/CANVAS_DIV;
                    SetCursorPos(lpMousePoint.x, lpMousePoint.y);
                    break;
                
                case VK_DOWN:
                    GetCursorPos(&lpMousePoint);
                    lpMousePoint.y += canvasSize/CANVAS_DIV;
                    SetCursorPos(lpMousePoint.x, lpMousePoint.y);
                    break;
            }
            return 0;
        
        case WM_PAINT:
                    nLength = wsprintf(szBuffer, "Paint %d, %d", counter++, 80);

                    hdc = BeginPaint(hwnd, &ps);
                    TextOut(hdc, 5, 80, szBuffer, nLength);
                    EndPaint(hwnd, &ps);
                    return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            ;

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
    int x, y, pixel;
    static HPEN hPen;
    static short xSel, ySel;
    HBRUSH hBrush;
    POINT lpMousePoint;

    switch(message){
        case WM_CREATE:
            for(x=0; x<EXTRA_WORDS; x++){
                SetWindowWord(hwnd, x, (((x+1)%256)<<8) | (x%256));
            }
            hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            return 0;
        
        case WM_SIZE:
            cxBlock = LOWORD(lParam) / CANVAS_DIV;
            cyBlock = HIWORD(lParam) / CANVAS_DIV;
            return 0;

        case WM_LBUTTONDOWN:
            x = LOWORD(lParam);// / cxBlock; //CANVAS_DIV;
            y = HIWORD(lParam);// / cyBlock; // CANVAS_DIV;
                    
            hdc = GetDC(hwnd);
            hBrush = CreateSolidBrush(RGB(255, 0, 0));
            
            lpMousePoint.x = x - x%cxBlock;
            lpMousePoint.y = y - y%cyBlock;
            // GetCursorPos(&lpMousePoint);
            // ScreenToClient(hwnd, &lpMousePoint);
            // ClientToScreen(hwnd, &lpMousePoint);
            rect.left = lpMousePoint.x+1;
            rect.top = lpMousePoint.y+1;
            rect.right = rect.left + cxBlock - 2 ;
            rect.bottom = rect.top + cyBlock - 2;

            Rectangle(hdc, rect.left-1, rect.top-1, rect.right+1, rect.bottom+1);
            FillRect(hdc, &rect, hBrush);
            ReleaseDC(hwnd, hdc);
            DeleteObject(hBrush);
            ValidateRect(hwnd, NULL);


        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

            SelectObject(hdc, hPen);

            for(x=0; x<CANVAS_DIV; x++){
                for(y=0; y<CANVAS_DIV; y++){
                    Rectangle(hdc, rect.left + x*cxBlock, rect.top + y*cyBlock, 
                                    rect.left + x*cxBlock +cxBlock, rect.top + y*cyBlock + cyBlock);
                }
            }

            for(pixel=0; pixel<EXTRA_WORDS*2; pixel++){
                x = pixel % 32;
                y = pixel/32;

                if(pixel % 2 == 0){
                hBrush = CreateSolidBrush(RGB(
                    LOBYTE(GetWindowWord(hwnd, pixel>>1)), 
                    LOBYTE(GetWindowWord(hwnd, pixel>>1)), 
                    LOBYTE(GetWindowWord(hwnd, pixel>>1)) 
                    ));
                }else{
                hBrush = CreateSolidBrush(RGB(
                    HIBYTE(GetWindowWord(hwnd, (pixel>>1))), 
                    HIBYTE(GetWindowWord(hwnd, (pixel>>1))), 
                    HIBYTE(GetWindowWord(hwnd, (pixel>>1))) 
                    ));

                }

                rect.left = x*cxBlock + 1;
                rect.top = y*cyBlock + 1;
                rect.right = rect.left + cxBlock - 2 ;
                rect.bottom = rect.top + cyBlock - 2;

                FillRect(hdc, &rect, hBrush);
                GetWindowWord(hwnd, pixel);
                DeleteObject(hBrush);
            }


            
            // Draw X
            // MoveTo(hdc, rect.left, rect.top);
            // LineTo(hdc, rect.right, rect.bottom);
            // MoveTo(hdc, rect.left, rect.bottom);
            // LineTo(hdc, rect.right, rect.top);




            
//            DrawText(hdc, "Canvas", -1, &rect, 
  //                      DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
            return 0;

        case WM_DESTROY:
            DeleteObject(hPen);
            PostQuitMessage(0);
            return 0;
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