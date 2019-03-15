#include "init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>  
#include "sig.h"
#include "common.h"
#include "myutils/debug.h"
#include "TuringWeChatList.h"
#include "libcchip/function/pwm_control/pwmconfig.h"

GLOBALPRARM_STRU g;

#ifdef ENABLE_VOICE_DIARY
static char *g_key="1d465391260749458b65c27b020207f1";
#else
#ifndef ENABLE_COMMSAT
static char *g_key="242ad604899245ea87cc95d2a8fac976";
#else
//九天微星apikey
static char *g_key="6ecb5cc23566471ea2b050938b271f35";
#endif

#endif

#ifdef ENABLE_VOICE_DIARY
static	char *g_aesKey = "3fcfbea64174456b";
#else

#ifndef ENABLE_COMMSAT
static	char *g_aesKey = "RJH983ZPWhuMMJ11";
#else
//九天微星aesKey
static	char *g_aesKey     = "55d429232359a731";
#endif

#endif
static void iot_cdb_init()
{
	cdb_set_int("$current_play_mode", 1);
	cdb_set_int("$turing_stop_time", 1);
	cdb_set_int("$turing_mpd_play", 0);
	cdb_set_int("$turing_power_state", 0);
}


void TuringInit(char *apiKey, char *aesKey)
{
	char *userId = NULL;
	char *deviceId[17]= {0};
	char token[33] = {0};
	
	char *api = NULL;
	char *aes = NULL;
	
	char *apiKeyVal = NULL;
	char *aesKeyVal = NULL;	

    //每次从json文件中只解析一个变量的值，浪费
    //读取之后没有释放堆内存，导致内存泄漏
    //记录于：2018-04-28 16:30:23
    //这部分代码是李笑笑写的，等我有空再来改
    
	api = ConfigParserGetValue("apiKeyTuring");
	if(!api) 
	{
		apiKeyVal = apiKey;
	} else {
		apiKeyVal = api;
	}
	info("apiKeyVal:%s",apiKeyVal);
	aes = ConfigParserGetValue("aesKeyTuring");
	if(!aes) 
	{
		aesKeyVal = aesKey;
	} else {
		aesKeyVal = aes;
	}
	info("aesKeyVal:%s",aesKeyVal);
	userId = GetUserID(apiKeyVal, aesKeyVal);
	info("userId:%s",userId);

	//char *turingToken = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.TuringToken");
    cdb_get("$TuringToken",token);


	
	if(userId){
		GetDeviceID(deviceId);
		TuringUserInit(apiKeyVal, aesKeyVal, deviceId, userId, token);
	}
	char *toneStr = ConfigParserGetValue("toneTuring");
	if(toneStr) 
	{
		int tone = atoi(toneStr);
		if(tone >= 0 && tone <= 2) 
			SetTuringTone(tone);
		free(toneStr);
        toneStr = NULL;
	} 

    char *productId = ConfigParserGetValue("productId");
    if(productId)
    {
        SetTuringProductId(productId);
        free(productId);
        productId = NULL;
    }
    

	info("token:%s",GetTuringToken());
	info("userId:%s",GetTuringUserId());
	info("deviceId:%s",GetTuringDeviceId());
	info("apiKey:%s",GetTuringApiKey());
	info("aesKey:%s",GetTuringAesKey());
	info("tone:%d",GetTuringTone());
    info("productId:%d",GetTuringProductId());
    
	if(api) {
		free(api);
		api = NULL;
	}
	if(aes) {
		free(aes);
		aes = NULL;
	}
}

static void Init_VADThreshold(void)
{
	char *token = NULL;
	g.m_iVadThreshold = 0;
    
	char *pVADThrshold = ConfigParserGetValue("vad_Threshold");
    info("pVADThrshold = %s",pVADThrshold);
	if(pVADThrshold)
    {
		g.m_iVadThreshold = atoi(pVADThrshold);
		free(pVADThrshold);
	}
    else
    {
        g.m_iVadThreshold = 1000000;
    }
}
static void light_init()
{
    char buf[4] = {0};
    char *color = "WHI";
    int brightness = cdb_get_int("$sys_brightness",100);
    cdb_get_str("$sys_color",buf,4,"WHI");
    color = buf;
    Rgb_Set_Singer_Color(color,brightness);
}

void init(int argc , char **argv)
{
	PidLock(argv[0]);
	signal_setup();
	LogInit(argc, argv);
	iot_cdb_init();
	Init_VADThreshold();
	TuringWeChatListInit();
	MpdInit();
	MqttInit();
	QueueInit();
	TuringInit(g_key, g_aesKey);
#ifndef ENABLE_HUABEI
	ColumnAudioInit();
	AlertInit();
	//SwitchInit();
	IotInit();
	//ResumeMpdStateStartingUp();
#endif

#ifdef ENABLE_HUABEI
	HuabeiInit();
	HuabeiEventInit();
#endif	

#if defined(CONFIG_PROJECT_BEDLAMP_V1) 
    //WIFI自己使用PWM调节灯颜色亮度
    Rgb_All_Init();
    light_init();
#endif    
}


void deinit(void)
{
	QueueDeinit();
#ifndef ENABLE_HUABE
	ColumnAudioDeinit();
	AlertDeinit();
#endif
	MpdDeinit();
	TuringWeChatListDeinit();
	PidUnLock();
}



