
#include "canvas.h"

CanvasHistoryEntry_s *canvasHistory[CANVAS_HISTORY_LEN];
int canvasHistoryWriteIndex, canvasHistoryReadIndex;

static short pixW, pixH;
static int callDepthCurrent, callDepthMax;
#define CALL_DEPTH_LIMIT 1024
static short nLength;
static char szBuffer[80];

void free_history_entry(int index)
{
    if (canvasHistory[index] != NULL)
    {
        free(canvasHistory[index]->nextAction->args);
        free(canvasHistory[index]->prevAction->args);
        free(canvasHistory[index]);
        canvasHistory[index] = NULL;
    }
}

BOOL create_history_entry(CanvasTool_e tool)
{
    void *doArgs = NULL;
    void *undoArgs = NULL;
    CanvasAction_s *doAction = NULL;
    CanvasAction_s *undoAction = NULL;
    CanvasHistoryEntry_s *newEntry = NULL;

    newEntry = (CanvasHistoryEntry_s *)malloc(sizeof(CanvasHistoryEntry_s));
    if (newEntry == NULL)
    {
        goto FAIL;
    }

    doAction = (CanvasAction_s *)malloc(sizeof(CanvasAction_s));
    if (doAction == NULL)
    {
        goto FAIL;
    }

    undoAction = (CanvasAction_s *)malloc(sizeof(CanvasAction_s));
    if (undoAction == NULL)
    {
        goto FAIL;
    }

    switch (tool)
    {
    case CanvasToolBrush:
        doAction->tool = CanvasToolBrush;
        doArgs = malloc(sizeof(CanvasBrushArgs_s));

        undoAction->tool = CanvasToolBrush;
        undoArgs = malloc(sizeof(CanvasBrushArgs_s));
        break;

    case CanvasToolLine:
        doAction->tool = CanvasToolLine;
        doArgs = malloc(sizeof(CanvasLineArgs_s));

        undoAction->tool = CanvasToolRestore;
        undoArgs = malloc(sizeof(CanvasRestoreArgs_s));
        break;

    case CanvasToolRect:
        doAction->tool = CanvasToolRect;
        doArgs = malloc(sizeof(CanvasRectArgs_s));

        undoAction->tool = CanvasToolRestore;
        undoArgs = malloc(sizeof(CanvasRestoreArgs_s));
        break;

    case CanvasToolFlood:
        doAction->tool = CanvasToolFlood;
        doArgs = malloc(sizeof(CanvasFloodArgs_s));

        undoAction->tool = CanvasToolRestore;
        undoArgs = malloc(sizeof(CanvasRestoreArgs_s));
        break;

    default:
        goto FAIL;
    }

    // check here instead of after each malloc for readibility
    if ((doArgs == NULL) || (undoArgs == NULL))
    {
        goto FAIL;
    }

    free_history_entry(canvasHistoryWriteIndex);

    // make actions
    doAction->args = doArgs;
    undoAction->args = undoArgs;

    // make entry
    newEntry->valid = TRUE;
    newEntry->nextAction = doAction;
    newEntry->prevAction = undoAction;

    canvasHistory[canvasHistoryWriteIndex] = newEntry;
    canvasHistoryReadIndex = canvasHistoryWriteIndex;
    canvasHistoryWriteIndex = (canvasHistoryWriteIndex + 1) % CANVAS_HISTORY_LEN;
    if (canvasHistory[canvasHistoryWriteIndex] != NULL)
    {
        canvasHistory[canvasHistoryWriteIndex]->valid = FALSE;
    }
    return TRUE;

FAIL:
    free(newEntry);
    free(doAction);
    free(undoAction);
    free(doArgs);
    free(undoArgs);
    return FALSE;
}

BYTE canvas_draw_brush(HDC *hdc, BYTE *pixelFrame, CanvasBrushArgs_s *args)
{
    COLORREF newColor;
    HBRUSH hBrush;
    RECT rect;
    short pixelCol, pixelRow;
    BYTE oldColorCode;

    pixelCol = PIXEL_1D_2_COL(args->pixel);
    pixelRow = PIXEL_1D_2_ROW(args->pixel);

    oldColorCode = pixelFrame[args->pixel];
    pixelFrame[args->pixel] = (BYTE)(args->newColorCode);

    pixel_color_code_to_rgb(args->newColorCode, &newColor);
    hBrush = CreateSolidBrush(newColor);

    rect.left = (pixelCol * args->size) + 1;
    rect.top = (pixelRow * args->size) + 1;
    rect.right = rect.left + args->size - 2;
    rect.bottom = rect.top + args->size - 2;

    FillRect(*hdc, &rect, hBrush);
    DeleteObject(hBrush);

    return oldColorCode;
}

// Bresenham's line algo
// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
// Specifically, the versions tracking error for X and Y
BOOL canvas_draw_line(HDC *hdc, BYTE *pixelFrame, CanvasLineArgs_s *args, int *restoreLength, BYTE **restoreData)
{
    short pixelX, pixelY;
    short deltaX, deltaY, errorD, signX, signY, error2;
    short bailCounter = CANVAS_DIM * 2;
    CanvasBrushArgs_s brushArgs;

    // abs of difference
    deltaX = args->pt2.x - args->pt1.x;
    deltaX = deltaX < 0 ? -1 * deltaX : deltaX;

    // get sign
    signX = args->pt1.x < args->pt2.x ? 1 : -1;

    //-1*abs of difference
    deltaY = args->pt2.y - args->pt1.y;
    deltaY = deltaY < 0 ? deltaY : -1 * deltaY;

    // get sign
    signY = args->pt1.y < args->pt2.y ? 1 : -1;

    errorD = deltaX + deltaY;
    pixelX = args->pt1.x;
    pixelY = args->pt1.y;

    if ((restoreLength != NULL) && (*restoreLength > 0) && (*restoreLength <= PIXEL_COUNT))
    {
        int leftX, rightX, topY, botY, x, y, counter;

        leftX = min(args->pt1.x, args->pt2.x);
        rightX = max(args->pt1.x, args->pt2.x);
        topY = min(args->pt1.y, args->pt2.y);
        botY = max(args->pt1.y, args->pt2.y);

        *restoreLength = (rightX - leftX + 1) * (botY - topY + 1);

        *restoreData = (BYTE *)malloc(sizeof(BYTE) * (*restoreLength));
        if (*restoreData == NULL)
        {
            MessageBox(NULL, "Unable to malloc restore data", "Line", MB_OK);
            *restoreLength = 0;
            return FALSE;
        }

        counter = 0;
        for (x = leftX; x <= rightX; x++)
        {
            for (y = topY; y <= botY; y++)
            {
                (*restoreData)[counter] = pixelFrame[PIXEL_2D_2_1D(x, y)];
                counter++;
            }
        }
    }

    while (bailCounter-- > 0)
    {
        brushArgs.pixel = PIXEL_2D_2_1D(pixelX, pixelY);
        brushArgs.size = args->size;
        brushArgs.newColorCode = args->newColorCode;
        canvas_draw_brush(hdc, pixelFrame, &brushArgs);

        if ((pixelX == args->pt2.x) && (pixelY == args->pt2.y))
        {
            break; // handle double-click on same spot
            // MessageBox(hwnd, "First break", "Line", MB_OK);
        }
        error2 = 2 * errorD;
        if (error2 >= deltaY)
        {
            if (pixelX == args->pt2.x)
            {
                break;
                // MessageBox(hwnd, "2nd break", "Line", MB_OK);
            }
            errorD += deltaY;
            pixelX += signX;
        }
        if (error2 <= deltaX)
        {
            if (pixelY == args->pt2.y)
            {
                break;
                // MessageBox(hwnd, "3rd break", "Line", MB_OK);
            }
            errorD += deltaX;
            pixelY += signY;
        }
    }
    if (bailCounter <= 0)
    {
        MessageBox(NULL, "Error", "Line", MB_OK);
    }
    return TRUE;
}

BOOL canvas_draw_rect(HDC *hdc, BYTE *pixelFrame, CanvasRectArgs_s *args, int *restoreLength, BYTE **restoreData)
{
    short pixelX, pixelY, pxLeftCol, pxRightCol, pxTopRow, pxBotRow;
    CanvasBrushArgs_s brushArgs;

    pxLeftCol = min(args->pt1.x, args->pt2.x);
    pxRightCol = max(args->pt1.x, args->pt2.x);
    pxTopRow = min(args->pt1.y, args->pt2.y);
    pxBotRow = max(args->pt1.y, args->pt2.y);

    if ((restoreLength != NULL) && (*restoreLength > 0) && (*restoreLength <= PIXEL_COUNT))
    {
        int counter;
        *restoreLength = (pxRightCol - pxLeftCol + 1) * (pxBotRow - pxTopRow + 1);

        *restoreData = (BYTE *)malloc(sizeof(BYTE) * (*restoreLength));
        if (*restoreData == NULL)
        {
            MessageBox(NULL, "Unable to malloc restore data", "Rect", MB_OK);
            *restoreLength = 0;
            return FALSE;
        }

        counter = 0;
        for (pixelX = pxLeftCol; pixelX <= pxRightCol; pixelX++)
        {
            for (pixelY = pxTopRow; pixelY <= pxBotRow; pixelY++)
            {
                (*restoreData)[counter] = pixelFrame[PIXEL_2D_2_1D(pixelX, pixelY)]; // TODO replace w/ linear copy
                counter++;
            }
        }
    }

    for (pixelX = pxLeftCol; pixelX <= pxRightCol; pixelX++)
    {
        for (pixelY = pxTopRow; pixelY <= pxBotRow; pixelY++)
        {
            brushArgs.pixel = PIXEL_2D_2_1D(pixelX, pixelY);
            brushArgs.size = args->size;
            brushArgs.newColorCode = args->newColorCode;

            canvas_draw_brush(hdc, pixelFrame, &brushArgs);
        }
    }

    return TRUE;
}

BOOL canvas_restore_rect(HDC *hdc, BYTE *pixelFrame, CanvasRestoreArgs_s *args)
{
    short pixelX, pixelY, pxLeftCol, pxRightCol, pxTopRow, pxBotRow;
    short counter;
    CanvasBrushArgs_s brushArgs;

    pxLeftCol = args->ptNW.x;
    pxRightCol = args->ptSE.x;
    pxTopRow = args->ptNW.y;
    pxBotRow = args->ptSE.y;

    if ((args->dataLength) != ((pxRightCol - pxLeftCol + 1) * (pxBotRow - pxTopRow + 1)))
    {
        nLength = wsprintf(szBuffer, "Data length error. %d vs %d.", args->dataLength, (pxRightCol - pxLeftCol + 1) * (pxBotRow - pxTopRow + 1));
        MessageBox(NULL, szBuffer, "Restore", MB_OK);
        return FALSE;
    }

    brushArgs.size = args->size;
    counter = 0;
    for (pixelX = pxLeftCol; pixelX <= pxRightCol; pixelX++)
    {
        for (pixelY = pxTopRow; pixelY <= pxBotRow; pixelY++)
        {
            brushArgs.pixel = PIXEL_2D_2_1D(pixelX, pixelY);

            // oldColorCode = pixelFrame[brushArgs.pixel];
            // brushArgs.newColorCode = (oldColorCode + 8)%16;

            brushArgs.newColorCode = (args->colorData)[counter];
            canvas_draw_brush(hdc, pixelFrame, &brushArgs);

            counter++;
        }
    }

    return TRUE;
}

// Fixed length queue of max length without adding duplicates to queue
// SP about 3k (recheck after adding restore code)
BOOL canvas_draw_flood_v7(HDC *hdc, BYTE *pixelFrame, CanvasFloodArgs_s *args, int *restoreLength, BYTE **restoreData, POINT *ptNW, POINT *ptSE)
{
    int pixelQueue[PIXEL_COUNT];
    BYTE pixelAdded[PIXEL_COUNT];
    short pixelRow, pixelCol;
    short rowMin, rowMax, colMin, colMax;
    short i;
    short readIndex, writeIndex;
    int checkPixel;
    CanvasBrushArgs_s brushArgs;

    if (pixelFrame[args->pixel] != args->targetColorCode)
    {
        // bail early for flooding shape with same color
        goto FLOOD_EXIT;
    }

    for (i = 0; i < PIXEL_COUNT; i++)
    {
        pixelAdded[i] = 0;
    }

    readIndex = writeIndex = 0;

    // Set up first entry
    pixelQueue[writeIndex++] = args->pixel;
    pixelAdded[args->pixel] = 1;

    rowMin = colMin = CANVAS_DIM;
    rowMax = colMax = 0;
    do
    {
        // for breaking infinite loop error
        // callDepthCurrent++;
        // if(callDepthCurrent > CALL_DEPTH_LIMIT){
        //     MessageBox(NULL, "Depth", "Flood v6", MB_OK);
        //     goto FLOOD_EXIT;
        // }

        if (writeIndex > 1024)
        {
            MessageBox(NULL, "Write max", "Flood v7", MB_OK);
            goto FLOOD_EXIT;
        }

        // Get next pixel
        args->pixel = pixelQueue[readIndex++];

        // Compute row and column
        pixelRow = PIXEL_1D_2_ROW(args->pixel);
        pixelCol = PIXEL_1D_2_COL(args->pixel);

        // Check 4-way adjacent pixels and set flag if need to flood them
        // Left
        checkPixel = PIXEL_2D_2_1D(pixelCol - 1, pixelRow);
        if ((pixelCol > 0) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0))
        {
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;
            rowMin = min(rowMin, pixelRow);
            rowMax = max(rowMax, pixelRow);
            colMin = min(colMin, pixelCol - 1);
            colMax = max(colMax, pixelCol - 1);
        }

        // Up
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow - 1);
        if ((pixelRow > 0) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0))
        {
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;
            rowMin = min(rowMin, pixelRow - 1);
            rowMax = max(rowMax, pixelRow - 1);
            colMin = min(colMin, pixelCol);
            colMax = max(colMax, pixelCol);
        }

        // Right
        checkPixel = PIXEL_2D_2_1D(pixelCol + 1, pixelRow);
        if ((pixelCol < CANVAS_DIM - 1) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0))
        {
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;
            rowMin = min(rowMin, pixelRow);
            rowMax = max(rowMax, pixelRow);
            colMin = min(colMin, pixelCol + 1);
            colMax = max(colMax, pixelCol + 1);
        }

        // Down
        checkPixel = PIXEL_2D_2_1D(pixelCol, pixelRow + 1);
        if ((pixelRow < CANVAS_DIM - 1) && (args->targetColorCode == pixelFrame[checkPixel]) && (pixelAdded[checkPixel] == 0))
        {
            // Add
            pixelQueue[writeIndex++] = checkPixel;
            pixelAdded[checkPixel] = 1;
            rowMin = min(rowMin, pixelRow + 1);
            rowMax = max(rowMax, pixelRow + 1);
            colMin = min(colMin, pixelCol);
            colMax = max(colMax, pixelCol);
        }

        // Check if pixels left to check
    } while (readIndex < writeIndex);

    // Capture current state of region
    if ((restoreLength != NULL) && (*restoreLength > 0) && (*restoreLength <= PIXEL_COUNT))
    {
        int x, y, counter;

        *restoreLength = (colMax - colMin + 1) * (rowMax - rowMin + 1);

        *restoreData = (BYTE *)malloc(sizeof(BYTE) * (*restoreLength));
        if (*restoreData == NULL)
        {
            MessageBox(NULL, "Unable to malloc restore data", "Flood", MB_OK);
            *restoreLength = 0;
            return FALSE;
        }

        counter = 0;
        for (x = colMin; x <= colMax; x++)
        {
            for (y = rowMin; y <= rowMax; y++)
            {
                (*restoreData)[counter] = pixelFrame[PIXEL_2D_2_1D(x, y)];
                counter++;
            }
        }
        (*ptNW).x = colMin;
        (*ptNW).y = rowMin;

        (*ptSE).x = colMax;
        (*ptSE).y = rowMax;
    }

    // Draw all pixels
    for (i = 0; i < PIXEL_COUNT; i++)
    {
        if (pixelAdded[i] == 1)
        {
            brushArgs.pixel = i;
            brushArgs.size = args->size;
            brushArgs.newColorCode = args->newColorCode;
            canvas_draw_brush(hdc, pixelFrame, &brushArgs);
        }
    }

FLOOD_EXIT:

    return TRUE;
}

void FAR PASCAL _export copy_img_to_canvas(BYTE huge *lpImg, BYTE huge *addrStart, WORD width, WORD height)
{
    BYTE huge *lpDibBits;
    short cxDib, cyDib;
    int idx, idx2;
    BYTE shift, mask, row, col;

    // ReadDib section
    if (lpImg == NULL)
    {
        MessageBox(NULL, "Null lpImg", "Canvas", MB_OK);
        return;
    }
    // for (idx = PIXEL_COUNT-1; idx >= 0; idx--)
    row = CANVAS_DIM-1;
    col = 0;
    for (idx = 0; idx < PIXEL_COUNT; idx++)
    {
        // pixelFrame[idx] = *(addrStart + idx/2) & 0x0F;
        if (idx % 2 == 0)
        {
            shift = 0;
            mask = 0x0F;
        }
        else
        {
            shift = 4;
            mask = 0xF0;   
        }

        // Method 1
        idx2 = PIXEL_2D_2_1D(col, row);
        pixelFrame[idx2] = (*(addrStart + idx / 2) & mask) >> shift;
        if(col < CANVAS_DIM-1){
            col++;
        }else{
            col = 0;
            row--;
        }

        // Method 2
        // idx2 = (PIXEL_COUNT - 1) - (31 - idx%32) - (idx/32)*32;
        // pixelFrame[idx2] = (*(addrStart + idx / 2) & mask) >> shift;

    }

    // lpDibBits = GetDibBitsAddr(lpDib);
    // cxDib = GetDibWidth(lpDib);
    // cyDib = GetDibHeight(lpDib);

    // SetStretchBltMode(hdc, COLORONCOLOR);

    // StretchDIBits(hdc, 0, 0, CANVAS_DIM * cxBlock, CANVAS_DIM * cyBlock,
    //               0, 0, width, height,
    //               (LPSTR)addrStart,
    //               (LPBITMAPINFO)lpDib,
    //               DIB_RGB_COLORS, SRCCOPY);
}

long FAR PASCAL _export WndProcCanvas(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    static short cxBlock, cyBlock;
    int x, y, pixel, pixRow, pixCol;
    static HPEN hPen;
    static short xSel, ySel;
    HBRUSH hBrush;
    POINT lpPoint;
    BYTE colorCode;
    // WORD activeColorForeground;
    COLORREF newColor;
    
    static WORD drawState;
    static POINT ptPixelDraw1, ptPixelDraw2;
    HWND hwndParent;

    static LOGPEN lpBlack = {PS_SOLID, 2, 2, RGB(0, 0, 0)},
                lpWhite = {PS_SOLID, 1, 1, RGB(255, 255, 255)};
    HPEN hPenBlack, hPenWhite;

    switch (message)
    {
    case WM_CREATE:
    {
        drawState = DRAW_STATE_START;
        SetWindowWord(hwnd, CanvasWordForeColor, PixelColorCodeBlack);
        SetWindowWord(hwnd, CanvasWordBackColor, PixelColorCodeWhite);
        SetWindowWord(hwnd, CanvasWordTool, CanvasToolBrush);

        for (x = 0; x < PIXEL_COUNT; x++)
        {
            pixelFrame[x] = (BYTE)PixelColorCodeWhite;
        }
        hPen = CreatePen(PS_SOLID, 1, COLOR_SILVER);

        canvasHistoryWriteIndex = canvasHistoryReadIndex = 0;

        return 0;
    }
    case WM_SIZE:
    {
        cxBlock = LOWORD(lParam) / CANVAS_DIM;
        cyBlock = HIWORD(lParam) / CANVAS_DIM;
        pixW = cxBlock;
        pixH = cyBlock;
        return 0;
    }

    case WM_COMMAND:
    {
        CanvasAction_s *action;

        hdc = GetDC(hwnd);
        switch (HIWORD(lParam))
        {
        case 0: // undo
            action = canvasHistory[LOWORD(lParam)]->prevAction;
            break;
        case 1: // redo
            action = canvasHistory[LOWORD(lParam)]->nextAction;
            break;
        default:
            break;
        }
        switch (action->tool)
        {
        case CanvasToolBrush:
            canvas_draw_brush(&hdc, pixelFrame, action->args);
            ValidateRect(hwnd, NULL);
            break;
        case CanvasToolLine:
            canvas_draw_line(&hdc, pixelFrame, action->args, NULL, NULL);
            ValidateRect(hwnd, NULL);
            break;
        case CanvasToolRect:
            canvas_draw_rect(&hdc, pixelFrame, action->args, NULL, NULL);
            ValidateRect(hwnd, NULL);
            break;
        case CanvasToolFlood:
            canvas_draw_flood_v7(&hdc, pixelFrame, action->args, NULL, NULL, NULL, NULL);
            ValidateRect(hwnd, NULL);
            break;
        // case CanvasToolErase:{//TODO make function for this
        //     int x;
        //     for(x=0; x<PIXEL_COUNT; x++){
        //         pixelFrame[x] = (BYTE)PixelColorCodeWhite;
        //     }
        //     InvalidateRect(hwnd, NULL, FALSE);
        //     break;
        // }
        case CanvasToolRestore:
            canvas_restore_rect(&hdc, pixelFrame, action->args);
            ValidateRect(hwnd, NULL);
            break;
        default:
            MessageBox(NULL, "Unsupported history tool", "Canvas", MB_OK);
            break;
        }
        ReleaseDC(hwnd, hdc);
        break;
    }

    case WM_LBUTTONDOWN:
    {
        x = LOWORD(lParam);
        y = HIWORD(lParam);

        pixCol = x / cxBlock;
        pixRow = y / cyBlock;
        pixel = pixCol + pixRow * CANVAS_DIM;

        if ((pixCol >= CANVAS_DIM) || (pixCol < 0) || (pixRow >= CANVAS_DIM) || (pixRow < 0))
        {
            return 0;
        }

        hdc = GetDC(hwnd);

        switch ((BYTE)GetWindowWord(hwnd, CanvasWordTool))
        {
        case CanvasToolBrush:
        {
            CanvasBrushArgs_s *brushDoArgs = NULL;
            CanvasBrushArgs_s *brushUndoArgs = NULL;

            if (FALSE == create_history_entry(CanvasToolBrush))
            {
                MessageBox(NULL, "Unable to create history entry", "Brush", MB_OK);
                break;
            }

            brushDoArgs = (CanvasBrushArgs_s *)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
            brushUndoArgs = (CanvasBrushArgs_s *)canvasHistory[canvasHistoryReadIndex]->prevAction->args;

            brushDoArgs->pixel = pixel;
            brushDoArgs->size = cxBlock;
            brushDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);

            brushUndoArgs->pixel = brushDoArgs->pixel;
            brushUndoArgs->size = brushDoArgs->size;
            brushUndoArgs->newColorCode = canvas_draw_brush(&hdc, pixelFrame, brushDoArgs);
            ValidateRect(hwnd, NULL);

            break;
        }

        case CanvasToolLine:
        {
            switch (drawState)
            {
            case DRAW_STATE_START:
                drawState = DRAW_LINE_FIRST;
                ptPixelDraw1.x = pixCol;
                ptPixelDraw1.y = pixRow;
                break;
            case DRAW_LINE_FIRST:
            {
                CanvasLineArgs_s *lineDoArgs;
                CanvasRestoreArgs_s *lineUndoArgs;

                ptPixelDraw2.x = pixCol;
                ptPixelDraw2.y = pixRow;

                if (FALSE == create_history_entry(CanvasToolLine))
                {
                    MessageBox(NULL, "Unable to create history entry", "Line", MB_OK);
                    break;
                }

                lineDoArgs = (CanvasLineArgs_s *)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
                lineUndoArgs = (CanvasRestoreArgs_s *)canvasHistory[canvasHistoryReadIndex]->prevAction->args;

                lineDoArgs->size = cxBlock;
                lineDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                lineDoArgs->pt1 = ptPixelDraw1;
                lineDoArgs->pt2 = ptPixelDraw2;

                lineUndoArgs->dataLength = PIXEL_COUNT;
                if (FALSE == canvas_draw_line(&hdc, pixelFrame, lineDoArgs, &(lineUndoArgs->dataLength), &(lineUndoArgs->colorData)))
                {
                    free_history_entry(canvasHistoryReadIndex);
                    drawState = DRAW_STATE_START;
                    MessageBox(NULL, "Draw line failed", "Line", MB_OK);
                    break;
                }

                lineUndoArgs->size = cxBlock;
                lineUndoArgs->ptNW.x = min(lineDoArgs->pt1.x, lineDoArgs->pt2.x);
                lineUndoArgs->ptSE.x = max(lineDoArgs->pt1.x, lineDoArgs->pt2.x);
                lineUndoArgs->ptNW.y = min(lineDoArgs->pt1.y, lineDoArgs->pt2.y);
                lineUndoArgs->ptSE.y = max(lineDoArgs->pt1.y, lineDoArgs->pt2.y);

                drawState = DRAW_STATE_START;
                break;
            }
            }

            ValidateRect(hwnd, NULL);
            break;
        }

        case CanvasToolRect:
        {
            switch (drawState)
            {
            case DRAW_STATE_START:
                drawState = DRAW_LINE_FIRST;
                ptPixelDraw1.x = pixCol;
                ptPixelDraw1.y = pixRow;
                break;
            case DRAW_LINE_FIRST:
            {
                CanvasRectArgs_s *rectDoArgs;
                CanvasRestoreArgs_s *rectUndoArgs;

                ptPixelDraw2.x = pixCol;
                ptPixelDraw2.y = pixRow;

                if (FALSE == create_history_entry(CanvasToolRect))
                {
                    MessageBox(NULL, "Unable to create history entry", "Rect", MB_OK);
                    break;
                }

                rectDoArgs = (CanvasRectArgs_s *)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
                rectUndoArgs = (CanvasRestoreArgs_s *)canvasHistory[canvasHistoryReadIndex]->prevAction->args;

                rectDoArgs->size = cxBlock;
                rectDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
                rectDoArgs->pt1 = ptPixelDraw1;
                rectDoArgs->pt2 = ptPixelDraw2;

                rectUndoArgs->dataLength = PIXEL_COUNT;
                if (FALSE == canvas_draw_rect(&hdc, pixelFrame, rectDoArgs, &(rectUndoArgs->dataLength), &(rectUndoArgs->colorData)))
                {
                    free_history_entry(canvasHistoryReadIndex);
                    drawState = DRAW_STATE_START;
                    MessageBox(NULL, "Draw rect failed", "Rect", MB_OK);
                    break;
                }

                rectUndoArgs->size = cxBlock;
                rectUndoArgs->ptNW.x = min(rectDoArgs->pt1.x, rectDoArgs->pt2.x);
                rectUndoArgs->ptSE.x = max(rectDoArgs->pt1.x, rectDoArgs->pt2.x);
                rectUndoArgs->ptNW.y = min(rectDoArgs->pt1.y, rectDoArgs->pt2.y);
                rectUndoArgs->ptSE.y = max(rectDoArgs->pt1.y, rectDoArgs->pt2.y);

                drawState = DRAW_STATE_START;
                break;
            }
            }
            ValidateRect(hwnd, NULL);
            break;
        }

        case CanvasToolFlood:
        {
            CanvasFloodArgs_s *floodDoArgs;
            CanvasRestoreArgs_s *floodUndoArgs;

            if (pixelFrame[pixel] == (BYTE)GetWindowWord(hwnd, CanvasWordForeColor))
            {
                break; // skip, already equal to target color
            }

            if (FALSE == create_history_entry(CanvasToolFlood))
            {
                MessageBox(NULL, "Unable to create history entry", "Flood", MB_OK);
                break;
            }

            floodDoArgs = (CanvasFloodArgs_s *)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
            floodUndoArgs = (CanvasRestoreArgs_s *)canvasHistory[canvasHistoryReadIndex]->prevAction->args;

            lpPoint.x = pixCol;
            lpPoint.y = pixRow;

            floodDoArgs->pixel = pixel;
            floodDoArgs->size = cxBlock;
            floodDoArgs->newColorCode = GetWindowWord(hwnd, CanvasWordForeColor);
            floodDoArgs->targetColorCode = pixelFrame[pixel];

            floodUndoArgs->dataLength = PIXEL_COUNT;
            if (FALSE == canvas_draw_flood_v7(&hdc, pixelFrame, floodDoArgs, &(floodUndoArgs->dataLength), &(floodUndoArgs->colorData), &(floodUndoArgs->ptNW), &(floodUndoArgs->ptSE)))
            {
                free_history_entry(canvasHistoryReadIndex);
                MessageBox(NULL, "Flood failed", "Flood", MB_OK);
                break;
            }

            floodUndoArgs->size = cxBlock;
            // nLength = wsprintf (szBuffer, "Flood undo args. Length %d.", floodUndoArgs->dataLength);
            // MessageBox(NULL, szBuffer, "Flood", MB_OK);

            ValidateRect(hwnd, NULL);
            break;
        }

        case CanvasToolErase:
        {
            CanvasRectArgs_s *eraseDoArgs;
            CanvasRestoreArgs_s *eraseUndoArgs;
            // Treating erase as a rectangle draw so it will work better if/when I add editing only a section of the canvas

            if (FALSE == create_history_entry(CanvasToolRect))
            {
                MessageBox(NULL, "Unable to create history entry", "Erase", MB_OK);
                break;
            }

            eraseDoArgs = (CanvasRectArgs_s *)canvasHistory[canvasHistoryReadIndex]->nextAction->args;
            eraseUndoArgs = (CanvasRestoreArgs_s *)canvasHistory[canvasHistoryReadIndex]->prevAction->args;

            eraseDoArgs->size = cxBlock;
            eraseDoArgs->newColorCode = PixelColorCodeWhite;
            eraseDoArgs->pt1.x = 0;
            eraseDoArgs->pt1.y = 0;
            eraseDoArgs->pt2.x = CANVAS_DIM - 1;
            eraseDoArgs->pt2.y = CANVAS_DIM - 1;

            eraseUndoArgs->dataLength = PIXEL_COUNT;
            if (FALSE == canvas_draw_rect(&hdc, pixelFrame, eraseDoArgs, &(eraseUndoArgs->dataLength), &(eraseUndoArgs->colorData)))
            {
                free_history_entry(canvasHistoryReadIndex);
                drawState = DRAW_STATE_START;
                MessageBox(NULL, "Erase failed", "Erase", MB_OK);
                break;
            }

            eraseUndoArgs->size = cxBlock;
            eraseUndoArgs->ptNW.x = min(eraseDoArgs->pt1.x, eraseDoArgs->pt2.x);
            eraseUndoArgs->ptSE.x = max(eraseDoArgs->pt1.x, eraseDoArgs->pt2.x);
            eraseUndoArgs->ptNW.y = min(eraseDoArgs->pt1.y, eraseDoArgs->pt2.y);
            eraseUndoArgs->ptSE.y = max(eraseDoArgs->pt1.y, eraseDoArgs->pt2.y);

            ValidateRect(hwnd, NULL);
            // InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        default:
            MessageBeep(0);
            ValidateRect(hwnd, NULL);
            break; // TODO signal error here
        }

        wsprintf(szBuffer, "Write %d, read %d", canvasHistoryWriteIndex, canvasHistoryReadIndex);
        log_message(szBuffer);
        hwndParent = GetParent(hwnd);
        SendMessage(hwndParent, WM_COMMAND, GetWindowWord(hwnd, GWW_ID), 0);

        // Cleanup
        ReleaseDC(hwnd, hdc);
        return 0;
    }

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);
        

        


        
        // Rectangle(hdc, rect.left, rect.top, rect.left + 32*cxBlock, rect.top + 32*cyBlock);
        // SetBkMode(hdc, TRANSPARENT);
        // for (x = 0; x < CANVAS_DIM; x++)
        // {
        //     for (y = 0; y < CANVAS_DIM; y++)
        //     {
        //         Rectangle(hdc, rect.left + x * cxBlock, rect.top + y * cyBlock,
        //                   rect.left + x * cxBlock + cxBlock, rect.top + y * cyBlock + cyBlock);
        //     }
        // }

        hPenBlack = CreatePenIndirect(&lpBlack);
        SelectObject(hdc, hPenBlack);
        for(y=0; y<=CANVAS_DIM; y++){
            MoveTo(hdc, 0, y*cyBlock);
            LineTo(hdc, CANVAS_DIM*cxBlock, y*cyBlock);
        }
        for(x=0; x<=CANVAS_DIM; x++){
            MoveTo(hdc, x*cxBlock, 0);
            LineTo(hdc, x*cxBlock, CANVAS_DIM*cyBlock);
        }

        for(pixel=0; pixel<PIXEL_COUNT; pixel++){
            x = pixel % 32;
            y = pixel / 32;

            colorCode = pixelFrame[pixel];

            if(FALSE == pixel_color_code_to_rgb(colorCode, &newColor)){
                MessageBeep(1);
                newColor = COLOR_BLACK; //TODO signal error w/ dialog box w/ option to quit
            }

            hBrush = CreateSolidBrush(newColor);

            rect.left = x*cxBlock + 1;
            rect.top = y*cyBlock + 1;
            rect.right = rect.left + cxBlock - 2 ;
            rect.bottom = rect.top + cyBlock - 2;

            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);
        }

        EndPaint(hwnd, &ps);
        // DeleteObject(hPenBlack);
        return 0;

    case WM_DESTROY:
    {
        int x;
        for (x = 0; x < CANVAS_HISTORY_LEN; x++)
        {
            free_history_entry(x);
        }
        DeleteObject(hPen);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
