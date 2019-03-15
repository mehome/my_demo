#include "ring_buffer.h"

static pthread_mutex_t audio_mutex;
//static int16_t audio_ring_buffer[RING_BUFFER_SIZE];
static int16_t * audio_ring_buffer = NULL;
static uint32_t ring_head, ring_tail;
static int32_t total_ring_size;
static int32_t ring_size = 0;

void ring_buffer_lock(void) {
    pthread_mutex_lock(&audio_mutex);
}

void ring_buffer_unlock(void) {
    pthread_mutex_unlock(&audio_mutex);
}

int ring_buffer_queue(const int16_t *samples, size_t count) {
    uint32_t new_head;
    uint8_t looped = 0;

    pthread_mutex_lock(&audio_mutex);

    if (ring_size + count >= total_ring_size) {
        pthread_mutex_unlock(&audio_mutex);
        return -1; // Not enough space
    }

    new_head = ring_head + count;
    if (new_head >= total_ring_size) {
        new_head -= total_ring_size;
        looped = 1;
    }

    if (!looped) {
        memcpy(&audio_ring_buffer[ring_head], samples, count * 2);
    } else {
        memcpy(audio_ring_buffer + ring_head, samples, (total_ring_size - ring_head) * 2);
        memcpy(audio_ring_buffer, samples + total_ring_size - ring_head, new_head * 2);
    }
    ring_head = new_head;
    ring_size += count;
    pthread_mutex_unlock(&audio_mutex);
    return 0;
}

int ring_buffer_dequeue(int16_t *samples, size_t count) {
    uint32_t new_tail;
    uint8_t looped = 0;

    pthread_mutex_lock(&audio_mutex);

    if (ring_size - (int)count < 0) {
        // maybe it will has data never play
        // FIXIT?
        pthread_mutex_unlock(&audio_mutex);
        return -1; // Not enough data
    }

    new_tail = ring_tail + count;
    if (new_tail >= total_ring_size) {
        new_tail -= total_ring_size;
        looped = 1;
    }

    if (!looped) {
        memcpy(samples, &audio_ring_buffer[ring_tail], count * 2);
    } else {
        memcpy(samples, audio_ring_buffer + ring_tail, (total_ring_size - ring_tail) * 2);
        memcpy(samples + total_ring_size - ring_tail, audio_ring_buffer, new_tail * 2);
    }
    ring_tail = new_tail;
    ring_size -= count;
    pthread_mutex_unlock(&audio_mutex);
    return 0;
}

int32_t ring_buffer_size(void) {
    return ring_size;
}

int32_t ring_buffer_remaining(void) {
    return (total_ring_size - ring_size);
}

void ring_buffer_flush(void) {
    pthread_mutex_lock(&audio_mutex);
    ring_head = 0;
    ring_tail = 0;
    ring_size = 0;
    pthread_mutex_unlock(&audio_mutex);
}

int ring_buffer_init(void) {
    if (pthread_mutex_init(&audio_mutex, NULL) < 0)
        return -1;

    if (audio_ring_buffer != NULL)
        return -1;

    char val[32];
    int ringbuffer_sec = 5;
    int ret = cdb_get("$spotify_ringbuf_sec", &val);
    if (ret >= 0)
        ringbuffer_sec = atoi(val);

    total_ring_size = RING_BUFFER_SIZE * ringbuffer_sec;
    audio_ring_buffer = (int16_t*)malloc(total_ring_size * sizeof(int16_t));
    
    ring_buffer_flush();

    return 0;
}

int ring_buffer_destory(void) {
    free(audio_ring_buffer);
    audio_ring_buffer = NULL;
}

