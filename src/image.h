#pragma once

#include <stdint.h>

typedef struct {
    int width;
    int height;
    uint32_t *data; // RGBA order. (R at first memory index)
} Image;

Image* image_load(char *filename);
void image_free(Image* image);
