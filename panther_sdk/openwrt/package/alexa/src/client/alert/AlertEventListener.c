
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "RequestFactory.h"

#include "AlertEventListener.h"

#include "../globalParam.h"

extern GLOBALPRARM_STRU g;

//向亚马逊服务器发送EVENT
//extern void submit_AlertEvent (char *json_str);


void send_request(char * requestBoby, char byType)
{
	//利用libevent api 向亚马逊发送requestBoby Event;
	DEBUG_INFO("requestBoby: %s", requestBoby);
	if(1 == GetAmazonConnectStatus())
	{

		switch (byType)
		{
			case ALERT_STARTED_EVENT:
#ifdef     ALERT_EVENT_QUEUE
				Queue_Put_Data(ALEXA_EVENT_STARTED_ALERT, requestBoby);
#else
				g.m_alert_struct.alert_started_json_str = requestBoby;
				Queue_Put(ALEXA_EVENT_STARTED_ALERT);
#endif
				break;

			case ALERT_ENTERED_BACKGROUND_EVENT:

#ifdef     ALERT_EVENT_QUEUE
				Queue_Put_Data(ALEXA_EVENT_BACKGROUND_ALERT, requestBoby);
#else
				g.m_alert_struct.alert_background_json_str = requestBoby;
				Queue_Put(ALEXA_EVENT_BACKGROUND_ALERT);
#endif


				break;

			case ALERT_ENTERED_FOREGROUND_EVENT:
#ifdef     ALERT_EVENT_QUEUE
				Queue_Put_Data(ALEXA_EVENT_FOREGROUND_ALERT, requestBoby);
#else
				g.m_alert_struct.alert_foreground_json_str = requestBoby;
				Queue_Put(ALEXA_EVENT_FOREGROUND_ALERT);
#endif
				break;

			case ALERT_SET_EVENT:
#ifdef     ALERT_EVENT_QUEUE
				Queue_Put_Data(ALEXA_EVENT_SET_ALERT, requestBoby);
#else
				g.m_alert_struct.alert_set_json_str = requestBoby;
				Queue_Put(ALEXA_EVENT_SET_ALERT);
#endif

				break;
			
			case ALERT_DELETE_EVENT:

#ifdef     ALERT_EVENT_QUEUE
				Queue_Put_Data(ALEXA_EVENT_DELETE_ALERT, requestBoby);
#else
				g.m_alert_struct.alert_delete_json_str = requestBoby;
				Queue_Put(ALEXA_EVENT_DELETE_ALERT);
#endif

				break;
			
			case ALERT_STOP_EVENT:
#ifdef     ALERT_EVENT_QUEUE
				Queue_Put_Data(ALEXA_EVENT_STOP_ALERT, requestBoby);
#else
				g.m_alert_struct.alert_stop_json_str = requestBoby;
				Queue_Put(ALEXA_EVENT_STOP_ALERT);
#endif
				break;
		}

	}
	return ;
}
void alerteventlistener_on_alert_started(char * alertToken)
{
	//DEBUG_INFO("...");
	send_request(requestfactory_create_alerts_alert_started_event(alertToken), ALERT_STARTED_EVENT);

	if(2 == g.m_CaptureFlag) {  //判断是否正在录音 
	//正在录音，则发送Alert BACKGROUND ALERT EVENT请求
		send_request(requestfactory_create_alerts_entered_background_event(alertToken), ALERT_ENTERED_BACKGROUND_EVENT);
	} else {
	//否则，则发送Alert FOREGROUND ALERT EVENT请求 
		send_request(requestfactory_create_alerts_entered_foreground_event(alertToken), ALERT_ENTERED_FOREGROUND_EVENT);
	}
}


void alerteventlistener_on_alert_set(char * alertToken, int success)
{
	send_request(requestfactory_create_alerts_set_alert_event(alertToken,success), ALERT_SET_EVENT);
}

void alerteventlistener_on_alert_delete(char * alertToken,int success)
{
	send_request(requestfactory_create_alerts_delete_alert_event(alertToken, success), ALERT_DELETE_EVENT);
}

void alerteventlistener_on_alert_stopped(char * alertToken)
{	
	send_request(requestfactory_create_alerts_alert_stopped_event( alertToken), ALERT_STOP_EVENT);
}






