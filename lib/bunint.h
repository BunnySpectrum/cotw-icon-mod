#ifndef _BUNINT_H_
#define _BUNINT_H_

#ifdef WIN31

#define uint8_t unsigned char
#define uint16_t unsigned int
#define uint32_t unsigned long

#define PRIu8 "hu"
#define PRIu16 "u"
#define PRIu32 "lu"

#define PRIx8 "x"
#define PRIx16 "x"
#define PRIx32 "lx"

#define PRIz PRIu32


#else

#include <stdint.h>
#include <inttypes.h>

#define PRIz "zu"

#endif

#endif //include guard


