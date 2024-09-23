#ifndef _IO_H_
#define _IO_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>

#include "bunint.h"
#include "bun_io.h"

uint8_t read_byte(bun_file_s* pfile, uint8_t* result);
uint8_t read_word(bun_file_s* pfile, uint16_t* result);
uint8_t read_dword(bun_file_s* pfile, uint32_t* result);


#ifdef __cplusplus
}
#endif

#endif //include guard
