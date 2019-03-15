#ifndef __CONTROL_H__
#define __CONTROL_H__

#include <json/json.h>

enum {
	NOTIFY_DEVICE_STATUS_REPORT = 0 , // report device status;
	NOTIFY_DEVICE_STATUS_UNBUNDING, //解绑
	NOTIFY_DEVICE_STATUS_BUNDING,  //绑定成功
};


int ParseNotifyData(json_object *message);






#endif

