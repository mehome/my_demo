#include "HuabeiDispatcher.h"

#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <assert.h>


#include "event_queue.h"
#include "event.h"

#include "HuabeiDispatcher.h"
#include "HuabeiStruct.h"

static pthread_t huabeiPthread = 0;


void HuabeiPingPthread(void *arg)
{
	while(1) {
		int result = cdb_get_int("$wanif_state", 0);
		if(result == 2) {
			if( 1 == GetSendFlag())  
			{
				char *body = NULL;
				char *apikey = NULL;	
				IotEvent * event = NULL;
				apikey = GetHuabeiApiKey();
				char *deviceId = GetTuringDeviceId();
				body = HuabeiJsonNotifyStatus(apikey, deviceId, 0x80);
				warning("body:%s",body);
				char *iotHost = GetTuringMqttIotHost();
				HttpPost(iotHost, "/api/v1/iot/status/notify", body, HUABEI_NOTIFY_STATUS_EVENT);
			}
		} 
		sleep(30);
	}
}


int   CreateHuabeiPingPthread(void)
{
	int iRet;
	pthread_attr_t attr; 

	info("...");
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
	
	iRet = pthread_create(&huabeiPthread, &attr, HuabeiPingPthread, NULL);
	pthread_attr_destroy(&attr);
	if(iRet != 0)
	{
		info("pthread_create error:%s\n", strerror(iRet));
		return -1;
	}
	
	return 0;
}
int   DestoryHuabeiPingPthread(void)
{
	if (huabeiPthread != 0)
	{
		pthread_join(huabeiPthread, NULL);
		info("capture_destroy end...\n");
	}	
}

