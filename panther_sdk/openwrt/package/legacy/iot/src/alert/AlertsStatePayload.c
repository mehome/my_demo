
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "Alert.h"
#include "AlertsStatePayload.h"
#include "list_iterator.h"
#include "myutils/debug.h"


AlertsStatePayload *alertsstatepayload_new(list_t *allAlerts ,list_t *activeAlerts)
{	

	AlertsStatePayload *payload = NULL;
	payload = calloc(1, sizeof(AlertsStatePayload));

	if(NULL == payload) {
		error("calloc AlertsStatePayload failed");
		return NULL;
	}
	if(payload->activeAlerts)
		list_destroy(payload->activeAlerts);
	if(payload->allAlerts)
		list_destroy(payload->allAlerts);
	payload->activeAlerts = activeAlerts;
	payload->allAlerts    = allAlerts;

	return payload;
}

list_t *alertsstatepayload_get_all_alerts(AlertsStatePayload *payload)
{
	return payload->allAlerts;
}

list_t *alertsstatepayload_get_active_alerts(AlertsStatePayload *payload)
{
	return payload->activeAlerts;
}



void alertsstatepayload_free(AlertsStatePayload *payload)
{
	if(NULL != payload) {
		if(NULL != payload->activeAlerts)
			list_destroy(payload->activeAlerts);
		if(NULL != payload->allAlerts)
			list_destroy(payload->allAlerts);
		free(payload);
		payload = NULL;
	}
}


struct json_object * alertsstatepayload_to_activealerts(AlertsStatePayload *payload)
{
	struct json_object *activeAlerts=NULL;
	int len;
	list_iterator_t *it = NULL;


	activeAlerts = json_object_new_array();
	if(NULL == activeAlerts) {
		error("json_object_new_array failed");
		return NULL;
	}


	len = list_length(payload->activeAlerts);
	
	it = list_iterator_new(payload->activeAlerts,LIST_HEAD);

	while(list_iterator_has_next(it)) {
		Alert *alert = NULL ;
		struct json_object *val = NULL;

		char *token 		= NULL;
		char *repate 		= NULL;
		char *scheduledTime = NULL;

		alert = (Alert *)list_iterator_next_value(it);
	
		token			= alert_get_token( alert);
		repate			= alert_get_repate(alert);
		scheduledTime	= alert_get_scheduledtime(alert);

		val = json_object_new_object();
		if(NULL == val) {
			error("json_object_new_object failed");
			return NULL;
		}

		json_object_object_add(val, "token", json_object_new_string(token));
		json_object_object_add(val, "repate", json_object_new_string(repate));
		json_object_object_add(val, "scheduledTime", json_object_new_string(scheduledTime));

		
		json_object_array_add(activeAlerts,val);
		
		
	}
	if(it)
		list_iterator_destroy(it);
	return activeAlerts;
}


struct json_object * alertsstatepayload_to_allalerts(AlertsStatePayload *payload)
{

	struct json_object *allAlerts=NULL;

	list_iterator_t *it = NULL;

	allAlerts = json_object_new_array();
	if(NULL == allAlerts) {
		error("json_object_new_array failed");
		return NULL;
	}
	
	it = list_iterator_new(payload->allAlerts,LIST_HEAD);

	while(list_iterator_has_next(it )) {
		Alert *alert = NULL ;
		struct json_object *val = NULL;

		char *token 		= NULL;
		char *repate 		= NULL;
		char *scheduledTime = NULL;

		alert = (Alert *)list_iterator_next_value(it);
	
		token 			= alert_get_token( alert);
		repate 				= alert_get_token( alert);
		scheduledTime 	= alert_get_scheduledtime(alert);

		info("list token:%s,repate:%s,schedulerTime:%s",token, repate, scheduledTime);

		val = json_object_new_object();
		if(NULL == val) {
			error("json_object_new_object failed");
			return NULL;
		}

		json_object_object_add(val, "token", json_object_new_string(token));
		json_object_object_add(val, "repate", json_object_new_string(repate));
		json_object_object_add(val, "scheduledTime", json_object_new_string(scheduledTime));

		
		json_object_array_add(allAlerts,val);
		
		
	}
	if(it)
		list_iterator_destroy(it);
	
	return allAlerts;
}





