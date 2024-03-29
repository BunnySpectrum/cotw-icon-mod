#ifndef _ICONEDIT_H_
#define _ICONEDIT_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif  

long FAR PASCAL _export WndProcMain(HWND, UINT, UINT, LONG);

#define CHILD_ID_CANVAS 1
#define CHILD_ID_LOG 2
#define CHILD_ID_TOOLBAR 3
#define CHILD_ID_COLORBOX 4

// Macro functions
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define ALIGN(x, y) ((x) - (x)%(y))



// Color definitions
#define COLOR_BLACK (RGB(0, 0, 0))
#define COLOR_GRAY (RGB(128, 128, 128))

#define COLOR_RED (RGB(255, 0, 0))
#define COLOR_MAROON (RGB(128, 0, 0))

#define COLOR_LIME (RGB(0, 255, 0))
#define COLOR_GREEN (RGB(0, 128, 0))

#define COLOR_YELLOW (RGB(255, 255, 0))
#define COLOR_OLIVE (RGB(128, 128, 0))

#define COLOR_BLUE (RGB(0, 0, 255))
#define COLOR_NAVY (RGB(0, 0, 128))

#define COLOR_FUCHIA (RGB(255, 0, 255))
#define COLOR_PURPLE (RGB(128, 0, 128))

#define COLOR_AQUA (RGB(0, 255, 255))
#define COLOR_TEAL (RGB(0, 128, 128))

#define COLOR_WHITE (RGB(255, 255, 255))
#define COLOR_SILVER (RGB(192, 192, 192))


COLORREF pixelColorList[] = {COLOR_BLACK, COLOR_MAROON, COLOR_GREEN, COLOR_OLIVE, COLOR_NAVY, COLOR_PURPLE, COLOR_TEAL, COLOR_SILVER,
                            COLOR_GRAY, COLOR_RED, COLOR_LIME, COLOR_YELLOW, COLOR_BLUE, COLOR_FUCHIA, COLOR_AQUA, COLOR_WHITE};



typedef enum PixelColorCode{
    PixelColorCodeBlack = 0,
    PixelColorCodeGray = 8,
    
    PixelColorCodeMaroon = 1,
    PixelColorCodeRed = 9,
    
    PixelColorCodeGreen = 2,
    PixelColorCodeLime = 10,
    
    PixelColorCodeOlive = 3,
    PixelColorCodeYellow = 11,
    
    PixelColorCodeNavy = 4,
    PixelColorCodeBlue = 12,
    
    PixelColorCodePurple = 5,
    PixelColorCodeFuchia = 13,
    
    PixelColorCodeTeal = 6,
    PixelColorCodeAqua = 14,

    PixelColorCodeSilver = 7,
    PixelColorCodeWhite = 15
} PixelColorCode_e;



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
