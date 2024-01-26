#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include "castle.h"
#include "win.h"

uint8_t castle_init(){
    printf("Starting process\n");
    return 1;
}



uint8_t get_nameinfo_for_resource(FILE* fileHandle, castleResourceType_t castleResourceType, uint16_t index, nameInfo_t* nameInfo){
    uint32_t address = castleResourceType.nameInfoAddress + index*NAMEINFO_BYTE_LENGTH;
    read_nameinfo(fileHandle, address, nameInfo);
    return 1;
}