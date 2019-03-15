	
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <json/json.h>
#include <openssl/rand.h>

#include "RequestFactory.h"
#include "../uuid.h"

extern char* last_token;

static int get_uuid(char *id)
{	
	getuuidString(id);

	return 0;
}


static struct json_object * create_header(char * namespace, char * name)
{
	struct json_object *header=NULL;
	
	header = json_object_new_object();
	if(header == NULL) {
		DEBUG_ERR("json_object_new_object header failed");
		return NULL;
	}
	json_object_object_add(header, "namespace", json_object_new_string(namespace));
	json_object_object_add(header, "name", json_object_new_string(name));

	DEBUG_INFO("header:%s",json_object_to_json_string(header));
	return header;
}




static struct json_object *create_alert_payload(char *alertToken)
{
	struct json_object *payload=NULL;
	if(alertToken == NULL) {
		DEBUG_INFO("alertToken:%s",alertToken);
	} else {
		DEBUG_INFO("alertToken:is NULL",alertToken);
		return NULL;
	}
	payload = json_object_new_object();
	if(payload == NULL) {
		DEBUG_ERR("json_object_new_object payload failed");
		return NULL;
	}
	json_object_object_add(payload, "token", json_object_new_string(alertToken));
	DEBUG_INFO("header:%s",json_object_to_json_string(payload));
	
	return payload;
	
}

static  struct json_object *create_event(struct json_object *header, struct json_object *payload)
{
	struct json_object *event=NULL, *val = NULL;
	event = json_object_new_object();
	if(event == NULL) {
		DEBUG_ERR("json_object_new_object event failed");
		return NULL;
	}
	val = json_object_new_object();
	if(val == NULL) {
		DEBUG_ERR("json_object_new_object val failed");
		return NULL;
	}
	json_object_object_add(val, "header", header);
	json_object_object_add(val, "payload", payload);

	json_object_object_add(event, "event", val);
	
	DEBUG_INFO("event:%s",json_object_to_json_string(event));
	
	return event;
}

static char * create_alerts_event(char * name, char * alertToken)
{
	DEBUG_INFO("################## last_token = %s",alertToken);
	char *requestBody = NULL;
	char messageId[36]={0};
	struct json_object *header =NULL ,*payload=NULL, *event=NULL;
	header = create_header(ALERTS_NAMESPACE,name);
	if(NULL == header) {
		DEBUG_ERR("create_header failed");
		return NULL;
	}

	get_uuid(messageId);
	json_object_object_add(header,"messageId",json_object_new_string(messageId));
	DEBUG_INFO("header:%s",json_object_to_json_string(header));

	//payload = create_alert_payload(alertToken);
	//if(NULL == payload) {
	//	DEBUG_ERR("create_alert_payload failed");
	//	return NULL;
	//}
	 payload = json_object_new_object();
	if(NULL == payload) {
		DEBUG_ERR("json_object_new_object failed");
		return NULL;
	}

	json_object_object_add(payload, "token", json_object_new_string(alertToken));
	DEBUG_INFO("payload:%s",json_object_to_json_string(payload));

	event = create_event(header,payload);
	if(NULL == event) {
		DEBUG_ERR("create_alert_payload failed");
		return NULL;
	}
	
	requestBody = strdup(json_object_to_json_string(event));
	json_object_put(header);
	json_object_put(payload);
	json_object_put(event);

	return requestBody;
	
}


static  char * create_speech_synthesizer_event(char *name, char * alertToken)
{
	DEBUG_INFO("################## last_token = %s",alertToken);

	char *requestBody = NULL;
	char messageId[36]={0};
	struct json_object *header =NULL ,*payload=NULL, *event=NULL;
	header = create_header(SPEECH_SYNTHESIZER_NAMESPACE,name);
	if(NULL == header) {
		DEBUG_ERR("create_header failed");
		return NULL;
	}

	get_uuid(messageId);
	json_object_object_add(header,"messageId",json_object_new_string(messageId));
	DEBUG_INFO("header:%s",json_object_to_json_string(header));

		//payload = create_alert_payload(last_token);
	payload = json_object_new_object();
	if(NULL == payload) {
		DEBUG_ERR("json_object_new_object failed");
		return NULL;
	}

	json_object_object_add(payload, "token", json_object_new_string(alertToken));
	DEBUG_INFO("header:%s",json_object_to_json_string(payload));
	

	event = create_event(header,payload);
	if(NULL == event) {
		DEBUG_ERR("create_alert_payload failed");
		return NULL;
	}
	
	requestBody = strdup(json_object_to_json_string(event));
	json_object_put(header);
	json_object_put(payload);
	json_object_put(event);

	return requestBody;
}



char *  requestfactory_create_alerts_set_alert_event(char * alertToken, int success) 
{
		DEBUG_INFO("...");
	if(success)
		return	create_alerts_event(ALERTS_EVENT_SET_ALERT_SUCCEEDED, alertToken);
	else
		return	create_alerts_event(ALERTS_EVENT_SET_ALERT_FAILED, alertToken);
		
}


char *  requestfactory_create_alerts_delete_alert_event(char * alertToken,int success)
{
		DEBUG_INFO("...");
	if(success)
		return	create_alerts_event(ALERTS_EVENT_DELETE_ALERT_SUCCEEDED, alertToken);
	else
		return	create_alerts_event(ALERTS_EVENT_DELETE_ALERT_FAILED, alertToken);
}


char *  requestfactory_create_alerts_alert_started_event(char * alertToken) 
{
		DEBUG_INFO("...");
	return create_alerts_event(ALERTS_EVENT_ALERT_STARTED, alertToken);
}

char *  requestfactory_create_alerts_alert_stopped_event(char * alertToken) 
{
		DEBUG_INFO("...");
	return create_alerts_event(ALERTS_EVENT_ALERT_STOPPED, alertToken);
}



char *  requestfactory_create_alerts_entered_foreground_event(char * alertToken) 
{
		DEBUG_INFO("...");
	return create_alerts_event(ALERTS_EVENT_ALERT_ENTERED_FOREGROUND, alertToken);
}

char *  requestfactory_create_alerts_entered_background_event(char * alertToken) 
{
		DEBUG_INFO("...");
	return create_alerts_event(ALERTS_EVENT_ALERT_ENTERED_BACKGROUND, alertToken);
}

char *  requestfactory_create_speech_synthesizer_speech_started_event(char * alertToken)
{	DEBUG_INFO("################## last_token = %s",alertToken);
	return create_speech_synthesizer_event(SPEECH_SYNTHESIZER_EVENT_SPECCH_STARTED, alertToken);
}

char *  requestfactory_create_speech_synthesizer_speech_finished_event(char * alertToken)
{
	return create_speech_synthesizer_event(SPEECH_SYNTHESIZER_EVENT_SPECCH_FINISHED, alertToken);
}












