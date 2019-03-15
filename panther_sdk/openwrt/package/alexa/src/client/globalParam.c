#include "globalParam.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

UINT8 g_byDebugFlag = DEBUG_OFF;
GLOBALPRARM_STRU g;
UINT8 g_bysubmitToken[2560] = {0};

/*
 * 0:登陆失败或未登陆
 * 1:登陆成功
 * 2:登陆中
*/
UINT8 g_byAmazonLogInFlag = 0;

char g_bySynStatusIndex = -1;
char PingPackageIndex = -1;

char g_byWakeUpFlag = 0;
char g_playFlag = 0;
FT_FIFO * g_DataFifo = NULL;
FT_FIFO * g_CaptureFifo = NULL;
FT_FIFO * g_DecodeAudioFifo = NULL;
FT_FIFO * g_PlayAudioFifo = NULL;
int32_t inactiveTime = 0;
extern int g_iexpires_in = 0;


extern struct  timeval	begin_tv;

static void TimerRefreshAmazonToken(void)
{
	char icount = 0;
	bool iRet = false;

	g_iexpires_in = 0;
	for(icount = 0; icount < 5;icount++)
	{
		DEBUG_PRINTF("TimerRefreshAmazonToken Refresh_AccessToken");
		if ((iRet = OnRefreshToken()) == 0)
		{
			break;
		}
	}

	if (iRet != 0)
	{
		exit(-1);
	}
}

void alarm_handler(void)
{
 	static int TokenCount = 0;
	static int InactivityCount = 0;
	struct  timeval	end_tv;

	if (1 == GetAmazonConnectStatus())
	{
		TokenCount++;
		InactivityCount++;

		if(InactivityCount == SUBMIT_USER_INACTIVITY)	// 60 minutes submit amazon user activity event
		{
			InactivityCount = 0;	
			gettimeofday(&end_tv, NULL);	//»ñÈ¡ÏµÍ³µ±Ç°Ê±¼ä
			//inactive time
			if(end_tv.tv_sec >= begin_tv.tv_sec)
				inactiveTime = end_tv.tv_sec - begin_tv.tv_sec;
			else
				inactiveTime = (24 * 60 * 60) - begin_tv.tv_sec + end_tv.tv_sec;

			Queue_Put(ALEXA_EVENT_REPORTUSERINACTIVITY);
		}

		if(TokenCount == TIMER_REFRESH_TOKEN)	// 50 minutues refresh token
		{
			TokenCount = 0;
			TimerRefreshAmazonToken();
		}

		Queue_Put(ALEXA_EVENT_PINGPACKS);

	}
	alarm(60 * 5);
}

void register_alarm(void)
{
    signal(SIGALRM, alarm_handler); //让内核做好准备，一旦接受到SIGALARM信号,就执行 handler
    alarm(60 * 5);
}

int cmp_int_ptr(int *a, int *b) {
	if(*a < *b)
		return -1;
	else if(*a > *b)
		return 1;
	else
		return 0;
}

static void Init_Queue(void)
{
	int i;

	/*if(pthread_mutex_init(&g.m_RequestQueue.lock, NULL) != 0) {
		DEBUG_PRINTF("Mutex initialization failed!\n");
	}

	g.m_RequestQueue.queue = queue_create_sorted(1, (int (*)(void *, void *))cmp_int_ptr);
	for (i = 0; i < 10; i++)
	{
		g.m_RequestQueue.m_queueData[i] = -1;
	}*/
#ifndef USE_EVENT_QUEUE
	g_DataFifo = ft_fifo_alloc(10);
#else
	QueueInit();
#endif
}


static void Semaphore_Init(void)
{
	int iRet = -1;

	iRet = sem_init(&g.Semaphore.Capture_sem, 0, 0);
    if(iRet < 0)
    {
	    DEBUG_ERROR("Semaphore Capture_sem initialization failed..");  
	    exit(-1);  
    }

	iRet = sem_init(&g.Semaphore.MadPlay_sem, 0, 0);
    if(iRet < 0)
    {
	    DEBUG_ERROR("Semaphore MadPlay_sem initialization failed..");  
	    exit(-1);  
    }

	iRet = sem_init(&g.Semaphore.WakeUp_sem, 0, 0);
    if(iRet < 0)
    {
	    DEBUG_ERROR("Semaphore MadPlay_sem initialization failed..");  
	    exit(-1);  
    }

	
}

static void Init_VADThreshold(void)
{
	g.m_iVadThreshold = ReadConfigValueInt("vad_Threshold");
	if(0 == g.m_iVadThreshold) {
		g.m_iVadThreshold = 1000000;
	}
}

static void Init_NotificationParam(void)
{
	g.m_Notifications.m_IsEnabel = ReadConfigValueInt("Notification_Enable");
	g.m_Notifications.m_isVisualIndicatorPersisted = ReadConfigValueInt("Notification_VisualIndicatorPersisted");
}

void SystemPara_Init(void)
{
	if(pthread_mutex_init(&g.Clear_easy_handler_mtx, NULL) != 0) {
		DEBUG_PRINTF("Mutex initialization failed!\n");
		return NULL;
	}

	if(pthread_mutex_init(&g.vadFlag.VAD_mtx, NULL) != 0) {
		DEBUG_PRINTF("Mutex initialization failed!\n");
		return NULL;
	}

	if(pthread_mutex_init(&g.Handle_mtc, NULL) != 0) {
		DEBUG_PRINTF("Handle_mtc failed!\n");
		return NULL;
	}


	memset(g.m_mpd_status.mpd_state, 0, 10);
	memcpy(g.m_mpd_status.mpd_state, "IDLE", strlen("IDLE"));
	g.m_AmazonMuteStatus = 0;
	g.m_AmazonMuteOldStatus = 0;
	g.m_byPlayBookFlag = 0;
	g.m_mpd_status.volume = GetMpcVolume();
	g.m_mpd_status.intervalInMilliseconds = 0;
	g.m_TTSAudioFlag = 0;
	g.m_bySpeechFlag = 0;
	g.m_byAbortCallback = 0;	
	g.endpoint.m_byDownChannelFlag = 0;
	g.m_mpd_status.total_time = 0;

	Init_VADThreshold();

	Init_NotificationParam();

	Init_easy_handler();

	Init_Queue();

	InitAmazonConnectStatus();

	OnUpdateURL(NULL);

	Semaphore_Init();

	register_alarm();

	MpdInit();

	SetPlayStatus(AUDIO_STATUS_STOP);
}


