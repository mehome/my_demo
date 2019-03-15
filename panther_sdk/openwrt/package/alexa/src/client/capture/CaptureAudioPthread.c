#include "CaptureAudioPthread.h"
#include "../globalParam.h"

pthread_t CaptureAudio_Pthread;

extern FT_FIFO * g_CaptureFifo;
extern GLOBALPRARM_STRU g;

extern FT_FIFO * g_DecodeAudioFifo;
extern FT_FIFO * g_PlayAudioFifo;
extern char g_playFlag;
extern char g_byWakeUpFlag;

void *CaptureAudioPthread(void)
{
	while (1)
	{
		sem_wait(&g.Semaphore.Capture_sem);
		if (NULL == g_CaptureFifo)
		{
			DEBUG_PRINTF("alloc g_CaptureFifo");
			g_CaptureFifo = ft_fifo_alloc(CAPTURE_FIFO_LENGTH);
		}
		else
		{
			DEBUG_PRINTF("clear g_CaptureFifo");
			ft_fifo_clear(g_CaptureFifo);
		}

		g.m_byAudioRequestFlag = AUDIO_REQUEST_START;

		if (g_DecodeAudioFifo)
		{
			printf(LIGHT_BLUE "free g_DecodeAudioFifo\n"NONE);
			ft_fifo_free(g_DecodeAudioFifo);
			g_DecodeAudioFifo = NULL;
		}

		if (g_PlayAudioFifo)
		{
			ft_fifo_free(g_PlayAudioFifo);
			g_PlayAudioFifo = NULL;
		}

		if (g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle != NULL)
		{
			DEBUG_PRINTF(LIGHT_PURPLE"Abort _handler\n"NONE);
			SetCallbackStatus(1);
		}

		if (1 == GetAmazonPlayBookFlag())
		{
			// 如果播放的是书，暂停播放，并发送STOP指令
			MpdPause();
			Queue_Put(ALEXA_EVENT_PLAYBACK_STOP);
		}

		g_playFlag = 1;

		g_byWakeUpFlag = 1;

		// 唤醒,通知蓝牙
		OnWriteMsgToUartd(ALEXA_WAKE_UP);

#ifdef ALEXA_FF
		capture20921();
#else
		capture_voice();
#endif
	}
}

void CreateCapturePthread(void)
{
	int iRet;

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);

	iRet = pthread_create(&CaptureAudio_Pthread, &a_thread_attr, CaptureAudioPthread, NULL);

	//iRet = pthread_create(&CaptureAudio_Pthread, NULL, CaptureAudioPthread, NULL);
	if(iRet != 0)
	{
		DEBUG_PRINTF("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}



