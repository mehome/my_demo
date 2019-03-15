#include "HuabeiMsgOperationControlHandle.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include "HuabeiJson.h"
#include "HuabeiUart.h"

#include "debug.h"


int OperationControlHandler(void *arg)
{
	json_object *root = NULL, *message = NULL;;
	json_object *pSub = NULL;
	int operate ;
	int ret;
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
	if(operate == 0x03) {
		info("mpd play done");
		SetHuabeiSendStop(0);
		cdb_set_int("$turing_mpd_play", 0);
		exec_cmd("mpc clear");
	}
	return HuabeiUartSendCommand(operate);
exit:
	return -1;

}


