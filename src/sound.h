#pragma once

#include <stdint.h>

typedef struct {
    int channels;
    int sampleRate;
    int frames; // For stereo, 1 frame means 2 floats: left + right.
    int len; // The number of floats in data, eg frames * channels.
    float *data;
} Sound;

Sound* sound_load(const char *filename);
void sound_free(Sound* image);
