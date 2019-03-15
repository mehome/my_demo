

#include "notify.h"


#include <stdio.h>

#include "myutils/debug.h"
#include "common.h"
#include "mpdcli.h"
#include "DeviceStatus.h"

static int DeviceStatusReport(void *arg)
{
	IotDeviceStatusReport();
	return 0;
}

static int DeviceUnBunding(void *arg)
{
	info("...");
	int pause = 0;
	int state = MpdGetPlayState();
	if(state == MPDS_PLAY) {
		pause = 1;
		MpdPause();
	}
	cdb_set_int("$device_status", 0);
	wav_play2("/root/iot/unbind.wav");
	if(pause) {
		warning("resume play...");
		MpdPlay(-1);
	}
	unlink("/etc/config/wechat.json");
	TuringWeChatListDeinit();
	return 0;
}


static int DeviceBunding(void *arg)
{
	info("...");
	int pause = 0;
	int state = MpdGetPlayState();
	if(state == MPDS_PLAY) {
		pause = 1;
		MpdPause();
	}
	cdb_set_int("$device_status", 1);
	wav_play2("/root/iot/bind_success.wav");
	if(pause) {
		warning("resume play...");
		MpdPlay(-1);
	}
	TuringWeChatListInit();
	return 0;
}

int ParseNotifyData(json_object *message)
{
	int ret = -1;
	struct json_object *operate = NULL, *arg = NULL;
	operate =  json_object_object_get(message, "operate");
	warning("message:%s",json_object_to_json_string(message));
	if(operate) {
		int iOperate; 
		iOperate = json_object_get_int(operate);
		warning("iOperate:%d",iOperate);
		switch(iOperate) {
			case NOTIFY_DEVICE_STATUS_REPORT:
				ret = DeviceStatusReport(message);//报告设备接口状态
				break;
			case NOTIFY_DEVICE_STATUS_UNBUNDING:
				ret = DeviceUnBunding(message);	//解绑成功
				break;
			case NOTIFY_DEVICE_STATUS_BUNDING:
				ret = DeviceBunding(message);	//绑定成功
				break;	
			default:
				error("unsupport message");
				break;	
		}

		
	}
	return ret;
}



