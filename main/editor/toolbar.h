#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

long FAR PASCAL _export WndProcToolbar(HWND, UINT, UINT, LONG);
char szNameToolbar[] = "Toolbar";

#define TOOLBAR_ROWS 10
#define TOOLBAR_COLS 2


#endif
