#ifndef _ICONEDIT_H_
#define _ICONEDIT_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif  

long FAR PASCAL _export WndProcMain(HWND, UINT, UINT, LONG);

#define ID_TIMER    1

typedef struct ProgramMetrics{
    HWND handle;
    DWORD space;
} ProgramMetrics_s;


typedef struct SystemMetrics{
    DWORD dwCurrFreeSpace;
    DWORD dwDiffFree;
    BOOL sign;    
} SystemMetrics_s;

// Macro functions
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define ABS_DIFF(a, b) (((a) > (b)) ? (a-b) : (b-a))
#define ALIGN(x, y) ((x) - (x)%(y))



#endif
