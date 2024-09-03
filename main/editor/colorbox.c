#include "colorbox.h"

char* colorLabels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"};
char szNameColorBox[] = "Color";

long FAR PASCAL _export WndProcColorBox(HWND hwnd, UINT message, UINT wParam, LONG lParam){
    HDC hdc;
    PAINTSTRUCT ps;
    RECT clientRect;
    static short cxBlock, cyBlock, cxChar, cyChar;
    short x, y;

    int colorCode;
    HWND hwndParent;


    static WORD activeColorCode;
    static HWND hwndButton[COLORBOX_COLS * COLORBOX_ROWS];
    LPDRAWITEMSTRUCT lpdis;

    switch(message){
        case WM_CREATE:
            SetWindowWord(hwnd, ColorBoxWordForeColor, PixelColorCodeBlack); 
            SetWindowWord(hwnd, ColorBoxWordBackColor, PixelColorCodeWhite);  
            activeColorCode = PixelColorCodeBlack;

            cxChar = LOWORD(GetDialogBaseUnits());
            cyChar = HIWORD(GetDialogBaseUnits());

            for(x=0; x<COLORBOX_COLS; x++){
                for(y=0; y<COLORBOX_ROWS; y++){
                    colorCode = x + y*COLORBOX_COLS;
                    hwndButton[x + y*COLORBOX_COLS] = CreateWindow("button", 
                                                                    "", 
                                                                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                                                    0, 0,
                                                                    COLOR_BTN_SIZE, COLOR_BTN_SIZE,
                                                                    hwnd, colorCode,
                                                                    ((LPCREATESTRUCT) lParam) -> hInstance, NULL);
                }
            }

            return 0;
        
        case WM_SIZE:{
            cxBlock = LOWORD(lParam) / COLORBOX_COLS;
            cyBlock = HIWORD(lParam) / COLORBOX_ROWS;

            // Position the windows for the color pickers
            for(x=0; x<COLORBOX_COLS; x++){
                for(y=0; y<COLORBOX_ROWS; y++){
                    colorCode = x + y*COLORBOX_COLS;
                    MoveWindow(hwndButton[colorCode], 
                    x*cxBlock+1, 
                    y*cyBlock + 4, 
                    cxBlock-1, cyBlock - 8, TRUE);
                }
            }
            return 0;}

        case WM_PAINT:{
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &clientRect);   

            // Outer border
            Rectangle(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
        {
            if (HIWORD(lParam) == BN_CLICKED)
            {
                activeColorCode = wParam;
                hwndParent = GetParent(hwnd);

                SendMessage(hwndParent, WM_COMMAND, GetWindowWord(hwnd, GWW_ID), activeColorCode);

                InvalidateRect(hwnd, NULL, FALSE);
            }

            

            return 0;
        }

        case WM_DRAWITEM:
        {
            HBRUSH hBrush;
            COLORREF newColor;
            short cx, cy;

            lpdis = (LPDRAWITEMSTRUCT) lParam;
            pixel_color_code_to_rgb(lpdis->CtlID, &newColor);

            

            if (lpdis->CtlID == activeColorCode)
            {
                hBrush = CreateHatchBrush(HS_DIAGCROSS, newColor);
            }
            else
            {
                hBrush = CreateSolidBrush(newColor);
            }

            FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
            FrameRect(lpdis->hDC, &lpdis->rcItem, GetStockObject(BLACK_BRUSH));

            DeleteObject(hBrush);

            if(lpdis->itemState & ODS_SELECTED){
                InvertRect(lpdis->hDC, &lpdis->rcItem);
            }

            cx = lpdis->rcItem.right - lpdis->rcItem.left;
            cy = lpdis->rcItem.bottom - lpdis->rcItem.top;

            if(lpdis->CtlID == activeColorCode){
                lpdis->rcItem.left += cx/8;
                lpdis->rcItem.top += cy/8;
                lpdis->rcItem.right -= cx/8;
                lpdis->rcItem.bottom -= cy/8;

                FrameRect(lpdis->hDC, &lpdis->rcItem, GetStockObject(BLACK_BRUSH));
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

