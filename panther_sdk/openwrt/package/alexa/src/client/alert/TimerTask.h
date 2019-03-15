#ifndef __TIMER_H__
#define __TIMER_H__


#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>


typedef struct timer_task_s{
	struct event_base *base;
	struct event *timeout;
	struct timeval tv;
	struct timeval lasttime;
	char *name;
	pthread_t tid;
}TimerTask;

#endif