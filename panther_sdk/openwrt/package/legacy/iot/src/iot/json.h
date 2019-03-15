#ifndef __JSON_H__
#define __JSON_H__

#include "common.h"
#include "myutils/debug.h"
#include "DeviceStatus.h"
#include "TuringParam.h"


extern int translation_flag;

enum {	
	STATUS_DEVICE_TYPE = 0,
	STATUS_AUDIO_TYPE,
	STATUS_EMERG_TYPE,
	STATUS_MUSIC_TYPE,
	STATUS_STORY_TYPE,
};

enum {	
	PLAY_MODE_IDLE = 0,    
	PLAY_MODE_LOCAL_MUSIC, 
	PLAY_MODE_IMME_SPEECH, 
	PLAY_MODE_LEAVE_WORD, 
	PLAY_MODE_POST_MUSIC, 
	PLAY_MODE_PAUSE,	
	PLAY_MODE_CAPTURE,	
	PLAY_MODE_SAFETY,	
	PLAY_MODE_PHONE_CALL,	
};

enum {	
	PLAY_STATUS_STOP = 0,  
	PLAY_STATUS_PLAY, 
	PLAY_STATUS_PAUSE, 
	PLAY_STATUS_RESUME, 
};

enum {	
	IOT_MQTT_TOPIC = 0,
	IOT_STATUS_NOTIFY,
	IOT_STATUS_GET,
	IOT_FILE_UPLOAD,
	IOT_MESSAGE_SEND,
	IOT_DATA_GET,
};

extern char *CreateDeviceUnbindMessageReportJson(char *key, char *deviceId);
extern char *CreateIntercomMessageReportJson(char *key, char *deviceId, char *fromUser, char *mediaId);

extern char *CreateAudioStatusReportJson(char *key, char *deviceId, AudioStatus *audioStatus);

extern char *CreateStatusGetJson(char *key, char *deviceId, int type);
extern char *CreateEmergStatusReportJson(char *key, char *deviceId);

extern char *CreateSynchronizeLocalMusicReportJson(char *key, char *deviceId, int operate,char *names);
extern char *CreateSynchronizeLocalStoryReportJson(char *key, char *deviceId, int operate,char *names);


extern char *CreateIotDataReportJson(char *key, char *deviceId);
extern char *CreateGetDataReportJson(char *key, char *deviceId, int type);
extern char *CreateCollecSongJson(char *key, char *deviceId, int id);
extern char *CreateAudioGetJson(char *key, char *deviceId, int type);

extern      char *CreateRequestJson(char *key, char *userid, char *token, int iLength, int index, int realTime, int tone, int type);


#endif

