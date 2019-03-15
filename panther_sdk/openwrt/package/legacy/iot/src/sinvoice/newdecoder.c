#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <limits.h>
#include <sys/syscall.h>
#include <semaphore.h>

#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include "voice_decode.h"
#include "myutils/debug.h"
#include "monitor.h"

int in_aborting = 0;
#define VOICE_SIZE  (4 * 1024)
#define TIME_OUT    (60)
static pthread_t voiceDecoder = 0;
int killAirkiss = 0;
static pthread_mutex_t countMtx = PTHREAD_MUTEX_INITIALIZER;
static sem_t sinvoiceSem;


void get_start()
{
    error("Get start...............................");
    
    if(killAirkiss == 0) 
    {
        int chan;
        char buf[512] = {0};
    
        //收到声波数据，此时是声波配网，把airkiss干掉并将标志位置为初识状态
        exec_cmd("killall -9 airkiss");
        cdb_set_int("$airkiss_state",0);
        cdb_set_int("$airkiss_start",0);
        
        exec_cmd("wd smrtcfg stop usr");
        //exec_cmd("mtc wlhup");
        chan = cdb_get_int("$wl_channel", 6);
        info("chan = %d",chan);
        snprintf(buf, 512, "wd setchan %d", chan);
        exec_cmd(buf);
        
       // exec_cmd("mtc ra restart");
        //sleep(5);
       info("kill_airkiss.....over");
            
        killAirkiss = 1;
    }  
}



void get_finish( char *ssid,char  *passwd)
{
    printf("ssid = %s,passwd = %s\n",ssid,passwd);
    in_aborting = 1;
    error("\033[32msinvoice:ssid = \"%s\",pwd = \"%s\"\033[0m\n",ssid,passwd);
    wav_play2("/tmp/res/receive_network_info.wav");
    cdb_set_int("$sinvoice_state", 1);  //sinvoice正在配网
    cdb_set_int("$sinvoice_start", 0);
    set_mic_record_status(0);
    char buf[512] = {0};
    bzero(buf, 512);
    snprintf(buf, 512, "/lib/wdk/omnicfg_apply 9 \"%s\" \"%s\" & ",ssid,passwd);
    error("buf:%s",buf);
    system(buf);
}


//发送信号量，开始声波配网的解码动作
void sinvoice_start_decode()
{
    pthread_mutex_lock(&countMtx);
    sem_post(&sinvoiceSem);
    pthread_mutex_unlock(&countMtx);
}




void *VoiceDecoderProcessor(void *arg)
{
    int iLength = 0;
    char *buf = calloc(VOICE_SIZE,1);
    int start_time = 0;
    int current_time = 0;
    
    debug("VoiceDecoderProcessor  enter");
    error("pid = [%d]",syscall(SYS_gettid));

    while(1)
    {
        info("VoiceDecoderProcessor  waiting.....");
        sem_wait(&sinvoiceSem);
        in_aborting = 0;
        cdb_set_int("$sinvoice_start",1);  //正在使用sinvoice配网
        
        voice_decoder_cb vdc = {get_start,get_finish};
        voice_decoder_init("Turing_20181016",&vdc);
        
        info("VoiceDecoderProcessor  start");
        start_time = time(NULL);
		info("start_time = %ld",start_time);
        while ( !in_aborting) 
        {
            iLength = CaptureFifoLength();

            if (iLength >= VOICE_SIZE)  
            {
                info("Get %d bytes",VOICE_SIZE);
                iLength  = VOICE_SIZE;
            }  
            else
            {
                continue;
            }
            memset(buf,0,iLength);
            if(CaptureFifoSeek(buf, iLength) != iLength) {
                error("CaptureFifoSeek != iLength");
                break;
            }
            voice_decode_put_data(buf, iLength);
            current_time = time(NULL);
			info("current_time = %ld,current_time ----------%ld",current_time,(current_time-start_time));
			
            if(current_time - start_time > TIME_OUT)
            {
            	info("111111111111111");
                cdb_set_int("$sinvoice_start",0);
                wav_play2("/tmp/res/sinvoice_timeout.wav");
                set_mic_record_status(0);
				break;
            }
        }
    }
    if(buf)
        free(buf);
}

void CreateRTVoiceDecoderPthread()
{
	int iRet;

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	iRet = pthread_create(&voiceDecoder, NULL, VoiceDecoderProcessor, NULL);
	pthread_attr_destroy(&a_thread_attr);

	if(iRet != 0)
	{
		printf("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
	
}

void DestoryRTVoiceDecoderPthread(void)
{

	if (voiceDecoder != 0)
	{
		pthread_join(voiceDecoder, NULL);
		info("DestoryMonitorPthread end...");
	}



}

