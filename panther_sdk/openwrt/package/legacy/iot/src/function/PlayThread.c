#include "PlayThread.h"

#include <pthread.h>
#include <limits.h>
#include <semaphore.h>

#include "myutils/debug.h"
#include "common.h"

enum {
	PLAY_THREAD_NONE = 0,
	PLAY_THREAD_STARTED,
	PLAY_THREAD_ONGING,
	PLAY_THREAD_DONE,
	PLAY_THREAD_CLOSE,
	PLAY_THREAD_CANCLE,
};

static int      pleaseStop = 0;

static pthread_t playThread = -1;
static sem_t playThreadSem;
static int playThreadWait = 1;
static int playThreadState = 0;
static int notPlay = 0;

static pthread_mutex_t playThreadMtx = PTHREAD_MUTEX_INITIALIZER;

static void SetPlayThreadWait(int wait)
{
	pthread_mutex_lock(&playThreadMtx);
	playThreadWait = wait;
	pthread_mutex_unlock(&playThreadMtx);
}

static void SetPlayThreadState(int state)
{
	pthread_mutex_lock(&playThreadMtx);
	playThreadState = state;
	pthread_mutex_unlock(&playThreadMtx);
}
void SetNotPlay(int state)
{
	pthread_mutex_lock(&playThreadMtx);
	notPlay = state;
	pthread_mutex_unlock(&playThreadMtx);
}

static int IsPlayThreadFinshed()
{
	int ret;
	pthread_mutex_lock(&playThreadMtx);
	ret = (playThreadState == PLAY_THREAD_NONE);
	pthread_mutex_unlock(&playThreadMtx);
	return ret;
}

int IsPlayThreadCancled()
{
	int ret;
	pthread_mutex_lock(&playThreadMtx);
	ret = (playThreadState == PLAY_THREAD_CANCLE);
	pthread_mutex_unlock(&playThreadMtx);
	return ret;
}
/* 等待播放upload.wav完成,如果再播取消 */
void WaitForPlayThreadFinish()
{
	if(!IsPlayThreadFinshed()) {
		debug("Set PlayThread Cancle...");
		SetPlayThreadState(PLAY_THREAD_CANCLE);
		while(!IsPlayThreadFinshed()) {
			usleep(50*1000);
		}
	}
	SetNotPlay(0);
}
/* 等待播放upload.wav完成,如果再播不取消 */
void WaitForPlayDone()
{
	if(!IsPlayThreadFinshed()) {
		while(!IsPlayThreadFinshed()) {
			usleep(50*1000);
		}
	}
	SetNotPlay(0);
}

static void PlayThread(void *arg)
{
	warning("PlayThread start ...");
	while(1) {
		SetPlayThreadWait(1);
		SetPlayThreadState(PLAY_THREAD_NONE);
		sem_wait(&playThreadSem);
		SetPlayThreadState(PLAY_THREAD_ONGING);
		SetPlayThreadWait(0);
		exec_cmd("killall -9 wavplayer");
		wav_unblock("/root/iot/upload.wav", IsPlayThreadCancled);
		SetPlayThreadState(PLAY_THREAD_DONE);
	}
}
/* 播放upload.wav */
void StartPlay()
{
	pthread_mutex_lock(&playThreadMtx);
	if(notPlay == 0) {
		if(playThreadWait == 1) {
			sem_post(&playThreadSem);
		}
	}
	pthread_mutex_unlock(&playThreadMtx);

}


/* 启动播放upload.wav的线程 */
void CreatePlayThread(void)
{
	pthread_t a_thread;
    pthread_attr_t a_thread_attr;
	//PTHREAD_SET_UNATTACH_ATTR(a_thread_attr);
  	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);
	pthread_create(&playThread, &a_thread_attr, PlayThread, NULL);
	pthread_attr_destroy(&a_thread_attr);
}

/* 销毁播放upload.wav的线程 */
void DestoryPlayThread()
{
	
	if (playThread != 0)
	{
		pthread_join(playThread, NULL);
	}
}


