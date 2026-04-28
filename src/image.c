#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

Image* image_load(const char *filename) {
    // Using '4' as the last argument forces the output to RGBA.
    int width, height, channels;
    void *data = stbi_load(filename, &width, &height, &channels, 4);
    if (data == NULL) {
        printf("Error: Could not load %s\n", filename);
        return NULL;
    }

    Image* img = calloc(sizeof(Image), 1);
    img->width = width;
    img->height = height;
    img->data = data;
    return img;
}

void image_free(Image* image) {
    if (!image) { return; }
    if (image->data) {
        stbi_image_free(image->data);
    }
    free(image);
}