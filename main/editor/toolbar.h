#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

long FAR PASCAL _export WndProcToolbar(HWND, UINT, UINT, LONG);
extern char szNameToolbar[];

#define TOOLBAR_ROWS 10
#define TOOLBAR_COLS 2
#define TOOL_BTN_SIZE 4*cxChar

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
    ToolbarToolUndo,
    ToolbarToolRedo,
    ToolbarToolErase,  
    // ToolbarToolRestore,
    ToolbarToolMAX,       
} ToolbarTool_e;

static const LPSTR toolbarToolNames[] = {"Br", "L", "Fl", "R", "Un", "Re", "E"};

#endif
