#ifndef _WIN_H_
#define _WIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN31
	#include "bunint.h"
#else
	#include <stdint.h>
	#include <inttypes.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#define WIN_CHAR_LEN 1
#define WIN_WORD_LEN 2
#define WIN_DWORD_LEN 4

#define DOS_OFFSET_TABLE 0x18
#define DOS_OFFSET_WINDOWS 0x3C




typedef struct{
    uint8_t signature[2]; // [0x4D, 0x5A] ([77d, 90d], "MZ")
    uint16_t tableOffset;
    uint32_t windowsOffset;

} dosHeader_t;

uint8_t dos_read_magic(FILE* fileHandle, dosHeader_t* header);
uint8_t dos_read_table_offset(FILE* fileHandle, dosHeader_t* header);
uint8_t dos_read_windows_offset(FILE* fileHandle, dosHeader_t* header);


#define WIN_OFFSET_RESOURCE_TABLE 0x24

typedef struct{
    uint8_t signature[2]; // [0x4E, 0x45] ([78d, 69d], "NE")
    uint16_t resourceTableOffset;

} windowsHeader_t;

uint8_t win_read_magic(FILE* fileHandle, windowsHeader_t* header, uint32_t baseAddress);
uint8_t win_read_resource_table_offset(FILE* fileHandle, windowsHeader_t* header, uint32_t baseAddress);



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


typedef struct{
    uint16_t x;
} resourceName_t;

typedef struct resourceNameList_s{
    resourceName_t resourceName;
    uint32_t address;
    struct resourceNameList_s* next;
} resourceNameList_t;


typedef struct{
    uint16_t rtTypeID;
    uint16_t rtResourceCount;
} typeInfo_t;

typedef struct typeInfoList_s{
    typeInfo_t typeInfo;
    uint32_t address;
    struct typeInfoList_s* next;
} typeInfoList_t;

typedef struct{
    uint32_t baseAddress;
    uint16_t rcsAlignShift;
    typeInfoList_t rscTypes;
    uint16_t rscEndTypes; //must be zero
    resourceNameList_t rscResourceNames;
    uint8_t rscEndNames; //must be zero

} resourceTable_t;

uint8_t rcs_table_read_shift(FILE* fileHandle, resourceTable_t* table);
uint8_t rt_read_type_id(FILE* fileHandle, typeInfoList_t* typeInfoList);
uint8_t rt_read_resource_count(FILE* fileHandle, typeInfoList_t* typeInfoList);
uint8_t read_nameinfo(FILE* fileHandle, uint32_t address, nameInfo_t* nameInfo);


void print_nameinfo(nameInfo_t nameInfo);

void access_group_icon(FILE* fileHandle, uint32_t address, groupIconDir_t* record);
void print_group_icon_dir(groupIconDir_t record);
void access_group_icon_entry(FILE* fileHandle, uint32_t address, uint16_t index, groupIconDirEntry_t* record);
void print_group_icon_dir_entry(groupIconDirEntry_t record);



#ifdef __cplusplus
}
#endif

#endif //include guard
