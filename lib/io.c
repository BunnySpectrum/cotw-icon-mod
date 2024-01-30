#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "win.h"


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
