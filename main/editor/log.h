#ifndef _LOG_H_
#define _LOG_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

#include <string.h>

#include "utils.h"

long FAR PASCAL _export WndProcLog(HWND, UINT, UINT, LONG);
void FAR PASCAL _export log_message(char* message);

extern char szNameLog[];
#define LOG_EXTRA_WORDS 0 
static char szLogLines[10][80];
static int logWriteIndex, logReadIndex;
#define LOG_LINE_MAX 4



#endif
