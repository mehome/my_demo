#include "HuabeiMsgMusicStoryHandle.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include "HuabeiJson.h"

#include "debug.h"
#include "mpdcli.h"

enum {
	OPERATE_STOP = 0,
	OPERATE_PLAY ,
	OPERATE_PAUSE,
	OPERATE_RESUME = 4,
};

int MusicStoryHanlder(void *arg)
{
	json_object *root = NULL, *message = NULL;
	
	char *pSub = NULL;
	char *url = NULL;
	int operate ,category = -1;
	char *musicName = NULL;
	int state;
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

	pSub = json_object_object_get(message, "url");
	if(!pSub) {
		error("json_object_object_get url failed");
		goto exit;
	}

	url = json_object_get_string(pSub);
	info("url:%s", url);
	
	pSub = json_object_object_get(message, "arg");
	if(pSub != NULL)  {
		musicName = json_object_get_string(pSub);
		info("musicName:%s", musicName);
	}

	pSub = json_object_object_get(message, "category");
	if(pSub != NULL) {
		category = json_object_get_int(pSub);
		info("category:%d", category);
	}
	
	pSub = json_object_object_get(message, "operate");
	if(pSub != NULL) 
		operate = json_object_get_int(pSub);
	switch(operate) {
		case OPERATE_STOP:
			MpdStop();
			break;
		case OPERATE_PLAY:
			MpdClear();
			MpdAdd(url);
			MpdPlay(-1);
			break;
		case OPERATE_PAUSE:
			MpdPause();
			break;
		case OPERATE_RESUME:
			state = MpdGetPlayState();
			if(state == MPDS_PAUSE) {
				info("MPDS_PAUSE");
				MpdPlay(-1);
			} else {
				MpdClear();
				MpdAdd(url);
				MpdPlay(-1);
			}
			break;
		default:
			break;
	}
	return 0;
exit:
	return -1;
}



