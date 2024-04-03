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


#endif
