#include "globalParam.h"
#include <json/json.h>
#include "alert/AlertHead.h"

extern AlertManager *alertManager;

extern GLOBALPRARM_STRU g;
extern FT_FIFO * g_DecodeAudioFifo;
extern AlertManager *alertManager;
extern struct  timeval	begin_tv;
extern UINT8 g_byAmazonLogInFlag;

extern UINT8 g_byDebugFlag;
extern unsigned char g_byPlayFinishedRecFlag;

int directiveCount;

int g_bycidFlag = 0;

static char * get_line( char *dst, char *src) 
{
	char * cs;
	int i = 0;
	char *tmp = src;
	int len = strlen(src) - 1;
	//DEBUG_PRINTF("len :%d", len);
	if(len <= 0)
		return NULL;
	cs = dst;

	while ( i < len )
	{
		if (*tmp == '\n')
		{
			tmp++;
			*cs++ = '\0';
			return tmp;
		}

		*cs++ = *tmp++;

		i++;
	}
	return NULL;
}

#if 1
static int ProcessAlertDirective(char *pData)
{
	char *name = NULL;
	char *data = NULL;

	struct json_object *directive = NULL, *payload = NULL, *root=NULL, *header =NULL;
	struct json_object *tokenObj = NULL, *typeObj = NULL, *scheduledObj=NULL,*nameObj= NULL;

	root = json_tokener_parse(pData);
	if (is_error(root)) {
		DEBUG_PRINTF("json_tokener_parse directive\n");
		return -1;
	}
	directive = json_object_object_get(root, "directive");
	if(directive == NULL) {
		DEBUG_PRINTF("json_object_object_get payload failed\n");
		return -1;
	}
	DEBUG_PRINTF("@@@@@@@@@@@@@@@@@@@  directive:%s  @@@@@@@@@@@@@@@@@@@ \n",json_object_to_json_string(directive));
	header  = json_object_object_get(directive, "header");
	if(header == NULL) {
		DEBUG_PRINTF("json_object_object_get payload failed");
		return -1;
	}
	DEBUG_PRINTF("@@@@@@@@@@@@@@@@@@@  header:%s  @@@@@@@@@@@@@@@@@@@ \n",json_object_to_json_string(header));


	payload = json_object_object_get(directive, "payload");
	if(payload == NULL) {
		DEBUG_PRINTF("json_object_object_get payload failed\n");
		return -1;
	}
	DEBUG_PRINTF("@@@@@@@@@@@@@@@@@@@  payload:%s  @@@@@@@@@@@@@@@@@@@ \n",json_object_to_json_string(payload));

	nameObj 		= json_object_object_get(header, "name");
	name 			= strdup(json_object_get_string(nameObj));

	data  = strdup(json_object_to_json_string(payload));

	//tokenObj 		= json_object_object_get(payload, "token");
	//typeObj 		= json_object_object_get(payload, "type");
	//scheduledObj 	= json_object_object_get(payload, "scheduledTime");

	//token 			= strdup(json_object_get_string(tokenObj));
	//type 			= strdup(json_object_get_string(typeObj));
	//scheduledTime 	= strdup(json_object_get_string(scheduledObj));

	json_object_put(root);

	return handle_alerts_directive(alertManager, name, data);

	return 0;
}
#endif

static int ProcessSystemDirective(char *pData)
{
	int iRet = 0;

	char *pTempName = NULL;
	struct json_object *root = NULL, *directive = NULL, *header = NULL;
	struct json_object *Name = NULL, *payload = NULL, *endpoint = NULL;

	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		DEBUG_PRINTF("json_tokener_parse directive");
		return -1;
	}

	directive = json_object_object_get(root, "directive");
	if(directive == NULL) 
	{
		DEBUG_PRINTF("json_object_object_get payload failed");
		return -1;
	}

	header = json_object_object_get(directive, "header");
	if(header == NULL) 
	{				
		DEBUG_PRINTF("json_object_object_get header failed");
		return NULL;			
	}

	Name = json_object_object_get(header, "name");
	pTempName = strdup(json_object_get_string(Name));

	if (!strcmp(pTempName, SETENDPOINT))
	{
		payload = json_object_object_get(directive, "payload");
		if (payload == NULL) 
		{				
			DEBUG_PRINTF("json_object_object_get payload failed");
		}
		else
		{
			endpoint = json_object_object_get(payload, "endpoint");
			if (endpoint != NULL) 
			{				
				char *pTempURL = strdup(json_object_get_string(endpoint));
				
				WriteConfigValueString("endpoint", pTempURL);
				OnUpdateURL(pTempURL);
				free(pTempURL);
				printf(LIGHT_RED"endpoint is :%s\n"NONE, json_object_get_string(endpoint));
			}
		}
		
	}
	else if (!strcmp(pTempName, SYSTEM_RESETUSERINACTIVITY)) 
	{
		gettimeofday(&begin_tv, NULL); //获取会话活动成功的系统时间
		DEBUG_PRINTF("[ResetUserInactivity]");
	}
	else
	{
		DEBUG_PRINTF("Unsupported command..");
	}

	json_object_put(root);	

	if (pTempName)
	{
		free(pTempName);
		pTempName = NULL;
	}

	return iRet;
}

static int ProcessNotificationsDirective(char *pData)
{
	char *pTempName = NULL;
	char *pTempURL = NULL;

	struct json_object *root = NULL, *directive = NULL, *header = NULL;
	struct json_object *Name = NULL, *payload = NULL, *asset = NULL, *url = NULL;
	//struct json_object *persistVisualIndicator = NULL, 


	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		DEBUG_PRINTF("json_tokener_parse directive");
		return -1;
	}

	directive = json_object_object_get(root, "directive");
	if(directive == NULL) 
	{
		DEBUG_PRINTF("json_object_object_get payload failed");
		return -1;
	}

	header = json_object_object_get(directive, "header");
	if(header == NULL) 
	{				
		DEBUG_PRINTF("json_object_object_get header failed");
		return NULL;			
	}

	Name = json_object_object_get(header, "name");
	pTempName = strdup(json_object_get_string(Name));

	if (!strcmp(pTempName, "SetIndicator"))
	{
		payload = json_object_object_get(directive, "payload");
		if(payload == NULL) 
		{				
			DEBUG_PRINTF("json_object_object_get payload failed");
			return NULL;			
		}
		
		asset = json_object_object_get(payload, "asset");
		if(asset == NULL) 
		{				
			DEBUG_PRINTF("json_object_object_get asset failed");
			return NULL;			
		}
		
		url = json_object_object_get(asset, "url");
		if(url == NULL) 
		{				
			DEBUG_PRINTF("json_object_object_get url failed");
			return NULL;			
		}
		
		pTempURL = strdup(json_object_get_string(url));
		
		// 判断是否为默认提示音
		if (NULL != strstr(pTempURL, "default_notification_sound.mp3"))
		{
			printf(LIGHT_RED "<<<<<<<<<<<NotificationsDirective CMD.....\n"NONE);
			//PlayWav("/root/res/default_notification_sound.wav");
			myexec_cmd("aplay /root/res/default_notification_sound.wav");
		}
		else
		{
			//PlayWav("/root/res/med_alerts_notification_01._TTH_.wav ");
			myexec_cmd("aplay /root/res/med_alerts_notification_01._TTH_.wav ");
		}

		g.m_Notifications.m_IsEnabel = 1;		
		g.m_Notifications.m_isVisualIndicatorPersisted = 1;
		
		WriteConfigValueInt("Notification_Enable", 1);
		WriteConfigValueInt("Notification_VisualIndicatorPersisted", 1);

		OnWriteMsgToUartd(ALEXA_NOTIFICATIONS_START);

		if (pTempURL)
		{
			free(pTempURL);
			pTempURL = NULL;
		}
	}
	else if (!strcmp(pTempName, "ClearIndicator"))
	{
		g.m_Notifications.m_IsEnabel = 0;		
		g.m_Notifications.m_isVisualIndicatorPersisted = 0;
		
		WriteConfigValueInt("Notification_Enable", "0");
		WriteConfigValueInt("Notification_VisualIndicatorPersisted", "0");

		OnWriteMsgToUartd(ALEXA_NOTIFACATIONS_END);
	}

	json_object_put(url);
	json_object_put(asset);
	json_object_put(payload);
	json_object_put(header);
	json_object_put(directive);
	json_object_put(root);	

	if (pTempName)
	{
		free(pTempName);
		pTempName = NULL;
	}

	return 0;
}

static int ProcessSpeakerDirective(char *pData)
{
	static char byVolume = 0;
	char byTemVolume = 0;
	char byTemBuffer[64] = {0};
	char *pTempName = NULL;
	char MuteStatus = 0;
	struct json_object *root = NULL, *directive = NULL, *payload = NULL, *volume = NULL, *header = NULL, *Name = NULL, *Mute = NULL;			

	DEBUG_PRINTF("ProcessVolumeJson....");

	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		DEBUG_PRINTF("json_tokener_parse directive");
		return -1;
	}

	directive = json_object_object_get(root, "directive");
	if(directive == NULL) 
	{
		DEBUG_PRINTF("json_object_object_get payload failed");
		return -1;
	}

	header = json_object_object_get(directive, "header");
	if(header == NULL) 
	{				
		DEBUG_PRINTF("json_object_object_get header failed");
		return NULL;			
	}

	Name = json_object_object_get(header, "name");
	pTempName = strdup(json_object_get_string(Name));

	payload = json_object_object_get(directive, "payload");
	if(payload == NULL) 
	{				
		DEBUG_PRINTF("json_object_object_get payload failed");
		return -1;			
	}

	volume = json_object_object_get(payload, "volume");
	byTemVolume = json_object_get_int(volume);

	Mute = json_object_object_get(payload, "mute");
	g.m_AmazonMuteOldStatus = (char)json_object_get_boolean(Mute);

	json_object_put(volume);
	json_object_put(Mute);
	json_object_put(payload);
	json_object_put(directive);	
	json_object_put(root);	

	if (!strncmp(pTempName, SPEAKER_SETVOLUME, (sizeof(SPEAKER_SETVOLUME)-1)))
	{
		//if (byVolume != byTemVolume)
		{
			DEBUG_PRINTF("[Set volume]");
			byVolume = byTemVolume;
			g.m_localValume = byVolume;
			DEBUG_PRINTF("byTemVolume:%d, g.m_localValume:%d", byTemVolume, g.m_localValume);
			SetMpcVolume(g.m_localValume);
			sprintf(byTemBuffer, "mpc volume 200");
			myexec_cmd(byTemBuffer);
			//sprintf(byTemBuffer, "mpc volume %d && mpc volume 200", byVolume);
			//myexec_cmd(byTemBuffer);

			Queue_Put(ALEXA_EVENT_VOLUME_CHANGED);
			OnWriteMsgToUartd(ALEXA_VOICE_PLAY_END);
			alerthandler_resume_alerts(alertManager);

			if (1 == GetAmazonPlayBookFlag())
			{
				myexec_cmd("mpc play");
				Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);
			}
		}
	}
	else if (!strncmp(pTempName, SPEAKER_ADJUSTVOLUME, (sizeof(SPEAKER_ADJUSTVOLUME)-1)))
	{
		DEBUG_PRINTF("[Adjust volume]");
		byVolume += byTemVolume;
		g.m_localValume = GetMpcVolume();
		g.m_localValume += byTemVolume;
		DEBUG_PRINTF("byVolume:%d", byVolume);
		SetMpcVolume(g.m_localValume);
		//sprintf(byTemBuffer, "mpc volume %d && mpc volume 200", g.m_localValume);
		sprintf(byTemBuffer, "mpc volume 200");
		myexec_cmd(byTemBuffer);

		Queue_Put(ALEXA_EVENT_VOLUME_CHANGED);
		OnWriteMsgToUartd(ALEXA_VOICE_PLAY_END);
		alerthandler_resume_alerts(alertManager);

		if (1 == GetAmazonPlayBookFlag())
		{
			myexec_cmd("mpc play");
			Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);
		}
	}
	else if (!strncmp(pTempName, SPEAKER_SETMUTE, (sizeof(SPEAKER_SETMUTE)-1)))
	{
		DEBUG_PRINTF("[Set Mute]:%d", g.m_AmazonMuteOldStatus);
	}
	else
	{
		DEBUG_PRINTF("Not supported command :%s", pTempName);
		free(pTempName);

		alerthandler_resume_alerts(alertManager);

		if (1 == GetAmazonPlayBookFlag())
		{
			myexec_cmd("mpc play");
			Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);
		}

		return -2;
	}

	free(pTempName);

	return 0;
}

static int ProcessSpeechSynthesizerDirective(char *pData)
{
	struct json_object *root = NULL, *directive = NULL, *payload = NULL, *token = NULL;			

	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		DEBUG_PRINTF("json_tokener_parse directive");
		return -1;
	}

	directive = json_object_object_get(root, "directive");
	if(directive == NULL) 
	{
		DEBUG_PRINTF("json_object_object_get payload failed");
		return -1;
	}

	payload = json_object_object_get(directive, "payload");
	if(payload == NULL) 
	{				
		DEBUG_PRINTF("json_object_object_get payload failed");
		return -1;			
	}

	token = json_object_object_get(payload, "token");

	g.m_SpeechToken = strdup(json_object_get_string(token));

	json_object_put(token);
	json_object_put(payload);
	json_object_put(directive);	
	json_object_put(root);	
	g.m_AudioFlag = 1;
	DEBUG_PRINTF("------------g.m_AudioFlag:%d", g.m_AudioFlag);

	if (g_DecodeAudioFifo == NULL)
	{
		g_DecodeAudioFifo = ft_fifo_alloc(DECODE_FIFO_BUFFLTH);

		DEBUG_PRINTF("[ProcessSpeechSynthesizerDirective]:%d", g.m_AudioFlag);
		sem_post(&g.Semaphore.MadPlay_sem);
	}

	return 0;
}

int playMusic(char byChange)
{
	char byFlag = 0;
	g.m_PlayerStatus = AMAZON_AUDIOPLAYER_PLAY;
	int iSeekSec = g.CurrentPlayDirective.offsetInMilliseconds / 1000;
	
	myexec_cmd("mpc repeat off &");
	OnWriteMsgToUartd(ALEXA_VOICE_PLAY_END);

	// 无语音指令，直接播放音乐
	DEBUG_PRINTF("-------------------[do not have speech] -> [play].byChange:%d, iSeekSec:%d", byChange, iSeekSec);

	// 如果当前正zai闹铃，则直接将音乐声音放到最小,并恢复播放闹钟
	if(alerthandler_get_state(alertManager->handler) != ALERT_FINISHED)
	{
		byFlag = 1;
		MpdVolume(140);
		alerthandler_resume_alerts(alertManager);
	}

	// 如果当前正zai请求语音数据或者zai播放语音数据，也应该将声音放到最小
	char tmp_audiorequest_status = GetAudioRequestStatus();
	char tmp_TTSAudio_Status = GetTTSAudioStatus();
	DEBUG_PRINTF("\n\ntmp_audiorequest_status: %d\ntmp_speech_status: %d g_byPlayFinishedRecFlag:%x\n\n", tmp_audiorequest_status, tmp_TTSAudio_Status, g_byPlayFinishedRecFlag);
	if(g_DecodeAudioFifo){
		DEBUG_PRINTF("g_DecodeAudioFifo not null\n");
	}

	if (0xFE == g_byPlayFinishedRecFlag)
	{
		DEBUG_PRINTF("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<##########################$$$$$$$$$$$$$$$$$$$$");
		if ((AUDIO_REQUEST_IDLE != tmp_audiorequest_status) || (0 != tmp_TTSAudio_Status))
		{
			byFlag = 1;
			MpdVolume(101);
		}
	}

	SetPlayStatus(AUDIO_STATUS_STOP);


	if (1 != byFlag)
	{
		MpdVolume(200);
	}

	byFlag = 0;

	// 如果不为切换歌曲则直接恢复歌曲的播放。
	if (1 == byChange)
	{
		printf(LIGHT_GREEN"[Resume Playing..]\n"NONE);
		MpdPlay(-1);
	}
	else
	{
		printf(LIGHT_GREEN"[Start Play..]\n"NONE);
		MpdClear();
		/*if (1 != byFlag)
		{
			MpdVolume(200);
		}
		byFlag = 0;*/

		MpdAdd(g.CurrentPlayDirective.m_PlayUrl);
		MpdRunSeek(0, iSeekSec);
	}

	SetPlayStatus(AUDIO_STATUS_PLAY);
	//WriteConfigValueString("playtoken", g.CurrentPlayDirective.m_PlayToken);

	memcpy(g.m_mpd_status.user_state, "PLAYING", strlen("PLAYING"));

}

static int ClearAudioPlayList(char byFlag)
{
	char byClearFlag = byFlag;

	if (NULL == g.play_directive) 
	{
		DEBUG_PRINTF("<<<<<<<<<<<<<<< list_new");
		g.play_directive = list_new();
	}
	else if (NULL != g.play_directive)
	{
		list_iterator_t *it = NULL ;
		PLAY_DIRECTIVE_STRUCT *playDirective = NULL;
		DEBUG_PRINTF("<<<<<<<<<<<<<<< list_length:%d", list_length(g.play_directive));
		it = list_iterator_new(g.play_directive,LIST_HEAD);
		while(list_iterator_has_next(it))  // 从链表中读出数据，这里并没有删除，后面需要自己删除
		{
			DEBUG_PRINTF("<<<<<<<<<<<<<<< list_length:%d", list_length(g.play_directive));
			playDirective =  list_iterator_next_value(it);
			DEBUG_PRINTF("<<<<<<<<<<<<<<< byClearFlag:%d", byClearFlag);
			if(playDirective != NULL) 
			{
				if (1 != byClearFlag)
				{
					// 删除数据
					//DEBUG_PRINTF("<<<<<<<<<<<<<<< 2222222222222222222");
					list_node_t * node_t = list_find(g.play_directive, playDirective);
					//DEBUG_PRINTF("<<<<<<<<<<<<<<< 3333333333333333333");
					if (node_t != NULL)
					{
						//DEBUG_PRINTF("<<<<<<<<<<<<<<< 4444444444444444");
						list_remove(g.play_directive, node_t);
						if(playDirective != NULL)
						{
							if (playDirective->m_PlayUrl)
							{
								//DEBUG_PRINTF("<<<<<<<<<<<<<<< 555555555555555");
								free(playDirective->m_PlayUrl);
								//DEBUG_PRINTF("<<<<<<<<<<<<<<< 66666666666666");
								playDirective->m_PlayUrl = NULL;
								//DEBUG_PRINTF("<<<<<<<<<<<<<<< 777777777777777");
							}
							if (playDirective->m_PlayToken)
							{
								//DEBUG_PRINTF("<<<<<<<<<<<<<<< 555555555555555");
								free(playDirective->m_PlayToken);
								//DEBUG_PRINTF("<<<<<<<<<<<<<<< 6666666666666");
								playDirective->m_PlayToken = NULL;
								//DEBUG_PRINTF("<<<<<<<<<<<<<<< 77777777777777777");
							}
							free(playDirective);
							//DEBUG_PRINTF("<<<<<<<<<<<<<<< 8888888888888888");
							playDirective = NULL;
						}
					}
				}
				else
				{
					byClearFlag = 0;
				}
			}
		}
		list_iterator_destroy(it);		
	}
}


static int AddAudioPlayToList(PLAY_DIRECTIVE_STRUCT *playDirective)
{
	list_add(g.play_directive, playDirective);
}


static int ProcessAudioPlayerDirective(char *pData)
{
	char *pTempName = NULL;
	char *pTempPlayToken = NULL;
	char *playQueueCommand = NULL;
	char byChange = 0;

	struct json_object *root = NULL, *directive = NULL, *header = NULL, *Name = NULL, *payload = NULL, *audioItem = NULL, *audioItemId = NULL, *playBehavior = NULL;
	struct json_object *stream = NULL, *token = NULL, *url = NULL, *offsetInMilliseconds = NULL;
	struct json_object *progressReport = NULL, *delayInMilliseconds = NULL, *intervalInMilliseconds = NULL;

	DEBUG_PRINTF("ProcessAudioPlayerDirective....");

	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		DEBUG_PRINTF("json_tokener_parse root failed");
		return NULL;
	}

	directive = json_object_object_get(root, "directive");
	if(directive == NULL) 
	{
		DEBUG_PRINTF("json_object_object_get directive failed");
		return NULL;
	}

	header = json_object_object_get(directive, "header");
	if(header == NULL) 
	{				
		DEBUG_PRINTF("json_object_object_get header failed");
		return NULL;			
	}

	Name = json_object_object_get(header, "name");
	pTempName = strdup(json_object_get_string(Name));

	if (!strncmp(pTempName, AUDIOPLAYER_PLAY, (sizeof(AUDIOPLAYER_PLAY)-1)))
	{
		payload = json_object_object_get(directive, "payload");
		if(payload == NULL) 
		{				
			DEBUG_PRINTF("json_object_object_get header failed");
			return NULL;			
		}

		audioItem = json_object_object_get(payload, "audioItem");
		audioItemId = json_object_object_get(audioItem, "audioItemId");
		char *pTempStr = strdup(json_object_get_string(audioItemId));
		printf(LIGHT_RED "audioItemId:%s\n"NONE, pTempStr);
		if ((NULL != strstr(pTempStr, "ebotts")) || (NULL != strstr(pTempStr, "Audible")))
		{
			// 播放的是书
			printf(YELLOW "SetAmazonPlayBookFlag\n"NONE);
			SetAmazonPlayBookFlag(1);
		}
		else
		{
			SetAmazonPlayBookFlag(0);
		}

		if (pTempStr)
		{
			free(pTempStr);
			pTempStr = NULL;
		}

		stream = json_object_object_get(audioItem, "stream");

		progressReport = json_object_object_get(stream, "progressReport");
		if(!is_error(progressReport))
		{
			printf(LIGHT_RED "progressReport is not equal NULL\n"NONE);
			delayInMilliseconds = json_object_object_get(progressReport, "progressReportDelayInMilliseconds");			
			intervalInMilliseconds = json_object_object_get(progressReport, "progressReportIntervalInMilliseconds");
		}

		// 播放指令，获取需要播放的信息
		if (!strncmp(pTempName, AUDIOPLAYER_PLAY, (sizeof(AUDIOPLAYER_PLAY)-1)))
		{
			token                = json_object_object_get(stream, "token");
			url                  = json_object_object_get(stream, "url");
			offsetInMilliseconds = json_object_object_get(stream, "offsetInMilliseconds");
			playBehavior         = json_object_object_get(payload, "playBehavior");
			playQueueCommand     = strdup(json_object_get_string(playBehavior));		

			PLAY_DIRECTIVE_STRUCT *PlayDirective = (PLAY_DIRECTIVE_STRUCT *)malloc(sizeof(PLAY_DIRECTIVE_STRUCT));
			PlayDirective->m_PlayUrl			= strdup(json_object_get_string(url));
			PlayDirective->m_PlayToken			= strdup(json_object_get_string(token));
			PlayDirective->offsetInMilliseconds = json_object_get_int(offsetInMilliseconds);

			printf(LIGHT_RED "offsetInMilliseconds == %d\n"NONE,PlayDirective->offsetInMilliseconds);

			if (g.CurrentPlayDirective.m_PlayToken != NULL && g.CurrentPlayDirective.m_PlayUrl != NULL)
			{
				if ((0 == strcmp(g.CurrentPlayDirective.m_PlayToken, PlayDirective->m_PlayToken)) && 
					(0 == strcmp(g.CurrentPlayDirective.m_PlayUrl, PlayDirective->m_PlayUrl)))
				{
					if (PlayDirective->offsetInMilliseconds)
					{
						byChange = 1;
					}
				}
			}

			// 获取delayInMilliseconds和intervalInMilliseconds
			if(!is_error(delayInMilliseconds))
			{
				PlayDirective->delayInMilliseconds = json_object_get_int(delayInMilliseconds);
				g.CurrentPlayDirective.delayInMilliseconds = PlayDirective->delayInMilliseconds;
				printf(LIGHT_RED "delayInMilliseconds is not equal NULL, delayInMilliseconds == %d\n"NONE,PlayDirective->delayInMilliseconds);
			}
			else
			{
				g.CurrentPlayDirective.delayInMilliseconds = 0;
				PlayDirective->delayInMilliseconds = 0;
				printf(LIGHT_RED"delayInMilliseconds is %d\n"NONE, g.CurrentPlayDirective.delayInMilliseconds);
			}

			if(!is_error(intervalInMilliseconds))
			{
				PlayDirective->intervalInMilliseconds = json_object_get_int(intervalInMilliseconds);
				g.CurrentPlayDirective.intervalInMilliseconds = PlayDirective->intervalInMilliseconds;
				printf(LIGHT_RED "intervalInMilliseconds is not equal NULL, intervalInMilliseconds ==%d\n"NONE,PlayDirective->intervalInMilliseconds);
			}
			else
			{
				g.CurrentPlayDirective.intervalInMilliseconds = 0;
				PlayDirective->intervalInMilliseconds = 0;
				printf(LIGHT_RED"intervalInMilliseconds is %d\n"NONE, g.CurrentPlayDirective.intervalInMilliseconds);
			}

			/*
			 立即开始播放Play指令返回的流，并替换当前和入队流。
			 当流播放时，您收到一个Play指令playBehavior，REPLACE_ALL您必须向PlaybackStoppedAVS 发送一个事件。
			*/
			if (!strncmp(playQueueCommand, REPLACE_ALL, (sizeof(REPLACE_ALL)-1)))
			{
				DEBUG_PRINTF("--------------------------REPLACE_ALL");

				PlayDirective->byPlayType = TYPE_REPLACE_ALL;

				// 收到REPLACE_ALL指令，先判断当前是否在播放音乐，如果在播放需要先停止播放，发送STOP事件
				if (MPDS_PLAY == MpdGetPlayState())
				{
					MpdPause();
					Queue_Put(ALEXA_EVENT_PLAYBACK_STOP);
				}

				SetPlayStatus(AUDIO_STATUS_STOP);

				// 清除所有链表
				ClearAudioPlayList(0);

				// 将当前audio信息加入链表，并开始播放此流
				AddAudioPlayToList(PlayDirective);

				printf(LIGHT_RED"<<<<<<<<<<<<<<< list_length:%d\n"NONE, list_length(g.play_directive));
				g.CurrentPlayDirective.m_PlayUrl            = strdup(json_object_get_string(url));
				g.CurrentPlayDirective.m_PlayToken          = strdup(json_object_get_string(token));
				g.CurrentPlayDirective.offsetInMilliseconds = json_object_get_int(offsetInMilliseconds);

				// 如果为cid的话，则直接结束这次解析，不再往后解析
				if (NULL != strstr(g.CurrentPlayDirective.m_PlayUrl, "cid:"))
				{
					printf(LIGHT_RED "Url is cid, break\n" NONE);
				
					g.m_PlayMusicFlag = 1;
					if (g_DecodeAudioFifo == NULL)
					{
						g_DecodeAudioFifo = ft_fifo_alloc(DECODE_FIFO_BUFFLTH);
						
						DEBUG_PRINTF("[ProcessSpeechSynthesizerDirective]:%d", g.m_AudioFlag);
						sem_post(&g.Semaphore.MadPlay_sem);
					}
					goto end;
				}

				DEBUG_PRINTF("------------g.m_AudioFlag:%d", g.m_AudioFlag);
				if (g.m_AudioFlag != 0)
				{
					DEBUG_PRINTF("[000000] -> [play].");
				
					// 有语音指令，先播放语音指令再播放音乐
					g.m_AudioFlag = 0;
					g.m_PlayMusicFlag = 1;
				}
				else
				{
					playMusic(byChange);
				}
			}
			// 将流添加到当前队列的末尾
			else if (!strncmp(playQueueCommand, ENQUEUE, (sizeof(ENQUEUE)-1)))
			{
				DEBUG_PRINTF("--------------------ENQUEUE");
				PlayDirective->byPlayType = TYPE_ENQUEUE;
				if (NULL == g.play_directive) 
				{
					g.play_directive = list_new();
					DEBUG_PRINTF("--------------------list_new:%d", list_length(g.play_directive));
					list_add(g.play_directive, PlayDirective);
					printf(LIGHT_BLUE"list length:%d\n"NONE, list_length(g.play_directive));
					// 链表为空，直接播放
					g.CurrentPlayDirective.m_PlayUrl			= strdup(json_object_get_string(url));
					g.CurrentPlayDirective.m_PlayToken			= strdup(json_object_get_string(token));
					g.CurrentPlayDirective.offsetInMilliseconds = json_object_get_int(offsetInMilliseconds);

					// 如果为cid的话，则直接结束这次解析，不再往后解析
					if (NULL != strstr(g.CurrentPlayDirective.m_PlayUrl, "cid:"))
					{
						printf(LIGHT_RED "Url is cid, break\n" NONE);
					
						g.m_PlayMusicFlag = 1;
						if (g_DecodeAudioFifo == NULL)
						{
							g_DecodeAudioFifo = ft_fifo_alloc(DECODE_FIFO_BUFFLTH);

							DEBUG_PRINTF("[ProcessSpeechSynthesizerDirective]:%d", g.m_AudioFlag);
							sem_post(&g.Semaphore.MadPlay_sem);
						}
						goto end;
					}

					playMusic(byChange);
				}
				else
				{
					// 判断里面是否有音乐，没有的话直接播放，
					// 有的话放入链表并判断当前是否播放音乐，没有播放音乐的话则从队列中取出音乐开始播放
					// 有音乐则判断是否为当前播放音乐，是的话则删除并继续
					if (list_length(g.play_directive))
					{
						DEBUG_PRINTF("AAAAAAAAAAA------list_add......playStatus:%s", g.m_mpd_status.mpd_state);
						list_add(g.play_directive, PlayDirective);

						printf(LIGHT_BLUE"list length:%d\n"NONE, list_length(g.play_directive));

						if (0 == strcmp(g.m_mpd_status.mpd_state, "FINISHED"))
						{
							// 从队列中取出音乐信息，并开始播放
							list_iterator_t *it = NULL ;
							PLAY_DIRECTIVE_STRUCT *playDirective = NULL;
							it = list_iterator_new(g.play_directive,LIST_HEAD);
							while(list_iterator_has_next(it))  // 从链表中读出数据，这里并没有删除，后面需要自己删除
							{
								playDirective =  list_iterator_next_value(it);
								DEBUG_PRINTF("<<<<<<<<<<<<<<< 2222222222222222");
								if(playDirective != NULL) 
								{
									if (!strcmp(g.CurrentPlayDirective.m_PlayUrl, playDirective->m_PlayToken))
									{
										DEBUG_PRINTF("<<<<<<<<<<<<<<< 5555555555555555555");
										list_node_t * node_t = list_find(g.play_directive, playDirective);
										if (node_t != NULL)
											list_remove(g.play_directive, node_t);

										DEBUG_PRINTF("<<<<<<<<<<<<<<< list_remove");
										// 删除数据
										if(playDirective != NULL)
										{
											if (playDirective->m_PlayUrl)
												free(playDirective->m_PlayUrl);
											if (playDirective->m_PlayToken)
												free(playDirective->m_PlayToken);
											free(playDirective);
											playDirective = NULL;
										}
										DEBUG_PRINTF("<<<<<<<<<<<<<<< list_remove date");
										continue;
									}
								
									if (g.CurrentPlayDirective.m_PlayUrl != NULL)
									{
										free(g.CurrentPlayDirective.m_PlayUrl);
										g.CurrentPlayDirective.m_PlayUrl = NULL;
									}
									if (g.CurrentPlayDirective.m_PlayToken != NULL)
									{
										free(g.CurrentPlayDirective.m_PlayToken);
										g.CurrentPlayDirective.m_PlayToken = NULL;
									}
						
									DEBUG_PRINTF("<<<<<<<<<<<<<<< 7777777777777777777");
									g.CurrentPlayDirective.m_PlayToken			  = strdup(playDirective->m_PlayToken);
									g.CurrentPlayDirective.m_PlayUrl			  = strdup(playDirective->m_PlayUrl);
									g.CurrentPlayDirective.offsetInMilliseconds   = playDirective->offsetInMilliseconds;
									g.CurrentPlayDirective.delayInMilliseconds	  = playDirective->delayInMilliseconds;
									g.CurrentPlayDirective.intervalInMilliseconds = playDirective->intervalInMilliseconds;
									g.CurrentPlayDirective.byPlayType             = playDirective->byPlayType;

									playMusic(byChange);
									break;
								}
							}

							DEBUG_PRINTF("all list length : %d ", list_length(g.play_directive));
							list_iterator_destroy(it);
						}
					}
					else
					{
						DEBUG_PRINTF("BBBBBBBBBBBB--------list_add......");
						list_add(g.play_directive, PlayDirective);
						printf(LIGHT_BLUE"list length:%d\n"NONE, list_length(g.play_directive));
						g.CurrentPlayDirective.m_PlayUrl			= strdup(json_object_get_string(url));
						g.CurrentPlayDirective.m_PlayToken			= strdup(json_object_get_string(token));
						g.CurrentPlayDirective.offsetInMilliseconds = json_object_get_int(offsetInMilliseconds);
						// 如果为cid的话，则直接结束这次解析，不再往后解析
						if (NULL != strstr(g.CurrentPlayDirective.m_PlayUrl, "cid:"))
						{
							printf(LIGHT_RED "Url is cid, break\n" NONE);

							g.m_PlayMusicFlag = 1;
							if (g_DecodeAudioFifo == NULL)
							{
								g_DecodeAudioFifo = ft_fifo_alloc(DECODE_FIFO_BUFFLTH);
								
								DEBUG_PRINTF("[ProcessSpeechSynthesizerDirective]:%d", g.m_AudioFlag);
								sem_post(&g.Semaphore.MadPlay_sem);
							}
							goto end;
						}

						playMusic(byChange);
					}
				}
			}
			// 替换队列中的所有流。这不影响当前的播放流。
			else if (!strncmp(playQueueCommand, REPLACE_ENQUEUED, (sizeof(REPLACE_ENQUEUED)-1)))
			{
				// 清除所有链表，并开始播放此流
				PlayDirective->byPlayType = TYPE_REPLACE_ENQUEUED;
				if (NULL == g.play_directive)
				{
					DEBUG_PRINTF("<<<<<<<<<<<<<<<< 000000000000000000");
					g.play_directive = list_new();
				}
				if (g.CurrentPlayDirective.m_PlayUrl)
				{
					printf(LIGHT_BLUE"g.CurrentPlayDirective.m_PlayUrl:%s\n"NONE, g.CurrentPlayDirective.m_PlayUrl);
					printf(LIGHT_BLUE"g.CurrentPlayDirective.m_PlayToken:%s\n"NONE, g.CurrentPlayDirective.m_PlayToken);
				}
				ClearAudioPlayList(1);

				list_add(g.play_directive, PlayDirective);
				DEBUG_PRINTF("<<<<<<<<<<<<<<<< 1111111111111111111");
				printf(LIGHT_BLUE"list length:%d\n"NONE, list_length(g.play_directive));
			}
		}
	}
	else if (!strncmp(pTempName, AUDIOPLAYER_STOP, (sizeof(AUDIOPLAYER_STOP)-1)))
	{
		if (alertmanager_has_activealerts(alertManager))
		{
			alertmanager_stop_active_alert(alertManager);
			OnWriteMsgToUartd(ALEXA_ALER_END);

			//MpdInit();
			MpdVolume(200);
			//MpdDeinit();
		}
		else
		{
			DEBUG_PRINTF("[Stop music]");

			g.m_PlayerStatus = AMAZON_AUDIOPLAYER_STOP;
			myexec_cmd("mpc pause && mpc volume 200");
			OnWriteMsgToUartd(ALEXA_VOICE_PLAY_END);

			memcpy(g.m_mpd_status.user_state, "STOPPED", strlen("STOPPED"));
			memcpy(g.m_mpd_status.mpd_state, "STOPPED", strlen("STOPPED"));

			Queue_Put(ALEXA_EVENT_PLAYBACK_STOP);

			if (1 == GetAmazonPlayBookFlag())
			{
				// 如果播放的是书，暂停播放，并发送STOP指令
				printf(YELLOW "SetAmazonPlayBookFlag 0---\n"NONE);
				SetAmazonPlayBookFlag(0);
			}
		}
	}
	else if (!strncmp(pTempName, AUDIOPLAYRT_CLEARQUEUE, (sizeof(AUDIOPLAYRT_CLEARQUEUE)-1)))
	{
		// clear queue

		// CLEAR_ENQUEUED它清除队列并继续播放当前的播放流; 
		// CLEAR_ALL清除整个播放队列并停止当前播放的流（如果适用）
		if (NULL != strstr(pData, "CLEAR_ENQUEUED"))
		{
			ClearAudioPlayList(1);
		}
		else if (NULL != strstr(pData, "CLEAR_ALL"))
		{
			// 清除所有链表
			ClearAudioPlayList(0);
			if (MPDS_PLAY == MpdGetPlayState())
			{
				MpdPause();
				Queue_Put(ALEXA_EVENT_PLAYBACK_STOP);
			}

		}

		Queue_Put(ALEXA_EVENT_PLAYBACK_QUEUECLEAR);
	}
	else
	{
		DEBUG_PRINTF("Not supported command :%s", pTempName);
	}

end:
	json_object_put(root);	

	if (pTempPlayToken)
	{
		free(pTempPlayToken);
		pTempPlayToken = NULL;
	}

	if (pTempName)
	{
		free(pTempName);
		pTempName = NULL;
	}

	return 0;
}


static int ProcessSpeechRecognizerDirective(char *pData)
{
	int iRet = -1;

	char *pName = NULL;

	pName = strstr(pData, "\"name\":\"ExpectSpeech\"");
	if (pName != NULL)
	{
		g.m_ExpectSpeechFlag = 1;
		DEBUG_PRINTF("ProcessSpeechRecognizerDirective:%d", g.m_ExpectSpeechFlag);
	    iRet = 0;
	}
	else
	{
		pName = strstr(pData, "\"name\":\"StopCapture\"");
		if (pName != NULL)
		{
			DEBUG_PRINTF("StopCapture...");

			SetVADFlag(RECORD_VAD_END);
			g.m_RemoteVadFlag = REMOTE_VAD_END;
			OnWriteMsgToUartd(ALEXA_CAPTURE_END);

		    iRet = 0;
		}
	}

	return iRet;
}

static void del_chr( char *s, char ch )
{
    char *t=s; 
    while( *s != '\0' )
    {
        if ( *s != ch )
            *t++=*s;
        s++ ; 
    }
    *t='\0';
}

static char *GetDirectiveNameSpace(char *pData)
{
	char *pDirectiveNamespace = NULL;
	struct json_object *root = NULL, *directive = NULL, *header = NULL, *Name = NULL;

	del_chr(pData, '\r');
	del_chr(pData, '\n');

	//printf("GetDirectiveNameSpace....:%s", pData);
	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		DEBUG_PRINTF("json_tokener_parse root failed");
		return NULL;
	}
	directive = json_object_object_get(root, "directive");
	if(directive == NULL) 
	{
		DEBUG_PRINTF("json_object_object_get directive failed");
		return NULL;
	}

	header = json_object_object_get(directive, "header");
	if(header == NULL) 
	{				
		DEBUG_PRINTF("json_object_object_get header failed");
		return NULL;			
	}

	Name = json_object_object_get(header, "namespace");
	pDirectiveNamespace = strdup(json_object_get_string(Name));

	json_object_put(Name);
	json_object_put(header);
	json_object_put(directive);	
	json_object_put(root);	

	DEBUG_PRINTF("pDirectiveNamespace:%s", pDirectiveNamespace);

	return pDirectiveNamespace;
}

static int ProcessJsonData(char *pData)
{

	int iRet = -1;
	char *pNamespace = NULL;

	pNamespace = GetDirectiveNameSpace(pData);

	if (!strncmp(pNamespace, AMAZON_SPEECHRECOGNIZER, (sizeof(AMAZON_SPEECHRECOGNIZER)-1)))
	{
		// SpeechRecognizer Interface
		iRet = ProcessSpeechRecognizerDirective(pData);
	}
	else if (!strncmp(pNamespace, AMAZON_SPEECHSYNTHESIZER, (sizeof(AMAZON_SPEECHSYNTHESIZER)-1)))
	{
		// SpeechSynthesizer Interface
		iRet = ProcessSpeechSynthesizerDirective(pData);
	}
	else if (!strncmp(pNamespace, AMAZON_AUDIOPLAYER, (sizeof(AMAZON_AUDIOPLAYER)-1)))
	{
		// AudioPlayer Interface
		iRet = ProcessAudioPlayerDirective(pData);
	}
	else if (!strncmp(pNamespace, AMAZON_SPEAKER, (sizeof(AMAZON_SPEAKER)-1)))
	{
		// Speaker Interface
		iRet = ProcessSpeakerDirective(pData);
	}
	else if (!strncmp(pNamespace, AMAZON_ALERTS, (sizeof(AMAZON_ALERTS)-1)))
	{
		// Alerts Interface
		iRet = ProcessAlertDirective(pData);
	}
	else if (!strncmp(pNamespace, AMAZON_SYSTEM, (sizeof(AMAZON_SYSTEM)-1)))
	{
		// System Interface
		iRet = ProcessSystemDirective(pData);
	}
	else if (!strncmp(pNamespace, AMAZON_NOTIFICATIONS, (sizeof(AMAZON_NOTIFICATIONS)-1)))
	{
		// Notifications Interface
		DEBUG_PRINTF("ttttthhhhheeee   NNNoootttiiifffiiicccaaatttiiiooonnnsss");
		iRet = ProcessNotificationsDirective(pData);
	}
	else 
	{
		DEBUG_PRINTF("pData:%s", pData);
	}

	if(pNamespace != NULL)
	{
		free(pNamespace);
	}

	return iRet;
}


int ProcessAmazonDirective(char *pData, int iSize)
{
	int iRet = -1;
	char *pName = NULL;
	char *line = NULL;
	char *byTempBuf = NULL;
	char byFlag = 0;


	pthread_mutex_lock(&g.Handle_mtc);

	pName = strstr(pData, "directive");
	if (NULL != pName)
	{
		if (NULL == byTempBuf)
		{
			byTempBuf = (char *)malloc(iSize);
		}

		line = get_line(byTempBuf, pData);
		while (1)
		{
			bzero(byTempBuf, iSize);
			line = get_line(byTempBuf, line);
			if (NULL == line)
			{
				DEBUG_PRINTF("don't have directive..");

				if (g.m_AmazonMuteOldStatus != g.m_AmazonMuteStatus)
				{
					g.m_AmazonMuteStatus = g.m_AmazonMuteOldStatus;
					DEBUG_PRINTF("Send MuteChanged event:%d", g.m_AmazonMuteOldStatus);
					Queue_Put(ALEXA_EVENT_MUTECHANGE);
				
					if (g.m_AudioFlag != 1)
					{
						if (0 == g.m_AmazonMuteStatus)
						{
							// Mute off
							DEBUG_PRINTF("Mute Off...");
							myexec_cmd("mpc volume 200");
						}
						else if (1 == g.m_AmazonMuteStatus)
						{
							// Mute On
							DEBUG_PRINTF("Mute On...");
							myexec_cmd("mpc volume 101");
						}
						OnWriteMsgToUartd(ALEXA_VOICE_PLAY_END);
					}
				}
				pthread_mutex_unlock(&g.Handle_mtc);

				if (byTempBuf)
					free(byTempBuf);

				return iRet;
			}
			else if (NULL != strstr(byTempBuf, "directive"))
			{
				//DEBUG_PRINTF(LIGHT_CYAN "[ProcessAmazonDirective line] = %s\n" NONE, byTempBuf);

				if (DEBUG_ON == g_byDebugFlag)
					printf(LIGHT_CYAN "[ProcessAmazonDirective line] = %s\n" NONE, byTempBuf);

				ProcessJsonData(byTempBuf);
				iRet = 0;
			}
			/*else if (NULL != strstr(byTempBuf, "Content-Type: application/octet-stream"))
			{
				byFlag = 1;
			}
			else if (1 == byFlag)
			{
				if (g_DecodeAudioFifo != NULL)
				{
					ft_fifo_put(g_DecodeAudioFifo, byTempBuf, strlen(byTempBuf));
				}
			}*/
		}
	}
	else
	{
		//DEBUG_PRINTF("ERROR:[%s]", pData);

		// 需要重新获取token
		// 需要重新获取token
		char *pStrstr = strstr(pData,"Please provide a valid authorization token");

		//DEBUG_PRINTF("str :     //////////  %s",pStrstr);
		if( NULL != pStrstr)
		{
			DEBUG_PRINTF("Refresh_AccessToken...");
			if(0 == OnRefreshToken())
			{
				//g_byGetTokenStatus = 2;
				DEBUG_PRINTF("Refresh_AccessToken...");
			}
		}
	}

	if (byTempBuf)
		free(byTempBuf);

	pthread_mutex_unlock(&g.Handle_mtc);

	return iRet;
}

