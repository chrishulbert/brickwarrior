#include "endian.h"

// 1. Try to use compiler-provided macros (Clang/GCC/MinGW)
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    #define PLATFORM_IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined(_WIN32)
    // 2. Fallback for MSVC (Windows is always Little Endian)
    #define PLATFORM_IS_LITTLE_ENDIAN 1
#elif defined(__APPLE__)
    // 3. Fallback for very old Apple headers if needed
    #include <machine/endian.h>
    #define PLATFORM_IS_LITTLE_ENDIAN (BYTE_ORDER == LITTLE_ENDIAN)
#else
    // 4. If all else fails, default to Little Endian but warn the dev
    #warning "Endianness not detected, assuming Little Endian"
    #define PLATFORM_IS_LITTLE_ENDIAN 1
#endif

typedef union {
    uint16_t value;
    uint8_t  bytes[2];
} Swap16;

uint16_t le_to_native_16(uint16_t le) {
    #if PLATFORM_IS_LITTLE_ENDIAN
        return le;
    #else
        Swap16 s;
        s.value = be;
        uint8_t temp = s.bytes[0];
        s.bytes[0] = s.bytes[1];
        s.bytes[1] = temp;
        return s.value;
    #endif
}

typedef union {
    uint32_t value;
    uint8_t  bytes[4];
} Swap32;

uint32_t le_to_native_32(uint32_t le) {
    #if PLATFORM_IS_LITTLE_ENDIAN
        return le;
    #else
        Swap32 s;
        s.value = be;
        // Swap outer bytes:
        uint8_t temp = s.bytes[0];
        s.bytes[0] = s.bytes[3];
        s.bytes[3] = temp;
        // Swap inner bytes:
        temp = s.bytes[1];
        s.bytes[1] = s.bytes[2];
        s.bytes[2] = temp;
        return s.value;
    #endif
}

unsigned char fread_u8(FILE* fh) {
	unsigned char i = 0;
	fread(&i, 1, 1, fh);
	return i;
}

int16_t fread_i16(FILE* fh) {
	uint16_t le = 0;
	fread(&le, 2, 1, fh);
    uint16_t native = le_to_native_16(le);
    return (int16_t)native;
}

int32_t fread_i32(FILE* fh) {
	uint32_t le = 0;
	fread(&le, 4, 1, fh);
    uint32_t native = le_to_native_32(le);
    return (int32_t)native;
}