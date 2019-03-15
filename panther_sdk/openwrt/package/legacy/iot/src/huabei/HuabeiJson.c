#include "HuabeiJson.h"

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include "HuabeiStruct.h"
#include "debug.h"

typedef char* (*JsonCreateCB)(char *key, char *deviceId, void *msg);


typedef struct CreateMsgJsonProc {
    int type;
    JsonCreateCB exec;
}CreateMsgJsonProc;



char *HuabeiJsonUploadFile(char *key, char *deviceId, int type, char *speech)
{
	char * json_str = NULL;
	json_object *root = NULL;
	root = json_object_new_object();

	json_object_object_add(root, "apikey", json_object_new_string(key));
	//json_object_object_add(root, "deviceid", json_object_new_string(deviceId));
	json_object_object_add(root, "type", json_object_new_string("2"));
	json_object_object_add(root, "speech", json_object_new_string(speech));

	json_str = strdup(json_object_to_json_string(root));
	

	json_object_put(root);

	return json_str;
}

char *HuabeiJsonGetTopic(char *key, char *deviceId)
{
	char * json_str = NULL;
	json_object *root = NULL;
	root = json_object_new_object();

	json_object_object_add(root, "apikey", json_object_new_string(key));
	json_object_object_add(root, "deviceid", json_object_new_string(deviceId));

	json_str = strdup(json_object_to_json_string(root));
	

	json_object_put(root);

	return json_str;
}

char *HuabeiJsonNotifyStatus(char *key, char *deviceId, int devstatus)
{
	char * json_str = NULL;
	json_object *root =NULL, *status = NULL;
	root = json_object_new_object();
	status = json_object_new_object();
	json_object_object_add(status, "devstatus", json_object_new_int(devstatus));	

	
	json_object_object_add(root, "apikey", json_object_new_string(key));
	json_object_object_add(root, "deviceid", json_object_new_string(deviceId));
	json_object_object_add(root, "type", json_object_new_int(0));
	
	json_object_object_add(root, "status", status);
	
	json_str = strdup(json_object_to_json_string(root));


	json_object_put(root);

	return json_str;

}

static char *CreateVoiceInterMsgJson(char *key, char *deviceId,  void *arg)
{
	HuabeiSendMsg *msg = (HuabeiSendMsg *)arg;
	json_object *root=NULL, *message= NULL;
	char * json_str = NULL;
	
	root = json_object_new_object();
	json_object_object_add(root, "apikey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(msg->type));

	json_object_object_add(root, "deviceid", json_object_new_string(deviceId));
	if(msg->tousers)
		json_object_object_add(root, "tousers", json_object_new_string(msg->tousers));
	else 
		json_object_object_add(root, "tousers", json_object_new_string(""));
	if(msg->mediaid) {
		message = json_object_new_object();
		json_object_object_add(message, "mediaid", json_object_new_string(msg->mediaid));
		json_object_object_add(root, "message", message);
	}
	
	json_str = strdup(json_object_to_json_string(root));
	json_object_put(root);
	
	return json_str;
}
static char *CreateUnBindMsgJson(char *key, char *deviceId,  void *arg)
{
	HuabeiSendMsg *msg = (HuabeiSendMsg *)arg;
	json_object *root=NULL;
	char * json_str = NULL;
	root = json_object_new_object();
	json_object_object_add(root, "apikey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(msg->type));

	json_object_object_add(root, "deviceid", json_object_new_string(deviceId));
	
	if(msg->tousers)
		json_object_object_add(root, "tousers", json_object_new_string(msg->tousers));
	else 
		json_object_object_add(root, "tousers", json_object_new_string(""));

	json_str = strdup(json_object_to_json_string(root));
	

	json_object_put(root);

	return json_str;
}
static char *CreateSelectSubjectMsgJson(char *key, char *deviceId,  void *arg)
{
	HuabeiSendMsg *msg = (HuabeiSendMsg *)arg;
	json_object *root=NULL, *message= NULL;
	char * json_str = NULL;
	info("...");
	root = json_object_new_object();
	json_object_object_add(root, "apikey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(msg->type));

	json_object_object_add(root, "deviceid", json_object_new_string(deviceId));
	message = json_object_new_object();
	
	if(msg->userkeys) {
		json_object_object_add(message, "userkeys", json_object_new_int(msg->userkeys));
		json_object_object_add(root, "message", message);
	}
	
	if(msg->mediaid) {
		json_object_object_add(message, "mediaid", json_object_new_string(msg->mediaid));
		json_object_object_add(root, "message", message);
	}
	
	json_str = strdup(json_object_to_json_string(root));
	json_object_put(root);
	
	return json_str;
}

static CreateMsgJsonProc procTable[] = {
	{0x00, CreateVoiceInterMsgJson},
	{0x01, CreateUnBindMsgJson},
	{0x02, CreateSelectSubjectMsgJson},
};


char *HuabeiJsonSendMessage(char *key, char *deviceId, void *arg)
{
	char *str = NULL;
	int i = 0;
	int tbl_len = sizeof(procTable) / sizeof(procTable[0]);
	HuabeiSendMsg *msg = (HuabeiSendMsg *)arg;

	info("msg->type:%d",msg->type);
	for(i = 0; i < tbl_len; i++) 
	{
		if( procTable[i].type == msg->type)
		{
			if(procTable[i].exec != NULL) {
				str = procTable[i].exec(key, deviceId, arg);
				break;
			}
			
		}
	}


	return str;
}

