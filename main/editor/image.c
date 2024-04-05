
#include "image.h"

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


ReturnCode_e FAR PASCAL _export LoadBMPFile(char* szFileName, BitmapFields_s* bmpFields){
    // BITMAPFILEHEADER bmfh;
    BYTE huge *lpDib, huge *lpColorTable;
    DWORD dwDibSize, dwOffset, dwHeaderSize;
    int hFile;
    WORD wDibRead;
    int rc = RC_ERROR;

    if (bmpFields->lpDibBits != NULL)
    {
        GlobalFreePtr(bmpFields->lpDibBits);
        bmpFields->lpDibBits = NULL;
    }

    if ((bmpFields->colorTable).lpColorData != NULL)
    {
        GlobalFreePtr((bmpFields->colorTable).lpColorData);
        (bmpFields->colorTable).lpColorData = NULL;
    }
    

    //
    /* Open file and confirm it's a BMP*/
    //
    if (-1 == (hFile = _lopen(szFileName, OF_READ | OF_SHARE_DENY_WRITE)))
    {
        MessageBox(NULL, "Failed to open", "ReadDib", MB_OK);
        return RC_ERROR;
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
    rc = RC_SUCCESS;

CLEANUP:
    _lclose(hFile);

    return rc;
    
}

ReturnCode_e FAR PASCAL _export LoadIconFile(char* szFileName, IconFields_s* iconFields){
    // BITMAPFILEHEADER bmfh;
    BYTE huge *lpDib, huge *lpColorTable;
    BYTE far* lpMask;
    DWORD dwDibSize, dwOffset, dwHeaderSize;
    int hFile;
    WORD wDibRead;
    int rc = RC_ERROR;

    if (iconFields->lpDibBits != NULL)
    {
        GlobalFreePtr(iconFields->lpDibBits);
        iconFields->lpDibBits = NULL;
    }

    if ((iconFields->colorTable).lpColorData != NULL)
    {
        GlobalFreePtr((iconFields->colorTable).lpColorData);
        (iconFields->colorTable).lpColorData = NULL;
    }

    if ((iconFields->imageMask).lpImageMask != NULL)
    {
        GlobalFreePtr((iconFields->imageMask).lpImageMask);
        (iconFields->imageMask).lpImageMask = NULL;
    }
    

    //
    /* Open file and confirm it's an icon*/
    //
    if (-1 == (hFile = _lopen(szFileName, OF_READ | OF_SHARE_DENY_WRITE)))
    {
        MessageBox(NULL, "Failed to open", "LoadIcon", MB_OK);
        return RC_ERROR;
    }

    // Read Icon Directory.
    //  confirm is an icon (vs cursor) and there's only one entry
    if (_lread(hFile, (LPSTR)&(iconFields->idir), sizeof(ICONDIR_s)) != sizeof(ICONDIR_s))
    {
        MessageBox(NULL, "Failed to read icon directory", "LoadIcon", MB_OK);
        goto CLEANUP;
    }

    if ((iconFields->idir).wType != TYPE_ICON)
    {
        MessageBox(NULL, "Not a Icon", "LoadIcon", MB_OK);
        goto CLEANUP;
    }

    if ((iconFields->idir).wIconCount != 1)
    {
        MessageBox(NULL, "Only single-entry ico files supported", "LoadIcon", MB_OK);
        goto CLEANUP;
    }

    // Read in icon directory entry
    if (_lread(hFile, (LPSTR)&(iconFields->iDirEntry), sizeof(ICONDIRENTRY_s)) != sizeof(ICONDIRENTRY_s))
    {
        MessageBox(NULL, "Failed to read icon dir entry", "LoadIcon", MB_OK);
        goto CLEANUP;
    }



    // Read in bitmap info header

    // Confirm size mathces the BITMAPINFOHEADER size
    wDibRead = 4;
    if (wDibRead != _lread(hFile, (LPSTR)(&dwHeaderSize), wDibRead))
    {
        MessageBox(NULL, "Unable to read dwHeaderSize", "LoadIcon", MB_OK);
        goto CLEANUP;
    }

    if(sizeof(BITMAPINFOHEADER) != dwHeaderSize){
        MessageBox(NULL, "Unsupported header size", "LoadIcon", MB_OK);
        goto CLEANUP;
    }
    // Move back to start of bitmap info header
    _llseek(hFile, sizeof(ICONDIR_s) + sizeof(ICONDIRENTRY_s) * ((iconFields->idir).wIconCount), 0);

    wDibRead = (WORD)min(32768ul, dwHeaderSize);
    if (wDibRead != _lread(hFile, (LPSTR)(&(iconFields->bmih)), wDibRead))
    {
        MessageBox(NULL, "Unable to read bitmap info header", "LoadIcon", MB_OK);
        goto CLEANUP;
    }

    // Indirect calculation of color table size
    // FIXME for other BMP formats, the color table consists of RGBTRIPLEs. These are not currently supported.
    if((iconFields->bmih).biClrUsed == 0){
        // calc by num of bits
        (iconFields->colorTable).dwColorTableSize = sizeof(RGBQUAD) * (1 << (iconFields->bmih).biBitCount);
    }else{
        // bmih.biClrUsed has the number of entries
        (iconFields->colorTable).dwColorTableSize = sizeof(RGBQUAD) * ((iconFields->bmih).biClrUsed);
    }
    
    lpColorTable = (BYTE huge *)GlobalAllocPtr(GMEM_MOVEABLE, (iconFields->colorTable).dwColorTableSize);
    if (lpColorTable == NULL)
    {
        MessageBox(NULL, "lpColorTable was null", "LoadIcon", MB_OK);
        goto CLEANUP;
    }

    // Read color table
    wDibRead = (WORD)((iconFields->colorTable).dwColorTableSize);
    if (wDibRead != _lread(hFile, (LPSTR)(lpColorTable), wDibRead))
    {
        GlobalFreePtr(lpColorTable);
        MessageBox(NULL, "Unable to read color table", "LoadIcon", MB_OK);
        goto CLEANUP;
    }
    (iconFields->colorTable).lpColorData = lpColorTable;


    // Read in image bits

    // FIXME again assuming 1 directory entry
    (iconFields->imageMask).wMaskSize = ((iconFields->iDirEntry).bWidth * (iconFields->iDirEntry).bHeight)/8;

    if(ICON_MASK_SIZE != (iconFields->imageMask).wMaskSize){
        // Double-check here since we only support 32x32 for now
        GlobalFreePtr(lpColorTable);
        MessageBox(NULL, "Error calculating icon mask", "LoadIcon", MB_OK);
        goto CLEANUP;
    }
    
    dwDibSize = (iconFields->bmih).biSizeImage - (iconFields->imageMask).wMaskSize;
    lpDib = (BYTE huge *)GlobalAllocPtr(GMEM_MOVEABLE, dwDibSize);
    if (lpDib == NULL)
    {
        GlobalFreePtr(lpColorTable);
        MessageBox(NULL, "lpDib was null", "LoadIcon", MB_OK);
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
            MessageBox(NULL, "Error reading bits", "LoadIcon", MB_OK);
            goto CLEANUP;
        }

        dwDibSize -= wDibRead;
        dwOffset += wDibRead;
    }
    iconFields->lpDibBits = lpDib;



    // Read in image mask
    dwDibSize = (iconFields->imageMask).wMaskSize;
    lpMask = (BYTE far *)GlobalAllocPtr(GMEM_MOVEABLE, dwDibSize);
    if (lpMask == NULL)
    {
        GlobalFreePtr(lpDib);
        GlobalFreePtr(lpColorTable);
        MessageBox(NULL, "lpMask was null", "LoadIcon", MB_OK);
        goto CLEANUP;
    }

    wDibRead = (WORD)((iconFields->imageMask).wMaskSize);
    if (wDibRead != _lread(hFile, (LPSTR)(lpMask), wDibRead))
    {
        GlobalFreePtr(lpDib);
        GlobalFreePtr(lpColorTable);
        MessageBox(NULL, "Unable to read image mask", "LoadIcon", MB_OK);
        goto CLEANUP;
    }
    (iconFields->imageMask).lpImageMask = lpMask;




    rc = RC_SUCCESS;

CLEANUP:
    _lclose(hFile);

    return rc;
    
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


void FAR PASCAL _export WriteICOToFile (char * szFileName, IconFields_s iconFields, ICONDIRENTRY_s iconEntry, BitmapFields_s bmpFields, ImageMask_s imageMask)
{
    int hFile;

    if (-1 == (hFile = _lcreat(szFileName, 0)))
    {
        MessageBox(NULL, "Failed to create", "WriteICO", MB_OK);
        return;
    }

    // Icon dir
    if(sizeof(iconFields.idir) != _lwrite(hFile, &(iconFields.idir), sizeof(iconFields.idir))){
        MessageBox(NULL, "Error writing ICONDIR", "WriteICO", MB_OK);
        goto CLEANUP;
    }

    // Icon entry (only 1 for now, maybe 2 later for b/w)
    if (sizeof(iconEntry) != _lwrite(hFile, &(iconEntry), sizeof(iconEntry)))
    {
        MessageBox(NULL, "Error writing icon entry", "WriteICO", MB_OK);
        goto CLEANUP;
    }

    // BMP info header
    if (sizeof(bmpFields.bmih) != _lwrite(hFile, &(bmpFields.bmih), sizeof(bmpFields.bmih)))
    {
        MessageBox(NULL, "Error writing BMP info header", "WriteICO", MB_OK);
        goto CLEANUP;
    }

    // Color table
    if (bmpFields.colorTable.dwColorTableSize != _lwrite(hFile, bmpFields.colorTable.lpColorData, (UINT)bmpFields.colorTable.dwColorTableSize))
    {
        MessageBox(NULL, "Error writing color table", "WriteICO", MB_OK);
        goto CLEANUP;
    }

    // Image bits
    if ((bmpFields.bmih.biSizeImage - 0x80) != _lwrite(hFile, bmpFields.lpDibBits, (UINT)bmpFields.bmih.biSizeImage - 0x80))
    {
        MessageBox(NULL, "Error writing bits", "WriteICO", MB_OK);
        goto CLEANUP;
    }

    // Image mask
    if (imageMask.wMaskSize != _lwrite(hFile, imageMask.lpImageMask, (UINT)imageMask.wMaskSize))
    {
        MessageBox(NULL, "Error writing image mask", "WriteICO", MB_OK);
        goto CLEANUP;
    }

CLEANUP:
    _lclose(hFile);
    return;
}

