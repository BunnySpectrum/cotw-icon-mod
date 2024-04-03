
#include "image.h"

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
        MessageBox(NULL, "Bad header size", "ReadDib", MB_OK);
        return NULL;
    }

    return lpDib;
}