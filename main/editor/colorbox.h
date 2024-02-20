#ifndef _COLORBOX_H_
#define _COLORBOX_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

long FAR PASCAL _export WndProcColorBox(HWND, UINT, UINT, LONG);

char szNameColorBox[] = "Color";

#define COLORBOX_COLS 8
#define COLORBOX_ROWS 2


#endif
