#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <json/json.h>
#include <curl/curl.h>
#include "TuringWeChatList.h"
#include <wakeup/interface.h>
#include "wavwriter.h"

#include "myutils/debug.h"
#include "common.h"
#include "DeviceStatus.h"
#ifdef ENABLE_COMMSAT
#include "commsat.h"
#endif
#ifdef      ENABLE_HUABEI
#include "HuabeiDispatcher.h"
#endif
extern int recover_playing_status;

static sem_t chatSem;
static pthread_t chatPthread;
static pthread_mutex_t playingMutex = PTHREAD_MUTEX_INITIALIZER;
static int please_shutdown = 0;
static int chatWait = 0;
static pthread_mutex_t chatMtx = PTHREAD_MUTEX_INITIALIZER;


void SetChatWait(int wait)
{
	pthread_mutex_lock(&chatMtx);
	chatWait = wait;
	pthread_mutex_unlock(&chatMtx);
}

/* 开启本地微聊. */
void StartChat()
{
	pthread_mutex_lock(&chatMtx);
	if(chatWait == 0) {
		sem_post(&chatSem);
	}
	pthread_mutex_unlock(&chatMtx);

}
void ChatStopAllPlayer(void)
{
    if(MpdGetPlayState()==2) //正在播放
    {
        recover_playing_status = 1;
        MpdPause();
    }
    if(IsMpg123PlayerPlaying())
    {
        set_intflag(1);
    }

    SetHttpRequestCancle();
}


 
/* 处理公众号传来的微聊消息. */
int ParseChatData(json_object *message)
{
	json_object *url = NULL, *tip = NULL;
	json_object *relationId = NULL, *relationName = NULL;
	char *speech = "接收到来自微信的消息";
	char *urlStr = NULL, *relationNameStr = NULL;
	char *tipStr = NULL;
	int relationIdInt;
	warning("message:%s",json_object_to_json_string(message));
// message:{ "operate": 0, "url": "http:\/\/turing-iot.oss-cn-beijing.aliyuncs.com\/tts\/tts-5df506b0d3c34efdb6f0a8b8fc044a33.amr", "relationId": 0 }
	unlink("/root/chat.raw");
	unlink("/root/tip.raw");
	tip  =	json_object_object_get(message, "tip");
	if(tip) {
		tipStr = strdup(json_object_get_string(tip)); 
		info("tipStr:%s",tipStr);
		/* 下载到本地 */
		HttpDownLoadFile(tipStr, "/tmp/tip.amr");
		amr_dec("/tmp/tip.amr", "/root/tip.raw");
	}
	relationName =	json_object_object_get(message, "relationName");
	if(relationName) {
		relationNameStr = strdup(json_object_get_string(relationName)); 
		info("relationNameStr:%s",relationNameStr);
	}
	relationId =	json_object_object_get(message, "relationId");
	if(relationId) {
		relationIdInt = json_object_get_int(relationId); 
		info("relationIdInt:%d",relationIdInt);
	}
	url  =	json_object_object_get(message, "url");
	if(url) {
		urlStr = strdup(json_object_get_string(url)); 
		info("urlStr:%s",urlStr);
		/* 下载到本地 */
		HttpDownLoadFile(urlStr,"/tmp/chat.amr");
		
		amr_dec("/tmp/chat.amr", "/root/chat.raw");
	
	}
	
#ifdef ENABLE_ALL_FUNCTION
	text_to_sound(speech);
#else
	ChatStopAllPlayer();	//暂停所有播放状态

	/* mpc 静音 */
	//eval("mpc", "volume", "101");
	/* 立即播放 */
	if(relationIdInt != 0) {	//aplay -r 8000 -f s16_le -c 1 /root/chat.raw
		eval("aplay", "-r", "8000", "-f" , "s16_le", "-c" ,"1", "/root/tip.raw");	
	} 
	eval("aplay", "-r", "8000", "-f" , "s16_le", "-c" ,"1", "/root/chat.raw");	//播放微信语音
	/* mpc 恢复 */
	//eval("mpc", "volume", "200");
	 if(recover_playing_status )
    {
        //usleep(200*1000);
        exec_cmd("mpc play");
        recover_playing_status = 0;
    }
#endif

	time_t curr = time(NULL);
	/* 创建WeChatItem并添加到weChatList. */
	TuringWeChatListAdd(curr, urlStr, NULL, tipStr, relationNameStr, relationIdInt);
	
}

/*
 * 微聊线程. 
 * 1.录音
 * 2.将录音文件转成amr的格式
 * 3.上传到微信公众号
 */
void * ChatPthread(void)
{	
	char *body ;
	int ret;
	
	while (1)
	{
		
		SetChatState(0);
		SetChatWait(0);
		ResumeMpdState();
		sem_wait(&chatSem);
		SetChatState(2);
		SetChatWait(1);
#ifdef ENABLE_COMMSAT
		OnWriteMsgToCommsat(CAPTURE_START,NULL);
#endif
	
		eval("mpc", "volume", "101");
		/* 微聊录音 */
		ret = capture_turing_chat("/tmp/8k.wav");
		eval("mpc", "volume", "200");
				
#ifdef ENABLE_COMMSAT
		OnWriteMsgToCommsat(CAPTURE_END,NULL);
#endif	
		/* 
		 * 判断是正在录音时取消这次Session 
		 * 则不执行后续操作.
		 */
		if(IsChatCancled()) {
			warning("isChatCancled...");
			SetChatState(3);
			continue;
		}
		SetChatState(3);
#ifndef      ENABLE_HUABEI
		EventQueuePut(EVENT_UPLOAD_FILE, NULL);
#else
		EventQueuePut(HUABEI_UPLOAD_FILE_EVENT, NULL);
#endif

	}
}

/* 启动微聊的线程. */
void CreateChatPthread(void)
{
	int iRet;

	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*2);
	
	iRet = pthread_create(&chatPthread, &a_thread_attr, ChatPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);

	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}
}

/* 销毁微聊的线程. */
void DestoryChatPthread()
{
	
	if (chatPthread != 0)
	{
		pthread_join(chatPthread, NULL);
	}
}


