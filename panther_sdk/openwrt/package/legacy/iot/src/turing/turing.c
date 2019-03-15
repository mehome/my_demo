#include "turing.h"
#include "request_and_response.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <curl/curl.h>
#include <semaphore.h>
#include <myutils/mystring.h>
#include <errno.h>
#include "processjson.h"
#include "fifobuffer.h"
#include "myutils/debug.h"
#include "common.h"
#include "TuringParam.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_UPLOAD_AMR
#include "amr_enc.h"
#endif

#ifdef ENABLE_OPUS
#include "opus/opus_audio_encode.h"
#include "opus_encode_convert.h"

#endif

#include "CaptureAudioPthread.h"
#include "common.h"

#ifdef ENABLE_YIYA
#include "PlayThread.h"
#endif

extern int g_send_over;
int g_send_fd = 0;
static pthread_t httpRequestPthread = 0; 
static pthread_t httpRecvPthread = 0; 

static sem_t httpRequestSem;
static int httpRequestWait = 0;
static int httpRequestState = HTTP_REQUEST_NONE;
static pthread_mutex_t httpRequestCurlMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t httpRequestMtx = PTHREAD_MUTEX_INITIALIZER;

static sem_t httpRecvSem;
static int httpRecvState = HTTP_RECV_NONE;
static pthread_mutex_t httpRecvMtx = PTHREAD_MUTEX_INITIALIZER;

#ifdef ENABLE_VOICE_DIARY
#define EVENTS_PATH "http://beta.app.tuling123.com/speech/chat"
#else
#define EVENTS_PATH "http://smartdevice.ai.tuling123.com/speech/chat"
#endif

static char *host = "smartdevice.ai.tuling123.com";
//static char *host = "beta.app.tuling123.com";

/* BEGIN: Modified by Frog, 2018-07-14 18:05:54 */
#ifdef USE_CURL

static CURL *httpCurlHandle = NULL;
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)   
{
	return (nmemb * size);
}

static size_t write_Body_data(void *ptr, size_t size, size_t nmemb, void *stream)  
{
	ProcessturingJson((char *)ptr);

	return (nmemb * size);
}
static size_t onHeadData(void *ptr, size_t size, size_t nmemb, void *stream)   
{
	return (nmemb * size);
}
static size_t receive_data_from_server(void *ptr, size_t size, size_t nmemb, void *stream)  
{
	mystring* str = ((mystring *)stream);
	//info("len:%d",size * nmemb);
	string_append(str, ptr, size * nmemb);
	return (nmemb * size);
}
static int  onProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)    
{	 
	
	if(IsHttpRequestCancled()) {
		error("http canled...");
		return -1;
	}
	return 0;	 
}  

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
static int ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)    
{    
	
	if(IsHttpRequestCancled()) {
		error("http canled...");
		return -1;
	}
    return 0;    
}  
/* 上传录制的pcm数据到图灵服务器 */
//static int SubmitSpeechRecognizeEvent(int index, char *pData, int iLength) 
static int send_data_to_server(int index, char *pData, int iLength) 
{
	int iRet = -1;
	CURLcode code;

	CURL *easy_handle = NULL;
	char *token = NULL;
	struct curl_slist *http_headers=NULL;
	struct curl_httppost *postFirst = NULL, *postLast = NULL;	
	
	token = GetTuringToken();
	char *apiKey = GetTuringApiKey();
	char *userId = GetTuringUserId();
	int tone = GetTuringTone();
	int type = GetTuringType();
//	info("CreateRequestJson token :%s",token);
	char *jsonData = CreateRequestJson(apiKey, userId , token, 0, index, 1, tone, type != 1 ? 0 : 1);

	info("jsonData:%s",jsonData);

	code = curl_global_init(CURL_GLOBAL_DEFAULT);
	easy_handle = curl_easy_init();
	if (easy_handle == NULL) 
	{
		error("curl_easy_init() failed");
		return -1;
	}
	//ShareCurlInit();

	//curl_easy_setopt(easy_handle, CURLOPT_SHARE, share_curl);
	curl_easy_setopt(easy_handle, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5);
	curl_easy_setopt(easy_handle, CURLOPT_URL, EVENTS_PATH);

//	error("EVENTS_PATH:%s",EVENTS_PATH);
	http_headers = curl_slist_append(http_headers, "Expect:");
    http_headers = curl_slist_append(http_headers, "Charset: UTF-8");


	curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "parameters",
				CURLFORM_COPYCONTENTS, jsonData,
				CURLFORM_END);
#if defined USE_UPLOAD_AMR
curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "speech",
				CURLFORM_BUFFER, "/tmp/enc.amr",
				CURLFORM_BUFFERPTR, pData,				// 该参数是指向要上传的缓冲区的指针
				CURLFORM_BUFFERLENGTH, iLength,			// 缓冲区的长度
				CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
				CURLFORM_END);
#elif defined ENABLE_OPUS
curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "speech",
				CURLFORM_BUFFER, "/tmp/pcm.opus",
				CURLFORM_BUFFERPTR, pData,				// 该参数是指向要上传的缓冲区的指针
				CURLFORM_BUFFERLENGTH, iLength,			// 缓冲区的长度
				CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
				CURLFORM_END);
#else
curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "speech",
				CURLFORM_BUFFER, "/tmp/pcm.raw",
				CURLFORM_BUFFERPTR, pData,				// 该参数是指向要上传的缓冲区的指针
				CURLFORM_BUFFERLENGTH, iLength,			// 缓冲区的长度
				CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
				CURLFORM_END);

#endif
    curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, http_headers);

#if 0
	curl_easy_setopt(easy_handle, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(easy_handle, CURLOPT_PROGRESSFUNCTION, onProgress);
	curl_easy_setopt(easy_handle, CURLOPT_HEADERFUNCTION, onHeadData);  //CURLOPT_WRITEFUNCTION 将后继的动作交给write_data函数处理	
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_Body_data );  //CURLOPT_WRITEFUNCTION 将后继的动作交给write_data函数处理   
	//curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 4);
#endif
	curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT_MS,4000);
	
	curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST, postFirst);
	iRet = curl_easy_perform(easy_handle); 
	if(iRet != CURLE_OK)		
	{
	    error("Curl curl_easy_perform failed: errno=%d ",iRet);
        //wav_play2("/root/iot/网络超时.wav");
	}
end:

	if(jsonData) {
		free(jsonData);
		jsonData = NULL;
	}
	if(http_headers) {
		curl_slist_free_all(http_headers);
	}

	if(postFirst) {
		curl_formfree(postFirst);
	}
	curl_easy_cleanup(easy_handle);
	curl_global_cleanup();
	debug("exit");
	return iRet;
}

/* 上传录制的pcm数据到图灵服务器 */



//上传最后一次语音数据并接收返回的数据
static int SubmitSpeechEvent(char *jsonData, char *pData, int iLength,void *arg) 
{
	int iRet = -1;
	CURLcode code;
	CURL *easy_handle = NULL;
	char *token = NULL;
	struct curl_slist *http_headers=NULL;
	struct curl_httppost *postFirst = NULL, *postLast = NULL;	
	

	code = curl_global_init(CURL_GLOBAL_DEFAULT);
	easy_handle = curl_easy_init();
	if (easy_handle == NULL) 
	{
		error("curl_easy_init() failed");
		return -1;
	}

	curl_easy_setopt(easy_handle, CURLOPT_URL, EVENTS_PATH);

	//error("EVENTS_PATH:%s",EVENTS_PATH);
	http_headers = curl_slist_append(http_headers, "Expect:");
	http_headers = curl_slist_append(http_headers, "Charset: UTF-8");


	curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "parameters",
				CURLFORM_COPYCONTENTS, jsonData,
				CURLFORM_END);
#if defined USE_UPLOAD_AMR
curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "speech",
				CURLFORM_BUFFER, "/tmp/enc.amr",
				CURLFORM_BUFFERPTR, pData,				// 该参数是指向要上传的缓冲区的指针
				CURLFORM_BUFFERLENGTH, iLength, 		// 缓冲区的长度
				CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
				CURLFORM_END);
#elif defined ENABLE_OPUS
curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "speech",
				CURLFORM_BUFFER, "/tmp/pcm.opus",
				CURLFORM_BUFFERPTR, pData,				// 该参数是指向要上传的缓冲区的指针
				CURLFORM_BUFFERLENGTH, iLength, 		// 缓冲区的长度
				CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
				CURLFORM_END);
#else
curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "speech",
				CURLFORM_BUFFER, "/tmp/pcm.raw",
				CURLFORM_BUFFERPTR, pData,				// 该参数是指向要上传的缓冲区的指针
				CURLFORM_BUFFERLENGTH, iLength, 		// 缓冲区的长度
				CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
				CURLFORM_END);
#endif

	curl_easy_setopt(easy_handle, CURLOPT_NOPROGRESS, false);
	curl_easy_setopt(easy_handle, CURLOPT_PROGRESSFUNCTION, onProgress);
	curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, http_headers);

	curl_easy_setopt(easy_handle, CURLOPT_HEADERFUNCTION, onHeadData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, receive_data_from_server);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, arg);  
	//curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 1);
	curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT_MS,4000);

	curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST, postFirst);
	
	iRet = curl_easy_perform(easy_handle); 
    if(iRet != 0)
    {
        error("remaining data send error");
    }
end:


	if(http_headers) {
		curl_slist_free_all(http_headers);
	}

	if(postFirst) {
		curl_formfree(postFirst);
	}
	curl_easy_cleanup(easy_handle);
	curl_global_cleanup();
	debug("exit");
	return iRet;
}

//将FIFO中残留的录音数据上传给服务器
static int submitSpeechRecognize(int index, int *result, void *arg)
{
	char *pData = NULL;
	int iLength = 0;
    int send_length = 0;
	int read = 0, type = 0, tone = 0;
	char *apiKey = NULL, *token = NULL;
	char *userId = NULL, *jsonData;
	token = GetTuringToken();
	apiKey = GetTuringApiKey();
	userId = GetTuringUserId();
	tone = GetTuringTone();
	type = GetTuringType();
	//info("CreateRequestJson token :%s",token);
	jsonData = CreateRequestJson(apiKey, userId , token, 0, index, 1, tone, type != 1 ? 0 : 1);
    
#if defined(ENABLE_OPUS)
	iLength = OpusEncodeFifoLength();
#else
	iLength = CaptureFifoLength();
#endif
    info("iLength:%d,jsonData:%s",iLength,jsonData);
	if (iLength > 0)
	{

		if (iLength > UPLOAD_SIZE)
		{
			send_length = UPLOAD_SIZE;
		}
        else
        {
            send_length = iLength;
        }
		pData = malloc(send_length);
		
		if (NULL == pData)
		{
			error("malloc failed.");
			goto end;
		}
#if defined(USE_UPLOAD_AMR) || defined(ENABLE_OPUS)
		read = EncodeFifoSeek(pData, send_length);
#else
		read = CaptureFifoSeek(pData, send_length);
#endif	
		warning("total:%d,send:%d,left:%d",iLength,send_length,iLength-send_length);

		if(read != send_length) {
			error("read != iLength");
			goto end;
		}
	}
    else
    {
        pData = malloc(3);
        strcpy(pData,"end");
        send_length = 3;
    }
   
	*result = SubmitSpeechEvent(jsonData, pData, send_length, arg);
	//*result = SubmitSpeechEvent(jsonData, "end", 3, arg);
end:
	if(jsonData) {
		free(jsonData);
		jsonData = NULL;
	}
	if(pData)
		free(pData);
	return read;
}
/* 主动交互接口 */
int submitActiveInteractioEvent() 
{
	int result, read;
	mystring *string = NULL;
	string = string_new();
	if(string == NULL)
		return -1;
	result = send_data_to_server(-1, NULL, 0);
	if(result != 0 ){
		error("string_empty string");
		goto exit;
	}
	if(string_empty(string)) {
		error("string_empty string");
		goto exit;
	}
	int len ;
	char *json = string_data(string, &len);

	ProcessturingJson(json);
exit:
	string_free(string);
	return read;
}
/* 上传最后一次pcm数据的接口 */
//static int submitSpeechRecognizeEvent(int index) 
static int upload_remaining_data_in_fifo(int index) 
{
	int result, read;
	mystring *string = NULL;
	string = string_new();
	if(string == NULL)
		return -1;
#ifdef ENABLE_YIYA
	/* yiya用于播放upload.wav */
	StartPlay();
#endif
    //把最后的数据上传完毕后，接收到反馈回来的数据再继续往下走
	read = submitSpeechRecognize(index, &result, string);
#ifdef ENABLE_YIYA
	/* 等待播放upload.wav完成 */
	WaitForPlayDone();
#endif
	/* 上传超时播放/root/iot/again.wav       */
	if(result != 0)
    {
        wav_play2("/root/iot/网络超时.wav");
#ifdef TURN_ON_SLEEP_MODE	
		InteractionMode();
#else
		ResumeMpdState();
#endif

		error("string_empty string");
		goto exit;
	}
	if(string_empty(string)) {
		error("string_empty string");
		goto exit;
	}
    
    if(IsInterrupt())  //此处适用于：录音、上传虽均已取消，但是数据也已经上传完毕的情况,直接丢弃数据
    {
        info("SetInterruptState");
        SetInterruptState(0);
        goto exit;
    }

	int len ;
	char *json = string_data(string, &len);
    g_send_over = 1;   //最后一帧数据已经上传完毕
	ProcessturingJson(json);
exit:
	string_free(string);

	return read;
}
#else

static int send_data_to_server_by_socket(int index, char *pData, int iLength) 
{
    int ret = -1;
    char *text = NULL;
    char *token = GetTuringToken();
    char *apiKey = GetTuringApiKey();
    char *userId = GetTuringUserId();
    int tone = GetTuringTone();
    int type = GetTuringType();
	info("begin CreateRequestJson ");
    char *jsonData = CreateRequestJson(apiKey, userId , token, 0, index, 1, tone, type != 1 ? 0 : 1);
	info("jsonData:%s",jsonData);

    debug("jsonData:%s",jsonData);
    
    if(NULL == jsonData) 
        return -1;

    ret = turingBuildRequest(g_send_fd, host, pData, iLength,jsonData);
    if(ret < 0) {
        goto EXIT;
    }
    if(IsHttpRequestCancled()) {
        goto EXIT;
    }
#if 0
    ret = getResponse(g_send_fd, &text);
    if (text) {
        ProcessturingJson(text);
        free(text);
    }
#endif    
EXIT:

	if(jsonData) {
		free(jsonData);
		jsonData = NULL;
	}
	//debug("exit");
	return ret;
}
//#define SAVE_UPLOAD_PCM 1

//#define UPLOAD_FILE 1

#ifdef SAVE_UPLOAD_PCM 
FILE *upload_fp = NULL;
#endif
//static int submit_SpeechRecognizeEvent(int index) 
static int get_fifo_data_and_upload_to_server(int index)
{
	char *pData = NULL;
	int iLength = 0;
    int send_length = 0;
	int read_length = 0;
#if defined(ENABLE_OPUS)
	iLength = OpusEncodeFifoLength();
#else
	iLength = CaptureFifoLength();
#endif

	if (iLength > 0)
	{
#if defined(ENABLE_OPUS)
	 send_length = iLength;
#else
		if (iLength > UPLOAD_SIZE)
		{
			send_length = UPLOAD_SIZE;
		}
        else
        {
            send_length = iLength;
        }
#endif       
       
		pData = malloc(send_length);
		
		if (NULL == pData)
		{
			error("malloc failed.");
			goto end;
		}
#if defined(ENABLE_OPUS)
		read_length = OpusEncodeFifoSeek(pData, send_length);
#else
		read_length = CaptureFifoSeek(pData, send_length);
#endif	
		warning("total:%d,send:%d,left:%d",iLength,send_length,iLength-send_length);

		if(read_length != send_length) {
			error("read != send_length");
			goto end;
		}
#ifdef SAVE_UPLOAD_PCM
    fwrite(pData,1,send_length,upload_fp);
    fflush(upload_fp);
#endif

/* BEGIN: Modified by Frog, 2018-7-14 18:1:22 */
#ifdef UPLOAD_FILE
int fd = -1;
int ret;
fd = open("/tmp/upload.opus", O_RDWR);
  if (fd < 0)        // 
     {
         printf("文件打开错误\n");
     }
    else
     {
         printf("文件打开成功，fd = %d.\n", fd);
     }
	if(pData)
		free(pData);
	pData = malloc(1024*10);
	ret = read(fd, pData, 1024*10);
     if (ret < 0)
     {
         printf("read失败\n");
     }
     else
     {
         printf("实际读取了%d字节.\n", ret);
         printf("文件内容是：[%s].\n", pData);
     }
	 send_length = ret;

#endif
#ifdef USE_CURL    
	send_data_to_server(index, pData, send_length);
#else
    send_data_to_server_by_socket(index, pData, send_length);
#endif
/* END:   Modified by Frog, 2018-7-14 18:1:31 */
	}
end:
	if(pData)
		free(pData);
	return read_length;
}
#endif
/* END:   Modified by Frog, 2018-07-14 18:06:00 */

int IsHttpRequestCancled()
{
	int ret;
	pthread_mutex_lock(&httpRequestMtx);
	ret = (httpRequestState == HTTP_REQUEST_CANCLE);
	pthread_mutex_unlock(&httpRequestMtx);
	
	return ret;
}


void SetHttpRequestState(int state)
{
	pthread_mutex_lock(&httpRequestMtx);
	httpRequestState = state;
	pthread_mutex_unlock(&httpRequestMtx);

}

void SetHttpRequestCancle()
{
	if(!IsHttpRequestFinshed()) {
		SetHttpRequestState(HTTP_REQUEST_CANCLE);
	}
    
    if(!IsHttpRecvFinshed())
    {
        SetHttpRecvCancel();
    }

    if(!OpusIsEncodeFinshed())
    {
        SetOpusEncodeCancle();
    }
}

int IsHttpRequestFinshed()
{
	int ret;

	pthread_mutex_lock(&httpRequestMtx);
	ret = (httpRequestState == HTTP_REQUEST_NONE);
	pthread_mutex_unlock(&httpRequestMtx);
	return ret;
}


void SetHttpRequestWait(int wait)
{
	pthread_mutex_lock(&httpRequestMtx);
	httpRequestWait= wait;
	pthread_mutex_unlock(&httpRequestMtx);
}

//1：正在上传      0：已经结束上传
int GetHttpRequestWait()
{
    int ret = 0;
    
    pthread_mutex_lock(&httpRequestMtx);
    ret = (httpRequestWait == 1);
    pthread_mutex_unlock(&httpRequestMtx);

    return ret;
}



void NewHttpRequest()
{
	pthread_mutex_lock(&httpRequestMtx);
	if(httpRequestWait == 0) {
		error("sem_post(&httpRequestSem);");
		sem_post(&httpRequestSem);
	}
	pthread_mutex_unlock(&httpRequestMtx);
}


#ifdef USE_CURL

void CancleHttpRequest()
{
	if(!IsHttpRequestFinshed()) {
		ClearHttpRequestCurlHandle();
	}
	
}


void SetHttpRequestCurlHandle(CURL *handle)
{
	pthread_mutex_lock(&httpRequestCurlMtx);
	if(httpCurlHandle) {
		curl_easy_cleanup(httpCurlHandle); 
	}
	httpCurlHandle = handle;
	pthread_mutex_unlock(&httpRequestCurlMtx);
}
void ClearHttpRequestCurlHandle()
{
	pthread_mutex_lock(&httpRequestCurlMtx);
	if(httpCurlHandle == NULL) {
		info("httpCurlHandle == NULL");
		pthread_mutex_unlock(&httpRequestCurlMtx);
		return ;
	}
	
	info("curl_easy_cleanup httpCurlHandle");
	curl_easy_cleanup(httpCurlHandle); 
	httpCurlHandle = NULL;
	pthread_mutex_unlock(&httpRequestCurlMtx);
}
#endif

void SetHttpRecvState(int state)
{
    pthread_mutex_lock(&httpRecvMtx);
    httpRecvState = state;
    pthread_mutex_unlock(&httpRecvMtx);
}

void SetHttpRecvCancel()
{
    pthread_mutex_lock(&httpRecvMtx);
    httpRecvState = HTTP_RECV_CANCLE;
    pthread_mutex_unlock(&httpRecvMtx);
}


int IsHttpRecvCancled()
{
    int ret = 0;
    pthread_mutex_lock(&httpRecvMtx);
    ret = (httpRecvState == HTTP_RECV_CANCLE);
    pthread_mutex_unlock(&httpRecvMtx);
    return ret;
}

int IsHttpRecvFinshed()
{
    int ret = 0;
    pthread_mutex_lock(&httpRecvMtx);
    ret = (httpRecvState == HTTP_RECV_NONE);
    pthread_mutex_unlock(&httpRecvMtx);
    return ret;
}

void Start_http_recv()
{
    pthread_mutex_lock(&httpRecvMtx);
    error("sem_post(&httpRecvSem)  start");
    sem_post(&httpRecvSem);
    error("sem_post(&httpRecvSem)  over");
    pthread_mutex_unlock(&httpRecvMtx);
}


void *HttpRecvPthread(void *arg)
{
    char *text = NULL;
    int ret = 1;
        
    while(1)
    {   
        info("HttpRecvPthread waiting ........");
        SetHttpRecvState(HTTP_RECV_NONE);
        sem_wait(&httpRecvSem);
        info("HttpRecvPthread starting ........");
        SetHttpRecvState(HTTP_RECV_STARTED);

        while(1)
        {
            if(IsHttpRecvCancled())
            {   
                info("HttpRecvPthread cancled");
                break; 
            }
                
        
            ret = getResponse(g_send_fd,&text);
            if(ret > 0)
            {
                if (text) 
                {
                    ProcessturingJson(text);
                    free(text);
                    text = NULL;
                    break;
                } 
            }
            else if(ret == -1)
            {
                network_timeout();
                break;
            }
        }        
        close(g_send_fd);
        info("HttpRecvPthread done ........");
    }
}



/* 启动http接收数据线程. */
void CreateHttpRecvPthread(void)
{
	int iRet;

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);

	iRet = pthread_create(&httpRecvPthread, &a_thread_attr, HttpRecvPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);

	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}

}

/* 销毁http接收数据线程. */
void DestoryHttpRecvPthread(void)
{

	if (httpRecvPthread != 0)
	{
		pthread_join(httpRecvPthread, NULL);
	}
}



#if defined(ENABLE_OPUS)
static void *HttpRequestPthread(void)
{
	unsigned int iLength = 0;
	unsigned int rLen = 0;
	unsigned int total_len = 0;
	unsigned int encode_len = 0;
	int iCount = 0;
	int send_length = 0;
	char buf[512]= {0};
	while (1)
	{
again:

		info("HTTP_REQUEST_NONE ...");

		SetHttpRequestState(HTTP_REQUEST_NONE);
		SetHttpRequestWait(0);
		total_len = 0;
		iCount = 0;
		iLength = 0;
		/*等待编码，编码完成后，会释放资源，此时开始上传录音的数据*/
		sem_wait(&httpRequestSem);
		//SetHttpRequestState(HTTP_REQUEST_STARTED);
		warning("check and send to server");
		SetHttpRequestState(HTTP_REQUEST_ONGING);
		SetHttpRequestWait(1);
		g_send_fd = get_socket_fd(host);
        if(g_send_fd < 0)
        {
            goto again;
        }
        int flags = fcntl(g_send_fd, F_GETFL, 0); //获取文件的flags值。
        fcntl(g_send_fd, F_SETFL, flags | O_NONBLOCK);//设置成非阻塞模式；
        //通知http接收线程开始接收
        Start_http_recv();  
		
#ifdef SAVE_UPLOAD_PCM
			upload_fp = fopen("/tmp/upload.opus","wb+");
#endif
		while(1) {	
			
			if(IsHttpRequestCancled()) {
				warning("HTTP_REQUEST_CANCLE ...");
				break;
			}
			//将录音数据全部获取出来，保存到iLength
			iLength = OpusEncodeFifoLength();
			//info("OpusEncodeFifoLength =%d",iLength);
			if(iLength == 0) {
				break;
			}
			
			if (iLength > UPLOAD_SIZE)// >32K数据
			{	
				warning("iLength:%d",iLength);
				warning("UPLOAD_SIZE:%d",UPLOAD_SIZE);
				iCount++;
				rLen = get_fifo_data_and_upload_to_server(iCount);
				total_len += rLen;

				iLength = 0;
			}
			else		//	<32K数据
			{	
				if(OpusIsEncodeFinshed())
				{
					iCount++;
					iCount = 0-iCount;
					rLen = get_fifo_data_and_upload_to_server(iCount);
					total_len += rLen;
					break;
				}
			}
		}
		CaptureFifoClear();
		SetHttpRequestState(HTTP_REQUEST_DONE);
		warning("HTTP_REQUEST_DONE ...");

//close(g_send_fd);
#ifdef SAVE_UPLOAD_PCM
    fclose(upload_fp);
#endif   
	}
}

#else
static void *HttpRequestPthread(void)
{
	unsigned int iLength = 0;
	unsigned int rLen = 0;
	unsigned int total_len = 0;
	int iCount = 0;
    int send_length = 0;
	while (1)
	{
again:
		warning("HTTP_REQUEST_NONE ...");
		/* 设置为初始状态. */
		SetHttpRequestState(HTTP_REQUEST_NONE);
		SetHttpRequestWait(0);
		total_len = 0;
		iCount = 0;
        iLength = 0;
#ifdef USE_CURL_MULTI
		MultiInit();
#endif
		/* 等待发起pcm数据上传 */
		sem_wait(&httpRequestSem);
		SetHttpRequestState(HTTP_REQUEST_ONGING);
		SetHttpRequestWait(1);
        g_send_fd = get_socket_fd(host);
        if(g_send_fd < 0)
        {
            goto again;
        }
        int flags = fcntl(g_send_fd, F_GETFL, 0); //获取文件的flags值。
        fcntl(g_send_fd, F_SETFL, flags | O_NONBLOCK);//设置成非阻塞模式；
        
        Start_http_recv();  

#ifdef SAVE_UPLOAD_PCM
    upload_fp = fopen("/tmp/upload.pcm","w+");
#endif
        
		while(1) 
        {
			if(IsHttpRequestCancled()) {
				warning("HTTP_REQUEST_CANCLE ...");
				break;
			}
			iLength = CaptureFifoLength();
			if(iLength == 0) 
            {
				warning("Upload over");
				break;
			}
            
			if ( !IsHttpRequestCancled() &&  iLength > UPLOAD_SIZE)
			{
				iCount++;
				/* 第iCount次上传 */
				rLen = get_fifo_data_and_upload_to_server(iCount);
                //debug("Already send %d times",iCount);
			}
			else
			{	
				if(!IsHttpRequestCancled() && IsCaptureFinshed()) 
				{	
				    warning("The last time--------iCount = %d",iCount);
					iCount++;
					iCount = 0-iCount;
					/* 当录音结束后,且BUFFER剩余数据小于UPLOAD_SIZE, 为上传最后一次(-iCount)数据 */
#ifdef USE_CURL                 

                    rLen =  upload_remaining_data_in_fifo(iCount);
#else
                    get_fifo_data_and_upload_to_server(iCount);
#endif
                    //total_len += rLen;
                    warning("The last time++++++++, %d",iCount);

                    break;
            }
        }
    }
        close(g_send_fd);
#ifdef SAVE_UPLOAD_PCM
    fclose(upload_fp);
#endif        
    //warning("UPLOAD total_len:%u",total_len);
#ifdef USE_CURL_MULTI
		MultiFree();
#endif
	}
}
#endif

/* 启动PCM数据上传线程. */
void CreateHttpRequestPthread(void)
{
	int iRet;

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);

	iRet = pthread_create(&httpRequestPthread, &a_thread_attr, HttpRequestPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);

	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}

}

/* 销毁PCM数据上传线程. */
void DestoryHttpRequestPthread(void)
{

	if (httpRequestPthread != 0)
	{
		pthread_join(httpRequestPthread, NULL);
	}
}

