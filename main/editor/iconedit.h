#ifndef _ICONEDIT_H_
#define _ICONEDIT_H_

#include <WINDOWS.H>
#include <windowsx.h>
#include <commdlg.h>
#include <stdlib.h>

#ifndef WIN31  
#include "windef.h"                     
#endif  

#include "utils.h"
#include "cmd.h"

long FAR PASCAL _export WndProcMain(HWND, UINT, UINT, LONG);
static char szNameApp[] = "CharacterCreator";
// extern BYTE huge *lpDib;











typedef enum ControlKeyGroup{
    CTRL_GROUP_CURSOR = 0,
    CTRL_GROUP_LH_TOOLBOX = 1,
    CTRL_GROUP_RH_TOOLBOX = 2
} ControlKeyGroup_e;

typedef enum MovementDirection{
    MOVEMENT_LEFT = 0,
    MOVEMENT_RIGHT = 1,
    MOVEMENT_UP = 2,
    MOVEMENT_DOWN = 3,
    MOVEMENT_UPRIGHT = 4,
    MOVEMENT_RIGHTUP = 4,
    MOVEMENT_UPLEFT = 5,
    MOVEMENT_LEFTUP = 5,
    MOVEMENT_DOWNRIGHT = 6,
    MOVEMENT_RIGHTDOWN = 6,
    MOVEMENT_DOWNLEFT = 7,
    MOVEMENT_LEFTDOWN = 7
} MovementDirection_e;


#endif
