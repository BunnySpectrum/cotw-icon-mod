#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

long FAR PASCAL _export WndProcCanvas(HWND, UINT, UINT, LONG);
char szNameCanvas[] = "Canvas";

#define CANVAS_DIM 32
#define PIXEL_COUNT CANVAS_DIM * CANVAS_DIM
#define PIXEL_2D_2_1D(col, row)  ((col) + (row)*(CANVAS_DIM))
#define PIXEL_1D_2_ROW(pixel) ((pixel) / CANVAS_DIM)
#define PIXEL_1D_2_COL(pixel) ((pixel) % CANVAS_DIM)

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
    CanvasToolEllipse, 
    CanvasToolErase,    
    CanvasToolSelect,       
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

#endif
