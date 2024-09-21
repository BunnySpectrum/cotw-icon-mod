#include "win.h"
#include "io.h"


fd file_open(char* name, fdmode mode){
    #ifdef WIN31
    return _lopen(name, mode);
    #else
    return fopen(name, mode);
    #endif
}

void file_close(fd fileHandle){
    #ifdef WIN31
    _lclose(fileHandle);
    #else
    fclose(fileHandle);
    #endif
}

int file_seek(fd fileHandle, long position, int flags){
    #ifdef WIN31
    return _llseek(fileHandle, position, flags);
    #else
    return fseek(fileHandle, position, flags);
    #endif
}

int file_read(void *ptr, size_t size, size_t nmemb, fd fileHandle){
    #ifdef WIN31
    return _lread(fileHandle, ptr, size);
    #else
    return fread(ptr, size, nmemb, fileHandle);
    #endif
}

int file_write(void *ptr, size_t size, size_t nmemb, fd fileHandle){
    #ifdef WIN31
    return _lwrite(fileHandle, ptr, size);
    #else
    return fwrite(ptr, size, nmemb, fileHandle);
    #endif
}

uint8_t dos_read_magic(fd fileHandle, dosHeader_t* header){
    // printf("Reading magic number\n");

    file_seek(fileHandle, 0, SEEK_SET);
    read_byte(fileHandle, &header->signature[0]);
    read_byte(fileHandle, &header->signature[1]);
    return 1;
}

uint8_t dos_read_table_offset(fd fileHandle, dosHeader_t* header){
    //printf("Reading table offset\n");

    file_seek(fileHandle, DOS_OFFSET_TABLE, SEEK_SET);
    read_word(fileHandle, &header->tableOffset);
    return 1;
}

uint8_t dos_read_windows_offset(fd fileHandle, dosHeader_t* header){
    //printf("Reading Windows header offset\n");

    file_seek(fileHandle, DOS_OFFSET_WINDOWS, SEEK_SET);
    read_dword(fileHandle, &header->windowsOffset);
    return 1;
}

// Windows header section

uint8_t win_read_magic(fd fileHandle, windowsHeader_t* header, uint32_t baseAddress){
    //printf("Reading magic number\n");

    file_seek(fileHandle, baseAddress, SEEK_SET);
    read_byte(fileHandle, &header->signature[0]);
    read_byte(fileHandle, &header->signature[1]);
    return 1;
}

uint8_t win_read_resource_table_offset(fd fileHandle, windowsHeader_t* header, uint32_t baseAddress){
    file_seek(fileHandle, baseAddress + WIN_OFFSET_RESOURCE_TABLE, SEEK_SET);
    read_word(fileHandle, &header->resourceTableOffset);
    return 1;
}




uint8_t rcs_table_read_shift(fd fileHandle, resourceTable_t* table){
    file_seek(fileHandle, table->baseAddress + 0, SEEK_SET);
    read_word(fileHandle, &table->rcsAlignShift);
    return 1;
}

uint8_t rt_read_type_id(fd fileHandle, typeInfoList_t* typeInfoList){
    file_seek(fileHandle, typeInfoList->address + 0, SEEK_SET);
    read_word(fileHandle, &typeInfoList->typeInfo.rtTypeID);
    return 1;
}

uint8_t rt_read_resource_count(fd fileHandle, typeInfoList_t* typeInfoList){
    file_seek(fileHandle, typeInfoList->address + 0x2, SEEK_SET);
    read_word(fileHandle, &typeInfoList->typeInfo.rtResourceCount);
    return 1;
}

uint8_t read_nameinfo(fd fileHandle, uint32_t address, nameInfo_t* nameInfo){
    file_seek(fileHandle, address, SEEK_SET);
    read_word(fileHandle, &nameInfo->rnOffset);
    read_word(fileHandle, &nameInfo->rnLength);
    read_word(fileHandle, &nameInfo->rnFlags);
    read_word(fileHandle, &nameInfo->rnID);
    return 1;
}


void print_nameinfo(nameInfo_t nameInfo){
    // printf("Offset: %#x\n", nameInfo.rnOffset);
    // printf("Length: %#x\n", nameInfo.rnLength);
    // printf("Flags: %#x\n", nameInfo.rnFlags);
    // printf("ID: %#x\n", nameInfo.rnID);
}


void access_group_icon(fd fileHandle, uint32_t address, groupIconDir_t* record){
    // printf("\tDBG address: %#"PRIx32"\n", address);
    file_seek(fileHandle, address, SEEK_SET);
    read_word(fileHandle, &record->rsvd);
    read_word(fileHandle, &record->type);
    read_word(fileHandle, &record->count);
    record->entryAddress = (address + 3*WIN_WORD_LEN);

}

void print_group_icon_dir(groupIconDir_t record){
    // printf("Rsvd: %#x\n", record.rsvd);
    // printf("Type: %#x\n", record.type);
    // printf("Count: %#x\n", record.count);
    // printf("Entries address: %#"PRIx32"\n", record.entryAddress);
}

void access_group_icon_entry(fd fileHandle, uint32_t address, uint16_t index, groupIconDirEntry_t* record){
    file_seek(fileHandle, address + index*ICON_DIR_ENTRY_BYTE_LENGTH, SEEK_SET);
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
    // printf("Width: %#x\n", record.width);
    // printf("Height: %#x\n", record.height);
    // printf("Color count: %#x\n", record.colorCount); 
    // printf("Rsvd: %#x\n", record.rsvd);
    // printf("Planes: %#x\n", record.planes);
    // printf("Bit count: %#x\n", record.bitCount);
    // printf("Bytes in res: %#x\n", record.bytesInRes);
    // printf("Id: %#x\n", record.id);
}
