#ifndef __CURL_HTTPS_REQUEST_H__
#define __CURL_HTTPS_REQUEST_H__


//typedef enum {
#define ALEXA_EVENT_SPEECH_RECOGNIZE			0
#define ALEXA_EVENT_SYN_STATE					1
#define ALEXA_EVENT_PLAYBACK_STARTED			2
#define ALEXA_EVENT_PLAYBACK_PAUSED				3
#define ALEXA_EVENT_PLAYBACK_STOP				4
#define ALEXA_EVENT_PLAYBACK_NEARLYFINISHED		5
#define ALEXA_EVENT_PLAYBACK_FINISHED			6
#define ALEXA_EVENT_PLAYBACK_FAILED				7
//#define ALEXA_EVENT_PLAYBACK_CONTROLLER			8  // 对应三个按键事件
#define ALEXA_EVENT_SPEECH_STARTED				9
#define ALEXA_EVENT_SPEECH_FINISHED				10
#define ALEXA_EVENT_VOLUME_CHANGED				11
//#define ALEXA_EVENT_STARTED_ALERT				12
#define ALEXA_EVENT_PINGPACKS                   13

#define ALEXA_EVENT_MPCPLAYSTOP					14  // 播放/暂停按键按下
#define ALEXA_EVENT_MPCPREV						15  // 上一曲按键按下
#define ALEXA_EVENT_MPCNEXT						16  // 下一曲按键按下

#define ALEXA_EVENT_STARTED_ALERT				17
#define ALEXA_EVENT_BACKGROUND_ALERT			18
#define ALEXA_EVENT_FOREGROUND_ALERT			19
#define ALEXA_EVENT_SET_ALERT					20
#define ALEXA_EVENT_STOP_ALERT					21
#define ALEXA_EVENT_DELETE_ALERT				22




#define ALEXA_EVENT_REPORTUSERINACTIVITY		23
#define ALEXA_EVENT_REFRESHTOKEN				24	

#define ALEXA_EVENT_PROGRESSREPORT_DELAYELAPSED 25
#define ALEXA_EVENT_PROGRESSREPORT_INTERVALELAPSED 26

#define ALEXA_EVENT_MUTECHANGE					27

#define ALEXA_EVENT_SETTING						28

#define ALEXA_EVENT_PLAYBACK_QUEUECLEAR			29

#define ALEXA_EVENT_PLAYBACK_STUTTERSTARTED		30
#define ALEXA_EVENT_PLAYBACK_STUTTERFINISHED	31

//}AlexaEvent;

#define DOWN_CHANNEL_STREAM_INDEX  0
#define RECOGNIZER_INDEX    1

extern int getToken(void);
extern void Init_easy_handler(void);
extern void CreateCurlHttpsRequestPthread(void);
extern void submit_PingPackets(void);
extern void Clear_easy_handler(CURL * curl_handle);

#endif /* __CURL_HTTPS_REQUEST_H__ */



