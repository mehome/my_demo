
#include "dispatcher.h"

#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <assert.h>


#include "event_queue.h"
#include "DeviceStatus.h"

#ifdef ENABLE_TTPOD
#include "TtpodMusic.h"
#endif

#define SEND_MESSAGE_PATH   			"/iot/message/send"
#define REPORT_STATUS_PATH  			"/iot/status/notify"
#define GET_STATUS_PATH 		 		"/iot/status"
#define GET_DATA_PATH 	 		 		"/iot/data"
#define GET_TOPIC_PATH 			 		"/iot/mqtt/topic"
#define GET_AUDIO_PATH 			 		"/v2/iot/audio"
#define COLLECT_SONG_PATH  				"/v2/iot/audio/collect"
#define UPLOAD_FILE_PATH		 	  	"/resources/file/upload?apiKey="
#define VENDOR_AUTHORIZE_PATH   	 	"/vendor/new/authorize"
#define QUERY_DEVICE_STATUS_PATH  		"/vendor/new/device/status"
#define GET_MQTT_INFO_PATH   			"/vendor/mqtt?apiKey="
#define GET_WECHAT_TOKEN   				"/vendor/wechat_token?apiKey="


static int currentEvent = -1;
static pthread_mutex_t currentEventMtx = PTHREAD_MUTEX_INITIALIZER;  

static pthread_t dispatcher_pthread = 0;

static int please_stop = 0;

static char *g_topic = NULL;  
static char *g_clientId = NULL;  
static char *g_fromUser = NULL; 
static char *g_mediaId = NULL; 
static char      *g_deviceId = NULL;
static char      *g_key = NULL;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static pthread_mutex_t     eventQueueMtx = PTHREAD_MUTEX_INITIALIZER;  

static int  eventQueueWait = 0; 

#ifdef ENABLE_TTPOD
static char *gRecResult = NULL;
static IntelChatEvent *gInterEvent = NULL;
static int gCode = 0;
#endif


static void SetCurrentEvent(int event)
{
	pthread_mutex_lock(&currentEventMtx);
	currentEvent= event;
	pthread_mutex_unlock(&currentEventMtx);
}
static void SetEventQueueWait(int wait)
{
	pthread_mutex_lock(&eventQueueMtx);
	eventQueueWait= wait;
	pthread_mutex_unlock(&eventQueueMtx);
}
/* 往队列中添加数据 */
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
/* 
 * 处理收到微信公众号发送过来的微聊信息
 * EVENT_GET_WECHAT_MSG处理函数
 */
static int TuringGetWeChatMsgEvent(void *data)
{
	char *url = (char *)data;

	assert(url != NULL);
	HttpDownLoadFile(url, "/tmp/chat.amr");
			
	amr_dec("/tmp/chat.amr", "/root/chat.raw");
#ifdef ENABLE_ALL_FUNCTION
	text_to_sound(speech);
#else
	eval("mpc", "volume", "101");
	eval("aplay", "-r", "8000", "-f" , "s16_le", "-c" ,"1", "/root/chat.raw");	
	eval("mpc", "volume", "200");
#endif
	free(url);
	return 0;
}
/*
 * 上传本地微聊文件/tmp/8k.amr,会返回mediaId,发送消息会到 
 * EVENT_UPLOAD_FILE处理函数
 */
static int TuringUploadFileEvent(void *data)
{
	char *apiKey = NULL;
	char url[256] = {0};
	amr_enc_file("/tmp/8k.wav", "/tmp/8k.amr");
	apiKey = GetTuringApiKey();
	char *iotHost = GetTuringMqttIotHost();
	snprintf(url, 256, "http://%s%s%s", iotHost, UPLOAD_FILE_PATH, apiKey);
	UploadFile(url, "/tmp/8k.amr", EVENT_UPLOAD_FILE);
    unlink("/tmp/8k.wav");
	return 0;
}
/* 
 * 消息发送,需要到上个函数返回的mediaId 
 * EVENT_SEND_MESSAGE处理函数
 */
static int TuringSendMessageEvent(void *data)
{	

	char *fromUser = GetTuringMqttFromUser();
	char *mediaId = GetTuringMqttMediaId();
	char *deviceId  = GetTuringDeviceId();
	char *apiKey	= GetTuringApiKey();
	info("fromUser:%s",fromUser );
	info("mediaId:%s",mediaId );

	char *body = CreateIntercomMessageReportJson(apiKey ,deviceId, fromUser, mediaId);		
	if(body) {
		info("body:%s",body);
		//body:{ "apiKey": "8e3941df07b14bc183766ebd51a3b8b0", "type": 0, "deviceId": "aiAA8005dfc07621", "message": { "mediaId": "UdeOVIXorbOSOxeAEx8uApxonCNCcnxBIAM4Fnn6QcLLSJ_ifBAjZCIbXSPqkMN6" } }
		char *iotHost = GetTuringMqttIotHost();
		HttpPost(iotHost, SEND_MESSAGE_PATH ,body, EVENT_SEND_MESSAGE);
		free(body); 
	}
		
	return 0;
}
/* 获取mqtt信息的接口 EVENT_GET_MQTT_INFO处理函数 */
static int TuringGetMqttInfoEvent(void *data)
{	
	char url[256] = {0};
	char *apiKey  = GetTuringApiKey();
	char *iotHost = GetTuringMqttIotHost();
	snprintf(url, 256, "http://%s%s%s", iotHost, GET_MQTT_INFO_PATH, apiKey);
	HttpGet(url);		
	return 0;
}
static int TuringQueryDeviceStatusEvent(void *data)
{	
	assert(data != NULL);
	IotEvent *deviceStatus = (IotEvent *)data;
	char *iotHost = GetTuringMqttIotHost();
	HttpPost(iotHost, QUERY_DEVICE_STATUS_PATH,deviceStatus->body, EVENT_QUERY_DEVICE_STATUS);
	freeIotEvent(&deviceStatus);	
	return 0;
}
/* 从队列中获取event,然后根据类别执行相关操作 */
void *DispatcherPthread(void *arg)
{
	QUEUE_ITEM *item = NULL;
	char *iotHost = NULL;
	debug("DispatcherPthread...");
	
	while (1) 
	{  
	    pthread_mutex_lock(&mtx);         
		debug("pthread_mutex_lock(&mtx)...");
	    while (QueueSizes() == 0)   
		{   
			debug("Queue is  empty...");
			SetEventQueueWait(0);							
	        pthread_cond_wait(&cond, &mtx);         		
	       
	    }
		SetEventQueueWait(1);
		debug("Queue is not empty...");
		int size = QueueSizes();
		debug("QueueSizes:%d",size);
		item = QueueGet();
		SetCurrentEvent(item->type);
		debug("item->type:%d",item->type);
		switch(item->type) {
			case EVENT_MQTT_START:
				StartMqttThread(NULL);
				break;
			case EVENT_GET_STAUTS:
				info("EVENT_GET_STAUTS");
				IotEvent *getStatus = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, GET_STATUS_PATH, getStatus->body, item->type);
				freeIotEvent(&getStatus);
				break;
			case EVENT_GET_DATA:
				info("EVENT_GET_DATA");
				IotEvent *data = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, GET_DATA_PATH, data->body, item->type);
				freeIotEvent(&data);
				break;
			case EVENT_GET_AUDIO:
				info("EVENT_GET_DATA");
				IotEvent *audio = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, GET_AUDIO_PATH, audio->body, item->type);
				freeIotEvent(&data);
				break;
			case EVENT_UPLOAD_FILE:
				pthread_mutex_unlock(&mtx);   
				info("EVENT_UPLOAD_FILE");
				TuringUploadFileEvent(item->data);
				pthread_mutex_lock(&mtx);   
				break;
			case EVENT_SEND_MESSAGE:
				info("EVENT_SEND_MESSAGE");
				TuringSendMessageEvent(item->data);
				break;
			case EVENT_REPROT_DEVICE_STATUS:
				info("EVENT_REPROT_DEVICE_STATUS");
				IotEvent *reportStatus = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, REPORT_STATUS_PATH, reportStatus->body, item->type);
				freeIotEvent(&reportStatus);
				break;
			case EVENT_REPROT_AUDIO_STATUS:
				info("EVENT_REPROT_AUDIO_STATUS");
				IotEvent *audioStatus = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, REPORT_STATUS_PATH, audioStatus->body, item->type);
				freeIotEvent(&audioStatus);
				break;
			case EVENT_REPROT_IOT_STATUS:
				info("EVENT_REPROT_IOT_STATUS");
				IotEvent *iotevent = (IotEvent *)item->data;
				warning("iotevent->body:%s",iotevent->body);
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, REPORT_STATUS_PATH, iotevent->body, item->type);
				freeIotEvent(&iotevent);
				break;	
			case EVENT_REQUEST_TOPIC:
				info("EVENT_REQUEST_TOPIC");
				IotEvent *topicStatus = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, GET_TOPIC_PATH, topicStatus->body, item->type);
				freeIotEvent(&topicStatus);
				break;
			case EVENT_GET_WECHAT_MSG:
				info("EVENT_GET_WECHAT_MSG");
				TuringGetWeChatMsgEvent(item->data);
				break;
			case EVENT_VENDOR_AUTHORIZE:
				info("EVENT_VENDOR_AUTHORIZE");
				IotEvent *authorizeStatus = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, VENDOR_AUTHORIZE_PATH, authorizeStatus->body, item->type);
				freeIotEvent(&authorizeStatus);
				break;
			case EVENT_QUERY_DEVICE_STATUS:
				info("EVENT_QUERY_DEVICE_STATUS");
				//TuringQueryDeviceStatusEvent(item->data);
				IotEvent *deviceStatus = (IotEvent *)(item->data);
				char *iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, QUERY_DEVICE_STATUS_PATH, deviceStatus->body,  item->type);
				freeIotEvent(&deviceStatus);	
				break;
			case EVENT_COLLECT_SONG:
				info("EVENT_COLLECT_SONG");
				IotEvent *collectSong = (IotEvent *)(item->data);
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, COLLECT_SONG_PATH, collectSong->body,  item->type);
				freeIotEvent(&collectSong);	
				break;
			case EVENT_GET_MQTT_INFO:
				info("EVENT_GET_MQTT_INFO");
				TuringGetMqttInfoEvent(NULL);
			default:
				break;
		}
		Free_Queue_Item(item);
		debug("Free_Queue_Item(item)");
        pthread_mutex_unlock(&mtx);            
	}  
}
void FromUserInit()
{
	char *fromUser = NULL;
	
	//fromUser = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.TuringFromUser");
    fromUser = cdb_get("$TuringFromUser","xxxx");
	if(fromUser != NULL) {
		if(strcmp(fromUser, "0")) {
			SetTuringMqttFromUser(fromUser);

		}
	}
}
void FromUserDeinit()
{
	
}

/* 启动dispatcher线程 */
int   CreateDispatcherPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	
	
	info("CreateDispatcherPthread");
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*6);
	
	iRet = pthread_create(&dispatcher_pthread, &a_thread_attr, DispatcherPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		info("pthread_create error:%s\n", strerror(iRet));
		return -1;
	}

	
	return 0;
}
/* 销毁dispatcher线程 */
int   DestoryDispatcherPthread(void)
{
	please_stop = 1;
	if (dispatcher_pthread != 0)
	{
		pthread_join(dispatcher_pthread, NULL);
		info("capture_destroy end...\n");
	}	

}



