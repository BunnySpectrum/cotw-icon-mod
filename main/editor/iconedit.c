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
#include "utils.h"
#include "image.h"



static char szBuffer[80];


static short nLength;
static HWND debugWindow;


static HWND floodHWND;
static HDC floodHDC;


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









int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow){
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
        wndclass.lpszMenuName = "CHARCRTR";
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

    // For ReadDib
    static char szFileName[_MAX_PATH],
        szTitleName[_MAX_FNAME + _MAX_EXT];
    static char *szFilter[] = {"ICON Files (*.ICO)", "*.ico",
                                "Bitmap Files (*.BMP)", "*.bmp",
                               "DIB Files (*.DIB)", "*.dib",
                               ""};
    
    static OPENFILENAME ofn;
    static HBITMAP hBitmap;
    static short cxClient, cyClient, xClient, yClient;
    // static WORD wDisplay = IDM_ACTUAL;
    // BYTE huge *lpDibBits;
    // short cxDib, cyDib;

    // Menudemo (Petzold, ch 9, pg 349)
    static int wColorID[5] = {WHITE_BRUSH, LTGRAY_BRUSH, GRAY_BRUSH, DKGRAY_BRUSH, BLACK_BRUSH};
    static WORD wSelection = IDM_WHITE;
    HMENU hMenu;
    static BitmapFields_s inputBitmap, outputBitmap;
    static IconFields_s inputIcon, outputIcon;
    static ICONDIRENTRY_s iconEntry;
    static ImageMask_s imageMask;
    int fileType;

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

            // ReadDib section
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = szFilter[0];
            ofn.lpstrFile = szFileName;
            ofn.nMaxFile = _MAX_PATH;
            ofn.lpstrFileTitle = szTitleName;
            ofn.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
            ofn.lpstrDefExt = "ico";

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

            //ReadDib section
            xClient = cxBlock + max(0, (4*cxBlock - canvasSize)/2);
            yClient = max(0, (5*cyBlock - canvasSize)/2);
            cxClient = canvasSize;
            cyClient = canvasSize;
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
            hMenu = GetMenu(hwnd);

            switch(wParam){
                case IDM_NEW:
                case IDM_OPEN:
                {
                    if (GetOpenFileName(&ofn))
                    {
                        
                        fileType = get_file_ext(szFileName, ofn.nFileExtension);
                        
                        switch(fileType){
                            case IMAGE_DIB: //no break on purpose
                            case IMAGE_BMP:
                                // MessageBox(hwnd, "BMP", szNameApp, MB_OK);

                                if(RC_SUCCESS != LoadBMPFile(szFileName, &inputBitmap)){
                                    MessageBox(hwnd, "Unable to load BMP", szNameApp, MB_ICONEXCLAMATION | MB_OK);
                                    return 0;
                                }
                                
                                if(RC_SUCCESS != copy_bmp_to_canvas(inputBitmap)){
                                    MessageBox(hwnd, "Unable to copy BMP to canvas", szNameApp, MB_ICONEXCLAMATION | MB_OK);
                                    return 0;
                                }
                            
                                break;

                            // case IMAGE_DIB:
                            //     MessageBox(hwnd, "DIB", szNameApp, MB_OK);

                            //     LoadBMP(szFileName, &inputBitmap);
                            //     break;

                            case IMAGE_ICO:
                                // MessageBox(hwnd, "ICO", szNameApp, MB_OK);

                                if(RC_SUCCESS != LoadIconFile(szFileName, &inputIcon)){
                                    MessageBox(hwnd, "Unable to load icon", szNameApp, MB_ICONEXCLAMATION | MB_OK);
                                    return 0;
                                }
                                
                                if(RC_SUCCESS != copy_icon_to_canvas(inputIcon)){
                                    MessageBox(hwnd, "Unable to copy icon to canvas", szNameApp, MB_ICONEXCLAMATION | MB_OK);
                                    return 0;
                                }


                                break;

                            default:
                                MessageBox(hwnd, "Unsupported file", szNameApp, MB_OK);
                                return 0;
                                break;

                        }

                       
                            
                        

                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    return 0;
                }
                case IDM_SAVE:
                case IDM_SAVEAS:
                    if (GetSaveFileName(&ofn))
                    {
                        fileType = get_file_ext(szFileName, ofn.nFileExtension);

                        switch(fileType){
                        case IMAGE_DIB: // no break on purpose
                        case IMAGE_BMP:
                            // BMP
                            outputBitmap.bmfh.bfType = 0x4d42;
                            outputBitmap.bmfh.bfSize = 0x276;
                            outputBitmap.bmfh.bfReserved1 = 0x0;
                            outputBitmap.bmfh.bfReserved2 = 0x0;
                            outputBitmap.bmfh.bfOffBits = 0x76;

                            outputBitmap.bmih.biSize = 0x28;
                            outputBitmap.bmih.biWidth = 0x20;
                            outputBitmap.bmih.biHeight = 0x20;
                            outputBitmap.bmih.biPlanes = 0x1;
                            outputBitmap.bmih.biBitCount = 0x4;
                            outputBitmap.bmih.biCompression = 0x0;
                            outputBitmap.bmih.biSizeImage = 0x200;
                            outputBitmap.bmih.biXPelsPerMeter = 0xEC4;
                            outputBitmap.bmih.biYPelsPerMeter = 0xEC4;
                            outputBitmap.bmih.biClrUsed = 0x0;
                            outputBitmap.bmih.biClrImportant = 0x0;

                            outputBitmap.colorTable.dwColorTableSize = sizeof(colorTable16Colors);
                            outputBitmap.colorTable.lpColorData = (BYTE huge *)colorTable16Colors;

                            outputBitmap.lpDibBits = (BYTE huge *)GlobalAllocPtr(GMEM_MOVEABLE, outputBitmap.bmih.biSizeImage);
                            if (outputBitmap.lpDibBits == NULL)
                            {
                                MessageBox(NULL, "Unable to make bit pointer", "Save BMP", MB_OK);
                                return 0;
                            }

                            copy_canvas_to_img(outputBitmap.lpDibBits, IMAGE_BMP);
                            WriteDIBBitmapToFile(szFileName, outputBitmap);

                            break;

                        case IMAGE_ICO:

                            // ICO file
                            outputIcon.idir.wRsvd = 0;
                            outputIcon.idir.wType = TYPE_ICON;
                            outputIcon.idir.wIconCount = 1;

                            iconEntry.bWidth = 0x20;
                            iconEntry.bHeight = 0x20;
                            iconEntry.bColorCount = 0x10;
                            iconEntry.bRsvd = 0x0;
                            iconEntry.wPlanes = 0x0;
                            iconEntry.wBitCount = 0x00;
                            iconEntry.dwSize = 0x2E8;
                            iconEntry.dwOffset = 0x16;

                            outputBitmap.bmih.biSize = 0x28;
                            outputBitmap.bmih.biWidth = 0x20;
                            outputBitmap.bmih.biHeight = 0x20 * 2;
                            outputBitmap.bmih.biPlanes = 0x1;
                            outputBitmap.bmih.biBitCount = 0x4;
                            outputBitmap.bmih.biCompression = 0x0;
                            outputBitmap.bmih.biSizeImage = 0x280; // 0x80 larger
                            outputBitmap.bmih.biXPelsPerMeter = 0x0;
                            outputBitmap.bmih.biYPelsPerMeter = 0x0;
                            outputBitmap.bmih.biClrUsed = 0x0;
                            outputBitmap.bmih.biClrImportant = 0x0;

                            // color table should be at 0x3E
                            outputBitmap.colorTable.dwColorTableSize = sizeof(colorTable16Colors);
                            outputBitmap.colorTable.lpColorData = (BYTE huge *)colorTable16Colors;

                            // image bits should start at 0x7E, through 0x27D
                            outputBitmap.lpDibBits = (BYTE huge *)GlobalAllocPtr(GMEM_MOVEABLE, outputBitmap.bmih.biSizeImage);
                            if (outputBitmap.lpDibBits == NULL)
                            {
                                MessageBox(NULL, "Unable to make bit pointer", "Save Icon", MB_OK);
                                return 0;
                            }
                            copy_canvas_to_img(outputBitmap.lpDibBits, IMAGE_ICO);

                            // XOR masking should start at 0x28E
                            build_image_mask_from_canvas(&imageMask);
                            if (imageMask.lpImageMask == NULL)
                            {
                                MessageBox(NULL, "Unable to make mask pointer", "Save Icon", MB_OK);
                                return 0;
                            }

                            WriteICOToFile(szFileName, outputIcon, iconEntry, outputBitmap, imageMask);

                            break;

                        default:
                            MessageBox(hwnd, "Unsupported file", szNameApp, MB_OK);
                            return 0;
                            break;
                        }
                    }
                    return 0;

                case IDM_EXIT:
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return 0;

                case IDM_UNDO:
                case IDM_REDO:
                case IDM_WHITE:
                case IDM_LTGRAY:
                case IDM_GRAY:
                case IDM_DKGRAY:
                case IDM_BLACK:
                {
                    CheckMenuItem(hMenu, wSelection, MF_UNCHECKED);
                    wSelection = wParam;
                    CheckMenuItem(hMenu, wSelection, MF_CHECKED);

                    SetClassWord(hwnd, GCW_HBRBACKGROUND,
                    GetStockObject(wColorID[wParam - IDM_WHITE]));

                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
                }

                case IDM_HELP:
                    MessageBox(hwnd, "Help not yet implemented.", szNameApp, MB_ICONEXCLAMATION | MB_OK);
                    return 0;

                case IDM_ABOUT:
                    MessageBox(hwnd, "Icon Editor.", szNameApp, MB_ICONINFORMATION | MB_OK);
                    return 0;

                case CHILD_ID_COLORBOX:
                    SetWindowWord(hwndCanvas, CanvasWordForeColor, LOWORD(lParam));
                    SetWindowWord(hwndCanvas, CanvasWordBackColor, HIWORD(lParam));
                    return 0;

                case CHILD_ID_TOOLBAR:
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
                    return 0;

                case CHILD_ID_CANVAS:
                    InvalidateRect(hwndLog, NULL, FALSE);
                    return 0;
            }
            break;
        }

        case WM_SETFOCUS:
            ShowCursor(TRUE);
            return 0;

        case WM_KILLFOCUS:
            ShowCursor(FALSE);
            return 0;

        case WM_DESTROY:
            if (inputBitmap.lpDibBits != NULL)
            {
                GlobalFreePtr(inputBitmap.lpDibBits);
            }

            if (inputBitmap.colorTable.lpColorData != NULL)
            {
                GlobalFreePtr(inputBitmap.colorTable.lpColorData);
            }
            
            if (outputBitmap.lpDibBits != NULL)
            {
                GlobalFreePtr(inputBitmap.lpDibBits);
            }

            if (imageMask.lpImageMask != NULL)
            {
                GlobalFreePtr(imageMask.lpImageMask);
            }

            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


