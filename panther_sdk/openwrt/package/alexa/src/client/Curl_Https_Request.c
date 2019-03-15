#include "globalParam.h"
#include <curl/curl.h>
#include "alert/AlertHead.h"

extern UINT8 g_byDebugFlag;
extern UINT8 g_bysubmitToken[];

extern GLOBALPRARM_STRU g;
extern char g_byWakeUpFlag ;

extern FT_FIFO * g_CaptureFifo;

extern FT_FIFO * g_DecodeAudioFifo;

extern char g_bySynStatusIndex;

extern UINT8 g_byAmazonLogInFlag;

extern 	struct	timeval begin_tv;

extern AlertManager *alertManager;

extern char g_byDeleteAlertFlag;

extern int playMusic(char byChange);

pthread_t CurlHttpsRequest_Pthread;


#define MULTI_PART_CONTENT_TYPE_AUDIO "application/octet-stream"

#define BODY_FILE_NAME       "/tmp/BodyFile.mp3"
unsigned char g_byPlayFinishedRecFlag = 0;


FILE *saveBodyFile;

extern char PingPackageIndex;
extern FT_FIFO * g_DataFifo;

//extern static int MusciPlayFinishProcess(void);

int getToken(void)
{
	char byLength = strlen("Authorization:Bearer ");
	char *pTemp = NULL;
	
	pTemp = ReadConfigValueString("accessToken");
	if (pTemp != NULL)
	{
		memset(g_bysubmitToken, 0, 2560);
		sprintf(g_bysubmitToken, "Authorization:Bearer %s", pTemp);
		free(pTemp);
	}
	else
	{
		DEBUG_ERROR("g_bysubmitToken:%s", g_bysubmitToken);
	}		

	return 0;
}

static char find_IDLE_easy_handler(void)
{
	int i;
	for ( i = 2 ; i < MAX_CONNECT_STREM - 2; i++ )
	{
	    if (NULL == g.m_RequestBoby[i].easy_handle)
		{
			return i;
		}
	}

	return -1;
}

void Init_easy_handler(void)
{
	int i;
	g.multi_handle = NULL;
	for ( i = 0 ; i < 10; i++ )
	{
	    g.m_RequestBoby[i].easy_handle = NULL;
	}
}

void Clear_easy_handler(CURL * curl_handle)
{
	int i;

	pthread_mutex_lock(&g.Clear_easy_handler_mtx);

	if (NULL == curl_handle || NULL == g.multi_handle)
	{
		pthread_mutex_unlock(&g.Clear_easy_handler_mtx);
		return ;
	}

	curl_multi_remove_handle(g.multi_handle, curl_handle);
	curl_easy_cleanup(curl_handle);

	for (i = 0; i < MAX_CONNECT_STREM; i++)
	{
		if (curl_handle == g.m_RequestBoby[i].easy_handle)
		{
			g.m_RequestBoby[i].easy_handle = NULL;
			if (g.m_RequestBoby[i].BodyBuff != NULL)
			{
				free(g.m_RequestBoby[i].BodyBuff);
				g.m_RequestBoby[i].BodyBuff = NULL;
			}

		    if (g.m_RequestBoby[i].httpHeaderSlist)
	    	{
      			curl_slist_free_all(g.m_RequestBoby[i].httpHeaderSlist);
				g.m_RequestBoby[i].httpHeaderSlist = NULL;
	    	}

			if (g.m_RequestBoby[i].formpost != NULL)
			{
				curl_formfree(g.m_RequestBoby[i].formpost);
				g.m_RequestBoby[i].formpost = NULL;
			}
			break;
		}
	}

	g.m_RequestBoby[i].easy_handle = NULL;

    pthread_mutex_unlock(&g.Clear_easy_handler_mtx);

}

static void Clear_multi_handler(void)
{
	int i;
	for ( i = 0 ; i < MAX_CONNECT_STREM; i++ )
	{
	    if (NULL != g.m_RequestBoby[i].easy_handle)
		{
			curl_multi_remove_handle(g.multi_handle, g.m_RequestBoby[i].easy_handle);
			curl_easy_cleanup(g.m_RequestBoby[i].easy_handle); 
			g.m_RequestBoby[i].easy_handle = NULL;
		}
	}

	curl_multi_cleanup(g.multi_handle);
	g.multi_handle = NULL;
}


size_t SpeechRecognize_Header_callback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	if (13 == (nmemb * size))
	{
		if (NULL == strstr((char *)ptr, "HTTP/2 200"))
		{
			//DEBUG_PRINTF("%s", ptr);

			// 删除闹钟时这里可能会收到204
			if (1 == g_byDeleteAlertFlag)
			{
				g_byDeleteAlertFlag = 0;
				return (nmemb * size);
			}

			// 语音识别失败
			OnWriteMsgToUartd(ALEXA_IDENTIFY_ERR);
			if(alerthandler_get_state(alertManager->handler) != ALERT_FINISHED)
			{
				MpdVolume(140);
				alerthandler_resume_alerts(alertManager);
			}
			else
			{
			//alerthandler_resume_alerts(alertManager);
				myexec_cmd("aplay /root/res/stop_prompt.wav");
				MpdVolume(200);
			}
			if (1 == GetAmazonPlayBookFlag())
			{
				myexec_cmd("mpc play");
				Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);
			}
			
			if (1 == g_byWakeUpFlag)
			{
				g_byWakeUpFlag = 0;
				//printf(LIGHT_RED"OnWriteMsgToWakeup(start)\n"NONE);
				//OnWriteMsgToWakeup("start");
			}
		}
	}
	return (nmemb * size);
}

// write callback function
size_t SpeechRecognize_ProcessResponse(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	//pthread_mutex_lock(&g.Handle_mtc);

	static UINT8 byFlag = 0;

	if (0 == byFlag)
	{
		byFlag = 1;
		DEBUG_PRINTF("write Body data begin...nmemb:%d", nmemb * size);
		ProcessAmazonDirective((char *)ptr, (int)(nmemb * size));
	}

	if (g_DecodeAudioFifo != NULL)
	{
		DEBUG_PRINTF("write audio data begin...nmemb:%d", nmemb * size);
		ft_fifo_put(g_DecodeAudioFifo, (char *)ptr, nmemb * size);
	}

	if ((nmemb * size) < 16384)
	{
		byFlag = 0;
		DEBUG_PRINTF("write Body data end...nmemb:%d", nmemb * size);
	}

	//int written = fwrite(ptr, size, nmemb, (FILE *)stream);  
	//fflush((FILE *)stream);
	
	//pthread_mutex_unlock(&g.Handle_mtc);
	return (nmemb * size);
}


// write callback function
size_t SpeechFinish_ProcessResponse(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	static UINT8 byFlag = 0;

	if (AUDIO_REQUEST_IDLE == GetAudioRequestStatus())
	{
		if (0 == byFlag)
		{
			byFlag = 1;
			DEBUG_PRINTF("write Body data begin...nmemb:%d", nmemb * size);
			ProcessAmazonDirective((char *)ptr, (int)(nmemb * size));
		}

		if (g_DecodeAudioFifo != NULL)
		{
			DEBUG_PRINTF("write audio data begin...nmemb:%d", nmemb * size);
			ft_fifo_put(g_DecodeAudioFifo, (char *)ptr, nmemb * size);
		}

		if ((nmemb * size) < 16384)
		{
			byFlag = 0;
			DEBUG_PRINTF("write Body data end...nmemb:%d", nmemb * size);
		}

		//int written = fwrite(ptr, size, nmemb, (FILE *)stream);  
		//fflush((FILE *)stream);
	}
	return (nmemb * size);

}




/*size_t PlaybackFailed_ProcessResponse(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	static UINT8 byFlag = 0;

	if (0 == byFlag)
	{
		byFlag = 1;
		DEBUG_PRINTF("write Body data begin...nmemb:%d", nmemb * size);
		ProcessAmazonDirective((char *)ptr, (int)(nmemb * size));
	}

	if (g_DecodeAudioFifo != NULL)
	{
		ft_fifo_put(g_DecodeAudioFifo, (char *)ptr, nmemb * size);
	}

	if ((nmemb * size) < 16384)
	{
		byFlag = 0;
		DEBUG_PRINTF("write Body data end...nmemb:%d", nmemb * size);
	}

	int written = fwrite(ptr, size, nmemb, (FILE *)stream);  
	fflush((FILE *)stream);
	return (nmemb * size);
}*/


// write callback function
size_t PlaybackController_ProcessResponse(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	ProcessAmazonDirective((char *)ptr, (int)(nmemb * size));

	return (nmemb * size);
}

static int ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)    
{    
	if(GetCallbackStatus()) {
		SetCallbackStatus(0);
		DEBUG_ERROR("http canled...");
		return -1;
	}
    return 0;    
} 


// read callback function for sending audio
size_t SpeechRecognize_DataReader(char *buffer, size_t size, size_t nitems, void *instream)
{
	static int iCount = 0;
	static char byFlag = 0;

	size_t nread = 0;

	size_t length = (size * nitems);

	//size_t length = 320;
	
	SetAudioRequestStatus(AUDIO_REQUEST_UPDATE);

	if (NULL == g_CaptureFifo)
	{
		printf(LIGHT_RED"g_CaptureFifo is null, return zero..\n"NONE);
		return 0;
	}

	// 收到停止录音，后判断本地录音是否结束，结束的话则结束此次上传
	if (REMOTE_VAD_END == g.m_RemoteVadFlag)
	{
		if (g.m_CaptureFlag == 3)
		{
			DEBUG_PRINTF("REMOTE_VAD_END ASR data upload completed ..");
			/*if (g_CaptureFifo != NULL)
			{	
				DEBUG_PRINTF("ft_fifo_free(g_CaptureFifo)");
				ft_fifo_free(g_CaptureFifo);
				g_CaptureFifo = NULL;
			}*/
			return nread;
		}
	}

	while (1)
	{
		nread = (size_t)ft_fifo_getlenth(g_CaptureFifo);
		if (nread > 0)
		{
			if (nread > length)
			{
			 	nread = length;
			}

			//DEBUG_PRINTF("[INFO] C -------------------> S (DATA post body), Readlength:%d, Targetlength:%d",nread, size * nitems);

			ft_fifo_seek(g_CaptureFifo, (char *)buffer, 0, nread);

			ft_fifo_setoffset(g_CaptureFifo, nread);

			break;
		}
		else if (g.m_CaptureFlag == 3)
		{
			DEBUG_PRINTF("ASR data upload completed ..");
			/*if (g_CaptureFifo != NULL)
			{	
				DEBUG_PRINTF("ft_fifo_free(g_CaptureFifo)");
				ft_fifo_free(g_CaptureFifo);
				g_CaptureFifo = NULL;
			}*/
			break;
		}
	}
	//return CURL_READFUNC_PAUSE;

	return nread;
}

size_t PlaybackNearlyFinished_Header_callback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	DEBUG_PRINTF("ilength:%d:%s", strlen((char *)ptr), ptr);

	if (13 == (nmemb * size))
	{
		if (NULL != strstr((char *)ptr, "HTTP/2 204"))
		{
			//cdb_set_int("$current_play_mode", 0);
			//SetPlayMode(0);
			//Queue_Put(ALEXA_EVENT_PLAYBACK_FINISHED);
			//DEBUG_PRINTF("%s", ptr);
			if (1 == GetAmazonPlayBookFlag())
			{
				SetAmazonPlayBookFlag(0);
			}
		}

		if (g.CurrentPlayDirective.m_PlayToken)
		{
			char *pData = strdup(g.CurrentPlayDirective.m_PlayToken);
			Queue_Put_Data(ALEXA_EVENT_PLAYBACK_FINISHED, pData);
		}

		//Queue_Put(ALEXA_EVENT_PLAYBACK_FINISHED);
	}
	return (nmemb * size);
}

// write callback function
size_t PlayBackNearFinished_ProcessResponse(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	static UINT8 byFlag = 0;

	if (0 == byFlag)
	{
		byFlag = 1;
		DEBUG_PRINTF("write Body data begin...nmemb:%d", nmemb * size);
		g_byPlayFinishedRecFlag = 0xFE;
		ProcessAmazonDirective((char *)ptr, (int)(nmemb * size));
		g_byPlayFinishedRecFlag = 0;
	}

	if (g_DecodeAudioFifo != NULL)
	{
		DEBUG_PRINTF("write audio data begin...nmemb:%d", nmemb * size);
		ft_fifo_put(g_DecodeAudioFifo, (char *)ptr, nmemb * size);
	}else{
//		g_DecodeAudioFifo = ft_fifo_alloc(DECODE_FIFO_BUFFLTH);
		
		DEBUG_PRINTF("decode audio fifo is NULL\n");
//		sem_post(&g.Semaphore.MadPlay_sem);

	}

	if ((nmemb * size) < 16384)
	{
		byFlag = 0;
		DEBUG_PRINTF("write Body data end...nmemb:%d", nmemb * size);
	}

	//int written = fwrite(ptr, size, nmemb, (FILE *)stream);  
	//fflush((FILE *)stream);
	return (nmemb * size);
}


size_t PlaybackFinished_Header_callback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	//DEBUG_PRINTF("ilength:%d:%s", strlen((char *)ptr), ptr);

	if (13 == (nmemb * size))
	{
		if (NULL != strstr((char *)ptr, "HTTP/2 204"))
		{
			//cdb_set_int("$current_play_mode", 0);
			//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "0");

			list_iterator_t *it = NULL ;
			PLAY_DIRECTIVE_STRUCT *playDirective = NULL;
			it = list_iterator_new(g.play_directive, LIST_HEAD);
			while(list_iterator_has_next(it))  // 从链表中读出数据，这里并没有删除，后面需要自己删除
			{
				playDirective =  list_iterator_next_value(it);
				DEBUG_PRINTF("<<<<<<<<<<<<<<< 2222222222222222");
				if(playDirective != NULL) 
				{
					if (0 == strcmp(g.CurrentPlayDirective.m_PlayToken, playDirective->m_PlayToken))
					{
						break;
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
					playMusic(0);
					break;
				}
			}

			DEBUG_PRINTF("all list length : %d ", list_length(g.play_directive));
			list_iterator_destroy(it);

		}
	}
	return (nmemb * size);
}

size_t PlayBackFinished_ProcessResponse(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	printf(LIGHT_GREEN"PlayBackFinished_ProcessResponse....\n"NONE);
	
	//ProcessAmazonDirective((char *)ptr, (int)(nmemb * size));
	return (nmemb * size);
}


/*size_t PlaybackFinished_Header_callback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	//DEBUG_PRINTF("ilength:%d:%s", strlen((char *)ptr), ptr);

	if (13 == (nmemb * size))
	{
		if (NULL == strstr((char *)ptr, "HTTP/2 204"))
		{
			MusciPlayFinishProcess();
		}
	}
	return (nmemb * size);
}*/


/*size_t PlaybackNearlyFinished_Chunk_callback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	DEBUG_PRINTF("PlaybackNearlyFinished_Chunk_callback：%s", ptr);
	if(ptr != NULL)
	{
		ProcessAmazonDirective((char *)ptr, (int)(nmemb * size));
	}
	return (nmemb * size);
}*/


#define SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
void submit_PingPackets(void) 
{
	CURLcode code;
	CURLMcode mcode;

	char byIndex = find_IDLE_easy_handler();

	if (byIndex > 0)
	{
		g.m_RequestBoby[byIndex].easy_handle = curl_easy_init();
		if (g.m_RequestBoby[byIndex].easy_handle == NULL) 
		{
			DEBUG_PRINTF("curl_easy_init() failed, :%d\n", byIndex);
			return ;
		}

		DEBUG_PRINTF("-----------submit_PingPackets------");

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, PING_PATH);

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_NOSIGNAL, 1L);

		getToken();

		g.m_RequestBoby[byIndex].httpHeaderSlist = NULL;
		g.m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[byIndex].httpHeaderSlist, g_bysubmitToken);

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTPHEADER, g.m_RequestBoby[byIndex].httpHeaderSlist);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		if (DEBUG_ON == g_byDebugFlag)
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_TIMEOUT, 10);

		DEBUG_PRINTF("Adding easy %p to multi %p\n", g.m_RequestBoby[byIndex].easy_handle, g.multi_handle);
		mcode = curl_multi_add_handle(g.multi_handle, g.m_RequestBoby[byIndex].easy_handle);
		if (mcode != CURLM_OK) 
		{
			DEBUG_PRINTF("curl_multi_add_handle() failed: %s",curl_multi_strerror(mcode));
			return ;
		}
		PingPackageIndex = byIndex;
	}
}

void submit_SpeechRecognizeEvent(void) 
{
	CURLcode code;
	CURLMcode mcode;

	g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle = curl_easy_init();
	if (g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle == NULL) 
	{
		DEBUG_PRINTF("curl_easy_init() failed\n");
		return ;
	}

	//saveBodyFile = fopen(BODY_FILE_NAME,  "wb+" );

	DEBUG_PRINTF("[Start].");

	if (g.endpoint.m_event_path)
		curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_URL, g.endpoint.m_event_path);
	else
		curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_URL, EVENTS_PATH);

	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_NOSIGNAL, 1L);

	if (DEBUG_ON == g_byDebugFlag)
		curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_VERBOSE, 1);

	getToken();

	// metadate
	g.m_RequestBoby[RECOGNIZER_INDEX].BodyBuff = GetSpeechRecognizeEventjsonData();

	g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist = NULL;
	g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist, "Content-Type: multipart/form-data");
	g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist, "Transfer-Encoding: chunked");
	g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist, g_bysubmitToken);

	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HTTPHEADER, g.m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist);
	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_TIMEOUT, 30);

	g.m_RequestBoby[RECOGNIZER_INDEX].formpost = NULL;
	g.m_RequestBoby[RECOGNIZER_INDEX].lastptr = NULL;

	curl_formadd(&g.m_RequestBoby[RECOGNIZER_INDEX].formpost,
				 &g.m_RequestBoby[RECOGNIZER_INDEX].lastptr,
				 CURLFORM_COPYNAME, "metadata",
				 CURLFORM_COPYCONTENTS, g.m_RequestBoby[RECOGNIZER_INDEX].BodyBuff,
				 CURLFORM_CONTENTTYPE, "application/json; charset=UTF-8",
				 CURLFORM_END);

	curl_formadd(&g.m_RequestBoby[RECOGNIZER_INDEX].formpost,
				 &g.m_RequestBoby[RECOGNIZER_INDEX].lastptr,
				 CURLFORM_COPYNAME, "audio",
				 CURLFORM_CONTENTTYPE, MULTI_PART_CONTENT_TYPE_AUDIO,
				 CURLFORM_STREAM, &g_CaptureFifo,
				 //CURLFORM_CONTENTSLENGTH, DATA_SIZE,
				 CURLFORM_END);

	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HTTPPOST, g.m_RequestBoby[RECOGNIZER_INDEX].formpost);

	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HEADERFUNCTION, SpeechRecognize_Header_callback);
    curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_WRITEFUNCTION, SpeechRecognize_ProcessResponse);
	//curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_WRITEDATA, saveBodyFile );	

	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_READFUNCTION, SpeechRecognize_DataReader);
	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_NOPROGRESS, false);
	curl_easy_setopt(g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);


	DEBUG_PRINTF("[submit_SpeechRecognizeEvent] easy_handle:%p", g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle);
	
	SetAudioRequestStatus(AUDIO_REQUEST_START);

	mcode = curl_multi_add_handle(g.multi_handle, g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle);
	if (mcode != CURLM_OK) 
	{
		DEBUG_PRINTF("curl_multi_add_handle() failed: %s，mcode:%d", curl_multi_strerror(mcode), mcode);
		return ;
	}
}

/*size_t CurlRequest_ChunkData_callback(void *buffer,size_t size,size_t count,void **response)  
{
	char * ptr = buffer;

	//DEBUG_PRINTF("rec: %s",ptr);  

	ProcessAmazonDirective((char *)ptr, (int)(count * size));

	return count;  
}*/

#ifdef USE_EVENT_QUEUE
void submit_Alexa_AlertEvent(int iEventName, char *data)
{
	CURLcode code;
	CURLMcode mcode;
	char *pBoundary = NULL;
	char content_type[256] = {0};
	char content_type_tmp[128] = {0};

	char byIndex = find_IDLE_easy_handler();

	if (byIndex > 0)
	{
		g.m_RequestBoby[byIndex].easy_handle = curl_easy_init();
		if (g.m_RequestBoby[byIndex].easy_handle == NULL) 
		{
			DEBUG_PRINTF("curl_easy_init() failed, :%d\n", byIndex);
			return ;
		}

		getToken();

		if (g.endpoint.m_event_path)
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, g.endpoint.m_event_path);
		else
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, EVENTS_PATH);

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

		if (DEBUG_ON == g_byDebugFlag)
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 1);

		pBoundary = GetBoundary(content_type_tmp, "multipart/form-data; boundary=");

		sprintf(content_type,"Content-type: %s", content_type_tmp);
		g.m_RequestBoby[byIndex].httpHeaderSlist = NULL;
		g.m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[byIndex].httpHeaderSlist, content_type);
		g.m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[byIndex].httpHeaderSlist, g_bysubmitToken);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTPHEADER, g.m_RequestBoby[byIndex].httpHeaderSlist);

		// Data
		if (1 == GetAmazonConnectStatus())
		{	
			g.m_RequestBoby[byIndex].BodyBuff = GetAlertEventJosnData(pBoundary, data);	
		}

		if (pBoundary)
			free(pBoundary);
		if (NULL == g.m_RequestBoby[byIndex].BodyBuff)
		{
			curl_easy_cleanup(g.m_RequestBoby[byIndex].easy_handle);
			DEBUG_PRINTF("submit_SynchronizeState malloc failed..");
			return;
		}

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDS, g.m_RequestBoby[byIndex].BodyBuff);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDSIZE, strlen(g.m_RequestBoby[byIndex].BodyBuff));

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_TIMEOUT, 10); 
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, SpeechRecognize_ProcessResponse);

		DEBUG_PRINTF("Adding easy %p to multi %p\n", g.m_RequestBoby[byIndex].easy_handle, g.multi_handle);

		mcode = curl_multi_add_handle(g.multi_handle, g.m_RequestBoby[byIndex].easy_handle);
		if (mcode != CURLM_OK) 
		{
			DEBUG_PRINTF("curl_multi_add_handle() failed: %s",curl_multi_strerror(mcode));
			return ;
		}
	}
}

void submit_Alexa_PlayFinishedEvent(int iEventName, char *pData)
{
	CURLcode code;
	CURLMcode mcode;
	char *pBoundary = NULL;
	char content_type[256] = {0};
	char content_type_tmp[128] = {0};

	char byIndex = find_IDLE_easy_handler();

	if (byIndex > 0)
	{
		g.m_RequestBoby[byIndex].easy_handle = curl_easy_init();
		if (g.m_RequestBoby[byIndex].easy_handle == NULL) 
		{
			DEBUG_PRINTF("curl_easy_init() failed, :%d\n", byIndex);
			return ;
		}

		getToken();

		if (g.endpoint.m_event_path)
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, g.endpoint.m_event_path);
		else
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, EVENTS_PATH);

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

		if (DEBUG_ON == g_byDebugFlag)
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 1);

		pBoundary = GetBoundary(content_type_tmp, "multipart/form-data; boundary=");

		sprintf(content_type,"Content-type: %s", content_type_tmp);
		g.m_RequestBoby[byIndex].httpHeaderSlist = NULL;
		g.m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[byIndex].httpHeaderSlist, content_type);
		g.m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[byIndex].httpHeaderSlist, g_bysubmitToken);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTPHEADER, g.m_RequestBoby[byIndex].httpHeaderSlist);

		// Data
		if (1 == GetAmazonConnectStatus())
		{
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HEADERFUNCTION, PlaybackFinished_Header_callback);
			g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackFinishedEventJosnData(pBoundary, pData);
		}
		

		if (pBoundary != NULL)
			free(pBoundary);
		if (NULL == g.m_RequestBoby[byIndex].BodyBuff)
		{
			curl_easy_cleanup(g.m_RequestBoby[byIndex].easy_handle);
			DEBUG_PRINTF("submit_SynchronizeState malloc failed..");
			return;
		}

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDS, g.m_RequestBoby[byIndex].BodyBuff);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDSIZE, strlen(g.m_RequestBoby[byIndex].BodyBuff));

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_TIMEOUT, 20); 

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, SpeechRecognize_ProcessResponse);
		//curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, PlayBackFinished_ProcessResponse);

		//curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 0);

		DEBUG_PRINTF("----------------Adding easy %p to multi %p, EventName:%d", g.m_RequestBoby[byIndex].easy_handle, g.multi_handle, iEventName);


		//printf(LIGHT_GREEN "g.m_RequestBoby[%d].BodyBuff:%s\n" NONE, byIndex, g.m_RequestBoby[byIndex].BodyBuff);


		mcode = curl_multi_add_handle(g.multi_handle, g.m_RequestBoby[byIndex].easy_handle);
		if (mcode != CURLM_OK) 
		{
			DEBUG_PRINTF("curl_multi_add_handle() failed: %s",curl_multi_strerror(mcode));
			return ;
		}
	}
}

#endif

static int MusciPlayFinishProcess(void)
{

	int iRet = -1;
	PLAY_DIRECTIVE_STRUCT *playDirective = NULL;
	
	// 发送playback finished事件时判断下链表中是否还有其它音乐
	if (g.play_directive != NULL)
	{
		DEBUG_PRINTF("<<<<<<<<<<<<<<< 111111111111111111:%d", list_length(g.play_directive));
		list_iterator_t *it = NULL ;
		it = list_iterator_new(g.play_directive,LIST_HEAD);

		char *pTmpToken = strdup(g.CurrentPlayDirective.m_PlayToken);// 注意，这里可能存有内存泄露的风险???
		
		while(list_iterator_has_next(it))  // 从链表中读出数据，这里并没有删除，后面需要自己删除
		{
			playDirective =  list_iterator_next_value(it);
			if(playDirective != NULL) 
			{
				/*if (playDirective->byPlayType == TYPE_REPLACE_ENQUEUED)
				{
					continue;
				}*/
			
				// 取出数据，对比信息是否相同,相同则删除信息，继续下一次查找
				if (pTmpToken != NULL)
				{
					if (0 == strcmp(g.CurrentPlayDirective.m_PlayToken, playDirective->m_PlayToken))
					{
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

						if (pTmpToken != NULL)
						{
							free(pTmpToken);
							pTmpToken = NULL;
						}
					}
					else
					{
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
						
						g.CurrentPlayDirective.m_PlayToken			  = strdup(playDirective->m_PlayToken);
						g.CurrentPlayDirective.m_PlayUrl			  = strdup(playDirective->m_PlayUrl);
						g.CurrentPlayDirective.offsetInMilliseconds   = playDirective->offsetInMilliseconds;
						g.CurrentPlayDirective.delayInMilliseconds	  = playDirective->delayInMilliseconds;
						g.CurrentPlayDirective.intervalInMilliseconds = playDirective->intervalInMilliseconds;
						g.CurrentPlayDirective.byPlayType			  = playDirective->byPlayType;

						if (NULL != strstr(g.CurrentPlayDirective.m_PlayUrl, "cid:"))
						{
							DEBUG_PRINTF("<<<<<<<<<<<<<<< is cid >>>>>>>>>>>>>>>>>>>  return");
							return iRet;
						}
						
						SetPlayStatus(AUDIO_STATUS_STOP);
						g.m_PlayerStatus = AMAZON_AUDIOPLAYER_PLAY;
						
						printf(LIGHT_GREEN"URL:%s\n"NONE, g.CurrentPlayDirective.m_PlayUrl);
						
						// 开始播放新音乐
						int iSeekSec = g.CurrentPlayDirective.offsetInMilliseconds / 1000;
						//MpdInit();
						MpdClear();
						MpdVolume(200);
						MpdAdd(g.CurrentPlayDirective.m_PlayUrl);
						MpdRunSeek(0, iSeekSec);
						DEBUG_PRINTF("<<<<<<<<<<<<<<< 444444444444444444444444");
						
						// 如果当前正zai闹铃，则直接将音乐声音放到最小
						if(alerthandler_get_state(alertManager->handler) != ALERT_FINISHED)
						{
							MpdVolume(140);
							alerthandler_resume_alerts(alertManager);
						}

						DEBUG_PRINTF("<<<<<<<<<<<<<<< 555555555555555555555555");
						//MpdDeinit();
						SetPlayStatus(AUDIO_STATUS_PLAY);
						printf("%s, %d\n", __func__, __LINE__);
						WriteConfigValueString("playtoken", g.CurrentPlayDirective.m_PlayToken);
						
						memcpy(g.m_mpd_status.user_state, "PLAYING", strlen("PLAYING"));

						iRet = 0;

						break;
					}
				}
			}
		}
		
		DEBUG_PRINTF("all list length : %d ", list_length(g.play_directive));
		list_iterator_destroy(it);
	}

	return iRet;
}
struct timeval tv_start={0};

char *GetPlaybackStutterfinishedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};
	unsigned long stutterDuration = 0;
	struct timeval tv_finish;

	gettimeofday(&tv_finish, NULL);
	stutterDuration = (tv_finish.tv_sec - tv_start.tv_sec)*1000 + (tv_finish.tv_usec - tv_start.tv_usec)/1000;

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload, *settings;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackStutterFinished"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );

	json_object_object_add(event, "payload", payload = json_object_new_object());
	
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));
	
	int elapsed_ms = MpdGetCurrentElapsed_ms();
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(elapsed_ms));

	json_object_object_add(payload, "stutterDurationInMilliseconds", json_object_new_int(stutterDuration));

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	if (json_str)
		free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");
	
	printf(LIGHT_GREEN "g_byBodyBuff:%s\n"NONE, g_byBodyBuff);

	return g_byBodyBuff;

}
char *GetPlaybackStutterstartedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	gettimeofday(&tv_start, NULL);

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload, *settings;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackStutterStarted"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );

	json_object_object_add(event, "payload", payload = json_object_new_object());
	
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));
	
	int elapsed_ms = MpdGetCurrentElapsed_ms();
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(elapsed_ms));

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	if (json_str)
		free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");
	
	printf(LIGHT_GREEN "g_byBodyBuff:%s\n"NONE, g_byBodyBuff);

	return g_byBodyBuff;

}	

void submit_Alexa_OtherEvent(int iEventName)
{
	CURLcode code;
	CURLMcode mcode;
	char *pBoundary = NULL;
	char content_type[256] = {0};
	char content_type_tmp[128] = {0};

	char byIndex = find_IDLE_easy_handler();

	if (byIndex > 0)
	{
		g.m_RequestBoby[byIndex].easy_handle = curl_easy_init();
		if (g.m_RequestBoby[byIndex].easy_handle == NULL) 
		{
			DEBUG_PRINTF("curl_easy_init() failed, :%d\n", byIndex);
			return ;
		}

		getToken();

		if (g.endpoint.m_event_path)
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, g.endpoint.m_event_path);
		else
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, EVENTS_PATH);

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

		if (DEBUG_ON == g_byDebugFlag)
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 1);

		pBoundary = GetBoundary(content_type_tmp, "multipart/form-data; boundary=");

		sprintf(content_type,"Content-type: %s", content_type_tmp);
		g.m_RequestBoby[byIndex].httpHeaderSlist = NULL;
		g.m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[byIndex].httpHeaderSlist, content_type);
		g.m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[byIndex].httpHeaderSlist, g_bysubmitToken);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTPHEADER, g.m_RequestBoby[byIndex].httpHeaderSlist);

		if (iEventName == ALEXA_EVENT_SYN_STATE)
		{
			g.m_RequestBoby[byIndex].BodyBuff = GetSynchronizeStateEventJosnData(pBoundary);
			g_bySynStatusIndex = byIndex;
		}

		// Data
		if (1 == GetAmazonConnectStatus())
		{
			switch (iEventName)
			{
				case ALEXA_EVENT_PLAYBACK_STARTED:
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackStartedEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_PLAYBACK_PAUSED:
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackPausedEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_PLAYBACK_STOP:
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackStopEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_PLAYBACK_NEARLYFINISHED:
					DEBUG_PRINTF("<<<<<<<<<<<<<<<<<< ALEXA_EVENT_PLAYBACK_NEARLYFINISHED");
					curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HEADERFUNCTION, PlaybackNearlyFinished_Header_callback);
					//curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, PlaybackNearlyFinished_Chunk_callback);
					if (MusciPlayFinishProcess() != 0)
						g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackNearlyFinishedEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_PLAYBACK_FINISHED:
					curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_HEADERFUNCTION, PlaybackFinished_Header_callback);
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackFinishedEventJosnData(pBoundary, NULL);
					break;

				case ALEXA_EVENT_PLAYBACK_FAILED:
					//curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, PlaybackFailed_ProcessResponse);
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackFailedEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_PLAYBACK_QUEUECLEAR:
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackQueueClearEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_SPEECH_STARTED:
					g.m_RequestBoby[byIndex].BodyBuff = GetSpeechStartedEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_SPEECH_FINISHED:
					g.m_RequestBoby[byIndex].BodyBuff = GetSpeechFinishedEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_VOLUME_CHANGED:
				case ALEXA_EVENT_MUTECHANGE:
					g.m_RequestBoby[byIndex].BodyBuff = GetVolumeChangedEventJosnData(pBoundary, iEventName);
					break;

				//case ALEXA_EVENT_ALERT:
				//	DEBUG_PRINTF("ALEXA_EVENT_ALERT...");
				//	g.m_RequestBoby[byIndex].BodyBuff = GetAlertEventJosnData(pBoundary);
				//	break;
				case ALEXA_EVENT_REPORTUSERINACTIVITY:
					DEBUG_PRINTF("ALEXA_EVENT_REPORTUSERINACTIVITY...");
					gettimeofday(&begin_tv, NULL); //»ñÈ¡»á»°»î¶¯³É¹¦µÄÏµÍ³Ê±¼ä
					g.m_RequestBoby[byIndex].BodyBuff = UserInactivityReportEventJosnData(pBoundary);
					break;

				case ALEXA_EVENT_MPCPLAYSTOP:
				case ALEXA_EVENT_MPCPREV:
				case ALEXA_EVENT_MPCNEXT:
					//curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, PlaybackController_ProcessResponse);
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackControllerEventJosnData(pBoundary, iEventName);
					break;
#ifndef ALERT_EVENT_QUEUE

				case ALEXA_EVENT_STARTED_ALERT:
				case ALEXA_EVENT_BACKGROUND_ALERT:
				case ALEXA_EVENT_FOREGROUND_ALERT:
				case ALEXA_EVENT_SET_ALERT:
				case ALEXA_EVENT_STOP_ALERT:
				case ALEXA_EVENT_DELETE_ALERT:	
					g.m_RequestBoby[byIndex].BodyBuff = GetAlertEventJosnData(pBoundary, iEventName);
#endif
					break;

				case ALEXA_EVENT_PROGRESSREPORT_DELAYELAPSED:
					g.m_RequestBoby[byIndex].BodyBuff = GetProgressReportDelayElapsed(pBoundary);
					break;

				case ALEXA_EVENT_PROGRESSREPORT_INTERVALELAPSED:
					g.m_RequestBoby[byIndex].BodyBuff = GetProgressReportIntervalElapsed(pBoundary);
					break;

				case ALEXA_EVENT_SETTING:
					g.m_RequestBoby[byIndex].BodyBuff = GetSettingJosnData(pBoundary);
					break;
				case ALEXA_EVENT_PLAYBACK_STUTTERFINISHED:
					PrintSysTime("ALEXA_EVENT_PLAYBACK_STUTTERFINISHED");
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackStutterfinishedEventJosnData(pBoundary);					
					break;
				case ALEXA_EVENT_PLAYBACK_STUTTERSTARTED:
					PrintSysTime("ALEXA_EVENT_PLAYBACK_STUTTERSTARTED");
					g.m_RequestBoby[byIndex].BodyBuff = GetPlaybackStutterstartedEventJosnData(pBoundary);					
					break;				
				default:
					break;
			}
		}

		if (pBoundary != NULL)
			free(pBoundary);
		if (NULL == g.m_RequestBoby[byIndex].BodyBuff)
		{
			curl_easy_cleanup(g.m_RequestBoby[byIndex].easy_handle);
			DEBUG_PRINTF("submit_SynchronizeState malloc failed..");
			return;
		}

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDS, g.m_RequestBoby[byIndex].BodyBuff);
		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDSIZE, strlen(g.m_RequestBoby[byIndex].BodyBuff));

		curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_TIMEOUT, 20); 

		if (ALEXA_EVENT_SPEECH_FINISHED == iEventName)
		{
			g.speech_easy_handle = g.m_RequestBoby[byIndex].easy_handle;
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, SpeechFinish_ProcessResponse);
		}
		else if (ALEXA_EVENT_PLAYBACK_NEARLYFINISHED == iEventName)
		{
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, PlayBackNearFinished_ProcessResponse);
		}
		else
		{
			curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, SpeechRecognize_ProcessResponse);
		}

		//curl_easy_setopt(g.m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 0);

		DEBUG_PRINTF("----------------Adding easy %p to multi %p, EventName:%d", g.m_RequestBoby[byIndex].easy_handle, g.multi_handle, iEventName);


		//printf(LIGHT_GREEN "g.m_RequestBoby[%d].BodyBuff:%s\n" NONE, byIndex, g.m_RequestBoby[byIndex].BodyBuff);


		mcode = curl_multi_add_handle(g.multi_handle, g.m_RequestBoby[byIndex].easy_handle);
		if (mcode != CURLM_OK) 
		{
			DEBUG_PRINTF("curl_multi_add_handle() failed: %s",curl_multi_strerror(mcode));
			return ;
		}
	}
}

#define EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE


static int getTransferStatus(CURLM *multiHandleInstance,CURL *currentHandle)
{
	//check the status of transfer    CURLcode return_code=0;
	int i;
	int msgs_left = 0;
	CURLMsg *msg = NULL;
	CURL *eh = NULL;
	CURLcode return_code;
	long http_status_code = 0;
	static char byCount = 0;

	while ((msg = curl_multi_info_read(multiHandleInstance, &msgs_left)))
	{
		if (msg->msg == CURLMSG_DONE) 
		{
			eh = msg->easy_handle;
		    return_code = msg->data.result;

			// 是speech finished完成的handle
			if (g.speech_easy_handle == eh)
			{
				g.speech_easy_handle = NULL;
				printf(LIGHT_GREEN"SetSpeechStatus is 0 !\n"NONE);
				SetSpeechStatus(0);
			}

		    if ((return_code != CURLE_OK) && (return_code != CURLE_RECV_ERROR))
		    {
		    	DEBUG_PRINTF("CURL error code: %d\n", msg->data.result);
				if (g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle == eh)
				{
					if (return_code == CURLE_OPERATION_TIMEDOUT)
					{
						DEBUG_PRINTF("curl error from speech timeout...");
						SetAudioRequestStatus(AUDIO_REQUEST_IDLE);
						myexec_cmd("aplay /root/res/stop_prompt.wav");
						MpdVolume(200);
					}
						if (1 == GetAmazonPlayBookFlag())
						{
							myexec_cmd("mpc play");
							Queue_Put(ALEXA_EVENT_PLAYBACK_STARTED);
						}
						else
						{
							// 不是播放的书，不需要恢复							
							// 语音识别超时
							alerthandler_resume_alerts(alertManager);
						}
					//}
					if (return_code != 24)
						OnWriteMsgToUartd(ALEXA_IDENTIFY_ERR);

					if (1 == g_byWakeUpFlag)
					{
						g_byWakeUpFlag = 0;
						//printf(LIGHT_RED"OnWriteMsgToWakeup(start)\n"NONE);
						//OnWriteMsgToWakeup("start");
					}
				}
				else if (g.m_RequestBoby[PingPackageIndex].easy_handle == eh)
				{
					//printf(LIGHT_RED"PingPackets Timeout\n"NONE);
					PingPackageIndex = -1;
					byCount++;
					if (byCount >= 3)
					{
						printf(LIGHT_RED"Ping more than three times..\n"NONE);
						exit(-1);
					}
					else
					{
						Queue_Put(ALEXA_EVENT_PINGPACKS);
					}
				}
				else if (g.m_RequestBoby[g_bySynStatusIndex].easy_handle == eh)
				{
					// 同步状态超时
					DEBUG_PRINTF("SynStatusHandler TimeOut.");
					g_bySynStatusIndex = -1;
					Queue_Put(ALEXA_EVENT_SYN_STATE);
				}
		    }
			else
			{
				DEBUG_PRINTF("CURL Down_channel_handler:%p , CURLMSG_DONE: %p", currentHandle, eh);

				// Get HTTP status code
				http_status_code = 0;
				curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);

				DEBUG_PRINTF("http_status_code:%d", http_status_code);
				if (g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle)
					DEBUG_PRINTF("g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle:%p", g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle);

				if (currentHandle == eh)
				{
					printf(LIGHT_GREEN "Down channel Handler close..\n" NONE);
					Clear_easy_handler(eh);
					return -1;
				}
				else if (g.m_RequestBoby[RECOGNIZER_INDEX].easy_handle == eh)
				{
					SetAudioRequestStatus(AUDIO_REQUEST_IDLE);
					if (g_CaptureFifo != NULL)
					{
						printf(LIGHT_GREEN "Recognizer Event Handler close..\n" NONE);
						/*if (g_CaptureFifo)
						{
							printf(LIGHT_GREEN "11111111111..\n" NONE);
							ft_fifo_free(g_CaptureFifo);
							g_CaptureFifo = NULL;
						}*/
					}
				}
				else if (g.m_RequestBoby[PingPackageIndex].easy_handle == eh)
				{
					printf(LIGHT_RED"PingPackets Succeed!!\n"NONE);
					PingPackageIndex = -1;
					byCount = 0;
				}
				else if (g.m_RequestBoby[g_bySynStatusIndex].easy_handle == eh)
				{
					g_bySynStatusIndex = -1;
					if (204 == http_status_code)
					{
						g.m_AmazonInitIDFlag |= AMAZON_SYSN_REQUEST;
						if (AMAZON_INIT_FLAG == g.m_AmazonInitIDFlag)
						{
							g.m_AmazonInitIDFlag = 0;
							SetAmazonConnectStatus(1);
							Queue_Put(ALEXA_EVENT_SETTING);
							printf(LIGHT_GREEN "Amamzon alexa init ok..\n" NONE);
						}
					}
					else if (200 == http_status_code)
					{
						DEBUG_PRINTF("close downchannel..");
						CloseDownchannel();
					}
				}
			}

			Clear_easy_handler(eh);
		}
	}

	return http_status_code;
}

bool curl_send_request(void)
{
	CURLMcode mc;
	int numfds;
	int still_running;

	DEBUG_PRINTF("curl_send_request..");

	do {
		mc = curl_multi_perform(g.multi_handle, &still_running);

		if(mc == CURLM_OK ) {
			mc = curl_multi_wait(g.multi_handle, NULL, 0, 1000, &numfds);
		}
		if(mc != CURLM_OK) {
			DEBUG_PRINTF("curl_multi failed, code %d.n", mc);
			break;
		}

		if(!numfds){
			struct timeval wait = { 0, 100 * 1000 }; // 100ms
			select(0, NULL, NULL, NULL, &wait);
		}

		if (-1 == getTransferStatus(g.multi_handle, g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle))
		{
			DEBUG_PRINTF("-----down_channel_curl_easy_init----");
			down_channel_curl_easy_init();
		}
	} while(still_running);

	SetAmazonConnectStatus(0);

	Clear_multi_handler();

	return true;
} 


#ifdef USE_EVENT_QUEUE


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static pthread_mutex_t     eventQueueMtx = PTHREAD_MUTEX_INITIALIZER;  
static int  eventQueueWait = 0; 

static void SetEventQueueWait(int wait)
{
	pthread_mutex_lock(&eventQueueMtx);
	eventQueueWait= wait;
	pthread_mutex_unlock(&eventQueueMtx);
}

void EventQueuePut(int type, void *data)
{	
	int ret; 

	pthread_mutex_lock(&mtx); 

	pthread_mutex_lock(&eventQueueMtx);
	QueuePut(type,data);
	if(eventQueueWait == 0) {
		pthread_cond_signal(&cond);  
	}
	pthread_mutex_unlock(&eventQueueMtx);
	
	pthread_mutex_unlock(&mtx); 
}

void *CurlRequestPthread(void *ppath)
{
	QUEUE_ITEM *item = NULL;

	while (1) 
	{  
	    pthread_mutex_lock(&mtx);         
		
	    while (QueueSizes() == 0)   
		{
			DEBUG_PRINTF("Queue is  empty... ");
			SetEventQueueWait(0);	
			DEBUG_PRINTF("SetEventQueueWait..");
	        pthread_cond_wait(&cond, &mtx);
	    }
		char *data = NULL;
		SetEventQueueWait(1);
		DEBUG_PRINTF("Queue is not empty...");
		int size = QueueSizes();
		DEBUG_PRINTF("QueueSizes:%d",size);
		item = QueueGet();
		DEBUG_PRINTF("item->type:%d",item->type);
		switch(item->type) {

			case ALEXA_EVENT_SPEECH_RECOGNIZE:
				//saveBodyFile = fopen(BODY_FILE_NAME,  "wb+" );
				submit_SpeechRecognizeEvent();
				break;

			case ALEXA_EVENT_PINGPACKS:
				submit_PingPackets();
				break;
				
#ifdef ALERT_EVENT_QUEUE	
			case ALEXA_EVENT_STARTED_ALERT:
			case ALEXA_EVENT_BACKGROUND_ALERT:
			case ALEXA_EVENT_FOREGROUND_ALERT:
			case ALEXA_EVENT_SET_ALERT:
			case ALEXA_EVENT_STOP_ALERT:
			case ALEXA_EVENT_DELETE_ALERT:	
				if(item->data) { 
					data = (char *)item->data;
					printf(LIGHT_RED "data addr:%p value:%s\n" NONE, data, data);
				}

				if (item->type == ALEXA_EVENT_STOP_ALERT)
				{
					OnWriteMsgToUartd(ALEXA_ALER_END);
					MpdVolume(200);
				}

				submit_Alexa_AlertEvent(item->type, data);
				if(data) {
					free(data);
				}
				break;

			case ALEXA_EVENT_PLAYBACK_FINISHED:
				if(item->data)
				{
					data = (char *)item->data;
					printf(LIGHT_RED "data addr:%p value:%s\n" NONE, data, data);
					submit_Alexa_PlayFinishedEvent(item->type, data);
					if(data) {
						free(data);
					}
				}
				else
				{
					submit_Alexa_OtherEvent(item->type);
				}
				break;
#endif
			default:	
				submit_Alexa_OtherEvent(item->type);
				break;
		}

		Free_Queue_Item(item);
        pthread_mutex_unlock(&mtx);
	}  
}


#else
void *CurlRequestPthread(char *ppath)
{
	int iCount = 0;
	char byQueueValue = -1;
	int iLength = 0;
	while(1)
	{
		iLength = ft_fifo_getlenth(g_DataFifo);
		if (iLength > 0)
		{
			ft_fifo_seek(g_DataFifo, &byQueueValue, 0, 1);

			ft_fifo_setoffset(g_DataFifo, 1);
			iLength = 0;

			printf(LIGHT_RED"byQueueValue:%d\n"NONE, byQueueValue);

			switch (byQueueValue)
			{
				case ALEXA_EVENT_SPEECH_RECOGNIZE:
					//saveBodyFile = fopen(BODY_FILE_NAME,  "wb+" );
					submit_SpeechRecognizeEvent();
					break;

				case ALEXA_EVENT_PINGPACKS:
					submit_PingPackets();
					break;

				default:
					submit_Alexa_OtherEvent(byQueueValue);
					break;
			}

		}
		usleep(1000 * 100);
	}
}
#endif

void CreateCurlHttpsRequestPthread(void)
{
	int iRet;

	//pthread_attr_t attr;
	//struct sched_param param;
	
	//pthread_attr_init(&attr);
	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	//param.sched_priority = 55;
	//pthread_attr_setschedpolicy (&attr, SCHED_RR);
	//pthread_attr_setschedparam(&attr, &param);
	//pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);


	iRet = pthread_create(&CurlHttpsRequest_Pthread, NULL, CurlRequestPthread, NULL);
	if(iRet != 0)
	{
		DEBUG_PRINTF("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}

