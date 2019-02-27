#ifndef __EVENT_QUEUE_H__
#define __EVENT_QUEUE_H__

#include "queue.h"

void QueueInit(void);
//int QueuePut(int type, void *data, size_t	len);
int QueuePut(int type, void *data);

void QueueDeinit();
QUEUE_ITEM * QueueGet();
int QueueIsEmpty(void);

void freeEvent(SocketEvent **event);

#endif


