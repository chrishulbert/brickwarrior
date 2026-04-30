// This handles endian conversions in a cross-platform way.
// This is needed because the original game was windows intel little endian files.

#pragma once

#include <stdint.h>
#include <stdio.h>

uint16_t be_to_native_16(uint16_t be);
uint32_t be_to_native_32(uint32_t be);

unsigned char fread_u8(FILE* fh);
int16_t fread_i16(FILE* fh);
int32_t fread_i32(FILE* fh);
