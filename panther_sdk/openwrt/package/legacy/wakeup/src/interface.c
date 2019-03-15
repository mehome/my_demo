#include "interface.h"

#define _GNU_SOURCE
#include <sched.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

wakeupStartArgs_t *wakeupStartArgsPtr = NULL;
static char MutMicStatus =0;
static pthread_mutex_t MutMicMtx = PTHREAD_MUTEX_INITIALIZER;

int wakeupStart(wakeupStartArgs_t *pArgs){	
	if(!pArgs){
		return -1;
	}
	printf("wakeupStart begin into snowboy_start_routine \n");
#if defined(CONFIG_KWD_KITTAI)
	extern void * snowboy_start_routine(void *args);
	pArgs->pStart_routine=snowboy_start_routine;
#endif
#if defined(CONFIG_KWD_SENSORY)
	extern void * sensory_start_routine(void *args);
	pArgs->pStart_routine=sensory_start_routine;
#endif	
#if defined(CONFIG_KWD_SKV)
	extern void * skv_start_routine(void *args);
	pArgs->pStart_routine=skv_start_routine;
#endif	
	wakeupStartArgsPtr=pArgs;
    printf("wakeup_waitctl = [%p]\n",wakeupStartArgsPtr->wakeup_waitctl);
	int err=pthread_create(&pArgs->wakeupPthreadId,pArgs->pAttr,pArgs->pStart_routine,pArgs);
	if(err){
		show_errno(err,"pthread_create");
		return -1;
	}
	return 0;
}


char getMutMicStatus(void){
	char status=0;
	pthread_mutex_lock(&MutMicMtx);
	status=MutMicStatus;
	pthread_mutex_unlock(&MutMicMtx);
	return status;
}

void setMutMicStatus(char status){
	pthread_mutex_lock(&MutMicMtx);
	MutMicStatus=status;
	pthread_mutex_unlock(&MutMicMtx);
}


