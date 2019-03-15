#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "TuringControl.h"
#include "myutils/msg_queue.h"
#include "myutils/log.h"



void * parse_msg_queue_data()
{
    char buf[32] = {0};
    char *tmp = NULL;
    msg_queue_create_msg_queue(MAGIC_NUMBER);
    while(1)
    {
        memset(buf,0,32);
        tmp = msg_queue_recv(MSG_UARTD_TO_IOT);
        if(tmp)
        {   
            strncpy(buf,tmp,32);
            free(tmp);
            
            TuringMonitorControl(buf);
         
        }
    }
}


static pthread_t monitorPthread = 0;

void   create_msg_queue_monitor_thread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);


	iRet = pthread_create(&monitorPthread, &a_thread_attr, parse_msg_queue_data, NULL);
	//iRet = pthread_create(&monitorPthread, NULL, MonitorPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}
	
}
int   destory_msg_queue_monitor_thread(void)
{
	if (monitorPthread != 0)
	{
		pthread_join(monitorPthread, NULL);
		info("capture_destroy end...\n");
	}	
}

