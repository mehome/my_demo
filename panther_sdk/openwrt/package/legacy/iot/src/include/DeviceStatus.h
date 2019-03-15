
#ifndef __DEVICE_STATUS_H__

#define __DEVICE_STATUS_H__

enum   {
	EVENT_GET_STAUTS = 0,
	EVENT_GET_DATA,
	EVENT_GET_AUDIO,
	EVENT_UPLOAD_FILE,
	EVENT_SEND_MESSAGE,
	EVENT_REPROT_IOT_STATUS,
	EVENT_REPROT_DEVICE_STATUS,
	EVENT_REPROT_AUDIO_STATUS,
	EVENT_REQUEST_TOPIC, 
	EVENT_GET_WECHAT_MSG,
	EVENT_VENDOR_AUTHORIZE,
	EVENT_QUERY_DEVICE_STATUS,
	EVENT_COLLECT_SONG,

	EVENT_GET_MQTT_INFO,
	EVENT_MQTT_START, 
};
typedef struct SleepStatus
{
	int on;
	long bed;
	long wake;
}SleepStatus;
typedef struct DeviceStatus
{
	int vol; 			// volume  0 - 100
	int battery; 		// battery 0 - 100
	int sfree; 			// storagecurrent 
	int stotal;			// storagetotal 
	int shake; 			// shakeswitch
	int power;			// low powervalue
	int	bln;	 		// blnswitch;
	int play;			//playmode
	int charging;
	int lbi;
	int tcard;
	SleepStatus sleepStatus;
	
}DeviceStatus;

enum
{
	TYPE_TTL_URL = 0,
	TYPE_FILE_URL,
};
	
typedef struct audio_status 
{
	int type; 		// 
	char *title; 	
	char *mediaId;  //mediaId
	char *artist;  //artist
	char *url;		//
	char *tip;		//
	unsigned int duration;   //
	int play;		//playstatus 0-stop;1-play;2-pause;4-resume;
	int state;     	//
	unsigned int progress;	//
	int repeat;
	int random;
	int single;
	
}AudioStatus;


typedef struct topic_event
{
	char *clientId;
	char *topic;
}TopicEvent;

typedef struct media_id_event
{
	char *mediaId;
}MediaIdEvent;

typedef struct from_user_event
{
	char *fromUser;
}FromUserEvent;
typedef struct mqtt_event
{
	char *message;
}MqttEvent;

typedef struct iot_event
{
	char *body;
}IotEvent;

typedef struct audio_event
{
	char *body;
}AudioEvent;

typedef struct inter_chat_event
{
	int code;
	char *pttsUrl;
	char *pfileUrl;
	char *pttsUrlAfter;
	char *tip;
	int id;
	char *title;
	char *author;
}IntelChatEvent;


typedef struct upload_file
{
	char *file;
	char *body;
	int type;
}UploadFileEvent;


#endif
