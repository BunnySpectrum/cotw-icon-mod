#ifndef _LOG_H_
#define _LOG_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

long FAR PASCAL _export WndProcLog(HWND, UINT, UINT, LONG);

char szNameLog[] = "Log";
#define LOG_EXTRA_WORDS 0 



#endif
