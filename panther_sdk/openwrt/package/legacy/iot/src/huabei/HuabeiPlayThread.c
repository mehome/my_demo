#include "HuabeiPlayThread.h"

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include "queue.h"

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static QUEUE *huabeiPlayQueue=NULL;
static pthread_t huabeiPlayPthread = 0;

static pthread_mutex_t mpdMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t mpdCond = PTHREAD_COND_INITIALIZER;

static      int playQueueExit  = 0;
int HuabeiPlayQueueAdd(char *url)
{

	int ret = 0;
	assert(huabeiPlayQueue != NULL);
	warning("before add ....");
	pthread_mutex_lock(&mtx);
	warning("pthread_mutex_lock(&mtx);");
	Add_Queue_Item(huabeiPlayQueue,0, url);
	pthread_cond_signal(&cond);  
	pthread_mutex_unlock(&mtx);
	warning("add HuabeiPlayQueueAdd done..");
	return 0;
	
}

void SetHuabeiPlayQueueExit(int exit)
{
	playQueueExit = exit;
}

void MpdPlayDone()
{
	pthread_mutex_lock(&mpdMtx);
	pthread_cond_signal(&mpdCond);  
	pthread_mutex_unlock(&mpdMtx);
}

void MpdPlayUrl(char *url)
{
	HuabeiPlayHttp(url);	
	pthread_mutex_lock(&mpdMtx);
	pthread_cond_wait(&mpdCond, &mpdMtx);
	pthread_mutex_unlock(&mpdMtx);
}


void HuabeiPlayPthread(void *arg)
{
	char *url;
	QUEUE_ITEM *item = NULL;
	
	while(1) {
		pthread_mutex_lock(&mtx);
		while(Get_Queue_Sizes(huabeiPlayQueue) == 0) {
			 warning("huabeiPlayQueue is empty..");
			 pthread_cond_wait(&cond, &mtx);
		}
		item = Get_Queue_Item(huabeiPlayQueue);
		debug("item->type:%d",item->type);
		url	= (char *)item->data;
		if(url) {
			info("url:%s",url);
			pthread_mutex_unlock(&mtx);
			MpdPlayUrl(url);
			pthread_mutex_lock(&mtx);
			free(url);
		}
		Free_Queue_Item(item);
		if(playQueueExit == 1) {
			playQueueExit = 0;
			while(Get_Queue_Sizes(huabeiPlayQueue)) {
				item = Get_Queue_Item(huabeiPlayQueue);
				url	= (char *)item->data;
				if(url) {
					info("url:%s",url);
					free(url);
				}
				Free_Queue_Item(item);
			}
		}
		pthread_mutex_unlock(&mtx);
	}
}

int HuabeiPlayQueueInit()
{
	huabeiPlayQueue = Initialize_Queue();
	if(!huabeiPlayQueue) {
		error("Initialize_Queue failed");
		exit(-1);
	}

	return 0;
}

void HuabeiPlayQueueDeinit()
{
	if(huabeiPlayQueue) {
		Free_Queue(huabeiPlayQueue);
		huabeiPlayQueue = NULL;
	}
}

void CreateHuabeiPlayPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);
	HuabeiPlayQueueInit();
	
	iRet = pthread_create(&huabeiPlayPthread, &a_thread_attr, HuabeiPlayPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}
	
	
}
int DestoryHuabeiPlayPthread(void)
{
	if (huabeiPlayPthread != 0)
	{
		pthread_join(huabeiPlayPthread, NULL);
		info("capture_destroy end...\n");
	}	
	HuabeiPlayQueueDeinit();
}




