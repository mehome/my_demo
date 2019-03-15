#ifndef  __ALERTS_STATE_PAYLOAD_H__
#define  __ALERTS_STATE_PAYLOAD_H__

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


#include "AlertHead.h"
#include <json/json.h>


extern AlertsStatePayload *alertsstatepayload_new(list_t *allAlerts ,list_t *activeAlerts);

extern list_t *alertsstatepayload_get_all_alerts(AlertsStatePayload *payload);

extern list_t *alertsstatepayload_get_active_alerts(AlertsStatePayload *payload);


extern void alertsstatepayload_free(AlertsStatePayload *payload);

extern  struct json_object * alertsstatepayload_to_allalerts(AlertsStatePayload *payload);

extern  struct json_object * alertsstatepayload_to_activealerts(AlertsStatePayload *payload);

#endif




