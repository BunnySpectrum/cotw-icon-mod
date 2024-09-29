#ifndef _PATCH_H_
#define _PATCH_H_

#include <WINDOWS.H>
#include <commdlg.h>
#include <stdlib.h>
#include <windowsx.h>

#include "bunint.h"

// int patch(OPENFILENAME* pofnExe, OPENFILENAME* pofnIcon, uint16_t* result);
int patch(int hfileExe, int hfileIcon, uint16_t* result);

void PatchInit(HWND hwnd);
BOOL PatchFileOpenDlg(HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName);

#endif
