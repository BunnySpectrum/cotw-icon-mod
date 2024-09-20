#include "castle.h"
#include "win.h"


uint8_t get_nameinfo_for_resource(FILE* fileHandle, castleResourceType_t castleResourceType, uint16_t index, nameInfo_t* nameInfo){
    uint32_t address = castleResourceType.nameInfoAddress + index*NAMEINFO_BYTE_LENGTH;
    read_nameinfo(fileHandle, address, nameInfo);
    return 1;
}
