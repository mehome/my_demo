#include "HuabeiMsgVoiceIntercomHandle.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include "HuabeiJson.h"

#include "debug.h"
#include "HuabeiUart.h"

int VoiceIntercomHandler(void *arg)
{
	int ret = 0;
	json_object *root = NULL, *message = NULL;
	json_object *pSub = NULL;
	char *url = NULL;
	int operate ,category;
	info("...");
	root = (json_object *)arg;
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		ret = -1;
		goto exit;
	}
	message = json_object_object_get(root, "message");
	if(!message) {
		error("json_object_object_get message failed");
		ret = -1;
		goto exit;
	}

	pSub = json_object_object_get(message, "url");
	if(!pSub) {
		error("json_object_object_get url failed");
		ret = -1;
		goto exit;
	}
	url = json_object_get_string(pSub);
	info("url:%s",url);
	pSub = json_object_object_get(message, "category");
	if(pSub) {
		category = json_object_get_int(pSub);
	}
	
	MpdClear();
	MpdAdd(url);
	MpdPlay(-1);

	ret = HuabeiUartSendCommand(HUABEI_COMMAND_GET_WECHAT_MSG);
	return ret;
exit:
	return -1;
}



