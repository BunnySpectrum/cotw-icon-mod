
#include "image.h"

void FAR PASCAL _export InspectBMP (HDC hdc, HBITMAP hBmp){

}


DWORD GetDibInfoHeaderSize(BYTE huge *lpDib)
{
    return ((BITMAPINFOHEADER huge *)lpDib)->biSize;
}

WORD GetDibWidth(BYTE huge *lpDib)
{
    if (GetDibInfoHeaderSize(lpDib) == sizeof(BITMAPCOREHEADER))
    {
        return (WORD)(((BITMAPCOREHEADER huge *)lpDib)->bcWidth);
    }
    else
    {
        return (WORD)(((BITMAPINFOHEADER huge *)lpDib)->biWidth);
    }
}

WORD GetDibHeight(BYTE huge *lpDib)
{
    if (GetDibInfoHeaderSize(lpDib) == sizeof(BITMAPCOREHEADER))
    {
        return (WORD)(((BITMAPCOREHEADER huge *)lpDib)->bcHeight);
    }
    else
    {
        return (WORD)(((BITMAPINFOHEADER huge *)lpDib)->biHeight);
    }
}

BYTE huge* FAR PASCAL _export GetDibBitsAddr(BYTE huge *lpDib)
{
    DWORD dwNumColors, dwColorTableSize;
    WORD wBitCount;

    if (GetDibInfoHeaderSize(lpDib) == sizeof(BITMAPCOREHEADER))
    {
        wBitCount = ((BITMAPCOREHEADER huge *)lpDib)->bcBitCount;
        // dwNumColors = (wBitCount != 24) ? (1L << wBitCount) : (0);
        if (wBitCount != 24)
        {
            dwNumColors = 1L << wBitCount;
        }
        else
        {
            dwNumColors = 0;
        }

        dwColorTableSize = dwNumColors * sizeof(RGBTRIPLE);
    }
    else
    {
        wBitCount = ((BITMAPINFOHEADER huge *)lpDib)->biBitCount;

        if (GetDibInfoHeaderSize(lpDib) >= 36)
        {
            dwNumColors = ((BITMAPINFOHEADER huge *)lpDib)->biClrUsed;
        }
        else
        {
            dwNumColors = 0;
        }

        if (dwNumColors == 0)
        {
            // dwNumColors = (wBitCount != 24) ? (1L << wBitCount) : (0);
            if (wBitCount != 24)
                dwNumColors = 1L << wBitCount;
            else
                dwNumColors = 0;
        }

        dwColorTableSize = dwNumColors * sizeof(RGBQUAD);
    }

    return lpDib + GetDibInfoHeaderSize(lpDib) + dwColorTableSize;
}

// Read a DIB from a file into memory

BYTE huge* FAR PASCAL _export ReadDib(char *szFileName)
{
    BITMAPFILEHEADER bmfh;
    BYTE huge *lpDib;
    DWORD dwDibSize, dwOffset, dwHeaderSize;
    int hFile;
    WORD wDibRead;

    if (-1 == (hFile = _lopen(szFileName, OF_READ | OF_SHARE_DENY_WRITE)))
    {
        MessageBox(NULL, "Failed to open", "ReadDib", MB_OK);
        return NULL;
    }

    if (_lread(hFile, (LPSTR)&bmfh, sizeof(BITMAPFILEHEADER)) != sizeof(BITMAPFILEHEADER))
    {
        _lclose(hFile);
        MessageBox(NULL, "Failed to read", "ReadDib", MB_OK);
        return NULL;
    }

    if (bmfh.bfType != *(WORD *)"BM")
    {
        _lclose(hFile);
        MessageBox(NULL, "Not a BMP", "ReadDib", MB_OK);
        return NULL;
    }

    dwDibSize = bmfh.bfSize - sizeof(BITMAPFILEHEADER);

    lpDib = (BYTE huge *)GlobalAllocPtr(GMEM_MOVEABLE, dwDibSize);

    if (lpDib == NULL)
    {
        _lclose(hFile);
        MessageBox(NULL, "lpDib was null", "ReadDib", MB_OK);
        return NULL;
    }

    dwOffset = 0;

    while (dwDibSize > 0)
    {
        wDibRead = (WORD)min(32768ul, dwDibSize);

        if (wDibRead != _lread(hFile, (LPSTR)(lpDib + dwOffset), wDibRead))
        {
            _lclose(hFile);
            GlobalFreePtr(lpDib);
            MessageBox(NULL, "Wrong number read", "ReadDib", MB_OK);
            return NULL;
        }

        dwDibSize -= wDibRead;
        dwOffset += wDibRead;
    }

    _lclose(hFile);

    dwHeaderSize = GetDibInfoHeaderSize(lpDib);

    if (dwHeaderSize < 12 || (dwHeaderSize > 12 && dwHeaderSize < 16))
    {
        GlobalFreePtr(lpDib);
        MessageBox(NULL, "Header isn't a BITMAPINFOHEADER", "ReadDib", MB_OK);
        return NULL;
    }

    return lpDib;
}


void FAR PASCAL _export CreateDIBBitmapFromFile (char* szFileName, BitmapFields_s* bmpFields){
    // BITMAPFILEHEADER bmfh;
    BYTE huge *lpDib, huge *lpColorTable;
    DWORD dwDibSize, dwOffset, dwHeaderSize;
    int hFile;
    WORD wDibRead;

    bmpFields->lpDibBits = NULL;

    //
    /* Open file and confirm it's a BMP*/
    //
    if (-1 == (hFile = _lopen(szFileName, OF_READ | OF_SHARE_DENY_WRITE)))
    {
        MessageBox(NULL, "Failed to open", "ReadDib", MB_OK);
        return;
    }

    if (_lread(hFile, (LPSTR)&(bmpFields->bmfh), sizeof(BITMAPFILEHEADER)) != sizeof(BITMAPFILEHEADER))
    {
        MessageBox(NULL, "Failed to read bitmap file header", "ReadDib", MB_OK);
        goto CLEANUP;
    }

    if ((bmpFields->bmfh).bfType != *(WORD *)"BM")
    {
        MessageBox(NULL, "Not a BMP", "ReadDib", MB_OK);
        goto CLEANUP;
    }

    wDibRead = 4;
    if (wDibRead != _lread(hFile, (LPSTR)(&dwHeaderSize), wDibRead))
    {
        MessageBox(NULL, "Unable to read dwHeaderSize", "CreateBMP", MB_OK);
        goto CLEANUP;
    }

    if(sizeof(BITMAPINFOHEADER) != dwHeaderSize){
        MessageBox(NULL, "Unsupported header size", "CreateBMP", MB_OK);
        goto CLEANUP;
    }
    // Move back to start of bitmap info header
    _llseek(hFile, sizeof(BITMAPFILEHEADER), 0);

    wDibRead = (WORD)min(32768ul, dwHeaderSize);
    if (wDibRead != _lread(hFile, (LPSTR)(&(bmpFields->bmih)), wDibRead))
    {
        MessageBox(NULL, "Unable to read info header", "CreateBMP", MB_OK);
        goto CLEANUP;
    }

    // Indirect calculation of color table size
    (bmpFields->colorTable).dwColorTableSize = (bmpFields->bmfh).bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
    lpColorTable = (BYTE huge *)GlobalAllocPtr(GMEM_MOVEABLE, (bmpFields->colorTable).dwColorTableSize);
    if (lpColorTable == NULL)
    {
        MessageBox(NULL, "lpColorTable was null", "ReadDib", MB_OK);
        goto CLEANUP;
    }

    wDibRead = (WORD)((bmpFields->colorTable).dwColorTableSize);
    if (wDibRead != _lread(hFile, (LPSTR)(lpColorTable), wDibRead))
    {
        GlobalFreePtr(lpColorTable);
        MessageBox(NULL, "Unable to read color table", "CreateBMP", MB_OK);
        goto CLEANUP;
    }
    (bmpFields->colorTable).lpColorData = lpColorTable;


    // Seek to beginning of bitmap bits
    _llseek(hFile, (bmpFields->bmfh).bfOffBits, 0);
    dwDibSize = (bmpFields->bmih).biSizeImage;
    lpDib = (BYTE huge *)GlobalAllocPtr(GMEM_MOVEABLE, dwDibSize);
    if (lpDib == NULL)
    {
        GlobalFreePtr(lpColorTable);
        MessageBox(NULL, "lpDib was null", "ReadDib", MB_OK);
        goto CLEANUP;
    }

    dwOffset = 0;
    while (dwDibSize > 0)
    {
        wDibRead = (WORD)min(32768ul, dwDibSize);

        if (wDibRead != _lread(hFile, (LPSTR)(lpDib + dwOffset), wDibRead))
        {
            GlobalFreePtr(lpDib);
            GlobalFreePtr(lpColorTable);
            MessageBox(NULL, "Error reading bits", "ReadDib", MB_OK);
            goto CLEANUP;
        }

        dwDibSize -= wDibRead;
        dwOffset += wDibRead;
    }
    bmpFields->lpDibBits =  lpDib;

CLEANUP:
    _lclose(hFile);

    
}

void FAR PASCAL _export WriteDIBBitmapToFile(char *szFileName, BitmapFields_s bmpFields)
{
    int hFile;

    if (-1 == (hFile = _lopen(szFileName, OF_WRITE | OF_SHARE_EXCLUSIVE)))
    {
        if (-1 == (hFile = _lcreat(szFileName, 0)))
        {
            MessageBox(NULL, "Failed to create", "WriteDib", MB_OK);
            return;
        }
    }


    if(sizeof(bmpFields.bmfh) != _lwrite(hFile, &(bmpFields.bmfh), sizeof(bmpFields.bmfh))){
        MessageBox(NULL, "Error writing file header", "WriteDib", MB_OK);
        goto CLEANUP;
    }

    if (sizeof(bmpFields.bmih) != _lwrite(hFile, &(bmpFields.bmih), sizeof(bmpFields.bmih)))
    {
        MessageBox(NULL, "Error writing info header", "WriteDib", MB_OK);
        goto CLEANUP;
    }

    if (bmpFields.colorTable.dwColorTableSize != _lwrite(hFile, bmpFields.colorTable.lpColorData, (UINT)bmpFields.colorTable.dwColorTableSize))
    {
        MessageBox(NULL, "Error writing color table", "WriteDib", MB_OK);
        goto CLEANUP;
    }

    if (bmpFields.bmih.biSizeImage != _lwrite(hFile, bmpFields.lpDibBits, (UINT)bmpFields.bmih.biSizeImage))
    {
        MessageBox(NULL, "Error writing bits", "WriteDib", MB_OK);
        goto CLEANUP;
    }

CLEANUP:
    _lclose(hFile);
    return;
}

