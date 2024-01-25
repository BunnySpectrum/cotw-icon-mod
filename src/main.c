
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include "main.h"
#include "win.h"
#include "castle.h"

//TODO replace return code w/ enum
uint8_t read_byte(FILE* fileHandle, uint8_t* result){
    return WIN_CHAR_LEN == fread(result, sizeof(uint8_t), WIN_CHAR_LEN, fileHandle);
}

uint8_t read_word(FILE* fileHandle, uint16_t* result){
    uint8_t buf[WIN_WORD_LEN];
    uint8_t numRead;
    
    numRead = fread(buf, sizeof(uint8_t), WIN_WORD_LEN, fileHandle);
    if (numRead < WIN_WORD_LEN){
        return 0;
    }
    *result = buf[0] | (buf[1] << 8);

    return 1;
}

uint8_t read_dword(FILE* fileHandle, uint32_t* result){
    uint8_t buf[WIN_DWORD_LEN];
    uint8_t numRead;
    
    numRead = fread(buf, sizeof(uint8_t), WIN_DWORD_LEN, fileHandle);
    if (numRead < WIN_DWORD_LEN){
        return 0;
    }
    *result = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

    return 1;
}


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

uint8_t get_nameinfo_for_resource(FILE* fileHandle, castleResourceType_t castleResourceType, uint16_t index, nameInfo_t* nameInfo){
    uint32_t address = castleResourceType.nameInfoAddress + index*NAMEINFO_BYTE_LENGTH;
    read_nameinfo(fileHandle, address, nameInfo);
    return 1;
}

void print_nameinfo(nameInfo_t nameInfo){
    printf("Offset: %#x\n", nameInfo.rnOffset);
    printf("Length: %#x\n", nameInfo.rnLength);
    printf("Flags: %#x\n", nameInfo.rnFlags);
    printf("ID: %#x\n", nameInfo.rnID);
}


void access_group_icon(FILE* fileHandle, uint32_t address, groupIconDir_t* record){
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
    printf("Entries address: %#x\n", record.entryAddress);
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

//iconIndex is relative to the dirEntry
void write_ico(FILE* exeFile, FILE* fileHandle, groupIconDirEntry_t dirEntry, castleResourceType_t iconResource, nameInfo_t iconNameInfo){
    // little endian file
    //icon dir
    // write [00, 00] rsvd
    putc(0x00, fileHandle);
    putc(0x00, fileHandle);

    // write [01, 00] 1 = icon, 2 = cursor
    putc(0x01, fileHandle);
    putc(0x00, fileHandle);

    // write [01, 00] number of icons in file, will always be 1 for this function
    putc(0x01, fileHandle);
    putc(0x00, fileHandle);

    // write contents of icondirentry
    // except offset 12 is offset to BMP data from start of file
    putc(dirEntry.width, fileHandle);
    putc(dirEntry.height, fileHandle);
    putc(dirEntry.colorCount, fileHandle);
    putc(dirEntry.rsvd, fileHandle);

    putc(dirEntry.planes & 0xFF, fileHandle);
    putc((dirEntry.planes >> 8)&0xFF, fileHandle);

    putc(dirEntry.bitCount & 0xFF, fileHandle);
    putc((dirEntry.bitCount >> 8)&0xFF, fileHandle);

    putc(dirEntry.bytesInRes & 0xFF, fileHandle);
    putc((dirEntry.bytesInRes >> 8)&0xFF, fileHandle);
    putc((dirEntry.bytesInRes >> 16)&0xFF, fileHandle);
    putc((dirEntry.bytesInRes >> 24)&0xFF, fileHandle);

    // write offset
    putc(0x16, fileHandle);
    putc(0x00, fileHandle);
    putc(0x00, fileHandle);
    putc(0x00, fileHandle);        

    //need to write rnLength bytes from offset<<4
    //printf("DBG: Writing %"PRIu16" bytes from %"PRIu16"\n", dirEntry.bytesInRes, iconNameInfo.rnOffset << 4);
    fseek(exeFile, iconNameInfo.rnOffset << 4, SEEK_SET);
    uint8_t data;
    for(uint32_t addr=0; addr < dirEntry.bytesInRes; addr++){
        read_byte(exeFile, &data);
        putc(data, fileHandle);
    }

}


void replace_ico(FILE* exeFile, FILE* iconFile, groupIconDirEntry_t dirEntry, castleResourceType_t iconResource, nameInfo_t iconNameInfo){

    //cheating - I know the offset is 0x16
    fseek(iconFile, 0x16, SEEK_SET);


    //need to replace rnLength bytes from offset<<4
    printf("Seek to %#x, write %#x bytes\n", iconNameInfo.rnOffset << 4, dirEntry.bytesInRes);
    fseek(exeFile, iconNameInfo.rnOffset << 4, SEEK_SET);

    uint8_t data;
    uint32_t addr;
    uint32_t wrote;
    for(addr=0; addr < dirEntry.bytesInRes; addr++){
        read_byte(iconFile, &data);
        // read_byte(exeFile, &data);
        // printf("%#x ", data);
        wrote = fwrite(&data, 1, 1, exeFile);
        // printf("Wrote %d\n", wrote);
    }
    printf("\nWrote %#x bytes\n", addr);

}


int main(){ 
    castle_init();

    FILE *fp;


    uint8_t scratch8;
    uint16_t scratch16;
    uint32_t scratch32;

    dosHeader_t dosHeader;
    windowsHeader_t winHeader;
    resourceTable_t resourceTable;

    fp = fopen("CASTLE1.EXE", "rb");

    dos_read_magic(fp, &dosHeader);
    dos_read_table_offset(fp, &dosHeader);
    dos_read_windows_offset(fp, &dosHeader);


    printf("***DOS header***\n");
    printf("Signature: %c%c\n", dosHeader.signature[0], dosHeader.signature[1]);
    printf("Table offset: %#x\n", dosHeader.tableOffset);
    printf("Windows offset: %#x\n", dosHeader.windowsOffset);


    // Start reading windows header
    //  Address offsets are relative to dosHeader.windowsOffset (0x400) unless otherwise specified

    //Check for magic number at 0x00
    // byte 0 should be 0x4E (N), byte 1 should be 0x45 (E)
    win_read_magic(fp, &winHeader, dosHeader.windowsOffset);
    win_read_resource_table_offset(fp, &winHeader, dosHeader.windowsOffset);

    printf("\n***Windows header***\n");
    printf("Signature: %c%c\n", winHeader.signature[0], winHeader.signature[1]);
    printf("Resource table offset: %#0*x rel, %#x abs\n", 6, winHeader.resourceTableOffset, winHeader.resourceTableOffset + dosHeader.windowsOffset);


    // Start reading resource table
    resourceTable.baseAddress = winHeader.resourceTableOffset + dosHeader.windowsOffset;
    rcs_table_read_shift(fp, &resourceTable);

    printf("\n***Resource Table***\n");
    printf("Alignment shift count: %d\n", resourceTable.rcsAlignShift);

    uint32_t currentTypeInfoAddress = resourceTable.baseAddress + 0x2;
    uint32_t rcsResourceNamesAddress;
    typeInfoList_t typeInfoList;
    castleResources_t castleResources;
    
    do{
        typeInfoList.address = currentTypeInfoAddress;
        rt_read_type_id(fp, &typeInfoList);

        // When we see a type of 0, it means we've left the array. So we're done
        if (typeInfoList.typeInfo.rtTypeID == 0x0){
            printf("rcsEndTypes at %#x\n", currentTypeInfoAddress);
            rcsResourceNamesAddress = currentTypeInfoAddress + 0x2;
            break;
        }

        rt_read_resource_count(fp, &typeInfoList);
        printf("\tTypeinfo @ %#x: type %#x, count %d\n", typeInfoList.address, typeInfoList.typeInfo.rtTypeID, typeInfoList.typeInfo.rtResourceCount);


        switch(typeInfoList.typeInfo.rtTypeID & 0xFFF){
            case RT_GROUP_CURSOR:
                castleResources.groupCursor.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.groupCursor.nameInfoAddress = typeInfoList.address + 0x8;
                break;
            
            case RT_CURSOR:
                castleResources.cursor.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.cursor.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case RT_GROUP_ICON:
                castleResources.groupIcon.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.groupIcon.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case RT_ICON:
                castleResources.icon.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.icon.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case RT_BITMAP:
                castleResources.bitmap.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.bitmap.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case RT_MENU:
                castleResources.menu.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.menu.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case RT_DIALOG:
                castleResources.dialog.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.dialog.nameInfoAddress = typeInfoList.address + 0x8;
                break;

        }


        // if ( (typeInfoList.typeInfo.rtTypeID & 0xFFF) == RT_BITMAP){
        //     printf("Look at bitmap\n");
        //     uint32_t currentNameInfoAddress = typeInfoList.address + 0x8;
        //     for(uint16_t idx=0; idx < typeInfoList.typeInfo.rtResourceCount; idx++){
        //         fseek(fp, currentNameInfoAddress, SEEK_SET);

        //         read_word(fp, &scratch16);
        //         printf("rnOffset: %#x\n", scratch16);

        //         read_word(fp, &scratch16);
        //         printf("rnLength: %#x\n", scratch16);

        //         read_word(fp, &scratch16);
        //         printf("rnFlags: %#x\n", scratch16);

        //         read_word(fp, &scratch16);
        //         printf("rnID: %#x\n", scratch16);
                
        //         printf("\n");
        //         currentNameInfoAddress += NAMEINFO_BYTE_LENGTH;
        //     }
        // }




        currentTypeInfoAddress += 0x8 + typeInfoList.typeInfo.rtResourceCount*6*2;
    } while(typeInfoList.typeInfo.rtTypeID != 0x0);
    // first word of a TYPEINFO is the typeID. When we read a typeID of 0x0 we know we are done


    nameInfo_t nameInfo;
    // get_nameinfo_for_resource(fp, castleResources.groupCursor, 0, &nameInfo);
    // printf("Group cursor 0\n");
    // print_nameinfo(nameInfo);
    // printf("\n");

    // get_nameinfo_for_resource(fp, castleResources.cursor, 0, &nameInfo);
    // printf("Cursor 0\n");
    // print_nameinfo(nameInfo);
    // printf("\n");

    uint16_t groupIconNum;
    uint16_t endIndex = castleResources.groupIcon.resourceCount;

    groupIconDir_t groupIconDir;
    groupIconDirEntry_t groupIconDirEntry;
    nameInfo_t groupIconNameInfo;
    nameInfo_t iconNameInfo;
    FILE* iconFile;

    for(groupIconNum = 0; groupIconNum < endIndex; groupIconNum++){
        // printf("*** Icon Dir %d ***\n", groupIconNum);
        get_nameinfo_for_resource(fp, castleResources.groupIcon, groupIconNum, &nameInfo);
        access_group_icon(fp, nameInfo.rnOffset << 4, &groupIconDir);

        // walk through all entries
        // for(uint16_t entryNum = 0; entryNum < groupIconDir.count; entryNum++){
        //     // printf("Entry %d/%d\n", entryNum, groupIconDir.count-1);
        //     access_group_icon_entry(fp, groupIconDir.entryAddress, entryNum, &groupIconDirEntry);
        //     // print_group_icon_dir_entry(groupIconDirEntry);
        //     // printf("\n");

        //     char* fileName;
        //     asprintf(&fileName, "icons/group_%"PRIu16"_entry_%"PRIu16".ico", groupIconNum, entryNum);
        //     iconFile = fopen(fileName, "wb");
        //     free(fileName);

        //     if (!iconFile)
        //     {
        //         printf("Unable to open file!");
        //         break;
        //     }
        //     get_nameinfo_for_resource(fp, castleResources.icon, groupIconDirEntry.id - 1, &iconNameInfo);
        //     write_ico(fp, iconFile, groupIconDirEntry, castleResources.icon, iconNameInfo);
        //     fclose(iconFile);


        // }
    }

    iconFile = fopen("player_init.ico", "wb");
    //get dirEntry
    get_nameinfo_for_resource(fp, castleResources.groupIcon, GROUP_ICON_ID_PLAYER_FEM, &groupIconNameInfo);
    access_group_icon(fp, groupIconNameInfo.rnOffset << 4, &groupIconDir);

    //get entry for icon we want
    access_group_icon_entry(fp, groupIconDir.entryAddress, ICON_ENTRY_PLAYER_FEM_COLOR, &groupIconDirEntry);

    //get icon data
    get_nameinfo_for_resource(fp, castleResources.icon, groupIconDirEntry.id - 1, &iconNameInfo);

    //write 
    write_ico(fp, iconFile, groupIconDirEntry, castleResources.icon, iconNameInfo);
    fclose(iconFile);

    printf("Player\n");
    print_group_icon_dir_entry(groupIconDirEntry);
    

    FILE* newExe;
    newExe = fopen("patch.EXE", "rb+");
    if (!newExe){
        printf("Unable to open newExe!");
        return 0;
    }
    
    iconFile = fopen("bun2.ico", "rb");
    // iconFile = fopen("icons/group_216_entry_1.ico", "rb");
    if (!iconFile){
        printf("Unable to open iconFIle!");
        return 0;
    }
    
    replace_ico(newExe, iconFile, groupIconDirEntry, castleResources.icon, iconNameInfo);
    fclose(newExe);

    // get_nameinfo_for_resource(fp, castleResources.icon, 0, &nameInfo);
    // printf("Icon 0\n");
    // print_nameinfo(nameInfo);
    // printf("\n");




    // get_nameinfo_for_resource(fp, castleResources.bitmap, 0, &nameInfo);
    // printf("Bitmap 0\n");
    // print_nameinfo(nameInfo);
    // printf("\n");

    // get_nameinfo_for_resource(fp, castleResources.menu, 0, &nameInfo);
    // printf("Menu 0\n");
    // print_nameinfo(nameInfo);
    // printf("\n");

    // get_nameinfo_for_resource(fp, castleResources.dialog, 0, &nameInfo);
    // printf("Dialog 0\n");    
    // print_nameinfo(nameInfo);
    // printf("\n");



    

    /*
    uint8_t nameLength;
    fseek(fp, rcsResourceNamesAddress, SEEK_SET);
    read_byte(fp, &nameLength);
    
    while(nameLength > 0){
        for(uint8_t idx = 0; idx < nameLength; idx++){
            read_byte(fp, &scratch8);
            printf("%c", scratch8);
        }
        printf("\n");
        read_byte(fp, &nameLength);
    }
    // rscEndNames at 0x2fa0 + 0x4 (last item of resource table)
    // Implies some other body of text at 0x2FA6?
    */
    





    fclose(fp);




}
