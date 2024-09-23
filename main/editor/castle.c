#include "castle.h"
#include "win.h"
#include "bun_io.h"

uint8_t get_nameinfo_for_resource(bun_file_s* pfile, castleResourceType_t castleResourceType, uint16_t index, nameInfo_t* nameInfo){
    uint32_t address = castleResourceType.nameInfoAddress + index*NAMEINFO_BYTE_LENGTH;
    read_nameinfo(pfile, address, nameInfo);
    return 1;
}
