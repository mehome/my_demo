

#include <stdio.h>
#include <pthread.h>

//#include "myutils/debug.h"
#include "event_queue.h"

static QUEUE *g_queue=NULL;

void QueueInit(void)
{
	if(g_queue == NULL) {
		g_queue = Initialize_Queue();
		if(!g_queue) {
			error("Initialize_Queue failed");
			exit(-1);
		}
	}
}

int QueuePut(int type, void *data)
{
	if(g_queue == NULL) {
		error("queue is not Init...");
		return -1;
	}
	Add_Queue_Item(g_queue,type, data);
	return 0;
}

QUEUE_ITEM * QueueGet()
{
	if(g_queue == NULL) {
		error("queue is not Init...");
		return NULL;
	}
	return Get_Queue_Item(g_queue);
}
// 0 not empty 1 empty
int QueueIsEmpty(void)
{
	int len = 0;
	if(g_queue == NULL)
		return 1;
	if(Get_Queue_Sizes(g_queue) > 0)
		return 0;
	else 
		return 1;
}
int QueueSizes()
{
	int ret = 0;
	if(g_queue)
		ret = Get_Queue_Sizes(g_queue);
	return ret;
}

void QueueDeinit()
{
	if(g_queue) {
		Free_Queue(g_queue);
			g_queue = NULL;
	}
}


void freeEvent(SocketEvent **event)
{
	if(*event) 
	{
		if((*event)->body) {
			free((*event)->body);
			(*event)->body = NULL;
		}
		free((*event));
		*event = NULL;
	}
}




