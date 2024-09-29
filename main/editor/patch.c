

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "patch.h"
#include "win.h"
#include "castle.h"
#include "io.h"

#include "bun_io.h"
// File handling largely based on popfile.c from the Petzold book, page 477

static OPENFILENAME ofnExe;
static OPENFILENAME ofnIco;

void PatchInit(HWND hwnd){
    static char* szFilterExe[] = {"EXE (*.EXE)", "*.exe", ""}; 
    static char *szFilterIco[] = {"ICON (*.ICO)", "*.ico", ""};

    ofnExe.lStructSize = sizeof(OPENFILENAME);
    ofnExe.hwndOwner = hwnd;
    ofnExe.hInstance = NULL;
    ofnExe.lpstrFilter = szFilterExe[0];
    ofnExe.lpstrCustomFilter = NULL;
    ofnExe.nMaxCustFilter = 0;
    ofnExe.nFilterIndex = 0;
    ofnExe.lpstrFile = NULL;
    ofnExe.nMaxFile = _MAX_PATH;
    ofnExe.lpstrFileTitle = NULL;
    ofnExe.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
    ofnExe.lpstrInitialDir = NULL;
    ofnExe.lpstrTitle = NULL;
    ofnExe.Flags = 0;
    ofnExe.nFileOffset = 0;
    ofnExe.nFileExtension = 0;
    ofnExe.lpstrDefExt = "exe";
    ofnExe.lCustData = 0L;
    ofnExe.lpfnHook = NULL;
    ofnExe.lpTemplateName = NULL;

    ofnIco.lStructSize = sizeof(OPENFILENAME);
    ofnIco.hwndOwner = hwnd;
    ofnIco.hInstance = NULL;
    ofnIco.lpstrFilter = szFilterIco[0];
    ofnIco.lpstrCustomFilter = NULL;
    ofnIco.nMaxCustFilter = 0;
    ofnIco.nFilterIndex = 0;
    ofnIco.lpstrFile = NULL;
    ofnIco.nMaxFile = _MAX_PATH;
    ofnIco.lpstrFileTitle = NULL;
    ofnIco.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
    ofnIco.lpstrInitialDir = NULL;
    ofnIco.lpstrTitle = NULL;
    ofnIco.Flags = 0;
    ofnIco.nFileOffset = 0;
    ofnIco.nFileExtension = 0;
    ofnIco.lpstrDefExt = "ico";
    ofnIco.lCustData = 0L;
    ofnIco.lpfnHook = NULL;
    ofnIco.lpTemplateName = NULL;
}



BOOL PatchFileOpenDlg(HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName){



}


#define MAIN_OK 1
#define MAIN_ERR 0

void test_run(int* inout){
    *inout += 1;
}

//iconIndex is relative to the dirEntry
void write_ico(bun_file_s *pExeFile, bun_file_s *pIconFile, groupIconDirEntry_t dirEntry, castleResourceType_t iconResource, nameInfo_t iconNameInfo){
    uint8_t data;
    uint32_t addr;

    // little endian file
    //icon dir
    // write [00, 00] rsvd
    put_char(0x00, pIconFile);
    put_char(0x00, pIconFile);

    // write [01, 00] 1 = icon, 2 = cursor
    put_char(0x01, pIconFile);
    put_char(0x00, pIconFile);

    // write [01, 00] number of icons in file, will always be 1 for this function
    put_char(0x01, pIconFile);
    put_char(0x00, pIconFile);

    // write contents of icondirentry
    // except offset 12 is offset to BMP data from start of file
    put_char(dirEntry.width, pIconFile);
    put_char(dirEntry.height, pIconFile);
    put_char(dirEntry.colorCount, pIconFile);
    put_char(dirEntry.rsvd, pIconFile);

    put_char((uint8_t)(dirEntry.planes & 0xFF), pIconFile);
    put_char((uint8_t)((dirEntry.planes >> 8)&0xFF), pIconFile);

    put_char((uint8_t)(dirEntry.bitCount & 0xFF), pIconFile);
    put_char((uint8_t)((dirEntry.bitCount >> 8)&0xFF), pIconFile);

    put_char((uint8_t)(dirEntry.bytesInRes & 0xFF), pIconFile);
    put_char((uint8_t)((dirEntry.bytesInRes >> 8)&0xFF), pIconFile);
    put_char((uint8_t)((dirEntry.bytesInRes >> 16)&0xFF), pIconFile);
    put_char((uint8_t)((dirEntry.bytesInRes >> 24)&0xFF), pIconFile);

    // write offset
    put_char(0x16, pIconFile);
    put_char(0x00, pIconFile);
    put_char(0x00, pIconFile);
    put_char(0x00, pIconFile);        

    //need to write rnLength bytes from offset<<4
    //printf("DBG: Writing %"PRIu16" bytes from %"PRIu16"\n", dirEntry.bytesInRes, iconNameInfo.rnOffset << 4);
    file_seek(pExeFile, iconNameInfo.rnOffset << 4, SEEK_SET);
    for(addr=0; addr < dirEntry.bytesInRes; addr++){
        read_byte(pExeFile, &data);
        put_char(data, pIconFile);
    }
}


void replace_ico(bun_file_s *pExeFile, bun_file_s *pIconFile, groupIconDirEntry_t dirEntry, castleResourceType_t iconResource, nameInfo_t iconNameInfo){
    uint8_t data;
    uint32_t addr;

    //cheating - I know the offset is 0x16
    file_seek(pIconFile, 0x16, SEEK_SET);


    //need to replace rnLength bytes from offset<<4
    // printf("Seek to %#"PRIx32", write %#"PRIx32" bytes\n", (uint32_t)iconNameInfo.rnOffset << 4, dirEntry.bytesInRes); //cast then shift for win3.1
    file_seek(pExeFile, (uint32_t)iconNameInfo.rnOffset << 4, SEEK_SET); //cast then shift for win3.1

    for(addr=0; addr < dirEntry.bytesInRes; addr++){
        read_byte(pIconFile, &data);
        file_write(pExeFile, &data, 1, 1);
    }
    // printf("\nWrote %#x bytes\n", addr);

}


// int patch(OPENFILENAME* pofnExe, OPENFILENAME* pofnIcon, uint16_t* result){
int patch(int hfileExe, int hfileIcon, uint16_t* result){
    int exe_handle, icon_handle;
    uint32_t currentTypeInfoAddress;
    typeInfoList_t typeInfoList;
    castleResources_t castleResources;
    groupIconDir_t groupIconDir;
    groupIconDirEntry_t groupIconDirEntry;
    nameInfo_t groupIconNameInfo;
    nameInfo_t iconNameInfo;
    dosHeader_t dosHeader;
    windowsHeader_t winHeader;
    resourceTable_t resourceTable;
    bun_file_s exe_file, icon_file;

    // exe_file.name = pofnExe->lpstrFile;
    exe_file.handle = hfileExe;
    exe_file.mode_flags = OF_READ | OF_WRITE;
    
    // icon_file.name = pofnIcon->lpstrFile;
    icon_file.handle = hfileIcon;
    icon_file.mode_flags = OF_READ;


    // exe_handle = _lopen(pofnExe->lpstrFile, OF_READ);
    // exe_handle = file_open(&exe_file);
    // printf("EXE: %s\n", exePath);
    // if (-1 == exe_handle){
        // return -1;
    // }else{
        // return 2; XXX got here
    // }
    // exe_file.handle = exe_handle;

    dosHeader.signature[0] = 1;
    dosHeader.signature[1] = 1;
    *result = 0x20;
    _llseek(hfileExe, 0, SEEK_SET);
    if(2 != _lread(hfileExe, (LPSTR)(result), 2)){
        return -1;
    }

    return hfileExe;

    dos_read_magic(&exe_file, &dosHeader);
    *result = ((uint16_t)dosHeader.signature[0] << 8) || (dosHeader.signature[1]);
    return 99;

    dos_read_table_offset(&exe_file, &dosHeader);
    dos_read_windows_offset(&exe_file, &dosHeader);

    // return 3; XXX got here


    // Start reading windows header
    //  Address offsets are relative to dosHeader.windowsOffset (0x400) unless otherwise specified

    //Check for magic number at 0x00
    // byte 0 should be 0x4E (N), byte 1 should be 0x45 (E)
    win_read_magic(&exe_file, &winHeader, dosHeader.windowsOffset);
    win_read_resource_table_offset(&exe_file, &winHeader, dosHeader.windowsOffset);

    // return 4; XXX got here


    // Start reading resource table
    resourceTable.baseAddress = winHeader.resourceTableOffset + dosHeader.windowsOffset;
    rcs_table_read_shift(&exe_file, &resourceTable);

    // return 5; XXX got here

    currentTypeInfoAddress = resourceTable.baseAddress + 0x2;
    
    do{
        typeInfoList.address = currentTypeInfoAddress;
        rt_read_type_id(&exe_file, &typeInfoList);

        // When we see a type of 0, it means we've left the array. So we're done
        if (typeInfoList.typeInfo.rtTypeID == 0x0){
            // printf("rcsEndTypes at %#"PRIx32"\n", currentTypeInfoAddress);
            break;
        }

        rt_read_resource_count(&exe_file, &typeInfoList);
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
    get_nameinfo_for_resource(&exe_file, castleResources.groupIcon, GROUP_ICON_ID_PLAYER_FEM, &groupIconNameInfo);
    print_nameinfo(groupIconNameInfo);

    // printf("\nGroupIcon Dir\n");
    access_group_icon(&exe_file, (uint32_t)(groupIconNameInfo.rnOffset) << 4, &groupIconDir); // have to cast first for win3.1 sizes
    print_group_icon_dir(groupIconDir);

    //get entry for icon we want
    // printf("\ngroupIcon dir entry\n");
    access_group_icon_entry(&exe_file, groupIconDir.entryAddress, ICON_ENTRY_PLAYER_FEM_COLOR, &groupIconDirEntry);
    print_group_icon_dir_entry(groupIconDirEntry);

    //get icon data
    // printf("\nIcon name info\n");
    get_nameinfo_for_resource(&exe_file, castleResources.icon, groupIconDirEntry.id - 1, &iconNameInfo);
    print_nameinfo(iconNameInfo);

    
    

    icon_handle = file_open(&icon_file);
    // printf("Icon: %s\n", iconPath);

    if (!icon_handle){
        return -1;
    }
    icon_file.handle = icon_handle;
    
    replace_ico(&exe_file, &icon_file, groupIconDirEntry, castleResources.icon, iconNameInfo);
    file_close(&exe_file);
    return 100;
}
