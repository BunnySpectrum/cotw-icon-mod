#ifndef _IO_H_
#define _IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>


uint8_t read_byte(FILE* fileHandle, uint8_t* result);
uint8_t read_word(FILE* fileHandle, uint16_t* result);
uint8_t read_dword(FILE* fileHandle, uint32_t* result);










#ifdef __cplusplus
}
#endif

#endif //include guard