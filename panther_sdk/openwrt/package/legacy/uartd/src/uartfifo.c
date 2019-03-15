#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include <pthread.h>
#include <sys/time.h>  
#include "uciConfig.h"
#include <sys/types.h>
#include "debug.h"
#include "montage.h"
#include <wdk/cdb.h>
#include <libcchip/platform.h>
#include "uartfifo.h"
#include <libcchip/function/fifobuffer/fifobuffer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mon_config.h"

extern void SpeechSelect(void);
extern int iUartfd;
extern int record;
extern pthread_mutex_t pmMUT;

#define BUFF_LENGTH 1024
#define FIFO_FILE "/tmp/UartFIFO"
#define PROGRESS_DIR "/tmp/progress"

#define IFLYTEK			"iflytek"

#define WIFI_CONFIG			"config"		//dueros：进入配网模式
#define TLKON				"tlkon"			//dueros：按键唤醒
#define PLAY				"play"			//dueros：播放
#define PAUSE				"pause"			//dueros：暂停
#define DU_LED 				"led"			//dueros：灯效显示命令头
#define DU_BT_HEAD			"bluetooth"		//dueros：蓝牙传送数据给dueros的命令头
#define DU_BLE_TEST			"blenet_bt"		//dueros：测试命令，uartd传送给dueros的命令识别测试
#define DU_DU_HEAD			"blenet_du"		//dueros：dueros传送数据给蓝牙的命令头
#define TESTMODE			"testmode"		//测试GPIO、IIS、USB、TF卡


#define GETVER				"getver"
#define CAPGET				"capget"
#define SNET_CONNECT_OK		"snetok"
#define SNET_CONNECT_FAILED	"snetfail"
#define START_BOOT_MUSIC    "bootmusic"
#define NET_CONNECT_OK		"netok"
#define NET_CONNECT_FAILED	"netfail"
#define USB_TEST_OK			"usbok"
#define USB_TEST_FAILED		"usbfail"
#define SD_TEST_OK			"sdok"
#define SD_TEST_FAILED		"sdfail"
#define STARTTEST			"starttest"
#define WIFI_CONNECTING		"connecting"
#define WIFI_SUCCEED		"succeed"
#define WIFI_FAILED			"failed"
#define POWEROFF			"poweroff"
#define GETVOL				"getvol"
#define SETVOL				"setvol"
#define GETCAP				"getcap"
#define GETBAT				"getbat"
#define GETPLM				"getplm"
#define SETPLM				"setplm"
#define TLKOFF				"tlkoff"
#define TESTTLKON			"testtlkon"
#define TESTTLKOFF			"testtlkoff"
#define TLKONTIME			"timetlk"
#define PLP					"plp"
#define CHANNEL				"channel"
#define MUTE				"mute"
#define TIME				"settime"
#define ALARM				"setalarm"
#define DELETEALARM			"deletealarm"
#define SEARCHSTART			"searchstart"
#define SEARCHSUCCEED		"searchsucceed"
#define SEARCHFAILED		"searchfailed"
#define UPBT				"upbt"
#define UPWIFI				"upwifi"
#define TFINT				"tfint"
#define TFOUT				"tfout"
#define TFONL				"tfonl"
#define TFOUL				"tfoul"
#define USBINT				"usbint"
#define USBOUT				"usbout"
#define USBONL				"usbonl"
#define USBOUL				"usboul"
#define INWIFIMODE 			"inwifimode"
#define INTFMODE 			"intfmode"
#define INUSBMODE 			"inusbmode"
#define WIFI_BACK_FAILE     "backfailed"
#define DISPOWER			"dispower"
#define DUER_LED_SET_MATCH	"duerSceneLedSet" //百度灯效设置

#define VAD_THRESHOLD_ON 				"vadthreshold_on"
#define VAD_THRESHOLD_OFF 				"vadthreshold_off"

#define ALEXA_LOGIN_SUCCED				"LogInSucceed"		// 登录成功
#define ALEXA_WAKE_UP 					"WakeUp"			// 远场唤醒
#define ALEXA_CAPTURE_START 			"CaptureStart"		// 录音开始
#define ALEXA_CAPTURE_END				"CaptureEnd"		// 录音结束
#define ALEXA_IDENTIFY_OK				"Identify_OK"		// 识别成功
#define ALEXA_IDENTIFY_ERR				"Identify_ERR"		// 识别失败
#define ALEXA_VOICE_PLAY_END			"VoicePlaybackEnd" 	// 播放回复语音完成
#define VOICE_INTERACTION				"VoiceInteraction"	// 语音交互模式切换
#define ALEXA_ALER_START        		"AlexaAlerStart"	// 闹钟开启
#define ALEXA_ALER_END	        		"AlexaAlerEnd"		// 闹钟停止
#define ALEXA_ALER_END_IDENTIFY_START	"AlerEndIdentifyOk"	// 闹钟停止，继续播放回复语音

#define ALEXA_NOTIFICATIONS_START		"NotificationsStart"
#define ALEXA_NOTIFACATIONS_END			"NotificationsEnd"

#define WIFI_NOT_CONNECTED				"WiFiNotConnected"
#define ALEXA_NOT_LOGIN					"AlexaNotLogin"

#define BT_SEARCH 						"btsearch"    //搜索周围设备
#define BT_IN 							"btin"        //蓝牙配对
#define BT_CON 							"btcon"       //连接
#define BT_DIS 							"btdiscon"    //断开
#define BT_LINK							"btlink"      //查询当前连接设备

char *LED_CMD[] = {
	"AXX+LED+001&", 	// LED_NET_RECOVERY
	"AXX+LED+002&", 	// LED_NET_WAIT_CONNECT
	"AXX+LED+003&", 	// LED_NET_DO_CONNECT
	"AXX+NET+FAL&", 	// LED_NET_CONNECT_FAILED
	"AXX+NET++OK&", 	// LED_NET_CONNECT_SUCCESS
	"AXX+LED+006&", 	// LED_NET_WAIT_LOGIN
	"AXX+LED+007&", 	// LED_NET_DO_LOGIN
	"AXX+LED+008&", 	// LED_NET_LOGIN_FAILED
	"AXX+LED+009&", 	// LED_NET_LOGIN_SUCCESS
	"AXX+LED+010&", 	// LED_WAKE_UP_DOA	  
	"AXX+TLK++ON&", 	// LED_WAKE_UP		 
	"AXX+LED+012&", 	// LED_SPEECH_PARSE
	"AXX+LED+013&", 	// LED_PLAY_TTS
	"AXX+LED+014&", 	// LED_PLAY_RESOURCE
	"AXX+LED+015&", 	// LED_BT_WAIT_PAIR
	"AXX+LED+016&", 	// LED_BT_DO_PAIR
	"AXX+LED+017&", 	// LED_BT_PAIR_FAILED
	"AXX+LED+000",		// LED_BT_PAIR_SUCCESS
	"AXX+LED+000",		// LED_BT_PLAY
	"AXX+LED+000",		// LED_BT_CLOSE
	"AXX+LED+000",		// LED_VOLUME
	"AXX+LED+000",		// LED_MUTE
	"AXX+LED+000",		// LED_DISABLE_MIC
	"AXX+LED+000",		// LED_ALARM
	"AXX+LED+000",		// LED_SLEEP_MODE
	"AXX+LED+000",		// LED_OTA_DOING
	"AXX+LED+000",		// LED_OTA_SUCCESS
	"AXX+LED+000",		// LED_CLOSE_A_LAYER
	"AXX+LED+000"		// LED_ALL_OFF	  
};



#define UNIX_DOMAIN "/tmp/UNIX.baidu"
typedef struct DIRECTION_HANDLER{
	const char *direction;
	void (*handler)(void *argv);
}DIRECTION_HANDLER;

/*
typedef struct{
	char *buf;
	unsigned int len;
}BUF_INF;
*/

typedef struct DUEROS_SENDDATA_FLAG{
	pthread_mutex_t rwlock;
	char flag;
}DUEROS_SENDDATA_FLAG;

DUEROS_SENDDATA_FLAG duer_send_flag;


//pthread_mutex_t line_lock = PTHREAD_MUTEX_INITIALIZER;


#define SEPERATER 0x44557766

FT_FIFO * du_bt_fifo = NULL;

enum LedState {
    LED_NET_RECOVERY = 0,	//恢复
    LED_NET_WAIT_CONNECT,	//联网中
    LED_NET_DO_CONNECT,		//联网完成
    LED_NET_CONNECT_FAILED,	//联网失败
    LED_NET_CONNECT_SUCCESS,//联网成功
    LED_NET_WAIT_LOGIN,		//
    LED_NET_DO_LOGIN,
    LED_NET_LOGIN_FAILED,
    LED_NET_LOGIN_SUCCESS,

    LED_WAKE_UP_DOA,    // param -180 ~ 180
    LED_WAKE_UP,        // no param
    LED_SPEECH_PARSE,
    LED_PLAY_TTS,
    LED_PLAY_RESOURCE,

	//蓝牙灯效
    LED_BT_WAIT_PAIR,	
    LED_BT_DO_PAIR,
    LED_BT_PAIR_FAILED,
    LED_BT_PAIR_SUCCESS,
    LED_BT_PLAY,
    LED_BT_CLOSE,

    LED_VOLUME,
    LED_MUTE,

    LED_DISABLE_MIC,

    LED_ALARM,

    LED_SLEEP_MODE,

    LED_OTA_DOING,
    LED_OTA_SUCCESS,

    LED_CLOSE_A_LAYER,
    LED_ALL_OFF,    // no param
};

enum BtControl {
    BT_OPEN,
    BT_CLOSE,
    BT_IS_OPENED,
    BT_IS_CONNECTED,
    BT_UNPAIR,
    BT_PAUSE_PLAY,
    BT_RESUME_PLAY,
    BT_AVRCP_FWD,
    BT_AVRCP_BWD,
    BT_AVRCP_STOP,
    BT_HFP_RECORD,
    BLE_OPEN_SERVER,
    BLE_CLOSE_SERVER,
    BLE_SERVER_SEND,
    BLE_IS_OPENED,
    GET_BT_MAC,
    GET_WIFI_MAC,
    GET_WIFI_IP,
    GET_WIFI_SSID,
    GET_WIFI_BSSID
};

static char *BySeadBuf = NULL;

led_proc_handler turing_leds_tbl[] = {
	{TURING_CONNECT_ING,	turing_leds_connecting},
	{TURING_CONNECT_OK,		turing_leds_connect_ok},
	{TURING_CONNECT_FAL,	turing_leds_connect_fal},
	{TURING_TLK_ON,			turing_leds_tlk_on},
	{TURING_TLK_PLAYING,	turing_leds_tlk_playing},
	{TURING_FACTORY,		turing_leds_factory},
	{TURING_DEFAULT,		turing_leds_default},
	{TURING_UPGRADE,		turing_leds_upgrade},
};


void turing_leds_connecting(char *byReadBuf)
{
	printf("---------------wifi connecting ----------------\n");
#if defined(CONFIG_PROJECT_K2_V1)
			BySeadBuf = "AXX+STA+002";		
#elif defined(CONFIG_PROJECT_DOSS_1MIC_V1)
			BySeadBuf = "AXX+LED+011";
#endif
		write(iUartfd, BySeadBuf, strlen(BySeadBuf));
		return 0;
}

void turing_leds_connect_fal(char *byReadBuf)
{
	printf("---------------wifi connectfal ----------------\n");
#if   defined (CONFIG_PROJECT_K2_V1)
		BySeadBuf = "AXX+STA+000";	
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		BySeadBuf = "AXX+LED+004";								
#endif
		write(iUartfd, BySeadBuf, strlen(BySeadBuf));
		return 0;
}

void turing_leds_connect_ok(char *byReadBuf)
{
	printf("---------------wifi connectok ----------------\n");
#if   defined (CONFIG_PROJECT_K2_V1)
			BySeadBuf = "AXX+STA+001";		
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
			BySeadBuf = "AXX+LED+007";				
#endif
			write(iUartfd, BySeadBuf, strlen(BySeadBuf));
			return 0;
}

void turing_leds_tlk_on(char *byReadBuf)
{
	printf("--------------- tlk on ----------------\n");
#if   defined (CONFIG_PROJECT_K2_V1)
		BySeadBuf = "AXX+TLK++ON&"; 
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		BySeadBuf = "AXX+LED+012";												
#endif
		write(iUartfd, BySeadBuf, strlen(BySeadBuf));
		return 0;
}

void turing_leds_tlk_playing(char *byReadBuf)
{
	printf("---------------tlk play ----------------\n");
#if   defined (CONFIG_PROJECT_K2_V1)
		BySeadBuf = "AXX+TLK+PLAY&";
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		BySeadBuf = "AXX+LED+003";																
#endif
		write(iUartfd, BySeadBuf, strlen(BySeadBuf));
		return 0;
}

void turing_leds_factory(char *byReadBuf)
{
	printf("---------------factory ----------------\n");
#if   defined (CONFIG_PROJECT_K2_V1)			
		BySeadBuf = "AXX+FACTORY";		
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		BySeadBuf = "AXX+LED+009";			//用的wifi升级的灯效，后期可以更换																			
#endif
		write(iUartfd, BySeadBuf, strlen(BySeadBuf));
		return 0;
}

void turing_leds_default(char *byReadBuf)
{
	printf("--------------- default ----------------\n");
#if   defined (CONFIG_PROJECT_K2_V1)	
		BySeadBuf = "AXX+PWR++ON";		
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		BySeadBuf = "AXX+LED+007";																								
#endif
		write(iUartfd, BySeadBuf, strlen(BySeadBuf));
		return 0;
}

void turing_leds_upgrade(char *byReadBuf)
{
	printf("---------------upgrade----------------\n");
#if   defined (CONFIG_PROJECT_K2_V1)	
		BySeadBuf = "AXX+STA+002";
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
		BySeadBuf = "AXX+LED+009";	
#endif
		write(iUartfd, BySeadBuf, strlen(BySeadBuf));
		return 0;
}




DIRECTION_HANDLER uartdfifo_directive[] = {
	{WIFI_CONFIG, uartdfifo_config_handler},
	{TLKON, uartdfifo_tlkon_handler},
	{PLAY, uartdfifo_play_pause_handler},
	{PAUSE, uartdfifo_play_pause_handler},
	{DU_LED, uartdfifo_led_handler},
	{DU_BT_HEAD, uartdfifo_bluetooth_handler},
	{DU_BLE_TEST, uartdfifo_blenet_bt_handler},
	{DU_DU_HEAD, uartdfifo_blenet_du_handler},
#if 0
	{TESTMODE, uartdfifo_testmode_handler}
	{"WakeUp"},
	{"getver"},
	{"capget"},
	{"snetok"},
	{"snetfail"},
	{"bootmusic"},
	{"netok"},
	{"netfail"},
	{"usbok"},
	{"usbfail"},
	{"sdok"},
	{"sdfail"},
	{"starttest"},
	{"connecting"},
	{"succeed"},
	{"failed"},
	{"poweroff"},
	{"getvol"},
	{"setvol"},
	{"getcap"},
	{"getbat"},
	{"getplm"},
	{"setplm"},
	{"tlkoff"},
	{"testtlkon"},
	{"testtlkoff"},
	{"timetlk"},
	{"plp"},
	{"channel"},
	{"mute"},
	{"settime"},
	{"setalarm"},
	{"deletealarm"},
	{"searchstart"},
	{"searchsucceed"},
	{"searchfailed"},
	{"upbt"},
	{"upwifi"},
	{"tfint"},
	{"tfout"},
	{"tfonl"},
	{"tfoul"},
	{"usbint"},
	{"usbout"},
	{"usbonl"},
	{"usboul"},
	{"inwifimode"},
	{"intfmode"},
	{"inusbmode"},
	{"backfailed"},
	{"dispower"},
	{"vadthreshold_on"},
	{"vadthreshold_off"},
	{"LogInSucceed"},
	{"CaptureStart"},
	{"CaptureEnd"},
	{"Identify_OK"},
	{"Identify_ERR"},
	{"VoicePlaybackEnd"},
	{"VoiceInteraction"},
	{"AlexaAlerStart"},
	{"AlexaAlerEnd"},
	{"AlerEndIdentifyOk"},
	{"NotificationsStart"},
	{"NotificationsEnd"},
	{"WiFiNotConnected"},
	{"AlexaNotLogin"},
	{"btsearch" },
	{"btin"     },
	{"btcon"    },
	{"btdiscon" },
	{"btlink"   }
#endif
};


DIRECTION_HANDLER at_direction[] = {
#if 0
	{"AXX+POW+ON1&",NULL},
	{"AXX+STARTTEST&"},
	{"AXX+STA+002&"},
	{"AXX+STA+003&"},
	{"AXX+STA+001&"},
	{"AXX+STA+000&"},
	{"AXX+POW+OFF&"},
	{"AXX+VOL+GET&"},
	{"AXX+VOL+000&"},
	{"AXX+PLP+000&"},
	{"AXX+PLY+000&"},
	{"AXX+PLM+GET&"},
	{"AXX+PLM+000&"},
	{"AXX+SEQ+000&"},
	{"AXX+CAP+GET&"},
	{"AXX+BAT+GET&"},
	{"AXX+TLK++ON&"},
	{"AXX+TLK+OFF&"},
	{"AXX+CHN+000&"},
	{"AXX+MUT+000&"},
	{"AXX+ALR+0000087f&"},
	{"AXX+ALR+C00&"},
	{"AXX+TIM+00000821010616&"},
	{"AXX+SEA+STA&"},
	{"AXX+SEA+STC&"},
	{"AXX+SEA+STO&"},
	{"AXX+TIM+STA&"},
	{"AXX+TIM+END&"},
	{"AXX+TIM+PAS&"},
	{"AXX+NTF+STA&"},
	{"AXX+NTF+END&"},
	{"AXX+RA0+NOT&"},
	{"AXX+TF++INT&"},
	{"AXX+TF++OUT&"},
	{"AXX+TF++ONL&"},
	{"AXX+TF++OUL&"},
	{"AXX+USB+INT&"},
	{"AXX+USB+OUT&"},
	{"AXX+USB+ONL&"},
	{"AXX+USB+OUL&"},
	{"AXX+PLM+WIF&"},
	{"AXX+PLM++TF&"},
	{"AXX+PLM+USB&"},
	{"AXX+KEY+BAT&"},
	{"AXX+NET++OK&"},	
	{"AXX+NET+FAL&"},
	{"AXX+USB++OK&"},
	{"AXX+USB+FAL&"},
	{"AXX+SDC++OK&"},
	{"AXX+SDC+FAL&"},
	{"SXX+NET++OK&"},
	{"SXX+NET+FAL&"},
	{"BOOTMUSICSTART&"},
	{"AXX+MCU+VER&"},
	{"AXX+SEA+LDI&"},
	{"AXX+SEA+WEK&"},
	{"AXX+SEA+SUC&"},
	{"AXX+SEA+FAL&"},
	{"AXX+SEA+IDL&"},
	{"AXX+SEA+LIS&"},
	{"AXX+SEA+RED&"},
	{"AXX+CAP+GET&"},
	{"AXX+UP+DATE&"},
	{"AXX+NET+ERR&"},
	{"AXX+ALE+ERR&"},
	{"AXX+UP+WIFI&"},
	{"CMD+EMIT_INQ&"},
	{"CMD+IN01+888888888888&"},
	{"CMD+EMIT_CON&"},
	{"CMD+EMIT_DISCON&"},
	{"CMD+EMIT_LINK&"}
#endif
};

int du_bt_fifo_init()
{
	if(du_bt_fifo == NULL){
		du_bt_fifo = ft_fifo_alloc(1024);	
		if(NULL == du_bt_fifo)
			return -1;
		ft_fifo_clear(du_bt_fifo);
	}
	return 0;
}

int is_du_bt_fifo_empty()
{
	if(du_bt_fifo->in == du_bt_fifo->out){
		return 1;
	}else{
		return 0;
	}
}

int du_bt_fifo_pop(char *buf, unsigned int maxlen)
{
	if(du_bt_fifo->in == du_bt_fifo->out){
		DEBUG_INFO("du_bt_fifo is empty");
		return;
	}

	char *head = DU_DU_HEAD;
	int s = SEPERATER;
	char *sep = (char *)&s;
	int sep_flag = 0;
	int head_flag = 0;
	
	int i = 0;
	pthread_mutex_lock(&du_bt_fifo->lock);
	for(;i < maxlen; i++){		
//		(!((du_bt_fifo->buffer[du_bt_fifo->out] == sep[0]) && (du_bt_fifo->buffer[du_bt_fifo->out+1] == sep[1]) &&
//			(du_bt_fifo->buffer[du_bt_fifo->out+2] == sep[2]) && (du_bt_fifo->buffer[du_bt_fifo->out+3] == sep[3])))
		if((du_bt_fifo->in != du_bt_fifo->out)){
/*			if((du_bt_fifo->buffer[du_bt_fifo->out] == 'b') &&(du_bt_fifo->buffer[du_bt_fifo->out+1] == 'l')&&(du_bt_fifo->buffer[du_bt_fifo->out+2] == 'e')&&
				(du_bt_fifo->buffer[du_bt_fifo->out+3] == 'n')&&(du_bt_fifo->buffer[du_bt_fifo->out+4] == 'e')&&(du_bt_fifo->buffer[du_bt_fifo->out+5] == 't')&&
				(du_bt_fifo->buffer[du_bt_fifo->out+6] == '_')&&(du_bt_fifo->buffer[du_bt_fifo->out+7] == 'd')&&(du_bt_fifo->buffer[du_bt_fifo->out+8] == 'u')){
				du_bt_fifo->out += 8;
				DEBUG_INFO("blenet_du is put in fifo");
				continue;
			}
*/
			if(du_bt_fifo->buffer[du_bt_fifo->out] == head[head_flag]){
				head_flag++;
			}else if((head_flag != 0 ) && (du_bt_fifo->buffer[du_bt_fifo->out] == head[0])){
				head_flag = 1;
			}else{
				head_flag = 0;
			}
				
			if(du_bt_fifo->buffer[du_bt_fifo->out] == sep[sep_flag]){
				sep_flag++;
			}else if((sep_flag != 0 ) && (du_bt_fifo->buffer[du_bt_fifo->out] == sep[0])){
				sep_flag = 1;
			}else{
				sep_flag = 0;
			}
			buf[i] = du_bt_fifo->buffer[du_bt_fifo->out]; 		//根据分隔符的位置确定发送数据量
//			du_bt_fifo->buffer[du_bt_fifo->out] = 0;
			du_bt_fifo->out++;
			if(du_bt_fifo->out == du_bt_fifo->size){
				du_bt_fifo->out = 0;
			}
			if(sep_flag == 4){
				i -= 3;
				break;
			}
			if(head_flag == 9){
				i -= 8;
				du_bt_fifo->out -= 9;
				break;
			}
			
		}else{
			if(((du_bt_fifo->buffer[du_bt_fifo->out] == sep[0]) && (du_bt_fifo->buffer[du_bt_fifo->out+1] == sep[1]) &&
				(du_bt_fifo->buffer[du_bt_fifo->out+2] == sep[2]) && (du_bt_fifo->buffer[du_bt_fifo->out+3] == sep[3])) && 
				(du_bt_fifo->in != du_bt_fifo->out)){
//				du_bt_fifo->buffer[du_bt_fifo->out] = 0;		//发现该字符是分隔符，将分隔符清除
				du_bt_fifo->out += 4;
				if(du_bt_fifo->out == du_bt_fifo->size){
					du_bt_fifo->out = 0;
				}
				if(i == 0){
					i--;
					continue;
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&du_bt_fifo->lock);
	return i;
}



unsigned int du_bt_fifo_push(char *buffer, unsigned int len)
{
	unsigned int ret;
	int s = SEPERATER;
	char *sep = (char *)&s;

	pthread_mutex_lock(&du_bt_fifo->lock);
	//由于dueros中的数据是十六进制且不确定数据量，需要分段发送，为防止数据顺序错乱，将每段数据分隔开
	ret = _ft_fifo_put(du_bt_fifo, buffer, len);	//从dueros中输出到uartd的数据放入到fifo中
	ret = _ft_fifo_put(du_bt_fifo, sep, 4);			//每一组数据后插入一个分隔符
	
	pthread_mutex_unlock(&du_bt_fifo->lock);

	return ret;

}

void du_bt_fifo_free()
{
	if(NULL != du_bt_fifo) 
	{
		ft_fifo_free(du_bt_fifo);
	}
}


void uartdfifo_config_handler(void *argv)
{
	DEBUG_INFO("send_button_direction_to_dueros:KEY_ONE_LONG");
	send_button_direction_to_dueros(1, KEY_ONE_LONG);
}

void uartdfifo_tlkon_handler(void *argv)
{
	DEBUG_INFO("send_button_direction_to_dueros:KEY_WAKE_UP");
	send_button_direction_to_dueros(1, KEY_WAKE_UP);
}

void uartdfifo_play_pause_handler(void *argv)
{
	DEBUG_INFO("send_button_direction_to_dueros:KEY_PLAY_PAUSE");
	send_button_direction_to_dueros(1, KEY_PLAY_PAUSE);
}		

void uartdfifo_led_handler(void *argv)
{	
	int ledstate = -1;
	char *pled = ((BUF_INF *)argv)->buf;	//AXX+LED+001
	char led_cmd[32] = {0};
	int led_cmd_len = 0;
	sscanf(&pled[3], "%d", &ledstate);
	switch(ledstate){
		case LED_NET_RECOVERY:
		case LED_NET_WAIT_CONNECT:
		case LED_NET_DO_CONNECT:
		case LED_NET_CONNECT_FAILED:
		case LED_NET_CONNECT_SUCCESS:
		case LED_NET_WAIT_LOGIN:
		case LED_NET_DO_LOGIN:
		case LED_NET_LOGIN_FAILED:
		case LED_NET_LOGIN_SUCCESS:
		case LED_WAKE_UP:		 // no param			
		case LED_SPEECH_PARSE:
		case LED_PLAY_TTS:
		case LED_PLAY_RESOURCE:
		case LED_BT_WAIT_PAIR:
		case LED_BT_DO_PAIR:
		case LED_BT_PAIR_FAILED:
		case LED_BT_PAIR_SUCCESS:
		case LED_BT_PLAY:
		case LED_BT_CLOSE:
		case LED_MUTE:
		case LED_ALARM:
		case LED_SLEEP_MODE:
		case LED_OTA_DOING:
		case LED_OTA_SUCCESS:
		case LED_CLOSE_A_LAYER:
		case LED_ALL_OFF:	 // no param
			DEBUG_INFO("%s", LED_CMD[ledstate]);
			led_cmd_len = strlen(LED_CMD[ledstate]);
			strncpy(led_cmd, LED_CMD[ledstate], led_cmd_len);
			break;
		case LED_WAKE_UP_DOA:	// param -180 ~ 180
		case LED_VOLUME:
		case LED_DISABLE_MIC:
			break;
		default:
			DEBUG_INFO(pled);
			break;
	}
	if(led_cmd_len > 0){
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, led_cmd, led_cmd_len);
		usleep(20*1000);
		pthread_mutex_unlock(&pmMUT);
	}
}

void uartdfifo_bluetooth_handler(void *argv)
{
	int blectl = -1;
	char *pble = ((BUF_INF *)argv)->buf;
	sscanf(&pble[9], "%d", &blectl);
	switch(blectl){
		case BT_OPEN:{
			break;
		}
		case BT_CLOSE:{
			break;
		}
		case BT_IS_OPENED:{
			break;
		}
		case BT_IS_CONNECTED:{
			break;
		}
		case BT_UNPAIR:{
			break;
		}
		case BT_PAUSE_PLAY:{
			break;
		}
		case BT_RESUME_PLAY:{
			break;
		}
		case BT_AVRCP_FWD:{
			break;
		}
		case BT_AVRCP_BWD:{
			break;
		}
		case BT_AVRCP_STOP:{
			break;
		}
		case BT_HFP_RECORD:{
			break;
		}
		case BLE_OPEN_SERVER:{
			break;
		}
		case BLE_CLOSE_SERVER:{
			break;
		}
		case BLE_SERVER_SEND:{
			break;
		}
		case BLE_IS_OPENED:{
			break;
		}
		case GET_BT_MAC:{
			break;
		}
		case GET_WIFI_MAC:{
			break;
		}
		case GET_WIFI_IP:{
			break;
		}
		case GET_WIFI_SSID:{
			break;
		}
		case GET_WIFI_BSSID:{
			break;
		}
	}

}

void uartdfifo_blenet_bt_handler(void * argv)
{
	char bt_buf[] = {0x88, 0x99, 0x08, 0x00, 0x02, 0x01, 0x00, 0x00};
	DEBUG_INFO("send_player_url_to_dueros:yin");
	send_data_to_dueros(bt_buf, sizeof(bt_buf));	
}

void blenet_write_duerdata_to_bt(void *argv)
{	
	char buf[32] = {0};
	int count =0;

	do{
		count =0;
		memset(buf, 0, sizeof(buf));
		buf[0] = 0x32;
		buf[1] = 0x66;
		count = du_bt_fifo_pop(&buf[3], 32-3);
		buf[2] = (char)(count);
#if UART_DATA_CHECK
		int i ;
		printf("uartdfifo sending:");
		for(i = 0 ; i < count+3; i++){
			printf("%hhx ", buf[i]);
		}
		printf("\n");
#endif
		if(count > 0){
			pthread_mutex_lock(&pmMUT);
			write(iUartfd, buf, count+3);
			usleep(100*1000);
			pthread_mutex_unlock(&pmMUT);
		}
		
		if(is_du_bt_fifo_empty() || count == 0){
			pthread_mutex_lock(&duer_send_flag.rwlock);
			duer_send_flag.flag = 0;
			pthread_mutex_unlock(&duer_send_flag.rwlock);
			break;
		}
	}while(1);
	
}

#if 0
int dueros_data_filter(char *clean_data, int *c_len, char *dirty_data, int d_len)
{
	char *tmp0 = strstr(dirty_data, "blenet_du");
	char *tmp1 = NULL;
	if(tmp0 != NULL){
		int i = 0;
		for(i = 9; i < d_len-9; i++){
			if((dirty_data[i] == 'b') &&(dirty_data[i+1] == 'l')&&(dirty_data[i+2] == 'e')&&
				(dirty_data[i+3] == 'n')&&(dirty_data[i+4] == 'e')&&(dirty_data[i+5] == 't')&&
				(dirty_data[i+6] == '_')&&(dirty_data[i+7] == 'd')&&(dirty_data[i+8] == 'u')){
					//delete dirty_data[i]-dirty_data[i+8]
					memcpy(clean_data, &tmp0[9], i-9);
					i += 9;
				}
		}
	}else{
		DEBUG_ERR("NO HEAD: blenet_du!!!!!!");
		return -1;
	}
}
#endif

void uartdfifo_blenet_du_handler(void *argv)
{
	BUF_INF *du_buf = (BUF_INF *)argv;
	pthread_t t;
	char buf[512] = {0};
	int count = 0;
	DEBUG_INFO("uartdfifo_blenet_du_handler: %s, len: %d", du_buf->buf, du_buf->len);
/*
	buf[0] = 0x32;
	buf[1] = 0x66;
	buf[2] = (char)(du_buf->len - 9);
	count += 3;
*/
	if((count + (du_buf->len - 9))> 510){
		DEBUG_ERR("too much data, not enough buffer");
		return;
	}
	memcpy(&buf[count], &(du_buf->buf[9]), du_buf->len - 9);
	count += (du_buf->len - 9);
#if UART_DATA_CHECK
	int i = 0;
	printf("uartdfifo recving from duer:");
	for(i = 0 ; i < count; i++){
		printf("%hhx ", buf[i]);
	}
	printf("\n");
#endif
	du_bt_fifo_push(buf, count);
	if(duer_send_flag.flag == 0){
		pthread_mutex_lock(&duer_send_flag.rwlock);
		duer_send_flag.flag = 1;
		pthread_mutex_unlock(&duer_send_flag.rwlock);
		
		if(pthread_create(&t, NULL, blenet_write_duerdata_to_bt, NULL) != 0)
		{
			perror("command_parse_thread create failed..\n");
		}
	}
}

extern pthread_mutex_t pmMUT;
void uartdfifo_testmode_handler(void *argv)
{
	int iRet = -1;
	DEBUG_INFO("AT_TESTMODE");
//	pthread_mutex_lock(&pmMUT);
//	write(iUartfd, "AXX+TESTMODE+OK\r\n",sizeof("AXX+TESTMODE+OK\r\n"));
//	pthread_mutex_unlock(&pmMUT);
	gpio_test_xzx();
	scan_usb_mmc();
	my_system("elian.sh connect yin 66666666");
	
	//iRet = my_system("uci set elian.config.conf=false && uci commit");

	return ;
}



static char AT_POW_ON[]   	 =      "AXX+POW+ON1&";
static char AT_STARTTEST[]   = 		"AXX+STARTTEST&";
static char AT_WIFI_CONFIG[] = 		"AXX+STA+002&";
static char AT_WIFI_CONNECTING[] = 	"AXX+STA+003&";
static char AT_WIFI_SUCCEED[] = 	"AXX+STA+001&";
static char AT_WIFI_FAILED[] = 		"AXX+STA+000&";
static char AT_POWR_OFF[] = 		"AXX+POW+OFF&";
static char AT_VOL_GET[] =	 		"AXX+VOL+GET&";
static char AT_VOL_SET[] = 			"AXX+VOL+000&";//000-100音级
static char AT_PLP[] =	 			"AXX+PLP+000&";//播放循环模式 000:顺序 001:重复 002:单曲 003:随机
static char AT_PLY[] =	 			"AXX+PLY+000&";//000:播放中 001:暂停中
static char AT_PLM_GET[] =			"AXX+PLM+GET&";//获取播放模式 000:WIFI 001:BT 002:AUX
static char AT_PLM_SET[] =			"AXX+PLM+000&";//设置播放模式 000:WIFI 001:BT 002:AUX
static char AT_SEQ[] =	 			"AXX+SEQ+000&";
static char AT_CAP_GET[] =			"AXX+CAP+GET&";//Wifi要求MCU回复项目的各种信息
static char AT_BAT_GET[] =			"AXX+BAT+GET&";//Wifi要求MCU回复电量信息
static char AT_CHG_GET[] =			"AXX+CHG+GET&";//获取是是否充电标志位
static char AT_TLK_ON[] =			"AXX+TLK++ON&";
static char AT_TLK_OFF[] =			"AXX+TLK+OFF&";
static char AT_CHANNEL[] =			"AXX+CHN+000&";//000:立体声 001:左声道 002:右声道
static char AT_MUTE[] =				"AXX+MUT+000&";//000:静音 001:取消静音
static char AT_ALARM[] =			"AXX+ALR+0000087f&";//组号 是否开启 分 时 week
static char AT_DELETE_ALARM[] =		"AXX+ALR+C00&";//C00第一组 C01第二组
static char AT_TIME[] =				"AXX+TIM+00000821010616&";//秒 分 时 日 week month year
static char AT_SEARCHSTART[] =		"AXX+SEA+STA&";//语音搜索开始
static char AT_SEARCHSUCCEED[] =	"AXX+SEA+STC&";//语音搜索成功
static char AT_SEARCHFAILED[] =		"AXX+SEA+STO&";//语音搜索失败

static char AXX_TIM_STA[] =         "AXX+TIM+STA&";// Alexa闹钟开始
static char AXX_TIM_END[] =         "AXX+TIM+END&";// Alexa闹钟结束

static char AXX_TIM_PAS[] =			"AXX+TIM+PAS&";// Alexa闹钟结束，语音继续播放

static char AXX_NTF_STA[] =         "AXX+NTF+STA&";// Alexa通知开始
static char AXX_NTF_END[] =         "AXX+NTF+END&";// Alexa通知结束
static char AT_WIFI_CON_FAILED[] = 	"AXX+RA0+NOT&"; //开机进入WIFI模式回连失败,WIFI主动发给MCU：AXX+RA0+NOT&
static char AT_TF_INT[] = 			"AXX+TF++INT&"; //WIFI主动发给MCU：插入TF卡：
static char AT_TF_OUT[] = 			"AXX+TF++OUT&"; //拔出TF卡：
static char AT_TF_ONL[] = 			"AXX+TF++ONL&";//开机稳定后，WIFI主动发给MCU：TF卡在线：
static char AT_TF_OUL[] = 			"AXX+TF++OUL&";//TF卡不在线：

//WIFI主动发给MCU：插入USB：AXX+USB+INT&拔出USB：AXX+USB+OUT& 开机稳定后，WIFI主动发给MCU：USB在线：AXX+USB+ONL& USB不在线：AXX+USB+OUL&
static char AT_USB_INT[] = 			"AXX+USB+INT&"; 
static char AT_USB_OUT[] = 			"AXX+USB+OUT&"; 
static char AT_USB_ONL[] = 			"AXX+USB+ONL&";
static char AT_USB_OUL[] = 			"AXX+USB+OUL&";

//模式切换结果
static char AT_IN_WIFIMODE[] = "AXX+PLM+WIF&";	//进入WIFI模式，WIFI主动发给MCU：AXX+PLM+WIF&

static char AT_IN_TFMODE[] = "AXX+PLM++TF&";
static char AT_IN_USBMODE[] = "AXX+PLM+USB&";


static char AT_DIS_POWER[]="AXX+KEY+BAT&";


static char AT_NET_OK[]			="AXX+NET++OK&";
static char AT_NET_FAIL[]		="AXX+NET+FAL&";
static char AT_USB_OK[]			="AXX+USB++OK&";
static char AT_USB_FAIL[]		="AXX+USB+FAL&";
static char AT_SD_OK[]			="AXX+SDC++OK&";
static char AT_SD_FAIL[]		="AXX+SDC+FAL&";

static char ST_NET_OK[]			="SXX+NET++OK&";
static char ST_NET_FAIL[]		="SXX+NET+FAL&";
static char ST_BOOT_START[]		="BOOTMUSICSTART&";
static char AT_GET_VER[]		="AXX+MCU+VER&";

static char AXX_SEA_LDI[] 		="AXX+SEA+LDI&";		  // Alexa登录成功 
static char AXX_SEA_WEK[] 		="AXX+SEA+WEK&"; 		  // Alexa唤醒成功
static char AXX_SEA_SUC[] 		="AXX+SEA+SUC&";		  // Alexa识别成功，开始播放Alexa回复语音
static char AXX_SEA_FAL[] 		="AXX+SEA+FAL&";		  // Alexa无法识别
static char AXX_SEA_IDL[] 		="AXX+SEA+IDL&"	;	  	  // Alexa回复语音播放结束
static char AXX_SEA_LIS[]		="AXX+SEA+LIS&"; 	  	  // Alexa开始录音
static char AXX_SEA_RED[] 		="AXX+SEA+RED&";	      // Alexa录音结束

static char AXX_CAP_GET[]  = "AXX+CAP+GET&";
static char AT_UP_DATE[]		="AXX+UP+DATE&";	//蓝牙进入升级模式


static char AXX_NET_ERR[]  = "AXX+NET+ERR&";
static char AXX_ALE_ERR[]  = "AXX+ALE+ERR&";

static char AXX_UP_WIFI[]  = "AXX+UP+WIFI&";
static char CMD_EMIT_INQ[]		="CMD+EMIT_INQ&";           //搜索周围设备
static char CMD_IN[]			="CMD+IN01+888888888888&";  //蓝牙配对
static char CMD_EMIT_CON[]		="CMD+EMIT_CON&";           //连接
static char CMD_EMIT_DISCON[]	="CMD+EMIT_DISCON&";        //断开
static char CMD_EMIT_LINK[]	    ="CMD+EMIT_LINK&";          //查询当前连接设备
static char DUER_LED_SET[] = 	"AXX+LED+003&";
#define BT_ADDR     "/tmp/BT_ADDR"     //正在连接的蓝牙地址


extern int iflytek_file;
int bt_flage = 0;

typedef unsigned char  					uint8_t;
typedef unsigned short 					uint16_t;
typedef unsigned int  					uint32_t;


#define DEC( c ) ((c) >= '0' && (c) <= '9')//判断是否10进制

int file_exists(const char *path)    // note: anything but a directory
{
    struct stat st;
    
    memset(&st, 0 , sizeof(st));
    return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode)) && (st.st_size > 0);
}

const uint16_t wCRC_Table[256] =
{
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

#define CAL_CRC(__CRCVAL,__NEWCHAR)             \
     {\
        (__CRCVAL) = ((uint16_t)(__CRCVAL) >> 8) \
            ^ wCRC_Table[((uint16_t)(__CRCVAL) ^ (uint16_t)(__NEWCHAR)) & 0x00ff]; \
     }

#define F_PATH "/tmp/bt_upgrade.bin"
uint8_t bt_mac_frame[6]={0};

uint16_t cal_crc(uint8_t *p,uint32_t len,uint16_t crc)
{
    if (p == NULL)
    {
        return 0xFFFF;
    }

    while(len--)
    {
        uint8_t chData = *p++;
        CAL_CRC(crc, chData);
    }

    return crc;
}


void getcap()
{
    char audioname[SSBUF_LEN_128];
	//char *str = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.audioname");
	cdb_get_str("$audioname",audioname,sizeof(audioname),"");
	if (audioname == NULL)
		return;
	if (audioname[0] == ' ' && audioname[1] == 0)
	{
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, AT_CAP_GET, strlen(AT_CAP_GET));
		pthread_mutex_unlock(&pmMUT);
	}
	//free(str);
}

unsigned int CaculateCRC(unsigned char *byte, int count)
{
	unsigned int CRC = 0;
	int i;
	unsigned char j;
	unsigned char temp;
	for(i = 0;i < count;i++) 
	{
		temp = byte[i+3]; 
		for(j = 0x80;j != 0;j /= 2)
		{
			if((CRC & 0x8000) != 0){CRC <<= 1; CRC ^= 0x1021;}
			else CRC <<= 1;
			if((temp & j) != 0) CRC ^= 0x1021;
		}
	}	
	return CRC;
}

//by yin
#if 0
#define DIRECT_URL_BUFSIZE 1024
#define DIRECT_BUTTON_BUFSIZE 64
#define DELIMITER '`'
#define TERMINATOR '\n'

enum INSTRUCTION_TYPE{
	BUTTON,
	URL
};

//para instruction:
//	int sum: 参数个数(不算sum.从1开始数)
//只能传int型参数
int send_button_direction_to_dueros(int sum, int src, ...)
{
  int para;
  int argno = 1, i = 0, type = BUTTON;  
  char *dest = (char *)malloc(DIRECT_BUTTON_BUFSIZE);
  if(dest){
	  memset(dest, 0, DIRECT_BUTTON_BUFSIZE);
  }else{
	  DEBUG_ERR("malloc error");
	  return -1;
  }
  memcpy(&dest[i], &type, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  va_list args;
  va_start(args,src);
  DEBUG_INFO("fmt: %d", src);
  memcpy(&dest[i], &src, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  DEBUG_INFO("contex: %s", dest);
  while(argno < sum){
    para = va_arg(args, int );
    DEBUG_INFO("para: %d, i: %d", para, i);
    memcpy(&dest[i], &para, sizeof(int));
    i += sizeof(int);
    dest[i++] = DELIMITER;
    argno++;
  }
  va_end(args);
  dest[i-1] = TERMINATOR;
  
  socket_write(UNIX_DOMAIN, dest, i);
  free(dest);
  return 0;
}

//para instruction:
//	int sum: 参数个数(不算sum.从1开始数)
//只能传char *型参数
int send_player_url_to_dueros(int sum, const char *src, ...)
{
  char *para;
  int argno = 1, i = 0, type = URL;
  char *dest = (char *)malloc(DIRECT_URL_BUFSIZE);
  if(dest){
	  memset(dest, 0, DIRECT_URL_BUFSIZE);
  }else{
	  DEBUG_ERR("malloc error");
	  return -1;
  }
  memcpy(&dest[i], &type, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  
  va_list args;
  va_start(args,src);
  vsprintf(&dest[i],src,args);
  i += strlen(src);
  dest[i++] = DELIMITER;
  DEBUG_INFO("contex: %s", dest);
  while(argno < sum){
	para = va_arg(args, char *);
	DEBUG_INFO("para: %s, i: %d", para, i);
	vsprintf(&dest[i],para,args);
	i += strlen(para);
	dest[i++] = DELIMITER;
	argno++;
  }
  va_end(args);
  dest[i-1] = TERMINATOR;
  socket_write(UNIX_DOMAIN, dest, i);
  free(dest);
  return 0;
}

int send_data_to_dueros(const void *data, unsigned int len)
{
  int i = 0, type = URL;
  char *dest = (char *)malloc(DIRECT_URL_BUFSIZE);
  if(dest){
	  memset(dest, 0, DIRECT_URL_BUFSIZE);
  }else{
	  DEBUG_ERR("malloc error");
	  return -1;
  }
  memcpy(&dest[i], &type, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  memcpy(&dest[i], data, len);
  i += len;
  dest[i++] = TERMINATOR;
  socket_write(UNIX_DOMAIN, dest, i);
  free(dest);
  return 0;
}
#endif

//end yin

// by mars
void *uartfifo_thread(void *arg)
{
	char byReadBuf[BUFF_LENGTH] = {0};
	char *bySendBuf;
	int fd = -1;
	int iLength = 0;
	FILE *pf = NULL;
	BUF_INF rec_data_inf;
	du_bt_fifo_init();
	pthread_mutex_init(&(duer_send_flag.rwlock), NULL);

	duer_send_flag.flag = 0;

	unlink(FIFO_FILE);
	mkfifo(FIFO_FILE, 0666);
	
	//getcap();
	pthread_mutex_lock(&pmMUT);
	//write(iUartfd, AT_POW_ON, strlen(AT_POW_ON));
	//usleep(20*1000);
//	write(iUartfd,AT_VOL_GET,strlen(AT_VOL_GET));
//	usleep(20*1000);
	write(iUartfd, AT_PLM_GET, strlen(AT_PLM_GET));
	usleep(20*1000);
	write(iUartfd, AT_GET_VER, strlen(AT_GET_VER));
	usleep(20*1000);
	write(iUartfd, CMD_EMIT_LINK, sizeof(CMD_EMIT_LINK)-1);
	usleep(20*1000);
	int vol=cdb_get_int("$ra_vol",60);
	snprintf(AT_VOL_SET,sizeof(AT_VOL_SET),"AXX+VOL+%03d&",vol);
	write(iUartfd, AT_VOL_SET, strlen(AT_VOL_SET));
	usleep(20*1000);
	write(iUartfd, AT_BAT_GET, strlen(AT_BAT_GET));
	usleep(20*1000);
	write(iUartfd, AT_STARTTEST, strlen(AT_STARTTEST));
	pthread_mutex_unlock(&pmMUT);

	while(1)
	{
		if ((fd = open(FIFO_FILE, O_RDONLY)) == -1)
		{
			exit(-1);
		}
		
		memset(byReadBuf, 0, BUFF_LENGTH);
		iLength = read(fd, byReadBuf, BUFF_LENGTH);
		DEBUG_DBG("byReadBuf:%s",byReadBuf);
		DEBUG_DBG("Received length:%d\n", iLength);
		if (iLength > 0)
		{
#if defined(CONFIG_PACKAGE_duer_linux)
			int i = 0;
			int buf_len = sizeof(uartdfifo_directive)/sizeof(DIRECTION_HANDLER);
			for(; i < buf_len; i++){
				if (0 == strncmp(byReadBuf, uartdfifo_directive[i].direction, strlen(uartdfifo_directive[i].direction)-1)){		//判断指令是否相等
					rec_data_inf.buf = byReadBuf;
					rec_data_inf.len = iLength;
					uartdfifo_directive[i].handler(&rec_data_inf);
				}
			}
#endif    

#if defined(CONFIG_PROJECT_K2_V1) || defined(CONFIG_PROJECT_DOSS_1MIC_V1)
			int loopk;
			int iLength_buf=0;
			iLength_buf = sizeof(turing_leds_tbl) / sizeof(led_proc_handler);
			for(loopk=0;loopk < iLength_buf;loopk++)
			{
				if (0 == strncmp(byReadBuf, turing_leds_tbl[loopk].buf, strlen(turing_leds_tbl[loopk].buf)-1))
				{
					turing_leds_tbl[loopk].func(&byReadBuf);
					memset(byReadBuf, 0, BUFF_LENGTH);
					close(fd);
					fd = -1;
				}
			}
#endif

/**********************灯效命令控制************************/
/*
#if defined (CONFIG_PROJECT_K2_V1) || defined (CONFIG_PROJECT_DOSS_1MIC_V1)		//k2项目/Doss项目
			if(0 == strncmp(byReadBuf,"StartConnecting",strlen("StartConnecting")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)
				bySendBuf = "AXX+STA+002";		
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+011";
#endif
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"StartConnectedOK",strlen("StartConnectedOK")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)
				bySendBuf = "AXX+STA+001";		
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+007";				
#endif
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"StartConnectedFAL",strlen("StartConnectedFAL")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)
				bySendBuf = "AXX+STA+000";	
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+004";								
#endif

				DEBUG_INFO("bySendBuf by tian =[%s]\n",bySendBuf);
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"StartTlkOn",strlen("StartTlkOn")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)
				bySendBuf = "AXX+TLK++ON&";	
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+012";												
#endif
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"StartTlkPlay",strlen("StartTlkPlay")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)
				bySendBuf = "AXX+TLK+PLAY&";
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+003";																
#endif
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"StartKeyFactory",strlen("StartKeyFactory")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)			
				bySendBuf = "AXX+FACTORY";		
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+009";	//用的wifi升级的灯效，后期可以更换																			
#endif
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"StartPowerOn",strlen("StartPowerOn")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)	
				bySendBuf = "AXX+PWR++ON";		
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+007";																								
#endif
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"StartWifiUp",strlen("StartWifiUp")))
			{
#if   defined (CONFIG_PROJECT_K2_V1)	
				bySendBuf = "AXX+STA+002";
#elif defined (CONFIG_PROJECT_DOSS_1MIC_V1)
				bySendBuf = "AXX+LED+009";	
#endif
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
#endif
*/
			/************************by tian,add "standby" ***********************************/
			if(0 == strncmp(byReadBuf,"standby",strlen("standby")))
			{
				bySendBuf = "AXX+POW+OFF&"; 	//语音关机
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"BatteryChangeState",strlen("BatteryChangeState")))
			{
				bySendBuf = AT_CHG_GET;		
				write(iUartfd, bySendBuf, strlen(bySendBuf));
				close(fd);
				fd = -1;
				continue;
			}
			if(0 == strncmp(byReadBuf,"curtime",strlen("curtime")))
			{
			    char  buf[24] = {0};
                char*wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
                time_t t;
                struct tm *p;
                t=time(NULL);/*获取从1970年1月1日零时到现在的秒数，保存到变量t中*/ 
                p=localtime(&t); /*localtime current time ; gmtime UTC time*/   
                snprintf(buf,24,"AXX+CUR+TIME+%02d%02d%02d&",p->tm_hour,p->tm_min, p->tm_sec);
				write(iUartfd, buf, strlen(buf));
				close(fd);
				fd = -1;
				continue;
			}
			if (0 == strncmp(byReadBuf, WIFI_CONFIG, sizeof(WIFI_CONFIG) - 1))
			{
				bySendBuf = AT_WIFI_CONFIG;
			}
			//duerSceneLedSet009
			else if (0 == strncmp(byReadBuf, DUER_LED_SET_MATCH, sizeof(DUER_LED_SET_MATCH) - 1))
			{			
				char* idx=byReadBuf+sizeof(DUER_LED_SET_MATCH)-1;
				strncpy(DUER_LED_SET+sizeof(DUER_LED_SET)-5,idx,3);									
				 DEBUG_TRACE("bySendBuf:%s",DUER_LED_SET);//AXX+LED+009&
				bySendBuf = DUER_LED_SET;
			}			
			else if (0 == strncmp(byReadBuf, WIFI_CONNECTING, sizeof(WIFI_CONNECTING) - 1))
			{
				bySendBuf = AT_WIFI_CONNECTING;
			}
			else if (0 == strncmp(byReadBuf, WIFI_SUCCEED, sizeof(WIFI_SUCCEED) - 1))
			{
				bySendBuf = AT_WIFI_SUCCEED;
			}
			else if (0 == strncmp(byReadBuf, WIFI_FAILED, sizeof(WIFI_FAILED) - 1))
			{
				bySendBuf = AT_WIFI_FAILED;
			}
			else if (0 == strncmp(byReadBuf, POWEROFF, sizeof(POWEROFF) - 1))
			{
				bySendBuf = AT_POWR_OFF;
			}
			else if (0 == strncmp(byReadBuf, GETVOL, sizeof(GETVOL) - 1))
			{
				bySendBuf = AT_VOL_GET;
			}
			//setvol000
			else if (0 == strncmp(byReadBuf, SETVOL, sizeof(SETVOL) - 1)){	//语音设置音量值
				char buf[5]="";
				int iRet=0;
				unsigned idx=byReadBuf+sizeof(SETVOL)-1;
				memset(buf,0,sizeof(buf));
				strncpy(buf,idx,3);			//取出音量值000
				unsigned volume=atoi(buf);
				
				if(cdb_get_int("$ra_vol",60) == volume){

					if(volume>100){
					iRet=1;
					goto isContinue;
					}

					DEBUG_INFO("volume :%d",volume);
					snprintf(AT_VOL_SET,sizeof(AT_VOL_SET),"AXX+VOL+%03d&",volume);
					bySendBuf=AT_VOL_SET;
					
				}
//				if(cdb_get_int("$ra_vol",60)==volume){
//					iRet=-1;
//					goto isContinue;
//				}
//				iRet=amixer_set_playback_volume("All",0,get_vol_set(volume));
//				if(iRet){
//					goto isContinue;
//				}
//				cdb_set_int("$ra_vol", volume);
				// timer_cdb_save();

				
 				
//				idx+=3;
//				memset(buf,0,sizeof(buf));
//				strncpy(buf,idx,1);
//				playerId_t playerId=atoi(buf)%END_PLAYER_ID;
//				iRet =mutiPlayerVolumeSync(playerId,volume);
isContinue:
				close(fd);
				fd = -1;
				if(iRet){
					iRet=0;
					continue;
				}
			}
			else if (0 == strncmp(byReadBuf, TLKON, sizeof(TLKON) - 1))
			{
			    // printf("enter tlkon byReadBuf =%s\n",byReadBuf);
				//tlkon();
				send_button_direction_to_dueros(1, KEY_WAKE_UP);
				bySendBuf = AT_TLK_ON;
				// close(fd);
				// fd = -1;
				// continue;
			}
			else if (0 == strncmp(byReadBuf, TLKOFF, sizeof(TLKOFF) - 1))
			{
				// tlkoff();
				bySendBuf = AT_TLK_OFF;
				// close(fd);
				// fd = -1;
				// continue;
			}
			else if (0 == strncmp(byReadBuf, TESTTLKON, sizeof(TESTTLKON) - 1))	//厂测录音开始
			{
				tlkon_test();
				//bySendBuf = AT_TLK_OFF;
				close(fd);
				fd = -1;
				continue;
			}
			else if (0 == strncmp(byReadBuf, TESTTLKOFF, sizeof(TESTTLKOFF) - 1))//厂测录音结束
			{
				tlkoff_test();
				//bySendBuf = AT_TLK_OFF;
				close(fd);
				fd = -1;
				continue;
			}
			else if (0 == strncmp(byReadBuf, TLKONTIME, sizeof(TLKONTIME) - 1))
			{
			     printf("enter TLKONTIME byReadBuf =%s\n",byReadBuf);
				tlkon_test_time();
				//bySendBuf = AT_TLK_OFF;
				close(fd);
				fd = -1;
				continue;
			}
			else if (0 == strncmp(byReadBuf, GETCAP, sizeof(GETCAP) - 1))
			{
				bySendBuf = AT_CAP_GET;
			}
			else if (0 == strncmp(byReadBuf, GETBAT, sizeof(GETBAT) - 1))
			{
				bySendBuf = AT_BAT_GET;
			}
			else if (0 == strncmp(byReadBuf, GETPLM, sizeof(GETPLM) - 1))
			{
				bySendBuf = AT_PLM_GET;
			}
			else if (0 == strncmp(byReadBuf, SETPLM, sizeof(SETPLM) - 1))
			{
				char plm[4] = {0};
				sscanf(&byReadBuf[sizeof(SETPLM)-1], "%3s", plm);
				if (DEC(plm[0]) && DEC(plm[1]) &&  DEC(plm[2]))
				{
					bySendBuf = AT_PLM_SET;
					bySendBuf[8] = plm[0];
					bySendBuf[9] = plm[1];
					bySendBuf[10] = plm[2];
				}
				else
				{
					close(fd);
					fd = -1;
					continue;
				}
			}
			else if (0 == strncmp(byReadBuf, PLAY, sizeof(PLAY) - 1))
			{
//				bySendBuf = AT_PLY;
//				bySendBuf[10] = '1';
				send_button_direction_to_dueros(1, KEY_PLAY_PAUSE);
			}
			else if (0 == strncmp(byReadBuf, PAUSE, sizeof(PAUSE) - 1))
			{
//				bySendBuf = AT_PLY;
//				bySendBuf[10] = '0';
				send_button_direction_to_dueros(1, KEY_PLAY_PAUSE);
			}
			else if (0 == strncmp(byReadBuf, CHANNEL, sizeof(CHANNEL) - 1))
			{
				char run[128] = {0};
				char channel[4] = {0};
				sscanf(&byReadBuf[sizeof(CHANNEL)-1], "%3s", channel);
				if (DEC(channel[0]) && DEC(channel[1]) &&  DEC(channel[2]))
				{
					//snprintf(run, sizeof(run), "uci set xzxwifiaudio.config.channel=%s && uci commit", channel);
					//my_system(run);
	         // WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.channel", channel);
					cdb_set("$channel",channel);
					//bySendBuf = AT_CHANNEL;
					//bySendBuf[8] = channel[0];
					//bySendBuf[9] = channel[1];
					//bySendBuf[10] = channel[2];
					if(strcmp(channel, "000") == 0) {
						my_system("wavplayer /tmp/res/stereo.wav");
					} else if(strcmp(channel, "001") == 0) {
						my_system("wavplayer /tmp/res/left_channel.wav");
					} else if(strcmp(channel, "002") == 0) {
						my_system("wavplayer /tmp/res/right_channel.wav");
					} else {
						//do nothing
					}
					bzero(run, sizeof(run));
					snprintf(run, sizeof(run), "amixer cset numid=2 %s", channel);
					DEBUG_INFO("run:%s",run);
					my_system(run);
					close(fd);
					fd = -1;
					continue;
				}
				else
				{
					close(fd);
					fd = -1;
					continue;
				}
			}
			else if (0 == strncmp(byReadBuf, MUTE, sizeof(MUTE) - 1))
			{
				char mute[4] = {0};
				sscanf(&byReadBuf[sizeof(MUTE)-1], "%3s", mute);
				DEBUG_INFO("mute=%s",mute);
				if (DEC(mute[0]) && DEC(mute[1]) &&  DEC(mute[2]))
				{
					bySendBuf = AT_MUTE;
					bySendBuf[8] = mute[0];
					bySendBuf[9] = mute[1];
					bySendBuf[10] = mute[2];
				}
				else
				{
					close(fd);
					fd = -1;
					continue;
				}
			}
			else if (0 == strncmp(byReadBuf, SEARCHSTART, sizeof(SEARCHSTART) - 1))
			{
			//	system("wavplayer /root/res/searchstart.wav -M");
				bySendBuf = AT_SEARCHSTART;
			}
			else if (0 == strncmp(byReadBuf, SEARCHSUCCEED, sizeof(SEARCHSUCCEED) - 1))
			{
				//system("wavplayer /root/res/searchsucceed.wav -M");
				bySendBuf = AT_SEARCHSUCCEED;
			}
			else if (0 == strncmp(byReadBuf, SEARCHFAILED, sizeof(SEARCHFAILED) - 1))
			{
			//	system("wavplayer /root/res/searchfailed.wav -M");
				bySendBuf = AT_SEARCHFAILED;
			}
			else if (0 == strncmp(byReadBuf, ALARM, sizeof(ALARM) - 1))
			{
				char num,enable;
				char minute[3], hour[3], week[3];
				//DEBUG_INFO(&byReadBuf[sizeof(ALARM)]);
				sscanf(&byReadBuf[sizeof(ALARM)], "%c %c %2s %2s %2s", &num, &enable, minute, hour, week);
				bySendBuf = AT_ALARM;
				bySendBuf[8] = num;
				bySendBuf[9] = enable;
				bySendBuf[10] = minute[0];
				bySendBuf[11] = minute[1];
				bySendBuf[12] = hour[0];
				bySendBuf[13] = hour[1];
				bySendBuf[14] = week[0];
				bySendBuf[15] = week[1];
				//DEBUG_INFO(bySendBuf);
			}
			else if (0 == strncmp(byReadBuf, DELETEALARM, sizeof(DELETEALARM) - 1))
			{
				char num;
				sscanf(&byReadBuf[sizeof(DELETEALARM)], "%c", &num);
				bySendBuf = AT_DELETE_ALARM;
				bySendBuf[10] = num;
				//DEBUG_INFO(bySendBuf);
			}
			else if (0 == strncmp(byReadBuf, TIME, sizeof(TIME) - 1))
			{
				char second[3], minute[3], hour[3], day[3], week[3], month[3], year[5];
				//DEBUG_INFO("scanf %s\n",&byReadBuf[sizeof(TIME)]);
				sscanf(&byReadBuf[sizeof(TIME)], "%2s %2s %2s %2s %c %2s %4s", second, minute, hour, day, &week[0], month, year);
				switch (week[0])
				{
					case '0':week[0]='0';week[1]='1';break;
					case '1':week[0]='0';week[1]='2';break;
					case '2':week[0]='0';week[1]='4';break;
					case '3':week[0]='0';week[1]='8';break;
					case '4':week[0]='1';week[1]='0';break;
					case '5':week[0]='2';week[1]='0';break;
					case '6':week[0]='4';week[1]='0';break;
					default:close(fd);fd = -1;continue;
				}
				//DEBUG_INFO("sscanf %s %s %s %s %s %s %s\n", second, minute, hour, day, week, month, year);
				bySendBuf = AT_TIME;
				bySendBuf[8] = second[0];
				bySendBuf[9] = second[1];
				bySendBuf[10] = minute[0];
				bySendBuf[11] = minute[1];
				bySendBuf[12] = hour[0];
				bySendBuf[13] = hour[1];
				bySendBuf[14] = day[0];
				bySendBuf[15] = day[1];
				bySendBuf[16] = week[0];
				bySendBuf[17] = week[1];
				bySendBuf[18] = month[0];
				bySendBuf[19] = month[1];
				bySendBuf[20] = year[2];
				bySendBuf[21] = year[3];
				//DEBUG_INFO(bySendBuf);
			}
			else if (0 == strncmp(byReadBuf, STARTTEST, sizeof(STARTTEST) - 1))
			{
				bySendBuf = AT_STARTTEST;
			}
			else if(0 == strncmp(byReadBuf, TFINT, sizeof(TFINT) - 1))
			{
				bySendBuf = AT_TF_INT;
			}
			else if(0 == strncmp(byReadBuf, TFONL, sizeof(TFONL) - 1))
			{
				bySendBuf = AT_TF_ONL;
			}
			else if(0 == strncmp(byReadBuf, TFOUT, sizeof(TFOUT) - 1))
			{
				//char *str = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");  //search wifi config playmode 
                int usb_status = cdb_get_int("$usb_status",0);
			    int playmode = cdb_get_int("$playmode",0);

				DEBUG_INFO("playmode = %d \n",playmode);
				pthread_mutex_lock(&pmMUT);
				write(iUartfd, "AXX+TF++OUT&",strlen("AXX+TF++OUT&"));
				pthread_mutex_unlock(&pmMUT);
			 	if(playmode == 004) {

				if(cdb_get_int("$vtf003_usb_tf",0)){
						if(usb_status==0){
						bySendBuf = AT_IN_WIFIMODE;
						cdb_set_int("$playmode",000);
						//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
						}else{
	                    bySendBuf = AT_IN_USBMODE;
						}
					}else{
						bySendBuf = AT_IN_WIFIMODE;
						cdb_set_int("$playmode",000);
						//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");

						}
				} else 
					bySendBuf="";
					
			}
			else if(0 == strncmp(byReadBuf, TFOUL, sizeof(TFOUL) - 1))
			{
				bySendBuf = AT_TF_OUL;
			}
			else if(0 == strncmp(byReadBuf, USBINT, sizeof(USBINT) - 1))
			{
				bySendBuf = AT_USB_INT;
			}
			else if(0 == strncmp(byReadBuf, USBOUT, sizeof(USBOUT) - 1))
			{
				//char *str = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.playmode");  //search wifi config playmode 
				int playmode = cdb_get_int("$playmode",0);
				int tf_status = cdb_get_int("$tf_status",0);

				DEBUG_INFO("playmode = %d \n",playmode);
				pthread_mutex_lock(&pmMUT);
				write(iUartfd, "AXX+USB+OUT&",strlen("AXX+USB+OUT&"));
				pthread_mutex_unlock(&pmMUT);
				
				//bySendBuf = AT_IN_WIFIMODE;
				if(playmode == 003) {
					if(cdb_get_int("$vtf003_usb_tf",0)){
						if(tf_status==0){
						bySendBuf = AT_IN_WIFIMODE;
						cdb_set_int("$playmode",000);
						//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
						}
						else{
						bySendBuf = AT_IN_TFMODE;
						}
					}else{
						bySendBuf = AT_IN_WIFIMODE;
						cdb_set_int("$playmode",000);
						//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
					 }
					
				}else {
					bySendBuf="";
				}

			}
			else if(0 == strncmp(byReadBuf, USBOUL, sizeof(USBOUL) - 1))
			{

				bySendBuf = AT_USB_OUL;

			}
			else if(0 == strncmp(byReadBuf, USBONL, sizeof(TFONL) - 1))
			{
				bySendBuf = AT_USB_ONL;
			}
			else if(0 == strncmp(byReadBuf, INWIFIMODE, sizeof(INWIFIMODE) - 1))
			{
				bySendBuf = AT_IN_WIFIMODE;
			}
			else if(0 == strncmp(byReadBuf, INUSBMODE, sizeof(INUSBMODE) - 1))
			{
				bySendBuf = AT_IN_USBMODE;
			}
			else if(0 == strncmp(byReadBuf, INTFMODE, sizeof(INTFMODE) - 1))
			{
				bySendBuf = AT_IN_TFMODE;
			}
			else if(0 == strncmp(byReadBuf, WIFI_BACK_FAILE, sizeof(WIFI_BACK_FAILE) - 1))
			{
				bySendBuf = AT_WIFI_CON_FAILED;
			}
			else if(0 == strncmp(byReadBuf, DISPOWER, sizeof(DISPOWER) - 1))
			{
				bySendBuf = AT_DIS_POWER;
			}
			else if(0 == strncmp(byReadBuf, NET_CONNECT_OK, sizeof(NET_CONNECT_OK) - 1))
			{
				bySendBuf = AT_NET_OK;
			}
			else if(0 == strncmp(byReadBuf, NET_CONNECT_FAILED, sizeof(NET_CONNECT_FAILED) - 1))
			{
				bySendBuf = AT_NET_FAIL;
			}
			else if(0 == strncmp(byReadBuf, START_BOOT_MUSIC, sizeof(START_BOOT_MUSIC) - 1))
			{
				bySendBuf = ST_BOOT_START;
			}
			else if(0 == strncmp(byReadBuf, SNET_CONNECT_FAILED, sizeof(SNET_CONNECT_FAILED) - 1))
			{
				bySendBuf = ST_NET_FAIL;
			}
			else if(0 == strncmp(byReadBuf, SNET_CONNECT_OK, sizeof(SNET_CONNECT_OK) - 1))
			{
				bySendBuf = ST_NET_OK;
			}
			else if(0 == strncmp(byReadBuf, USB_TEST_OK, sizeof(USB_TEST_OK) - 1))
			{
				bySendBuf = AT_USB_OK;
			}
			else if(0 == strncmp(byReadBuf, USB_TEST_FAILED, sizeof(USB_TEST_FAILED) - 1))
			{
				bySendBuf = AT_USB_FAIL;
			}
			else if(0 == strncmp(byReadBuf, SD_TEST_OK, sizeof(SD_TEST_OK) - 1))
			{
				bySendBuf = AT_SD_OK;
			}
			else if(0 == strncmp(byReadBuf, SD_TEST_FAILED, sizeof(SD_TEST_FAILED) - 1))
			{
				bySendBuf = AT_SD_FAIL;
			}
			else if(0 == strncmp(byReadBuf, GETVER, sizeof(GETVER) - 1))
			{
				bySendBuf = AT_GET_VER;
			}
			else if (0 == strncmp(byReadBuf, VAD_THRESHOLD_ON, sizeof(VAD_THRESHOLD_ON) - 1)) {
				pthread_mutex_lock(&pmMUT);
				write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));
				pthread_mutex_unlock(&pmMUT);
			}
			else if(0 == strncmp(byReadBuf, VAD_THRESHOLD_OFF, sizeof(VAD_THRESHOLD_OFF) - 1)) {
				pthread_mutex_lock(&pmMUT);
				write(iUartfd, "AXX+TLK+OFF&", strlen("AXX+TLK+OFF&"));
				pthread_mutex_unlock(&pmMUT);
			}
			else if (0 == strncmp(byReadBuf, ALEXA_LOGIN_SUCCED, sizeof(ALEXA_LOGIN_SUCCED) - 1))
			{
				bySendBuf = AXX_SEA_LDI;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_WAKE_UP, sizeof(ALEXA_WAKE_UP) - 1))
			{
				bySendBuf = AXX_SEA_WEK;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_CAPTURE_START, sizeof(ALEXA_CAPTURE_START) - 1))
			{
				bySendBuf = AXX_SEA_LIS;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_CAPTURE_END, sizeof(ALEXA_CAPTURE_END) - 1))
			{
				bySendBuf = AXX_SEA_RED;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_IDENTIFY_OK, sizeof(ALEXA_IDENTIFY_OK) - 1))
			{
				bySendBuf = AXX_SEA_SUC;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_IDENTIFY_ERR, sizeof(ALEXA_IDENTIFY_ERR) - 1))
			{
				bySendBuf = AXX_SEA_FAL;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_VOICE_PLAY_END, sizeof(ALEXA_VOICE_PLAY_END) - 1))
			{
				bySendBuf = AXX_SEA_IDL;
			}
			else if (0 == strncmp(byReadBuf, VOICE_INTERACTION, sizeof(VOICE_INTERACTION) - 1))
			{
				SpeechSelect();
			}
			else if (0 == strncmp(byReadBuf, ALEXA_ALER_START, sizeof(ALEXA_ALER_START) - 1))
			{
				bySendBuf = AXX_TIM_STA;
				//printf("22222\n");
				/*
				pthread_mutex_lock(&pmMUT);
				write(iUartfd, "AXX+TIM+STA&", strlen("AXX+TIM+STA&"));
			//	usleep(1000 * 20);
			//	write(iUartfd, "AXX+TIM+STA&", strlen("AXX+TIM+STA&"));
				pthread_mutex_unlock(&pmMUT);
				bySendBuf = NULL;
				continue;
				*/
			}
			else if (0 == strncmp(byReadBuf, ALEXA_ALER_END, sizeof(ALEXA_ALER_END) - 1))
			{
				bySendBuf = AXX_TIM_END;
				/*
				pthread_mutex_lock(&pmMUT);
				write(iUartfd, "AXX+TIM+END&", strlen("AXX+TIM+END&"));
				usleep(1000 * 20);
				write(iUartfd, "AXX+TIM+END&", strlen("AXX+TIM+END&"));
				pthread_mutex_unlock(&pmMUT);
				bySendBuf = NULL;
				continue;
				*/
			}
			else if (0 == strncmp(byReadBuf, ALEXA_ALER_END_IDENTIFY_START, sizeof(ALEXA_ALER_END_IDENTIFY_START) - 1))
			{
				bySendBuf = AXX_TIM_PAS;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_NOTIFICATIONS_START, sizeof(ALEXA_NOTIFICATIONS_START) - 1))
			{
				bySendBuf = AXX_NTF_STA;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_NOTIFACATIONS_END, sizeof(ALEXA_NOTIFACATIONS_END) - 1))
			{
				bySendBuf = AXX_NTF_END;
			}
			else if (0 == strncmp(byReadBuf, WIFI_NOT_CONNECTED, sizeof(WIFI_NOT_CONNECTED) - 1))
			{
				bySendBuf = AXX_NET_ERR;
			}
			else if (0 == strncmp(byReadBuf, ALEXA_NOT_LOGIN, sizeof(ALEXA_NOT_LOGIN) - 1))
			{
				bySendBuf = AXX_ALE_ERR;
			}
			else if (0 == strncmp(byReadBuf, CAPGET, sizeof(CAPGET) - 1))
			{
				bySendBuf = AXX_CAP_GET;
			}
			else if( 0 == strncmp(byReadBuf, IFLYTEK, sizeof(IFLYTEK) - 1)) 
			{
				if(iflytek_file == 0) 
				{
					iflytek_file = 1;
				} else 
				{	
					iflytek_file = 0;
				}
			}
			else if(0 == strncmp(byReadBuf, BT_SEARCH, sizeof(BT_SEARCH) - 1)) {
				bySendBuf = CMD_EMIT_INQ;
			}
			else if(0 == strncmp(byReadBuf, BT_IN, sizeof(BT_IN) - 1)) {
				char index[3] = {0}, addr[13] = {0};
				char run[64];
				sscanf(&byReadBuf[sizeof(BT_IN)-1], "%2s %12s", index, addr);
				bySendBuf = CMD_IN;
				bySendBuf[6] = index[0];
				bySendBuf[7] = index[1];
				bySendBuf[9] = addr[0];
				bySendBuf[10] = addr[1];
				bySendBuf[11] = addr[2];
				bySendBuf[12] = addr[3];
				bySendBuf[13] = addr[4];
				bySendBuf[14] = addr[5];
				bySendBuf[15] = addr[6];
				bySendBuf[16] = addr[7];
				bySendBuf[17] = addr[8];
				bySendBuf[18] = addr[9];
				bySendBuf[19] = addr[10];
				bySendBuf[20] = addr[11];
				snprintf(run, sizeof(run), "echo %s > "BT_ADDR, addr);
				my_system(run);
			}
			else if(0 == strncmp(byReadBuf, BT_CON, sizeof(BT_CON) - 1)) {
				bySendBuf = CMD_EMIT_CON;
			}
			else if(0 == strncmp(byReadBuf, BT_DIS, sizeof(BT_DIS) - 1)) {
				bySendBuf = CMD_EMIT_DISCON;
			}
			else if(0 == strncmp(byReadBuf, BT_LINK, sizeof(BT_LINK) - 1)) {
				bySendBuf = CMD_EMIT_LINK;
			}
			else if(0 == strncmp(byReadBuf, UPWIFI, sizeof(UPWIFI) - 1)) {
				bySendBuf = AXX_UP_WIFI;
			}
			else if (0 == strncmp(byReadBuf, UPBT, sizeof(UPBT) - 1))
			{
				bt_flage = 1; 
				int BufLen;
				int fd2, len,Length;							
				int PacketSize = 4*1024;
				char buf[3+PacketSize+2];
				int index;
				unsigned int CRC;
				int j = 0, i = 0;
				int ret = 0;
				int GetInBufferCount= -1;
				unsigned char dataPtr[1] = {0};
				struct stat st;	
				FILE *fp;
				int normal_state = 0;
				int error_state = 1;
				int handle; char string[40];
			 	int length, res;

				if (stat(F_PATH, &st) < 0)
				{
					printf("Open /tmp/bt_upgrade.bin failed \n");
					close(fd);
					fd = -1;
					continue ;
				}
				fd2 = open(F_PATH, O_RDONLY);
				if (fd2 < 0)
				{
					printf("Can't open /tmp/bt_upgrade.bin\n");
					close(fd);
					fd = -1;
					continue ;
				}
				printf("Bt firwmare size = %d\n",st.st_size);
				i = st.st_size%PacketSize;
				if(i != 0)	BufLen = ((st.st_size/PacketSize) + 1) * PacketSize;
				else BufLen = st.st_size;
				if (st.st_size % (512*1024) != 0)//鍒ゆ柇钃濈墮鍥轰欢鏄惁姝ｇ‘
				{
					printf("Firmware size must be 512K integer multiples faile\n");
					close(fd);
					fd = -1;
					continue;
				}
				unsigned char *Ptr;
				uint16_t crc_val = 0;
				char bufmac[18] = {0};
				memset(buf,0,sizeof(buf));
				index = 0;
				int Tick =0;
				char bufflash[1];
				char bufflash1[4*1024];
				i = 0;
				ret = write(iUartfd, AT_UP_DATE, strlen(AT_UP_DATE));

				while(++Tick < 650*10000){
					pthread_mutex_lock(&pmMUT);
					ret = 0;
					memset(bufmac,0,sizeof(bufmac));
					ret = read(iUartfd,bufmac,sizeof(bufmac));
					pthread_mutex_unlock(&pmMUT);
			//		DEBUG_INFO(" read data from mcu ret=%d",ret);		
					if (ret >= 18){
						Ptr = bufmac;
						ret = 0;
						while((*Ptr) != 'C' && ret < 18){
								Ptr++;
								ret++;
						}															
						if( (*Ptr) != 'C'){//67-->'C'
							continue;
						 }else {
						/*	crc_val = cal_crc(Ptr,7,0x55aa);
							if(((crc_val&0x0ff)==Ptr[7])&&((crc_val>>8)==Ptr[8])){
							 	Ptr++;
								memcpy(bt_mac_frame,Ptr,6);
							 }*/
							for ( i = 0;i < 6;i ++){
								bt_mac_frame[i] = *(++Ptr);
								printf(" %02x ",bt_mac_frame[i]);
									
							}
							printf("\n Get bt mac  success.begin erase flash \n");
							 break;
							}			
					}
				}
				if(Tick >= 650*10000){
					printf("Get bt addr timeout\n");
					cdb_set_int("$bt_update_status",normal_state);
					cdb_set_int("$wifi_bt_update_flage",1);
					continue;
					}
			
	
				ret = read(iUartfd,bufflash1,sizeof(bufflash1));
				memset(bufflash1,0,sizeof(bufflash1));

				for(;;){
					cdb_set_int("$bt_update_status",error_state);
					buf[0] = 'E';
					buf[1] = (st.st_size/1024)&0xff;
					buf[2] = ((st.st_size/1024)>>8)&0xff;
					pthread_mutex_lock(&pmMUT);
					ret = 0;
					ret = write(iUartfd,buf,3);
					pthread_mutex_unlock(&pmMUT);
					Tick =0;

					GetInBufferCount = -1;
					pthread_mutex_lock(&pmMUT);
					while(++Tick <65*10000) 
					{		
						usleep(1);
						GetInBufferCount = read(iUartfd,bufflash,sizeof(bufflash));
						Ptr = bufflash;
						 if(*Ptr == 'E') {//69--> 'E'
						 break;
						 }
					}

					pthread_mutex_unlock(&pmMUT);
					if(Tick >= 65*10000)	{
						printf("erase flash timeout .try again\n");
						cdb_set_int("$bt_update_status",normal_state);
						cdb_set_int("$wifi_bt_update_flage",1);
						break;
					}

					 if(*Ptr == 'E') //69--> 'E'
						 break;

				}	

				printf("erase flash success \n");
				index = 1;
				int flage = 1,k;
				float Step;
				int Cursor = 0;
				unsigned char bufrev[1] = {0};
				Step = (float)BufLen/100;
				uint16_t tf_crc_val;
				int index_cnt = 1;
				int write_num = 0;
				int write_size = 0;
				while((len = read(fd2, &buf[3], PacketSize) )> 0){
					if (index >=5){//前16k为蓝牙BootLoader，直接跳过不写
				/*	if (index_cnt == 252){	//进行最后的CRC校验
						 tf_crc_val = cal_crc(&buf[3],PacketSize-2,0x55aa);
						buf[PacketSize + 1] = (uint8_t)tf_crc_val;
						buf[PacketSize + 2] = (uint8_t)(tf_crc_val>>8);
					}else{
						 tf_crc_val = cal_crc(&buf[3],PacketSize,0x55aa);
					}*/
					if (index == 66){	//在0x41063处写入获取的蓝牙地址，保持蓝牙地址与升级前一致
						memcpy(&buf[3 + 96 +3],bt_mac_frame,6);
					}
					buf[0] = 0x01;//SOH
					buf[1] = index_cnt;
					buf[2] = ~index_cnt;
					CRC = CaculateCRC(buf, PacketSize);
					buf[3+PacketSize] = CRC>>8;
					buf[3+PacketSize+1] = CRC;
					pthread_mutex_lock(&pmMUT);
					ret = write(iUartfd, buf, sizeof(buf));
					pthread_mutex_unlock(&pmMUT);

					GetInBufferCount = -1;
					memset(bufrev,0,sizeof(bufrev));
					Tick = 0;
					pthread_mutex_lock(&pmMUT);
					while(++Tick < 65*10000) {							
						usleep(1);
						GetInBufferCount = read(iUartfd,bufrev,sizeof(bufrev));
						Ptr = bufrev;
						 if((*Ptr == 0x06) ||(*Ptr == 0x18)) {//ACK-->0x06  0x18-->CAN
							 break;
						 }
					}
					pthread_mutex_unlock(&pmMUT);
					if(Tick >= 65*10000){
						printf("*Ptr = %x  Transfer to the  %d frame timeout failed\n",*Ptr,index);
					//	goto ack;
						break;					
					}
					if (GetInBufferCount > 0){
						if(*Ptr != 0x06 && *Ptr != 0x18) //ACK-->0x06  0x18-->CAN
						 {
							printf("*Ptr = 0x%x Transfer to the  %d frame  failed\n",*Ptr ,index);
							break;
						 }
						else if(*Ptr++ == 0x18 || *Ptr== 0x18)
						 {
							printf("CRC faile \n");	
						 }
					}else{
						printf("don't konw the faile\n");
					}
				
				k = i + PacketSize;
	  	        while(k-Step*Cursor>-0.003){	
				Cursor++;

			    if ((handle = open(PROGRESS_DIR, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
			    {
			        printf("Error opening file.\n");
			        exit(1);
			    }
			    sprintf(string,"%d.0%%",Cursor);
				fprintf(stderr,"\r%s", string);
			   // printf("\n%s ",string);	
			    length = strlen(string);
			    if ((res = write(handle, string, length)) != length)
			    {
			        printf("Error writing to the file.\n");
			        exit(1);
			    }
			 //   printf("Wrote %d bytes to the file.\n", res);
			    close(handle); 				
				}
				i += PacketSize;
				memset(buf,0,sizeof(buf));
				*Ptr = 0;
				index_cnt++;
				}
	
				index++;
				}
				
				cdb_set_int("$bt_update_status",normal_state);
				buf[0] = 0x04;//EOT
				i = 0;
				pthread_mutex_lock(&pmMUT);
				write(iUartfd, buf, 1);
				pthread_mutex_unlock(&pmMUT);
				close(fd2);
				close(fd);
				dataPtr[0] = 100;
				if ((handle = open(PROGRESS_DIR, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
			    {
			        printf("Error opening file.\n");
			        exit(1);
			    }
			    sprintf(string,"%d.0%%",dataPtr[0]);
			    printf("\n%s bt up success \n",string);	
			    length = strlen(string);
			    if ((res = write(handle, string, length)) != length)
			    {
			        printf("Error writing to the file.\n");
			        exit(1);
			    }
			 //   printf("Wrote %d bytes to the file.\n", res);
			    close(handle); 

				bt_flage = 0;
				continue;
			}
			else
			{
				close(fd);
				fd = -1;
				continue;
			}
	exit:
			pthread_mutex_lock(&pmMUT);
			DEBUG_INFO("bySendBuf=[%s]\n",bySendBuf);
			write(iUartfd, bySendBuf, strlen(bySendBuf));
			pthread_mutex_unlock(&pmMUT);
			usleep(20*1000);

		}
		close(fd);
		fd = -1;
	}
}


