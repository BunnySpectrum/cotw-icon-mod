#include "win.h"
#include "io.h"


fd file_open(bun_file_s* pfile){
    #ifdef WIN31
    return _lopen(pfile->name, pfile->mode_flags);
    #else
    return fopen(pfile->name, pfile->mode_str);
    #endif
}

void file_close(bun_file_s* pfile){
    #ifdef WIN31
    _lclose(pfile->handle);
    #else
    fclose((FILE*)pfile->handle);
    #endif
}

long file_seek(bun_file_s* pfile, long position, int flags){
    #ifdef WIN31
    return _llseek(pfile->handle, position, flags);
    #else
    return fseek((FILE*)pfile->info, position, flags);
    #endif
}

int file_read(bun_file_s* pfile, void *ptr, size_t size, size_t nmemb){
    #ifdef WIN31
    return (size == _lread(pfile->handle, (LPSTR)ptr, size));
    #else
    return fread(ptr, size, nmemb, (FILE*)pfile->info);
    #endif
}

int file_write(bun_file_s* pfile, void *ptr, size_t size, size_t nmemb){
    #ifdef WIN31
    return _lwrite(pfile->handle, ptr, size);
    #else
    return fwrite(ptr, size, nmemb, (FILE*)pfile->info);
    #endif
}

int put_char(char data, bun_file_s* pfile){
    return file_write(pfile, &data, 1, 0);
}

uint8_t dos_read_magic(bun_file_s* pfile, dosHeader_t* header){
    // printf("Reading magic number\n");

    file_seek(pfile, 0, SEEK_SET);
    read_byte(pfile, &header->signature[0]);
    read_byte(pfile, &header->signature[1]);
    return 1;
}

uint8_t dos_read_table_offset(bun_file_s* pfile, dosHeader_t* header){
    //printf("Reading table offset\n");

    file_seek(pfile, DOS_OFFSET_TABLE, SEEK_SET);
    read_word(pfile, &header->tableOffset);
    return 1;
}

uint8_t dos_read_windows_offset(bun_file_s* pfile, dosHeader_t* header){
    //printf("Reading Windows header offset\n");

    file_seek(pfile, DOS_OFFSET_WINDOWS, SEEK_SET);
    read_dword(pfile, &header->windowsOffset);
    return 1;
}

// Windows header section

uint8_t win_read_magic(bun_file_s* pfile, windowsHeader_t* header, uint32_t baseAddress){
    //printf("Reading magic number\n");

    file_seek(pfile, baseAddress, SEEK_SET);
    read_byte(pfile, &header->signature[0]);
    read_byte(pfile, &header->signature[1]);
    return 1;
}

uint8_t win_read_resource_table_offset(bun_file_s* pfile, windowsHeader_t* header, uint32_t baseAddress){
    file_seek(pfile, baseAddress + WIN_OFFSET_RESOURCE_TABLE, SEEK_SET);
    read_word(pfile, &header->resourceTableOffset);
    return 1;
}




uint8_t rcs_table_read_shift(bun_file_s* pfile, resourceTable_t* table){
    file_seek(pfile, table->baseAddress + 0, SEEK_SET);
    read_word(pfile, &table->rcsAlignShift);
    return 1;
}

uint8_t rt_read_type_id(bun_file_s* pfile, typeInfoList_t* typeInfoList){
    file_seek(pfile, typeInfoList->address + 0, SEEK_SET);
    read_word(pfile, &typeInfoList->typeInfo.rtTypeID);
    return 1;
}

uint8_t rt_read_resource_count(bun_file_s* pfile, typeInfoList_t* typeInfoList){
    file_seek(pfile, typeInfoList->address + 0x2, SEEK_SET);
    read_word(pfile, &typeInfoList->typeInfo.rtResourceCount);
    return 1;
}

uint8_t read_nameinfo(bun_file_s* pfile, uint32_t address, nameInfo_t* nameInfo){
    file_seek(pfile, address, SEEK_SET);
    read_word(pfile, &nameInfo->rnOffset);
    read_word(pfile, &nameInfo->rnLength);
    read_word(pfile, &nameInfo->rnFlags);
    read_word(pfile, &nameInfo->rnID);
    return 1;
}


void print_nameinfo(nameInfo_t nameInfo){
    // printf("Offset: %#x\n", nameInfo.rnOffset);
    // printf("Length: %#x\n", nameInfo.rnLength);
    // printf("Flags: %#x\n", nameInfo.rnFlags);
    // printf("ID: %#x\n", nameInfo.rnID);
}


void access_group_icon(bun_file_s* pfile, uint32_t address, groupIconDir_t* record){
    // printf("\tDBG address: %#"PRIx32"\n", address);
    file_seek(pfile, address, SEEK_SET);
    read_word(pfile, &record->rsvd);
    read_word(pfile, &record->type);
    read_word(pfile, &record->count);
    record->entryAddress = (address + 3*WIN_WORD_LEN);

}

void print_group_icon_dir(groupIconDir_t record){
    // printf("Rsvd: %#x\n", record.rsvd);
    // printf("Type: %#x\n", record.type);
    // printf("Count: %#x\n", record.count);
    // printf("Entries address: %#"PRIx32"\n", record.entryAddress);
}

void access_group_icon_entry(bun_file_s* pfile, uint32_t address, uint16_t index, groupIconDirEntry_t* record){
    file_seek(pfile, address + index*ICON_DIR_ENTRY_BYTE_LENGTH, SEEK_SET);
    read_byte(pfile, &record->width);
    read_byte(pfile, &record->height);
    read_byte(pfile, &record->colorCount);
    read_byte(pfile, &record->rsvd);
    read_word(pfile, &record->planes);    
    read_word(pfile, &record->bitCount);
    read_dword(pfile, &record->bytesInRes);
    read_word(pfile, &record->id);    

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
