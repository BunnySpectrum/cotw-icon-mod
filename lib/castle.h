#ifndef _CASTLE_H_
#define _CASTLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include "bunint.h"
#include "win.h"


uint8_t castle_init(void);

/*
34 GROUP_CURSOR at 0x542
34 CURSOR at 0x2CFE

236 GROUP_ICON at 0x6E2
    15 point to 1 icon only, rest point to two
457 ICON at 0x178A

98 BITMAP at 0x11FA

3 MENU at 0x169A
14 DIALOG at 0x16C6

1 RCDATA at 0x1776
15 0x8104 at 0x2E9E
2 0x8105 at 0x2F5A
*/

typedef struct castleResourceType_s{
    uint16_t resourceCount;
    uint32_t nameInfoAddress;
} castleResourceType_t;

typedef struct castleResources_s{
    castleResourceType_t groupCursor;
    castleResourceType_t cursor;
    castleResourceType_t groupIcon;
    castleResourceType_t icon;
    castleResourceType_t bitmap;
    castleResourceType_t menu;
    castleResourceType_t dialog;
} castleResources_t;


#define GROUP_ICON_ID_PLAYER_FEM 208
#define ICON_ENTRY_PLAYER_FEM_COLOR 1

#define GROUP_ICON_ID_CUBE 216
#define ICON_ENTRY_CUBE_COLOR 1

uint8_t get_nameinfo_for_resource(FILE* fileHandle, castleResourceType_t castleResourceType, uint16_t index, nameInfo_t* nameInfo);

#ifdef __cplusplus
}
#endif

#endif //castle.h include guard
