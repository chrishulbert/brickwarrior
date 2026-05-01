// This is responsible for mixing the sounds into an audio stream.
// This uses a lockless message passing command queue to pass instructions to the
// mixer thread.
// This assumes mono.

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "mixer.h"
#include "sound.h"

// Command queue:
// https://en.wikipedia.org/wiki/Circular_buffer
// https://softwareengineering.stackexchange.com/questions/144477/on-a-queue-which-end-is-the-head
// https://www.kernel.org/doc/Documentation/circular-buffers.txt
// Since nobody can agree on whether you add to the head vs the tail, i'm just calling the indices 'write' and 'read'.
// The indices are separated to hopefully keep them on separate cache lines to avoid changes to one flushing the other's cache.

typedef enum {
    COMMAND_PLAY,
    COMMAND_STOP_ONE,
    COMMAND_STOP_ALL,
} CommandType;

typedef struct {
    CommandType type;
    int id;
    Sound *sound;
} Command;

#define QUEUE_SIZE 16 // Must be a power of 2 for fast masking.

static struct {
    atomic_size_t writeIndex; // Producer's index (game thread).
    size_t writeIndexMirror; // Since the producer is the only thing that touches writeIndex, it can fetch it faster from here.
    Command buffer[QUEUE_SIZE];
    atomic_size_t readIndex; // Consumer's index (mixer thread).
    size_t readIndexMirror;
} commandQueue;

// https://en.cppreference.com/cpp/atomic/memory_order
// memory_order_relaxed means this value is atomic but the CPU can reorder other variables changed it.
// memory_order_acquire is for loads:
// * no reads or writes in the current thread can be reordered before this load.
// * all writes in other threads that release the same atomic variable are visible in the current thread.
// memory_order_release is for stores
// * no reads or writes in the current thread can be reordered after this store.
// * all writes in the current thread are visible in other threads that acquire the same atomic variable.

// Producer (controls the writeIndex).
bool queue_write(Command cmd) {
    size_t writeIndex = commandQueue.writeIndexMirror;
    size_t readIndex = atomic_load_explicit(&commandQueue.readIndex, memory_order_acquire);

    // Check if queue is full.
    if ((writeIndex + 1) % QUEUE_SIZE == readIndex) {
        return false; 
    }

    commandQueue.buffer[writeIndex] = cmd;
    
    // Use 'release' to ensure the buffer write is visible before the head update.
    atomic_store_explicit(&commandQueue.writeIndex, (writeIndex + 1) % QUEUE_SIZE, memory_order_release);
    return true;
}

// This is the playback state - only to be touched by the callback thread:
#define MAX_VOICES 8
typedef struct {
    Sound *sound;
    int sampleRate; // Played rate, which will be a little different.
    int frame; // Where it's up to.
    int id;
} Voice; // A currently playing sound.
static struct {
    Voice voice[MAX_VOICES];
    int voices;
} state;

// Consumer (controls the readIndex).
void queue_read() {
    size_t readIndex = commandQueue.readIndexMirror;
    size_t writeIndex = atomic_load_explicit(&commandQueue.writeIndex, memory_order_acquire);

    while (readIndex != writeIndex) {
        Command* cmd = &commandQueue.buffer[readIndex];
        readIndex = (readIndex + 1) % QUEUE_SIZE;
        
        // Apply the command:
        switch (cmd->type) {
            case COMMAND_PLAY:
                if (state.voices < MAX_VOICES) {
                    state.voice[state.voices].sound = cmd->sound;
                    state.voice[state.voices].sampleRate = cmd->sound->sampleRate - 500 + (rand()%1000);
                    state.voice[state.voices].frame = 0;
                    state.voice[state.voices].id = cmd->id;
                    state.voices++;
                }

            case COMMAND_STOP_ONE:
                for (int i=0; i<state.voices; i++) {
                    if (state.voice[i].id == cmd->id) {
                        if (i == state.voices-1) { // This is the last one.
                            state.voices--;
                        } else { // This is mid-list, so swap it with the last.
                            memcpy(&state.voice[i], &state.voice[state.voices-1], sizeof(Voice));
                            state.voices--;
                            i--; // Iterate this one again.
                        }
                    }
                }

            case COMMAND_STOP_ALL:
                state.voices = 0;
                break;
        }
    }

    // Update tail so the producer knows space has been cleared.
    atomic_store_explicit(&commandQueue.readIndex, readIndex, memory_order_release);
}

// TODO get sound to pre-interpolate to 22050? Or do pitch time maths here?

// Called from the audio thread, needs to be hard realtime.
void mixer_stream_callback(float* buffer, int num_frames, int num_channels) {
    for (int i=0; i<num_frames; i++) {
        float sample = 0;
        for (int j=0; j<state.voices; j++) {
            auto voice = &state.voice[j];
            sample += voice->sound->data[voice->frame];
            voice->frame++;
            if (voice->frame >= voice->sound->len) { // Finished.
                if (j==state.voices-1) { // End of the list.
                    state.voices--;
                } else { // Mid-list:
                    memcpy(&state.voice[j], &state.voice[state.voices-1], sizeof(Voice));
                    state.voices--;
                    j--; // Iterate this index again.
                }
            }
        }
        buffer[i] = sample;
    }
}

// Externally-called helpers:
void mixer_play(Sound* sound, int id) {
    queue_write((Command){
        .type = COMMAND_PLAY,
        .id = id,
        .sound = sound,
    });
}
