#include <stdint.h>






typedef struct{
    uint8_t signature[2]; // [0x4E, 0x45] ([78d, 69d], "NE")
    uint16_t resourceTableOffset;

} windowsHeader_t;

#define WIN_OFFSET_RESOURCE_TABLE 0x24



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






