#ifndef _WIN_H_
#define _WIN_H_

#include <stdint.h>
#define WIN_CHAR_LEN 1
#define WIN_WORD_LEN 2
#define WIN_DWORD_LEN 4


typedef struct{
    uint8_t signature[2]; // [0x4D, 0x5A] ([77d, 90d], "MZ")
    uint16_t tableOffset;
    uint32_t windowsOffset;

} dosHeader_t;

#define DOS_OFFSET_TABLE 0x18
#define DOS_OFFSET_WINDOWS 0x3C

enum ResourceType{
    RT_CURSOR = 0x1,
    RT_BITMAP = 0x2,
    RT_ICON = 0x3,
    RT_MENU = 0x4,
    RT_DIALOG = 0x5,
    RT_STRING = 0x6,
    RT_FONTDIR = 0x7,
    RT_FONT = 0x8,
    RT_ACCELERATOR = 0x9,
    RT_RCDATA = 0xA,
    RT_GROUP_CURSOR = RT_CURSOR + 11, //0xC
    RT_GROUP_ICON = RT_ICON + 11, //0xE
};

#define NAMEINFO_BYTE_LENGTH 12

typedef struct nameInfo_s{
    uint16_t rnOffset;
    uint16_t rnLength;
    uint16_t rnFlags;
    uint16_t rnID;
    //uint16_t rnHandle;
    //uint16_t rnUsage;
} nameInfo_t;

typedef struct groupIconDirEntry_s
{
    uint8_t  width;
    uint8_t  height;
    uint8_t  colorCount;
    uint8_t  rsvd;
    uint16_t  planes;
    uint16_t  bitCount;
    uint32_t bytesInRes;
    uint16_t  id;
} groupIconDirEntry_t;
#define ICON_DIR_ENTRY_BYTE_LENGTH 14

typedef struct groupIconDir_s
{
    uint16_t rsvd;
    uint16_t type;
    uint16_t count;
    uint32_t entryAddress;
} groupIconDir_t;

typedef struct bitmapFileHeader_s{
    uint16_t type; //should be BM?
    uint32_t size; //size of file in bytes
    uint16_t rsvd1;
    uint16_t rsvd2;
    uint32_t offBits; //byte offset to bitmap data    
} bitmapFileHeader_t;

#endif