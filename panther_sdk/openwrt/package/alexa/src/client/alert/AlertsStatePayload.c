
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "Alert.h"
#include "AlertsStatePayload.h"
#include "list_iterator.h"
#include "alert_api.h"

AlertsStatePayload *alertsstatepayload_new(list_t *allAlerts ,list_t *activeAlerts)
{	
//	DEBUG_INFO("...");
	AlertsStatePayload *payload = NULL;
	payload = calloc(1, sizeof(AlertsStatePayload));

	if(NULL == payload) {
		DEBUG_ERR("calloc AlertsStatePayload failed");
		return NULL;
	}

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

	//DEBUG_INFO("...");
	activeAlerts = json_object_new_array();
	if(NULL == activeAlerts) {
		DEBUG_INFO("json_object_new_array failed");
		return NULL;
	}
	//

	len = list_length(payload->activeAlerts);
	//DEBUG_INFO("#################################################payload->activeAlerts:len = %d", len);
	it = list_iterator_new(payload->activeAlerts,LIST_HEAD);

	while(list_iterator_has_next(it)) {
		Alert *alert = NULL ;
		struct json_object *val = NULL;

		char *token 		= NULL;
		char *type 			= NULL;
		char *scheduledTime = NULL;

		alert = (Alert *)list_iterator_next_value(it);
	
		token			= alert_get_token( alert);
		type			= alert_get_type( alert);
		scheduledTime	= alert_get_scheduledtime(alert);


		DEBUG_INFO("list token:%s,type:%s,schedulerTime:%s",token, type, scheduledTime);

		val = json_object_new_object();
		if(NULL == val) {
			DEBUG_INFO("json_object_new_object failed");
			return NULL;
		}

		json_object_object_add(val, "token", json_object_new_string(token));
		json_object_object_add(val, "type", json_object_new_string(type));
		json_object_object_add(val, "scheduledTime", json_object_new_string(scheduledTime));

		
		json_object_array_add(activeAlerts,val);
		
		
	}
	list_iterator_destroy(it);

	//DEBUG_INFO("allAlerts :%s",json_object_to_json_string(activeAlerts));
	return activeAlerts;
}


struct json_object * alertsstatepayload_to_allalerts(AlertsStatePayload *payload)
{

	struct json_object *allAlerts=NULL;

	list_iterator_t *it = NULL;
	//DEBUG_INFO("...");
//
	allAlerts = json_object_new_array();
	if(NULL == allAlerts) {
		DEBUG_INFO("json_object_new_array failed");
		return NULL;
	}
	
	//
	it = list_iterator_new(payload->allAlerts,LIST_HEAD);

	while(list_iterator_has_next(it)) {
		Alert *alert = NULL ;
		struct json_object *val = NULL;

		alert = (Alert *)list_iterator_next_value(it);
	
		char *token 			    = alert_get_token( alert);
		char *type 			        = alert_get_type( alert);
		char *scheduledTime 	    = alert_get_scheduledtime(alert);
		int  stop 					= alert_get_stop(alert);

		char *pPlayOrder = NULL;
		int iLoopCount   = -2;   
		int iLoopPauseM  = -1;     
		char *pbackgroundAlertAsset = NULL;

		if (0 == strcmp(type, TYPE_REMINDER))
		{
			pPlayOrder            = alert_get_assetPlayOrder(alert);
		 	iLoopCount              = alert_get_loopCount(alert);
			iLoopPauseM             = alert_get_loopPauseInMilliseconds(alert);
			pbackgroundAlertAsset = alert_get_backgroundAlertAsset(alert);
		}
		else
		{
			iLoopCount	= alert_get_loopCount(alert);
			if (-1 == iLoopCount)
			{
				pPlayOrder            = alert_get_assetPlayOrder(alert);
				iLoopPauseM             = alert_get_loopPauseInMilliseconds(alert);
				pbackgroundAlertAsset = alert_get_backgroundAlertAsset(alert);
			}
		}

		DEBUG_INFO("list token:%s,type:%s,schedulerTime:%s",token, type, scheduledTime);

		DEBUG_INFO("iLoopCount:%d, iLoopPauseM:%d, pbackgroundAlertAsset:%s", iLoopCount, iLoopPauseM, pbackgroundAlertAsset);
		DEBUG_INFO("pPlayOrder:%s", pPlayOrder);

		val = json_object_new_object();
		if(NULL == val) {
			DEBUG_INFO("json_object_new_object failed");
			return NULL;
		}

		json_object_object_add(val, "token", json_object_new_string(token));
		json_object_object_add(val, "type", json_object_new_string(type));
		json_object_object_add(val, "scheduledTime", json_object_new_string(scheduledTime));
		json_object_object_add(val, "stop", json_object_new_int(stop));
		DEBUG_INFO("json_object_array_add123");
		if ((0 == strcmp(type, TYPE_REMINDER))|| (-1 == iLoopCount))
		{
			json_object_object_add(val, "assetPlayOrder", json_object_new_string(pPlayOrder));
			json_object_object_add(val, "backgroundAlertAsset", json_object_new_string(pbackgroundAlertAsset));
			json_object_object_add(val, "loopCount", json_object_new_int(iLoopCount));
			json_object_object_add(val, "loopPauseInMilliseconds", json_object_new_int(iLoopPauseM));
		}
		DEBUG_INFO("json_object_array_add");
		json_object_array_add(allAlerts,val);
	}

	list_iterator_destroy(it);

	//DEBUG_INFO("allAlerts :%s",json_object_to_json_string(allAlerts));
	return allAlerts;
}



struct json_object * alertsstatepayload_to_allalerts_update(AlertsStatePayload *payload)
{

	struct json_object *allAlerts=NULL;

	list_iterator_t *it = NULL;
	//DEBUG_INFO("...");
//
	allAlerts = json_object_new_array();
	if(NULL == allAlerts) {
		DEBUG_INFO("json_object_new_array failed");
		return NULL;
	}
	
	//
	it = list_iterator_new(payload->allAlerts,LIST_HEAD);

	while(list_iterator_has_next(it )) {
		Alert *alert = NULL ;
		struct json_object *val = NULL;

		alert = (Alert *)list_iterator_next_value(it);
	
		char *token 			    = alert_get_token( alert);
		char *type 			        = alert_get_type( alert);
		char *scheduledTime 	    = alert_get_scheduledtime(alert);

		DEBUG_INFO("list token:%s,type:%s,schedulerTime:%s",token, type, scheduledTime);

		val = json_object_new_object();
		if(NULL == val) {
			DEBUG_INFO("json_object_new_object failed");
			return NULL;
		}

		json_object_object_add(val, "token", json_object_new_string(token));
		json_object_object_add(val, "type", json_object_new_string(type));
		json_object_object_add(val, "scheduledTime", json_object_new_string(scheduledTime));

		json_object_array_add(allAlerts,val);
	}

	list_iterator_destroy(it);

	//DEBUG_INFO("allAlerts :%s",json_object_to_json_string(allAlerts));
	return allAlerts;
}





