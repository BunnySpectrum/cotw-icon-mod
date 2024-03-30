
#include "utils.h"

BOOL FAR PASCAL _export pixel_color_code_to_rgb(WORD code, COLORREF* color){
    if(code >= 16){
        return FALSE;
    }
    *color = pixelColorList[code];
    return TRUE;
}

void FAR PASCAL _export pixel_num_to_bitfield(int pixel, short* byteNum, short* byteOffset){
    *byteNum = pixel / 8;
    *byteOffset = pixel % 8;
}