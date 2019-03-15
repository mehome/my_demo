#ifndef __GLOBAL_PARAM_H__
#define __GLOBAL_PARAM_H__

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <mpd/client.h>
#include <curl/curl.h>
#include <sys/wait.h>  
#include <sys/types.h>
#include <alsa/asoundlib.h>

#include <avsclient/libavsclient.h>
#include <libcchip/function/uartd_fifo/uartd_fifo.h>

#include "fifobuffer.h"
#include "UPStatusMonitor.h"
#include "debug.h"
#include "c_types.h"
#include "Curl_Https_Request.h"
#include "Queue/queue.h"
#include "mpc/getMpdStatus.h"
#include "mpc/mpdcli.h"
#include "ProcessJsonDate.h"
#include "alexa_json.h"
#include "uuid.h"
#include "common.h"
#include "mp3player/mp3player.h"
#include "mpc/status.h"
#include "alert/AlertHandler.h"
#include "alert/list.h"
#include "alert/list_iterator.h"

#include "authDelegate/authDelegate.h"
#include "capture/capture.h"
#include "wakeup/wakeup.h"

#define AMAZON_DOWNCHANNEL_REQUEST    0x01
#define AMAZON_SYSN_REQUEST   		  0x02
#define AMAZON_INIT_FLAG      		  0x03


#define RECORD_VAD_START 	0
#define RECORD_VAD_END    	1

#define REMOTE_VAD_START			0
#define REMOTE_VAD_END				1


#define MAX_CONNECT_STREM 10

#define TIMER_REFRESH_TOKEN 10

#define SUBMIT_USER_INACTIVITY 12

#define CAPTURE_FIFO_LENGTH (1024 * 400) // 录音400k
#define DECODE_FIFO_BUFFLTH (16384 * 20) // 下载160k
#define PLAY_FIFO_BUFFLTH   (1024 * 200) // 播放200K


#define NONE "\033[0m"   			//正常终端颜色
#define RED "\033[0;32;31m"    		//红色
#define LIGHT_RED "\033[1;31m"  	//粗体红色
#define GREEN "\033[0;32;32m"    	//绿色
#define LIGHT_GREEN "\033[1;32m"  
#define BLUE "\033[0;32;34m"     	//蓝色
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"   	//暗灰色
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"  	//淡紫色
#define YELLOW "\033[1;33m"      	//黄色
#define WHITE "\033[1;37m"    		//粗体白色

#define ALEXA_LOGIN_SUCCED		"LogInSucceed"		// 登录成功
#define ALEXA_WAKE_UP 			"WakeUp"			// 远场唤醒
#define ALEXA_CAPTURE_START 	"CaptureStart"		// 录音开始
#define ALEXA_CAPTURE_END		"CaptureEnd"		// 录音结束
#define ALEXA_IDENTIFY_OK		"Identify_OK"		// 识别成功
#define ALEXA_IDENTIFY_ERR		"Identify_ERR"		// 识别失败
#define ALEXA_VOICE_PLAY_END	"VoicePlaybackEnd" 	// 播放回复语音完成
#define ALEXA_EXPECTSPEECH		"tlkon"				// 启动第二次交互
#define ALEXA_ALER_END			"AlexaAlerEnd"
#define ALEXA_ALER_START		"AlexaAlerStart"

#define ALEXA_NOTIFICATIONS_START	"NotificationsStart"
#define ALEXA_NOTIFACATIONS_END		"NotificationsEnd"


#define BASE_URL         "https://avs-alexa-na.amazon.com"
#define DIRECTIVES_PATH "https://avs-alexa-na.amazon.com/v20160207/directives"
#define EVENTS_PATH      "https://avs-alexa-na.amazon.com/v20160207/events"
#define PING_PATH      "https://avs-alexa-na.amazon.com/ping"

#define AMAZON_AUDIOPLAYER_PLAY       1
#define AMAZON_AUDIOPLAYER_PLAYING    2
#define AMAZON_AUDIOPLAYER_STOP		 3
#define AMAZON_AUDIOPLAYER_FINISH 	 4


/**
 * Information about MPD's current status.
 */
typedef struct  status{
	/** 0-100, or MPD_STATUS_NO_VOLUME when there is no volume support */
	int volume;

	/** Queue repeat mode enabled? */
	bool repeat;

	/** Random mode enabled? */
	bool random;

	/** Single song mode enabled? */
	bool single;

	/** Song consume mode enabled? */
	bool consume;

	/** Number of songs in the queue */
	unsigned queue_length;

	/**
	 * Queue version, use this to determine when the playlist has
	 * changed.
	 */
	unsigned queue_version;

	/** MPD's current playback state */
	enum mpd_state state;

	/** crossfade setting in seconds */
	unsigned crossfade;

	/** Mixramp threshold in dB */
	float mixrampdb;

	/** Mixramp extra delay in seconds */
	float mixrampdelay;

	/**
	 * If a song is currently selected (always the case when state
	 * is PLAY or PAUSE), this is the position of the currently
	 * playing song in the queue, beginning with 0.
	 */
	int song_pos;

	/** Song ID of the currently selected song */
	int song_id;

	/** The same as song_pos, but for the next song to be played */
	int next_song_pos;

	/** Song ID of the next song to be played */
	int next_song_id;

	/**
	 * Time in seconds that have elapsed in the currently
	 * playing/paused song.
	 */
	unsigned elapsed_time;

	/**
	 * Time in milliseconds that have elapsed in the currently
	 * playing/paused song.
	 */
	unsigned elapsed_ms;

	/** length in seconds of the currently playing/paused song */
	unsigned total_time;

	/** current bit rate in kbps */
	unsigned kbit_rate;

	/** the current audio format */
	struct mpd_audio_format audio_format;

	/** non-zero if MPD is updating, 0 otherwise */
	unsigned update_id;

	/** error message */
	char *error;

	// by hu
	/** mpd string state*/
	char mpd_state[10];

	/** user mpc state*/
	char user_state[10];

	int offsetInMilliseconds;

	int delayInMilliseconds;

	int intervalInMilliseconds;
}mpd_status;

typedef struct CURL_EASY_HANDLE
{
	CURL *easy_handle;
	char *BodyBuff;	
	struct curl_slist *httpHeaderSlist;
	struct curl_httppost *formpost;
	struct curl_httppost *lastptr;
}CURL_EASY_HANDLER;

// 信号量对象
typedef struct _PRIVINFO  
{  
	sem_t Capture_sem;
	sem_t MadPlay_sem;
	sem_t WakeUp_sem;
}PRIVINFO; 

typedef struct _AMAZON_CONNCET  
{  
	pthread_mutex_t connect_mtx;
	UINT8 conncet_status;
}AMAZON_CONNCET_STATUS; 

typedef struct _AMAZON_RECORD_VAD  
{  
	pthread_mutex_t VAD_mtx;
	UINT8 byVadFlag; // 0 vad开始 1vad结束
}AMAZON_RECORD_VAD_FLAG; 


typedef struct _ALERT_{
	char *alert_started_json_str;
	char *alert_background_json_str;
	char *alert_foreground_json_str;
	char *alert_set_json_str;
	char *alert_stop_json_str;
	char *alert_delete_json_str;
}ALERT_STRUCT;



typedef struct NOTIFICATIONS{
	char m_IsEnabel;
	char m_isVisualIndicatorPersisted;
}NOTIFICATIONS_STRUCT;

typedef struct PLAY_DIRECTIVE{
    char *m_PlayToken;
	char *m_PlayUrl;
	int offsetInMilliseconds;	// 偏移量
	int delayInMilliseconds; 
	int intervalInMilliseconds;
	char byPlayType;
}PLAY_DIRECTIVE_STRUCT;

typedef struct END_POINT{
	char *m_directives_path;
	char *m_event_path;
	char *m_ping_path;
	char m_byDownChannelFlag;
}END_POINT_STRUCT;

typedef struct GLOBALPRARM
{
	pthread_mutex_t Clear_easy_handler_mtx;
	pthread_mutex_t Handle_mtc;
    CURLM *multi_handle;	
	CURL *speech_easy_handle;
	CURL_EASY_HANDLER m_RequestBoby[MAX_CONNECT_STREM];
	UINT8 m_ExpectSpeechFlag;
	mpd_status m_mpd_status;
	PRIVINFO Semaphore;
	AMAZON_CONNCET_STATUS connect;
	AMAZON_RECORD_VAD_FLAG vadFlag;
	char m_AudioFlag;         // 有语音回复
	char m_PlayMusicFlag;     // 播放音乐标志，一般和m_AudioFlag配套使用
	char m_PlayerStatus;      // 亚马逊音乐播放状态，1 开始播放 2 正在播放 3 停止/暂停 4 完成
	char m_CaptureFlag; 	  // 0、空闲 1、开始录音 2、正在录音 3、录音结束
	char m_TTSAudioFlag;	  // 播放语音回答的状态。0、空闲 1、正在播放TTS回复
	char m_localValume;
	char m_AmazonInitIDFlag;
	char m_AmazonStopCapture;
	char m_AmazonMuteOldStatus;
	char m_AmazonMuteStatus;
    char *m_PlayToken;
	char *m_PlayUrl;
	char *m_SpeechToken;
	char m_byAudioRequestFlag; // 语音请求标志
	char m_bySpeechFlag;
	char m_byAbortCallback;    // 请求终止
	ALERT_STRUCT m_alert_struct;
	NOTIFICATIONS_STRUCT m_Notifications;
	UINT32 m_iVadThreshold;
	char m_RemoteVadFlag;
	char m_byPlayBookFlag;
	list_t *play_directive;
	PLAY_DIRECTIVE_STRUCT CurrentPlayDirective;
	snd_pcm_uframes_t m_chunk_size;
	char m_byVADCont;
	char m_byrecordFlag;
	char m_byMuteFlag;
	END_POINT_STRUCT endpoint;
	int m_PlayMode;
}GLOBALPRARM_STRU;


extern void SystemPara_Init(void);

#define USE_EVENT_QUEUE

#endif /* __GLOBAL_PARAM_H__ */


