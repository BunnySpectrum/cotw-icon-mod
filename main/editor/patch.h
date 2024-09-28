#ifndef _PATCH_H_
#define _PATCH_H_

#include "bunint.h"
#include <WINDOWS.H>
#include <windowsx.h>
#include <commdlg.h>
#include <stdlib.h>

// int patch(OPENFILENAME* pofnExe, OPENFILENAME* pofnIcon, uint16_t* result);
int patch(int hfileExe, int hfileIcon, uint16_t* result);


#endif
