#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

long FAR PASCAL _export WndProcToolbar(HWND, UINT, UINT, LONG);
char szNameToolbar[] = "Toolbar";

#define TOOLBAR_ROWS 10
#define TOOLBAR_COLS 2

// Get/SetWindowWord offsets
typedef enum ToolbarWindowWords{
    ToolbarWordTool = 0,    
} CanvasWindowWords_e;
#define TOOLBAR_EXTRA_WORDS 1 

// Canvas tools are invoked with selecting a pixel
typedef enum ToolbarTool{
    ToolbarToolBrush = 0,
    ToolbarToolLine,
    ToolbarToolFlood,
    ToolbarToolRect,
    ToolbarToolErase,    
    ToolbarToolMAX,       
} ToolbarTool_e;

const LPSTR toolbarToolNames[] = {"Brush", "Line", "Flood", "Rect", "Erase", ""};

#endif
