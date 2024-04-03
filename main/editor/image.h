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

typedef struct {
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
    ColorTable_s colorTable;
    BYTE huge* lpDibBits;
} BitmapFields_s;

RGBQUAD colorTable16Colors[16] = {
    {0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x80, 0x00},
    {0x00, 0x80, 0x00, 0x00},
    {0x00, 0x80, 0x80, 0x00},
    {0x80, 0x00, 0x00, 0x00},
    {0x80, 0x00, 0x80, 0x00},
    {0x80, 0x80, 0x00, 0x00},
    {0x80, 0x80, 0x80, 0x00},
    {0xC0, 0xC0, 0xC0, 0x00},
    {0x00, 0x00, 0xFF, 0x00},
    {0x00, 0xFF, 0x00, 0x00},
    {0x00, 0xFF, 0xFF, 0x00},
    {0xFF, 0x00, 0x00, 0x00},
    {0xFF, 0x00, 0xFF, 0x00},
    {0xFF, 0xFF, 0x00, 0x00},
    {0xFF, 0xFF, 0xFF, 0x00},
};

// Starting point for this file was showdib.c by Charles Petzold

extern BYTE huge *lpDib;

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

void FAR PASCAL _export InspectBMP (HDC hdc, HBITMAP hBmp);

void FAR PASCAL _export CreateDIBBitmapFromFile (char* szFileName, BitmapFields_s* bmpFields);




#endif
