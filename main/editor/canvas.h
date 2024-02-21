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
#define CANVAS_FORE_COLOR 0
#define CANVAS_BACK_COLOR CANVAS_FORE_COLOR + 2
#define PIXEL_CHANNEL_COUNT 3 //red, green, blue
#define PIXEL_CHANNEL_BYTES 1 //0-255
#define FRAME_BUFFER_BYTE_COUNT PIXEL_COUNT * PIXEL_CHANNEL_COUNT * PIXEL_CHANNEL_BYTES

// #define CANVAS_DATA_WW 0
// #define CANVAS_DATA_STATIC 1
// #define CANVAS_DATA CANVAS_DATA_STATIC



#endif
