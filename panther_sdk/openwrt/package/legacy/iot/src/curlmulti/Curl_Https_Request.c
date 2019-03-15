#include "globalParam.h"
#include <curl/curl.h>
#include <assert.h>





char PingPackageIndex;
FT_FIFO * g_DataFifo = NULL;



static char FindIdleEasyHandler(void)
{
	int i;
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);
	for ( i = 2 ; i < MAX_CONNECT_STREM - 2; i++ )
	{
	    if (NULL == g->m_RequestBoby[i].easy_handle)
		{
			return i;
		}
	}

	return -1;
}

void InitEasyHandler(GlobalParam *g)
{
	int i;
	assert(g != NULL);
	g->multi_handle = NULL;
	for ( i = 0 ; i < 10; i++ )
	{
	    g->m_RequestBoby[i].easy_handle = NULL;
	}
}

void ClearEasyHandler(CURL * curl_handle)
{
	int i;
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);
	pthread_mutex_lock(&g->Clear_easy_handler_mtx);

	if (NULL == curl_handle || NULL == g->multi_handle)
	{
		pthread_mutex_unlock(&g->Clear_easy_handler_mtx);
		return ;
	}

	curl_multi_remove_handle(g->multi_handle, curl_handle);
	curl_easy_cleanup(curl_handle);

	for (i = 0; i < MAX_CONNECT_STREM; i++)
	{
		if (curl_handle == g->m_RequestBoby[i].easy_handle)
		{
			g->m_RequestBoby[i].easy_handle = NULL;
			if (g->m_RequestBoby[i].BodyBuff != NULL)
			{
				free(g->m_RequestBoby[i].BodyBuff);
				g->m_RequestBoby[i].BodyBuff = NULL;
			}

		    if (g->m_RequestBoby[i].httpHeaderSlist)
	    	{
      			curl_slist_free_all(g->m_RequestBoby[i].httpHeaderSlist);
				g->m_RequestBoby[i].httpHeaderSlist = NULL;
	    	}

			if (g->m_RequestBoby[i].formpost != NULL)
			{
				curl_formfree(g->m_RequestBoby[i].formpost);
				g->m_RequestBoby[i].formpost = NULL;
			}
			if(g->m_RequestBoby[i].data = NULL) {
				free(g->m_RequestBoby[i].data);
				g->m_RequestBoby[i].data = NULL;
			}
			break;
		}
	}

	g->m_RequestBoby[i].easy_handle = NULL;

    pthread_mutex_unlock(&g->Clear_easy_handler_mtx);

}

static void ClearMultiHandler(void)
{
	int i;
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);
	for ( i = 0 ; i < MAX_CONNECT_STREM; i++ )
	{
	    if (NULL != g->m_RequestBoby[i].easy_handle)
		{
			curl_multi_remove_handle(g->multi_handle, g->m_RequestBoby[i].easy_handle);
			curl_easy_cleanup(g->m_RequestBoby[i].easy_handle); 
			g->m_RequestBoby[i].easy_handle = NULL;
		}
	}

	//curl_multi_cleanup(g->multi_handle);
	//g->multi_handle = NULL;
}



size_t SpeechRecognizeHeaderCallback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	if (13 == (nmemb * size))
	{
		if (NULL == strstr((char *)ptr, "HTTP/2 200"))
		{
			info("%s", ptr);

			// 语音识别失败
			exec_cmd("aplay /root/res/stop_prompt.wav && mpc volume 200");
		}
	}
	return (nmemb * size);
}

// write callback function
size_t SpeechRecognizeProcessResponse(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	//warning("ptr:%s",ptr);
	ProcessturingJson((char *)ptr);

	return (nmemb * size);
}

static int SpeechProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)    
{    
	
	if(IsHttpRequestCancled()) {
		error("http canled...");
		return -1;
	}
    return 0;    
}  



void SubmitPingPackets(void) 
{
	CURLcode code;
	CURLMcode mcode;

	char byIndex = FindIdleEasyHandler();
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);

	if (byIndex > 0)
	{
		g->m_RequestBoby[byIndex].easy_handle = curl_easy_init();
		if (g->m_RequestBoby[byIndex].easy_handle == NULL) 
		{
			error("curl_easy_init() failed, :%d\n", byIndex);
			return ;
		}

		info("-----------submit_PingPackets------");

		//curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, PING_PATH);

		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_NOSIGNAL, 1L);
		g->m_RequestBoby[byIndex].httpHeaderSlist = NULL;
		//g->m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g->m_RequestBoby[byIndex].httpHeaderSlist, g_bysubmitToken);

		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTPHEADER, g->m_RequestBoby[byIndex].httpHeaderSlist);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_TIMEOUT, 10);

		info("Adding easy %p to multi %p\n", g->m_RequestBoby[byIndex].easy_handle, g->multi_handle);
		mcode = curl_multi_add_handle(g->multi_handle, g->m_RequestBoby[byIndex].easy_handle);
		if (mcode != CURLM_OK) 
		{
			error("curl_multi_add_handle() failed: %s",curl_multi_strerror(mcode));
			return ;
		}
		PingPackageIndex = byIndex;
	}
}

#define EVENTS_PATH "http://smartdevice.ai.tuling123.com/speech/chat"
static CURLSH *share_curl = NULL;
static int porgress = 0;


static void ShareCurlInit()
{
	if( !share_curl ) {
			if ((share_curl = curl_share_init()) == NULL) {
		    fprintf(stderr, "curl_share_init() failed\n");
		    curl_global_cleanup();
		    return -1;
		  }
		  curl_share_setopt( share_curl, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  	}
}

void SubmitSpeechRecognizeEvent(int index ,char *data , int len) 
{
	CURLcode code;
	CURLMcode mcode;

	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);
	
	info("multi_handle:%#x", g->multi_handle);
	debug("g:%#x", g);
	if(g->multi_handle == NULL) {
		MultiInit();
	}
		
	g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle = curl_easy_init();
	if (g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle == NULL) 
	{
		error("curl_easy_init() failed \n");
		return ;
	}

	info("[Start]");

	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_URL, EVENTS_PATH);

	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_NOSIGNAL, 1L);

	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_VERBOSE, 0);
	
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_TCP_KEEPALIVE, 1l);
	/* keep-alive idle time to 120 seconds */
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_TCP_KEEPIDLE, 20l);
	/* interval time between keep-alive probes: 60 seconds */
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_TCP_KEEPINTVL, 3l);

	char *token = GetTuringToken();
	char *apiKey = GetTuringApiKey();
	char *userId = GetTuringUserId();
	int tone = GetTuringTone()
	char *jsonData = CreateRequestJson(apiKey, userId , token, 0, index, 1, tone);


	ShareCurlInit();
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_SHARE, share_curl);
	
	g->m_RequestBoby[RECOGNIZER_INDEX].BodyBuff = jsonData;
	g->m_RequestBoby[RECOGNIZER_INDEX].data = data;
	g->m_RequestBoby[RECOGNIZER_INDEX].len = len;
	g->m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist = NULL;
	g->m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist = curl_slist_append(g->m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist, "Expect:");
	g->m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist = curl_slist_append(g->m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist, "Charset: UTF-8");

	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HTTPHEADER, g->m_RequestBoby[RECOGNIZER_INDEX].httpHeaderSlist);
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_TIMEOUT, 30);

	g->m_RequestBoby[RECOGNIZER_INDEX].formpost = NULL;
	g->m_RequestBoby[RECOGNIZER_INDEX].lastptr = NULL;

	curl_formadd(&g->m_RequestBoby[RECOGNIZER_INDEX].formpost, 
				 &g->m_RequestBoby[RECOGNIZER_INDEX].lastptr,
				CURLFORM_COPYNAME, "parameters",
				CURLFORM_COPYCONTENTS, g->m_RequestBoby[RECOGNIZER_INDEX].BodyBuff,
				 CURLFORM_END);
	curl_formadd(&g->m_RequestBoby[RECOGNIZER_INDEX].formpost,
				 &g->m_RequestBoby[RECOGNIZER_INDEX].lastptr,
				 CURLFORM_COPYNAME, "speech",
				 CURLFORM_BUFFER, "/tmp/pcm.raw",
				 CURLFORM_BUFFERPTR, g->m_RequestBoby[RECOGNIZER_INDEX].data,				// 该参数是指向要上传的缓冲区的指针
				 CURLFORM_BUFFERLENGTH, g->m_RequestBoby[RECOGNIZER_INDEX].len,			// 缓冲区的长度
				 CURLFORM_CONTENTTYPE, "application/octet-stream",
				 CURLFORM_END);
	debug("curl_easy_setopt CURLOPT_HTTPPOST");

	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HTTPPOST, g->m_RequestBoby[RECOGNIZER_INDEX].formpost);

	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_HEADERFUNCTION, SpeechRecognizeHeaderCallback);
    curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_WRITEFUNCTION, SpeechRecognizeProcessResponse);	
	curl_easy_setopt(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle, CURLOPT_PROGRESSFUNCTION, SpeechProgressCallback);
	debug("curl_multi_add_handle");
	info("multi_handle:%#x", g->multi_handle);
	mcode = curl_multi_add_handle(g->multi_handle, g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle);
	if (mcode != CURLM_OK) 
	{
		info("curl_multi_add_handle() failed: %s，mcode:%d", curl_multi_strerror(mcode), mcode);
		exit(-1);
		return ;
	}
	debug("exit");
}
static void IotProcessCallback(void *buffer, size_t size, size_t nmemb, void *stream)
{
	info("buffer:%s",buffer);
   	info("stream:%d",stream);
	ProcessIotJson(buffer, (int)stream);
	return (nmemb * size);
}
static size_t IotHeaderCallback(void *ptr, size_t size, size_t nmemb, void *stream)  
{

	return (nmemb * size);
}

void SubmitTuringOtherEvent(int iEventName)
{
	CURLcode code;
	CURLMcode mcode;
	char *pBoundary = NULL;
	char content_type[256] = {0};
	char content_type_tmp[128] = {0};
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);

	char byIndex = FindIdleEasyHandler();

	if (byIndex > 0)
	{
		g->m_RequestBoby[byIndex].easy_handle = curl_easy_init();
		if (g->m_RequestBoby[byIndex].easy_handle == NULL) 
		{
			error("curl_easy_init() failed, :%d\n", byIndex);
			return ;
		}

		//curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_URL, EVENTS_PATH);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_VERBOSE, 0);

		
		g->m_RequestBoby[byIndex].httpHeaderSlist = NULL;
		g->m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g->m_RequestBoby[byIndex].httpHeaderSlist, "Expect:");
		g->m_RequestBoby[byIndex].httpHeaderSlist = curl_slist_append(g->m_RequestBoby[byIndex].httpHeaderSlist, "Content-Type:application/json;charset=UTF-8");
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_HEADERFUNCTION, IotHeaderCallback);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_WRITEFUNCTION, IotProcessCallback);

		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_HTTPHEADER, g->m_RequestBoby[byIndex].httpHeaderSlist);

		//if (iEventName == ALEXA_EVENT_SYN_STATE)
		{
			g->m_RequestBoby[byIndex].formpost = NULL;
			g->m_RequestBoby[byIndex].lastptr = NULL;

			curl_formadd(&g->m_RequestBoby[byIndex].formpost, 
						 &g->m_RequestBoby[byIndex].lastptr,
						CURLFORM_COPYNAME, "speech",
						CURLFORM_CONTENTTYPE,"application/octet-stream",
						CURLFORM_END);

		}

		switch (iEventName)
		{
	//		case ALEXA_EVENT_PLAYBACK_STARTED:
		//		g->m_RequestBoby[byIndex].BodyBuff = GetPlaybackStartedEventJosnData(pBoundary);
			//	break;

	
				
		}

		if (NULL == g->m_RequestBoby[byIndex].BodyBuff)
		{
			error("submit_SynchronizeState malloc failed..");
			return;
		}

		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDS, g->m_RequestBoby[byIndex].BodyBuff);
		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_POSTFIELDSIZE, strlen(g->m_RequestBoby[byIndex].BodyBuff));

		curl_easy_setopt(g->m_RequestBoby[byIndex].easy_handle, CURLOPT_TIMEOUT, 10); 

		error("Adding easy %p to multi %p\n", g->m_RequestBoby[byIndex].easy_handle, g->multi_handle);

		mcode = curl_multi_add_handle(g->multi_handle, g->m_RequestBoby[byIndex].easy_handle);
		if (mcode != CURLM_OK) 
		{
			error("curl_multi_add_handle() failed: %s",curl_multi_strerror(mcode));
			return ;
		}
	}
}


static int GetTransferStatus(CURLM *multiHandleInstance,CURL *currentHandle)
{
	//check the status of transfer    CURLcode return_code=0;
	int i;
	int msgs_left = 0;
	CURLMsg *msg = NULL;
	CURL *eh = NULL;
	CURLcode return_code;
	long http_status_code = 0;
	static char byCount = 0;
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);

	while ((msg = curl_multi_info_read(multiHandleInstance, &msgs_left)))
	{
		if (msg->msg == CURLMSG_DONE) 
		{
			eh = msg->easy_handle;
		    return_code = msg->data.result;

		    if ((return_code != CURLE_OK) && (return_code != CURLE_RECV_ERROR))
		    {
		    	info("CURL error code: %d\n", msg->data.result);
				if (g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle == eh)
				{
					warning("curl error from speech timeout...");
					exec_cmd("aplay /root/res/stop_prompt.wav && mpc volume 200");
				}
				/*
				else if (g->m_RequestBoby[PingPackageIndex].easy_handle == eh)
				{
					warning("PingPackets Timeout\n");
					PingPackageIndex = -1;
					byCount++;
					if (byCount >= 3)
					{
						warning("Ping more than three times..\n");
						exit(-1);
					}
					else
					{
						//Queue_Put(ALEXA_EVENT_PINGPACKS);
					}
				}*/
				
		    }
			else
			{
				info("CURL Down_channel_handler:%p , CURLMSG_DONE: %p", currentHandle, eh);

				// Get HTTP status code
				http_status_code = 0;
				curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);
				
				info("http_status_code:%d", http_status_code);
				
				if (currentHandle == eh)
				{
					info("Down channel Handler close..\n");
					return -1;
				}
				else if (g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle == eh)
				{
					//g->m_byAudioRequestFlag = 1;
					info("RECOGNIZER_INDEX ...\n");
					
					//CaptureFifoClear();
				}
				else if (g->m_RequestBoby[PingPackageIndex].easy_handle == eh)
				{
					info("PingPackets Succeed!!\n");
					//PingPackageIndex = -1;
					byCount = 0;
				}
			}

			ClearEasyHandler(eh);
		}
	}

	return http_status_code;
}

bool CurlSendRequest(void)
{
	CURLMcode mc;
	int numfds;
	int still_running;

	debug("curl_send_request..");
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);

	do {
		mc = curl_multi_perform(g->multi_handle, &still_running);

		if(mc == CURLM_OK ) {
			mc = curl_multi_wait(g->multi_handle, NULL, 0, 1000, &numfds);
		}
		if(mc != CURLM_OK) {
			error("curl_multi failed, code %d.n", mc);
			break;
		}

		if(!numfds){
			struct timeval wait = { 0, 100 * 1000 }; // 100ms
			select(0, NULL, NULL, NULL, &wait);
		}

		if (-1 == GetTransferStatus(g->multi_handle, g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle))
			break;
		
	} while(still_running);

	ClearMultiHandler();

	return true;
} 





