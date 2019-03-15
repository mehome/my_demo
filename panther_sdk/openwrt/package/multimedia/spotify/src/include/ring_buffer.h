#ifndef RING_BUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// 1 second ring buffer
#define RING_BUFFER_SIZE (44100 * 2)

int ring_buffer_init(void);
int ring_buffer_destory(void);
int ring_buffer_queue(const int16_t *samples, size_t count);
int ring_buffer_dequeue(int16_t *samples, size_t count);
void ring_buffer_flush(void);
int32_t ring_buffer_size(void);
int32_t ring_buffer_remaining(void);
void ring_buffer_lock(void);
void ring_buffer_unlock(void);

#define RING_BUFFER_H
#endif
