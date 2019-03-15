#include "thread.h"
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "myutils/debug.h"

#ifdef ENABLE_YIYA
#include "PlayThread.h"
#endif
#ifdef ENABLE_OPUS
#include "opus/opus_audio_encode.h"
#endif

#ifdef USE_UPLOAD_AMR
#include "amr_enc.h"
#endif


void thread_init()
{
#if defined(USE_UPLOAD_AMR) 
	CreateAmrEncodePthread();
#endif
#if defined(ENABLE_OPUS)
    CreateOpusEncodePthread();
#endif

    //录音线程
    //CreateCapturePthread(); 
    //微信聊天线程
    //CreateChatPthread();

    CreateMpg123Thread();
#ifdef USE_MSG_QUEUE
    create_msg_queue_monitor_thread();
//#else
    CreateMonitorPthread();
	CreateKeyControlPthread();		//按键处理的线程
#endif
    //上传录音数据、接收语音解析结果线程
    CreateHttpRequestPthread();
    CreateHttpRecvPthread();


#ifdef ENABLE_HUABEI
    CreateHuabeiDispatcherPthread();
#else
    //MQTT队列消息调度线程
    CreateDispatcherPthread();
    CreateListenWakeUpEvent();
    CreateCollectPthread();
    
    //CreateVoiceDecoderPthread();
    CreateRTVoiceDecoderPthread();
#endif
#ifdef ENABLE_ULE_IFLYTEK
    CreateIflytekPthread();
#endif
#ifdef ENABLE_YIYA
    CreatePlayThread();
#endif
}

void thread_deinit()
{	
#ifdef ENABLE_YIYA
	DestoryPlayThread();
#endif
#ifdef ENABLE_ULE_IFLYTEK
	DestoryIflytekPthread();
#endif

#ifdef ENABLE_HUABEI	
	DestoryHuabeiDispatcherPthread();
#else
	DestoryDispatcherPthread();
	DestoryCollectPthread();
#endif
	DestoryHttpRequestPthread();
    DestoryHttpRecvPthread();

#ifdef USE_MSG_QUEUE
	destory_msg_queue_monitor_thread();
//#else
    DestoryMonitorPthread();
	DestoryKeyControlPthread();

#endif
//	DestoryChatPthread();
//	DestoryCapturePthread();
    DestoryMpg123Thread();
    DestoryListenWakeUpPthread();
    //DestoryVoiceDecoderPthread();
    DestoryRTVoiceDecoderPthread();

#if defined(USE_UPLOAD_AMR)
	DestoryAmrEncodePthread();
#endif
#if defined(ENABLE_OPUS)
    DestoryOpusEncodePthread();
#endif

}


