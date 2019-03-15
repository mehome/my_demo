#include "globalParam.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>


GlobalParam g;

static GlobalParam *global = NULL;
GlobalParam * GetGlobalParam()
{
	GlobalParam * pa = NULL;
	pthread_mutex_lock(&global->Clear_easy_handler_mtx);
	pa = global;
	pthread_mutex_unlock(&global->Clear_easy_handler_mtx);
	return pa;
}

void SystemParaInit(void)
{
	global = calloc(1, sizeof(GlobalParam));
	if(global == NULL) {
		error("calloc GlobalParam failed!\n");
		exit(-1);
	}
	assert(global != NULL);
	warning("g:%#x", g);
	if(pthread_mutex_init(&global->Clear_easy_handler_mtx, NULL) != 0) {
		error("Mutex initialization failed!\n");
		exit(-1);
	}

	InitEasyHandler(global);
}
void SystemParaInitDeinit()
{
	if(global) {
		
		free(global);
	}
}



