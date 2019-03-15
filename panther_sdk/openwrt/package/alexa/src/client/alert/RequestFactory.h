#ifndef __REQUEST_FACTORY_H__
#define __REQUEST_FACTORY_H__

#include <stdlib.h>
#include <stdio.h>
#include "debug.h"


#define SPEECH_SYNTHESIZER_NAMESPACE							"SpeechSynthesizer"
#define SPEECH_SYNTHESIZER_EVENT_SPECCH_STARTED               	"SpeechStarted"
#define SPEECH_SYNTHESIZER_EVENT_SPECCH_FINISHED               	"SpeechFinished"



#define ALERTS_NAMESPACE							"Alerts"

#define ALERTS_EVENT_SET_ALERT_SUCCEEDED    		"SetAlertSucceeded"
#define ALERTS_EVENT_SET_ALERT_FAILED  				"SetAlertFailed"
#define ALERTS_EVENT_DELETE_ALERT					"DeleteAlert"
#define ALERTS_EVENT_DELETE_ALERT_FAILED  			"DeleteAlertFailed"
#define ALERTS_EVENT_DELETE_ALERT_SUCCEEDED  		"DeleteAlertSucceeded"
#define ALERTS_EVENT_ALERT_STARTED					"AlertStarted"
#define ALERTS_EVENT_ALERT_STOPPED					"AlertStopped"
#define ALERTS_EVENT_ALERT_ENTERED_FOREGROUND		"AlertEnteredForeground"
#define ALERTS_EVENT_ALERT_ENTERED_BACKGROUND		"AlertEnteredBackground"

extern char *  requestfactory_create_alerts_set_alert_event(char * alertToken, int success) ;

extern char *  requestfactory_create_alerts_delete_alert_event(char * alertToken,int success);

extern char *  requestfactory_create_alerts_alert_started_event(char * alertToken) ;
extern char *  requestfactory_create_alerts_alert_stopped_event(char * alertToken) ;
extern char *  requestfactory_create_alerts_entered_foreground_event(char * alertToken) ;

extern char *  requestfactory_create_alerts_entered_background_event(char * alertToken) ;

extern char *  requestfactory_create_speech_synthesizer_speech_finished_event(char * alertToken);

extern char *  requestfactory_create_speech_synthesizer_speech_started_event(char * alertToken);


#endif



