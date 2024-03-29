#include "log.h"


void FAR PASCAL _export log_message(char* message){

    strcpy(szLogLines[logWriteIndex], message);

    logWriteIndex = (logWriteIndex+1)%LOG_LINE_MAX;

    if(logWriteIndex == logReadIndex){
        logReadIndex = (logReadIndex + 1)%LOG_LINE_MAX;
    }

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

            logWriteIndex = logReadIndex = 0;
            log_message("Hello world!");

            hdc = GetDC(hwnd);

            GetTextMetrics(hdc, &tm);
            cxChar = tm.tmAveCharWidth;
            cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2)*cxChar/2;
            cyChar = tm.tmHeight + tm.tmExternalLeading;

            ReleaseDC(hwnd, hdc);

            nMaxWidth = 80 * cxChar;
            // MessageBox(NULL, "Log create", "Log", MB_OK);
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

            for(i=logReadIndex; ((i%LOG_LINE_MAX)!=logWriteIndex) && ((i-logReadIndex + nVscrollPos) <  nVscrollMax); i++){
                x = cxChar * (1-nHscrollPos);
                y = cyChar * (1-nVscrollPos+i-logReadIndex);

                TextOut(hdc, x, y, szLogLines[i%LOG_LINE_MAX], lstrlen(szLogLines[i%LOG_LINE_MAX]));
            }
            EndPaint(hwnd, &ps);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

