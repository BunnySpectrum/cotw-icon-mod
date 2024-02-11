#include <WINDOWS.H>

long FAR PASCAL _export WndProcMain(HWND, UINT, UINT, LONG);
long FAR PASCAL _export WndProcToolbar(HWND, UINT, UINT, LONG);
long FAR PASCAL _export WndProcCanvas(HWND, UINT, UINT, LONG);
long FAR PASCAL _export WndProcLog(HWND, UINT, UINT, LONG);

char szNameToolbar[] = "Toolbar";
char szNameCanvas[] = "Canvas";
char szNameLog[] = "Log";

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
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
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
        wndclass.cbWndExtra = sizeof(WORD);
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


    switch(message){
        case WM_CREATE:
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

            MoveWindow(hwndToolbar, 0, 0, cxBlock, cyBlock<<2, TRUE);
            MoveWindow(hwndCanvas, cxBlock, 0, cxBlock<<2, cyBlock<<2, TRUE);
            MoveWindow(hwndLog, 0, cyBlock<<2, LOWORD(lParam), cyBlock, TRUE);
            MessageBeep(0);   
            return 0;

        case WM_LBUTTONDOWN:
            MessageBeep(0);
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

    switch(message){
        case WM_CREATE:
            SetWindowWord(hwnd, 0, 0);
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);   
            
            MoveTo(hdc, 0, 0);
            LineTo(hdc, rect.right, 0);
            LineTo(hdc, rect.right, rect.bottom);
            LineTo(hdc, 0, rect.bottom);
            LineTo(hdc, 0, 0);
            LineTo(hdc, rect.right, rect.bottom);
            MoveTo(hdc, 0, rect.bottom);
            LineTo(hdc, rect.right, 0);

            DrawText(hdc, "Toolbar", -1, &rect, 
                        DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

long FAR PASCAL _export WndProcCanvas(HWND hwnd, UINT message, UINT wParam, LONG lParam){
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
            
            MoveTo(hdc, 0, 0);
            LineTo(hdc, rect.right, 0);
            LineTo(hdc, rect.right, rect.bottom);
            LineTo(hdc, 0, rect.bottom);
            LineTo(hdc, 0, 0);
            LineTo(hdc, rect.right, rect.bottom);
            MoveTo(hdc, 0, rect.bottom);
            LineTo(hdc, rect.right, 0);
            
            DrawText(hdc, "Canvas", -1, &rect, 
                        DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
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
            
            MoveTo(hdc, 0, 0);         
            LineTo(hdc, rect.right, 0);
            LineTo(hdc, rect.right, rect.bottom);
            LineTo(hdc, 0, rect.bottom);
            LineTo(hdc, 0, 0);
            LineTo(hdc, rect.right, rect.bottom);
            MoveTo(hdc, 0, rect.bottom);
            LineTo(hdc, rect.right, 0);
            
            DrawText(hdc, "Log", -1, &rect, 
                        DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}