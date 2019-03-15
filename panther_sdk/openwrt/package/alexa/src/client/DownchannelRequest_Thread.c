#include "globalParam.h"
#include <curl/curl.h>

pthread_t DownchannelRequest_Pthread;
struct  timeval	begin_tv;
extern GLOBALPRARM_STRU g;
extern UINT8 g_bysubmitToken[];
extern g_byAmazonLogInFlag;
extern FT_FIFO * g_DataFifo;
struct data {
  char trace_ascii; /* 1 or 0 */ 
};

#ifdef USE_EVENT_QUEUE
bool Queue_Put(int iVluae)
{
	printf(LIGHT_RED "Queue_Put iVluae:%d\n" NONE, iVluae);
	EventQueuePut(iVluae,NULL);
}
bool Queue_Put_Data(int iVluae, void *data)
{
	printf(LIGHT_RED "Queue_Put iVluae:%d, data:%s\n" NONE, iVluae, data);
	EventQueuePut(iVluae,data);
}

#else
bool Queue_Put(int iVluae)
{
	char byValue = (char)iVluae;
	ft_fifo_put(g_DataFifo, &byValue, 1);
}
#endif


size_t DownChannel_Header_callback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	//DEBUG_PRINTF("%s", ptr);

	if (13 == (nmemb * size))
	{
		if (strstr((char *)ptr, "HTTP/2 200"))
		{
			g.m_AmazonInitIDFlag |= AMAZON_DOWNCHANNEL_REQUEST;
			gettimeofday(&begin_tv, NULL);
			fprintf(stderr, LIGHT_RED"ALEXA_EVENT_SYN_STATE...\n"NONE);
			Queue_Put(ALEXA_EVENT_SYN_STATE);
		}
	}

	return (nmemb * size);
}

size_t DownChannel_ChunkData_callback(void *buffer,size_t size,size_t count,void **response)  
{
	char * ptr = buffer;

	//DEBUG_PRINTF("rec: %s",ptr);  
	//pthread_mutex_lock(&g.Handle_mtc);

	ProcessAmazonDirective((char *)ptr, (int)(count * size));
	//pthread_mutex_unlock(&g.Handle_mtc);

	return count;  
}

static void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;
 
  unsigned int width=0x10;
 
  if(nohex)
    /* without the hex output, we can fit more on screen */ 
    width = 0x40;
 
  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);
 
  for(i=0; i<size; i+= width) {
 
    fprintf(stream, "%4.4lx: ", (long)i);
 
    if(!nohex) {
      /* hex not disabled, show it */ 
      for(c = 0; c < width; c++)
        if(i+c < size)
          fprintf(stream, "%02x ", ptr[i+c]);
        else
          fputs("   ", stream);
    }
 
    for(c = 0; (c < width) && (i+c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */ 
      if(nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
        i+=(c+2-width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */ 
      if(nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
        i+=(c+3-width);
        break;
      }
    }
    fputc('\n', stream); /* newline */ 
  }
  fflush(stream);
}
 
static int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  struct data *config = (struct data *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */ 
 
  switch(type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */ 
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
  	return 0;
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
	return 0;
    break;
  }
 
  dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
  return 0;
}

static int DownchannelProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)    
{	  
	if(GetDownchannelStatus()) {
		SetDownchannelStatus(0);
		DEBUG_ERROR("close downchannel..");
		return -1;
	}
	return 0;	  
} 


void CloseDownchannel(void)
{
	SetDownchannelStatus(1);
}

bool down_channel_curl_easy_init(void)
{
	CURLcode code;
	CURLMcode mcode;
	struct data config;

	config.trace_ascii = 1; /* enable ascii tracing */ 

	g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle = curl_easy_init();
	if (g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle == NULL) 
	{
		DEBUG_PRINTF("curl_easy_init() failed\n");
		return false;
	}

	if (g.endpoint.m_directives_path)
		curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_URL, g.endpoint.m_directives_path);
	else
		curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_URL, DIRECTIVES_PATH);

	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_SSL_VERIFYHOST, 0);

	getToken();

	g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].httpHeaderSlist = NULL;
	g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].httpHeaderSlist = curl_slist_append(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].httpHeaderSlist, g_bysubmitToken);

	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_HTTPHEADER, g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].httpHeaderSlist);
	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_HEADERFUNCTION, DownChannel_Header_callback);
	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_WRITEFUNCTION, DownChannel_ChunkData_callback);
	//curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
	//curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_NOPROGRESS, false);
	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_PROGRESSFUNCTION, DownchannelProgressCallback);

	curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_NOSIGNAL, 1L);

	if (DEBUG_ON == g_byDebugFlag)
	{
		curl_easy_setopt(g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle, CURLOPT_VERBOSE, 1);
	}

	//curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);

	mcode = curl_multi_add_handle(g.multi_handle, g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle);
	if (mcode != CURLM_OK) 
	{
		DEBUG_PRINTF("curl_multi_add_handle() failed: %s",curl_multi_strerror(mcode));
		return false;
	}

	printf(LIGHT_RED "down_channel_handler:%p\n"NONE, g.m_RequestBoby[DOWN_CHANNEL_STREAM_INDEX].easy_handle);

	return true;
}


void *DownchannelRequestThread(char *ppath)
{
	char *getRecordStatus = NULL;
	char iCount = 0;

	while (1)
	{
		while (1)
		{
			if (SIGNIN_SUCCEED == GetAmazonLogInStatus())
			{
				break;
			}
			else
			{
				getRecordStatus = ReadConfigValueString("accessToken");
				if (getRecordStatus != NULL)
				{
					int iLen = strlen(getRecordStatus);
					if (iLen > 8)
					{
						free(getRecordStatus);
						getRecordStatus = NULL;

						if(0 == OnRefreshToken())
						{
							//g_byGetTokenStatus = 2;
							break;
						}
						else
						{
							iCount++;
							if (5 == iCount)
							{
								exit(0);
							}
						}
					}
					else
					{
						//fprintf(stderr, "getRecordStatus <=8,%s \n", getRecordStatus);
					}
				}
				else
				{
					fprintf(stderr, "getRecordStatus is null \n");
				}
			}
			sleep(1);
		}

		g.multi_handle = curl_multi_init();

		DEBUG_PRINTF("multi_handle:%#x, curl_version:%s", g.multi_handle, curl_version());

		curl_multi_setopt(g.multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
	  	curl_multi_setopt(g.multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);

		down_channel_curl_easy_init();

		curl_send_request();

		/*if(curl_send_request() == true )
		{	
			gettimeofday(&begin_tv, NULL);	//»ñÈ¡»á»°»î¶¯³É¹¦µÄÏµÍ³Ê±¼ä
		}*/

		DEBUG_PRINTF(LIGHT_RED "DownchannelRequestThread over..\n" NONE);
	//	OnWriteMsgToUartd("tlkoff");
	//	MpdVolume(200);

		myexec_cmd("uartdfifo.sh tlkoff");
		MpdVolume(200);

		//OnWriteMsgToWakeup("start");
	}
}

void CreateDownchannelRequestPthread(void)
{
	int iRet;

	iRet = pthread_create(&DownchannelRequest_Pthread, NULL, DownchannelRequestThread, NULL);
	if(iRet != 0)
	{
		DEBUG_PRINTF("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}

