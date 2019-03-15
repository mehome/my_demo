#include "processjson.h"

#include <json/json.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "capture.h"
#include "mon_config.h"

#include "myutils/debug.h"
#include "turing.h"
#include "SearchMusic.h"
//#include "mpgplayer.h"
#include "json.h"

#ifdef ENABLE_TTPOD
#include "DeviceStatus.h"
#include "TtpodMusic.h"
#endif
#ifdef ENABLE_MIGU
#include "MiguMusic.h"
#endif

#ifdef ENABLE_COMMSAT
#include "commsat.h"
#endif
#include "PlaylistStruct.h"
#include "init.h"
#include "TuringTTs.h"

#include "mpgplayer.h"

extern int frist_translate_flag;
int tfMode_IsDone_state = 0;
static int resumeMpd = 1;
int g_send_over = 0;
int recover_playing_status = 0;  //是否要回复播放  utf-8
int RecoverPlaying_status = 0;
static int FristInteract_flag = 0;
int Interact_flag = 0;
static int ErrorCount=0;
#define COUNT_INTERACT_MODE 5

static pthread_mutex_t InteractMtx = PTHREAD_MUTEX_INITIALIZER;

#ifdef TURN_ON_SLEEP_MODE
#define COUNT_SLEEP_MODE 4
static pthread_mutex_t interMtx = PTHREAD_MUTEX_INITIALIZER;  
static int errorCount = 0;
static int getToken = 0;
static int interState = INTER_STATE_NONE;

void SetErrorCount(int count)
{
	pthread_mutex_lock(&interMtx);
	errorCount = count;
	pthread_mutex_unlock(&interMtx);
}
int GetErrorCount()
{
	int count;
	pthread_mutex_lock(&interMtx);
	count = errorCount;
	pthread_mutex_unlock(&interMtx);
	return count;
}
void IncreaseErrorCount()
{
	pthread_mutex_lock(&interMtx);
	errorCount += 1;
	pthread_mutex_unlock(&interMtx);
}
/* 结束自动交互模式 */
int EnterSleepMode()
{
	int ret; 
	pthread_mutex_lock(&interMtx);
	ret = errorCount;
	errorCount = COUNT_SLEEP_MODE + 1;
	pthread_mutex_unlock(&interMtx);
	return ret;
}

/* 判断是否结束自动交互模式 */
int ShouldEnterSleepMode()
{
	int ret;
	int state ;
	IncreaseErrorCount();
	state = cdb_get_int("$turing_power_state", 0);
	pthread_mutex_lock(&interMtx);
	if(state == 0) {
		if(errorCount >= COUNT_SLEEP_MODE) {
			errorCount = 0;
			ret = 1;
		} else {
			ret = 0;
		}
	} else {
		errorCount = 0;
		ret = 1;
	}
	pthread_mutex_unlock(&interMtx);
	return ret;
}

int IsInterSuccess()
{
	int ret;
	pthread_mutex_lock(&interMtx);
	ret = (interState == INTER_STATE_SUCCESS);
	pthread_mutex_unlock(&interMtx);
	return ret;
}

void SetInterState(int state)
{
	pthread_mutex_lock(&interMtx);
	interState = state;
	pthread_mutex_unlock(&interMtx);
}
int IsInterFinshed()
{
	int ret;
	pthread_mutex_lock(&interMtx);
	ret = (interState == INTER_STATE_NONE);
	pthread_mutex_unlock(&interMtx);
	return ret;
}

int play_error_notice_speech()
{
    if(errorCount != COUNT_SLEEP_MODE )
    {
        if(IsInterrupt())
            SetErrorCount(COUNT_SLEEP_MODE);
        exec_cmd("aplay /root/iot/again.wav");
    }
    else
    {
        if(IsInterrupt())
            SetErrorCount(COUNT_SLEEP_MODE);
        exec_cmd("aplay /root/iot/怎么不理我了.wav");
    }

    return 1;
}


#endif

void set_interact_status(int flag)
{
	pthread_mutex_lock(&InteractMtx);
	Interact_flag = flag;
	pthread_mutex_unlock(&InteractMtx);
}

int get_interact_status(void)
{
	int ret;
	pthread_mutex_lock(&InteractMtx);
	ret = (Interact_flag == 1);
	pthread_mutex_unlock(&InteractMtx);
	return ret;
}


void AddErrorCount(void)
{
	pthread_mutex_lock(&InteractMtx);
	ErrorCount += 1;
	pthread_mutex_unlock(&InteractMtx);
}

void seterrorcount(int count)
{
	pthread_mutex_lock(&InteractMtx);
	ErrorCount = count;
	pthread_mutex_unlock(&InteractMtx);
}

int GetErrorCount(void)
{
	int ret;
	pthread_mutex_lock(&InteractMtx);
	ret = ErrorCount;
	pthread_mutex_unlock(&InteractMtx);
	return ret;
}


void EnterInteractionMode(void)
{
	set_interact_status(1);
	seterrorcount(0);
}

void EndInteractionMode(void)
{
	FristInteract_flag = 0;
	RecoverPlaying_status=0;
	set_interact_status(0);
	seterrorcount(0);
	stop_record_data();	
}

void InterruptInteractionMode(void)
{
	FristInteract_flag = 0;
	RecoverPlaying_status=0;
	set_interact_status(0);
	seterrorcount(0);
}

void SetResumeMpd(int state)
{
	resumeMpd = state;
}
int GetResumeMpd()
{
	return resumeMpd;
}


#define USE_MPG123 1

void mpg123_player(char *url)
{
#if 0
    char cmd_buf[256] = {0};
    sprintf(cmd_buf,"mpg123.sh %s",url);
    exec_cmd(cmd_buf);  //暂时使用mpg123进程来播放，稍后将作为一个线程来播放
#else
	
    AddMpg123Url(url);

#endif
}

void mpd_play_tts_url(char *url)
{
    //cdb_set_int("$turing_mpd_play", 1); //正在播放tts
    MpdClear();
    MpdAdd(url);
    MpdPlay(-1);
    MpdSingle(0);
    MpdRepeat(0);
}



//用于播放TTS合成语音用的
void text_2_sound(char *text)
{   
    char *purl = TuringGetTTsUrl(text);
    if(purl)
    {
        info("asr = [%s],text = [%s]",purl,text);
		int PlayValue = cdb_get_int("$turing_mpd_play",0);
        cdb_set_int("$turing_mpd_play", 1);
#if USE_MPG123         
        mpg123_player(purl);
#else
        mpd_play_tts_url(purl);
#endif
		if(PlayValue == 4)
			cdb_set_int("$turing_mpd_play", 4); //tts之前播公众号的歌曲
        free(purl);
    }
    else
    {
        info("Can not get url");
    }
}

//播放反馈回来的tts语音
void play_response_nlp(json_object *root)
{
    int i = 0;
    int dateLength = 0;
    json_object *nlp = NULL;
    json_object *val = NULL;
    char *pttsUrl = NULL;
	int PlayValue = -1;
    
    nlp = json_object_object_get(root, "nlp");
    if(nlp != NULL) 
    {
        dateLength =  json_object_array_length(nlp);
        for (i = 0; i < dateLength; i++)
        {
            val = json_object_array_get_idx(nlp, i);
            if (val != NULL)
            {
                pttsUrl = json_object_get_string(val);
            }
        }
    }
    if(pttsUrl)
    {
    	PlayValue = cdb_get_int("$turing_mpd_play",0);
        cdb_set_int("$turing_mpd_play", 1); //正在播放tts
#if USE_MPG123        
        mpg123_player(pttsUrl);
#else        
        mpd_play_tts_url(pttsUrl);
#endif
	if(PlayValue == 4)
		cdb_set_int("$turing_mpd_play", 4); //tts之前播公众号的歌曲

    }
            
}



//电量查询      utf-8
void electricity_quantity_inquiry(json_object *root)
{
    char ptts[64] = {0};
    int charging = GetDeviceCharging();
    int bat = GetDeviceBattery();

    if(charging){
        sprintf(ptts,"正在充电，已充电%d%%",bat);
    }
    else{
        if(bat > 75){
            sprintf(ptts,"当前未充电，剩余电量%d%%,尽情的用吧",bat);
        }
        else{
            sprintf(ptts,"当前未充电，剩余电量%d%%",bat);
        }
    }
    text_2_sound(ptts);
    if(GetResumeMpdState()){
        recover_playing_status = 1;   //本次交互前有在播放音频（除了TTS）
    } 
}

/*
{
"code":20001,
"asr":"关机。",
"tts":"再见。",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/audio/nlp-f250c4525e104fb69c0be31de60f996e-049f320914c14593bb77e2ab0cab0be0.mp3"],
"token":"1ce7b33a720f4a388d4543f1d3a09c6a",
"emotion":0
}
*/
void shutdown(json_object *root)
{
    //EnterSleepMode();
    play_response_nlp(root);
	usleep(2000*1000);
	while(!IsMpg123Finshed()){usleep(100*1000);}
    exec_cmd("uartdfifo.sh standby");  //通知MCU切断电源进行关机
}

/*
{
"code":20025,
"asr":"一分钟之后提醒我。",
"tts":"好的，我会在7月10日的10点45分提醒你的。",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/audio/nlp-f250c4525e104fb69c0be31de60f996e-1876c7ac4fb142cda2ab9928da73ee84.mp3"],
"token":"a0363a55487542e880ecdf716325dffe",
"func":
    {
        "cycleType":"0",
        "memoContent":"",
        "alarmType":"remind",
        "endDate":"2018-07-10",
        "timeLen":"60",
        "alarmTag":"alarmClock",
        "time":"10:45:11",
        "startDate":"2018-07-10"
    },
"emotion":0
}
*/
void setting_alarm_clock(json_object *root)
{
    struct tm tm_time;
    json_object *enddate = NULL,*endtime = NULL;
    json_object *func = NULL;
    char *endDate = NULL;
    char *time = NULL;
    char buf[24] = {0};
    char clockname[64] = {0};
    char pTime[12] = {0};

    play_response_nlp(root);
    
    func = json_object_object_get(root, "func");
    if(func)
    {
        enddate = json_object_object_get(func, "endDate");
        endtime = json_object_object_get(func, "time");
        if(enddate && endtime)
        {
            endDate = json_object_get_string(enddate);
            time = json_object_get_string(endtime);
            sprintf(buf,"%s %s",endDate,time);
			//
            strptime(buf, "%Y-%m-%d %H:%M:%S", &tm_time);//将buf的数据按照时间的格式输出，放入tm_time中
            sprintf(pTime,"%ld", mktime(&tm_time));
            
            sprintf(clockname,"turing_%s",pTime,pTime);

            //历史遗留问题，传进去的几个参数都需要动态分配，内部有释放动作
            AlertAdd(strdup(clockname), strdup(pTime),strdup("null"),0,strdup("null"));
        }
    }
    if(GetResumeMpdState())
    {
        recover_playing_status = 1;   //本次交互前有在播放音频（除了TTS）
    } 
}


/*
{
"code":20002,
"asr":"声音大一点。",
"tts":"我稍微大声点。",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/audio/nlp-f250c4525e104fb69c0be31de60f996e-2b938c16f61044de85cb77189d71375e.mp3"],
"token":"fba74b534f1143a388bbd6120c0917a4",
"func":
    {
        "operate":1,
        "arg":10
    },
"emotion":0}
*/


/*把声音调到最大
"func":
{
    "operate":1,
    "arg":10,
    "volumn":1
}
*/
void setting_system_volume(json_object *root)
{
    json_object *func = NULL;
    json_object *operate = NULL;
    json_object *arg = NULL;
    json_object *volumn = NULL;
    int iOperate = -1;
    int iArg = -1;
    int iVolumn = 0;
    play_response_nlp(root);
    func = json_object_object_get(root, "func");
    if(func){
        operate = json_object_object_get(func, "operate");
        if(operate)
            iOperate  = json_object_get_int(operate);
        
        arg = json_object_object_get(func, "arg");
        if(arg)
            iArg  = json_object_get_int(arg);

		volumn = json_object_object_get(func,"volumn");
		if(volumn)
			iVolumn = json_object_get_int(volumn);

		
        /* 音量控制 */
        int vol = cdb_get_int("$ra_vol",50);
        char buf[11] = {0};
        float tmp = 0 ;
        int volume = 0;
        info("volume = %d",vol);
        //amixer中的音量值非线性变化，120~255 对应 0~100
        if (1 == iOperate)
        {
            if(vol < 100)
            {
                vol += 10;
				if(vol > 100)
					vol = 100;
				
                tmp = vol*1.35;
                volume = (int)tmp + 120;
				
				MpdVolume(vol);
            }
			if(iVolumn == 1)
			{
				vol = 100;
				MpdVolume(vol);
			}
        }
        else if (0 == iOperate)
		{
            if(vol > 0)
            {
                vol -= 10;
                if(vol < 0)
                    vol = 0;
				
                 tmp = vol*1.35;
                 volume = (int)tmp + 120;
				 
				 MpdVolume(vol);  
            }
			if(iVolumn == -1)
			{
				vol = 0;
				MpdVolume(vol);
			}
        }
        
        
        cdb_set_int("$ra_vol",vol);
		snprintf(buf,sizeof(buf)-1,"setvol%03d",vol);
		
		printf("setvol to uartd is start:%s\n",buf);
		
//		uartd_fifo_write(buf);		//将音量发送给uartd
        if(GetResumeMpdState())
        {
            recover_playing_status = 1;
#ifdef TURN_ON_SLEEP_MODE
            EnterSleepMode();
#endif
        }
    }
}

/* 
{
"code":20007,
"asr":"来一首歌曲。",
"tts":"好的，我要开始唱亲亲猪猪宝贝这首歌啦",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/audio/nlp-f250c4525e104fb69c0be31de60f996e-3d817d47bc754437862d2089fd0ab336.mp3"],
"token":"489629b35a2943368ecbc2c260132700",
"func":
    {
        "duration":0,
        "operate":1000,
        "singer":"魏依曼",
        "originalTitle":"",
        "originalSinger":"",
        "isPlay":1,
        "tip":"http://turing-iot.oss-cn-beijing.aliyuncs.com/audio/nlp-f250c4525e104fb69c0be31de60f996e-4dadf137b6af455b8835d7c8e43fccb1.mp3",
        "id":85371,
        "title":"亲亲猪猪宝贝",
        "url":"http://appfile.tuling123.com/media/audio/20180524/35f55df30d144cb1a971ca6a4254a047.mp3"
    },
"emotion":0
}
////////////////////////////
{
"code":20007,
"asr":"暂停播放。",
"tts":"那我先不唱了",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-ece75b072cb74187b14c59e96aac51b6-2940212e999040eaaa7eff7ff78270ab.mp3"],
"token":"04fab0e9cd19459b8fa3c960b3596a1c",
"func":
    {
        "duration":0,
        "operate":1200,
        "singer":"",
        "originalTitle":"",
        "originalSinger":"",
        "isPlay":0,
        "id":0,
        "title":"",
        "url":""
    },
"emotion":0}
*/



/*{
"code":20007,
"asr":"继续播放。",
"tts":"好的，那我继续播放",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-ece75b072cb74187b14c59e96aac51b6-2940212e999040eaaa7eff7ff78270ab.mp3"],
"token":"04fab0e9cd19459b8fa3c960b3596a1c",
"func": {
             "duration": 0,
             "id": 57999,
             "isPlay": 1,
             "operate": 1300,
             "originalSinger": "儿歌",
             "originalTitle": "自食其果",
             "singer": "儿歌",
             "tip": "http://******.mp3",
             "title": "自食其果",
             "url": "http://******.mp3"
        }
   }
   
*/

void song_story_process(json_object *root)
{
   
   
    json_object *func = NULL;
    json_object *url_object = NULL,*isPlay_obj = NULL;
    char *purl = NULL;
    int i = 0,iOperate = 0;
    int dateLength = 0;
    json_object *nlp = NULL;
    json_object *val = NULL;
    json_object *asr = NULL;
	json_object *operate = NULL;
    char *pttsUrl = NULL;
	int  isPlay  = 0;
	int PlayValue = -1;
	
    nlp = json_object_object_get(root, "nlp");
    if(nlp != NULL) 
    {
        dateLength =  json_object_array_length(nlp);
        for (i = 0; i < dateLength; i++)
        {
            val = json_object_array_get_idx(nlp, i);
            if (val != NULL)
            {
                pttsUrl = json_object_get_string(val);
            }
        }
    }
    if(pttsUrl)
    {
    	PlayValue = cdb_get_int("$turing_mpd_play",0);
        cdb_set_int("$turing_mpd_play", 1); //正在播放tts
        cdb_set_int("$smartmode_state",0);
#if USE_MPG123       
        mpg123_player(pttsUrl);
#else
        MpdClear();
        //exec_cmd("mpg123.sh /root/iot/received_url.mp3");
        MpdAdd(pttsUrl);
#endif
    }
  
    func = json_object_object_get(root, "func");
    if(func) 
    {
        url_object = json_object_object_get(func, "url");
		if(url_object != NULL)
		{
			purl  = json_object_get_string(url_object);
		}
		operate = json_object_object_get(func, "operate");
		if(operate != NULL)
		{
			iOperate = json_object_get_int(operate);
		}

		isPlay_obj = json_object_object_get(func,"isPlay");
		if(isPlay_obj != NULL)
		{
			isPlay = json_object_get_int(isPlay_obj);
		}
		info("ioperate is value:%d",iOperate);
		info("isPlay is value :%d",isPlay);
		info("purl is value :%s",purl);
#if 1
	if((strlen(purl) > 10)&&(isPlay == 1))	//有歌曲链接
	{
		info("purl != NULL>>>>>>>>>>>");
		if(iOperate == 1300)			//继续播放		
		{
			usleep(1000*1000);
			while(!IsMpg123Finshed()){usleep(100*1000);}	
			MpdPlay(-1);
			tfMode_IsDone_state=1;
			StartTlkPlay_led();	
			if(PlayValue == 4)
				cdb_set_int("$turing_mpd_play", 4);	//tts之前播放的公众号歌曲
		}
		else if(iOperate == 3002)		//循环播放
		{
			usleep(500*1000);
			while(!IsMpg123Finshed()){usleep(100*1000);}
			MpdRepeat(1);
			MpdPlay(-1);
			tfMode_IsDone_state=1;
		}
		else
		{
			if(cdb_get_int("$playmode", 0) != 4)	
			{
				MpdClear();
		        MpdVolume(200);
		        MpdAdd(purl);
		        while(!IsMpg123Finshed()){usleep(100*1000);}
		        MpdPlay(0);
		        MpdRepeat(0);
		        MpdSingle(0);
				StartTlkPlay_led();
			}
			else
			{
				tfMode_IsDone_state=0;
			}
		}
	  
	}
	else		//无歌曲链接
	{
		if(isPlay == 0)
		{
			info("purl == NULL>>>>>>>>>>>");
			if(iOperate == 1200)		//暂停			
	        {
				if(recover_playing_status)	
				{
					while(!IsMpg123Finshed()){usleep(100*1000);}	
					MpdPause();
					tfMode_IsDone_state=1;
					StartPowerOn_led();
					if(PlayValue == 4)
						cdb_set_int("$turing_mpd_play", 4);	//tts之前播放的公众号歌曲
				}
			}
			else if(iOperate == 2002)	//停止		
			{
				cdb_set_int("$turing_mpd_play", 0);
				while(!IsMpg123Finshed()){usleep(100*1000);}	
				tfMode_IsDone_state=1;
				MpdClear();
				StartPowerOn_led();
			}
			
			else if(iOperate == 1000)	
			{
				if(recover_playing_status)
				{
					usleep(500*1000);
					while(!IsMpg123Finshed()){usleep(100*1000);}	
					tfMode_IsDone_state=1;
					 MpdPlay(-1);
					 if(PlayValue == 4)
						cdb_set_int("$turing_mpd_play", 4); //tts之前播放的公众号歌曲
				}
				else
				{
					StartPowerOn_led();
				}
			}
		 }	
	}
#endif
    }
}

/*
	
{
"code":20011,"asr":"学狗叫。",
"tts":"狗的叫声",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-732e7de6ab4943bd96f6387a82e86c0a.mp3"],
"token":"75b6851bf4e642ee989cda36811fb35c",
"func":
	{
		"animal":"狗",
		"url":"http://appfile.tuling123.com/media/audio/2018-10-16/de27ffec-d50f-414b-9a06-ac0e282881ad.mp3"
	},
	"emotion":0}
*/

void animal_cry_sound(json_object *root)
{

	
    json_object *func = NULL;
    json_object *url_object = NULL;
    char *purl = NULL;
    int i = 0,iOperate = 0;
    int dateLength = 0;
    json_object *nlp = NULL;
    json_object *val = NULL;
    json_object *asr = NULL;
	json_object *operate = NULL;
    char *pttsUrl = NULL;
	int PlayValue = -1; 
    nlp = json_object_object_get(root, "nlp");
    if(nlp != NULL) 
    {
        dateLength =  json_object_array_length(nlp);
        for (i = 0; i < dateLength; i++)
        {
            val = json_object_array_get_idx(nlp, i);
            if (val != NULL)
            {
                pttsUrl = json_object_get_string(val);
            }
        }
    }
    if(pttsUrl)
    {
    	PlayValue = cdb_get_int("$turing_mpd_play",0);
        cdb_set_int("$turing_mpd_play", 1); //正在播放tts
#if USE_MPG123       
        mpg123_player(pttsUrl);
#else
        MpdClear();
        MpdAdd(pttsUrl);
#endif
    }

	func = json_object_object_get(root,"func");
	if(func)
	{
		url_object = json_object_object_get(func,"url");
		if(url_object != NULL)
		{
			purl = json_object_get_string(url_object);
			info("purl animal_cry_process:%s",purl);
#if USE_MPG123       
        	mpg123_player(purl);
#else
        	MpdClear();
        	MpdAdd(purl);
#endif	

			if(PlayValue == 4)
				cdb_set_int("$turing_mpd_play", 4);	//tts之前播放的公众号歌曲
		}





		
	}
	
}

/*
{
"code":20014,
"asr":"谁在叫？",
"tts":"小朋友，欢迎你进入‘谁在叫’闯关游戏。一会我会发出一个声音，你来猜猜它是什么声音吧，如果不想玩了，可以跟我说退出或不想玩了。准备好哦，开始闯关喽！第1关：",
"nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-ae4f1c9b68c94544a965cb91984e0fde.mp3"],
"end":"http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-4042e099eda34bbb96e8786324948ce8.mp3",
"token":"75b6851bf4e642ee989cda36811fb35c",
"func":{
	"url":"http://download.tuling123.com/voice/125.mp3"
	},
	"emotion":0}
*/
void pass_game_process(json_object *root)
{

	
    json_object *func = NULL;
    json_object *url_object = NULL;
    char *purl = NULL;
    int i = 0,iOperate = 0;
    int dateLength = 0;
    json_object *nlp = NULL;
    json_object *val = NULL;
    json_object *asr = NULL;
	json_object *operate = NULL;
    char *pttsUrl = NULL;
	int PlayValue = -1;
    nlp = json_object_object_get(root, "nlp");
    if(nlp != NULL) 
    {
        dateLength =  json_object_array_length(nlp);
        for (i = 0; i < dateLength; i++)
        {
            val = json_object_array_get_idx(nlp, i);
            if (val != NULL)
            {
                pttsUrl = json_object_get_string(val);
            }
        }
    }
    if(pttsUrl)
    {
    	PlayValue = cdb_get_int("$turing_mpd_play",0);
        cdb_set_int("$turing_mpd_play", 1); //正在播放tts
#if USE_MPG123       
        mpg123_player(pttsUrl);
#else
        MpdClear();
        MpdAdd(pttsUrl);
#endif
    }

	func = json_object_object_get(root,"func");
	if(func)
	{
		url_object = json_object_object_get(func, "url");
		if(url_object != NULL)
		{
			purl  = json_object_get_string(url_object);
			
#if USE_MPG123       
					mpg123_player(purl);
#else
					MpdClear();
					MpdAdd(purl);
#endif

			if(PlayValue == 4)
				 cdb_set_int("$turing_mpd_play", 4); //tts之前播放的公众号歌曲
		}

		usleep(500*1000);
		while(!IsMpg123Finshed()){usleep(100*1000);}	
		StartPowerOn_led();






	}

	
}

/*
{
    "code":20027,
    "asr":"把灯变红。",
    "tts":"好的",
    "nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-ece75b072cb74187b14c59e96aac51b6-fb8ab855758348b5985020fdb9427411.mp3"],
    "token":"d98e58980f8a4db792a13552ef7e8775",
    "action":800,
    "emotion":800
}
*/


typedef struct 
{
    int action;
    char *color;
}action_color_t;

#define ACTION_BASE (800)
//好的[emotion:800;action:800]	把灯变红。|把灯调为红色。|把灯调为红色。|把灯调呈红色。|把灯调呈红色。|把灯调整红色。|灯变成红色。|灯调为红色。|灯调整呈红色。|调成红色。|帮我调到红灯。|灯光转为红灯。|变红灯。|把灯调橙红色。
//好的[emotion:801;action:801]	把灯变橘。|把灯调为橘色。|把灯调为橘色。|把灯调呈橘色。|把灯调呈橘色。|把灯调整橘色。|灯变成橘色。|灯调为橘色。|灯调整呈橘色。|调成橘色。|帮我调到橘灯。|灯光转为橘灯。|变橘灯。|把灯调橙橘色。
//好的[emotion:802;action:802]	把灯变黄。|把灯调为黄色。|把灯调为黄色。|把灯调呈黄色。|把灯调呈黄色。|把灯调整黄色。|灯变成黄色。|灯调为黄色。|灯调整呈黄色。|调成黄色。|帮我调到黄灯。|灯光转为黄灯。|变黄灯。|把灯调橙黄色。
//好的[emotion:803;action:803]	把灯变绿。|把灯调为绿色。|把灯调为绿色。|把灯调呈绿色。|把灯调呈绿色。|把灯调整绿色。|灯变成绿色。|灯调为绿色。|灯调整呈绿色。|调成绿色。|帮我调到绿灯。|灯光转为绿灯。|变绿灯。|把灯调橙绿色。
//好的[emotion:804;action:804]	把灯变蓝。|把灯调为蓝色。|把灯调为蓝色。|把灯调呈蓝色。|把灯调呈蓝色。|把灯调整蓝色。|灯变成蓝色。|灯调为蓝色。|灯调整呈蓝色。|调成蓝色。|帮我调到蓝灯。|灯光转为蓝灯。|变蓝灯。|把灯调橙蓝色。
//好的[emotion:805;action:805]	把灯变青。|把灯调为青色。|把灯调为青色。|把灯调呈青色。|把灯调呈青色。|把灯调整青色。|灯变成青色。|灯调为青色。|灯调整呈青色。调成青色。|帮我调到青灯。|灯光转为青灯。|变青灯。|把灯调橙青色。
//好的[emotion:806;action:806]	把灯变紫。|把灯调为紫色。|把灯调为紫色。|把灯调呈紫色。|把灯调呈紫色。|把灯调整紫色。|灯变成紫色。|灯调为紫色。|灯调整呈紫色。|调成紫色。|帮我调到紫灯。|灯光转为紫灯。|变紫灯。|把灯调橙紫色。
//好的[emotion:807;action:807]	把灯变白。|把灯调为白色。|把灯调为白色。|把灯调呈白色。|把灯调呈白色。|把灯调整白色。|灯变成白色。|灯调为白色。|灯调整呈白色。|调成白色。|帮我调到白灯。|灯光转为白灯。|变白灯。|把灯调橙白色。
#define ACTION_MAX  (819)

#define BRIGHTNESS_BASE (820)
//好的[emotion:821;action:821]	登亮一点。|等亮一点。|灯再亮一点。|灯在亮一点。|等在亮一点。|登在亮一点。
//好的[emotion:822;action:822]	把灯调成最亮。|把灯调呈最亮。|把灯挑到最亮。|把灯跳到最亮。|把灯调乘最亮。
//好的[emotion:823;action:823]	灯再暗一点。|登在暗一点。|灯在暗一点。|再暗一点。|灯暗一点。|登在案一点。|东安一点。
//好的[emotion:824;action:824]	把灯调到最暗。|把灯关闭。|把灯调呈最暗。|把等调呈最暗。|把登调呈最暗。|把灯调到最爱。|把灯调的最爱。|把灯挑到最爱。
//好的[emotion:825;action:825]	亮度调到百分之三十。|亮度设置为百分之三十。|把灯调到百分之三十。|亮度是百分之三十。|把灯挑到百分之三十。
//好的[emotion:826;action:826]	亮度调到百分之五十。|亮度设置为百分之五十。|把灯调到百分之五十。|亮度是百分之五十。|把灯挑到百分之五十。
//好的[emotion:827;action:827]	亮度调到百分之七十。|亮度设置为百分之七十。|把灯调到百分之七十。|亮度是百分之七十。|把灯挑到百分之七十。
//好的[emotion:828;action:828]	亮度调到百分之九十。|亮度设置为百分之九十。|把灯调到百分之九十。|亮度是百分之九十。|把灯挑到百分之九十。
#define BRIGHTNESS_MAX (829)


#define PLAYER_CONTROL_BASE (830)
//暂停播放	好的[emotion:830;action:830]	暂停播放。|站停播放。|展厅播放。|站厅播放。
//继续播放	好的[emotion:831;action:831]	继续播放。|急需播放。|积蓄播放。|记叙播放。|几许播放。
//停止播放	好的[emotion:832;action:832]	停止播放。|停滞播放。|挺直播放。|停职播放。|挺值播放。
#define PLAYER_CONTROL_MAX (835)




#define OTHER_BASE (836)
//路由器信息	好的[emotion:833;action:833]	IP是多少。|IP地址是多少。|路由器名称。|联网信息。|你的IP是多少。|	

#define OTHER_MAX (840)






action_color_t g_color[]={
{ACTION_BASE + 0,"RED"},
{ACTION_BASE + 1,"ORA"},
{ACTION_BASE + 2,"YEL"},
{ACTION_BASE + 3,"GRE"},
{ACTION_BASE + 4,"BLU"},
{ACTION_BASE + 5,"CYA"},
{ACTION_BASE + 6,"PUR"},
{ACTION_BASE + 7,"WHI"},
};


int adjust_bulb_color(json_object *root)
{
    int i = 0;
    int dateLength = 0;
    json_object *nlp = NULL;
    json_object *val = NULL;
    json_object *action = NULL;
	json_object *emotion = NULL;
    char *pttsUrl = NULL;
	int iemotion = 0;
	int song_flag = 0;
    nlp = json_object_object_get(root, "nlp");
    if(nlp != NULL) 
    {
        dateLength =  json_object_array_length(nlp);
        for (i = 0; i < dateLength; i++)
        {
            val = json_object_array_get_idx(nlp, i);
            if (val != NULL)
            {
                pttsUrl = json_object_get_string(val);
            }
        }
    }
    if(pttsUrl)
    {
     //   cdb_set_int("$turing_mpd_play", 1); //正在播放tts
#if USE_MPG123 
        mpg123_player(pttsUrl);
#else
        mpd_play_tts_url(pttsUrl);
#endif
    }
#if defined(CONFIG_PROJECT_BEDLAMP_V1)
    action = json_object_object_get(root, "action");
    if(action)
    {
        int val = json_object_get_int(action);
        int brightness = cdb_get_int("$sys_brightness",100);
        char *color = "WHI";
        char buf[4] = {0};
        
        if(val>BRIGHTNESS_BASE)		//调灯的亮度
        {
            if(val == 821)		//灯亮一点
                brightness +=10;
            else if(val == 822)	//灯最亮
                brightness =100;        
            else if(val == 823)	//灯暗一点
                brightness -=10;        
            else if(val == 824)	//灯最暗
                brightness =0;        
            else if(val == 825)	//灯调到30%
                brightness =20;        
            else if(val == 826)
                brightness =50;        
            else if(val == 827)
                brightness =70;
            cdb_get_str("$sys_color",buf,4,"WHI");
            color = buf;
        }
        else if(val >=ACTION_BASE && val<=ACTION_BASE+8)	//800-808，调灯的颜色
        {
            if(val == 808)
            {
                int index = (time(NULL)%8);
                color = g_color[index].color;
            }
            else
            {
                color = g_color[val - ACTION_BASE].color;
            }
            
        }
        if(brightness > 100)
        {
            brightness = 100;
        }
        else if(brightness < 0)
            brightness = 0;
        
        cdb_set("$sys_color",color);				//将颜色设置到cdb中
        cdb_set_int("$sys_brightness",brightness);	//将亮度设置到cdb中
        Rgb_Set_Singer_Color(color,brightness);		//设置颜色和亮度到设备
		return 0;
    }
#elif defined(CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_DOSS_1MIC_V1)
	emotion = json_object_object_get(root, "emotion");
	if(emotion)
	{
		iemotion = json_object_get_int(emotion);
		info("iemotion is value :%d",iemotion);
		if(iemotion == 830)						//暂停播放
		{
			if(recover_playing_status)	
			{
				while(!IsMpg123Finshed()){usleep(100*1000);}	
				MpdPause();
				tfMode_IsDone_state=1;
				StartPowerOn_led();
			}
		}
		else if(iemotion == 831)				//继续播放
		{
			while(!IsMpg123Finshed()){usleep(100*1000);}	
			usleep(1000*1000);
			MpdPlay(-1);
			tfMode_IsDone_state=1;
			StartTlkPlay_led();	
		}
		else if(iemotion == 832)				//停止播放
		{
			cdb_set_int("$turing_mpd_play", 0);		
			while(!IsMpg123Finshed()){usleep(100*1000);}	
			usleep(1000*1000);
			MpdClear();
			tfMode_IsDone_state=1;
			StartPowerOn_led();
		}
	}
	return !song_flag;
#endif
    
}


int customized_color(int action_val)
{
    char *color = "WHI";
    int brightness = cdb_get_int("$sys_brightness",100);
    
    color = g_color[action_val - ACTION_BASE].color;
    cdb_set("$sys_color",color);

    Rgb_Set_Singer_Color(color,brightness);
	return 0;
}

int customized_brightness(int action_val)
{
    int brightness = cdb_get_int("$sys_brightness",100);
    
    if(action_val == 821)
        brightness +=10;
    else if(action_val == 822)
        brightness =100;        
    else if(action_val == 823)
        brightness -=10;        
    else if(action_val == 824)
        brightness =0;        
    else if(action_val == 825)
        brightness =30;        
    else if(action_val == 826)
        brightness =50;        
    else if(action_val == 827)
        brightness =70;
    else if(action_val == 828)
        brightness =90;

    if(brightness > 100)
    {
        brightness = 100;
    }
    else if(brightness < 0)
        brightness = 0;
    
    
	char color[4] = {0};
    cdb_get_str("$sys_color",color,4,"WHI");
    cdb_set_int("$sys_brightness",brightness);
    Rgb_Set_Singer_Color(color,brightness);
	return 0;

}

//{"code":20027,"asr":"暂停播放。","tts":"好的","nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-23b8fc55308246d988280cc0fcf62151.mp3"],"token":"a87b94f6211c4070a8abd4fd1488d0a8","action":830,"emotion":830}
//{"code":20027,"asr":"继续播放。","tts":"好的","nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-23b8fc55308246d988280cc0fcf62151.mp3"],"token":"a87b94f6211c4070a8abd4fd1488d0a8","action":831,"emotion":831}
//{"code":20027,"asr":"停止播放。","tts":"好的","nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-23b8fc55308246d988280cc0fcf62151.mp3"],"token":"a87b94f6211c4070a8abd4fd1488d0a8","action":832,"emotion":832}

int  customized_player_control(int action_val)
{
    switch(action_val)
    {
        case 830: //暂停播放
        {
            if(recover_playing_status == 1)
            {
                usleep(500*1000);
                while(!IsMpg123Finshed()){usleep(100*1000);}
                MpdPause();
				tfMode_IsDone_state=1;
                recover_playing_status = 0;
				StartPowerOn_led();
            }
        }
        break;
        case 831://继续播放
        {
            if(MpdGetPlayState() == 3)
            {
                usleep(500*1000);
                while(!IsMpg123Finshed()){usleep(100*1000);}
                MpdPlay(-1);
				tfMode_IsDone_state=1;
				StartTlkPlay_led();	
            }
        }
        break;
        case 832://停止播放
        {
            if(recover_playing_status == 1)
            {
                usleep(500*1000);
                while(!IsMpg123Finshed()){usleep(100*1000);}
                MpdStop();
				tfMode_IsDone_state=1;
                recover_playing_status = 0;				
				cdb_set_int("$turing_mpd_play", 0); 	
				StartPowerOn_led();
            }
        }
        break;
    }

	return 1;
}


int  customized_get_info(int action_val )
{
    switch(action_val)
    {
        case 836:
        {
            char text[128] = {0};
            char ssid[32] = {0};
            char ip[32] = {0};
            char ap[32] = {0};
            cdb_get_str("$wl_bss_ssid2",ssid,32,"未找到");
            cdb_get_str("$wanif_ip",ip,32,"0.0.0.0");
            cdb_get_str("$ap_ssid1",ap,32,"未找到");
            sprintf(text,"您连接的路由器名称是：%s,设备IP是:%s，设备热点是%s",ssid,ip,ap);
            text_2_sound(text);
        }
        break;
    }

	return 0;
}


int  customized_functions_process(json_object *root)
{
    json_object *action = NULL;
    json_object *app_name = NULL;
    int action_val = 0;
    int ret = 0;
    play_response_nlp(root);
    
    action = json_object_object_get(root, "action");
    if(action)
    {
        action_val = json_object_get_int(action);
        if(action_val >= ACTION_BASE && action_val <= ACTION_MAX)
        {
            ret = customized_color(action_val);
        }
        else if(action_val >= BRIGHTNESS_BASE && action_val <= BRIGHTNESS_MAX)
        {
            ret = customized_brightness(action_val);
        }
        else if(action_val >= PLAYER_CONTROL_BASE && action_val <= PLAYER_CONTROL_MAX)
        {
            ret = customized_player_control(action_val);
        }
        else if(action_val >= OTHER_BASE && action_val <= OTHER_MAX)
        {
            ret = customized_get_info(action_val);
        }
        else
        {
            printf("To be continue..............\n");
        }
    }

	return ret;
}







/*
{
    "code":20021,
    "asr":"把灯调亮一点。",
    "tts":"好的",
    "nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-ece75b072cb74187b14c59e96aac51b6-fb8ab855758348b5985020fdb9427411.mp3"],
    "token":"fbcfd07cbb194fcf859f74c72a05ed96",
    "func":
        {
            "operate":1,
            "arg":10
        },
    "emotion":0
}

{
    "code":20021,
    "asr":"灯调暗一点。",
    "tts":"好的",
    "nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-ece75b072cb74187b14c59e96aac51b6-fb8ab855758348b5985020fdb9427411.mp3"],
    "token":"fbcfd07cbb194fcf859f74c72a05ed96",
    "func":
        {
            "operate":0,
            "arg":10
        },
    "emotion":0
}

*/

void adjust_bulb_brightness(json_object *root)
{
    int i = 0;
    int dateLength = 0;
    json_object *nlp = NULL;
    json_object *val = NULL;
    json_object *func = NULL;
    json_object *operate = NULL;
    char *pttsUrl = NULL;

    nlp = json_object_object_get(root, "nlp");
    if(nlp != NULL) 
    {
        dateLength =  json_object_array_length(nlp);
        for (i = 0; i < dateLength; i++)
        {
            val = json_object_array_get_idx(nlp, i);
            if (val != NULL)
            {
                pttsUrl = json_object_get_string(val);
            }
        }
    }
    if(pttsUrl)
    {
    	int PlayValue = cdb_get_int("$turing_mpd_play",0);
        cdb_set_int("$turing_mpd_play", 1); //正在播放tts
#if USE_MPG123 
        mpg123_player(pttsUrl);
#else 
        mpd_play_tts_url(pttsUrl);
#endif
		if(PlayValue == 4)
			cdb_set_int("$turing_mpd_play", 4); //tts之前播公众号的歌曲
    }

    func = json_object_object_get(root, "func");
    if(func)
    {
        operate = json_object_object_get(func, "operate");
        if(operate)
        {
            int val = json_object_get_int(operate);
            int brightness = cdb_get_int("$sys_brightness",100);
            if(val)
            {
                brightness += 10;
                if(brightness > 100)
                {
                    brightness = 100;
                }
            }
            else
            {
                brightness -= 10;
                if(brightness < 0)
                {
                    brightness = 0;
                }
            }
            char buf[4] = {0};
            cdb_get_str("$sys_color",buf,4,"WHI");
            cdb_set_int("$sys_brightness",brightness);
            Rgb_Set_Singer_Color(buf,brightness);
        }
        
    }
}

/*
{
    "code":20026,
    "asr":"打开灯。",
    "tts":"正在打开灯，请稍后。",
    "nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-95576364de254ce38aacfc5253def041.mp3"],
    "token":"b541e65bd04d44e2acdbc01068633466",
    "func":
        {
            "app_name":"灯",
            "app_package":""
        },
    "emotion":0}
*/
void open_app(json_object *root)
{
    int i = 0;
    int dateLength = 0;
    json_object *nlp = NULL;
    json_object *val = NULL;
    json_object *func = NULL;
    json_object *app_name = NULL;
    char *str = NULL;
    char *pttsUrl = NULL;

    nlp = json_object_object_get(root, "nlp");
    if(nlp != NULL) 
    {
        dateLength =  json_object_array_length(nlp);
        for (i = 0; i < dateLength; i++)
        {
            val = json_object_array_get_idx(nlp, i);
            if (val != NULL)
            {
                pttsUrl = json_object_get_string(val);
            }
        }
    }
    if(pttsUrl)
    {
        cdb_set_int("$turing_mpd_play", 1); //正在播放tts
#if USE_MPG123 
        mpg123_player(pttsUrl);
#else        
        mpd_play_tts_url(pttsUrl);
#endif
    }
    
    func = json_object_object_get(root, "func");
    if(func)
    {
        app_name = json_object_object_get(func, "app_name");
        if(app_name)
        {
            str = json_object_get_string(app_name);
            if(str)
            {
                if(!strcmp(str,"灯"))
                {
                    char buf[4] = {0};
                    int brightness = cdb_get_int("$sys_brightness",100);
                    if(brightness == 0)
                    {
                        brightness = 70;
                        cdb_set_int("$sys_brightness",brightness);
                    }
                    cdb_get_str("$sys_color",buf,4,"WHI");
                    Rgb_Set_Singer_Color(buf,brightness);
                }
            }
        }
    }
}


int network_timeout()
{
    int ret = 0;
    int count = 0;
    
    if(get_mic_record_status())
        set_mic_record_status(0);
    if(GetHttpRequestWait())
        SetHttpRequestCancle();
	if(translation_flag)
		translation_flag=0;
    while(get_mic_record_status())
    {
        info("------------------");
        usleep(100*1000);
    }   //停止需要时间，这里等待一下

    count = 0;
    while(GetHttpRequestWait())
    {
        info("+++++++++++++++++");
        usleep(100*1000);
        count++;
        if(count == 50)
        {
            exec_cmd("aplay -D common /root/iot/network_too_slow.wav");
        }
    }
#if 0    
    exec_cmd("aplay -D common /root/iot/error_occurred.wav");
#else
   // mpg123_player("/root/iot/error_occurred.mp3");
//    exec_cmd("aplay -D common /root/iot/error_occurred.wav");
#ifdef ENABLE_CONTINUOUS_INTERACT
	AddErrorCount();
	Start_record_data();
	if(GetErrorCount() != 5){
		return ;
	}
#endif
	//	info("recover_playing_status=%d,RecoverPlaying_status=%d",recover_playing_status,RecoverPlaying_status);
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
#endif
	printf("出现错误次数:total=%d,连续交互结束222\n",GetErrorCount());
    return ret;
}
/*
{
    "code":20016,
    "asr":"换一种颜色。",
    "tts":"好的，我要开始啦！",
    "nlp":["http://turing-iot.oss-cn-beijing.aliyuncs.com/tts/nlp-8e3941df07b14bc183766ebd51a3b8b0-1b1f6bf1c3ed40948c9a23514ed97ffd.mp3"],
    "token":"504206ccc8114644a122b7324f23874b",
    "emotion":0
}

*/

void change_current_color(json_object *root)
{
    play_response_nlp(root);
    int brightness = cdb_get_int("$sys_brightness",100);
    char *color = "WHI";
    char buf[4] = {0};

    cdb_get_str("$sys_color",buf,4,"WHI");

    int index = (time(NULL)%8);
    color = g_color[index].color;	//随机获取一种颜色
    if(!strcmp(color,buf))
    {
        color = g_color[(index+1)%8].color;
    }
    if(brightness > 100)
    {
        brightness = 100;
    }
    else if(brightness < 0)
        brightness = 0;
    
    cdb_set("$sys_color",color);
    Rgb_Set_Singer_Color(color,brightness);
}



int file_exists(const char *path)    // note: anything but a directory
{
    struct stat st;
    
    memset(&st, 0 , sizeof(st));
    return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode)) && (st.st_size > 0);
}


int get_line_count(char *filename)
{

 int ch=0;
 int n=0;
 FILE *fp;
 fp=fopen(filename,"r");
 if(fp==NULL)
      return -1;
 while((ch = fgetc(fp)) != EOF) 
         {  
             if(ch == '\n')  
             {  
                 n++;  
             } 
         }  

 fclose(fp); 
 return n; 

}
int compare_FileAndUrl(char *filepatch,char *Url)
{

   if((NULL!=Url)&&(NULL!=filepatch)){

      FILE *dp =NULL;
	  char *deline;
      char buf[512];
	  int ret=0;
	  int i;
	  int hangnum=get_line_count(filepatch);

   	   dp= fopen(filepatch, "r+");
	    if(NULL==dp){
		   DEBUG_INFO("file no exist\n");
           return -1;
		}
            
	         for(i=0;i<hangnum;i++)
	         {
			 deline=fgets(buf,512, dp);
			 if(deline==NULL)
			 	{
			 	    fclose(dp);
			 	 	return 0;
			 	}
			 ret=strncmp(deline,Url,strlen(Url)-1);
			 if(0==ret){
			 	i++;
				fclose(dp);
			 	return i;
			 	}	      	 
			 }
			 
			 
			 fclose(dp);
			 dp= NULL;
		  
		return 0;
 }
}



int continue_to_play_tf()
{
    char sd_last_contentURL[256] = {0};
    int result = 0;
    int playmode = cdb_get_int("$playmode", 0);
    if(playmode == 4)
    {
#if 0
        cdb_get_str("$sd_last_contentURL",sd_last_contentURL,sizeof(sd_last_contentURL),"");
        if(strlen(sd_last_contentURL) > 4)
      // if(sd_last_contentURL != NULL)
        {
            if(file_exists("/usr/script/playlists/tfPlaylist.m3u"))
            {
                result=compare_FileAndUrl("/usr/script/playlists/tfPlaylist.m3u",sd_last_contentURL);
                if(result > 0)
                {
                    char tempbuff[72];
                    memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
                    sprintf(tempbuff, "playlist.sh swtich_to_tf %d", result);
                    exec_cmd(tempbuff);
                }
            }
        }
#endif
    }
    
}
/*
 *tf模式播放时，将语音点播的歌曲加载到TURING_PLAYLIST中，有个弊端，
 *当歌曲加载很多的时候，语音点歌后,搜索TURING_PLAYLIST的时间加长，导致播放歌曲延时 
 */
void song_story_addTuringJson(json_object *root)
{
	json_object	*id = NULL,*name = NULL,*pTip = NULL,*func = NULL,*url = NULL;
	char *urlStr  = NULL;
	char *nameStr = NULL;
	char *tipStr  = NULL; 
	int intId = 0;
	
	int playmode = cdb_get_int("$playmode", 0);			//tf卡模式
	if((playmode == 4) && (tfMode_IsDone_state == 0))	//tfMode_IsDone_state:tf卡播放下。进行语音交互，播放/暂停/停止时，不需要执行该函数的标志位
	{
		func = json_object_object_get(root, "func");
		if(func)
		{
		
			url = json_object_object_get(func, "url");	
			if(url == NULL)
				return -1;
			urlStr = json_object_get_string(url);
			info("urlStr:%s",urlStr);
			
			name = json_object_object_get(func, "title");
			if(name) {
			nameStr = json_object_get_string(name);
			}
					
			id = json_object_object_get(func, "id");
			if(id) {
				intId = json_object_get_int(id);
			}
			
			pTip = json_object_object_get(func, "tip");
			if(pTip) {
			tipStr = json_object_get_string(pTip);
			}
		
			
	 	if(strlen(urlStr) > 10)		//有歌曲链接
	    {
	        long endTime ;
	        long startTime = getCurrentTime();
	     
	        //tts语音播放结束，判断有没有歌曲链接信息,有则加入到mpd中
			if(urlStr && strlen(urlStr) > 0) 
			{
				if(nameStr) 	//歌曲名
				{
#ifdef  USE_MPDCLI
					MpdAdd(urlStr);		//添加歌曲链接
#else
					eval("mpc","add", urlStr);
#endif
					
				}
	            cdb_set_int("$turing_mpd_play", 0);  //歌曲等音频链接就设置为0
			}
	       
	     // if(type) 
	        {
	        	MusicItem *item = NewMusicItem();
	        	if(item) 
	        	{
	        		info("item != NULL");
	        		char mediaId[128] = {0};
	        		snprintf(mediaId, 128, "%d", id);	
	        		info("mediaId:%s",mediaId);
	        		if(urlStr && strlen(urlStr) > 0)
	        			item->pContentURL = strdup(urlStr);
	        		info("item->pContentURL:%s",item->pContentURL);//歌曲链接
	        		if(nameStr && strlen(nameStr) > 0) {
	        			item->pTitle = strdup(nameStr);
	        			info("item->pTitle:%s",item->pTitle);	//歌曲名
	        		}
	        	/*	if(pSinger && strlen(pSinger) > 0 ) {
	        			item->pArtist = strdup(pSinger);
	        			info("item->pArtist:%s",item->pArtist);	//作者
	        		}*/
	        		if(tipStr && strlen(tipStr) > 0 )
	        			item->pTip = strdup(tipStr);		//歌曲名链接
	        		item->pId = strdup(mediaId);			//id号
	        		item->type = MUSIC_TYPE_SERVER;			//type类型
	        		TuringPlaylistInsert(TURING_PLAYLIST, item, 1);
					endTime = getCurrentTime();
					info("endTime - startTime :%ld ms",(endTime-startTime));
	        		FreeMusicItem(item);
	        	}
	        }
		}
			}
	}
}







/* BEGIN: Added by Frog, 2018/06/01 */
/*  关于TTS播放和歌曲的播放逻辑现在是这样的： 
 *  TTS和歌曲链接都使用mpd来播放，弊端就是：
 *  1.两种类型的音频链接都在播放列表中，难以维护
 *  2.恢复此前的状态比较麻烦，要记录下先前的 链接、播放进度、音量等
 *  其实，完全可以使用mpg123来播放TTS，歌曲什么的用mpd来播放，这样
 *  mpd被打断（调节音量、询问电量、百科知识）时，直接用mpc pause即可保护/恢复现场
 *  mpd被语音点播的新的歌曲打断，直接将新歌曲链接加入到列表头部并播放第一首即可
 */

/* AIWIFI返回数据处理函数 */
#if 0		/***********************单次交互对话************************/
void ProcessturingJson(char *pData)
{
    json_object *root = NULL, *code = NULL, *nlp = NULL, *func = NULL, *url = NULL, *val = NULL, *fileUrl = NULL, *ttsUrlAfter = NULL, *token = NULL, *control_flag = NULL;
    json_object *asr = NULL, *tts = NULL;
    int err = 0;
    char *pttsUrl = NULL;
    char *pfileUrl = NULL,*pfileTitle= NULL;
    char *pttsUrlAfter = NULL;
    char *pOriginalTitle = NULL;
    char *pOriginalSinger = NULL;
    char *pSinger = NULL;
    int i = 0;
    int dateLength = 0;
    int iCode;
    int type = 0;
    char *pAsr = NULL;
	int song_flag = 0;	

    info("pData:%s",pData);
    SetResumeMpd(1);	
    root = json_tokener_parse(pData);
    if (is_error(root)) 
    {
#ifdef TURN_ON_SLEEP_MODE
        SetInterState(INTER_STATE_ERROR);
#endif
        error("json_tokener_parse root failed");
        return ;
    }
    code = json_object_object_get(root, "code");
    if(code != NULL) 
    {     
        iCode = json_object_get_int(code); 
        //code 定义在 http://docs.turingos.cn/ai-wifi/ai-wifi-api/#_9
        switch(iCode)
        {
            /* 正在识别 */
            case 40000:json_object_put(root);
			StartPowerOn_led();
			return ;
            case 40001 ... 49999: //error
            err = 0;
#ifdef TURN_ON_SLEEP_MODE
            err = play_error_notice_speech();
            goto end;
#else                
            play_response_nlp(root); break;
#endif
            break;
            
            case 20001: shutdown(root);                     break;
            case 20002: setting_system_volume(root);        break;
            case 20000: 
            case 20003:
            case 20005:
            case 20006:
            case 20009:
            case 20012:
            case 20015: play_response_nlp(root); 			break;
            case 20016: change_current_color(root);         break;//随意切换颜色
            case 20018:
            case 20019:
            case 20020: play_response_nlp(root);            break;
            case 20021: adjust_bulb_brightness(root);       break; //灯亮度调节
            case 20007: 
            case 20008: song_story_process(root); song_flag = 1; break;
            case 20011: animal_cry_sound(root);	  tfMode_IsDone_state = 1;break;	//动物叫声
            case 20014: pass_game_process(root);  tfMode_IsDone_state = 1;song_flag = 1;break; //闯关游戏
            case 20022: electricity_quantity_inquiry(root); break;//电量查询
            case 20025: setting_alarm_clock(root);          break;//设置闹钟
            case 20028: play_response_nlp(root);break;			  //中英翻译
            case 20013: //not support
            case 20023: //not support
            case 20024: //not support
            case 20026: open_app(root);                     break;
            case 20027: song_flag = adjust_bulb_color(root);break;//灯颜色调节
           // case 20027: //not support
            case 29998: //not support
            case 29999: //not support
#ifdef TURN_ON_SLEEP_MODE
                SetInterState(INTER_STATE_SUCCESS);
                SetErrorCount(0);
#endif
                play_response_nlp(root);break;
                break;
            default:
                break ;
        }
    }

#if 0    
	ttsUrlAfter = json_object_object_get(root, "end");
	if(ttsUrlAfter) 			
		pttsUrlAfter  = json_object_get_string(ttsUrlAfter);
		info("pttsUrlAfter is :%s,pfileUrl is :%s",pttsUrlAfter,pfileUrl);
    if (pfileUrl != NULL ||  pttsUrlAfter != NULL)
    {
        long endTime ;
        long startTime = getCurrentTime();
     
        //tts语音播放结束，判断有没有歌曲链接信息,有则加入到mpd中
		if(pfileUrl && strlen(pfileUrl) > 0) 
		{
			if(pfileTitle) 	//歌曲名
			{
#ifdef USE_MPDCLI
				MpdAdd(pfileUrl);		//添加歌曲链接
#else
				eval("mpc","add", pfileUrl);
#endif
				
			}
            cdb_set_int("$turing_mpd_play", 0);  //歌曲等音频链接就设置为0
		}
		if(pttsUrlAfter) 		//添加后续的歌曲链接
        {
#ifdef USE_MPDCLI
            MpdAdd(pttsUrlAfter);
#else
            eval("mpc","add", pttsUrlAfter);
#endif
        }

        endTime = getCurrentTime();
        if(type) 
        {
        	MusicItem *item = NewMusicItem();
        	if(item) 
        	{
        		info("item != NULL");
        		char mediaId[128] = {0};
        		snprintf(mediaId, 128, "%d", id);	
        		info("mediaId:%s",mediaId);
        		if(pfileUrl && strlen(pfileUrl) > 0)
        			item->pContentURL = strdup(pfileUrl);
        		info("item->pContentURL:%s",item->pContentURL);//歌曲链接
        		if(pfileTitle && strlen(pfileTitle) > 0) {
        			item->pTitle = strdup(pfileTitle);
        			info("item->pTitle:%s",item->pTitle);	//歌曲名
        		}
        		if(pSinger && strlen(pSinger) > 0 ) {
        			item->pArtist = strdup(pSinger);
        			info("item->pArtist:%s",item->pArtist);	//作者
        		}
        		if(pTip && strlen(pTip) > 0 )
        			item->pTip = strdup(pTip);		//歌曲名链接
        		item->pId = strdup(mediaId);		//id号
        		item->type = MUSIC_TYPE_SERVER;		//type类型
        		TuringPlaylistInsert(TURING_PLAYLIST, item, 2);
        		FreeMusicItem(item);
        	}
        }
	}

#else
		if((cdb_get_int("$playmode", 0) == 4) && (tfMode_IsDone_state == 0))
			song_story_addTuringJson(root);					//tf卡播放时候，将语音点歌放入TURING_PLAYLIST列表中
#endif

    /* 更新token */
    token = json_object_object_get(root, "token");
    if(token != NULL) 
    {
        char *pToken = NULL;
        pToken = json_object_get_string(token);
        info("token:%s",pToken);
        SetTuringToken(pToken);
        cdb_set("$turing_token",pToken);
        //WIFIAudio_UciConfig_SearchAndSetValueStringNoCommit("xzxwifiaudio.config.TuringToken", pToken);
    }
    info("recover_playing_status = %d,song_flag =%d",recover_playing_status,song_flag);

    if(!song_flag)
    {
        if(recover_playing_status)
        {
            usleep(2000*1000);
            while(!IsMpg123Finshed()){usleep(100*1000);}
			MpdPlay(-1);
			StartTlkPlay_led();
        }else
        {
        	while(!IsMpg123Finshed()){usleep(100*1000);}
			StartPowerOn_led();
		}
    }
	
#if 0    
    else
    {
      if(recover_playing_status)
        {
            while(MpdGetPlayState() != 1){usleep(100*1000);}	
            continue_to_play_tf(); 					//原先在播TF卡的内容
        } 
    }
#endif
    recover_playing_status = 0;
	tfMode_IsDone_state = 0;
end:
    if(root)
        json_object_put(root);
    
#if 0

if(err) 
    {
#ifdef TURN_ON_SLEEP_MODE
		InteractionMode();
#else
		ResumeMpdState();
#endif
	}
#endif
	info("exit");
}

#else	


/*****************连续交互***********/
void ProcessturingJson(char *pData)
{

#ifdef ENABLE_CONTINUOUS_INTERACT
    json_object *root = NULL, *code = NULL, *nlp = NULL, *func = NULL, *url = NULL, *val = NULL, *fileUrl = NULL, *ttsUrlAfter = NULL, *token = NULL, *control_flag = NULL;
    json_object *asr = NULL, *tts = NULL;
    int err = 0;
    char *pttsUrl = NULL;
    char *pfileUrl = NULL,*pfileTitle= NULL;
    char *pttsUrlAfter = NULL;
    char *pOriginalTitle = NULL;
    char *pOriginalSinger = NULL;
    char *pSinger = NULL;
    int i = 0;
    int dateLength = 0;
    int iCode;
    int type = 0;
    char *pAsr = NULL;
	int song_flag = 0;	
	
    info("pData:%s",pData);
    SetResumeMpd(1);
	set_interact_status(0);
	if(!FristInteract_flag)
	{
		FristInteract_flag = 1;
		RecoverPlaying_status = recover_playing_status;
	}
    root = json_tokener_parse(pData);
    if (is_error(root)) 
    {
#ifdef TURN_ON_SLEEP_MODE
        SetInterState(INTER_STATE_ERROR);
#endif
        error("json_tokener_parse root failed");
        return ;
    }
    code = json_object_object_get(root, "code");
    if(code != NULL) 
    {     
        iCode = json_object_get_int(code); 
        //code 定义在 http://docs.turingos.cn/ai-wifi/ai-wifi-api/#_9
        switch(iCode)
        {
            /* 正在识别 */
            case 40000:json_object_put(root);
			StartPowerOn_led();
			return ;
            case 40001 ... 49999: //error
            err = 0;
#ifdef TURN_ON_SLEEP_MODE
            err = play_error_notice_speech();
            goto exit;
#else                
         //   play_response_nlp(root);
			AddErrorCount();
			set_interact_status(1);
			if(GetErrorCount() == 5)
			{
				AddMpg123Url("/root/iot/can_not_hear_you.mp3");
				printf("出现错误次数:total=%d,连续交互结束333\n",GetErrorCount());
				set_interact_status(0);
				seterrorcount(0);
			}
			break;
#endif
            break;
            
            case 20001: shutdown(root);                     break;
            case 20002: setting_system_volume(root);EnterInteractionMode(); break;
            case 20000: 
            case 20003:
            case 20005:
            case 20006:
            case 20009:
            case 20012:
            case 20015: play_response_nlp(root);EnterInteractionMode();break;
            case 20016: change_current_color(root);         	   	   break;									//随意切换颜色
            case 20018:
            case 20019:
            case 20020: play_response_nlp(root);EnterInteractionMode();break;
            case 20021: adjust_bulb_brightness(root);       	   	   break; 									//灯亮度调节
            case 20007: 
            case 20008: song_story_process(root); song_flag = 1;       break;
            case 20011: animal_cry_sound(root);	  tfMode_IsDone_state = 1;EnterInteractionMode();break;	//动物叫声
            case 20014: pass_game_process(root);  tfMode_IsDone_state = 1;song_flag = 1;EnterInteractionMode();break; 		//闯关游戏
            case 20022: electricity_quantity_inquiry(root);EnterInteractionMode();break;									//电量查询
            case 20025: setting_alarm_clock(root);EnterInteractionMode();break;									//设置闹钟
            case 20028: play_response_nlp(root);  EnterInteractionMode();break;									//中英翻译
            case 20013: //not support
            case 20023: //not support
            case 20024: //not support
            case 20026: open_app(root);break;
            case 20027: song_flag = adjust_bulb_color(root);EnterInteractionMode();break;									//灯颜色调节
          //  case 20027:	
            case 29998: //not support
            case 29999: //not support
#ifdef TURN_ON_SLEEP_MODE
                SetInterState(INTER_STATE_SUCCESS);
                SetErrorCount(0);
#endif
   //             play_response_nlp(root);EnterInteractionMode();break;
                break;
            default:
                break ;
        }
    }

#if 0    
	ttsUrlAfter = json_object_object_get(root, "end");
	if(ttsUrlAfter) 			
		pttsUrlAfter  = json_object_get_string(ttsUrlAfter);
		info("pttsUrlAfter is :%s,pfileUrl is :%s",pttsUrlAfter,pfileUrl);
    if (pfileUrl != NULL ||  pttsUrlAfter != NULL)
    {
        long endTime ;
        long startTime = getCurrentTime();
     
        //tts语音播放结束，判断有没有歌曲链接信息,有则加入到mpd中
		if(pfileUrl && strlen(pfileUrl) > 0) 
		{
			if(pfileTitle) 	//歌曲名
			{
#ifdef USE_MPDCLI
				MpdAdd(pfileUrl);		//添加歌曲链接
#else
				eval("mpc","add", pfileUrl);
#endif
				
			}
            cdb_set_int("$turing_mpd_play", 0);  //歌曲等音频链接就设置为0
		}
		if(pttsUrlAfter) 		//添加后续的歌曲链接
        {
#ifdef USE_MPDCLI
            MpdAdd(pttsUrlAfter);
#else
            eval("mpc","add", pttsUrlAfter);
#endif
        }

        endTime = getCurrentTime();
        if(type) 
        {
        	MusicItem *item = NewMusicItem();
        	if(item) 
        	{
        		info("item != NULL");
        		char mediaId[128] = {0};
        		snprintf(mediaId, 128, "%d", id);	
        		info("mediaId:%s",mediaId);
        		if(pfileUrl && strlen(pfileUrl) > 0)
        			item->pContentURL = strdup(pfileUrl);
        		info("item->pContentURL:%s",item->pContentURL);//歌曲链接
        		if(pfileTitle && strlen(pfileTitle) > 0) {
        			item->pTitle = strdup(pfileTitle);
        			info("item->pTitle:%s",item->pTitle);	//歌曲名
        		}
        		if(pSinger && strlen(pSinger) > 0 ) {
        			item->pArtist = strdup(pSinger);
        			info("item->pArtist:%s",item->pArtist);	//作者
        		}
        		if(pTip && strlen(pTip) > 0 )
        			item->pTip = strdup(pTip);		//歌曲名链接
        		item->pId = strdup(mediaId);		//id号
        		item->type = MUSIC_TYPE_SERVER;		//type类型
        		TuringPlaylistInsert(TURING_PLAYLIST, item, 2);
        		FreeMusicItem(item);
        	}
        }
	}

#else
		if((cdb_get_int("$playmode", 0) == 4) && (tfMode_IsDone_state == 0))
			song_story_addTuringJson(root);					//tf卡播放时候，将语音点歌放入TURING_PLAYLIST列表中
#endif

    /* 更新token */
    token = json_object_object_get(root, "token");
    if(token != NULL) 
    {
        char *pToken = NULL;
        pToken = json_object_get_string(token);
        info("token:%s",pToken);
        SetTuringToken(pToken);
        cdb_set("$turing_token",pToken);
    }
	
    info("recover_playing_status = %d,song_flag =%d",recover_playing_status,song_flag);
	info("RecoverPlaying_status = %d ,Interact_flag = %d",RecoverPlaying_status,get_interact_status());
    if(!song_flag)									//tts播放状态
    {
    	if(get_interact_status() == 0)				//表示连续交互结束
		{
			if(RecoverPlaying_status)
			{
			//	FristInteract_flag = 0;
				EndInteractionMode();
				usleep(1000*1000);
				while(!IsMpg123Finshed()){usleep(100*1000);}
				MpdPlay(-1);
				StartTlkPlay_led();
			}else
			{
			//	FristInteract_flag = 0;
				EndInteractionMode();
				while(!IsMpg123Finshed()){usleep(100*1000);}
				StartPowerOn_led();
			}
		}
		else if(get_interact_status() == 1)							//进行连续交互...
		{
			while(!IsMpg123Finshed()){usleep(100*1000);}
			StartPowerOn_led();
		} 	
    }
	else								//歌曲和故事状态
	{
	//	FristInteract_flag = 0;	
		EndInteractionMode();
	}
			
	
	if(get_interact_status()){
		while(!IsMpg123Finshed()){usleep(100*1000);}
		Start_record_data();								//闲聊模式下,进行连续交互
		info("FristInteract_flag = %d",FristInteract_flag);
	}else{
		info("FristInteract_flag = %d",FristInteract_flag);
	//	stop_record_data();									//返回码:40001 ... 49999,结束连续交互
	}		
		
		
    recover_playing_status = 0;
	tfMode_IsDone_state = 0;
end:
    if(root)
        json_object_put(root);
    
#if 0

if(err) 
    {
#ifdef TURN_ON_SLEEP_MODE
		InteractionMode();
#else
		ResumeMpdState();
#endif
	}
#endif
	info("exit");

#endif
}
#endif
