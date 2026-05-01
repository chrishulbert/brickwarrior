// This is responsible for mixing the sounds into an audio stream.
// This uses a lockless message passing command queue to pass instructions to the
// mixer thread.

#include <stdatomic.h>

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

// Consumer (controls the readIndex).
void queue_read() {
    size_t readIndex = commandQueue.readIndexMirror;
    size_t writeIndex = atomic_load_explicit(&commandQueue.writeIndex, memory_order_acquire);

    while (readIndex != writeIndex) {
        Command* cmd = &commandQueue.buffer[readIndex];
        readIndex = (readIndex + 1) % QUEUE_SIZE;
        
        // Internal logic to update audio state
        // handle_command(cmd);
    }

    // Update tail so the producer knows space has been cleared.
    atomic_store_explicit(&commandQueue.readIndex, readIndex, memory_order_release);
}

// Called from the audio thread, needs to be hard realtime.
void mixer_stream_callback(float* buffer, int num_frames, int num_channels) {
    const int num_samples = num_frames * num_channels;
    for (int i=0; i<num_samples; i++) {
        buffer[i] = 0.0f;
    }
}
