#include "HuabeiMsgUserInterfaceControlHandle.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>


#include "debug.h"

enum {
	OPERATE_VOLUME_DECREASE = 0,
	OPERATE_VOLUME_INCREASE ,
	OPERATE_VOLUME_VOLUME,
	OPERATE_BLN_TURN_ON,
	OPERATE_BLN_TURN_OFF,
};



int UserInterfaceControlHandler(void *arg)
{
	
	json_object *root = NULL, *message = NULL;

	json_object *pSub = NULL;
	char* volumeVal = NULL;
	int operate,volume = 50;
	info("...");

	root = (json_object *)arg;
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		goto exit;
	}
	message = json_object_object_get(root, "message");
	if(!message) {
		error("json_object_object_get message failed");
		goto exit;
	}
	pSub = json_object_object_get(message, "operate");
	if(pSub != NULL) 
		operate = json_object_get_int(pSub);
	info("operate:%d",operate);
	pSub = json_object_object_get(message, "arg");
	if(pSub != NULL) {
		volumeVal = json_object_get_string(pSub);
		volume = atoi(volumeVal);
		info("volumeVal:%s, volume:%d",volumeVal, volume);
	}
	
	switch(operate) {
		case OPERATE_VOLUME_DECREASE:
			exec_cmd("mpc volume -10");
			break;
		case OPERATE_VOLUME_INCREASE:
			exec_cmd("mpc volume +10");
			break;
		case OPERATE_VOLUME_VOLUME:
			MpdVolume(volume);
			break;
		case OPERATE_BLN_TURN_ON:
			info("OPERATE_BLN_TURN_ON");
			break;
		case OPERATE_BLN_TURN_OFF:
			info("OPERATE_BLN_TURN_OFF");
			break;
		default:
			break;
	}

	
	return 0;
exit:
	return -1;

}


