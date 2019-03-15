#ifndef __HUABEI_DISPATCHER_H__

#define __HUABEI_DISPATCHER_H__

#include "DeviceStatus.h"

#include "debug.h"
#include "common.h"
typedef enum {
	HUABEI_REQUEST_TOPIC_EVENT = 0, 
	HUABEI_NOTIFY_STATUS_EVENT ,
	HUABEI_UPLOAD_FILE_EVENT ,
	HUABEI_SEND_MESSAGE_EVENT ,
	HUABEI_START_MQTT_EVENT,
	HUABEI_REQUEST_DATA_EVENT,
	HUABEI_PREV_NEXT_EVENT,
	HUABEI_PLAY_MUSIC_EVENT,
};


void HuabeiQueuePut(int type, void *data);



#endif

