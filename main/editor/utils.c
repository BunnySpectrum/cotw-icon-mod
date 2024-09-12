
#include "utils.h"

BOOL FAR PASCAL _export pixel_color_code_to_rgb(WORD code, COLORREF* color){
    if(code >= 18){
        return FALSE;
    }
    *color = pixelColorList[code];
    return TRUE;
}

void FAR PASCAL _export pixel_num_to_bitfield(int pixel, short* byteNum, short* byteOffset){
    *byteNum = pixel / 8;
    *byteOffset = pixel % 8;
}

ImageFileType_e FAR PASCAL _export get_file_ext(const char* szFileName, int nFileExt){
    if(3 != strlen(&szFileName[nFileExt])){
        return IMAGE_OTHER;
    }

    if(0 == strcmpi(&szFileName[nFileExt], "BMP")){
        return IMAGE_BMP;
    }

    if(0 == strcmpi(&szFileName[nFileExt], "DIB")){
        return IMAGE_DIB;
    }

    if(0 == strcmpi(&szFileName[nFileExt], "ICO")){
        return IMAGE_ICO;
    }

    return IMAGE_OTHER;

}
