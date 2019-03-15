#include "globalParam.h"
#include <json/json.h>
#include <fcntl.h>
#include <errno.h>
#include "uuid.h"
#include <sys/types.h>

#define FIFO_NAME	"/tmp/UartFIFO"
#define TEN_MEG (1024 * 1024 * 10)  
#define FIFO_BUFFER_SIZE 32

char g_bySendInterFlag = 0;


#define MAX_COMMAND_LEN 128

extern GLOBALPRARM_STRU g;
extern int32_t inactiveTime;
extern UINT8 g_byAmazonLogInFlag;

static pthread_mutex_t PlayBookMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_mutex_t LogInMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t TTSAduioMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_mutex_t AudioRequestMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_mutex_t SpeechMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_mutex_t AbortCallbackMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_mutex_t WriteMsgUartdMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t SendInterFlagMtx = PTHREAD_MUTEX_INITIALIZER; 
static pthread_mutex_t getCDB_multiroom_enableMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t exec_cmdMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t playmode_Mtx = PTHREAD_MUTEX_INITIALIZER;

/*
 * 这里的播放状态只针对与alexa的播放状态
 */
void SetPlayStatus(int iStatus)
{
	pthread_mutex_lock(&playmode_Mtx);
	DEBUG_PRINTF("iStatus:%d", iStatus);
	g.m_PlayMode = iStatus;
	pthread_mutex_unlock(&playmode_Mtx);
}

int GetPlayStatus()
{
	int iPlaystatus = -1;
	pthread_mutex_lock(&playmode_Mtx);
	iPlaystatus = g.m_PlayMode;
	pthread_mutex_unlock(&playmode_Mtx);
	return iPlaystatus;
}

void OnUpdateURL(char *pBaseURL)
{
	int iLength = 0;
	char *bassurl = NULL;
	if (g.endpoint.m_directives_path)
	{
		free(g.endpoint.m_directives_path);
		g.endpoint.m_directives_path = NULL;
	}
	
	if (g.endpoint.m_event_path)
	{
		free(g.endpoint.m_event_path);
		g.endpoint.m_event_path = NULL;
	}
	
	if (g.endpoint.m_ping_path)
	{
		free(g.endpoint.m_ping_path);
		g.endpoint.m_ping_path = NULL;
	}

	if (pBaseURL)
	{
		bassurl = pBaseURL;
	}
	else
	{
		bassurl = ReadConfigValueString("endpoint");
		if (NULL == bassurl)
		{
			DEBUG_PRINTF("Failed to get endpoint from ConfigFile , using default URL..");
			bassurl = BASE_URL;
		}
	}

	DEBUG_PRINTF("bassurl is :%s", bassurl);

	iLength = strlen(bassurl) + strlen("/v20160207/directives") + 1;
	g.endpoint.m_directives_path = (char *)malloc(iLength);
	if (NULL == g.endpoint.m_directives_path)
	{
		DEBUG_PRINTF("m_directives_path malloc failed");
		g.endpoint.m_directives_path = DIRECTIVES_PATH;
	}
	else
	{
		memset(g.endpoint.m_directives_path, 0, iLength);
		strcat(g.endpoint.m_directives_path, bassurl);
		strcat(g.endpoint.m_directives_path, "/v20160207/directives");
	}

	iLength = strlen(bassurl) + strlen("/v20160207/events") + 1;
	g.endpoint.m_event_path = (char *)malloc(iLength);
	if (NULL == g.endpoint.m_event_path)
	{
		DEBUG_PRINTF("m_event_path malloc failed, ");
		g.endpoint.m_event_path = EVENTS_PATH;
	}
	else
	{
		memset(g.endpoint.m_event_path, 0, iLength);
		strcat(g.endpoint.m_event_path, bassurl);
		strcat(g.endpoint.m_event_path, "/v20160207/events");
	}

	iLength = strlen(bassurl) + strlen("/v20160207/ping") + 1;
	g.endpoint.m_ping_path = (char *)malloc(iLength);
	if (NULL == g.endpoint.m_ping_path)
	{
		DEBUG_PRINTF("m_ping_path malloc failed, ");
		g.endpoint.m_ping_path = PING_PATH;
	}
	else
	{
		memset(g.endpoint.m_ping_path, 0, iLength);
		strcat(g.endpoint.m_ping_path, bassurl);
		strcat(g.endpoint.m_ping_path, "/v20160207/ping");
	}

	DEBUG_PRINTF("[directives_path]:%s", g.endpoint.m_directives_path);
	DEBUG_PRINTF("[event_path     ]:%s", g.endpoint.m_event_path);	
	DEBUG_PRINTF("[ping_path      ]:%s", g.endpoint.m_ping_path);
}


int OnWriteMsgToUartd(const char *pData)
{
	int iRet = -1;
#if 1
    int pipe_fd;  
    int res;  

	pthread_mutex_lock(&WriteMsgUartdMtx);
	iRet = uartd_fifo_write_fmt(pData);

	pthread_mutex_unlock(&WriteMsgUartdMtx);
#else

	char byTemp[128] = {0};
	strcpy(byTemp, "uartdfifo.sh ");
	strcat(byTemp, pData);

	DEBUG_PRINTF("byTemp:%s", byTemp);

	myexec_cmd(byTemp);
#endif
	return iRet;
}  



int myexec_cmd(const char *cmd)
{
    char buf[MAX_COMMAND_LEN];
    FILE *pfile;
    int status = -2;

    pthread_mutex_lock(&exec_cmdMtx);
    if ((pfile = popen(cmd, "r"))) {
        fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
        while(!feof(pfile)) {
            fgets(buf, sizeof buf, pfile);
        }
        status = pclose(pfile);
    }
    pthread_mutex_unlock(&exec_cmdMtx);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

void PrintSysTime(char *pSrc)
{
#if 1
	struct   tm     *ptm; 
	long       ts; 
	int         y,m,d,h,n,s; 

	ts  = time(NULL); 
	ptm = localtime(&ts); 

	y = ptm-> tm_year+1900; 
	m = ptm-> tm_mon+1;     
	d = ptm-> tm_mday;      
	h = ptm-> tm_hour;      
	n = ptm-> tm_min;       
	s = ptm-> tm_sec;       
	DEBUG_PRINTF("[%s]---sys_time:%02d.%02d.%02d.%02d.%02d.%02d", pSrc, y, m, d, h, n, s);
	
#else
	struct timeval start;
    gettimeofday( &start, NULL );
   	printf("\033[1;31;40m [%s]---sys_time:%d.%d \033[0m\n", pSrc, start.tv_sec, start.tv_usec);
#endif

}


char GetWiFiConnectStatus(void)
{
	return (char)cdb_get_int("$wanif_state", 0);
}

char GetSendInterValFlag(void)
{
	char byRet = 0;

	pthread_mutex_lock(&SendInterFlagMtx);
	
	byRet = g_bySendInterFlag;
	
	pthread_mutex_unlock(&SendInterFlagMtx);
	
	return byRet;
}

void SetSendInterValFlag(char byFlag)
{
	char byRet = 0;

	pthread_mutex_lock(&SendInterFlagMtx);
	
	g_bySendInterFlag = byFlag;
	
	pthread_mutex_unlock(&SendInterFlagMtx);
}


char GetMuteFlag(void)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	byRet = g.m_byMuteFlag;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
	
	return byRet;
}

void SetMuteFlag(char byFlag)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	g.m_byMuteFlag = byFlag;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
}



char GetRecordFlag(void)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	byRet = g.m_byrecordFlag;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
	
	return byRet;
}

void SetRecordFlag(char byFlag)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	g.m_byrecordFlag = byFlag;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
}


char GetDownchannelStatus(void)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	byRet = g.endpoint.m_byDownChannelFlag;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
	
	return byRet;
}

void SetDownchannelStatus(char byStatus)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	g.endpoint.m_byDownChannelFlag = byStatus;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
}




char GetCallbackStatus(void)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	byRet = g.m_byAbortCallback;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
	
	return byRet;
}

void SetCallbackStatus(char byStatus)
{
	char byRet = 0;

	pthread_mutex_lock(&AbortCallbackMtx);
	
	g.m_byAbortCallback = byStatus;
	
	pthread_mutex_unlock(&AbortCallbackMtx);
}


char GetTTSAudioStatus(void)
{
	char byRet = 0;

	pthread_mutex_lock(&TTSAduioMtx);
	
	byRet = g.m_TTSAudioFlag;
	
	pthread_mutex_unlock(&TTSAduioMtx);
	
	return byRet;
}

void SetTTSAudioStatus(char byStatus)
{
	char byRet = 0;

	pthread_mutex_lock(&TTSAduioMtx);
	
	g.m_TTSAudioFlag = byStatus;
	
	pthread_mutex_unlock(&TTSAduioMtx);
}


char GetAmazonLogInStatus(void)
{
	char byRet = 0;

	pthread_mutex_lock(&LogInMtx);
	
	byRet = g_byAmazonLogInFlag;
	
	pthread_mutex_unlock(&LogInMtx);

	return byRet;
}

void SetAmazonLogInStatus(char byStatus)
{
	pthread_mutex_lock(&LogInMtx);
	
	g_byAmazonLogInFlag = byStatus;
	
	pthread_mutex_unlock(&LogInMtx);
}


char GetAmazonPlayBookFlag(void)
{
	char byRet = 0;

	pthread_mutex_lock(&PlayBookMtx);
	
	byRet = g.m_byPlayBookFlag;
	
	pthread_mutex_unlock(&PlayBookMtx);

	return byRet;
}

void SetAmazonPlayBookFlag(char byStatus)
{
	pthread_mutex_lock(&PlayBookMtx);
	
	g.m_byPlayBookFlag = byStatus;
	
	pthread_mutex_unlock(&PlayBookMtx);
}



char GetAmazonConnectStatus(void)
{
	char byRet = 0;

	pthread_mutex_lock(&g.connect.connect_mtx);
	
	byRet = g.connect.conncet_status;
	
	pthread_mutex_unlock(&g.connect.connect_mtx);

	return byRet;
}

void SetAmazonConnectStatus(char byStatus)
{
	pthread_mutex_lock(&g.connect.connect_mtx);
	
	g.connect.conncet_status = byStatus;
	
	pthread_mutex_unlock(&g.connect.connect_mtx);
}

char GetAudioRequestStatus(void)
{
	char byRet = 0;

	pthread_mutex_lock(&AudioRequestMtx);
	
	byRet = g.m_byAudioRequestFlag;
	
	pthread_mutex_unlock(&AudioRequestMtx);

	return byRet;
}

void SetAudioRequestStatus(char byStatus)
{
	pthread_mutex_lock(&AudioRequestMtx);
	
	g.m_byAudioRequestFlag = byStatus;
	
	pthread_mutex_unlock(&AudioRequestMtx);
}

char GetSpeechStatus(void)
{
	char byRet = 0;

	pthread_mutex_lock(&SpeechMtx);
	
	byRet = g.m_bySpeechFlag;
	
	pthread_mutex_unlock(&SpeechMtx);

	return byRet;
}

void SetSpeechStatus(char byStatus)
{
	pthread_mutex_lock(&SpeechMtx);
	
	g.m_bySpeechFlag = byStatus;
	
	pthread_mutex_unlock(&SpeechMtx);
}



char InitAmazonConnectStatus(void)
{
	if(pthread_mutex_init(&g.connect.connect_mtx, NULL) != 0) {
		DEBUG_PRINTF("Mutex initialization failed!\n");
	}

	g.connect.conncet_status = 0;
}

char *GetBoundary(char *pData, char *pContentType)
{
	char *byBoundary = NULL;

	memcpy(pData, pContentType, strlen(pContentType));

	byBoundary = malloc(66);
	getuuidString(&byBoundary[2]);
	
	strcat(pData, &byBoundary[2]);

	byBoundary[0] = '-';
	byBoundary[1] = '-';
	DEBUG_PRINTF("byBoundary:%s", byBoundary);

	return byBoundary;
}

#define XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

char *GetSynchronizeStateEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	char * json_str = UpDateAlexaCommonJsonStr();
	DEBUG_PRINTF("json_str:%s", json_str);

	g_byBodyBuff = malloc(strlen(json_str) + 1024);
	if (NULL == g_byBodyBuff)
	{
		DEBUG_PRINTF("malloc failed :%d", strlen(json_str));
		return NULL;
	}

	memset(g_byBodyBuff, 0, strlen(json_str) + 1024);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_tokener_parse(json_str);
	free(json_str);
	json_str = NULL;
	if (is_error(root))
	{
		DEBUG_PRINTF("json_tokener_parse failed\n");
		return NULL;
	}

	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("System"));
	json_object_object_add(header, "name", json_object_new_string("SynchronizeState"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );//"messageId-123"
	json_object_object_add(event, "payload", payload = json_object_new_object());

	json_str = strdup(json_object_to_json_string(root));
	
	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	//DEBUG_PRINTF("json_str:%s", json_str);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}

char *GetSpeechStartedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);

	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;
	
	root = json_object_new_object();

	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("SpeechSynthesizer"));
	json_object_object_add(header, "name", json_object_new_string("SpeechStarted"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	if (g.m_SpeechToken != NULL)
		json_object_object_add(payload, "token", json_object_new_string(g.m_SpeechToken));
	else
		json_object_object_add(payload, "token", json_object_new_string(""));

	char * json_str = strdup(json_object_to_json_string(root));

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

	return g_byBodyBuff;
}

char *GetSpeechFinishedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
		if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;
	
	root = json_object_new_object();

	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("SpeechSynthesizer"));
	json_object_object_add(header, "name", json_object_new_string("SpeechFinished"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "token", json_object_new_string(g.m_SpeechToken));

	char * json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}


char *GetSpeechRecognizeEventjsonData(void)
{
	char messageId[64] = {0};
	char dialogRequestId[64] = {0};

	getuuidString(dialogRequestId);
	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	char * json_str = UpDateAlexaCommonJsonStr();

	root = json_tokener_parse(json_str);
	free(json_str);
	json_str = NULL;
	if (is_error(root))
	{
		DEBUG_PRINTF("json_tokener_parse failed\n");
		return NULL;
	}

	event = json_object_object_get(root, "event");

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("SpeechRecognizer"));
	json_object_object_add(header, "name", json_object_new_string("Recognize"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );//"messageId-123"
	json_object_object_add(header, "dialogRequestId", json_object_new_string(dialogRequestId));//"dialogRequestId-123"
	json_object_object_add(event, "payload", payload = json_object_new_object());
	//json_object_object_add(payload, "profile", json_object_new_string("CLOSE_TALK")); // Client determines end of user speech.
	//json_object_object_add(payload, "profile", json_object_new_string("NEAR_FIELD")); // Cloud determines end of user speech. NEAR_FIELD: 0 to 5 ft. FAR_FIELD:0 to 20+ ft.
	json_object_object_add(payload, "profile", json_object_new_string("FAR_FIELD"));
	json_object_object_add(payload, "format",json_object_new_string("AUDIO_L16_RATE_16000_CHANNELS_1"));
	
	char *json_tmp = strdup(json_object_to_json_string(root));
	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	printf(GREEN "request_body is:%s\n" NONE, json_tmp);

	return json_tmp;
}

char *GetPlaybackStartedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_object_new_object();

	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackStarted"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	
	if (NULL == g.CurrentPlayDirective.m_PlayToken) 
		json_object_object_add(payload, "token", json_object_new_string(""));
	else {
		printf(LIGHT_RED "g.m_playtoken1111111:%s\n" NONE, g.CurrentPlayDirective.m_PlayToken);
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));
	}
	
	
	//json_object_object_add(payload, "token", json_object_new_string(g.m_PlayToken));
	//json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(g_running.m_mpd_status.elapsed_ms));
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(g.CurrentPlayDirective.offsetInMilliseconds));

	char * json_str = strdup(json_object_to_json_string(root));
	
	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	if (json_str)
		free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	//strcat(g_byBodyBuff, "--375baad4-d088-44c2-bb20-6c9a31dc828b");
	strcat(g_byBodyBuff, "--");

	printf(GREEN "request_body is:%s\n" NONE, g_byBodyBuff);

	return g_byBodyBuff;
}

char *GetPlaybackPausedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;
	memset(g_byBodyBuff, 0, 2048);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackPaused"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));

	//MpdInit();
	int elapsed_ms = MpdGetCurrentElapsed_ms();
	//MpdDeinit();
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

	return g_byBodyBuff;
}

char *GetPlaybackStopEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackStopped"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));

	//MpdInit();
	int elapsed_ms = MpdGetCurrentElapsed_ms();
	//MpdDeinit();
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

	printf(GREEN "request_body is:%s\n" NONE, g_byBodyBuff);

	return g_byBodyBuff;
}

#ifdef ALERT_EVENT_QUEUE	
char *GetAlertEventJosnData(char *pBoundary,  char *data)

#else
char *GetAlertEventJosnData(char *pBoundary, int iStatus)
#endif
{	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	char *json_str = NULL;

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;
	memset(g_byBodyBuff, 0, 2048);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");
	DEBUG_PRINTF("---------adjakfjkfsk//-");
	
#ifdef ALERT_EVENT_QUEUE	
	strcat(g_byBodyBuff, data);
#else
	DEBUG_PRINTF("iStatus:%d", iStatus);
	switch (iStatus)
	{
		case ALEXA_EVENT_STARTED_ALERT:
			DEBUG_PRINTF("g.ALEXA_EVENT_STARTED_ALERT ilength:%d, %s", strlen(g.m_alert_struct.alert_started_json_str), g.m_alert_struct.alert_started_json_str);
			strcat(g_byBodyBuff, g.m_alert_struct.alert_started_json_str);
			free(g.m_alert_struct.alert_started_json_str);
			g.m_alert_struct.alert_started_json_str = NULL;
			break;

		case ALEXA_EVENT_BACKGROUND_ALERT:
			DEBUG_PRINTF("g.ALEXA_EVENT_BACKGROUND_ALERT ilength:%d, %s", strlen(g.m_alert_struct.alert_background_json_str), g.m_alert_struct.alert_background_json_str);
			strcat(g_byBodyBuff, g.m_alert_struct.alert_background_json_str);
			free(g.m_alert_struct.alert_background_json_str);
			g.m_alert_struct.alert_background_json_str = NULL;
			break;
		case ALEXA_EVENT_FOREGROUND_ALERT:
			DEBUG_PRINTF("g.ALEXA_EVENT_FOREGROUND_ALERT ilength:%d, %s", strlen(g.m_alert_struct.alert_foreground_json_str), g.m_alert_struct.alert_foreground_json_str);
			strcat(g_byBodyBuff, g.m_alert_struct.alert_foreground_json_str);
			free(g.m_alert_struct.alert_foreground_json_str);
			g.m_alert_struct.alert_foreground_json_str = NULL;
			break;

		case ALEXA_EVENT_SET_ALERT:
			DEBUG_PRINTF("g.ALEXA_EVENT_SET_ALERT ilength:%d, %s", strlen(g.m_alert_struct.alert_set_json_str), g.m_alert_struct.alert_set_json_str);
			strcat(g_byBodyBuff, g.m_alert_struct.alert_set_json_str);
			free(g.m_alert_struct.alert_set_json_str);
			g.m_alert_struct.alert_set_json_str = NULL;
			break;
	
		case ALEXA_EVENT_STOP_ALERT:
			DEBUG_PRINTF("g.ALEXA_EVENT_STOP_ALERT ilength:%d, %s", strlen(g.m_alert_struct.alert_stop_json_str), g.m_alert_struct.alert_set_json_str);
			strcat(g_byBodyBuff, g.m_alert_struct.alert_stop_json_str);
			free(g.m_alert_struct.alert_stop_json_str);
			g.m_alert_struct.alert_stop_json_str = NULL;
			break;
		
		case ALEXA_EVENT_DELETE_ALERT:
			DEBUG_PRINTF("g.ALEXA_EVENT_DELETE_ALERT ilength:%d, %s", strlen(g.m_alert_struct.alert_delete_json_str), g.m_alert_struct.alert_set_json_str);
			strcat(g_byBodyBuff, g.m_alert_struct.alert_delete_json_str);
			free(g.m_alert_struct.alert_delete_json_str);
			g.m_alert_struct.alert_delete_json_str = NULL;
			break;
	}
#endif
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}


char *UserInactivityReportEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	char * json_str = UpDateAlexaCommonJsonStr();

	g_byBodyBuff = malloc(strlen(json_str) + 1024);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, strlen(json_str) + 1024);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;


	root = json_tokener_parse(json_str);
	if (is_error(root))
	{
		DEBUG_PRINTF("json_tokener_parse failed\n");
		return NULL;
	}

	event = json_object_object_get(root, "event");

	//DEBUG_PRINTF("######inactiveTime==%d###########\n",inactiveTime);
	
	json_object_object_add(root, "event", event = json_object_new_object());
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("System"));
	json_object_object_add(header, "name", json_object_new_string("UserInactivityReport"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );//"messageId-123"
	json_object_object_add(event, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "inactiveTimeInSeconds", json_object_new_int(inactiveTime));

	json_str = strdup(json_object_to_json_string(root));
	
	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	//DEBUG_PRINTF("json_str:%s", json_str);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;

}

char *GetPlaybackNearlyFinishedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackNearlyFinished"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));

	//MpdInit();
	//int elapsed_ms = MpdGetCurrentElapsed_ms();
	//MpdDeinit();
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(g.m_mpd_status.total_time));

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	
	DEBUG_PRINTF("json_str:%s", g_byBodyBuff);

	return g_byBodyBuff;
}

char *GetPlaybackFinishedEventJosnData(char *pBoundary, char *pData)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackFinished"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());

	DEBUG_PRINTF("<<<<<<<<<<<<<<<<<<111111111111111111");
	
	if (NULL == pData)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(pData));

	//MpdInit();
	//int elapsed_ms = MpdGetCurrentElapsed_ms();
	//MpdDeinit();
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(g.m_mpd_status.total_time));

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	DEBUG_PRINTF("json_str:%s", json_str);

	strcat(g_byBodyBuff, json_str);
	if (json_str)
		free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}


char *GetPlaybackQueueClearEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload, *currentPlaybackState, *error;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackQueueCleared"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	printf(LIGHT_GREEN "g_byBodyBuff:%s\n"NONE, g_byBodyBuff);

	return g_byBodyBuff;
}


char *GetPlaybackFailedEventJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload, *currentPlaybackState, *error;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackFailed"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));

	json_object_object_add(payload, "currentPlaybackState", currentPlaybackState = json_object_new_object());

	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(currentPlaybackState, "token", json_object_new_string(""));
	else
		json_object_object_add(currentPlaybackState, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));

	//MpdInit();
	unsigned elapsed_ms = get_elapsedRec();
	//MpdDeinit();
	json_object_object_add(currentPlaybackState, "offsetInMilliseconds", json_object_new_int(elapsed_ms));
	//json_object_object_add(currentPlaybackState, "playerActivity", json_object_new_string("FINISHED"));
	json_object_object_add(currentPlaybackState, "playerActivity", json_object_new_string(g.m_mpd_status.mpd_state));

	json_object_object_add(payload, "error", error = json_object_new_object());
	json_object_object_add(error, "type", json_object_new_string("MEDIA_ERROR_UNKNOWN"));
	//json_object_object_add(error, "type", json_object_new_string("MEDIA_ERROR_SERVICE_UNAVAILABLE"));
	
	//json_object_object_add(error, "type", json_object_new_string("MEDIA_ERROR_INTERNAL_DEVICE_ERROR"));
	json_object_object_add(error, "message", json_object_new_string("MEDIA_ERROR_UNKNOWN"));// 这里先为空，目前获取不到http错误信息

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(error);
	json_object_put(currentPlaybackState);
	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	printf(LIGHT_GREEN "g_byBodyBuff:%s\n"NONE, g_byBodyBuff);

	return g_byBodyBuff;
}

char *GetVolumeChangedEventJosnData(char *pBoundary, int iStatus)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
		if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;
	
	root = json_object_new_object();

	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("Speaker"));
	if (ALEXA_EVENT_VOLUME_CHANGED == iStatus)
	{
		json_object_object_add(header, "name", json_object_new_string("VolumeChanged"));
	}
	else
	{
		json_object_object_add(header, "name", json_object_new_string("MuteChanged"));
	}
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	g.m_localValume = GetMpcVolume();
	json_object_object_add(payload, "volume", json_object_new_int(g.m_localValume));
	json_object_object_add(payload, "muted", json_object_new_boolean(g.m_AmazonMuteStatus)); // 静音状态

	char * json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}

char *GetPlaybackControllerEventJosnData(char *pBoundary, int iStatus)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	char * json_str = UpDateAlexaCommonJsonStr();

	g_byBodyBuff = malloc(strlen(json_str) + 1024);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, strlen(json_str) + 1024);	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *context, *value, *event, *header, *payload, *temppayload;


	root = json_tokener_parse(json_str);
	if (is_error(root))
	{
		DEBUG_PRINTF("json_tokener_parse failed\n");
		return NULL;
	}
	
	context = json_object_object_get(root, "context");
	value = json_object_array_get_idx(context, 0);
	temppayload = json_object_object_get(value, "payload");

	event = json_object_object_get(root, "event");

	// 添加event字段里面的数据
	json_object_object_add(root, "event", event = json_object_new_object());
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("PlaybackController"));

	json_object_object_add(temppayload, "playerActivity", json_object_new_string(g.m_mpd_status.user_state));

	switch(iStatus)
	{
		case ALEXA_EVENT_MPCPLAYSTOP: 	
			json_object_object_add(header, "name", json_object_new_string("PauseCommandIssued"));
			//json_object_object_add(temppayload, "playerActivity", json_object_new_string("PLAYING"));
			break;

		case ALEXA_EVENT_MPCPREV:
			json_object_object_add(header, "name", json_object_new_string("PreviousCommandIssued"));
			//json_object_object_add(temppayload, "playerActivity", json_object_new_string("PLAYING"));
			break;

		case ALEXA_EVENT_MPCNEXT:
			json_object_object_add(header, "name", json_object_new_string("NextCommandIssued"));
			//json_object_object_add(temppayload, "playerActivity", json_object_new_string("PLAYING"));
			break;
		default:
			break;

	}
	
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );//"messageId-123"
	json_object_object_add(event, "payload", payload = json_object_new_object());

	json_str = strdup(json_object_to_json_string(root));
	
	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	DEBUG_PRINTF("json_str:%s", json_str);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}


char *GetProgressReportDelayElapsed(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("ProgressReportDelayElapsed"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));


	//MpdInit();
	int elapsed_ms = MpdGetCurrentElapsed_ms();
	//MpdDeinit();

	if((g.CurrentPlayDirective.offsetInMilliseconds > 0) && (g.CurrentPlayDirective.delayInMilliseconds > 0))
	{
		json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(elapsed_ms));
	}
	else if((g.CurrentPlayDirective.offsetInMilliseconds <= 0) && (g.CurrentPlayDirective.delayInMilliseconds > 0))
	{
		json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(g.CurrentPlayDirective.delayInMilliseconds));
	}

	char *json_str = strdup(json_object_to_json_string(root));
	g.CurrentPlayDirective.delayInMilliseconds = 0;

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}

char *GetProgressReportIntervalElapsed(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

	g_byBodyBuff = malloc(2048);
	if (NULL == g_byBodyBuff)
		return NULL;

	memset(g_byBodyBuff, 0, 2048);
	
	strcpy(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "\r\n");

	strcat(g_byBodyBuff, "Content-Disposition: form-data; name=\"metadata\"\r\n");
	strcat(g_byBodyBuff, "Content-Type: application/json; charset=UTF-8\r\n\r\n");

	getuuidString(messageId);

	json_object *root, *event, *header, *payload;

	root = json_object_new_object();
	json_object_object_add(root, "event", event = json_object_new_object());

	// 添加event字段里面的数据
	json_object_object_add(event, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("ProgressReportIntervalElapsed"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );
	json_object_object_add(event, "payload", payload = json_object_new_object());
	if (NULL == g.CurrentPlayDirective.m_PlayToken)
		json_object_object_add(payload, "token", json_object_new_string(""));
	else
		json_object_object_add(payload, "token", json_object_new_string(g.CurrentPlayDirective.m_PlayToken));

	//MpdInit();
	int elapsed_ms = MpdGetCurrentElapsed_ms();
	//MpdDeinit();

	/*if((g.CurrentPlayDirective.offsetInMilliseconds > 0) && (g.CurrentPlayDirective.intervalInMilliseconds > 0))
	{
		json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(elapsed_ms));
	}
	else if((g.CurrentPlayDirective.offsetInMilliseconds <= 0) && (g.CurrentPlayDirective.intervalInMilliseconds > 0))
	{
		json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(g.CurrentPlayDirective.intervalInMilliseconds));
	}
	*/
	//json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(1000));

	//int offset = elapsed_ms - g.CurrentPlayDirective.offsetInMilliseconds;
//    g.CurrentPlayDirective.offsetInMilliseconds = elapsed_ms;
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(elapsed_ms));

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	DEBUG_PRINTF("g_byBodyBuff:%s", g_byBodyBuff);

	return g_byBodyBuff;
}

char *GetSettingJosnData(char *pBoundary)
{
	char *g_byBodyBuff = NULL;

	char messageId[64] = {0};

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
	json_object_object_add(header, "namespace", json_object_new_string("Settings"));
	json_object_object_add(header, "name", json_object_new_string("SettingsUpdated"));
	json_object_object_add(header, "messageId", json_object_new_string(messageId) );

	json_object_object_add(event, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "settings", settings = json_object_new_array());
	//json_object_array_add(settings, Create_SettingsUpdatedJsonStr("locale", "de-DE"));
	//json_object_array_add(settings, Create_SettingsUpdatedJsonStr("locale", "en-GB"));
	char *pTemp = ReadConfigValueString("locale");	
	json_object_array_add(settings, Create_SettingsUpdatedJsonStr("locale", pTemp));

	char *json_str = strdup(json_object_to_json_string(root));

	json_object_put(payload);
	json_object_put(header);
	json_object_put(event);
	json_object_put(root);

	strcat(g_byBodyBuff, json_str);
	free(json_str);
	free(pTemp);
	strcat(g_byBodyBuff, "\r\n");
	strcat(g_byBodyBuff, pBoundary);
	strcat(g_byBodyBuff, "--");

	return g_byBodyBuff;
}


#define IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII

#define FILE_PATH "/usr/local/octopusHub/IsAlexaLogin.bin"
void LenovoSpeakersAmazonLogInStatus(char byStatus)
{
	FILE *fp = NULL;
	char byStatusVlue[3] = {0};
	
	sprintf(byStatusVlue, "%d", byStatus);

	fp = fopen(FILE_PATH, "w+");
	if (NULL == fp)
	{
		printf( "fopen %s fialed\n" , FILE_PATH);
		return -1;
	}
	else
	{
		if(fwrite(byStatusVlue, sizeof(byStatusVlue), 1, fp) != 1)
		{
			printf("write %s fialed\n", FILE_PATH);
		}
	}

	fclose(fp);
    fp = NULL;
}

