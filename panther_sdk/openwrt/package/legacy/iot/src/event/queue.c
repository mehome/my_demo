#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"

QUEUE *Initialize_Queue(void)
{
	QUEUE *q;
	
	q = calloc(1, sizeof(QUEUE));
	if (!(q))
		return NULL;
	
	pthread_mutex_init(&(q->modify_mutex), NULL);
	pthread_mutex_init(&(q->read_mutex), NULL);
	pthread_mutex_lock(&(q->read_mutex));
	
	return q;
}

//void Add_Queue_Item(QUEUE *queue, int type, void *data, size_t sz)
void Add_Queue_Item(QUEUE *queue, int type, void *data)
{
	QUEUE_ITEM *qi;
	
	qi = calloc(1, sizeof(QUEUE_ITEM));
	if (!(qi))
		return;
	
	qi->data = data;
	//qi->sz = sz;
	qi->type = type;
	
	pthread_mutex_lock(&(queue->modify_mutex));
	
	qi->next = queue->items;
	queue->items = qi;
	queue->numitems++;
	
	pthread_mutex_unlock(&(queue->modify_mutex));
	pthread_mutex_unlock(&(queue->read_mutex));
}
QUEUE_ITEM *Get_Queue_Item(QUEUE *queue)
{
	QUEUE_ITEM *qi, *pqi;
	
	pthread_mutex_lock(&(queue->read_mutex));
	pthread_mutex_lock(&(queue->modify_mutex));
	
  if(!(qi = pqi = queue->items)) {
      pthread_mutex_unlock(&(queue->modify_mutex));
      return (QUEUE_ITEM *)NULL;
  } 

	qi = pqi = queue->items;
	while ((qi->next))
	{
		pqi = qi;
		qi = qi->next;
	}
	
	pqi->next = NULL;
	if (queue->numitems == 1)
		queue->items = NULL;
	
	queue->numitems--;
	
	if (queue->numitems > 0)
		pthread_mutex_unlock(&(queue->read_mutex));
	
	pthread_mutex_unlock(&(queue->modify_mutex));
	
	return qi;
}

void Free_Queue_Item(QUEUE_ITEM *queue_item)
{
	if(queue_item)
		free(queue_item);
}
void Free_Queue(QUEUE*queue)
{
	if(queue)
		free(queue);
}

int Get_Queue_Sizes(QUEUE *queue)
{
	int size;
	pthread_mutex_lock(&(queue->modify_mutex));
	//pthread_mutex_lock(&(queue->read_mutex));
	size = queue->numitems;
	pthread_mutex_unlock(&(queue->modify_mutex));
	//pthread_mutex_unlock(&(queue->read_mutex));
	return size;
}

#if defined(TEST_CODE)
void Print_Queue_Items(QUEUE *queue)
{
    QUEUE_ITEM *item;

    while (queue->items) {
        item = Get_Queue_Item(queue);
        fprintf(stderr, "[*] type[%d]: %s (%u)\n", item->type, item->data, item->sz);
        Free_Queue_Item(item);
    }
}
#endif
