
#include "HuabeiDispatcher.h"

#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <assert.h>


#include "event_queue.h"
#include "event.h"

#include "HuabeiDispatcher.h"
#include "HuabeiStruct.h"

static pthread_mutex_t currentEventMtx = PTHREAD_MUTEX_INITIALIZER;  

static pthread_t dispatcherPthread = 0;

static int pleaseStop = 0;

static char      *g_deviceId = NULL;
static char      *g_key = NULL;


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static pthread_mutex_t     eventQueueMtx = PTHREAD_MUTEX_INITIALIZER;  

static int  eventQueueWait = 0; 

#define MESSAGE_SEND_URL  "http://api.drawbo.com/api/v1/iot/message/send"
#define STATUS_REPORT_URL "http://api.drawbo.com/api/v1/iot/status/notify"
#define STATUS_GET_URL    "http://api.drawbo.com/api/v1/iot/status"
#define DATA_GET_URL      "http://api.drawbo.com/api/v1/iot/data"
#define TOPIC_GET_URL     "http://api.drawbo.com/api/v1/iot/mqtt/topic"
#define AUDIO_GET_URL 	   "http://api.drawbo.com/api/v1/iot/audio"
#define FILE_UPLOAD_URL   "http://api.drawbo.com/api/v1/iot/resources/file/upload?apiKey="

#undef SEND_MESSAGE_PATH
#define SEND_MESSAGE_PATH  "/api/v1/iot/message/send"
#undef REPORT_STATUS_PATH
#define REPORT_STATUS_PATH "/api/v1/iot/status/notify"
#undef GET_STATUS_PATH
#define GET_STATUS_PATH 		"/api/v1/iot/status"
#undef GET_DATA_PATH
#define GET_DATA_PATH 	 		"/api/v1/iot/data"
#undef GET_TOPIC_PATH
#define GET_TOPIC_PATH 			"/api/v1/iot/mqtt/topic"
#undef GET_AUDIO_PATH
#define GET_AUDIO_PATH 			"/api/v1/iot/audio"
#undef UPLOAD_FILE_PATH
#define UPLOAD_FILE_PATH		"/api/v1/iot/resources/file/upload?apiKey="


static void SetEventQueueWait(int wait)
{
	pthread_mutex_lock(&eventQueueMtx);
	eventQueueWait= wait;
	pthread_mutex_unlock(&eventQueueMtx);
}

void HuabeiQueuePut(int type, void *data)
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

static int      HuabeiSendMsgEvent(void *arg)
{
		
	HuabeiSendMsg *msg = (HuabeiSendMsg *)arg;
	if(msg == NULL)
		return -1;
	char *mediaid  = GetTuringMqttMediaId();
	char *apikey   = GetHuabeiApiKey();
	char *deviceid = GetHuabeiDeviceId();
	info("msg->type:%d",msg->type);
	if(msg->type != 2) {
		if(mediaid)
			setHuabeiSendMsgMediaid(msg, mediaid);
	}
	info("msg->mediaid:%s",msg->mediaid);
	char *body = HuabeiJsonSendMessage(apikey, deviceid, msg);
	info("body:%s",body);
	if(body) {
		char *iotHost = GetTuringMqttIotHost();
		HttpPost(iotHost, SEND_MESSAGE_PATH ,body, HUABEI_SEND_MESSAGE_EVENT);
		free(body);
	}
	freeHuabeiSendMsg(&msg);
	return 0;
}

int HuabeiRequestDataEvent()
{
	char *url = NULL;
	char *dirName = NULL;
	char data[1024] = {0};
	int operate, ret , res = -1;
	url = GetHuabeiDataUrl();
	if(url == NULL)
		return -1;
	info("url:%s",url);
	dirName = GetHuabeiDataDirName();
	if(dirName == NULL) {
		return -1;
	}	
	info("dirName:%s",dirName);
	res = GetHuabeiDataRes();
	info("res:%d",res);
	if(res != 2) {
		SetHuabeiSendStop(0);
		snprintf(data, 1024 , "%s/%s_mc.mp3", url,  dirName);
		warning("data:%s", data);
#ifdef USE_MPDCLI
		exec_cmd("mpc clear");
		exec_cmd("mpc single on");
		exec_cmd("mpc repeat off");
		eval("mpc","add", data);
		exec_cmd("mpc play");
#else
		MpdClear();	
		MpdRepeat(0);
		MpdSingle(1);
		MpdAdd(data);
		MpdPlay(0);
#endif
	} else {
		HuabeiGetSendDataEvent();
	}

	//SetHuabeiDataRes(-1);
}
static int HuabeiPrevPlayEvent()
{
	char *url = NULL;
	char *dirName = NULL;
	url = GetHuabeiDataUrl();
	if(url == NULL)
		return -1;
	dirName = GetHuabeiDataDirName();
	if(dirName == NULL) {
		return -1;
	}
	MpdClear();
	MpdAdd(url);
	MpdPlay(0);
}
static int HuabeiUploadFileEvent(void *arg)
{
	UploadFileEvent *event = (UploadFileEvent *)arg;
	char url[256] = {0};
	if(event == NULL)
		return -1;
	char *body = event->body;
	char *file = event->file;
	int type   = event->type;
	info("body:%s",body);
	info("file:%s",file);
	char *apiKey = GetHuabeiApiKey();
	//amr_enc_file("/tmp/8k.wav", "/tmp/8k.amr");
	char *iotHost = GetTuringMqttIotHost();
	snprintf(url, 256, "http://%s%s%s", iotHost, UPLOAD_FILE_PATH, apiKey);
	UploadFile(url ,file, body,apiKey,type ,HUABEI_UPLOAD_FILE_EVENT);
	return 0;
}
static int HuabeiPlayMusicEvent(void *arg)
{
	int ret = 0;
	char *buf = NULL;
	buf = (char *)arg;
	assert(buf != NULL);
	ret = HuabeiPlayHttp(buf);
	free(buf);
	return ret;
}



static void HuabeiDispatcherPthread(void *arg)
{
	QUEUE_ITEM *item = NULL;
	char *iotHost = NULL;
	debug("HuabeiDispatcherPthread...");
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
		debug("item->type:%d",item->type);
		switch(item->type) {
			case HUABEI_REQUEST_TOPIC_EVENT:
				info("HUABEI_REQUEST_TOPIC_EVENT");
				IotEvent *topicStatus = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, GET_TOPIC_PATH,topicStatus->body, item->type);
				freeIotEvent(&topicStatus);
				break;
			case HUABEI_UPLOAD_FILE_EVENT:
				info("HUABEI_UPLOAD_FILE_EVENT");
				HuabeiUploadFileEvent(item->data);
				break;
			case HUABEI_SEND_MESSAGE_EVENT:
				info("HUABEI_SEND_MESSAGE_EVENT");
				pthread_mutex_unlock(&mtx);
				HuabeiSendMsgEvent(item->data);
				pthread_mutex_lock(&mtx);
				break;
			case HUABEI_NOTIFY_STATUS_EVENT:
				info("HUABEI_REPORT_STATUS_EVENT");
				IotEvent *audioStatus = (IotEvent *)item->data;
				iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, REPORT_STATUS_PATH, audioStatus->body, item->type);
				freeIotEvent(&audioStatus);
				break;
			case HUABEI_START_MQTT_EVENT:
				StartMqttThread(NULL);
				break;
			case HUABEI_REQUEST_DATA_EVENT:
				HuabeiRequestDataEvent(NULL);
				break;
			case HUABEI_PREV_NEXT_EVENT:
				HuabeiPrevPlayEvent(NULL);
				break;
			case HUABEI_PLAY_MUSIC_EVENT:
				info("HUABEI_PLAY_MUSIC_EVENT");
				HuabeiPlayMusicEvent(item->data);
				break;
			default:
				break;
		}
		Free_Queue_Item(item);
		debug("Free_Queue_Item(item)");
        pthread_mutex_unlock(&mtx);            
	}  
}




int   CreateHuabeiDispatcherPthread(void)
{
	int iRet;
	pthread_attr_t attr; 

	info("...");
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN*6);
	
	iRet = pthread_create(&dispatcherPthread, &attr, HuabeiDispatcherPthread, NULL);
	pthread_attr_destroy(&attr);
	if(iRet != 0)
	{
		info("pthread_create error:%s\n", strerror(iRet));
		return -1;
	}
	
	return 0;
}
int   DestoryHuabeiDispatcherPthread(void)
{
	pleaseStop = 1;
	if (dispatcherPthread != 0)
	{
		pthread_join(dispatcherPthread, NULL);
		info("capture_destroy end...\n");
	}	
}





