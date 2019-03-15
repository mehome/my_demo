
#include "HuabeiPostCallback.h"
#include <json/json.h>

#include "HuabeiDispatcher.h"
#include "debug.h"
#include "callback.h"


static int RequestTopicCallback(void *arg);
static int UploadFileCallback(void *arg) ;
static int SendMessageCallback(void *arg) ;


static Callback postCallbackTable[] = {
	{HUABEI_REQUEST_TOPIC_EVENT, RequestTopicCallback},
	{HUABEI_SEND_MESSAGE_EVENT, SendMessageCallback},
	{HUABEI_UPLOAD_FILE_EVENT, UploadFileCallback},
	
};
	
static int UploadFileCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	json_object *clientId = NULL, *topic = NULL;
	char *mediaId = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
		mediaId = json_object_get_string(payload);
		SetTuringMqttMediaId(mediaId);
	}
	return ret;
}

static int SendMessageCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL;
	json_object *val = NULL;
	int ret = -1, code = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val != NULL) {
		ret = json_object_get_int(val);
	}
	if(ret != 0)
		return ret;
	payload = json_object_object_get(root, "payload");
	if(payload != NULL) {
		json_object *val = json_object_object_get(payload, "url");
		if(val) {
			char *url = json_object_get_string(val);
			info("url:%s",url );		
			char buf[128] = {0};
			char *ptr = strrchr(url, '/');
			if(ptr) {
				char *name = ptr+1;
				info("name:%s",name);
				SetHuabeiDataDirName(name);
			}
			SetHuabeiDataUrl(url);
			HuabeiRequestDataEvent();
		}

	}
	return ret;
}


static int RequestTopicCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	json_object *clientid = NULL, *topic = NULL;
	char *pClientid = NULL, *pTopic = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
		clientid = json_object_object_get(payload, "clientid");
		if(NULL !=  clientid) 
		{
			pClientid = json_object_get_string(clientid);
			info("clientId:%s",pClientid );
			SetTuringMqttClientId(pClientid);
		}
		topic = json_object_object_get(payload, "topic");
		if(NULL !=  topic) 
		{
			
			pTopic = json_object_get_string(topic);
			info("topic:%s",pTopic );
			SetTuringMqttTopic(pTopic);

		}
		if(pTopic && pClientid) {
			info("StartMqttThread");
			StartMqttThread();
		}
	}


	return ret;
}


int ExecHuabeiPostCallback(int type, void *arg)
{
	int ret = -1;
	int i = 0;
		
	int tbl_len = sizeof(postCallbackTable) / sizeof(postCallbackTable[0]);
	debug("type:%d" ,type);
	debug("tbl_len:%d" ,tbl_len);
	for(i = 0; i < tbl_len; i++) 
	{
		if( postCallbackTable[i].type == type)
		{
			
			if(postCallbackTable[i].callback != NULL) {
				ret = postCallbackTable[i].callback(arg);	
			}
			break;
			
		}
	}

	return ret;
}



