#ifndef _COLORBOX_H_
#define _COLORBOX_H_

#include <WINDOWS.H>

#ifndef WIN31  
#include "windef.h"                     
#endif 

#include "iconedit.h"
#include "utils.h"

long FAR PASCAL _export WndProcColorBox(HWND, UINT, UINT, LONG);
extern char szNameColorBox[];
extern char* colorLabels[];

// Get/SetWindowWord offsets
typedef enum ColorBoxWindowWords{
    ColorBoxWordForeColor = 0,
    ColorBoxWordBackColor = 2        
} ColorBoxWindowWords_e;
#define COLORBOX_EXTRA_WORDS 2 

#define COLORBOX_COLS 2
#define COLORBOX_ROWS 8

#define COLOR_BTN_SIZE 4*cxChar


#endif
