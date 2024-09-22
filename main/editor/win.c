#include "win.h"
#include "io.h"


fd file_open(bun_file_s file){
    #ifdef WIN31
    return _lopen(file.name, file.mode_flags);
    #else
    return fopen(file.name, file.mode_str);
    #endif
}

void file_close(bun_file_s file){
    #ifdef WIN31
    _lclose(file.handle);
    #else
    fclose((FILE*)file.handle);
    #endif
}

int file_seek(bun_file_s file, long position, int flags){
    #ifdef WIN31
    return _llseek(file.handle, position, flags);
    #else
    return fseek((FILE*)file.info, position, flags);
    #endif
}

int file_read(bun_file_s file, void *ptr, size_t size, size_t nmemb){
    #ifdef WIN31
    return _lread(file.handle, ptr, size);
    #else
    return fread(ptr, size, nmemb, (FILE*)file.info);
    #endif
}

int file_write(bun_file_s file, void *ptr, size_t size, size_t nmemb){
    #ifdef WIN31
    return _lwrite(file.handle, ptr, size);
    #else
    return fwrite(ptr, size, nmemb, (FILE*)file.info);
    #endif
}

uint8_t dos_read_magic(bun_file_s file, dosHeader_t* header){
    // printf("Reading magic number\n");

    file_seek(file, 0, SEEK_SET);
    read_byte(file, &header->signature[0]);
    read_byte(file, &header->signature[1]);
    return 1;
}

uint8_t dos_read_table_offset(bun_file_s file, dosHeader_t* header){
    //printf("Reading table offset\n");

    file_seek(file, DOS_OFFSET_TABLE, SEEK_SET);
    read_word(file, &header->tableOffset);
    return 1;
}

uint8_t dos_read_windows_offset(bun_file_s file, dosHeader_t* header){
    //printf("Reading Windows header offset\n");

    file_seek(file, DOS_OFFSET_WINDOWS, SEEK_SET);
    read_dword(file, &header->windowsOffset);
    return 1;
}

// Windows header section

uint8_t win_read_magic(bun_file_s file, windowsHeader_t* header, uint32_t baseAddress){
    //printf("Reading magic number\n");

    file_seek(file, baseAddress, SEEK_SET);
    read_byte(file, &header->signature[0]);
    read_byte(file, &header->signature[1]);
    return 1;
}

uint8_t win_read_resource_table_offset(bun_file_s file, windowsHeader_t* header, uint32_t baseAddress){
    file_seek(file, baseAddress + WIN_OFFSET_RESOURCE_TABLE, SEEK_SET);
    read_word(file, &header->resourceTableOffset);
    return 1;
}




uint8_t rcs_table_read_shift(bun_file_s file, resourceTable_t* table){
    file_seek(file, table->baseAddress + 0, SEEK_SET);
    read_word(file, &table->rcsAlignShift);
    return 1;
}

uint8_t rt_read_type_id(bun_file_s file, typeInfoList_t* typeInfoList){
    file_seek(file, typeInfoList->address + 0, SEEK_SET);
    read_word(file, &typeInfoList->typeInfo.rtTypeID);
    return 1;
}

uint8_t rt_read_resource_count(bun_file_s file, typeInfoList_t* typeInfoList){
    file_seek(file, typeInfoList->address + 0x2, SEEK_SET);
    read_word(file, &typeInfoList->typeInfo.rtResourceCount);
    return 1;
}

uint8_t read_nameinfo(bun_file_s file, uint32_t address, nameInfo_t* nameInfo){
    file_seek(file, address, SEEK_SET);
    read_word(file, &nameInfo->rnOffset);
    read_word(file, &nameInfo->rnLength);
    read_word(file, &nameInfo->rnFlags);
    read_word(file, &nameInfo->rnID);
    return 1;
}


void print_nameinfo(nameInfo_t nameInfo){
    // printf("Offset: %#x\n", nameInfo.rnOffset);
    // printf("Length: %#x\n", nameInfo.rnLength);
    // printf("Flags: %#x\n", nameInfo.rnFlags);
    // printf("ID: %#x\n", nameInfo.rnID);
}


void access_group_icon(bun_file_s file, uint32_t address, groupIconDir_t* record){
    // printf("\tDBG address: %#"PRIx32"\n", address);
    file_seek(file, address, SEEK_SET);
    read_word(file, &record->rsvd);
    read_word(file, &record->type);
    read_word(file, &record->count);
    record->entryAddress = (address + 3*WIN_WORD_LEN);

}

void print_group_icon_dir(groupIconDir_t record){
    // printf("Rsvd: %#x\n", record.rsvd);
    // printf("Type: %#x\n", record.type);
    // printf("Count: %#x\n", record.count);
    // printf("Entries address: %#"PRIx32"\n", record.entryAddress);
}

void access_group_icon_entry(bun_file_s file, uint32_t address, uint16_t index, groupIconDirEntry_t* record){
    file_seek(file, address + index*ICON_DIR_ENTRY_BYTE_LENGTH, SEEK_SET);
    read_byte(file, &record->width);
    read_byte(file, &record->height);
    read_byte(file, &record->colorCount);
    read_byte(file, &record->rsvd);
    read_word(file, &record->planes);    
    read_word(file, &record->bitCount);
    read_dword(file, &record->bytesInRes);
    read_word(file, &record->id);    

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
