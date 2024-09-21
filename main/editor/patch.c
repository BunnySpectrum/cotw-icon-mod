
#ifdef WIN31
	#include "bunint.h"
#else
	#include <stdint.h>
	#include <inttypes.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "patch.h"
#include "win.h"
#include "castle.h"
#include "io.h"


#define MAIN_OK 1
#define MAIN_ERR 0

void test_run(int* inout){
    *inout += 1;
}

//iconIndex is relative to the dirEntry
void write_ico(fd exeFile, fd fileHandle, groupIconDirEntry_t dirEntry, castleResourceType_t iconResource, nameInfo_t iconNameInfo){
    uint8_t data;
    uint32_t addr;

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

    putc((uint8_t)(dirEntry.bytesInRes & 0xFF), fileHandle);
    putc((uint8_t)((dirEntry.bytesInRes >> 8)&0xFF), fileHandle);
    putc((uint8_t)((dirEntry.bytesInRes >> 16)&0xFF), fileHandle);
    putc((uint8_t)((dirEntry.bytesInRes >> 24)&0xFF), fileHandle);

    // write offset
    putc(0x16, fileHandle);
    putc(0x00, fileHandle);
    putc(0x00, fileHandle);
    putc(0x00, fileHandle);        

    //need to write rnLength bytes from offset<<4
    //printf("DBG: Writing %"PRIu16" bytes from %"PRIu16"\n", dirEntry.bytesInRes, iconNameInfo.rnOffset << 4);
    file_seek(exeFile, iconNameInfo.rnOffset << 4, SEEK_SET);
    for(addr=0; addr < dirEntry.bytesInRes; addr++){
        read_byte(exeFile, &data);
        putc(data, fileHandle);
    }
}


void replace_ico(fd exeFile, fd iconFile, groupIconDirEntry_t dirEntry, castleResourceType_t iconResource, nameInfo_t iconNameInfo){
    uint8_t data;
    uint32_t addr;

    //cheating - I know the offset is 0x16
    file_seek(iconFile, 0x16, SEEK_SET);


    //need to replace rnLength bytes from offset<<4
    // printf("Seek to %#"PRIx32", write %#"PRIx32" bytes\n", (uint32_t)iconNameInfo.rnOffset << 4, dirEntry.bytesInRes); //cast then shift for win3.1
    file_seek(exeFile, (uint32_t)iconNameInfo.rnOffset << 4, SEEK_SET); //cast then shift for win3.1

    for(addr=0; addr < dirEntry.bytesInRes; addr++){
        read_byte(iconFile, &data);
        file_write(&data, 1, 1, exeFile);
    }
    // printf("\nWrote %#x bytes\n", addr);

}


int patch(char* exePath, char* iconPath){
    // char* exePath;
    // char* iconPath;
    fd fp;
    uint32_t currentTypeInfoAddress;
    typeInfoList_t typeInfoList;
    castleResources_t castleResources;
    groupIconDir_t groupIconDir;
    groupIconDirEntry_t groupIconDirEntry;
    nameInfo_t groupIconNameInfo;
    nameInfo_t iconNameInfo;
    fd iconFile;
    dosHeader_t dosHeader;
    windowsHeader_t winHeader;
    resourceTable_t resourceTable;


    fp = file_open(exePath, OF_READ | OF_WRITE);
    // printf("EXE: %s\n", exePath);
    if (!fp){
	#ifdef WIN31
	    // printf("Unable to open EXE.");
	#else
        	perror("Unable to open EXE.");
	#endif
        return 0;
    }


    dos_read_magic(fp, &dosHeader);
    dos_read_table_offset(fp, &dosHeader);
    dos_read_windows_offset(fp, &dosHeader);


    // printf("***DOS header***\n");
    // printf("Signature: %c%c\n", dosHeader.signature[0], dosHeader.signature[1]);
    // printf("Table offset: %#x\n", dosHeader.tableOffset);
    // printf("Windows offset: %#x\n", dosHeader.windowsOffset);


    // Start reading windows header
    //  Address offsets are relative to dosHeader.windowsOffset (0x400) unless otherwise specified

    //Check for magic number at 0x00
    // byte 0 should be 0x4E (N), byte 1 should be 0x45 (E)
    win_read_magic(fp, &winHeader, dosHeader.windowsOffset);
    win_read_resource_table_offset(fp, &winHeader, dosHeader.windowsOffset);

    // printf("\n***Windows header***\n");
    // printf("Signature: %c%c\n", winHeader.signature[0], winHeader.signature[1]);
    // printf("Resource table offset: %#0*x rel, %#x abs\n", 6, winHeader.resourceTableOffset, winHeader.resourceTableOffset + dosHeader.windowsOffset);


    // Start reading resource table
    resourceTable.baseAddress = winHeader.resourceTableOffset + dosHeader.windowsOffset;
    rcs_table_read_shift(fp, &resourceTable);

    // printf("\n***Resource Table***\n");
    // printf("Alignment shift count: %d\n", resourceTable.rcsAlignShift);

    currentTypeInfoAddress = resourceTable.baseAddress + 0x2;
    
    do{
        typeInfoList.address = currentTypeInfoAddress;
        rt_read_type_id(fp, &typeInfoList);

        // When we see a type of 0, it means we've left the array. So we're done
        if (typeInfoList.typeInfo.rtTypeID == 0x0){
            // printf("rcsEndTypes at %#"PRIx32"\n", currentTypeInfoAddress);
            break;
        }

        rt_read_resource_count(fp, &typeInfoList);
        // printf("\tTypeinfo @ %#"PRIx32": type %#"PRIx16", count %"PRIu16"\n", typeInfoList.address, typeInfoList.typeInfo.rtTypeID, typeInfoList.typeInfo.rtResourceCount); //needs lx for address for win3.1


        switch(typeInfoList.typeInfo.rtTypeID & 0xFFF){
            case bRT_GROUP_CURSOR:
                castleResources.groupCursor.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.groupCursor.nameInfoAddress = typeInfoList.address + 0x8;
                break;
            
            case bRT_CURSOR:
                castleResources.cursor.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.cursor.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case bRT_GROUP_ICON:
                castleResources.groupIcon.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.groupIcon.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case bRT_ICON:
                castleResources.icon.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.icon.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case bRT_BITMAP:
                castleResources.bitmap.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.bitmap.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case bRT_MENU:
                castleResources.menu.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.menu.nameInfoAddress = typeInfoList.address + 0x8;
                break;

            case bRT_DIALOG:
                castleResources.dialog.resourceCount = typeInfoList.typeInfo.rtResourceCount;
                castleResources.dialog.nameInfoAddress = typeInfoList.address + 0x8;
                break;

        }


        currentTypeInfoAddress += 0x8 + typeInfoList.typeInfo.rtResourceCount*6*2;
    } while(typeInfoList.typeInfo.rtTypeID != 0x0);
    // first word of a TYPEINFO is the typeID. When we read a typeID of 0x0 we know we are done


    //get dirEntry
    // printf("\nNameinfo for groupIcon\n");
    get_nameinfo_for_resource(fp, castleResources.groupIcon, GROUP_ICON_ID_PLAYER_FEM, &groupIconNameInfo);
    print_nameinfo(groupIconNameInfo);

    // printf("\nGroupIcon Dir\n");
    access_group_icon(fp, (uint32_t)(groupIconNameInfo.rnOffset) << 4, &groupIconDir); // have to cast first for win3.1 sizes
    print_group_icon_dir(groupIconDir);

    //get entry for icon we want
    // printf("\ngroupIcon dir entry\n");
    access_group_icon_entry(fp, groupIconDir.entryAddress, ICON_ENTRY_PLAYER_FEM_COLOR, &groupIconDirEntry);
    print_group_icon_dir_entry(groupIconDirEntry);

    //get icon data
    // printf("\nIcon name info\n");
    get_nameinfo_for_resource(fp, castleResources.icon, groupIconDirEntry.id - 1, &iconNameInfo);
    print_nameinfo(iconNameInfo);

    
    

    iconFile = file_open(iconPath, OF_READ);
    // printf("Icon: %s\n", iconPath);

    if (!iconFile){
	#ifdef WIN31
	    // printf("Unable to open icon.");
	#else
        	perror("Unable to open icon.");
	#endif
        return 0;
    }
    
    replace_ico(fp, iconFile, groupIconDirEntry, castleResources.icon, iconNameInfo);
    file_close(fp);
}
