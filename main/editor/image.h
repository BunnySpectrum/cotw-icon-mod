#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <stdlib.h>

#ifndef WIN31  
#include "windef.h"                     
#endif  

#include "utils.h"

typedef struct {
    BYTE huge* lpColorData;
    DWORD dwColorTableSize;
} ColorTable_s;

// hardcoding for now, since so much is assumed we have a 32x32 image
#define ICON_MASK_SIZE 0x80

typedef struct {
    BYTE far* lpImageMask;
    WORD wMaskSize;
} ImageMask_s;

typedef struct {
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
    ColorTable_s colorTable;
    BYTE huge* lpDibBits;
} BitmapFields_s;

// at 0x36 in BMP, at 0x40 in ICO
extern RGBQUAD colorTable16Colors[16];

#define TYPE_ICON 1
#define TYPE_CUR 2

typedef struct{
    WORD wRsvd;
    WORD wType;
    WORD wIconCount;
} ICONDIR_s;

typedef struct{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bRsvd;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwSize;
    DWORD dwOffset;
} ICONDIRENTRY_s;

typedef struct{
    ICONDIR_s idir;
    ICONDIRENTRY_s iDirEntry; //only support one for now
    BITMAPINFOHEADER bmih;
    ColorTable_s colorTable;
    BYTE huge* lpDibBits;
    ImageMask_s imageMask;
} IconFields_s;

// Starting point for this file was showdib.c by Charles Petzold



// Return biSize from bitmap info header
DWORD GetDibInfoHeaderSize (BYTE huge * lpDib);

// Return bcWidth or biWidth from bitmap core header
WORD GetDibWidth (BYTE huge * lpDib);

// Return bcHeight or biHeight from bitmap core header
WORD GetDibHeight (BYTE huge * lpDib);


// Return pointer to start of image byte data
BYTE huge* FAR PASCAL _export GetDibBitsAddr (BYTE huge * lpDib);


// Read file into memory
BYTE huge* FAR PASCAL _export ReadDib (char * szFileName);

void FAR PASCAL _export WriteDIBBitmapToFile (char * szFileName, BitmapFields_s bmpFields);
void FAR PASCAL _export WriteICOToFile (char * szFileName, IconFields_s iconFields, ICONDIRENTRY_s iconEntry, BitmapFields_s bmpFields, ImageMask_s imageMask);



ReturnCode_e FAR PASCAL _export LoadBMPFile (char* szFileName, BitmapFields_s* bmpFields);
ReturnCode_e FAR PASCAL _export LoadIconFile (char* szFileName, IconFields_s* iconFields);



#endif
