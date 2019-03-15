#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>

#include <alsa/asoundlib.h>

#include "fifobuffer.h"
#include "myutils/debug.h"

extern size_t alsa_play_chunk_bytes;

#undef FIFO_LENGTH
#define FIFO_LENGTH     16384

static FT_FIFO * g_AplayFifo = NULL;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 

static pthread_t aplayPthread = 0;

static pthread_mutex_t aplayMtx = PTHREAD_MUTEX_INITIALIZER;

static sem_t playwav_sem;

int AplayStart()
{
	sem_post(&playwav_sem);
}
enum {
	APLAY_NONE= 0,
	APLAY_STARTED,
	APLAY_ONGING,
	APLAY_DONE,
	APLAY_CLOSE,
	APLAY_CANCLE,
};


static int aplayState = APLAY_NONE;

int IsAplayCancled()
{
	int ret;
	pthread_mutex_lock(&aplayMtx);
	ret = (aplayState == APLAY_CANCLE);
	pthread_mutex_unlock(&aplayMtx);
	return ret;
}

void SetAplayState(int state)
{
	pthread_mutex_lock(&aplayMtx);
	aplayState = state;
	pthread_mutex_unlock(&aplayMtx);
}
int IsAplayFinshed()
{
	int ret;
	pthread_mutex_lock(&aplayMtx);
	ret = (aplayState == APLAY_NONE);
	pthread_mutex_unlock(&aplayMtx);
	return ret;
}



void SetAplayCancle()
{
	pthread_mutex_lock(&aplayMtx);
	aplayState = APLAY_CANCLE;
	pthread_mutex_unlock(&aplayMtx);
	
}

#if 1
int AplayThread(void)
{
	int iFd = -1;
	char *pData = NULL;
	unsigned long wLenght = 0;

	int nread = 0;
	FILE * fp = NULL;

	char *pTemp = NULL;

	struct mpd_connection *conn = NULL;

	while (1)
	{
		error("APLAY_NONE...");
		SetAplayState(APLAY_NONE);
		sem_wait(&playwav_sem);
		SetAplayState(APLAY_ONGING);
		PrintSysTime("aplay alexa_start");

		fp = fopen("/root/res/alexa_start.wav", "rb");

		fseek(fp, 58, SEEK_SET);

		alsa_init();

		if (NULL == pTemp)
			pTemp = malloc(alsa_play_chunk_bytes);

		while (1)
		{
			if(IsAplayCancled()) {
				break;
			}
			nread = fread(pTemp, 1, alsa_play_chunk_bytes, fp);
			if(nread == 0)
			{
				PrintSysTime("aplay searchstart end");
				alsa_close();
				break;		  
			}
			snd_Play_Buf(pTemp, alsa_play_chunk_bytes);
		}

		free(pTemp);
		pTemp = NULL;

		fclose(fp);
		SetAplayState(APLAY_DONE);
		error("APLAY_DONE...");
	}

	return 0;
}

#else
int AplayThread(void)
{
	char *pData = NULL;
	unsigned long wLenght = 0;


	char *pTemp = NULL;


	while (1)
	{
		sem_wait(&playwav_sem);

		alsa_init();
		ft_fifo_clear(g_AplayFifo);
		
		while (1)
		{
			int iLength;
			int len;
			iLength = ft_fifo_getlenth(g_AplayFifo);
			
			if (iLength >= alsa_play_chunk_bytes)
			{
				info("iLength %d",iLength);
				if(iLength > alsa_play_chunk_bytes)
					len = alsa_play_chunk_bytes;
				else
					len = iLength;
				if (NULL == pTemp)
				pTemp = malloc(len);
				ft_fifo_seek(g_AplayFifo, pTemp, 0, len);
				
				ft_fifo_setoffset(g_AplayFifo, len);
				info("aplay %d",len);
				snd_Play_Buf(pTemp, len);
				
				free(pTemp);
				pTemp = NULL;
				continue;
			}
		}
exit:

		alsa_close();
	
	}

	return 0;
}

#endif

int  CreateAplayPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);

	iRet = pthread_create(&aplayPthread, &a_thread_attr, AplayThread, NULL);
	
	if(iRet != 0)
	{
		info("pthread_create error:%s\n", strerror(iRet));
		return -1;
	}
	return 0;
}
int  DestoryAplayPthread(void)
{
	if (aplayPthread != 0)
	{
		pthread_join(aplayPthread, NULL);
		info("aplay_pthread end...\n");
	}	
}

