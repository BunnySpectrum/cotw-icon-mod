#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <WINDOWS.H>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WIN31  
#include "windef.h"                     
#endif 

#include "utils.h"
#include "log.h"
#include "image.h"

long FAR PASCAL _export WndProcCanvas(HWND, UINT, UINT, LONG);
extern char szNameCanvas[];
#define CANVAS_HISTORY_LEN 16


ReturnCode_e FAR PASCAL _export copy_bmp_to_canvas(BitmapFields_s bitmap);
ReturnCode_e FAR PASCAL _export copy_icon_to_canvas(IconFields_s icon);

ReturnCode_e FAR PASCAL _export copy_canvas_to_img(BYTE huge *lpImg, ImageFileType_e fileType);
void FAR PASCAL _export build_image_mask_from_canvas(ImageMask_s* imageMask);



#define CANVAS_DIM 32
#define PIXEL_COUNT (CANVAS_DIM) * (CANVAS_DIM)
#define PIXEL_2D_2_1D(col, row)  ((col) + (row)*(CANVAS_DIM))
#define PIXEL_1D_2_ROW(pixel) ((pixel) / (CANVAS_DIM))
#define PIXEL_1D_2_COL(pixel) ((pixel) % (CANVAS_DIM))
#define PIXEL_BQUEUE_LEN ((PIXEL_COUNT)/8)
static BYTE pixelFrame[PIXEL_COUNT];

// Get/SetWindowWord offsets
typedef enum CanvasWindowWords{
    CanvasWordForeColor = 0,
    CanvasWordBackColor = 2,
    CanvasWordTool = 4,        
} CanvasWindowWords_e;
#define CANVAS_EXTRA_WORDS 3

// Canvas tools are invoked with selecting a pixel
typedef enum CanvasTool{
    CanvasToolBrush = 0,
    CanvasToolLine,
    CanvasToolFlood,
    CanvasToolRect,
    CanvasToolNull1,
    CanvasToolNull2,
    CanvasToolErase,  
    CanvasToolRestore         
} CanvasTool_e;

// Canvas modifiers affect either the active tool or a section of the canvas
typedef enum CanvasModifier{
    CanvasModifierMirrorDraw = 0,
    CanvasModifierFlipH,
    CanvasModifierFlipV,
    CanvasModifierRotate,
    CanvasModifierInvert,
    CanvasModifierBorderR,
    CanvasModifierBorderS,          
} CanvasModifier_e;



#define PIXEL_CHANNEL_COUNT 3 //red, green, blue
#define PIXEL_CHANNEL_BYTES 1 //0-255
#define FRAME_BUFFER_BYTE_COUNT PIXEL_COUNT * PIXEL_CHANNEL_COUNT * PIXEL_CHANNEL_BYTES

// #define CANVAS_DATA_WW 0
// #define CANVAS_DATA_STATIC 1
// #define CANVAS_DATA CANVAS_DATA_STATIC

#define DRAW_STATE_START 0
#define DRAW_LINE_FIRST 1
#define DRAW_LINE_2ND 2


typedef struct CanvasBrushArgs{
    int pixel;
    short size;
    PixelColorCode_e newColorCode;
}CanvasBrushArgs_s;

typedef struct CanvasLineArgs{
    short size;
    PixelColorCode_e newColorCode;
    POINT pt1;
    POINT pt2;
}CanvasLineArgs_s;

typedef struct CanvasRectArgs{
    short size;
    PixelColorCode_e newColorCode;
    POINT pt1;
    POINT pt2;
}CanvasRectArgs_s;

typedef struct CanvasFloodArgs{
    int pixel;
    short size;
    PixelColorCode_e newColorCode;
    PixelColorCode_e targetColorCode;
}CanvasFloodArgs_s;

typedef struct CanvasRestoreArgs{
    short size;
    POINT ptNW;
    POINT ptSE;
    int dataLength;
    BYTE* colorData;
}CanvasRestoreArgs_s;

typedef struct CanvasAction{
    CanvasTool_e tool;
    void* args; //change this to a union of arg structs
}CanvasAction_s;


typedef struct CanvasHistoryEntry{
    BOOL valid;
    CanvasAction_s* nextAction;
    CanvasAction_s* prevAction;
}CanvasHistoryEntry_s;

extern int canvasHistoryWriteIndex, canvasHistoryReadIndex;
extern CanvasHistoryEntry_s* canvasHistory[CANVAS_HISTORY_LEN];

#endif
