#ifndef _UTILS_H_
#define _UTILS_H_

#include <WINDOWS.H>
#include <string.h>

#ifndef WIN31  
#include "windef.h"                     
#endif 

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define ALIGN(x, y) ((x) - (x)%(y))

BOOL FAR PASCAL _export pixel_color_code_to_rgb(WORD code, COLORREF* color);
void FAR PASCAL _export pixel_num_to_bitfield(int pixel, short* byteNum, short* byteOffset);



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


static COLORREF pixelColorList[] = {COLOR_BLACK, COLOR_MAROON, COLOR_GREEN, COLOR_OLIVE, COLOR_NAVY, COLOR_PURPLE, COLOR_TEAL, COLOR_SILVER,
                            COLOR_GRAY, COLOR_RED, COLOR_LIME, COLOR_YELLOW, COLOR_BLUE, COLOR_FUCHIA, COLOR_AQUA, COLOR_WHITE,
                            COLOR_WHITE, COLOR_WHITE};



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
    PixelColorCodeWhite = 15,

    PixelColorCodeTransparent = 16,
    PixelColorCodeInvert = 17
} PixelColorCode_e;

typedef enum{
    IMAGE_OTHER = 0,
    IMAGE_BMP,
    IMAGE_DIB,
    IMAGE_ICO
} ImageFileType_e;

ImageFileType_e FAR PASCAL _export get_file_ext(const char* szFileName, int nFileExt);


typedef enum{
    RC_ERROR = 0,
    RC_SUCCESS
} ReturnCode_e;

#endif
