
#include "AlertHead.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include <libcchip/function/wav_play/wav_play.h>


#include "AlertHandler.h"
#include "AlertManager.h"

#include "myutils/debug.h"
#include "common.h"
#include "mpdcli.h"



static time_t start ;
#define TIMEOUT_IN_SEC 60
int alert_playing = 0;
int start_alert = 1;
AlertHandler *alerthandler_new(void)
{	int ret;
	AlertHandler *handler = NULL;

	handler = calloc(1,sizeof(AlertHandler));
	if(NULL == handler) {
		DEBUG_ERR("calloc AlertHandler failed");
		return NULL;
	}

	handler->alertState = ALERT_FINISHED;
	
	ret = pthread_mutex_init(&handler->aMutex,NULL);
	if(ret) {
		error("pthread_mutex_init failed");
		goto failed;
	}

	
	return handler;
failed:
	free(handler);
	handler = NULL;
	return NULL;
	
}

int alerthandler_is_alarming(AlertHandler *handler)
{
	int ret;
	//pthread_mutex_lock(&handler->aMutex);
	ret = (handler->alertState == ALERT_PLAYING) ;
	
	//pthread_mutex_unlock(&handler->aMutex);
	return ret;
	
}

int alert_playing_now()
{
    return (alert_playing==1);
}

void stop_alert()
{
    start_alert = 0;
}

void *alert_start_thread(void *args)
{
	time_t now ;
    int play_count = 0;
	start = time(NULL);
	AlertManager *manager = (AlertManager *)args;
	AlertHandler *handler = NULL;
	
	handler = alertmanager_get_alerthandler(manager);
	if(handler == NULL)
		goto exit;

	error("enter ...");
	info("alert_start_thread iPrompt:%s",iPrompt);
	int pause = 0;
	int state  = MpdGetPlayState();
	if(state == MPDS_PLAY) {
		pause = 1;
		//MpdPause();
		warning("mpc pause ...");
		exec_cmd("mpc pause");
	}
	int volume = cdb_get_int("$ra_vol",50);
	exec_cmd("amixer set ALERTS 40% > /dev/null");
	if(strcmp(iPrompt,"null"))
		exec_cmd("mpc volume 40");
	while(start_alert && alerthandler_is_alarming(handler) && !get_mic_record_status())
    {
        alert_playing = 1;
		int state  = MpdGetPlayState();
		if(state == MPDS_PLAY) {
			pause = 1;
			exec_cmd("mpc pause");
		}
        if(start_alert)
        {
            play_count++;
            system("aplay -D alerts /tmp/res/alarm.wav");
			if(strcmp(iPrompt,"null"))
			{
				text_2_sound(iPrompt);						
				usleep(500*1000);
				while(!IsMpg123Finshed()){usleep(100*1000);}
			}
				
            if(play_count == 1)
            {
                system("amixer set ALERTS 50% > /dev/null");
				if(strcmp(iPrompt,"null"))
					exec_cmd("mpc volume 50");
                sleep(1);
            }
            else if(play_count == 2)
            {
                system("amixer set ALERTS 60%");
				if(strcmp(iPrompt,"null"))
					exec_cmd("mpc volume 60");
                sleep(1);
            }
            else if(play_count == 3)
            {
                system("amixer set ALERTS 80%");
				if(strcmp(iPrompt,"null"))
					exec_cmd("mpc volume 80");
                sleep(1);
            }
        }
		now = time(NULL);
		double diff ;
		diff = difftime(now, start);
		if(diff > TIMEOUT_IN_SEC) {
			info("alert start thread timeout ...");
			break;
		}
	}
    exec_cmd("amixer set ALERTS 40% > /dev/null");
	if(strcmp(iPrompt,"null"))
		MpdVolume(volume);	
	alert_playing = 0;
    start_alert = 1;
    play_count = 0;
	free(iPrompt);
//	iPrompt = NULL;
	alerthandler_set_state(handler,ALERT_FINISHED);

	if(pause) {
		exec_cmd("mpc play");
	}
	MpdVolume(200);
exit:
	pthread_exit(0);
	info("alert start thread end ...");
}


void alerthandler_start_alert(AlertHandler *handler, AlertManager *manager)
{
	//播放 alert
	error("enter ...");	
	if(!alerthandler_is_alarming(handler)) 
    {

	//	if(g.m_CaptureFlag == 0 || g.m_CaptureFlag == 3 ) {
		if(!get_mic_record_status()) 
        {
			
			alerthandler_set_state(handler,ALERT_PLAYING);
			error("Start playing alert ....");

		    pthread_attr_t a_thread_attr;
		    pthread_attr_init(&a_thread_attr); 
    		pthread_attr_setdetachstate(&a_thread_attr,PTHREAD_CREATE_DETACHED);

		    pthread_attr_setstacksize(&a_thread_attr,PTHREAD_STACK_MIN);

		    pthread_create(&(handler->tid), &a_thread_attr, alert_start_thread, (void *)manager);
			
		    pthread_attr_destroy(&a_thread_attr);

		} else {
			alerthandler_set_state(handler,ALERT_INTERRUPTED);
			error("ALERT_INTERRUPTED ....");
		}
	} else {
		error("alerthandler is alarming....");
	}
	
}

void alerthandler_stop_alert(AlertHandler *handler, AlertManager *manager)
{
	if(alertmanager_has_activealerts(manager) == 0) {
		alerthandler_set_state(handler, ALERT_FINISHED);
	}

}

AlertState alerthandler_get_state(AlertHandler *handler)
{
	AlertState state;	
	//pthread_mutex_lock(&handler->aMutex);
	state = handler->alertState;
	//pthread_mutex_unlock(&handler->aMutex);
	return state;
}

void  alerthandler_set_state(AlertHandler *handler, AlertState state)
{
	//pthread_mutex_lock(&handler->aMutex);
	handler->alertState = state;
	//pthread_mutex_unlock(&handler->aMutex);
}

int alerthandler_resume_alerts(AlertManager * manager)
{	
	AlertHandler *handler =NULL;

	handler = manager->handler;
	if(alerthandler_get_state(handler) == ALERT_INTERRUPTED) {
		alerthandler_start_alert(handler, manager);
		return 1;
	}	
	return 0;
}


void alerthandler_free(AlertHandler *handler)
{	
	//EBUG_INFO("....");
	if(handler) {
		free(handler);
		handler = NULL;
	}
}


