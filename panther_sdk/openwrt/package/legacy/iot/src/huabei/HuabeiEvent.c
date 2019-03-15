#include "HuabeiEvent.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include "HuabeiJson.h"

#include "debug.h"

#include "HuabeiDispatcher.h"
#include "HuabeiStruct.h"
#include "huabei.h"
#include "HuabeiUart.h"

static char *huabieApikey = "of6dohiwn38eifq30jqrzq0rhcsdxolo";
static char *deviceId = NULL;

int HuabeiEventUploadFile(int type, char *speech)
{
    char *body = NULL;
	char *apikey = NULL;	
	UploadFileEvent * event = NULL;

	apikey = GetHuabeiApiKey();
	
	body = HuabeiJsonUploadFile(apikey, deviceId, type, speech);
	info("body:%s",body);
	event = newUploadFileEvent(body, strdup(speech), type);
	if(event) {
		HuabeiQueuePut(HUABEI_UPLOAD_FILE_EVENT, (void *)event);
		info("QueuePut HUABEI_UPLOAD_FILE_EVENT");
	}
	return 0;
}
int HuabeiEventGetTopic()
{
	char *body = NULL;
	char *apikey = NULL;	
	IotEvent * event = NULL;
	apikey = GetHuabeiApiKey();
	
	body = HuabeiJsonGetTopic(apikey, deviceId);
	info("body:%s",body);
	event = newIotEvent(body);
	if(event) {
		HuabeiQueuePut(HUABEI_REQUEST_TOPIC_EVENT, (void *)event);
		info("QueuePut HUABEI_REQUEST_TOPIC_EVENT");
	}
	return 0;
}
int HuabeiEventNotifyStatus(int devstatus)
{
	char *body = NULL;
	char *apikey = NULL;	
	IotEvent * event = NULL;
	apikey = GetHuabeiApiKey();
	body = HuabeiJsonNotifyStatus(apikey, deviceId, devstatus);
	info("body:%s",body);
	event = newIotEvent(body);
	if(event) {
		HuabeiQueuePut(HUABEI_NOTIFY_STATUS_EVENT, (void *)event);
		info("QueuePut HUABEI_SEND_MESSAGE_EVENT");
	}
	return 0;
}
int HuabeiEventSendMessage(void *arg)
{	
	HuabeiSendMsg *msg = NULL;
	msg = dupHuabeiSendMsg(arg);
	if(msg) {
		info("HUABEI_MESSAGE_SEND_SELECT_SUBJECT");
		HuabeiQueuePut(HUABEI_SEND_MESSAGE_EVENT, (void *)msg);
		info("QueuePut HUABEI_SEND_MESSAGE_EVENT");
	}

	return 0;
}

int HuabeiEventRequestData()
{	
	HuabeiQueuePut(HUABEI_REQUEST_DATA_EVENT, NULL);
	return 0;
}
int HuabeiEventPrevNext()
{	
	HuabeiQueuePut(HUABEI_PREV_NEXT_EVENT, NULL);
	return 0;
}


int HuabeiEventSelectSubject(char *buf)
{
	HuabeiSendMsg *msg;
	msg = newHuabeiSendMsg();
	if(msg) {
		msg->type = HUABEI_MESSAGE_SEND_SELECT_SUBJECT;
		setHuabeiSendMsgMediaid(msg, strdup(buf));
		HuabeiEventSendMessage(msg);
		freeHuabeiSendMsg(&msg);
	}
	return 0;
}

int HuabeiHttpPrevNext(int userkeys)
{
	HuabeiSendMsg *msg;
	info("....");
	msg = newHuabeiSendMsg();
	if(msg) {
		info("HUABEI_MESSAGE_SEND_SELECT_SUBJECT");
		msg->type = HUABEI_MESSAGE_SEND_SELECT_SUBJECT;
		msg->userkeys = userkeys;
		HuabeiEventSendMessage(msg);
		freeHuabeiSendMsg(&msg);
	}
	return 0;
}

int HuabeiEventPlayEvent(char *buf)
{
	HuabeiQueuePut(HUABEI_PLAY_MUSIC_EVENT, (void *)buf);
	return 0;
}

int HuabeiEventInit()
{
	//huabieApikey = GetHuabeiApiKey();
	HuabeiDataInit();
	deviceId = GetTuringDeviceId();
	return HuabeiEventGetTopic();
}

int HuabeiEventMqttConnected()
{
	int connect = cdb_get_int("$mqtt_connect", 0);
	info("connect:%d",connect);
	HuabeiUartSendCommand(HUABEI_COMMAND_CONNECT_SERVER);
	HuabeiEventNotifyStatus(0x80);
	if(connect == 1) 
		return 0;
	cdb_set_int("$mqtt_connect", 1);

	PlayBootMusicProc(NULL);

}


