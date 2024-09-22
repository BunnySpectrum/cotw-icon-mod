#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "win.h"


//TODO replace return code w/ enum
uint8_t read_byte(bun_file_s file, uint8_t* result){
    return WIN_CHAR_LEN == file_read(file, result, sizeof(uint8_t), WIN_CHAR_LEN);
}

uint8_t read_word(bun_file_s file, uint16_t* result){
    uint8_t buf[WIN_WORD_LEN];
    uint8_t numRead;
    
    numRead = file_read(file, buf, sizeof(uint8_t), WIN_WORD_LEN);
    if (numRead < WIN_WORD_LEN){
        return 0;
    }
    *result = buf[0] | (buf[1] << 8);

    return 1;
}

uint8_t read_dword(bun_file_s file, uint32_t* result){
    uint8_t buf[WIN_DWORD_LEN];
    uint8_t numRead;
    
    numRead = file_read(file, buf, sizeof(uint8_t), WIN_DWORD_LEN);
    if (numRead < WIN_DWORD_LEN){
        return 0;
    }
    *result = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

    return 1;
}
