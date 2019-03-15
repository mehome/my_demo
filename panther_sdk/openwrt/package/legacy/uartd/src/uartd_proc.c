#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <semaphore.h>
#include "uartd_proc.h"
#include "debug.h"
#include "mon_config.h"
#include "uartd_test_mode.h"
#include "montage.h"
#include <wdk/cdb.h>
#include <libcchip/platform.h>
#include <mon_config.h>
#include "uartfifo.h"
#include "myutils/msg_queue.h"

typedef struct __uartd_proc_t {
    const char *name;
    int (*exec_proc)(BUF_INF *cmd);
} uartd_proc_t;

#define UNIX_DOMAIN "/tmp/UNIX.baidu"
#define UNIX_WAKEUP   "/tmp/UNIX.wakeup"
#define TURING_DOMAIN "/tmp/UNIX.turing"
#define HUABEI_DOMAIN "/tmp/UNIX.huabei"
#define TURINGKEYCONTROL_DOMAIN "/tmp/UNIX.turingkeycontrol"


static  uartd_proc_t cmd_tbl[] = {
		{ "MCU+VOL", 		Process_VOL_CMD},	
	   	{ "MCU+WIF+",		Process_WIFI_CMD},
	  	{ "MCU+PLY+", 		Process_PLY_CMD },
	   	{ "MCU+PLP+",     	Process_PLP_CMD },
		{ "MCU+TLK+",     	Process_TLK_CMD },
		{ "MCU+DUE", 		Process_DUE_CMD},	
		{ "MCU+GET+TIM&", 	Process_AT_TIME_CMD },
		{ "MCU+FACTORY&", 	Process_AT_FACTORY_CMD},
		{ "MCU+SSID+", 		Process_SSID_CMD },
		{ "MCU+PLM+", 		Process_AT_PLAYMODE },	//wifi/蓝牙切换
		//{ "MCU+PLM+", 		Process_WIFI_CMD },
	    { "MCU+PLM-", 		Process_AT_PLAYMODE },		
	    { "MCU+NAM+", 		Process_AT_NAME_CMD },
	    { "MCU+LANG+", 		Process_AT_LANGUAGE_CMD },
	    { "MCU+SIDP+", 		Process_AT_SIDP_CMD },
	    { "MCU+MAC", 		Process_MAC_CMD },
	    { "MCU+SID+", 		Process_AT_SSID_CMD },
	    { "MCU+KEYS+", 		Process_AT_KEYS_CMD },
	    { "MCU+TESTMODE&", 	Process_AT_TESTMODE_CMD },
	    { "MCU+CHE", 		Process_CHE_CMD },
	    { "MCU+VER+", 		Process_AT_MCU_VER_CMD },
	    { "MCU+COL+", 		Process_COL_CMD },
	    { "MCU+KEY+", 		Process_AT_KEY_CMD },
	    { "MCU+POW+OFF&", 	Process_POWER_OFF_CMD},
//	    { "MCU+SEQ+", 		NULL },
//	    { "MCU+CHN+", 		NULL },
//	    { "MCU+MIC+",		NULL},
	    { "MCU+BAT", 		Process_AT_BATTERY },	//蓝牙定时发送电池电量信息
	    { "MCU+CHG", 		Process_AT_CHARGE },
//	    { "MCU+CAP+LAU", 		NULL },
	    { "MCU+IN", 		Process_AT_IN },
	    { "MCU+BTS+SLP&", 		Process_AT_BTS_SLP },
	    { "MCU+BCN", 		Process_AT_BCN },
	    { "MCU+BSC",		Process_AT_BSC },
	    { "MCU+MUT+MIC&", 	Process_MIC_MUT },
	    { "MCU+UNM+MIC&", 	Process_MIC_UNM },
	    { "MCU+TIM+END&", 	Process_Stop_Time },
	    { "MCU+TIM+GET&", 	Process_Get_Time },
		{ "MCU+LED+",		Process_LED_CMD }		//获取灯的颜色
};


extern int record ;

extern pthread_mutex_t pmMUT;
extern int iUartfd;

extern sem_t playwav_sem;
extern struct mpd_connection *setup_connection(void);

extern int speech_or_iflytek;

int g_nameFlag = 0;;
//int g_testmodeFlag=0;


static char AT_POW_OFF[] = "MCU+POW+OFF&";//1??ú
static char AT_WIFI_WPS[] = "MCU+WIF+WPS&";//????	百度SDK下该指令为切换到AP模式
static char AT_WIFI_DIS[] = "MCU+WIF+DIS&";//???a
static char AT_WIFI_CON[] = "MCU+WIF+CON&";//á??ó
static char AT_VOL_UP[] = "MCU+VOL++++&";//ò?á??ó
static char AT_VOL_DOWN[] = "MCU+VOL----&";//ò?á???
static char AT_VOL[] = "MCU+VOL+";//ò?á?μ÷?ú 000-100
static char AT_VOL2[] = "MCU+VOL-";//ò?á?μ÷?ú 000-100
static char AT_PLAY[] = "MCU+PLY+PUS&";//2￥·?/?Yí￡
static char AT_STOP[] = "MCU+PLY+STP&";//í￡?1
static char AT_NEXT[] = "MCU+PLY+NXT&";//??ò??ú
static char AT_PREV[] = "MCU+PLY+PRV&";//é?ò??ú
static char AT_TLK_ON[] = "MCU+TLK++ON&";//???ˉMIC
static char AT_TLK_WECHAT[] =   "MCU+TLK+WECHAT&";
static char AT_TLK_MENU[]   =   "MCU+TLK+MENU&";
static char AT_TLK_COLLECT[]=   "MCU+TLK+COLLECT&";
static char AT_TLK_TRAN[]   =   "MCU+TLK+TRAN&";




static char AT_TLK_OFF[] = "MCU+TLK+OFF&";//1?±?MIC
static char AT_FACTORY[] = "MCU+FACTORY&";//???′3?3§
static char AT_SSID_ON[] = "MCU+SSID+ON&";//??ê?apèèμ?
static char AT_SSID_OFF[] = "MCU+SSID+OFF&";//òt2?apèèμ?
static char AT_MUTE_ON[] = "MCU+MUT+000&";//000:?2ò?
static char AT_MUTE_OFF[] = "MCU+MUT+001&";//001:è????2ò?
static char AT_TIME[] = "MCU+GET+TIM&";//??è?ê±??
//static char AT_KEY[] = "MCU+KEY+";//2￥·??ì?Y?ü:000-100 PRV:é?ò??? NXT:??ò???
static char AT_PLP[] = "MCU+PLP+";//2￥·??-?·?￡ê? 000:?3Dò 001:???′ 002:μ￥?ú 003:???ú
static char AT_SEQ[] = "MCU+SEQ+";//?ùoa?÷
static char AT_CHANNEL[] = "MCU+CHN+";//éùμà 000:á￠ì?éù 001:×óéùμà 002:óòéùμà
static char AT_PLAYMODE[] = "MCU+PLM+";//2￥·??￡ê? 000:WIFI 001:BT 002:AUX
static char AT_PLAYMODE1[] = "MCU+PLM-";//2￥·??￡ê? 000:WIFI 001:BT 002:AUX
static char AT_BAT[] = "MCU+BAT+";//μ?á?000-010
static char AT_CHG[] = "MCU+CHG+";//不充电:000 充电:001

static char AT_NAME[] = "MCU+NAM+";//éè??dlna/airplay??×?
static char AT_MIC_ON[]="MCU+MIC+ON";
static char AT_MIC_OFF[]="MCU+MIC+OFF";
static char AT_CAP[] 		="MCU+CAP+LAU";
static char AT_LANGUAGE[] = "MCU+LANG+";//?D??ó???
static char AT_SIDP[] = "MCU+SIDP+";
static char AT_MAC1[] = "MCU+MAC1+";//DT??eth0 MAC address
static char AT_MAC2[] = "MCU+MAC2+";//DT??ra0 MAC address
static char AT_SSID[] = "MCU+SID+";//DT??wifi ssid
static char AT_KEYS[] = "MCU+KEYS+";//DT??wifi key
static char AT_TESTMODE[] = "MCU+TESTMODE&";//2aê??￡ê?

static char AT_TF_CHECK[] = "MCU+CHE++TF&";//2é?ˉTF?¨ê?·??ú??,MCU·￠??WIFI￡oMCU+CHE+TF&	WIFI???′￡oTF?¨?ú??￡oAXX+TF+ONL& TF?¨2??ú??￡oAXX+TF+OUL&
static char AT_USB_CHECK[] = "MCU+CHE+USB&";//2é?ˉU?ìê?·??ú??,MCU·￠??WIFI￡oMCU+CHE+USB& WIFI???′￡oUSB?ú??￡oAXX+USB+ONL& USB2??ú??￡oAXX+USB+OUL&


static char AT_SW_WIFIMODE[] = "MCU+PLY+WIF&"; //?D??μ?WIFI￡?MCU·￠??WIFI￡o
static char AT_SW_TFPLAY[] = "MCU+PLY++TF&";//?D??μ?TF?¨￡?MCU·￠??WIFI￡o(??è?2￥·?)
static char AT_SW_TFPAUSE[] = "MCU+PLP++TF&";//?D??μ?TF?¨￡?(??è??Yí￡)MCU+PLP+TF&
static char AT_SW_USBPLAY[] = "MCU+PLY+USB&";//?D??μ?USB?¨￡MCU+PLY+USB&(??è?2￥·?)
static char AT_SW_USBPAUSE[] = "MCU+PLP+USB&";//?D??μ?USB?¨￡?(??è??Yí￡)MCU+PLP+USB&

static char AT_MCU_VER[] ="MCU+VER+";


static char AT_COL_DEL[] ="MCU+COL+DEL&";   //é?3yê?2????è?ú

static char AT_COL[] ="MCU+COL+";   //ò??üìy ê?2?

static char AT_KEY[] ="MCU+KEY+";   //ò??üìy 2￥·?

static char AT_IN[] ="MCU+IN";				//???÷μ?BTéè±?
static char AT_BTS_INT[] ="MCU+BTS+INT&";	//é?μ?3?ê??ˉíê3é
static char AT_BTS_SLP[] ="MCU+BTS+SLP&";	//′y?ú?￡ê? (?′á??ó×′ì?)
static char AT_BCN_FAL[] ="MCU+BCN+FAL&";	//á??óê§°ü
static char AT_BCN_SUC[] ="MCU+BCN+SUC&";	//????3é1|
static char AT_BCN_ING[] ="MCU+BCN+ING&";	//?y?úá??ó
static char AT_BSC_ING[] ="MCU+BSC+ING&";	//?y?ú???÷
static char AT_BSC_END[] ="MCU+BSC+END&";	//???÷íê3é
static char AT_MUT_MIC[] ="MCU+MUT+MIC&";	//MUTE MIC
static char AT_UNM_MIC[] ="MCU+UNM+MIC&";	//UNMUTE MIC

int wav_play2(const char *file)
{	
	char buf[512] = {0};
    int ret = -1;
	exec_cmd("killall -9 wavplayer");
	snprintf(buf, 512, "wavplayer %s" ,file);
	ret = exec_cmd(buf);
    error("buf = %s",buf);
    return ret;
}



static int speaker_not_wifi(void)
{
	int ret1 = system("wavplayer /tmp/res/speaker_not_connected.wav -M");
	printf("ret1=== %d\n",ret1);
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK++OFF&", strlen("AXX+TLK++OFF&"));
	pthread_mutex_unlock(&pmMUT);
	record = 0;
	return 1;
}

static int amazon_unsuccessful(void)
{
	int ret1 = system("wavplayer /tmp/res/amazon_unsuccessful.wav -M");
	printf("ret1=== %d\n",ret1);
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK++OFF&", strlen("AXX+TLK++OFF&"));
	pthread_mutex_unlock(&pmMUT);
	record = 0;
	return 1;
}

int write_msg_to_Turingiot(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, TURINGKEYCONTROL_DOMAIN);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

int write_msg_to_turing_keycontrol(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}


#if 1
void StartConnecting_led()
{
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)
			exec_cmd("uartdfifo.sh StartConnecting");
//#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
//			exec_cmd("uartdfifo.sh StartConnecting");
#else
	
#endif
}


void StartConnectedOk_led()
{
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)
			exec_cmd("uartdfifo.sh StartConnectedOK");
//#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
//			exec_cmd("uartdfifo.sh StartConnectedOK");
#else
	
#endif
}

void StartConnectedFAL_led()
{
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)
			exec_cmd("uartdfifo.sh StartConnectedFAL");
//#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
//			exec_cmd("uartdfifo.sh StartConnectedFAL");
#else
	
#endif
}
#endif
static int Process_WIFI_WPS_CMD(void)
{
    int ret = -1;
    my_system("killall -9 elian.sh");
#if 1 //ENABLE_AIRKISS
    if (vfork() == 0)
    {
        ret = execl("/usr/bin/elian.sh", "elian.sh", "air_conf", NULL);
        exit(-1);
    }
#else
    if (vfork() == 0)
    {
        ret = execl("/usr/bin/elian.sh", "elian.sh", "config", NULL);
        exit(-1);
    }
#endif    

	return 0;
}

static int Process_WIFI_CON_CMD(void)
{
	return my_system("elian.sh restore");
}

static int Process_WIFI_DIS_CMD(void)
{
	return my_system("elian.sh disconnect");
}

static int Process_VOL_UP_CMD(void)
{
#if 0
	return iRet;
#else
	//char *mslave ;
	int multiroom_status = cdb_get_int("$multiroom_status",0);
	//if(strcmp(mslave, "2") == 0){
	  if(multiroom_status == 2){
		my_system("multiClient.sh addVol 10");
		}
	return my_system("mpc volume +10");
#endif
}

static int Process_VOL_DOWN_CMD(void)
{
#if 0
	return iRet;
#else
//	char *mslave ;
//	mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
	int multiroom_status = cdb_get_int("$multiroom_status",0);
	//if(strcmp(mslave, "2") == 0){
	if(multiroom_status == 2){
		my_system("multiClient.sh reduceVol 10");
		}
	return 	my_system("mpc volume -10");
#endif
}

static int Process_AT_TIME_CMD(BUF_INF *cmd)
{
	struct   tm     *ptm;		
	long       ts;		
	int         year,hour,day,month,minute,second,week;		

	char run[64]={0};		
	ts   =   time(NULL);		
	ptm   =   localtime(&ts); 				
	year   =   ptm-> tm_year+1900;     //?ê		
	month   =   ptm-> tm_mon+1;             //??		
	day   =   ptm-> tm_mday;               //è?		
	hour   =   ptm-> tm_hour;               //ê±		
	minute   =   ptm-> tm_min;                 //·?		
	second   =   ptm-> tm_sec;                 //?? 		
	week = ptm->tm_wday;				

	snprintf(run, 64, "AXX+TIM+%02d%02d%02d%02d%d%02d%4d&", second, minute, hour, day, week, month, year);

	pthread_mutex_lock(&pmMUT);
	write(iUartfd, run, strlen(run));
	pthread_mutex_unlock(&pmMUT);

	return 0;
}

static int Process_AT_VOL_CMD(BUF_INF *pCmd)
{
	int iRet = -1;
	char run[128] = {0};
	char vol[4] = {0};
	if(pCmd->len < 11){
		return -1;
	}
	strncpy(vol, &(pCmd->buf[8]), 3);
#if 0	
/* BEGIN: Added by Frog, 2017-5-27 11:06:40 */
    if(0 == access("/tmp/test_mode",0))
    {
    	//char *pVolume = NULL;
		//pVolume = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.volume");
		 int pVolume = cdb_get_int("$ra_vol", 0);
		if(pVolume - atoi(vol) < 0)
		{
			// send_button_direction_to_dueros(2, KEY_VOLUME_UP, 10);
	        send_cmd_to_server(BUTTON_VOLUME_UP_PRESSED);
	    }
	    else
	    {
			// send_button_direction_to_dueros(2, KEY_VOLUME_DOWN, 10);
	        send_cmd_to_server(BUTTON_VOLUME_DOWN_PRESSED);
	    }
	    //free(pVolume);
    }

/* END:   Added by Frog, 2017-5-27 11:06:49 */
#endif
//	WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.volume", vol);
	int volume = atoi(vol);
//	cdb_set_int("$ra_vol", volume);
	// iRet=set_system_volume(volume);
	
#if defined(CONFIG_PACKAGE_duer_linux)
	iRet=send_button_direction_to_dueros(2, KEY_RT_VOLUME, volume);
#elif defined(CONFIG_PACKAGE_iot)
    char buf[32]={0};
    sprintf(buf,"SystemVolume%d",volume);
    iRet = msg_queue_send(buf,MSG_UARTD_TO_IOT);
#endif


	if(iRet<0){
		return -1;
	}
	return 0;	
#if 0
//	int mode ;
//	mode = cdb_get_int("$current_play_mode",0);
//	DEBUG_INFO("mode:%d",mode);
//	if(mode == 1) { //MONDLNA
//		snprintf(run, sizeof(run), "mpc volume %d", volume);
//	} else if(mode == 2) { //MONAIRPLAY
//		snprintf(run, sizeof(run), "air_cli v %d", volume);
//	} else if(mode == 3) { //MONSPOTIFY
//		snprintf(run, sizeof(run), "mspotify_cli v %d", volume);
//	} else {// MONNONE
//		//do nothing
//	}
//	iRet = my_system(run);
	if(iRet == 0 && speech_or_iflytek == 3){
		DEBUG_INFO("OnSendCommandToAmazonAlexa VolumeDown");
//		OnWriteMsgToAlexa("VolumeDown");
		OnSendCommandToAmazonAlexa("VolumeDown");
	}
	
	//char *mslave ;
	int multiroom_status = cdb_get_int("$multiroom_status", 0);
	//mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
	if(multiroom_status == 2){
		memset(run,0,sizeof(run)/sizeof(char));
		snprintf(run, sizeof(run), "multiClient.sh setVolume %d", volume - 201);
		my_system(run);
	}
	return iRet;
#endif
}


static int Process_AT_SIDP_CMD(BUF_INF *pCmd)
{
	int iRet = -1;
	char run[128] = {0};
	char ssid[32] = {0};
	char psk[32] = {0};
	if (sscanf(&(pCmd->buf[sizeof(AT_SIDP)-1]), "%[^+]+%[^&]&", ssid, psk) != 2)
	{
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "ERR\r\n",sizeof("ERR\r\n"));
		pthread_mutex_unlock(&pmMUT);
		return;
	}
	printf("%s %s\n", ssid, psk);
	snprintf(run, sizeof(run), "elian.sh connect %s %s", ssid, psk);
	iRet = my_system(run);

	return iRet;
}

static int Process_AT_PLAY_CMD(void)
{
	int mode ;
	int iRet = -1;
	mode = cdb_get_int("$current_play_mode", 0);
	DEBUG_INFO("mode:%d",mode);
	if(mode == 1) { //MONDLNA
			iRet = my_system("mpc toggle");
	} else if(mode == 2) { //MONAIRPLAY
			iRet = my_system("air_cli d  playpause");
	} else if(mode == 3) { //MONSPOTIFY
			iRet = my_system("mspotify_cli s toggle");
	} else if(mode == 5 && speech_or_iflytek == 3){
			iRet = OnSendCommandToAmazonAlexa("MpcPlayEvent");
	} else {// MONNONE
			//do nothing
	}
   //char *mslave ;
   //mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
	//if(strcmp(mslave, "2") == 0){
  int multiroom_status = cdb_get_int("$multiroom_status", 0);
	//mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
	if(multiroom_status == 2){
			my_system("multiClient.sh toggle");
		}
	return iRet;
}

static int Process_AT_NEXT_CMD(void)
{
    int iRet = -1;
#ifdef CONFIG_PACKAGE_duer_linux
	int err=send_button_direction_to_dueros(2, KEY_VOLUME_UP, 10);
	if(err<0){
		return -1;
	}
	inf("KEY_VOLUME_DOWN volume +10");
	return 0;
#elif defined(CONFIG_PACKAGE_iot)
    iRet = my_system("mpc next");
    return iRet;
#else	
	//int mode ;
	int mode = cdb_get_int("$current_play_mode", 0);
	DEBUG_INFO("Process_AT_NEXT_CMD mode:%d",mode);
	if(mode == 1) { //MONDLNA
		iRet = my_system("mpc next");
	} else if(mode == 2) { //MONAIRPLAY
		iRet = my_system("air_cli d nextitem");
	} else if(mode == 3) { //MONSPOTIFY
		iRet = my_system("mspotify_cli s next");
	} else if(mode == 5){
		iRet = OnSendCommandToAmazonAlexa("MpcNextEvent");
	} else{
		//do nothing
	}
  //char *mslave ;
 //	mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
 //	if(strcmp(mslave, "2") == 0){
 int multiroom_status = cdb_get_int("$multiroom_status", 0);
	//mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
	if(multiroom_status == 2){
		my_system("multiClient.sh next");
		}
	DEBUG_INFO("Process_AT_NEXT_CMD iRet:%d",iRet);
	return iRet;
#endif	
}

static int Process_AT_PREV_CMD(void)
{
    int iRet = -1;
#ifdef CONFIG_PACKAGE_duer_linux
	int err=send_button_direction_to_dueros(2, KEY_VOLUME_DOWN, 10);
	if(err<0){
		return -1;
	}
	inf("KEY_VOLUME_DOWN volume -10");
	return 0;
#elif defined(CONFIG_PACKAGE_iot)
        iRet = my_system("mpc prev");
        return iRet;
#else	
	int mode ;

	mode = cdb_get_int("$current_play_mode", 4);
	DEBUG_INFO("mode:%d",mode);
	if(mode == 1) { //MONDLNA
		iRet = my_system("mpc prev");
	} else if(mode == 2) { //MONAIRPLAY
		iRet = my_system("air_cli d previtem");
	} else if(mode == 3) { //MONSPOTIFY
		iRet = my_system("mspotify_cli s prev");
	} else if(mode == 5 && speech_or_iflytek == 3){
		iRet = OnSendCommandToAmazonAlexa("MpcPrevEvent");
	}else {
		//do nothing
	}
  //char *mslave ;
//	mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
//	if(strcmp(mslave, "2") == 0){
  int multiroom_status = cdb_get_int("$multiroom_status", 0);
	  //mslave = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.multiroom_status");
	  if(multiroom_status == 2){
		my_system("multiClient.sh prev");
		}
	return iRet;
#endif	
}

static int Process_AT_STOP_CMD(void)
{
	int iRet = -1;
	int mode ;
	
	mode = cdb_get_int("$current_play_mode", 4);
	DEBUG_INFO("mode:%d",mode);
	if(mode == 1) { //MONDLNA
		iRet = my_system("mpc stop");
	} else if(mode == 2) { //MONAIRPLAY
		iRet = my_system("air_cli s stop");
	} else if(mode == 3) { //MONSPOTIFY
		iRet = my_system("mspotify_cli s stop");
	} else if(mode == 5 && speech_or_iflytek == 3){
		iRet = OnSendCommandToAmazonAlexa("MpcStopEvent");
	} 
	else {// MONNONE
		//do nothing
	}
	
	return iRet;
}

static int Process_AT_FACTORY_CMD(BUF_INF *cmd)
{
	int iRet = -1;
//    send_cmd_to_server(BUTTON_RESET_PRESSED);
#if defined (CONFIG_PROJECT_K2_V1)
	exec_cmd("uartdfifo.sh StartKeyFactory");
#endif
	iRet = my_system("/usr/bin/factoryReset.sh");
	DEBUG_INFO("factoryReset");
	//pthread_mutex_lock(&pmMUT);
	//write(iUartfd, "AXX+CAP+GET&\r\n",sizeof("AXX+CAP+GET&\r\n"));
	//pthread_mutex_unlock(&pmMUT);
	//iRet = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.factory", "1");
	iRet = cdb_set_int("$factory", 1);
	return iRet;
}

static int Process_AT_NAME_CMD(BUF_INF *pCmd)
{
	int iRet = -1;
	char audio_name[128] = "";
	int cmd_len = strlen(pCmd->buf)-1;//去掉结束符
	int name_len=cmd_len-((sizeof(AT_NAME))-1);
	strncpy(audio_name,pCmd->buf+cmd_len-name_len,name_len);
	char suffix[6]="";
	if(name_len+sizeof(suffix)>sizeof(audio_name))return -1;
	char t_mac[32]="";
	boot_cdb_get("mac0", t_mac);
	sprintf(suffix, "_%c%c%c%c", t_mac[12], t_mac[13], t_mac[15], t_mac[16]);
	strcat(audio_name,suffix);
	//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.audioname", audio_name);
	cdb_set("$ra_name", audio_name);
	return iRet;
}

static int Process_AT_PLAYMODE(BUF_INF *pCMD)
{
	int iRet = -1;
	char run[128] = {0};
	char plm[4] = {0};
	strncpy(plm, &(pCMD->buf[sizeof(AT_PLAYMODE)-1]), (sizeof(plm)-1));
//	send_cmd_to_server(BUTTON_MODE_SWITCH_PRESSED);
	
	DEBUG_INFO("AT_PLAYMODE");
	DEBUG_INFO("plm:%s",plm); 
   // int = atoi(plm); 
    if(cdb_get_int("$vtf003_usb_tf",0)){
	//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.shutdown_before_playmode", plm);
	cdb_set("$shutdown_before_playmode", plm);
    	}
	if (plm[0]=='0'&&plm[1]=='0'&&plm[2]=='0')		//智能模式
	{
	//    WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", plm);
		cdb_set("$playmode", plm);
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		//iRet = my_system("mpc play");
       // iRet = msg_queue_send("WifiBlueMode",MSG_UARTD_TO_IOT);
       	  iRet = write_msg_to_Turingiot("WifiBlueMode");
#else
		iRet = my_system("mpc play && upmpdfifo.sh setplm wifi && /etc/init.d/shairport start");
#endif


	}
	else if (plm[0]=='0'&&plm[1]=='0'&&plm[2]=='1')	//蓝牙模式
	{
	   // WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", plm);
	   cdb_set("$playmode", plm);
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		//iRet = my_system("mpc pause");
#else
		iRet = my_system("/etc/init.d/shairport stop && mpc pause && upmpdfifo.sh setplm bt");
#endif
		iRet = write_msg_to_Turingiot("WifiBlueMode");
	}
	else if (plm[0]=='0'&&plm[1]=='0'&&plm[2]=='2')	
	{
	   // WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", plm);
	    cdb_set("$playmode", plm);
		iRet = my_system("/etc/init.d/shairport stop && mpc pause && upmpdfifo.sh setplm aux");
	}else if (plm[0]=='0'&&plm[1]=='0'&&plm[2]=='5')
	{
	    //WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", plm);
	    cdb_set("$playmode", plm);
		iRet = my_system("/etc/init.d/shairport stop && mpc pause && upmpdfifo.sh setplm fm");
	}
	else if (plm[0]=='0'&&plm[1]=='0'&&plm[2]=='3')   //此处如果没有返回 将会打印uartd_proc
	{
	    iRet =0;
	}
	else if (plm[0]=='0'&&plm[1]=='0'&&plm[2]=='4')
	{
	    iRet =0;
	}

	return iRet;
}

static int Process_AT_MAC1_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	DEBUG_INFO("AT_MAC1");
	DEBUG_INFO("len=%d",sizeof(AT_MAC1));
	char mac1[3] = {0};		
	char mac2[3] = {0};		
	char mac3[3] = {0};		
	char mac4[3] = {0};		
	char mac5[3] = {0};		
	char mac6[3] = {0};

	char mac[18]={0};

	memcpy(mac, pCMD->buf+sizeof(AT_MAC1)-1, 17);
	DEBUG_INFO("****mac=%s",mac);
	sscanf(mac, "%2s %2s %2s %2s %2s %2s", mac1, mac2, mac3, mac4, mac5, mac6);
	//
	bzero(mac,18);
	snprintf(mac, 18,"%2s:%2s:%2s:%2s:%2s:%2s", mac1, mac2, mac3, mac4, mac5, mac6 );
	DEBUG_INFO("mac=%s",mac);

	char run[128] ={0};
	DEBUG_INFO("MAC1:%s",mac);
	
	snprintf(run, sizeof(run), "echo mac0 %s > /proc/bootvars",mac);
	DEBUG_INFO("run:%s",run);
	iRet = my_system(run);

	return iRet;
}

static int Process_AT_MAC2_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	DEBUG_INFO("AT_MAC2");
	char mac1[3] = {0};		
	char mac2[3] = {0};		
	char mac3[3] = {0};		
	char mac4[3] = {0};		
	char mac5[3] = {0};		
	char mac6[3] = {0};
	
	char mac[18]={0};
	DEBUG_INFO("AT_MAC2=%d",sizeof(AT_MAC2));
	memcpy(mac, pCMD->buf+sizeof(AT_MAC2)-1, 17);
	sscanf(mac, "%2s %2s %2s %2s %2s %2s", mac1, mac2, mac3, mac4, mac5, mac6);
	bzero(mac,18);
	//
	snprintf(mac, 18,"%2s:%2s:%2s:%2s:%2s:%2s", mac1, mac2, mac3, mac4, mac5, mac6 );
	DEBUG_INFO("mac=%s",mac);
	
	char run[128] ={0};
	DEBUG_INFO("MAC2:%s",mac );
	snprintf(run, sizeof(run), "echo mac2 %s > /proc/bootvars",mac);
	DEBUG_INFO("run:%s",run);
	iRet = my_system(run);

	return iRet;
}

static int Process_AT_SSID_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	char ssid[128] ="";
	int cmd_len = strlen(pCMD->buf)-1;//去掉结束符
	int name_len=cmd_len-((sizeof(AT_SSID))-1);
	strncpy(ssid,pCMD->buf+cmd_len-name_len,name_len);
	char suffix[6]="";
	if(name_len+sizeof(suffix)>sizeof(ssid))return -1;
	char t_mac[32]="";
	boot_cdb_get("mac0", t_mac);
	sprintf(suffix, "_%c%c%c%c", t_mac[12], t_mac[13], t_mac[15], t_mac[16]);
	strcat(ssid,suffix);
	cdb_set("$wl_bss_ssid1", ssid);
	//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.ap_ssid1", ssid);
	DEBUG_INFO("ssid:%s",ssid);
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "OK&\r\n",sizeof("OK&\r\n"));
	pthread_mutex_unlock(&pmMUT);

	return iRet;
}

static int Process_AT_KEYS_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	DEBUG_INFO("AT_KEYS");
	char run[128] = {0};
	char key[32] = {0};
	int i = strlen(pCMD->buf);
	while(pCMD->buf[i-1]=='\n' || pCMD->buf[i-1]=='\r')i--;
	if (pCMD->buf[i-1] == '&')//?áê?·?
		i --;
	strncpy(key, &(pCMD->buf[sizeof(AT_KEYS)-1]), i + 1 - sizeof(AT_KEYS));
	//snprintf(run, sizeof(run), "uci set wireless.@wifi-iface[0].key=%s && uci commit", key);
	//DEBUG_INFO("%s %s\n", key, run);

	//iRet = my_system(run);
	return iRet;
}

static int Process_AT_TESTMODE_CMD(BUF_INF *cmd)
{
	int iRet = -1;
	DEBUG_INFO("AT_TESTMODE");
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TESTMODE+OK\r\n",sizeof("AXX+TESTMODE+OK\r\n"));
	pthread_mutex_unlock(&pmMUT);
	gpio_test_xzx();
	scan_usb_mmc();
	my_system("elian.sh connect wifitest 88888888");
	
	//iRet = my_system("uci set elian.config.conf=false && uci commit");

	return iRet;
}

static int Process_AT_TF_CHECK_CMD(void)
{
	int tf_status =0;
	tf_status = cdb_get_int("$tf_status", 0);
	DEBUG_INFO("tf_status:%d",tf_status);
	if(tf_status == 1){
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+TF++ONL&",strlen("AXX+TF++ONL&"));
		pthread_mutex_unlock(&pmMUT); 
	}else {
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+TF++OUL&",strlen("AXX+TF++OUL&"));
		pthread_mutex_unlock(&pmMUT);
	}

	return 0;
}

static int Process_AT_USB_CHECT_CMD(void)
{
	int usb_status =0;
	
	usb_status = cdb_get_int("$usb_status", 0);
	DEBUG_INFO("usb_status:%d",usb_status);
	if(usb_status == 1){
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+USB+ONL&",strlen("AXX+USB+ONL&"));
		pthread_mutex_unlock(&pmMUT); 
	}else {
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+USB+OUL&",strlen("AXX+USB+OUL&"));
		pthread_mutex_unlock(&pmMUT);
	}

	return 0;
}

static int Process_AT_SW_WIFIMODE_CMD(void)
{
	int iRet = -1;
	char run[128] = {0};
	//char *playmode ;
	//playmode = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
	//if(playmode != NULL && !strcmp(playmode, "000")) 
	int playmode = cdb_get_int("$playmode", 0);
	if(playmode == 000)
	{
		DEBUG_INFO("WIFIPLAY mpc play ");
		iRet = my_system("mpc play");
		//free(playmode);
	}
	else
	{
		iRet = my_system("mpc stop && uartdfifo.sh inwifimode && playlist.sh poweron");
	}

	return iRet;
}

static int Process_AT_SW_TFPLAY_CMD(void)
{
	DEBUG_INFO("AT_SW_TFPLAY");

	int iRet = -1;
	int status;
	int result=-1;
	//char *playmode;
	char sd_last_contentURL[SSBUF_LEN_512];
	char buffer[56];
	char cmd[32];
	//playmode = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
    int playmode = cdb_get_int("$playmode", 0);
	DEBUG_INFO("playmode=%d",playmode);
    my_system("wavplayer /root/iot/prepare_tf.wav");
	status = cdb_get_int("$tf_status",0);
	DEBUG_INFO("status=%d", status);
	if(status == 1) 
    {
    	cdb_set_int("$turing_mpd_play",0);
        cdb_set_int("$playmode",4);
		if(cdb_get_int("$vtf003_usb_tf",0))
        {
             cdb_get_str("$sd_last_contentURL",sd_last_contentURL,sizeof(sd_last_contentURL),"");
    		//sd_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.sd_last_contentURL");
    		DEBUG_INFO("sd_last_contentURL =%s",sd_last_contentURL);
            if(sd_last_contentURL!=NULL )
            {
                result=compare_FileAndUrl("/usr/script/playlists/tfPlaylist.m3u",sd_last_contentURL);
                //free(sd_last_contentURL);
            }
    		if(result<=0){
                result=1;
    			delete_file("/usr/script/playlists/tfPlaylist.m3u");
    		}
            DEBUG_INFO("result==[%d]\n",result);
		}
        my_system("mpc clear > /dev/null");
        usleep(200*1000);
        DEBUG_INFO(" system creatPlayList tfmode  before");
    	iRet = my_system("uartdfifo.sh tfint");
        usleep(200*1000);
        if(file_exists("/usr/script/playlists/tfPlaylist.m3u")&&file_exists("/usr/script/playlists/tfPlaylistInfoJson.txt"))
        {   
			if(cdb_get_int("$vtf003_usb_tf",0)){		
			char tempbuff[72];
			memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			sprintf(tempbuff, "playlist.sh swtich_to_tf %d", result);
			iRet = my_system(tempbuff);

			}else{
             my_system("playlist.sh swtich_to_tf");

			}

        }
        else
        {
            memset(buffer,0,sizeof(buffer));
            memset(cmd,0,sizeof(cmd));
            cdb_get_str("$tf_mount_point", buffer, sizeof(buffer), "");
            sprintf(cmd,"mpc update %s",buffer);
            my_system(cmd);
            usleep(3000*1000);
         
			if(cdb_get_int("$vtf003_usb_tf",0)){
			char tempbuff[72];
			memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
			sprintf(tempbuff, "playlist.sh tfautoplay %d", result);
			iRet = my_system(tempbuff);

			}else{
			 my_system("playlist.sh tfautoplay");

			}
        }
		cdb_set_int("$smartmode_state",0);
    	DEBUG_INFO(" system creatPlayList tfmode  after");
    } 
	else 
	{	
		my_system("wavplayer /root/iot/请先插入tf卡.wav");
		return 0;
	}

	return iRet;
}

static int Process_AT_SW_TFPAUSE_CMD(void)
{
	DEBUG_INFO("AT_SW_TFPAUSE");

	int iRet = -1;
	int status = 0;
	//char *value;
	//char *playmode ;
	//playmode = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
    int playmode = cdb_get_int("$playmode",0);
	DEBUG_INFO("playmode=%d",playmode);
	if(playmode == 004) {

	//if(playmode != NULL && !strcmp(playmode, "004")) {
		//free(playmode);
		status = cdb_get_int("$tf_status",0);
		DEBUG_INFO("status=%d",status);
		if(status == 1) {
			DEBUG_INFO("TFPAUSE mpc  pause");
			iRet = my_system("mpc  pause");
		} else {	
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "AXX+TF++OUL&",sizeof("AXX+TF++OUL&"));
			pthread_mutex_unlock(&pmMUT);
			return 0;
		}
	}	

	return iRet;
}

static int Process_AT_SW_USBPLAY_CMD(void)
{
	DEBUG_INFO("AT_SW_USBPLAY");
	//char *value;
	int iRet = -1;
	int status = 0;
	int result=-1;
	//char *playmode ;
		char buffer[56];
		char cmd[32];

	char usb_last_contentURL[SSBUF_LEN_512];
	//playmode = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
     int playmode = cdb_get_int("$playmode",0);
	DEBUG_INFO("playmode=%d",playmode);
    if(playmode == 004)
	//if(playmode != NULL && !strcmp(playmode, "003")) 
	{
		DEBUG_INFO("USBPLAY mpc play ");
		//free(playmode);
		iRet = my_system("mpc  play");
	}
	else
	{
		status = cdb_get_int("$usb_status",0);
		DEBUG_INFO("status=%d",status);
		if(status == 1) 
		{
			if(cdb_get_int("$vtf003_usb_tf",0)){
		    //usb_last_contentURL = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.usb_last_contentURL");
		      cdb_get_str("$usb_last_contentURL",usb_last_contentURL,sizeof(usb_last_contentURL),"");
			    if(usb_last_contentURL!=NULL){
				/* 模式转换，U盘或tf卡一直在线，所以没有更新m3u文件*/	
				result=compare_FileAndUrl("/usr/script/playlists/usbPlaylist.m3u",usb_last_contentURL);
	            //free(usb_last_contentURL);
				}
	            if(result<=0){ /*如果再次插其他的U盘，歌曲不同，返回值为0，需要生成m3u文件*/
	                result=1;
					delete_file("/usr/script/playlists/usbPlaylist.m3u");
				}
			}
		    my_system("mpc clear > /dev/null");
            usleep(200*1000);
		    memset(buffer,0,sizeof(buffer));
			DEBUG_INFO("system creatPlayList usbmode before");
			iRet = my_system("uartdfifo.sh usbint");
			usleep(200*1000);
            if(file_exists("/usr/script/playlists/usbPlaylist.m3u")&&file_exists("/usr/script/playlists/usbPlaylistInfoJson.txt"))
            {    
				    if(cdb_get_int("$vtf003_usb_tf",0)){
					char tempbuff[72];
					memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		            sprintf(tempbuff, "playlist.sh swtich_to_usb %d", result);
		            iRet = my_system(tempbuff);  
					}else{
                      my_system("playlist.sh swtich_to_usb");
					}                 
            }
            else
            {
                memset(buffer,0,sizeof(buffer));
                memset(cmd,0,sizeof(cmd));
                cdb_get_str("$usb_mount_point", buffer, sizeof(buffer), "");
                sprintf(cmd,"mpc update %s",buffer);
                my_system(cmd);
                usleep(3000*1000);

				if(cdb_get_int("$vtf003_usb_tf",0)){
				char tempbuff[72];
				memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
				sprintf(tempbuff, "playlist.sh usbautoplay %d", result);
				iRet = my_system(tempbuff);
				}else{
				 my_system("playlist.sh usbautoplay");
				}
            }
			DEBUG_INFO(" system creatPlayList usbmode after");
			
		} else {
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "AXX+USB+OUL&",sizeof("AXX+USB+OUL&"));
			pthread_mutex_unlock(&pmMUT);
			return 0; 
		}
	}

	return iRet;
}

static int Process_AT_SW_USBPAUSE_CMD(void)
{
	int iRet = -1;
	int status=0;
	//char *playmode ;
	//playmode = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");
	int playmode = cdb_get_int("$playmode",0);
	DEBUG_INFO("playmode=%s",playmode);
	if(playmode == 003) 
	//if(playmode != NULL && !strcmp(playmode, "003")) 
	{
		//free(playmode);
		status = cdb_get_int("$usb_status",0);
		DEBUG_INFO("status=%d",status);
		if(status == 1) 
		{
			DEBUG_INFO("mpc  pause");
			iRet = my_system("mpc  pause");
		} 
		else 
		{
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "AXX+USB+OUL&",strlen("AXX+USB+OUL&"));
			pthread_mutex_unlock(&pmMUT);
			return 0; 
		}
	}

	return iRet;
}

static int Process_AT_PLP_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	DEBUG_INFO("CMD=%s, AT_PLP=%s",pCMD, AT_PLP);
	DEBUG_INFO("AT_PLP");
	iRet = my_system("mpc repeat off ");
	iRet = my_system("mpc random off ");
	iRet = my_system("mpc single off ");
	iRet = my_system("mpc consume off");
	if (pCMD->buf[sizeof(AT_PLP)+1] == '0')
	{
	}
	else if (pCMD->buf[sizeof(AT_PLP)+1] == '1')
	{
		iRet = my_system("mpc repeat on");
	}
	else if (pCMD->buf[sizeof(AT_PLP)+1] == '2')
	{
		iRet = my_system("mpc repeat on && mpc single on");
	}
	else if (pCMD->buf[sizeof(AT_PLP)+1] == '3')
	{
		iRet = my_system("mpc random on");
	}

	return iRet;
}

static int Process_AT_LANGUAGE_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	char run[128] = {0};
	char language[32] = {0};
	int i = strlen(pCMD->buf);
	while(pCMD->buf[i-1]=='\n' || pCMD->buf[i-1]=='\r')i--;
	if (pCMD->buf[i-1] == '&')
		i --;
	if (i < sizeof(AT_LANGUAGE))
		return;
	strncpy(language, &(pCMD->buf[sizeof(AT_LANGUAGE)-1]), i + 1 - sizeof(AT_LANGUAGE));
	snprintf(run, sizeof(run), "setLanguage.sh %s", language);
	printf("%s %s\n", language, run);
	iRet = my_system(run);

	return iRet;
}

static int Process_AT_MCU_VER_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	char run[128] = {0};
	char sw_ver[32] = {0};
	char tmp_ver[32] = {0};
	
	int i = strlen(pCMD->buf);
	
	while(pCMD->buf[i-1]=='\n' || pCMD->buf[i-1]=='\r')i--;
	if (pCMD->buf[i-1] == '&')
		i --;
	if (i < sizeof(AT_MCU_VER))
		return;
	strncpy(sw_ver, &(pCMD->buf[sizeof(AT_MCU_VER)-1]), i + 1 - sizeof(AT_MCU_VER));	
	DEBUG_INFO("sw_ver:%s",sw_ver);
	//sw_ver:2018/10/11-14:08:20	
	//20181011

#if 0
	char *token = strtok(sw_ver,"/-:");
	while(token != NULL)
	{
		token = strtok(NULL,"/-:");	//此时token = "20181011140820"
		if(token == NULL)
			break;
	}
	char *p=NULL;
	strncpy(p,token,strlen(token)-6);	//此时p="20181011"
#endif

	cdb_set("$sw_ver",sw_ver);
	cdb_set("$tmp_bt_ver",sw_ver);


	return iRet;
}

static int Process_AT_COL_DEL_CMD(void)
{
	int iRet = -1;
	DEBUG_INFO("echo del song...............");
	iRet = my_system("creatPlayList delcollplaylist all");

	return iRet;
}



static int Process_AT_COL_CMD(BUF_INF *pCMD)
{
	
	int iRet = -1;
	char tempbuff[128]={0};
	char value[12]={0};
    char vol[4]={0};
	strncpy(vol, &(pCMD->buf[sizeof(AT_VOL)-1]), 3);
	int volume = atoi(vol);
	volume+=1;
    switch (volume){
        case 1:strcpy(value,"one"); break;
        case 2:strcpy(value,"two"); break;
        case 3:strcpy(value,"three"); break;
        case 4:strcpy(value,"four"); break;
        case 5:strcpy(value,"five"); break;
        default:printf("error\n");
    }
	memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
	sprintf(tempbuff, "creatPlayList collplaylist  %s", value);
	iRet=my_system(tempbuff);  	
  	return iRet;		
}
static int Process_AT_KEY_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
	char tempbuff[128]={0};
	char value[12]={0};
    char vol[4]={0};
	strncpy(vol, &(pCMD->buf[sizeof(AT_VOL)-1]), 3);
	int volume = atoi(vol);
	volume+=1;
    switch (volume){
        case 1:strcpy(value,"one"); break;
        case 2:strcpy(value,"two"); break;
        case 3:strcpy(value,"three"); break;
        case 4:strcpy(value,"four"); break;
        case 5:strcpy(value,"five"); break;
        default:printf("error\n");
    }
	memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
	sprintf(tempbuff, "creatPlayList playcollplaylist  %s", value);
    iRet=my_system(tempbuff);  	
    return iRet;
}

static int Process_POWER_OFF_CMD(BUF_INF *pCMD)
{
	int iRet = -1;
#if defined(CONFIG_PACKAGE_iot)
    //iRet = msg_queue_send("PowerOff",MSG_UARTD_TO_IOT);
    cdb_save();
#endif
	iRet = OnSendCommandToAmazonAlexa("PowerOff");
	
    return iRet;
}

static int Process_WIFI_CMD( BUF_INF *cmd)
{
    printf("cmd = %s\n",cmd->buf);
	int iRet = -1;
#if  defined(CONFIG_PACKAGE_duer_linux)
	iRet = send_button_direction_to_dueros(1, KEY_ONE_LONG);
#else
	if(strncmp(cmd->buf, AT_WIFI_WPS, (sizeof(AT_WIFI_WPS)-1)) == 0)
	{

#if defined(CONFIG_PACKAGE_iot)
      	StartConnecting_led();
       // msg_queue_send("StartSetNetwork", MSG_UARTD_TO_IOT);	
          iRet = write_msg_to_Turingiot("StartSetNetwork");
#else
		send_cmd_to_server(1);
		iRet = Process_WIFI_WPS_CMD();
#endif        
	} 
	else if(strncmp(cmd->buf, AT_WIFI_CON, (sizeof(AT_WIFI_CON)-1)) == 0) 
	{
		iRet = Process_WIFI_CON_CMD();
	}
	else if(strncmp(cmd->buf, AT_WIFI_DIS, (sizeof(AT_WIFI_DIS)-1)) == 0)
	{
		iRet = Process_WIFI_DIS_CMD();
	}
#endif
	return iRet;
}
static int Process_VOL_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	if(strncmp(cmd->buf, AT_VOL_UP, (sizeof(AT_VOL_UP)-1)) == 0)		
	{
        iRet = send_button_direction_to_dueros(2, KEY_VOLUME_UP, 10);
//		iRet = Process_VOL_UP_CMD();
	} 
	else if(strncmp(cmd->buf, AT_VOL_DOWN, (sizeof(AT_VOL_DOWN)-1)) == 0) 
	{
		iRet = send_button_direction_to_dueros(2, KEY_VOLUME_DOWN, 10);
//		iRet = Process_VOL_DOWN_CMD();
	}
	else if((strncmp(cmd->buf, AT_VOL, (sizeof(AT_VOL)-1)) == 0) || (strncmp(cmd->buf, AT_VOL2, (sizeof(AT_VOL2)-1)) == 0))
	{
		iRet = Process_AT_VOL_CMD(cmd);
	}

	return iRet;
}

static int Process_DUE_CMD(BUF_INF *cmd)
{
	int i = 0;
	char *buf = cmd->buf;
	int len = cmd->len;
	
#if UART_DATA_CHECK	
	printf("uartdfifo receiving:");
	for(i = 0; i < len; i++){
		printf("%hhx ",buf[i]);
	};
	printf("\n");
#endif
	send_data_to_dueros(&buf[8], len - 9);
	return 0;
}

static int Process_LED_CMD(BUF_INF *cmd)
{
	int i = 0;
	char *buf = cmd->buf;
	int len = cmd->len;
    char led_cmd[4]={0};
	char led_buf[32] = "";
	strncpy(led_cmd, &buf[8], len - 9);	
#if 0	
	printf("uartdfifo receiving:");
	for(i = 0; i < len; i++){
		printf("%hhx ",buf[i]);
	};
	printf("\n");

	system("killall -9 pwmtest");
	DEBUG_INFO("Color:%s\n",led_cmd);
	sprintf(led_buf,"/usr/bin/pwmtest %s&",led_cmd);
	my_system(led_buf);
#endif	
#if defined(CONFIG_PACKAGE_iot)
		sprintf(led_buf,"SetColor%s",led_cmd);
        msg_queue_send(led_buf,MSG_UARTD_TO_IOT);
#endif	
	return 0;
}
static int Process_PLY_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	
//	OnSendCommandToAmazonAlexa("KeyEvent");
	
	if(strncmp(cmd->buf, AT_PLAY, (sizeof(AT_PLAY)-1)) == 0)
	{
//		send_cmd_to_server(BUTTON_PLAY_PAUSE_PRESSED);
#if defined(CONFIG_PACKAGE_duer_linux)
		iRet = send_button_direction_to_dueros(1, KEY_PLAY_PAUSE);
#elif defined(CONFIG_PACKAGE_iot)
        //iRet = msg_queue_send("PlayerPlayPause",MSG_UARTD_TO_IOT);
        iRet = write_msg_to_Turingiot("PlayerPlayPause");
#endif

//	    send_cmd_to_server(BUTTON_PLAY_PAUSE_PRESSED);		
//		iRet = Process_AT_PLAY_CMD();
	} 
	else if(strncmp(cmd->buf, AT_STOP, (sizeof(AT_STOP)-1)) == 0) 
	{
		iRet = send_button_direction_to_dueros(1, KEY_PLAY_PAUSE);
//		iRet = Process_AT_STOP_CMD();
	}
	
	else if((strncmp(cmd->buf, AT_NEXT, (sizeof(AT_NEXT)-1)) == 0))
	{
	
#if defined(CONFIG_PACKAGE_iot)
#if 0
		if(cdb_get_int("$turing_mpd_play",0) == 4)		//公众号请求下一首
		{
			//iRet = msg_queue_send("PlayerPlayNext",MSG_UARTD_TO_IOT);
			iRet = write_msg_to_Turingiot("PlayerPlayNext");
		}	
		else /*if(cdb_get_int("$turing_mpd_play",0) == 0)*/		//tf播放下一首
		{
			exec_cmd("mpc pause");
			exec_cmd("killall -9 aplay");
			exec_cmd("aplay -D common /root/iot/下一首.wav");	
			iRet = Process_AT_NEXT_CMD();			
		}
	/*	else if(cdb_get_int("$turing_mpd_play",0) == 1)	//语音点歌和TTS时，不需要
		{
			return ;
		}*/
#else
		iRet = write_msg_to_Turingiot("PlayerPlayNext");
#endif			
#endif
	}
	else if((strncmp(cmd->buf, AT_PREV, (sizeof(AT_PREV)-1)) == 0))
	{
		
#if defined(CONFIG_PACKAGE_iot)
#if 0
		if(cdb_get_int("$turing_mpd_play",0) == 4)				//公众号请求上一首
		{
			//iRet = msg_queue_send("PlayerPlayPrev",MSG_UARTD_TO_IOT);
			 iRet = write_msg_to_Turingiot("PlayerPlayPrev");

		}	
		else /* if(cdb_get_int("$turing_mpd_play",0) == 0)*/			//tf播放上一首
		{		
			exec_cmd("mpc pause");
			exec_cmd("killall -9 aplay");
			exec_cmd("aplay -D common /root/iot/上一首.wav");
			iRet = Process_AT_PREV_CMD();	
		}
	/*	else if(cdb_get_int("$turing_mpd_play",0) == 1)	//语音点歌和TTS时，不需要
		{
			return ;
		}*/
#else
		iRet = write_msg_to_Turingiot("PlayerPlayPrev");
#endif			
#endif

	}
	
	else if((strncmp(cmd->buf, AT_SW_WIFIMODE, (sizeof(AT_SW_WIFIMODE)-1)) == 0) )
	{
        send_cmd_to_server(BUTTON_MODE_SWITCH_PRESSED);
		iRet = Process_AT_SW_WIFIMODE_CMD();
	}
	else if((strncmp(cmd->buf, AT_SW_TFPLAY, (sizeof(AT_SW_WIFIMODE)-1)) == 0) )
	{
	
#if defined(CONFIG_PACKAGE_iot)
        //iRet = msg_queue_send("StartPlayTf",MSG_UARTD_TO_IOT);
         iRet = write_msg_to_Turingiot("StartPlayTf");
#endif
		iRet = Process_AT_SW_TFPLAY_CMD();
	}
	else if((strncmp(cmd->buf, AT_SW_USBPLAY, (sizeof(AT_SW_WIFIMODE)-1)) == 0) )
	{
	    send_cmd_to_server(BUTTON_MODE_SWITCH_PRESSED);
		iRet = Process_AT_SW_USBPLAY_CMD();
	}
	return iRet;
}

static int Process_PLP_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	
	// °′?ü°′??￡?2???ê?·??ú2￥·?alexaμ?ò?à?￡?Dèòa·￠?í°′?ü°′????á?μ?alexa,
	// òò?aóD?é?üalexa3ìDò?ú2￥·???á?
	OnSendCommandToAmazonAlexa("KeyEvent");
	
	if(strncmp(cmd->buf, AT_SW_USBPAUSE, (sizeof(AT_SW_USBPAUSE)-1)) == 0)
	{
		iRet = Process_AT_SW_USBPAUSE_CMD();
	} 
	else if(strncmp(cmd->buf, AT_SW_TFPAUSE, (sizeof(AT_SW_TFPAUSE)-1)) == 0) 
	{
		iRet = Process_AT_SW_TFPAUSE_CMD();
	}
	else if((strncmp(cmd->buf, AT_PLP, (sizeof(AT_PLP)-1)) == 0))
	{
		iRet = Process_AT_PLP_CMD(cmd);
	}
	
	return iRet;
}

static int Process_TLK_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	if(strncmp(cmd->buf, AT_TLK_ON, (sizeof(AT_TLK_ON)-1)) == 0)
	{
        //send_cmd_to_server(BUTTON_SPEECH_PRESSED);
		DEBUG_INFO("AT_TLK_ON ");

#if defined(CONFIG_PACKAGE_duer_linux)
		send_button_direction_to_dueros(1, KEY_WAKE_UP);
#else

#endif
	} 
	else if(strncmp(cmd->buf, AT_TLK_OFF, (sizeof(AT_TLK_OFF)-1)) == 0) 
	{
		iRet = 0;
	}
    else if(strncmp(cmd->buf, AT_TLK_WECHAT, (sizeof(AT_TLK_WECHAT)-1)) == 0) 
    {
#if defined(CONFIG_PACKAGE_iot)
       // iRet = msg_queue_send("WeChatStart",MSG_UARTD_TO_IOT);
       	  iRet = write_msg_to_Turingiot("WeChatStart");
#endif
    }
    else if(strncmp(cmd->buf, AT_TLK_MENU, (sizeof(AT_TLK_MENU)-1)) == 0)
    {
#if defined(CONFIG_PACKAGE_iot)
        //iRet = msg_queue_send("SwitchColumn",MSG_UARTD_TO_IOT);
       iRet = write_msg_to_Turingiot("SwitchColumn");
#endif
    }
    else if(strncmp(cmd->buf, AT_TLK_COLLECT, (sizeof(AT_TLK_COLLECT)-1)) == 0)
    {
#if defined(CONFIG_PACKAGE_iot)
       // iRet = msg_queue_send("CollectSong",MSG_UARTD_TO_IOT);
       iRet = write_msg_to_Turingiot("CollectSong");
#endif
    }
    else if(strncmp(cmd->buf, AT_TLK_TRAN, (sizeof(AT_TLK_TRAN)-1)) == 0)
    {
#if defined(CONFIG_PACKAGE_iot)
        //iRet = msg_queue_send("StartChinaEnglishTran",MSG_UARTD_TO_IOT);   
		iRet = write_msg_to_Turingiot("StartChinaEnglishTran");
#endif
    }
	return iRet;
}

static int Process_SSID_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	if(strncmp(cmd->buf, AT_SSID_ON, (sizeof(AT_SSID_ON)-1)) == 0)
	{
		iRet = 0;
	} 
	else if(strncmp(cmd->buf, AT_SSID_OFF, (sizeof(AT_SSID_OFF)-1)) == 0) 
	{
		iRet = 0;
	}
	return iRet;
}


static int Process_PLM_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	if(strncmp(cmd->buf, AT_SSID_ON, (sizeof(AT_SSID_ON)-1)) == 0)
	{
		iRet = 0;
	} 
	else if(strncmp(cmd->buf, AT_SSID_OFF, (sizeof(AT_SSID_OFF)-1)) == 0) 
	{
		iRet = 0;
	}
	return iRet;
}

static int Process_MAC_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	if(strncmp(cmd->buf, AT_MAC1, (sizeof(AT_MAC1)-1)) == 0)
	{
		iRet = Process_AT_MAC1_CMD(cmd);
	} 
	else if(strncmp(cmd->buf, AT_MAC2, (sizeof(AT_MAC2)-1)) == 0) 
	{
		iRet = Process_AT_MAC2_CMD(cmd);
	}
	return iRet;
}


static int Process_CHE_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	if(strncmp(cmd->buf, AT_USB_CHECK, (sizeof(AT_USB_CHECK)-1)) == 0)
	{
		iRet = Process_AT_USB_CHECT_CMD();
	} 
	else if(strncmp(cmd->buf, AT_TF_CHECK, (sizeof(AT_TF_CHECK)-1)) == 0) 
	{
		iRet = Process_AT_TF_CHECK_CMD();
	}
	return iRet;
}

static int Process_COL_CMD( BUF_INF *cmd)
{
	int iRet = -1;
	if(strncmp(cmd->buf, AT_COL, (sizeof(AT_COL)-1)) == 0)
	{
		iRet = Process_AT_COL_CMD(cmd);
	} 
	else if(strncmp(cmd->buf, AT_COL_DEL, (sizeof(AT_COL_DEL)-1)) == 0) 
	{
		iRet = Process_AT_COL_DEL_CMD();
	}
	return iRet;
}

static int Process_AT_BATTERY(BUF_INF *cmd)
{
	char bat[4] = {0};
	strncpy(bat, &(cmd->buf[sizeof(AT_BAT)-1]), (sizeof(bat)-1));
	DEBUG_INFO("AT_BAT");
//	DEBUG_INFO("bat:%s",bat);
	//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.power", bat);
	cdb_set_int("$battery_level",atoi(bat));
	err("$battery_level:%s",bat);
	int Bat_Level = strtol(bat,NULL,0);
	int battery_first_state = cdb_get_int("$battery_first_state",0);	
	err("battery_first_state111 is :%d",battery_first_state);
	
#if defined(CONFIG_PACKAGE_iot)	
	if(/*(Bat_Level <= 2)&&*/(battery_first_state == 0))	
		//msg_queue_send("BatteryLevel", MSG_UARTD_TO_IOT);
		write_msg_to_Turingiot("BatteryLevel");
#endif

	return 0;
}

static int Process_AT_CHARGE(BUF_INF *cmd)
{
	char chg[4] = {0};
	strncpy(chg, &(cmd->buf[sizeof(AT_CHG)-1]), (sizeof(chg)-1));
	DEBUG_INFO("AT_CHG");
//	DEBUG_INFO("chg:%s",chg);
	cdb_set("$charge_plug",chg);
	err("$charge_plug:%s",chg); 
	int battery_first_state = cdb_get_int("$battery_first_state",0);
	err("battery_first_state222 is :%d",battery_first_state);
	
#if defined(CONFIG_PACKAGE_iot)	
	if(/*!strcmp(chg,"001") && */(battery_first_state == 0))
		//msg_queue_send("BatteryCharge", MSG_UARTD_TO_IOT);
		write_msg_to_Turingiot("BatteryCharge");
#endif

	return 0;
}
#define BT_INFO    "/tmp/BT_INFO"    //éè±?áD±í
#define BT_STATUS  "/tmp/BT_STATUS"  //à??à×′ì?
static int Process_AT_IN(BUF_INF *cmd)
{
	int iRet = -1;
	int index;
	char run[128] = {0};
	DEBUG_INFO("echo BT info.............");
	my_system("[ ! -f "BT_INFO" ] && touch "BT_INFO" && echo -e '\n\n\n\n\n\n\n' > "BT_INFO);
	index = cmd->buf[sizeof(AT_IN)] - '0' + 1;
	if (index < 1 || index > 7)
	{
		DEBUG_INFO("invalid index %d", index);
		return -1;
	}
	if (cmd->buf[sizeof(AT_IN)-1] == '1')
	{
		snprintf(run, sizeof(run), "sed -i '/^1/s/1/0/' "BT_INFO);
		DEBUG_INFO("%s",run);
		my_system(run);
		cdb_set_int("$btconnectstatus",1);
		//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.btconnectstatus", "1");
		my_system("echo 1 > "BT_STATUS);
		my_system("killall btstatus.sh");
	}
	snprintf(run, sizeof(run), "sed -i '%dc %s' "BT_INFO, index, &(cmd->buf[sizeof(AT_IN)-1]));
	DEBUG_INFO("%s",run);
	my_system(run);
	return 0;
}

static int Process_AT_BTS_SLP(BUF_INF *cmd)
{
	char run[128] = {0};
	snprintf(run, sizeof(run), "sed -i '/^1/s/1/0/' "BT_INFO);
	DEBUG_INFO("%s",run);
	my_system(run);
	cdb_set_int("$btconnectstatus",0);
	//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.btconnectstatus", "0");
	return 0;
}

static int Process_AT_BCN_FAL()
{
	my_system("echo 0 > "BT_STATUS);
	my_system("killall btstatus.sh");
	return 0;
}

static int Process_AT_BCN_SUC()
{
    cdb_set_int("$btconnectstatus",1);
	//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.btconnectstatus", "1");
	my_system("echo 1 > "BT_STATUS);
	my_system("killall btstatus.sh");
	return 0;
}

static int Process_AT_BCN_ING()
{
	my_system("echo 2 > "BT_STATUS);
	my_system("killall btstatus.sh;btstatus.sh&");
	return 0;
}

static int Process_AT_BCN(BUF_INF *cmd)
{
	if(strncmp(cmd->buf, AT_BCN_FAL,(sizeof(AT_BCN_FAL)-1)) == 0)
	{
		Process_AT_BCN_FAL();
	}
	else if(strncmp(cmd->buf, AT_BCN_SUC,(sizeof(AT_BCN_SUC)-1)) == 0)
	{
		Process_AT_BCN_SUC();
	}
	else if(strncmp(cmd->buf, AT_BCN_ING,(sizeof(AT_BCN_ING)-1)) == 0)
	{
		Process_AT_BCN_ING();
	}
	return 0;
}

static int Process_AT_BSC_ING()
{
	my_system("echo 3 > "BT_STATUS);
	my_system("rm "BT_INFO);
	my_system("killall btstatus.sh;btstatus.sh&");
	return 0;
}

static int Process_AT_BSC_END()
{
	my_system("echo 4 > "BT_STATUS);
	my_system("killall btstatus.sh");
	return 0;
}

static int Process_AT_BSC(BUF_INF *cmd)
{
	if(strncmp(cmd->buf, AT_BSC_ING,(sizeof(AT_BSC_ING)-1)) == 0)
	{
		Process_AT_BSC_ING();
	}
	else if(strncmp(cmd->buf, AT_BSC_END,(sizeof(AT_BSC_END)-1)) == 0)
	{
		Process_AT_BSC_END();
	}
	return 0;
}

static int Process_MIC_MUT(BUF_INF *cmd)
{
    int iRet = 0;
#if defined(CONFIG_PACKAGE_duer_linux)  
	iRet = send_button_direction_to_dueros(1, KEY_MIC_MUTE);
#elif defined(CONFIG_PACKAGE_iot)
  	// msg_queue_send("MicOff", MSG_UARTD_TO_IOT);
  		write_msg_to_Turingiot("MicOff");
#else
    
	// mute on or mute off is the same button
    if (NO_TEST_MODE == send_cmd_to_server(BUTTON_MUTE_PRESSED))
	{
	
		// 静音，只是播放提示音
		system("wavplayer /root/res/Mute.wav");
		//cdb_set("$wakeup_nocheck", "1");
		//my_system("killall -9 wakeup &");
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+MIC+MUT&\r\n", sizeof("AXX+MIC+MUT&\r\n"));
		pthread_mutex_unlock(&pmMUT);
		OnSendCommandToAmazonAlexa("Mute");
	}
#endif
	return iRet;
}

static int Process_MIC_UNM(BUF_INF *cmd)
{
    int iRet = -1;
#if defined(CONFIG_PACKAGE_duer_linux)  
    iRet = send_button_direction_to_dueros(1, KEY_MIC_MUTE);
#elif defined(CONFIG_PACKAGE_iot)
   // iRet = msg_queue_send("MicOn", MSG_UARTD_TO_IOT);
   	  iRet = write_msg_to_Turingiot("MicOn");
#else

	// mute on or mute off is the same button
    if (NO_TEST_MODE == send_cmd_to_server(BUTTON_MUTE_PRESSED))
	{
		// 取消静音，只是播放提示音
		//cdb_set("$wakeup_nocheck", "0");
		system("wavplayer /root/res/UnMute.wav");
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+MIC+UMT&\r\n", sizeof("AXX+MIC+UMT&\r\n"));
		pthread_mutex_unlock(&pmMUT);
		OnSendCommandToAmazonAlexa("UnMute");
	}
#endif
	return iRet;
}

// 收到按键停止闹钟
static int Process_Stop_Time(BUF_INF *cmd)
{
	int iRet = OnSendCommandToAmazonAlexa("KeyStopAler");
	return iRet;
}

// 收到获取闹钟状态
static int Process_Get_Time(BUF_INF *cmd)
{
	int iRet = OnSendCommandToAmazonAlexa("GetTimerStatus");
	return iRet;
}

int Factory_testmode_volume_event(BUF_INF pCmd)
{
	int iRet = -1;
	char run[128] = {0};
	char vol[4] = {0};
	if(pCmd.len < 11){
		return -1;
	}
	strncpy(vol, &(pCmd.buf[8]), 3);	
//    if(0 == access("/tmp/test_mode",0))
    {
		 int pVolume = cdb_get_int("$ra_vol", 0);
		if(pVolume - atoi(vol) < 0)
	        send_cmd_to_server(BUTTON_VOLUME_UP_PRESSED);
	    else
	        send_cmd_to_server(BUTTON_VOLUME_DOWN_PRESSED);
    }
}
void Factory_testmode_playmode_event(BUF_INF *pCmd)
{
	char playmode[4] = {0};//MCU+PLM+001&
	if(pCmd->len < 11){
		return -1;
	}
	strncpy(playmode,&(pCmd->buf[8]),3);
	if(!strcmp(playmode,"000"))
	{
		send_cmd_to_server(BUTTON_MODE_SWITCH_PRESSED);
	}
	else
	{
		send_cmd_to_server(BUTTON_MODE_SWITCH_PRESSED);
	}

}

void Factory_testmode(BUF_INF cmd)
{
//		BUF_INF cmd = pcmd;
		if(!strcmp(AT_WIFI_WPS,cmd.buf)) 
			send_cmd_to_server(BUTTON_WPS_PRESSED);
		else if(!strcmp(AT_PLAY,cmd.buf))
			send_cmd_to_server(BUTTON_PLAY_PAUSE_PRESSED);
		else if(!strcmp(AT_NEXT,cmd.buf))
			send_cmd_to_server(BUTTON_NEXT_PRESSED);
		else if(!strcmp(AT_PREV,cmd.buf))
			send_cmd_to_server(BUTTON_PREV_PRESSED);
		else if(!strncmp(AT_VOL,cmd.buf,strlen(AT_VOL)))
			Factory_testmode_volume_event(cmd);
		else if(!strncmp(AT_TLK_WECHAT,cmd.buf,strlen(AT_TLK_WECHAT)))
			send_cmd_to_server(BUTTON_TURING_CHAT_START_PRESSED);
		else if(!strcmp(AT_TLK_MENU,cmd.buf))
			send_cmd_to_server(BUTTON_TURING_COLUMN_PRESSED);
		else if(!strcmp(AT_TLK_COLLECT,cmd.buf))
			send_cmd_to_server(BUTTON_COLLECTION_PRESSED);
		else if(!strcmp(AT_TLK_TRAN,cmd.buf))
			send_cmd_to_server(BUTTON_MCU1_PRESSED);
		else if(!strcmp(AT_FACTORY,cmd.buf))
			send_cmd_to_server(BUTTON_MCU2_PRESSED);
		else if(!strncmp(AT_PLAYMODE,cmd.buf,strlen(AT_PLAYMODE)))
			send_cmd_to_server(BUTTON_MODE_SWITCH_PRESSED);
		else if(!strcmp(AT_MUT_MIC,cmd.buf))
			send_cmd_to_server(BUTTON_MUTE_PRESSED);
		else if(!strcmp(AT_UNM_MIC,cmd.buf))
			send_cmd_to_server(BUTTON_MUTE_PRESSED);
}
int uartd_handler(char *ins, int len )
{
	int ret = -1;
	int montage = -1;
	char *buf = NULL;
    int i = 0;
    
	if (!ins){
		return -1;
	}

	BUF_INF cmd;
	cmd.buf = ins;
	cmd.len = len;

	DEBUG_INFO("len: %d, cmd:%s", cmd.len , cmd.buf);
	err("bt len: %d, bt cmd:%s", cmd.len , cmd.buf);
	if(access("/tmp/test_mode",0) == 0)
    {
		Factory_testmode(cmd);	
	}
	else if(access("/tmp/test_mode",0) == -1)
	{
	    //正在配网时，所有按键都不响应
	    if(cdb_get_int("$sinvoice_start",0) || cdb_get_int("$airkiss_start",0))
	    {
	        if(!strcmp(cmd.buf,AT_WIFI_WPS))
	        {
	            wav_play2("/tmp/res/wait_for_network_finish.wav");
	        }
	        return 0;
	    }
	    
	    //上一任务未完成，不响应下一次的按键
	    if(cdb_get_int("$iot_socket_finished",3) == 0 || cdb_get_int("$iot_turingkey_finished",3) == 0)  //正在处理
	    {
	        printf("\033[31mWait until the last task finished\033[0m");
	        return 0;
	    }
		int tbl_len = sizeof(cmd_tbl) / sizeof(uartd_proc_t);
		for(i = 0; i < tbl_len; i++) 
		{
			if(strncmp(cmd.buf, cmd_tbl[i].name, strlen(cmd_tbl[i].name)) == 0) 
			{			
				if(cmd_tbl[i].exec_proc != NULL) 
	            {
					montage = cmd_tbl[i].exec_proc(&cmd);
					if(montage){
						return-1;
					}
				}
				break;
			}
			ret=0;
		}
		if (ret == 0)
		{
			/*
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "OK&\r\n",strlen("OK&\r\n"));
			pthread_mutex_unlock(&pmMUT);
			*/
		}
		else
		{	/*	
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "ERR&\r\n",strlen("ERR&\r\n"));
			pthread_mutex_unlock(&pmMUT);
			*/
			DEBUG_ERR("uartd_proc err");
		}
	}
	return ret;
}

