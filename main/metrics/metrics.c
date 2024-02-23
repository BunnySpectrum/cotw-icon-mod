/*------------------------------------------
Modified from following example programs:
   FREEMEM.C -- Free Memory Display Program (c) Charles Petzold, 1992
   DIGCLOCK.C -- Digital Clock Program (c) Charles Petzold, 1992
  ------------------------------------------*/

#include <WINDOWS.H>
#include <stdio.h>
#include <time.h>

#include "metrics.h"


static WORD firstStaticInit[2] = {0x42, 0x20};
long FAR PASCAL _export WndProc (HWND, UINT, UINT, LONG) ;
void SizeTheWindow (short *, short *, short *, short *) ;
static int scratch[5]; 
static WORD lastStaticUnInit[2];

int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow){
     static char szAppName[] = "Metrics" ;
     HDC         hdc ;
     HWND        hwnd ;
     MSG         msg ;
     short       xStart, yStart, xClient, yClient ;
     WNDCLASS    wndclass ;


     __asm mov scratch[0], bp;
     __asm mov scratch[2], sp;
     // __asm mov scratch[4], cx;
     // __asm mov scratch[6], di;
     // __asm mov scratch[8], sp;
     

     if (!hPrevInstance)
          {
          wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
          wndclass.lpfnWndProc   = WndProc ;
          wndclass.cbClsExtra    = sizeof(WORD)*0 ;
          wndclass.cbWndExtra    = sizeof(WORD)*2 ;
          wndclass.hInstance     = hInstance ;
          wndclass.hIcon         = NULL ;
          wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
          wndclass.hbrBackground = GetStockObject (WHITE_BRUSH) ;
          wndclass.lpszMenuName  = NULL ;
          wndclass.lpszClassName = szAppName ;

	  RegisterClass (&wndclass) ;
          }

     SizeTheWindow (&xStart, &yStart, &xClient, &yClient) ;

     hwnd = CreateWindow (szAppName, szAppName,
                          WS_POPUP | WS_DLGFRAME | WS_SYSMENU,
                          xStart,  yStart,
                          xClient, yClient,
                          NULL, NULL, hInstance, NULL) ;
                         

     if (!SetTimer (hwnd, ID_TIMER, 1000, NULL))
          {
          MessageBox (hwnd, "Too many clocks or timers!",
                      szAppName, MB_ICONEXCLAMATION | MB_OK) ;
          return FALSE ;
          }

     ShowWindow (hwnd, SW_SHOWNOACTIVATE) ;
     UpdateWindow (hwnd) ;

     while (GetMessage (&msg, NULL, 0, 0))
          {
          TranslateMessage (&msg) ;
          DispatchMessage (&msg) ;
          }
     return msg.wParam ;
}


void SizeTheWindow (short *pxStart,  short *pyStart, short *pxClient, short *pyClient){
     HDC        hdc ;
     TEXTMETRIC tm ;

     hdc = CreateIC ("DISPLAY", NULL, NULL, NULL) ;
     GetTextMetrics (hdc, &tm) ;
     DeleteDC (hdc) ;

     *pxClient = 2 * GetSystemMetrics (SM_CXDLGFRAME) + 64*tm.tmAveCharWidth ;
     *pxStart  =     GetSystemMetrics (SM_CXSCREEN)   - *pxClient ;
     *pyClient = 2 * GetSystemMetrics (SM_CYDLGFRAME) + 2*tm.tmHeight ;
     *pyStart  =     GetSystemMetrics (SM_CYSCREEN)   - *pyClient ;
}

void WndPaint (HWND hwnd, HDC hdc, SystemMetrics_s sysMetrics, ProgramMetrics_s progMetrics, int* extra){
     char        cBuffer[80] ;
     RECT        rect ;
     short       nLength ;
     struct tm   *datetime ;
     time_t      lTime ;
     char sign;

     if(sysMetrics.sign == 1){
          sign = '+';
     }else{
          sign = '-';
     }
     
     nLength = wsprintf (cBuffer, "Sys %ld (%c%ld)\r\nProg %d: %ld", 
          sysMetrics.dwCurrFreeSpace, 
          sign, sysMetrics.dwDiffFree,
          progMetrics.handle, progMetrics.space);
     // nLength = wsprintf (cBuffer, "BP 0x%X, SP 0x%X", 
     // extra[0], extra[1]); 

     // DrawText (hdc, cBuffer, sprintf (cBuffer, "%.2f megs", dwFreeMem / 1024.0 / 1024.0), &rect, DT_WORDBREAK) ;

     GetClientRect (hwnd, &rect) ;
     DrawText (hdc, cBuffer, -1, &rect, DT_CENTER | DT_NOCLIP) ;
}

long FAR PASCAL _export WndProc (HWND hwnd, UINT message, UINT wParam,
                                                          LONG lParam)
     {
     // static DWORD  dwFreeMem, dwPrevMem ;
     static SystemMetrics_s sysMetrics;
     static DWORD dwPrevSysFreeSpace;

     static ProgramMetrics_s progMetrics;
     static DWORD dwPrevProgFreeSpace;

     static RECT   rect ;
     char          cBuffer [20] ;
     HDC           hdc ;
     PAINTSTRUCT   ps ;

     switch (message)
          {
          case WM_CREATE:
               sysMetrics.dwCurrFreeSpace = 0;
               dwPrevSysFreeSpace = 0;

               progMetrics.handle = 0;
               progMetrics.space = 0;
               dwPrevProgFreeSpace = 0;
               return 0;

          case WM_TIMER:
               sysMetrics.dwCurrFreeSpace = GetFreeSpace (0) ;

               if (sysMetrics.dwCurrFreeSpace != dwPrevSysFreeSpace){
                    InvalidateRect (hwnd, NULL, TRUE) ;
                    sysMetrics.dwDiffFree = ABS_DIFF(sysMetrics.dwCurrFreeSpace, dwPrevSysFreeSpace);
                    if(sysMetrics.dwCurrFreeSpace > dwPrevSysFreeSpace){
                         sysMetrics.sign = 1;
                    }else{
                         sysMetrics.sign = 0;
                    }
               }

               dwPrevSysFreeSpace = sysMetrics.dwCurrFreeSpace;
               return 0 ;

          case WM_PAINT:
               hdc = BeginPaint (hwnd, &ps) ;

               WndPaint (hwnd, hdc, sysMetrics, progMetrics, scratch);

               EndPaint (hwnd, &ps) ;
               return 0 ;


          case WM_DESTROY:
               KillTimer (hwnd, ID_TIMER) ;
               PostQuitMessage (0) ;
               return 0 ;
          }
     return DefWindowProc (hwnd, message, wParam, lParam) ;
     }
