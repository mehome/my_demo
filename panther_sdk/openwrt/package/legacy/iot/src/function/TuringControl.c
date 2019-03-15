#include "TuringControl.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "mpdcli.h"
#include <signal.h>
#include <myutils/ConfigParser.h>
#include <wakeup/interface.h>
#include <myutils/msg_queue.h>
#include "mon_config.h"
#include "capture.h"
#include "turing.h"
#include "json.h"
#ifdef ENABLE_COMMSAT
#include "commsat.h"
#endif
#ifdef ENABLE_HUABEI
#include "HuabeiMonitorControl.h"
#endif

#include "globalParam.h"

#include "common.h"


static int captureWait = 0;
static int httpRequestWait = 0;
static int mpg123Wait = 0;
static int chatWait = 0;
static int wavPlay = 0;
int tfPlay_flag = 0;
extern int tfSave_flag;
int tfFirst_flag = 0;
extern int recover_playing_status ;
char *CurrentMutual_url = NULL;
AudioStatus *audioAfterPlayStatus =NULL;
AudioStatus *audioBeforPlayStatus =NULL;
//int audioFirstPlayFlag=2;

static pthread_mutex_t captureMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t captureCond = PTHREAD_COND_INITIALIZER; 

static pthread_mutex_t httpRequestMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t httpRequestCond = PTHREAD_COND_INITIALIZER; 


static pthread_mutex_t chatMtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t chatCond = PTHREAD_COND_INITIALIZER;
#define TURING_FLYING
//#define ENABLE_YIYA

void StartHttpRequestNewSession()
{	
	pthread_mutex_lock(&httpRequestMtx);  
	if(httpRequestWait == 1) {
		pthread_cond_signal(&httpRequestCond);  
	}
	pthread_mutex_unlock(&httpRequestMtx);  
}

void StartCaptureNewSession()
{	
	pthread_mutex_lock(&captureMtx);  
	if(captureWait == 1) {
		pthread_cond_signal(&captureCond);  
	}
	pthread_mutex_unlock(&captureMtx);  
}


void StartChatNewSession()
{	
	pthread_mutex_lock(&chatMtx);  
	if(chatWait == 1) {
		pthread_cond_signal(&chatCond);  
	}
	pthread_mutex_unlock(&chatMtx);  
}


static void StartHttpRequest()
{	
	pthread_mutex_lock(&httpRequestMtx);   
	if(!IsHttpRequestFinshed()) {
			SetHttpRequestState(5);
			while(!IsHttpRequestFinshed()) {
				httpRequestWait = 1;
			}
			httpRequestWait = 0;
			error("HttpRequest is finished..");
	}
	pthread_mutex_unlock(&httpRequestMtx);     
}

/* 结束本次录音的过程. */
static void WaitForCaptureFinshed()
{	
	
	if(!IsCaptureFinshed()) {
		debug("Set Capture Cancle...");
		SetCaptureState(5);
		while(!IsCaptureFinshed()) {
		debug("doing  Capture Cancle................");
		}
	}
	debug("new capture start...");
}
/* 结束本次微聊的过程. */
static void WaitForChatFinshed()
{	
 
	if(!IsChatFinshed()) {
		debug("Last Chat Not Finshed....");
		SetChatState(5);
		debug("Set Chat Cancle...");
		while(!IsChatFinshed()) {
		debug("doing  Chat Cancle................");
		}
		debug("HttpRequest is finished..");
	}


}
/* 结束本次上传的过程. */
static void WaitForHttpFinshed()
{		 
	if(!IsHttpRequestFinshed()) {
		debug("Set Capture Cancle...");
		SetHttpRequestState(5);
		while(!IsHttpRequestFinshed()) {

		}
	}
	debug("new http start...");
}

/* 结束本次TTS播放的过程. */
static void WaitMpg123Finshed()
{		 
	if(!IsMpg123Finshed()) {
		debug("Set Mpg123 Cancle...");
		SetMpg123Cancle();
	}
	while(!IsMpg123Finshed()) 
	{
		if(GetMpg123Wait()){
			debug("doing Mpg123 Cancle................");
			usleep(100*1000);
		}
	}
	
	info("new http start...");
}



/* 结束当前的回话. */
void FinshLastSession()
{
#ifdef ENABLE_YIYA
	WaitForPlayThreadFinish();
#endif
	exec_cmd("killall -9 wavplayer");
	exec_cmd("killall -9 aplay");
#ifdef ENABLE_CONTINUOUS_INTERACT	
	//set_interact_status(0);		//结束连续交互
		EndInteractionMode();
#endif
	if(translation_flag)
		translation_flag=0;
	AlertFinshed();
	CaptureFifoClear();
	SetHttpRequestCancle();
	MpdVolume(200);
    WaitMpg123Finshed();
	WaitForChatFinshed();
	WaitForCaptureFinshed();
#ifdef ENABLE_ULE_IFLYTEK
	WaitForIflytekFinshed();
#endif

}

/* 开启新的回话. */
void StartNewSession()
{
	exec_cmd("killall -9 wavplayer");
#ifdef ENABLE_YIYA
	WaitForPlayThreadFinish();
	StopBootMusic();
#endif
	AlertFinshed();
	SaveMpdState(); 
	MpdClear();
	MpdVolume(200);
	CaptureFifoClear();
#ifdef USE_CURL_MULTI
	SetHttpRequestCancle();
	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);
	if (g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle != NULL)
	{
		info("Clear_easy_handler\n");
		ClearEasyHandler(g->m_RequestBoby[RECOGNIZER_INDEX].easy_handle);
	}
#else
	SetHttpRequestCancle();
#endif
	WaitForChatFinshed();
	WaitForCaptureFinshed();
#ifdef ENABLE_ULE_IFLYTEK
	WaitForIflytekFinshed();
#endif


}
/* 用于连续交互开启新的回话. */
void StartAiNewSession()
{
	exec_cmd("killall -9 wavplayer");
#ifdef ENABLE_YIYA
	WaitForPlayThreadFinish();
	StopBootMusic();
#endif
	AlertFinshed();
	//SaveMpdState(); 
	MpdClear();
	MpdVolume(200);
	CaptureFifoClear();
	WaitForHttpFinshed();
	WaitForChatFinshed();
	WaitForCaptureFinshed();
#ifdef ENABLE_ULE_IFLYTEK
	WaitForIflytekFinshed();
#endif

}
/* 系统唤醒. */
void TuringWakeUp()
{
	error("TuringWakeUp");
	cdb_set_int("$turing_stop_time", 1);
	cdb_set_int("$turing_power_state", 0);
	ResumeMpdState();
}

/* 主动结束交互. */
void TuringAiStandby()
{
//#ifndef ENABLE_YIYA 
//	cdb_set_int("$turing_power_state", POWER_STATE_STANDBY);
//	cdb_set_int("$turing_stop_time", 0);
//#else
#ifdef TURN_ON_SLEEP_MODE
	EnterSleepMode();
#endif
//#endif
#ifdef ENABLE_COMMSAT
	OnWriteMsgToCommsat(STANDY, NULL);
#endif
#ifdef ENABLE_YIYA
	SetResumeMpd(1);	
#else
	SetResumeMpd(1);	
#endif
	FinshLastSession();
	error("turing standby...");
#ifndef ENABLE_YIYA
	//eval("uartdfifo.sh", "standby");
#endif
		
}

/* 系统进入待机模式. */
void TuringStandby()
{

	cdb_set_int("$turing_power_state", POWER_STATE_STANDBY);
	cdb_set_int("$turing_stop_time", 0);
#ifdef ENABLE_COMMSAT
	OnWriteMsgToCommsat(STANDY, NULL);
#endif
	SetResumeMpd(0);
	StartNewSession();
	error("turing standby...");

	char *test = "我休息一会，待会再聊吧";
	
	text_to_sound(test);
#ifdef TURN_ON_SLEEP_MODE
	EnterSleepMode();
#endif

#ifdef     ENABLE_YIYA 	
	eval("uartdfifo.sh", "standby");
#endif

}

/* 系统休眠功能. */
void TuringSuspend()
{

#ifdef ENABLE_COMMSAT
	OnWriteMsgToCommsat(SUSPEND, NULL);
#endif
	SaveMpdState();
	cdb_set_int("$turing_power_state", POWER_STATE_SUSPEND);
	cdb_set_int("$turing_stop_time", 0);
#ifdef TURN_ON_SLEEP_MODE
	EnterSleepMode();
#endif

	exec_cmd("mpc stop");

	StartNewSession();
	error("turing suspend...");

	char *test = "宝宝，我该睡觉啦，等我醒来再聊吧";
	text_to_sound(test);

}
/* 系统resume. */
void TuringResume()
{
	error("turing resume..");
#ifdef ENABLE_COMMSAT
	OnWriteMsgToCommsat(RESUME, NULL);
#endif
	ResumeMpdState();
	cdb_set_int("$turing_stop_time", 1);
	cdb_set_int("$turing_power_state", 0);
#ifdef   ENABLE_YIYA 	
	int wanif_state = 0;
	wanif_state = cdb_get_int("$wanif_state", 0);
	if(wanif_state == 2)
		wav_play2("/tmp/res/starting_up.wav");
	else
		wav_play2("/tmp/res/speaker_not_connected.wav");
	eval("uartdfifo.sh", "resume");
#endif

}
/* YIYA连续五分钟没操作事件. */
void TuringFive()
{
	int state = 0, pause = 0;
	state = MpdGetPlayState();
	warning("state:%d",state);
	if(state == MPDS_PLAY) {
		pause = 1;
		MpdPause();
	}
	wav_play2("/tmp/res/idle_five.wav");
	if(pause) {
		warning("resume play...");
		MpdPlay(-1);
	}

}
/* YIYA连续十分钟没操作事件. */
void TuringTen()
{
	int state = 0, pause = 0;
	state = MpdGetPlayState();
	warning("state:%d",state);
	if(state == MPDS_PLAY) {
		pause = 1;
		MpdPause();
	}
	wav_play2("/tmp/res/idle_ten.wav");
	if(pause) {
		warning("resume play...");
		MpdPlay(-1);
	}

}

/* 播放最后一次的微聊内容. */
static void ChatPlay()
{
	char *snd = "你还没有收到来自微信的消息";
	int state = 0, pause = 0;
	int ret = 0;

#if 0
	if((access("/root/chat.raw",F_OK))==0)
	{
		exec_cmd("aplay -r 8000 -f s16_le -c 1 /root/chat.raw");
	} else {
		ColumnTextToSound(snd);
	}
#else
	ret = TuringWeChatListPlay(NULL);
	if(ret < 0)
		ColumnTextToSound(snd);
#endif

}

/* 播放微聊文件. */
void TuringChatPlay()
{
#ifdef TURN_ON_SLEEP_MODE
	SetAiMode(0);
	SetErrorCount(4);
#endif
	SetResumeMpd(1);
	StartNewSession();
	ChatPlay();
	ResumeMpdState();
}

/* 开启语音交互. */
void TuringSpeechStart(int type)
{
	int powerState =  cdb_get_int("$turing_power_state", 0);
	if(powerState == POWER_STATE_STANDBY) 
    {
#ifdef ENABLE_COMMSAT
		OnWriteMsgToCommsat(RESUME, NULL);
#endif
		cdb_set_int("$turing_power_state", POWER_STATE_ON);
	}
	SetTuringType(type);
#ifdef TURN_ON_SLEEP_MODE
	SetIsChat(0);
	SetAiMode(0);
	SetErrorCount(0);
#endif
    if(GetCaptureWait() || GetHttpRequestWait())  //在语音交互状态
    {
        SetInterruptState(1);
    }
	StartNewSession();
    
    while(GetCaptureWait()){info("------------------");usleep(100);}   //停止需要时间，这里等待一下
    while(GetHttpRequestWait()){info("+++++++++++++++++");usleep(100);}
	info("StartNewSession");
	StartCaptureAudio();

	cdb_set_int("$turing_stop_time", 0);
}

/* 自动交互. */
void TuringActiveInteraction()
{
#ifdef TURN_ON_SLEEP_MODE
	SetIsChat(0);
	SetAiMode(0);
	SetErrorCount(0);
#endif
	StartNewSession();
	info("TuringActiveInteraction");

	SetTuringType(1);
#ifdef USE_CURL    
	submitActiveInteractioEvent();
#endif
}

/* 用于连续交互时开启新的交互. */
void TuringAiStart()
{
	SetAiMode(1);
	StartAiNewSession();
	info("StartNewSession");
	StartCaptureAudio();						
	cdb_set_int("$turing_power_state", POWER_STATE_ON);
	cdb_set_int("$turing_stop_time", 0);
}

static int isChat = 0;
/* 设置是否是微聊模式. */
void SetIsChat(int mode)
{
	isChat = mode;
}
int GetIsChat()
{
	return isChat;
}

/* 开启微聊. */
void TuringChatStart()
{
#ifdef TURN_ON_SLEEP_MODE
	SetIsChat(1);
	SetAiMode(0);
	SetErrorCount(4);
#endif
	SetResumeMpd(1);
	StartNewSession();
	StartChat();
}

/* 音乐播放事件. */
void TuringPlayEvent()
{	
	int state ;
	struct mpd_song *song = NULL;
	int playmode = cdb_get_int("$turing_mpd_play", 0);
	if(playmode)
		PrintSysTime("mpc play ok");
	state = MpdGetPlayState();
	IotAudioStatusReport();
	 warning("state:%d",state);
	if(state == MPDS_PAUSE) 
	{
		/* 
		 * turing_stop_time == 1 表示没有音乐正播放
		 * monitor进程会根据这个进行判断
		 * 如果长时间没有播放,系统会进入到休眠状态
		 */
		cdb_set_int("$turing_stop_time",1);		
	} 
	else if(state == MPDS_PLAY)
	{
		cdb_set_int("$turing_stop_time",0);	
		StartTlkPlay_led();
	}

	warning("after TuringPlayEvent");
}

/* 音乐停止播放事件. */
void TuringStopEvent()
{	
	//IotDeviceStatusReport();						
	cdb_set_int("$turing_stop_time", 1);
	info("$turing_mpd_play =%d,$key_request_song =%d",cdb_get_int("$turing_mpd_play", 0),cdb_get_int("$key_request_song", 0));
#if defined(CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_DOSS_1MIC_V1)
	if(cdb_get_int("$key_request_song", 0) != 4)		//区分按键请求歌曲/自动播完请求歌曲
	{
		if(cdb_get_int("$turing_mpd_play", 0) == 4){
			IotGetAudio(0); 						//向服务器请求下一曲
		}
	}
#endif

#ifdef ENABLE_YIYA
	int mpd = cdb_get_int("$turing_mpd_play", 0);
	if(mpd == 4) {
		IotGetAudio(0); 						
	}
#endif
	cdb_set_int("$key_request_song", 0);
	StartPowerOn_led();
	
}

/* tf卡插入或拔掉事件. */
void TuringUpdateDeviceEvent()
{
	IotDeviceStatusReport();
	int tfStatus = cdb_get_int("$tf_status", 0);
	info("tfStatus is:%d",tfStatus);
	if(tfStatus == 0)
	{
		ColumnAudioFree();
		RemovePlayListFile();
	} 
	else 
	{
		exec_cmd("mpc update");
#ifdef ENABLE_YIYA
		SyncCollectResource();
#endif
	}
#ifndef ENABLE_YIYA
	SyncLocalResource();
#endif	
}

static int AiMode = 0;

void SetAiMode(int mode)
{
	AiMode = mode;
}

int GetAiMode()
{
	return AiMode;
}

/* 连续交互模式. */
#ifdef TURN_ON_SLEEP_MODE
void InteractionMode()
{
	int type = GetTuringType();
	info("type:%d",type);
	if(type != 0) 
    {
		exec_cmd("uartdfifo.sh tlkoff");
		ResumeMpdState();
		return;
	}
	/* 连续五次没有说话或者返回错误则结束交互并恢复之前的mpd状态 */
    if(ShouldEnterSleepMode() || IsInterrupt()) 
    {
		info(" Should Enter SleepMode ....");
		exec_cmd("uartdfifo.sh tlkoff");
		ResumeMpdState();
        SetInterruptState(0);
	} else {
	
		info(" start ai mode ....");
#ifdef USE_MPDCLI
			MpdRepeat(0);
#else
			exec_cmd("mpc repeat off");
#endif			

#ifdef ENABLE_COMMSAT
		OnWriteMsgToCommsat(VOICE_INTERACTION, NULL);
#endif
		SetAiMode(1);
		exec_cmd("uartdfifo.sh tlkon");
	}
}
#endif

/*
 * 一次交互的内容播放完成. 
 * 1.连续交互模式,就进行下次交互
 * 2.不是连续交互模式,则恢复mpd状态
 */
void TuringPlayDoneEvent()
{

	int state = MpdGetPlayState();
	error("state:%d",state);
	if(state == MPDS_PLAY || state == MPDS_PAUSE) {
		error("state == MPDS_PLAY || state == MPDS_PAUSE");
		return;
	}
	
#ifdef ENABLE_YIYA
	int mpd = cdb_get_int("$turing_mpd_play", 0);
	if(mpd == 4) {
#ifdef TURN_ON_SLEEP_MODE
		EnterSleepMode();
#endif
		IotGetAudio(0); //向服务器请求下一曲
		return;
	}
#endif

#ifdef TURN_ON_SLEEP_MODE
	/* 如果当前是微聊则不处理 */
	if(GetIsChat())
    {
		return;
	}
	InteractionMode();
#else
	ResumeMpdState();		//执行
#endif

#ifdef ENABLE_COMMSAT
	OnWriteMsgToCommsat(VOICE_PLAY_END, NULL);
#endif

	cdb_set_int("$turing_mpd_play", 0);	

}

/* 收藏. */
void TuringCollectSong()
{
	FreeMpdState();
#ifdef TURN_ON_SLEEP_MODE
	SetAiMode(0);
	SetErrorCount(4);
#endif
	FinshLastSession();
	CollectSong();
}
/* 栏目切换. */
void TuringStartColumn()
{
	FreeMpdState();
#ifdef TURN_ON_SLEEP_MODE
	SetAiMode(0);
	SetErrorCount(4);
#endif
	FinshLastSession();
	StartColumn();
	
}

/* 结束录音. */
int TuringCaptureEnd()
{
	SetVADFlag(1);
	
	return 0;
}
/* 更新Turing aesKey和apiKey. */
int TuringConfigChange()
{
	int ret = 0;
	char *aesKeyTuring = NULL;
	char *apiKeyTuring = NULL;
	aesKeyTuring = ConfigParserGetValue("aesKeyTuring");
	apiKeyTuring = ConfigParserGetValue("apiKeyTuring");
	if(aesKeyTuring && apiKeyTuring) {
		SetTuringAesKey(aesKeyTuring);
		SetTuringApiKey(apiKeyTuring);
		char *userId = NULL;
		userId = GetUserID(apiKeyTuring, aesKeyTuring);
		if(userId) {
			SetTuringUserId(userId);
			free(userId);
		}
	}
	
	if(aesKeyTuring) 
		free(aesKeyTuring);
	if(apiKeyTuring) 
		free(apiKeyTuring);
	return ret;
}

void TuringConfigWrite()
{
	int ret;
	ConfigParserSetValue("apiKeyTuring","242ad604899245ea87cc95d2a8fac976");
	ConfigParserSetValue("aesKeyTuring","RJH983ZPWhuMMJ11");
	return ;
}
/* 低电量事件. */
void TuringLowPower()
{
	IotLowPowerReport();
}
/* 正常电量事件. */
void TuringNormalPower()
{
	IotLowPowerReport();
}
/* 充电状态改变事件. */
void TuringChargeEvent()
{
	IotDeviceStatusReport();
}
/* 下一曲事件. */
int TuringMpcNextEvent()
{
	int state = MpdGetPlayState();
	if(state == MPDS_PAUSE || state == MPDS_PLAY)
		return ;
	return CheckTfExist();	
}
void TuringActiveInterEvent()
{
	TuringActiveInteraction();
}

static int mode = 0;

void TuringPlayMusicEvent()
{
	
	IotGetAudio(mode);
	mode = !mode;
}
/* 显示播放列表中的歌曲. */
void TuringListEvent()
{
	MusicListShow();
}

int GetWavPlay()
{
	return wavPlay;
}
void SetWavPlay(int play)
{
	wavPlay = play;
}

extern wakeupStartArgs_t turingWakeupStartArgs;

void process_volume_event(char *cmd)
{
    char buf[4] = {0};
    strcpy(buf,cmd+strlen("SystemVolume"));
    int volume = atoi(buf);
    info("volume = %d",volume);
    MpdVolume(volume);
	//IotDeviceStatusReport();
   // set_system_volume(volume);
}


void process_mic_off_event()
{
	int state=0;;
	int play_state=0;
    //cdb_set_int("$ra_mic",0);
    setMutMicStatus(1);
	StopAllPlayer();
    turingWakeupStartArgs.wakeup_waitctl->enter_listen(turingWakeupStartArgs.wakeup_waitctl);
    wav_preload("/root/iot/mic_off.wav");
}


void process_mic_on_event()
{
	int state=0;
	int play_state=0;
    //cdb_set_int("$ra_mic",1);
    setMutMicStatus(0);
	StopAllPlayer();
    turingWakeupStartArgs.wakeup_waitctl->resum(turingWakeupStartArgs.wakeup_waitctl);
    wav_preload("/root/iot/mic_on.wav");
	if(recover_playing_status)
	{
		MpdPlay(-1);
		recover_playing_status=0;
	}
}


void process_voleme_ledstate_event()
{
	if (cdb_get_int("$wanif_state", 0) != 2) 
    {
        wav_preload("/tmp/res/speaker_not_connected.wav");
        return ;
    }
	int state = MpdGetPlayState();
	if( state == MPDS_PLAY)
		StartTlkPlay_led();
	else
		StartPowerOn_led();
}

void pause_ledstate_event(int state)
{

	if(IsMpg123PlayerPlaying())	//当TTS播放的时候，新的唤醒可以设置打断TTS播放
    {
        set_intflag(1);
#ifdef ENABLE_CONTINUOUS_INTERACT
	//	set_interact_status(0);
		EndInteractionMode();			//结束连续交互
#endif
		usleep(150*1000);
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
    
    /****灯效的控制****/
	if(state == MPDS_PLAY )
	{
	    MpdPause();
		StartPowerOn_led();
	}
	else if(state == MPDS_PAUSE || state == MPDS_STOP)
	{
	    MpdPlay(-1);
		if(state == MPDS_PAUSE)
			StartTlkPlay_led();
		else
			StartPowerOn_led();
	}
}

void playnext_ledstate_event(int state)
{
	if( state == MPDS_PLAY)
		StartTlkPlay_led();
	else
		StartPowerOn_led();
}

void tf_next_event(void)
{
	StopAllPlayer();
	exec_cmd("killall -9 aplay");
	exec_cmd("aplay -D common /root/iot/下一首.wav");	
	exec_cmd("mpc next");
	if(recover_playing_status)
	{
		recover_playing_status=0;
		//MpdPlay(-1);
	}
}

void tf_prev_event(void)
{
	StopAllPlayer();
	exec_cmd("killall -9 aplay");
	exec_cmd("aplay -D common /root/iot/上一首.wav");	
	exec_cmd("mpc prev");
	if(recover_playing_status)
	{
		recover_playing_status=0;
		//MpdPlay(-1);
	}
}
void playprev_ledstate_event(int state)
{
	if( state == MPDS_PLAY)
		StartTlkPlay_led();
	else
		StartPowerOn_led();
}


void process_battary_event(char *byReadBuf)
{
		char chg[4] = {0};
		IotDeviceStatusReport();
		cdb_get_str("$charge_plug",chg,4,"");
		info("chg is :%s",chg);
		
		if(!strcmp(byReadBuf,"BatteryCharge"))										//是否充电
		{
			if(!strcmp(chg,"001"))
			{
				StopAllPlayer();
				exec_cmd("aplay -D common /root/iot/开始充电.wav");	
			}		
		}
		if(!strcmp(byReadBuf,"BatteryLevel")&&(strcmp(chg,"001")))					//电量等级
		{
			if(cdb_get_int("$battery_level",10) <= 2)
			{
				StopAllPlayer();
				exec_cmd("aplay -D common /root/res/cn/lowpower.wav");							//电量低，请充电
			}
		}
		
		if(recover_playing_status)
		{
			recover_playing_status=0;
			MpdPlay(-1);
		}
}		


void tencent_request_next(void)
{
	cdb_set_int("$key_request_song",4);
	StopAllPlayer();
	exec_cmd("killall -9 aplay");
	exec_cmd("aplay -D common /root/iot/下一首.wav");
	if(recover_playing_status){
		recover_playing_status=0;
		//MpdPlay(-1);
	}
	IotGetAudio(0);
}

void tencent_request_prev(void)
{
	cdb_set_int("$key_request_song",4);
	StopAllPlayer();
	exec_cmd("killall -9 aplay");
	exec_cmd("aplay -D common /root/iot/上一首.wav");
	if(recover_playing_status)
	{
		recover_playing_status=0;
		//MpdPlay(-1);
	}
	IotGetAudio(1); 	
}

#if 0
void process_player_state_event(char *byReadBuf)
{
    int state = MpdGetPlayState();
	if(!strcmp(byReadBuf, "PlayerPlayPause"))
	{
		pause_ledstate_event(state);
	}
	else if(!strcmp(byReadBuf, "PlayerPlayNext"))
	{
		cdb_set_int("$key_request_song",4);
#ifdef TURING_FLYING
		StopAllPlayer();
		exec_cmd("killall -9 aplay");
		exec_cmd("aplay -D common /root/iot/下一首.wav");
		if(recover_playing_status)
		{
			recover_playing_status=0;
			//MpdPlay(-1);
		}
#endif

		IotGetAudio(0);					
		playnext_ledstate_event(state);
	}
	else if(!strcmp(byReadBuf, "PlayerPlayPrev"))
	{
		cdb_set_int("$key_request_song",4);
#ifdef TURING_FLYING
		StopAllPlayer();
		exec_cmd("killall -9 aplay");
		exec_cmd("aplay -D common /root/iot/下一首.wav");
		if(recover_playing_status)
		{
			recover_playing_status=0;
			//MpdPlay(-1);
		}
#endif
		IotGetAudio(1);					
		playprev_ledstate_event(state);
	}
   
}
#else
void process_player_state_event(char *byReadBuf)
{
    int state = MpdGetPlayState();
	if(!strcmp(byReadBuf, "PlayerPlayPause"))
	{
		pause_ledstate_event(state);
	}
	else if(!strcmp(byReadBuf, "PlayerPlayNext"))
	{
		if(cdb_get_int("$turing_mpd_play",5) == 4)					//公众号请求
		{					
			tencent_request_next();
	    }
		if(cdb_get_int("$turing_mpd_play",5) == 0)					//tf播放
		{	
			if(cdb_get_int("$playmode",0) == 4)	
			{
				tf_next_event();
			}
			else if(cdb_get_int("$playmode",0) == 0 && cdb_get_int("$smartmode_state",0) == 0)
			{
				printf("请进入tf下播放\n");
			}
			else if(cdb_get_int("$smartmode_state",0) == 1)			//区分tf切换到智能模式下，仍然播放tf歌曲
			{ 	
				tf_next_event();
			}
	    }
		if(cdb_get_int("$turing_mpd_play",5) == 1)					//语音点歌和TTS播放
		{					
			if(cdb_get_int("$playmode",0) == 4)						//区分tf模式下语音点歌
			{					
				tf_next_event();
			}
			else if(cdb_get_int("$playmode",0) == 0 &&cdb_get_int("$smartmode_state",0) == 0)
			{
				printf("请进入tf下播放\n");
			}
			else if(cdb_get_int("$smartmode_state",0) == 1)			//区分tf切换到智能模式下，仍然播放tf歌曲
			{ 		
				tf_next_event();
			}
	    }
		playnext_ledstate_event(state);
	}
	else if(!strcmp(byReadBuf, "PlayerPlayPrev"))
	{
	
		if(cdb_get_int("$turing_mpd_play",5) == 4)			//公众号请求
		{ 	
			tencent_request_prev();
	  	}	

		if(cdb_get_int("$turing_mpd_play",5) == 0)			//tf模式播放
		{		
			if(cdb_get_int("$playmode",0) == 4)	
			{
				tf_prev_event();
			}
			else if(cdb_get_int("$playmode",0) == 0 &&cdb_get_int("$smartmode_state",0) == 0)
			{
				printf("请进入tf下播放\n");
			}
			else if(cdb_get_int("$smartmode_state",0) == 1)	//区分tf切换到智能模式下，仍然播放tf歌曲
			{ 	
				tf_prev_event();
			}
		}

		
		if(cdb_get_int("$turing_mpd_play",5) == 1)			//语音点歌和TTS播放
		{			
			if(cdb_get_int("$playmode",0) == 4)				//区分tf模式下语音点歌
			{			
				tf_prev_event();
			}
			else if(cdb_get_int("$playmode",0) == 0 &&cdb_get_int("$smartmode_state",0) == 0)
			{
				printf("请进入tf下播放\n");
			}
			else if(cdb_get_int("$smartmode_state",0) == 1)	//区分tf切换到智能模式下，仍然播放tf歌曲
			{ 	
				tf_prev_event();
			}
		}
			playprev_ledstate_event(state);
	}
   
}

#endif
//返回值：1：正在配网
int check_network_state()
{
    int airkiss_start = cdb_get_int("$airkiss_start",0);
    if(airkiss_start)
        return 1;
    else
        return 0;
}

/*wifi/蓝牙模式切换的灯效控制*/
void process_wifibluework_event()
{
/*	if (cdb_get_int("$wanif_state", 0) != 2) 
    {
        wav_preload("/tmp/res/speaker_not_connected.wav");
        return ;
    }
*/
	if(cdb_get_int("$playmode",5) == 1)
	{
		StopAllPlayer();
		StartPowerOn_led();
	}
	
	else if(cdb_get_int("$playmode",5) == 0)
	{
		if(recover_playing_status)
		{
			recover_playing_status = 0;
			StartTlkPlay_led();
			MpdPlay(-1);
			cdb_set_int("$smartmode_state",1);
		}else
		{
			StartPowerOn_led();
		}
		
	}
}

void process_network_event()
{
    if(IsMpg123Finshed())
        AddMpg123Url("/root/iot/setting_network.mp3");	
    else
    {
        set_intflag(1);
        AddMpg123Url("/root/iot/setting_network.mp3");//执行，正在配置网络,请稍等
    }

    if (vfork() == 0)
    {
        execl("/usr/bin/elian.sh", "elian.sh", "air_conf", NULL);
        exit(-1);
    }
   
   // start_record_sinvoice_data();
   // sinvoice_start_decode();
}


void process_wechat_event(char *byReadBuf)
{
    if(!strcmp(byReadBuf, "WeChatEnd"))
        wechat_finish();
    else if(!strcmp(byReadBuf, "WeChatStart"))
        wechat_start();
    else
        ;
}

void process_translation_event(char *byReadBuf)
{
    translation_start();
}


void process_sinvoice_event(char *byReadBuf)
{
    if(!strcmp(byReadBuf, "SinvoiceSuccess"))
        set_mic_record_status(0);
}

void TuringSetColor(char *cmd)
{
    char buf[4] = {0};
    int brightness = cdb_get_int("$sys_brightness",100);
    strcpy(buf,cmd+strlen("SetColor"));
    cdb_set("$sys_color",buf);
    Rgb_Set_Singer_Color(buf,brightness);
}

void TuringSetBrightness(char *cmd)
{
    char buf[4] = {0};
    strcpy(buf,cmd+strlen("SetBri"));
    int brightness = atoi(buf);
    
    cdb_get_str("$sys_color",buf,4,"WHI");
    cdb_set_int("$sys_brightness",brightness);
    
    Rgb_Set_Singer_Color(buf,brightness);
}

void TuringKeyControl(char *byReadBuf)
{

	info("Received message Key:%s",byReadBuf);
   if (0 == strncmp(byReadBuf, "PlayerPlay", sizeof("PlayerPlay") - 1)) 
   {
		process_player_state_event(byReadBuf);
   }
   else if (0 == strncmp(byReadBuf, "MicOn", sizeof("MicOn") - 1)) 
   {
	   process_mic_on_event();
   }
   else if (0 == strncmp(byReadBuf, "MicOff", sizeof("MicOff") - 1)) 
   {
	   process_mic_off_event();
   }
   else if(0 == strncmp(byReadBuf, "Battery", sizeof("Battery") - 1))
   {
	   process_battary_event(byReadBuf);
   }

   else if (0 == strncmp(byReadBuf, "StartSetNetwork", sizeof("StartSetNetwork") - 1)) 
   {
	   process_network_event();
   }
   else if (0 == strncmp(byReadBuf, "WifiBlueMode", sizeof("WifiBlueMode") - 1)) 
   {
	   process_wifibluework_event();
   }
   else if (0 == strncmp(byReadBuf, "WeChat", sizeof("WeChat") - 1)) 
   {
	   process_wechat_event(byReadBuf);
   }
   else if (0 == strncmp(byReadBuf, "CollectSong", sizeof("CollectSong") - 1)) 
   {
	   //收藏
	   TuringCollectSong();
   }
   else if (0 == strncmp(byReadBuf, "SwitchColumn", sizeof("SwitchColumn") - 1)) 
   {
	   //目录切换
	   TuringStartColumn();
   }
   else if (0 == strncmp(byReadBuf, "StartPlayTf", sizeof("StartPlayTf") - 1)) 
   {
	   StopAllPlayer();
	   IotDeviceStatusReport();
   }
   else if (0 == strncmp(byReadBuf, "Sinvoice", sizeof("Sinvoice") - 1)) 
   {
	   process_sinvoice_event(byReadBuf);
   }
   /******by add tian,2018/9/20******/
	else if (0 == strncmp(byReadBuf, "StartChinaEnglishTran", sizeof("StartChinaEnglishTran") - 1)) 
   {
	  process_translation_event(byReadBuf);//中/英翻译
   }

}

#define TEST
/*解析socket中接收到的命令,该函数由uartd通过消息队列发送数据过来，另外通过socket(tcp)也会发送数据过来*/
void TuringMonitorControl(char *byReadBuf)
{
	info("Received message:%s",byReadBuf);

#ifndef ENABLE_HUABEI
#if 0
    if (0 == strncmp(byReadBuf, "MicOn", sizeof("MicOn") - 1)) 
    {
        process_mic_on_event();
    }
    else if (0 == strncmp(byReadBuf, "MicOff", sizeof("MicOff") - 1)) 
    {
        process_mic_off_event();
    }
	else if(0 == strncmp(byReadBuf, "Battery", sizeof("Battery") - 1))
	{
		process_battary_event(byReadBuf);
	}
    else if (0 == strncmp(byReadBuf, "SystemVolume", sizeof("SystemVolume") - 1)) 
    {
        process_volume_event(byReadBuf);
    }
    else if (0 == strncmp(byReadBuf, "PlayerPlay", sizeof("PlayerPlay") - 1)) 
    {
        process_player_state_event(byReadBuf);
    }
    else if (0 == strncmp(byReadBuf, "StartSetNetwork", sizeof("StartSetNetwork") - 1)) 
    {
        process_network_event();
    }
	else if (0 == strncmp(byReadBuf, "WifiBlueMode", sizeof("WifiBlueMode") - 1)) 
    {
        process_wifibluework_event();
    }
    else if (0 == strncmp(byReadBuf, "WeChat", sizeof("WeChat") - 1)) 
    {
        process_wechat_event(byReadBuf);
    }
    else if (0 == strncmp(byReadBuf, "CollectSong", sizeof("CollectSong") - 1)) 
    {
        //收藏
        TuringCollectSong();
    }
    else if (0 == strncmp(byReadBuf, "SwitchColumn", sizeof("SwitchColumn") - 1)) 
    {
        //目录切换
        TuringStartColumn();
    }
    else if (0 == strncmp(byReadBuf, "StartPlayTf", sizeof("StartPlayTf") - 1)) 
    {
        StopAllPlayer();
		IotDeviceStatusReport();
    }
    else if (0 == strncmp(byReadBuf, "Sinvoice", sizeof("Sinvoice") - 1)) 
    {
        process_sinvoice_event(byReadBuf);
    }
	/******by add tian,2018/9/20******/
	 else if (0 == strncmp(byReadBuf, "StartChinaEnglishTran", sizeof("StartChinaEnglishTran") - 1)) 
    {
       process_translation_event(byReadBuf);//中/英翻译
    }
#endif

   if (0 == strncmp(byReadBuf, ALERT_DONE, sizeof(ALERT_DONE) - 1)) 
	{
		AlertFinshed();
	}
    else if (0 == strncmp(byReadBuf, "SystemVolume", sizeof("SystemVolume") - 1)) 
    {
	   process_volume_event(byReadBuf);
    }
	else if (0 == strncmp(byReadBuf, "list", sizeof("list") - 1)) 
	{
		TuringListEvent();
	}
	else if (0 == strncmp(byReadBuf, PLAY_MUSIC, sizeof(PLAY_MUSIC) - 1)) 
	{
		TuringPlayMusicEvent();
	}
	else if (0 == strncmp(byReadBuf, ACTIVE_INRET, sizeof(ACTIVE_INRET) - 1)) 
	{
		TuringActiveInterEvent();
	}
	else if (0 == strncmp(byReadBuf, START_UP_EVENT, sizeof(START_UP_EVENT) - 1)) 
	{
		ResumeMpdStateStartingUp();
	}
	else if (0 == strncmp(byReadBuf, CHARGE_EVENT, sizeof(CHARGE_EVENT) - 1)) 
	{
		TuringChargeEvent();	//充电状态改变事件
	}
	else if (0 == strncmp(byReadBuf, LOW_POWER_EVENT, sizeof(LOW_POWER_EVENT) - 1)) 
	{
		TuringLowPower();		//低电压电量
	}
	else if (0 == strncmp(byReadBuf, NORMAL_POWER_EVENT, sizeof(NORMAL_POWER_EVENT) - 1)) 
	{
		TuringNormalPower();	//正常电量
	}
	else if (0 == strncmp(byReadBuf, SHUT_DOWN_EVENT, sizeof(SHUT_DOWN_EVENT) - 1)) 
	{
		SaveMpdStateBeforeShutDown();
	}
	else if (0 == strncmp(byReadBuf, START_UP_EVENT, sizeof(START_UP_EVENT) - 1)) 
	{
		ResumeMpdStateStartingUp();
	}
#ifndef ENABLE_YIYA 
	if (0 == strncmp(byReadBuf, LOCK_EVENT, sizeof(LOCK_EVENT) - 1)) 
	{
		ChildLock();
	}
	else if (0 == strncmp(byReadBuf, LOCKED_EVENT, sizeof(LOCKED_EVENT) - 1)) 
	{
		ChildLocked();
	}
#endif
	else if (0 == strncmp(byReadBuf, UPDATE_DEVICE_EVENT, sizeof(UPDATE_DEVICE_EVENT) - 1)) 
	{
		TuringUpdateDeviceEvent();		//tf卡插拔事件
	}
	else if (0 == strncmp(byReadBuf, UPDATE_DEVICE_STATUS, sizeof(UPDATE_DEVICE_STATUS) - 1)) 
	{
		IotDeviceStatusReport();
	}
	else if (0 == strncmp(byReadBuf, SPEECH_START, sizeof(SPEECH_START) - 1)) 
	{		

		error("SPEECH_START");
		TuringSpeechStart(0);
			
	} 
	else if (0 == strncmp(byReadBuf, WAKEUP_TEST, sizeof(WAKEUP_TEST) - 1)) 
	{		
		error("SPEECH_START");
		TuringSpeechStart(0);
			
	}     
	else if (0 == strncmp(byReadBuf, VAD_OFF, sizeof(VAD_OFF) - 1)) 
	{		

		error("SPEECH_START");
		TuringSpeechStart(2);
			
	} 
	else if (0 == strncmp(byReadBuf, AI_START, sizeof(AI_START) - 1)) 
	{		

		error("ai start");
		TuringAiStart();
			
	} 
	else if (0 == strncmp(byReadBuf, CHAT_PLAY, sizeof(CHAT_PLAY) - 1)) 
	{
		TuringChatPlay();
	}
	else if (0 == strncmp(byReadBuf, SPEECH_END, sizeof(SPEECH_END) - 1)) 
	{	
		SetCaptureEnd(1);
	}
	else if (0 == strncmp(byReadBuf, MPC_VOLUME_EVENT, sizeof(MPC_VOLUME_EVENT) - 1)) 
	{
		IotDeviceStatusReport();
		info("MPC_VOLUME_EVENT after");
	}
	else if (0 == strncmp(byReadBuf, MPC_STOP_EVENT, sizeof(MPC_STOP_EVENT) - 1)) 
	{
		TuringStopEvent();
	}	
	else if (0 == strncmp(byReadBuf, MPC_NEXT_EVENT, sizeof(MPC_NEXT_EVENT) - 1)) 
	{	
		TuringMpcNextEvent();
	}
	else if (0 == strncmp(byReadBuf, MPC_PREV_EVENT, sizeof(MPC_PREV_EVENT) - 1)) 
	{	
		info("MPC_PREV_EVENT");
		cdb_set_int("$turing_stop_time",0);
		IotAudioStatusReport();
	}
	else if (0 == strncmp(byReadBuf, MPC_SONG_CHANGE, sizeof(MPC_SONG_CHANGE) - 1)) 
	{
		info("song change event");
		cdb_set_int("$turing_stop_time",0);
		//IotAudioStatusReport();
	}
	else if (0 == strncmp(byReadBuf, MPC_PLAY_EVENT, sizeof(MPC_PLAY_EVENT) - 1)) 
	{	
		info("MPC_PLAY_EVENT");
		TuringPlayEvent();
		warning("after TuringPlayEvent");
	}	
	else if (0 == strncmp(byReadBuf, MPC_TOGGLE_EVENT, sizeof(MPC_TOGGLE_EVENT) - 1)) 
	{	
		//info("MPC_PLAY_EVENT");
		//TuringPlayEvent();
		//warning("after TuringPlayEvent");
	}
	else if (0 == strncmp(byReadBuf, PM_STANDBY, sizeof(PM_STANDBY) - 1)) 
	{	
		TuringStandby();	//系统进入待机模式
	}
	else if (0 == strncmp(byReadBuf, PM_SUSPEND, sizeof(PM_SUSPEND) - 1)) 
	{	
		TuringSuspend();
	}
	else if (0 == strncmp(byReadBuf, PM_RESUME, sizeof(PM_RESUME) - 1)) 
	{	
		TuringResume();
	}
#ifdef      ENABLE_YIYA 
	else if (0 == strncmp(byReadBuf, PM_FIVE, sizeof(PM_FIVE) - 1)) 
	{	
		TuringFive();
	}
	else if (0 == strncmp(byReadBuf, PM_TEN, sizeof(PM_TEN) - 1)) 
	{	
		TuringTen();
	}
#endif
	else if (0 == strncmp(byReadBuf, CAPTURE_END, sizeof(CAPTURE_END) - 1)) 
	{	
		info("CAPTURE_END");
		TuringCaptureEnd();
	}
	else if (0 == strncmp(byReadBuf, WAV_PLAY_START, sizeof(WAV_PLAY_START) - 1)) 
	{	
		SetWavPlay(1);
	}
	else if (0 == strncmp(byReadBuf, WAV_PLAY_END, sizeof(WAV_PLAY_END) - 1)) 
	{	
		SetWavPlay(0);
	}
	else if (0 == strncmp(byReadBuf, PLAY_DONE, sizeof(PLAY_DONE) - 1)) 		
	{	
		//TuringPlayDoneEvent();
	} 
	else if (0 == strncmp(byReadBuf, CONFIG_CHANGE, sizeof(CONFIG_CHANGE) - 1)) 
	{	
		TuringConfigChange();
	}
	else if (0 == strncmp(byReadBuf, CONFIG_WRITE, sizeof(CONFIG_WRITE) - 1)) 
	{	
		TuringConfigWrite();
	}
#endif

#ifdef ENABLE_HUABEI
	if (0 == strncmp(byReadBuf, MPC_VOLUME_EVENT, sizeof(MPC_VOLUME_EVENT) - 1)) 
	{
		warning("MPC_VOLUME_EVENT");
	}
	else if (0 == strncmp(byReadBuf, MPC_STOP_EVENT, sizeof(MPC_STOP_EVENT) - 1)) 
	{
		warning("MPC_STOP_EVENT");
		int state = GetHuabeiSendStop();
		warning("HuabeiSendStop state:%d",state);		
	}	
	else if (0 == strncmp(byReadBuf, HUABEI_PLAY_STOP, sizeof(HUABEI_PLAY_STOP) - 1)) 
	{
		warning("HUABEI_PLAY_STOP");	
		int play = cdb_get_int("$turing_mpd_play", 0);
		if(play == 3)
			HuabeiPlayDoneEvent();		
	}
	else if (0 == strncmp(byReadBuf, MPC_PLAY_EVENT, sizeof(MPC_PLAY_EVENT) - 1)) 
	{	
		warning("MPC_PLAY_EVENT");
		int state = GetHuabeiSendStop();
		warning("HuabeiSendStop state:%d",state);
		if(state == 1) {
			HuabeiPlayPlayEvent();
		}
	}
	else if (0 == strncmp(byReadBuf, UPDATE_DEVICE_EVENT, sizeof(UPDATE_DEVICE_EVENT) - 1)) 
	{
		int tfStatus = cdb_get_int("$tf_status", 0);
		if (tfStatus == 0) {
			HuabeiPlaylistDeinit();
		}
	}
#endif

#ifdef TEST    
	else if (0 == strncmp(byReadBuf, "SetColor", sizeof("SetColor") - 1)) 
	{	
		TuringSetColor(byReadBuf);
	}
	else if (0 == strncmp(byReadBuf, "SetBri", sizeof("SetBri") - 1)) 
	{	
		TuringSetBrightness(byReadBuf);
	}
#endif
}

