#pragma once

#include "sound.h"

void mixer_stream_callback(float* buffer, int num_frames, int num_channels);
void mixer_play(Sound* sound, int id);
