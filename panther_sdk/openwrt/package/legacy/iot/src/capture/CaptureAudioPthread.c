#include "CaptureAudioPthread.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <libgen.h> 
#include "interface.h"
#include "init.h"
#include <mpdcli.h>
#include <mpdcli.h>
#include "wavwriter.h"
#include "libutils/procutils.h"
#include "mon_config.h"
#include "myutils/debug.h"
#include "common.h"
#include "fifobuffer.h"
#ifdef ENABLE_COMMSAT
#include "commsat.h"
#endif

#include "capture.h"
#include "json.h"

//extern void setMutMicStatus(char status);
//extern char getMutMicStatus(void);

static pthread_t capturePthread = 0;
static sem_t captureSem;
extern int recover_playing_status;
extern int RecoverPlaying_status;
//extern int FristInteract_flag;
//extern int Interact_flag;
static int captureWait = 0;
int tfSave_flag = 0; 

static pthread_mutex_t captureMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ListeningMtx = PTHREAD_MUTEX_INITIALIZER;

static pthread_t wakeup_pthread = 0;
wakeupStartArgs_t turingWakeupStartArgs={0};
record_t  wechat = {0};

static FT_FIFO * g_CaptureFifo = NULL;
#undef FIFO_LENGTH
#define FIFO_LENGTH         16384*5*2

extern GLOBALPRARM_STRU g;

#undef BIG_ENDIAN
#define BIG_ENDIAN 0

//返回值  0：没有声音了            1：仍有声音
static int vad_test(short samples[], int len)
{
    int i;
#if BIG_ENDIAN
    unsigned char *p;
    unsigned char temp;
#endif
    long long sum = 0, sum2 = 0;
    for(i = 0; i < len; i++) 
    {
#if BIG_ENDIAN
        p = &samples[i];
        temp = p[0];
        p[0] = p[1];
        p[1] = temp;
#endif
        sum += (samples[i] * samples[i]);
        sum2 += g.m_iVadThreshold;		//1000000*3200(字节)
    }
    printf("sum  = %15lld , sum2 = %15lld\n",sum,sum2);
    if (sum > sum2)
        return 1;		//有声音
    else
        return 0;		//没有声音
}

void StartTlkOn_led()
{
#if defined (CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_DOSS_1MIC_V1)
	exec_cmd("uartdfifo.sh StartTlkOn");
//#elif defined(CONFIG_PROJECT_DOSS_1MIC_V1)
//	exec_cmd("uartdfifo.sh StartTlkOn");	
#else 
	
#endif	
}

void StartTlkPlay_led()
{

#if defined (CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_DOSS_1MIC_V1)
			exec_cmd("uartdfifo.sh StartTlkPlay");
//#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
//			exec_cmd("uartdfifo.sh StartTlkPlay");
#else
	
#endif
}

void StartPowerOn_led()
{
#if   defined (CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_DOSS_1MIC_V1)
			exec_cmd("uartdfifo.sh StartPowerOn");
//#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
//			exec_cmd("uartdfifo.sh StartPowerOn");
#else

#endif
}




void CaptureFifoInit()
{
	if(!g_CaptureFifo)  {
		g_CaptureFifo = ft_fifo_alloc(FIFO_LENGTH);
		if(!g_CaptureFifo) {
			error("ft_fifo_alloc for g_CaptureFifo");
			exit(-1);
		}
		ft_fifo_clear(g_CaptureFifo);
	}
}
void CaptureFifoDeinit()
{
	if(g_CaptureFifo) {
		ft_fifo_free(g_CaptureFifo);
		g_CaptureFifo = NULL;
	}
}

void CaptureFifoClear()
{
	if(g_CaptureFifo) {
		ft_fifo_clear(g_CaptureFifo);
		//g_CaptureFifo = NULL;
	}
}

unsigned int CaptureFifoPut(unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_CaptureFifo) {
		length = ft_fifo_put(g_CaptureFifo, buffer, len);
	} else {
		error("NULL == g_CaptureFifo");
	}
	return length;
}

unsigned int CaptureFifoDelData(unsigned int len)
{
	unsigned int length = 0;
	if(g_CaptureFifo) {
		length = ft_fifo_delete_data(g_CaptureFifo, len);
	} else {
		error("NULL == g_CaptureFifo");
	}
	return length;
}



unsigned int CaptureFifoLength()
{
	unsigned int len = 0;
	if(g_CaptureFifo) {
		len = ft_fifo_getlenth(g_CaptureFifo);
	}
	return len;
}
unsigned int CaptureFifoSeek( unsigned char *buffer, unsigned int len)
{
	unsigned int length = 0;
	if(g_CaptureFifo) {
		ft_fifo_seek(g_CaptureFifo, buffer, 0, len);
		length = ft_fifo_setoffset(g_CaptureFifo, len);
	}
	return length;	
}



//发送信号量，开始录音
void StartCaptureAudio()
{
	pthread_mutex_lock(&captureMtx);
	if(captureWait == 0) {
		sem_post(&captureSem);
	}
	pthread_mutex_unlock(&captureMtx);

}

void SetCaptureWait(int wait)
{
	pthread_mutex_lock(&captureMtx);
	captureWait = wait;
	pthread_mutex_unlock(&captureMtx);
}

int  GetCaptureWait()
{
    int ret = 1;
	pthread_mutex_lock(&captureMtx);
	ret = (captureWait == 1);
	pthread_mutex_unlock(&captureMtx);

    return ret;
}


void *CaptureAudioPthread(void)
{
    while(1)
    {
        warning("Wait new capture.....");
        SetCaptureState(0);
        SetCaptureWait(0);
        sem_wait(&captureSem);
        SetCaptureState(2);
        SetCaptureWait(1);
        
        info("播放提示音");
        wav_play2("/tmp/res/record_start.wav");
    	capture_voice();
    
        PrintSysTime("capture end...");
        //exec_cmd("uartdfifo.sh tlkoff");
        warning("Capture done ...");
	}
	
}


void CreateCapturePthread(void)
{
	int iRet;


	CaptureFifoInit();
	
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);
	iRet = pthread_create(&capturePthread, &a_thread_attr, CaptureAudioPthread, NULL);
//	iRet = pthread_create(&capturePthread, NULL, CaptureAudioPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	
	if(iRet != 0)
	{
		printf("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
	
}


void DestoryCapturePthread(void)
{

	if (capturePthread != 0)
	{
		pthread_join(capturePthread, NULL);
	}
	CaptureFifoDeinit();


}
extern void SetHttpRequestCancle();
extern int alert_playing_now();
extern void stop_alert();



int translation_flag = 0 ;


void stop_record_data()
{
	pthread_mutex_lock(&ListeningMtx);
    turingWakeupStartArgs.isMutiVadListening = 0;
	pthread_mutex_unlock(&ListeningMtx);
}

int get_record_status(void)
{
	int ret;
	pthread_mutex_lock(&ListeningMtx);
	ret = (1 == turingWakeupStartArgs.isMutiVadListening); 
	pthread_mutex_unlock(&ListeningMtx);
	return ret;
}
void Start_record_data(void)
{
	pthread_mutex_lock(&ListeningMtx);
	turingWakeupStartArgs.isMutiVadListening = 1;
	pthread_mutex_unlock(&ListeningMtx);
}

#define WE_CHAT_MAX_SIZE   30*32*1024

int add_wav_header_to_pcm(FILE *fp)
{
    info("++++++++add_wav_header_to_pcm+++++++++++++");
    if(!fp) {return -1;}
    int data_len = ftell(fp);
    fseek(fp,0,SEEK_SET);
    
    info("data_len = %x",data_len);
    
    struct wav_writer hearder;
    hearder.bits_per_sample = 16;
    hearder.channels = 1;
    hearder.sample_rate = 16000;
    hearder.wav = fp;
    
    write_header(&hearder,data_len);
    fclose(fp);
    info("++++++++++++++++++++++++");
    return 0;
}


unsigned int wechat_silence_size = 0;
unsigned int wechat_silence_len = 0;
int  wechat_flag = 0;
long start_time = 0;
long end_time = 0;


#if 0
int get_data_for_wechat(unsigned char *buffer, unsigned int len)
{
    int ret = 0;
    
    if(wechat.first_put == 0)
    {
        info("Open /tmp/8k.wav");
        wechat.record_fp = fopen("/tmp/8k.wav", "w+");
        if (wechat.record_fp == NULL)
        {
            printf("open 1.raw failed.\n");
            exit(-1);
        }
		StopAllPlayer();
        turingWakeupStartArgs.wakeup_waitctl->enter_listen(turingWakeupStartArgs.wakeup_waitctl);
        wav_preload("/tmp/res/record_start.wav");
        ASP_clear_data();
        turingWakeupStartArgs.wakeup_waitctl->exit_listen(turingWakeupStartArgs.wakeup_waitctl);
        wechat.first_put = 1;
    }
    int write_size = fwrite(buffer,1,len,wechat.record_fp);
    fflush(wechat.record_fp);
    wechat.record_size += write_size;

    //预防把内存撑爆，只允许最长录音30秒，生成的文件大概1M
    if(wechat.record_size > WE_CHAT_MAX_SIZE)
    {
        wechat_finish();
    }
    return ret;
}


#else 
int get_data_for_wechat(unsigned char *buffer, unsigned int len)
{
    int ret = 0;
	if (cdb_get_int("$wanif_state", 0) !=2) 
    {
        wav_preload("/tmp/res/speaker_not_connected.wav");
        stop_record_data();
        return ret;
    }

	if(IsChatCancled())
	{
		info("wechat capture cancled \n");
        stop_record_data();
		SetChatState(CHAT_NONE);
	}
	
    if(wechat.first_put == 0)		
    {
        info("Open /tmp/8k.wav");
        wechat.record_fp = fopen("/tmp/8k.wav", "w+");
        if (wechat.record_fp == NULL)
        {
            printf("open 1.raw failed.\n");
            exit(-1);
        }
		StopAllPlayer();
        turingWakeupStartArgs.wakeup_waitctl->enter_listen(turingWakeupStartArgs.wakeup_waitctl);
        //开始微聊播放提示音
       	wav_preload("/tmp/res/record_start.wav");
		StartTlkPlay_led();
		usleep(1000*500);
		ASP_clear_data();		
        turingWakeupStartArgs.wakeup_waitctl->resum(turingWakeupStartArgs.wakeup_waitctl);	
        wechat.first_put = 1;
		SetChatState(CHAT_ONGING);		//微聊采集中...
		start_time = getCurrentTimeSec();
		return 0;
		
    }

	int write_size = fwrite(buffer,1,len,wechat.record_fp);
	fflush(wechat.record_fp);
	wechat.record_size += write_size;	
	info("write_size = %d",write_size);
	if(wechat_flag == 0)
	{
		ret = vad_test((short *)buffer,len/2);
		if(ret > 0)									//返回1，还有声音
		{
			info("识别到人的声音");
			wechat_flag=1;	
		}
		else										//返回0，没有声音
		{
			info("当前状态没人说话");
			end_time = getCurrentTimeSec();
			if(end_time - start_time >  5)	
			//if(wechat.record_size > WE_CHAT_MAX_SIZE/6)	//静音超过5秒
			{
				info("end_time - start_time = %d",end_time - start_time);
				info("超过5秒没人说话,没有听清，请再来一次");
				wechat_flag=0;
				wechat_silence_size=0;
				SetChatState(CHAT_NONE);
				wechat_finish();
			}	
		}
	}	
	else			//已经检测到过人声了，只需要检测VAD末端
	{
		ret = vad_test((short *)buffer,len/2);
		if(ret == 0)		
		{
			info("已经检测不到人声了");
			wechat_silence_size +=len; 
			if(wechat_silence_size > 16*1024)	//500ms
			{
				info("静音数据超过16k");
				wechat_silence_size=0;
				wechat.first_put=0;
				wechat_flag=0;				
				SetChatState(CHAT_NONE);
				wechat_finish();		//微聊结束
			}
		}
		else	
		{
			info("正在检测人声...........");
			//wechat_flag=0;
			wechat_silence_size=0;
		}

		info("wechat.record_size = %d",wechat.record_size);
	}
   
    //预防把内存撑爆，只允许最长录音30秒，生成的文件大概1M
    if(wechat.record_size > WE_CHAT_MAX_SIZE)
    {
		SetChatState(CHAT_NONE);		//微聊结束
        wechat_finish();	
    }
    return ret;
}
#endif



int frist_translate_flag = 0;
void translation_start()
{
	if (cdb_get_int("$wanif_state", 0) != 2) 
    {
        wav_preload("/tmp/res/speaker_not_connected.wav");
        return ;
    }
	/*
	if(!frist_translate_flag)
	{
		frist_translate_flag=1;
		turingWakeupStartArgs.isWakeUpSucceed=0;
	}else
	{
		turingWakeupStartArgs.isWakeUpSucceed=1;
	}*/
	
	turingWakeupStartArgs.isWakeUpSucceed=1;		//可按键打断交互
    turingWakeupStartArgs.isMutiVadListening = 3;
    translation_flag = 1;
}

void wechat_start()
{
    if (cdb_get_int("$wanif_state", 0) != 2) 
    {
        wav_preload("/tmp/res/speaker_not_connected.wav");
        return ;
    }
    memset(&wechat,0,sizeof(wechat));
    turingWakeupStartArgs.isMutiVadListening = 2;
   
}

void wechat_finish()
{
    if(turingWakeupStartArgs.isMutiVadListening == 2)
    {
        stop_record_data();
        add_wav_header_to_pcm(wechat.record_fp);
        EventQueuePut(EVENT_UPLOAD_FILE, NULL);
        
        if(recover_playing_status )
        {
            exec_cmd("mpc play");
            recover_playing_status = 0;
        }
    }
}

/* BEGIN: Added by Frog, 2018/7/12 */

unsigned int silence_size = 0;
int upload_flag = 0;
int first_put = 0;      //是否是第一次接收数据 0：第一次  1：不是第一次
int voice_flag = 0;     //是否检测到人声
int save_one_frame = 0; //识别到人声前，保存一次采样到fifo中
FILE *record_fp = NULL;
int backup = 0;
long keep_silence_size = 0; //一直不说话，5秒后结束
long total_size = 0;   
int  interact_count = 0;
//是否备份要上传的音频
#define WRITE_TO_FILE 1  

char * tips[]={
"/root/iot/zai_ne.wav",
"/root/iot/wo_zai_ting_ne.wav",
"/root/iot/wo_zai.wav",
"/root/iot/qing_jiang.wav",
"/root/iot/en.wav"
};


void process_record_data_init()
{
	StopAllPlayer();
//	info("StopAllPlayer recover_playing_status =%d ",recover_playing_status);
#if WRITE_TO_FILE
    if(record_fp != NULL)
    {
        fclose(record_fp);
        record_fp = NULL;
    }
    record_fp = fopen("/tmp/record.pcm", "w+");
    if (record_fp == NULL)
    {
        printf("open 1.raw failed.\n");
        exit(-1);
    }
#endif
    long sec = time(NULL);
    turingWakeupStartArgs.wakeup_waitctl->enter_listen(turingWakeupStartArgs.wakeup_waitctl);
    if(translation_flag)
        wav_preload("/tmp/res/record_start.wav");
    else
	{
#ifdef ENABLE_CONTINUOUS_INTERACT
//		info("Interact_flag = %d",get_interact_status());
		if(get_interact_status() == 1)
		{
			//	wav_preload("/tmp/res/record_start.wav");
		}
		else
#endif
			wav_preload(tips[sec%5]);
	}
     //system("aplay /root/iot/qing_jiang.wav");
	StartTlkOn_led();
    usleep(500*1000);
    turingWakeupStartArgs.wakeup_waitctl->resum(turingWakeupStartArgs.wakeup_waitctl);
    info("设置录音状态位");
    SetCaptureState(CAPTURE_ONGING);	//采集状态为进行中
    getuuidString(g.identify);
    CaptureFifoClear(); //第一次要清空原有的数据，可解决打断唤醒时原有数据仍残留的问题
    first_put = 1;
    ASP_clear_data();
}


void process_record_data_no_sound(unsigned char *buffer, unsigned int len)
{
	
    int ret = vad_test((short *)buffer,len/2);
    if(ret > 0)
    {
        voice_flag = 1;
        CaptureFifoPut(buffer,len);
        if(upload_flag == 0)
        {
            info("识别到人声，开始通知上传,长度 = %d",CaptureFifoLength());
            //NewHttpRequest();
            OpusStartNewEncode();
            upload_flag = 1;
        }
    }
    else
    {
   // 	debug("ttttttt");
        CaptureFifoClear();         //删掉上一次的数据
        CaptureFifoPut(buffer,len); //保留本次静音数据
        keep_silence_size += len;
        if(keep_silence_size > 5*16*1024*2)  //一直静音，超过5秒，结束本次对话
        {
            keep_silence_size = 0;
            info("静音5秒钟时间到");
		//	stop_record_data();			
			translation_flag=0;
            upload_flag = 0;       			  //为下一次通知上传做准备
            SetCaptureState(CAPTURE_NONE);    //通知上传线程，已经录音结束
            voice_flag = 0;
            first_put = 0;
            silence_size = 0;
#if WRITE_TO_FILE                    
            if(record_fp)
            {
                fclose(record_fp);
                record_fp = NULL;
            }
#endif

#ifdef ENABLE_CONTINUOUS_INTERACT
            AddErrorCount();
			Start_record_data();
			if(GetErrorCount() != 5){
				return ;
			}
#else
			stop_record_data();				//录音结束
#endif

//          AddMpg123Url("/root/iot/can_not_hear_you.mp3");
//			info("recover_playing_status=%d,RecoverPlaying_status=%d",recover_playing_status,RecoverPlaying_status);
            if(recover_playing_status || RecoverPlaying_status)
            {
            	
				recover_playing_status = 0;
#ifdef ENABLE_CONTINUOUS_INTERACT
				EndInteractionMode();
#endif
                usleep(2000*1000);
                while(!IsMpg123Finshed()){usleep(100*1000);}
                MpdPlay(-1);
            }
			else
			{
				recover_playing_status = 0;
#ifdef ENABLE_CONTINUOUS_INTERACT
				EndInteractionMode();
#endif			
				StartPowerOn_led();
			}
			printf("出现错误次数:total=%d,连续交互结束111\n",GetErrorCount());
        }
    }
}
void process_record_data_has_sound(unsigned char *buffer, unsigned int len)
{
//	debug("rrrrrrrrrrrr");

    CaptureFifoPut(buffer,len);
    int ret = vad_test((short *)buffer,len/2);
    if(ret == 0)
    {
        info("检测不到人声了");
        silence_size += len;
        if(silence_size > 16*1024)  //0.5S检测不到声音，结束录音
        {
            info("静音数据超过16K,结束本次录音");
            upload_flag = 0;       //为下一次通知上传做准备
            SetCaptureState(CAPTURE_NONE);    //通知上传线程，已经录音结束
            voice_flag = 0;
            first_put = 0;
            silence_size = 0;
            stop_record_data();
            keep_silence_size = 0;
#if WRITE_TO_FILE                    
            if(record_fp)
            {
                fclose(record_fp);
                record_fp = NULL;
            }
#endif
            if(CaptureFifoLength() > len)
            {
                CaptureFifoDelData(len);  //把最后一次的静音数据删掉
                debug("len final = %d",CaptureFifoLength());
            }
			SetCaptureState(CAPTURE_NONE);
			//StartNewEncode();
            return 1;
        }
    }
    else
    {
//		debug("rrrrrrrrrrrr");

        silence_size = 0;
        total_size += len;
        if(total_size > 15*16000 *2)
        {
        	info("时间超过15s,退出");
            stop_record_data();
            upload_flag = 0;       //为下一次通知上传做准备
            SetCaptureState(CAPTURE_NONE);    //通知上传线程，已经录音结束
            voice_flag = 0;
            first_put = 0;
            silence_size = 0;
            keep_silence_size = 0;
            total_size = 0;
        }
    }
    info("record len = %d ",CaptureFifoLength());
}



//函数说明：
//本函数用于截取数据中的人声部分，也即识别VAD的开始和结束
//原始数据
//--------------------^^^^^^--^^^^^----^^^^^--------------------
//处理后的数据
//000000000000--------^^^^^^--^^^^^----^^^^^-------0000000000000
//0000 就是删除掉的数据
//总之：就是掐头去尾，使要上传的数据尽量少且人声部分不能丢失

#if defined(ENABLE_OPUS)
int process_record_data(unsigned char *buffer, unsigned int len)		//执行
{
    int ret = 0;

	if (cdb_get_int("$wanif_state", 0) !=2) 
    {
        wav_preload("/tmp/res/speaker_not_connected.wav");
        stop_record_data();
        return ret;
    }
    
    if(first_put == 0)
    {   
    //	info("111111111");
        upload_flag = 0;
        voice_flag = 0;
        turingWakeupStartArgs.isWakeUpSucceed = 0;
        process_record_data_init();
    }
    else if(turingWakeupStartArgs.isWakeUpSucceed)  //打断交互
    {
    	
	//	info("22222222222");
        first_put = 0;
        voice_flag = 0;
        silence_size = 0;
        keep_silence_size = 0;
        upload_flag = 0;
        turingWakeupStartArgs.isWakeUpSucceed = 0;
        process_record_data_init();
    }

	if(IsCaptureCancled())
	{
		info("Capture audio pthread cancled ");
		stop_record_data();
		SetCaptureState(CAPTURE_NONE);	
	}
	
#if WRITE_TO_FILE
    int write_size = fwrite(buffer,1,len,record_fp);
    fflush(record_fp);
#endif
    if(voice_flag == 0)
    {
        process_record_data_no_sound(buffer,len);
    }
    else  //已经检测到过人声了，只需要检测VAD末端
    {
        process_record_data_has_sound(buffer,len);
    }
//	debug("44444444444444444\n");
    return ret;
}

#else
int process_record_data(unsigned char *buffer, unsigned int len)
{
    unsigned int length = 0;
    int ret = 0;
    if (cdb_get_int("$wanif_state", 0) !=2) {
		wav_preload("/tmp/res/speaker_not_connected.wav");
		//turingWakeupStartArgs.vad_end = 1;
	     turingWakeupStartArgs.isMutiVadListening=0;
	      return ret;
		}
	
    if(first_put == 0)
    {
       StopAllPlayer();
#if WRITE_TO_FILE
        record_fp = fopen("/tmp/record.pcm", "w+");
        if (record_fp == NULL)
        {
            printf("open 1.raw failed.\n");
            exit(-1);
        }
#endif
        long sec = time(NULL);
        turingWakeupStartArgs.wakeup_waitctl->enter_listen(turingWakeupStartArgs.wakeup_waitctl);
        wav_preload(tips[sec%5]);
         //system("aplay /root/iot/qing_jiang.wav");
        ASP_clear_data();
        turingWakeupStartArgs.wakeup_waitctl->exit_listen(turingWakeupStartArgs.wakeup_waitctl);
        info("设置录音状态位");
        SetCaptureState(2);
        getuuidString(g.identify);
        CaptureFifoClear(); //第一次要清空原有的数据，可解决打断唤醒时原有数据仍残留的问题
        first_put = 1;
    }
#if WRITE_TO_FILE
    int write_size = fwrite(buffer,1,len,record_fp);
    fflush(record_fp);
#endif

    if(voice_flag == 0)
    {
        ret = vad((short *)buffer,len/2);
        if(ret > 0)
        {
            voice_flag = 1;
            CaptureFifoPut(buffer,len);
            if(upload_flag == 0)
            {
                info("识别到人声，开始通知上传,长度 = %d",CaptureFifoLength());
                NewHttpRequest();
                upload_flag = 1;
            }
        }
        else
        {
            CaptureFifoClear();         //删掉上一次的数据
            CaptureFifoPut(buffer,len); //保留本次静音数据
            keep_silence_size += len;
            if(keep_silence_size > 5*16*1024*2)  //一直静音，超过5秒，结束本次对话
            {
                keep_silence_size = 0;
                info("静音5秒钟时间到");
              //  turingWakeupStartArgs.vad_end = 1;
               turingWakeupStartArgs.isMutiVadListening=0;
                upload_flag = 0;       //为下一次通知上传做准备
                SetCaptureState(0);    //通知上传线程，已经录音结束
                voice_flag = 0;
                first_put = 0;
                silence_size = 0;
#if WRITE_TO_FILE                    
                if(record_fp)
                {
                    fclose(record_fp);
                    record_fp = NULL;
                }
#endif
                
                AddMpg123Url("http://turing-iot.oss-cn-beijing.aliyuncs.com/err/chat/40004_5_6_0mp3.mp3");
            }
        }
    }
    else  //已经检测到过人声了，只需要检测VAD末端
    {
        CaptureFifoPut(buffer,len);
        ret = vad((short *)buffer,len/2);
        if(ret == 0)
        {
            debug("检测不到人声了");
            silence_size += len;
            if(silence_size > 16*1024)  //0.5S检测不到声音，结束录音
            {
                info("静音数据超过16K，结束本次录音");
               // turingWakeupStartArgs.vad_end = 1;
                turingWakeupStartArgs.isMutiVadListening=0;
                upload_flag = 0;       //为下一次通知上传做准备
                SetCaptureState(0);    //通知上传线程，已经录音结束
                voice_flag = 0;
                first_put = 0;
                silence_size = 0;
                keep_silence_size = 0;
#if WRITE_TO_FILE                    
                if(record_fp)
                {
                    fclose(record_fp);
                    record_fp = NULL;
                }
#endif
                if(CaptureFifoLength() > len)
                {
                    CaptureFifoDelData(len);  //把最后一次的静音数据删掉
                    debug("len final = %d",CaptureFifoLength());
                }
                return 1;
            }
        }
        else
        {
            silence_size = 0;
            total_size += len;
            if(total_size > 15*16000 *2)
            {
                //turingWakeupStartArgs.vad_end = 1;
                 turingWakeupStartArgs.isMutiVadListening=0;
                upload_flag = 0;       //为下一次通知上传做准备
                SetCaptureState(0);    //通知上传线程，已经录音结束
                voice_flag = 0;
                first_put = 0;
                silence_size = 0;
                keep_silence_size = 0;
                total_size = 0;
            }
        }
        debug("record len = %d",CaptureFifoLength());
    }

    return ret;
}
#endif
void StopAllPlayer(void)
{

	if(IsMpg123PlayerPlaying())	//当TTS播放的时候，新的唤醒可以设置打断TTS播放
    {
#ifdef ENABLE_CONTINUOUS_INTERACT
//    	set_interact_status(0);		//打断本次,开始新的交互	
		InterruptInteractionMode();
#endif
        set_intflag(1);
		usleep(150*1000);
    }

	if(MpdGetPlayState()==2) //正在播放
    {
        recover_playing_status = 1;
        MpdPause();
    }
	
#if defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		if(getMutMicStatus() == 1)
			exec_cmd("uartdfifo.sh StartConnectedFAL");
		else
			StartPowerOn_led();
#endif

    if(alert_playing_now())
    {
        stop_alert();
        killall("aplay",9);
		if(IsMpg123PlayerPlaying())
			set_intflag(1);
    }
    
    SetHttpRequestCancle();
}


/* END:   Added by Frog, 2018/7/12 */

int frist_flag=0;
FILE *testrecord_fp = NULL;
#define SINVOICE_FILE 1
int get_data_for_sinvoice(unsigned char *buffer, unsigned int len)
{
#if SINVOICE_FILE	
	if(!frist_flag){
		testrecord_fp = fopen("/tmp/sinvoice.pcm", "w+");
        if (testrecord_fp == NULL)
        {
            printf("open sinvoice.pcm failed.\n");
            exit(-1);
        }
		frist_flag = 1;
	}
		
	int write_size = fwrite(buffer,1,len,testrecord_fp);
    fflush(testrecord_fp);
#endif
    CaptureFifoPut(buffer,len);
}


void start_record_sinvoice_data()
{
#if SINVOICE_FILE
	frist_flag = 0;
#endif
    turingWakeupStartArgs.isMutiVadListening = 4;
}


int get_mic_record_status()
{
    return turingWakeupStartArgs.isMutiVadListening;
}

void set_mic_record_status(int state)
{
    turingWakeupStartArgs.isMutiVadListening = state;
#if SINVOICE_FILE
	if(testrecord_fp != NULL)
	{
		fclose(testrecord_fp);
		testrecord_fp = NULL;
	}
#endif
}


int process_data_from_mic(unsigned char *buffer, unsigned int len,void *userdata)
{
    int type = turingWakeupStartArgs.isMutiVadListening;
    switch(type)
    {
        case 1:process_record_data(buffer,len);break;	//语音交互
        case 2:get_data_for_wechat(buffer,len);break;	//微聊
        case 3:process_record_data(buffer,len);break;	//中英翻译
        case 4:get_data_for_sinvoice(buffer,len);break;	//声波配网
    }
}


int   CreateListenWakeUpEvent(void)
{
    CaptureFifoInit();
#if defined(CONFIG_KWD_SENSORY)
    sensory_init();
#endif

    turingWakeupStartArgs.wakeup_mode=0;
    turingWakeupStartArgs.pRecordDataInputCallback = process_data_from_mic;
    turingWakeupStartArgs.pCancelupload = SetHttpRequestCancle;
    turingWakeupStartArgs.pStopAllAudio = StopAllPlayer;
    turingWakeupStartArgs.wakeup_waitctl = posix_thread_ops_create(NULL);
    info("wakeup_waitctl=[%p]",turingWakeupStartArgs.wakeup_waitctl);
	if(!turingWakeupStartArgs.wakeup_waitctl){
		err("wakeup_waitctl failure");
		return false;
	}
    //mic是否保存状态
#if 0    
    else
    {
        if(1==cdb_get_int("$ra_mute",5)) //开机即为麦克风关闭状态
        {
            turingWakeupStartArgs.wakeup_waitctl->isListenSuspend = 1;
        }
    }
#endif    
    struct sched_param param;
    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    param.sched_priority = 60;
    pthread_attr_setschedpolicy (&attr, SCHED_RR);
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    turingWakeupStartArgs.pAttr=&attr;
    int err  = wakeupStart(&turingWakeupStartArgs);
    if(err<0)
    {
        printf("wakeupStart failure,err=%u",err);
        return -2;
    }

    return 0;
}

int   DestoryListenWakeUpPthread(void)
{
    if (turingWakeupStartArgs.wakeupPthreadId != 0)
    {
        pthread_join(turingWakeupStartArgs.wakeupPthreadId, NULL);
        info("capture_destroy end...\n");
    }
    CaptureFifoDeinit();
}



void micarray_mode_check(int argc, char** argv)
{
    char *record_time = NULL; 
	//if(!strcmp(get_last_name(argv[0]),"micarray"))
	if(argc ==1)
        return ;
    if(!strcmp("micarray",basename(argv[0])))
    {
        record_time = argv[1];
    }
    else if(!strcmp("turingIot",basename(argv[0])) && !strcmp("micarray",argv[1]))
    {
        record_time = argv[2];
    }
    else
    {
        return ;
    }
	wakeupStartArgs_t wakeupStartArgs={0};
	wakeupStartArgs.wakeup_waitctl = posix_thread_ops_create(NULL);
	if(!wakeupStartArgs.wakeup_waitctl)
    {
		error("posix_thread_ops_create failure");
		return -1;
	}
#if defined(CONFIG_KWD_SENSORY)
	sensory_init();
#endif
	if(record_time && atoi(record_time))
    {
		wakeupStartArgs.wakeup_mode=1;
	}
	struct sched_param param;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	param.sched_priority = 60;
	pthread_attr_setschedpolicy (&attr, SCHED_RR);
	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	wakeupStartArgs.pAttr=&attr;
    int err  = wakeupStart(&wakeupStartArgs);
	if(err<0){
		error("wakeupStart failure,err=%d",err);
		return -2;
	}
	info("recorder thread is OK!");
	if(record_time){
		if(atoi(record_time)>0){
			sleep(atoi(record_time));
		}else{
			while(1)pause();
		}
	}else{
		sleep(20);
	}
	exit(0);
	return 0;
}

