#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include "AlertHead.h"
#include "AlertHandler.h"
#include "AlertManager.h"
#include "../globalParam.h"
#include "../wavplay/wav_play.h"
#include <mpd/client.h>

#include "debug.h"
#include "alert_api.h"



extern GLOBALPRARM_STRU g;

static time_t start ;
#define TIMEOUT_IN_SEC 360

#define ALEXA_ALER_START "AlexaAlerStart"
#define ALEXA_ALER_END	 "AlexaAlerEnd"
#define ALEXA_ALER_END_IDENTIFY_START	"AlerEndIdentifyOk"	// 闹钟停止，继续播放回复语音

AlertHandler *alerthandler_new(void)
{	int ret;
	AlertHandler *handler = NULL;
	DEBUG_INFO("...");

	handler = calloc(1,sizeof(AlertHandler));
	if(NULL == handler) {
		DEBUG_ERR("calloc AlertHandler failed");
		return NULL;
	}

	handler->alertState = ALERT_FINISHED;
	
	ret = pthread_mutex_init(&handler->aMutex,NULL);
	if(ret) {
		DEBUG_ERR("pthread_mutex_init failed");
		goto failed;
	}

	
	return handler;
failed:
	free(handler);
	handler = NULL;
	return NULL;
	
}

int alerthandler_is_alarming(AlertHandler *handler)
{
	int ret;
	//DEBUG_INFO("....");
	pthread_mutex_lock(&handler->aMutex);
	ret = (handler->alertState == ALERT_PLAYING) ;
	
	pthread_mutex_unlock(&handler->aMutex);
	return ret;
	
}

static void SetMPD_Volume(unsigned volume)
{
	struct mpd_connection *conn = NULL;

	PrintSysTime("Set MPD volume start.");
	// 设置mpd音量，这里没有考虑mpd挂掉或者卡死的情况,测试出使用此函数调节声音的话只需要11ms
	conn = setup_connection();
	if (conn != NULL) {
		if (!mpd_run_set_volume(conn, volume))
			printErrorAndExit(conn);
		else
			mpd_connection_free(conn);
	}
	conn = NULL;
	PrintSysTime("Set MPD volume end.");
}

void delete_audio_file(char *pData)
{
	char fileName[42] = {0};

	char *pMedia1 = strstr(pData, "media");
	memcpy(fileName, &pMedia1[6], 36);

	if ((strcmp(fileName, "ffcc7012-dec2-312d-b02c-a8c39b2cdc2a")) || (strcmp(fileName, "ed7edb59-6b25-323a-a878-a96032d0f11f")))
	{
		if (access(fileName,0) == 0)
		{
			DEBUG_INFO("have a files, continue");
			if (0 == get_file_size(fileName))
			{
				remove(fileName);
			}
		}
	}
	else
	{
		remove(fileName);
	}

	// 取第一个< /dev/null的位置
	char *pdev = strstr(pData, " /dev/null");
	
	char *pMedia2 = strstr(pdev, "media");
	memset(fileName, 0, 42);
	memcpy(fileName, &pMedia2[6], 36);

	if (access(fileName,0) == 0)
	{
		remove(fileName);
	}
}

void *alert_start_thread(void *args)
{
	time_t now ;
	char byFlag = 0;
	char byPlayFlag = 0;
	int iCount = 0;

	DEBUG_INFO("alert start thread start ...");

	AlertScheduler *scheduler = NULL;
	
	scheduler = (AlertScheduler *)args;
	Alert *alert = alertscheduler_get_alert(scheduler);
	AlertManager *alertManager = alertscheduler_get_alertmanager(scheduler);

	start = time(NULL);

	SetMPD_Volume(101);

	//while(alerthandler_is_alarming(alertManager->handler) && g.m_CaptureFlag != 2 && g.m_CaptureFlag != 1) 
	// alert状态改变了，退出线程
	while(alerthandler_is_alarming(alertManager->handler)) 
	{
		if (0 == byFlag)
		{
			byFlag = 1;
			myexec_cmd("uartdfifo.sh "ALEXA_ALER_START);
		//	OnWriteMsgToUartd(ALEXA_ALER_START);
		}

		// 播放reminder
		if ((0 == strcmp(alert->type, TYPE_REMINDER)) || (-1 == alert->m_reminder.loopCount))
		{
			DEBUG_INFO("---------------alert type is reminder ...loopCount：%d", alert->m_reminder.loopCount);
			DEBUG_INFO("assetPlayOrder:%s", alert->m_reminder.assetPlayOrder);

			exec_cmd(alert->m_reminder.assetPlayOrder);
			iCount++;
			if (-1 != alert->m_reminder.loopCount)
			{
				if (iCount >= alert->m_reminder.loopCount)
				{
					system("uartdfifo.sh "ALEXA_ALER_END);
				//	OnWriteMsgToUartd(ALEXA_ALER_END);

					alerthandler_set_state(alertManager->handler, ALERT_FINISHED);
					extern void active_alerts_remove(map_t activeAlerts, char *alertToken);
					active_alerts_remove(alertManager->activeAlerts,alert->token);

					SetMPD_Volume(200);

					// 闹钟完了，要删除之前下载的这个闹钟的文件
					delete_audio_file(alert->m_reminder.assetPlayOrder);

					break;
				}
			}

			usleep(1000 * alert->m_reminder.loopPauseInMilliseconds);
		}
		else
		{
			if (0 == strcmp(alert->type, TYPE_ALARM))
			{
				//system("aplay /root/res/alarm.wav");
				if (0 == byPlayFlag)
				{
					byPlayFlag = 1;
					//system("aplay /root/res/alarm_short.wav");
					wav_play("/root/res/alarm_short.wav");
				}
				else
				{
					//system("aplay /root/res/alarm.wav");
					wav_play("/root/res/alarm.wav");
				}

				// 播放闹钟的时候，如果有语音回复 则闹钟只播放一次
				if (1 == GetTTSAudioStatus())
				{
					myexec_cmd("uartdfifo.sh " ALEXA_ALER_END_IDENTIFY_START);
				//	OnWriteMsgToUartd(ALEXA_ALER_END_IDENTIFY_START);
					DEBUG_INFO("---------------play TTS,  Abort Alarm...");
					//system("uartdfifo.sh "ALEXA_IDENTIFY_OK);
					alerthandler_set_state(alertManager->handler,ALERT_INTERRUPTED);
				}				
			}
			else if (0 == strcmp(alert->type, TYPE_TIMER))
			{
				//system("aplay /root/res/timer.wav");
				if (0 == byPlayFlag)
				{
					byPlayFlag = 1;
					//system("aplay /root/res/timer_short.wav");
					wav_play("/root/res/timer_short.wav");
				}
				else
				{
					//system("aplay /root/res/timer.wav");
					wav_play("/root/res/timer.wav");
				}

				// 播放闹钟的时候，如果有语音回复 则闹钟只播放一次
				if (1 == GetTTSAudioStatus())
				{
					myexec_cmd("uartdfifo.sh " ALEXA_ALER_END_IDENTIFY_START);
			//		OnWriteMsgToUartd(ALEXA_ALER_END_IDENTIFY_START);
					DEBUG_INFO("---------------play TTS,  Abort Timer...");
					alerthandler_set_state(alertManager->handler,ALERT_INTERRUPTED);
				}
			}

			now = time(NULL);
			double diff ;
			diff = difftime(now, start);
			//DEBUG_INFO("diff:%f", diff);
			if(diff > TIMEOUT_IN_SEC) {
				DEBUG_INFO("alert start thread timeout ...");

				myexec_cmd("uartdfifo.sh "ALEXA_ALER_END);
		//		OnWriteMsgToUartd(ALEXA_ALER_END);

				alerthandler_set_state(alertManager->handler,ALERT_FINISHED);

				SetMPD_Volume(200);

				break;
			}
		}
	}
	
	if ((-1 == alert->m_reminder.loopCount) && (iCount != 0))
	{
		delete_audio_file(alert->m_reminder.assetPlayOrder);	
	}
	
	alertManager->handler->data = scheduler;
	if( alerthandler_get_state(alertManager->handler) == ALERT_FINISHED ) {
		//alert_set_stop(alert, 1);
		//alertmanager_write_current_alerts_to_disk(alertManager);
		char *token = alert_get_token(alert);
		alertmanager_set_alert_stop(alertManager, token , 1);
	}
	DEBUG_INFO("alert start thread end ...");
}

void alerthandler_start_alert(AlertScheduler *scheduler, AlertManager *manager)
{
	//播放 alert
	AlertManager *alertManager = alertscheduler_get_alertmanager(scheduler);
	if(!alerthandler_is_alarming(alertManager->handler)) {
		//暂停播放
		//if(1) { //判断是否Alexa is currently speaking 
		if(g.m_CaptureFlag == 0 || g.m_CaptureFlag == 3 ) {
			alerthandler_set_state(alertManager->handler,ALERT_PLAYING);
			DEBUG_INFO("Start playing alert ....");

			pthread_create(&alertManager->handler->tid, NULL, alert_start_thread, (void *)scheduler);

			pthread_detach(alertManager->handler->tid);
			//system("aplay /tmp/res/welcome_back.wav  -M");
			//DEBUG_INFO("aplay /tmp/res/welcome_back.wav  -M");
		} else {
			alerthandler_set_state(alertManager->handler,ALERT_INTERRUPTED);
			DEBUG_INFO("ALERT_INTERRUPTED ....");
		}
	} else {
		DEBUG_INFO("alerthandler is alarming....");
	}
	
}

void alerthandler_stop_alert(AlertHandler *handler, AlertManager *manager)
{
	//DEBUG_INFO("....");
	if(alertmanager_has_activealerts(manager) == 0) {
		alerthandler_set_state(handler, ALERT_FINISHED);
	}

}

AlertState alerthandler_get_state(AlertHandler *handler)
{
	//DEBUG_INFO("....");
	AlertState state;	
	pthread_mutex_lock(&handler->aMutex);
	state = handler->alertState;
	pthread_mutex_unlock(&handler->aMutex);
	return state;
}

void  alerthandler_set_state(AlertHandler *handler, AlertState state)
{
	//DEBUG_INFO("...");
	pthread_mutex_lock(&handler->aMutex);
	handler->alertState = state;
	pthread_mutex_unlock(&handler->aMutex);
}



int alerthandler_resume_alerts(AlertManager * manager)
{	
	AlertHandler *handler =NULL;
	AlertScheduler *scheduler = NULL;

	handler = manager->handler;

	DEBUG_INFO("handler = manager->handle ");
	if(alerthandler_get_state(handler) == ALERT_INTERRUPTED) {
		printf(LIGHT_RED"alerthandler_resume_alerts\n"NONE);	
		//scheduler = manager->handler->data;
		scheduler = alertmanager_get_activealertscheduler(manager);
		if (scheduler)
		{	
			DEBUG_INFO("alerthandler_start_alert :%s ",scheduler->alert->token);
			//pthread_mutex_unlock(&handler->aMutex);
			alerteventlistener_on_alert_started(scheduler->alert->token);
			alerthandler_start_alert(scheduler, manager);
		}
		else
		{
			DEBUG_INFO("alerthandler_start_alert 111111111");
		}
		return 1;
	}	
	DEBUG_INFO("return oooo ");
	//pthread_mutex_unlock(&handler->aMutex);
	return 0;
}


void alerthandler_free(AlertHandler *handler)
{	
	//EBUG_INFO("....");
	if(handler) {
		free(handler);
		handler = NULL;
	}
}


