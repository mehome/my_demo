
#include "HuabeiJsonParser.h"
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include "debug.h"

int HuabeiJsonParser(char *json)
{
	json_object *root = NULL;
	char *pSub = NULL;
	int type = -1, ret; 
	warning("json:%s",json);
	
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		return -1;
	}

	pSub = json_object_object_get(root, "type");
	if(pSub != NULL) 
		type = json_object_get_int(pSub);
	
	info("type:%d", type);
	
	ret = HuabeiMsgHanlde(type, root);

	if(root)
		json_object_put(root);

	return ret;
}


int HuabeiJsonIotParser(char *json)
{
	json_object *root = NULL, *code = NULL, *payload = NULL;
	char *mediaId = NULL;
	char * clientId=NULL, *pTopic= NULL;

	warning("json:%s",json);
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		return -1;
	}

	code = json_object_object_get(root, "code");
	if(NULL != code) {
		int iCode ;
		iCode = json_object_get_int(code);
		debug("iCode:%d",iCode);
		if(iCode == 0) 
		{
			;//info()
		}
		else if (iCode == -1) 
		{
			struct json_object *desc =NULL;
				
			desc = 	json_object_object_get(root, "desc");
			if(NULL !=  desc) 
			{
				error("desc:%s",json_object_get_string(desc));
			}
			json_object_put(root);
			return -1;
		}
	}

	payload = json_object_object_get(root, "payload");
	if( NULL != payload) {
		char *tmp;
		json_object *media = NULL;
		media = json_object_object_get(payload, "topic");
		if(media == NULL) {
			tmp = json_object_get_string(payload);
			debug("MediaId:%s",tmp );
			SetTuringMqttMediaId(tmp);

			debug("messageId:%s",tmp);
		}
		json_object *client_id=NULL, *topic= NULL;

		client_id = 	json_object_object_get(payload, "clientid");
		if(NULL !=  client_id) 
		{
			clientId = json_object_get_string(client_id);
			debug("clientId:%s",clientId );
			SetTuringMqttClientId(clientId);

		}
		topic = 	json_object_object_get(payload, "topic");
		if(NULL !=  topic) 
		{
			
			pTopic = json_object_get_string(topic);
			debug("topic:%s",pTopic );
			SetTuringMqttTopic(pTopic);
		}
		if(pTopic && clientId) {
			warning("StartMqttThread ");
			StartMqttThread();
		}

	}

	return 0;
}

