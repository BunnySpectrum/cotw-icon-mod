
#include "toolbar.h"

extern HANDLE hInst;
char szNameToolbar[] = "Toolbar";
// static char szBuffer[80];



void DrawBitmapT(HDC hdc, HBITMAP hBitmap, short xStart, short yStart, short xEnd, short yEnd){
    BITMAP bm;
    HBITMAP hOld;
    HDC hdcMem;
    POINT ptSize, ptOrg;
    int width, height;

    width = xEnd - xStart;
    height = yEnd - yStart;
    if(!hdc){
        MessageBeep(1);
    }

    hdcMem = CreateCompatibleDC(hdc);
    hOld = SelectObject(hdcMem, hBitmap);
    SetMapMode(hdcMem, GetMapMode(hdc));

    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdc, &ptSize, 1);

    ptOrg.x = 0;
    ptOrg.y = 0;
    DPtoLP(hdcMem, &ptOrg, 1);

    // BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, SRCCOPY);
    // StretchBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, bm.bmWidth, bm.bmHeight, SRCCOPY);
    StretchBlt(hdc, xStart, yStart, width, height, hdcMem, ptOrg.x, ptOrg.y, bm.bmWidth, bm.bmHeight, SRCCOPY);

    SelectObject(hdcMem, hOld);
    DeleteDC(hdcMem);
}



long FAR PASCAL _export WndProcToolbar(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    static short cxBlock, cyBlock, cxChar, cyChar;
    short x, y;
    char cBuffer[16] ;
    short nLength ;
    static WORD selectedTool, activeTool;
    HWND hwndParent;
    LPDRAWITEMSTRUCT lpdis;
    static HWND hwndButton[TOOLBAR_COLS * TOOLBAR_ROWS];

    switch(message){
        case WM_CREATE:{
            SetWindowWord(hwnd, ToolbarWordTool, ToolbarToolBrush);
            selectedTool = ToolbarToolBrush;  
            activeTool = selectedTool;   

            cxChar = LOWORD(GetDialogBaseUnits());
            cyChar = HIWORD(GetDialogBaseUnits());

            for(x=0; x<TOOLBAR_COLS; x++){
                for(y=0; y<TOOLBAR_ROWS; y++){
                    WORD toolCode = min(x + y*TOOLBAR_COLS, ToolbarToolMAX);
                    hwndButton[x + y*TOOLBAR_COLS] = CreateWindow("button", 
                                                                    "", 
                                                                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                                                    0, 0,
                                                                    cxBlock, cyBlock,
                                                                    hwnd, toolCode,
                                                                    ((LPCREATESTRUCT) lParam) -> hInstance, NULL);
                }
            }




            return 0;
        }
        
        case WM_SIZE:{
            short cSideLength;
            cxBlock = LOWORD(lParam) / TOOLBAR_COLS;
            cyBlock = HIWORD(lParam) / TOOLBAR_ROWS;

            // nLength = wsprintf(szBuffer, "Toolbox hInst %d", hInst);
            // MessageBox(hwnd, szBuffer, "Main", MB_OK);

            // cSideLength = min(cxBlock, cyBlock);
            cSideLength = cxBlock;

            // Position the windows for the tools
            for(x=0; x<TOOLBAR_COLS; x++){
                for(y=0; y<TOOLBAR_ROWS; y++){
                    WORD toolCode = x + y*TOOLBAR_COLS;
                    if(toolCode >= ToolbarToolMAX){
                        continue;
                    }
                    MoveWindow(hwndButton[toolCode], 
                    x*cSideLength, 
                    y*cSideLength, 
                    cSideLength - 1, cSideLength - 1, TRUE);
                }
            }


            return 0;
        }


        case WM_PAINT:{
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);   

            // Outer border
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
        {
            if (HIWORD(lParam) == BN_CLICKED)
            {
                selectedTool = wParam;
                hwndParent = GetParent(hwnd);

                SendMessage(hwndParent, WM_COMMAND, GetWindowWord(hwnd, GWW_ID), selectedTool);
                // MessageBeep(1);
                InvalidateRect(hwnd, NULL, FALSE);
            }

            

            return 0;
        }

        case WM_DRAWITEM:
        {
            HBRUSH hBrush;
            WORD tool;
            HDC hdc;
            short cx, cy;

            lpdis = (LPDRAWITEMSTRUCT) lParam;
            tool = lpdis->CtlID;
            hdc = lpdis->hDC;
            
            

            if (tool == selectedTool)
            {
                hBrush = GetStockObject(GRAY_BRUSH);
            }
            else
            {
                hBrush = GetStockObject(BLACK_BRUSH);
            }
            nLength = wsprintf (cBuffer, "%s", toolbarToolNames[min(tool, ToolbarToolMAX)]);

            if(tool == selectedTool){
                HBITMAP hImg = LoadBitmap(GetModuleHandle(NULL), "RCSUNKEN");
                // HBITMAP hImg = LoadBitmap(hInst, "RCSUNKEN");
                // DrawBitmapT(hdc, hImg, 
                //     lpdis->rcItem.left, 
                //     lpdis->rcItem.top);
                DrawBitmapT(hdc, hImg, 
                    lpdis->rcItem.left, 
                    lpdis->rcItem.top,
                    lpdis->rcItem.right, 
                    lpdis->rcItem.bottom); 
                    DeleteObject(hImg);

            }else{
                HBITMAP hImg = LoadBitmap(GetModuleHandle(NULL), "RCRAISED");
                // HBITMAP hImg = LoadBitmap(hInst, "RCRAISED");
                // DrawBitmapT(hdc, hImg, 
                //     lpdis->rcItem.left, 
                //     lpdis->rcItem.top);
                DrawBitmapT(hdc, hImg, 
                    lpdis->rcItem.left, 
                    lpdis->rcItem.top,
                    lpdis->rcItem.right, 
                    lpdis->rcItem.bottom); 
                    DeleteObject(hImg);

            }

            switch(tool){
                case ToolbarToolBrush:{
                    HBITMAP hImg = LoadBitmap(hInst, "RCBRUSH");
                // DrawBitmapT(hdc, hImg, 
                //     lpdis->rcItem.left, 
                //     lpdis->rcItem.top);
                    DrawBitmapT(hdc, hImg, 
                        lpdis->rcItem.left + 2, 
                        lpdis->rcItem.top + 2,
                        lpdis->rcItem.right - 2, 
                        lpdis->rcItem.bottom - 2); 
                    DeleteObject(hImg);
                    break;
                }
                case ToolbarToolLine:{
                    HBITMAP hImg = LoadBitmap(hInst, "RCLINE");
                // DrawBitmapT(hdc, hImg, 
                //     lpdis->rcItem.left, 
                //     lpdis->rcItem.top);
                    DrawBitmapT(hdc, hImg, 
                        lpdis->rcItem.left + 2, 
                        lpdis->rcItem.top + 2,
                        lpdis->rcItem.right - 2, 
                        lpdis->rcItem.bottom - 2); 
                    DeleteObject(hImg);
                    break;
                }
                default:
                    DrawText (lpdis->hDC, cBuffer, -1, &lpdis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOCLIP) ;
            }
     
            // FrameRect(lpdis->hDC, &lpdis->rcItem, hBrush);

            DeleteObject(hBrush);

            if(lpdis->itemState & ODS_SELECTED){
                InvertRect(lpdis->hDC, &lpdis->rcItem);
            }





            return 0;
        }
        
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
