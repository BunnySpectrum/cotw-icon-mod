#include "win.h"
#include "io.h"

uint8_t dos_read_magic(FILE* fileHandle, dosHeader_t* header){
    printf("Reading magic number\n");

    fseek(fileHandle, 0, SEEK_SET);
    read_byte(fileHandle, &header->signature[0]);
    read_byte(fileHandle, &header->signature[1]);
    return 1;
}

uint8_t dos_read_table_offset(FILE* fileHandle, dosHeader_t* header){
    //printf("Reading table offset\n");

    fseek(fileHandle, DOS_OFFSET_TABLE, SEEK_SET);
    read_word(fileHandle, &header->tableOffset);
    return 1;
}

uint8_t dos_read_windows_offset(FILE* fileHandle, dosHeader_t* header){
    //printf("Reading Windows header offset\n");

    fseek(fileHandle, DOS_OFFSET_WINDOWS, SEEK_SET);
    read_dword(fileHandle, &header->windowsOffset);
    return 1;
}

// Windows header section

uint8_t win_read_magic(FILE* fileHandle, windowsHeader_t* header, uint32_t baseAddress){
    //printf("Reading magic number\n");

    fseek(fileHandle, baseAddress, SEEK_SET);
    read_byte(fileHandle, &header->signature[0]);
    read_byte(fileHandle, &header->signature[1]);
    return 1;
}

uint8_t win_read_resource_table_offset(FILE* fileHandle, windowsHeader_t* header, uint32_t baseAddress){
    fseek(fileHandle, baseAddress + WIN_OFFSET_RESOURCE_TABLE, SEEK_SET);
    read_word(fileHandle, &header->resourceTableOffset);
    return 1;
}




uint8_t rcs_table_read_shift(FILE* fileHandle, resourceTable_t* table){
    fseek(fileHandle, table->baseAddress + 0, SEEK_SET);
    read_word(fileHandle, &table->rcsAlignShift);
    return 1;
}

uint8_t rt_read_type_id(FILE* fileHandle, typeInfoList_t* typeInfoList){
    fseek(fileHandle, typeInfoList->address + 0, SEEK_SET);
    read_word(fileHandle, &typeInfoList->typeInfo.rtTypeID);
    return 1;
}

uint8_t rt_read_resource_count(FILE* fileHandle, typeInfoList_t* typeInfoList){
    fseek(fileHandle, typeInfoList->address + 0x2, SEEK_SET);
    read_word(fileHandle, &typeInfoList->typeInfo.rtResourceCount);
    return 1;
}

uint8_t read_nameinfo(FILE* fileHandle, uint32_t address, nameInfo_t* nameInfo){
    fseek(fileHandle, address, SEEK_SET);
    read_word(fileHandle, &nameInfo->rnOffset);
    read_word(fileHandle, &nameInfo->rnLength);
    read_word(fileHandle, &nameInfo->rnFlags);
    read_word(fileHandle, &nameInfo->rnID);
    return 1;
}


void print_nameinfo(nameInfo_t nameInfo){
    printf("Offset: %#x\n", nameInfo.rnOffset);
    printf("Length: %#x\n", nameInfo.rnLength);
    printf("Flags: %#x\n", nameInfo.rnFlags);
    printf("ID: %#x\n", nameInfo.rnID);
}


void access_group_icon(FILE* fileHandle, uint32_t address, groupIconDir_t* record){
    printf("\tDBG address: %#"PRIx32"\n", address);
    fseek(fileHandle, address, SEEK_SET);
    read_word(fileHandle, &record->rsvd);
    read_word(fileHandle, &record->type);
    read_word(fileHandle, &record->count);
    record->entryAddress = (address + 3*WIN_WORD_LEN);

}

void print_group_icon_dir(groupIconDir_t record){
    printf("Rsvd: %#x\n", record.rsvd);
    printf("Type: %#x\n", record.type);
    printf("Count: %#x\n", record.count);
    printf("Entries address: %#"PRIx32"\n", record.entryAddress);
}

void access_group_icon_entry(FILE* fileHandle, uint32_t address, uint16_t index, groupIconDirEntry_t* record){
    fseek(fileHandle, address + index*ICON_DIR_ENTRY_BYTE_LENGTH, SEEK_SET);
    read_byte(fileHandle, &record->width);
    read_byte(fileHandle, &record->height);
    read_byte(fileHandle, &record->colorCount);
    read_byte(fileHandle, &record->rsvd);
    read_word(fileHandle, &record->planes);    
    read_word(fileHandle, &record->bitCount);
    read_dword(fileHandle, &record->bytesInRes);
    read_word(fileHandle, &record->id);    

}

void print_group_icon_dir_entry(groupIconDirEntry_t record){
    printf("Width: %#x\n", record.width);
    printf("Height: %#x\n", record.height);
    printf("Color count: %#x\n", record.colorCount); 
    printf("Rsvd: %#x\n", record.rsvd);
    printf("Planes: %#x\n", record.planes);
    printf("Bit count: %#x\n", record.bitCount);
    printf("Bytes in res: %#x\n", record.bytesInRes);
    printf("Id: %#x\n", record.id);
}
