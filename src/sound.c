#include "sound.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

Sound* sound_load(const char *filename) {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 frames;
    float* data = drwav_open_file_and_read_pcm_frames_f32(filename, &channels, &sampleRate, &frames, NULL);
    if (data == NULL) {
        printf("Error: Could not load %s\n", filename);
        return NULL;
    }

    Sound* sound = calloc(sizeof(Sound), 1);
    sound->channels = channels;
    sound->sampleRate = sampleRate;
    sound->frames = frames;
    sound->len = frames * channels;
    sound->data = data;
    return sound;
}

void sound_free(Sound* sound) {
    if (!sound) { return; }
    if (sound->data) {
        drwav_free(sound->data, NULL);
        sound->data = NULL;
    }
    free(sound);
}
