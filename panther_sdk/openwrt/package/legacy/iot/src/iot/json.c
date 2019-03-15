#include "json.h"
#include "mon_config.h"
#include <json/json.h>
#include "myutils/debug.h"
#include "common.h"
#include "init.h"



/* 语音交互上传json参数. */
char *CreateRequestJson(char *key, char *userid, char *token, int iLength, int index, int realTime, int tone, int type)
{
	char * json_str = NULL;
	json_object *root = NULL;

	root = json_object_new_object();

	json_object_object_add(root, "ak", json_object_new_string(key));
	json_object_object_add(root, "uid", json_object_new_string(userid)); 
#ifdef ASR_PCM_16K_16BIT
	json_object_object_add(root, "asr", json_object_new_int(0));
#endif
#ifdef ASR_PCM_8K_16BIT
	json_object_object_add(root, "asr", json_object_new_int(1));
#endif
/*
#ifdef ASR_AMR_8K_16BIT
	json_object_object_add(root, "asr", json_object_new_int(2));
#endif
*/
#ifdef ASR_AMR_16K_16BIT
	json_object_object_add(root, "asr", json_object_new_int(3));
#endif
#ifdef ENABLE_OPUS
	json_object_object_add(root, "asr", json_object_new_int(4));
#endif
	json_object_object_add(root, "tts", json_object_new_int(1));

	json_object_object_add(root, "realTime", json_object_new_int(realTime)); 

	json_object_object_add(root, "index", json_object_new_int(index));

	json_object_object_add(root, "identify", json_object_new_string(g.identify));
	json_object_object_add(root, "speed", json_object_new_int(5));
	json_object_object_add(root, "flag", json_object_new_int(3));

	if (token) {
		json_object_object_add(root, "token", json_object_new_string(token));
	}
	else
		json_object_object_add(root, "token", json_object_new_string(""));

	json_object_object_add(root, "type", json_object_new_int(type));
    
#if defined(CONFIG_PROJECT_BEDLAMP_V1)  
	json_object_object_add(root, "tone", json_object_new_int(11));
#elif defined(CONFIG_PROJECT_K2_V1)
    json_object_object_add(root, "tone", json_object_new_int(tone));
#else
	json_object_object_add(root, "tone", json_object_new_int(tone));
#endif
	json_object_object_add(root, "volume", json_object_new_int(9));

#if defined(CONFIG_PROJECT_K2_V1)
	info("Translation.translation_flag = %d...................",translation_flag);
	if(translation_flag)	//中英翻译组合
	{
		translation_flag=0;
		json_object *devices = NULL;
		devices = json_object_new_array();
		json_object_array_add(devices, json_object_new_int(20028));
		json_object_object_add(root, "seceneCodes", devices);
	}
#endif    
	json_str = strdup(json_object_to_json_string(root));
	json_object_put(root);
//turing/turing.c:send_data_to_server_by_socket:480: jsonData:{ "ak": "8e3941df07b14bc183766ebd51a3b8b0", "uid": "721acc90467547379944aa3180812080", "asr": 4, "tts": 1, "realTime": 1, "index": -1, "identify": "9a3bf2bc-9ec7-95ee-edce-0f53e4d2c105", "speed": 5, "flag": 3, "token": "", "type": 0, "tone": 0, "volume": 9 }
	return json_str;
}


/* 低电量请求数据 */
char *CreateEmergStatusReportJson(char *key, char *deviceId)
{
	char * json_str = NULL;
	json_object *root=NULL, *status= NULL;
	int battery;
	battery = GetDeviceBattery();
	root = json_object_new_object();
	status = json_object_new_object();
	
	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	json_object_object_add(root, "type", json_object_new_int(STATUS_EMERG_TYPE));
	json_object_object_add(status, "battery", json_object_new_int(battery));
	json_object_object_add(root, "status", status);
	json_str = strdup(json_object_to_json_string(root));

	json_object_put(root);

	return json_str;
}

/* 获取状态请求数据 */
char *CreateStatusGetJson(char *key, char *deviceId, int type)
{
	char * json_str = NULL;
	json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	json_object_object_add(root, "type", json_object_new_int(type));

	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}
/* 获取音频资源请求数据 */
char *CreateAudioGetJson(char *key, char *deviceId, int type)
{
	char * json_str = NULL;
	json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	json_object_object_add(root, "type", json_object_new_int(type));

	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}
/* 设备收藏歌曲请求数据 */
char *CreateCollecSongJson(char *key, char *deviceId, int id)
{
	char * json_str = NULL;
	json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	json_object_object_add(root, "id", json_object_new_int(id));

	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}
/* 获取topic和clientId请求数据 */
char *CreateTopicJson(char *key, char *deviceId)
{
	char * json_str = NULL;
	json_object *root = NULL;
	root = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));

	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}
/* 获取数据请求数据 */
char *CreateGetDataReportJson(char *key, char *deviceId, int type)
{
	char * json_str = NULL;
	
	json_object *root=NULL, *status= NULL;
	
	
	root = json_object_new_object();


	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	json_object_object_add(root, "type", json_object_new_int(type));

	json_str = strdup(json_object_to_json_string(root));
	
	json_object_put(root);

	return json_str;
}
/* 设备状态请求数据 */
char *CreateDeviceStatusReportJson(char *key, char *deviceId)
{
	char * json_str = NULL;
	
	json_object *root=NULL, *status= NULL;
	json_object *sleep=NULL;
	
	
	root = json_object_new_object();
	status = json_object_new_object();
	sleep = json_object_new_object();
	DeviceStatus deviceStatus; 
	GetDeviceStatus(&deviceStatus);		//获取当前设备状态
	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	json_object_object_add(root, "type", json_object_new_int(STATUS_DEVICE_TYPE));
	#if 0
	int vol , battery , power, sfree, stotal, shake, bln, play;
	vol  	= GetDeviceVolume();
	battery = GetDeviceBattery();
	sfree 	= GetDeviceSFree();
	stotal 	= GetDeviceSTotal();
	play 	= GetPlayState();
	#endif
	json_object_object_add(status, "vol", json_object_new_int(deviceStatus.vol));
	json_object_object_add(status, "battery", json_object_new_int(deviceStatus.battery));
	json_object_object_add(status, "sfree", json_object_new_int(deviceStatus.sfree));
	json_object_object_add(status, "stotal", json_object_new_int(deviceStatus.stotal));
	json_object_object_add(status, "shake", json_object_new_int(deviceStatus.shake));
	json_object_object_add(status, "power", json_object_new_int(deviceStatus.power));
	json_object_object_add(status, "bln", json_object_new_int(deviceStatus.bln));
	json_object_object_add(status, "play", json_object_new_int(deviceStatus.play));
	json_object_object_add(status, "lbi", json_object_new_int(deviceStatus.lbi));
	json_object_object_add(status, "tcard", json_object_new_int(deviceStatus.tcard));
	json_object_object_add(status, "charging", json_object_new_int(deviceStatus.charging));
	json_object_object_add(sleep, "on",  json_object_new_int(deviceStatus.sleepStatus.on));
	json_object_object_add(sleep, "bed",  json_object_new_int(deviceStatus.sleepStatus.bed));
	json_object_object_add(sleep, "wake",  json_object_new_int(deviceStatus.sleepStatus.wake));
	json_object_object_add(status, "sleep", sleep);
	json_object_object_add(root, "status", status);
	json_str = strdup(json_object_to_json_string(root));
	
	json_object_put(status);
	json_object_put(root);

	return json_str;
}
/* 设备认证请求数据 */
char *CreateDeviceVendorAuthorizeJson(char *key, char *productId, char *deviceId, char *mac)
{
	char * json_str = NULL;
	json_object *root=NULL, *status= NULL;
	root = json_object_new_object();
	info(".....");	

	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "productId", json_object_new_string(productId));

	json_object *devices = NULL, *item = NULL;
	
	devices = json_object_new_array();
	item = json_object_new_object();
	json_object_object_add(item, "deviceId", json_object_new_string(deviceId));
	json_object_object_add(item, "mac", json_object_new_string(mac));
	
	json_object_array_add(devices, item);
	json_object_object_add(root, "devices", devices);

	json_str = strdup(json_object_to_json_string(root));
	//info("json_str:%s" ,json_str);
	
	json_object_put(root);

	return json_str;
}
/* 查询设备状态请求数据 */
char *CreateDeviceVendorStatusQueryJson(char *key, char *deviceId)
{
	char * json_str = NULL;
	json_object *root=NULL, *status= NULL;
	root = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));
	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	
	json_str = strdup(json_object_to_json_string(root));
	//info("json_str:%s",json_str);
	json_object_put(root);

	return json_str;
}
/* 设备解绑请求数据 */
char *CreateDeviceUnbindMessageReportJson(char *key, char *deviceId)
{
	char * json_str = NULL;
	json_object *root=NULL, *status= NULL;
	root = json_object_new_object();
	

	json_object_object_add(root, "apiKey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(1));

	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));

	json_str = strdup(json_object_to_json_string(root));
	//info("json_str:%s",json_str);
	json_object_put(root);

	return json_str;
}
/* 消息发送请求数据 */
char *CreateIntercomMessageReportJson(char *key, char *deviceId, char *from_user, char *media_id)
{
	char * json_str = NULL;
	json_object *root=NULL, *status= NULL;
	json_object *message = NULL, *mediaId=NULL;
	
	root = json_object_new_object();
	
	json_object_object_add(root, "apiKey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(0));

	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));

#if 0
	if(from_user) {
		info("from_user!= NULL");
		info("from_user:%s",from_user);
		json_object *toUsers = NULL;
		
		toUsers = json_object_new_array();
		json_object_array_add(toUsers, json_object_new_string(from_user));
		json_object_object_add(root, "toUsers", toUsers);
		
		info("array:%s",json_object_to_json_string(toUsers));
	}
#endif	
	
	if(media_id) {
		message = json_object_new_object();
		json_object_object_add(message, "mediaId", json_object_new_string(media_id));
		json_object_object_add(root, "message", message);
	}

	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);
//json_str:{ "apiKey": "8e3941df07b14bc183766ebd51a3b8b0", "type": 0, "deviceId": "aiAA8005dfc07621", "message": { "mediaId": "UdeOVIXorbOSOxeAEx8uApxonCNCcnxBIAM4Fnn6QcLLSJ_ifBAjZCIbXSPqkMN6" } }
	json_object_put(root);

	return json_str;
}
/* 音频状态请求数据 */
char *CreateAudioStatusReportJson(char *key, char *deviceId, AudioStatus *audioStatus)
{
	char * json_str = NULL;
	json_object *root=NULL;
	json_object *status = NULL;
	info("CreateAudioStatusReportJson");	
	root = json_object_new_object();
	status = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(STATUS_AUDIO_TYPE));

	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));
	
	if(audioStatus->title)	
		json_object_object_add(status, "title", json_object_new_string(audioStatus->title));
	else 
		json_object_object_add(status, "title",  json_object_new_string(""));

#if 0
	if(audioStatus->mediaId)
		json_object_object_add(status, "mediaId", json_object_new_string(audioStatus->mediaId));
	else 
		json_object_object_add(status, "mediaId", json_object_new_string(""));
#else
	if(audioStatus->mediaId) {
		int mediaid = atoi(audioStatus->mediaId);
		json_object_object_add(status, "mediaId", json_object_new_int(mediaid));
	}

#endif
	//if(audioStatus->url)
	//	json_object_object_add(status, "url", json_object_new_string(audioStatus->url));
	//else 
	//	json_object_object_add(status, "url", json_object_new_string(""));
	
	//json_object_object_add(status, "duration",  json_object_new_int(audioStatus->duration));
	json_object_object_add(status, "play", json_object_new_int(audioStatus->play));
	//json_object_object_add(status, "progress", json_object_new_int(audioStatus->progress));
	
	json_object_object_add(root, "status", status);
	info("status:%s",json_object_to_json_string(status));
	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}
/* 同步本地音乐请求数据 */
char *CreateSynchronizeLocalMusicReportJson(char *key, char *deviceId, int operate,char *names)
{
	char * json_str = NULL;
	json_object *root=NULL;
	json_object *status = NULL;
	info("CreateSynchronizeLocalMusicReportJson");	
	root = json_object_new_object();
	status = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(STATUS_MUSIC_TYPE));

	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));

	
	json_object_object_add(status, "operate",  json_object_new_int(operate));
	if(names == NULL) 
		json_object_object_add(status, "names", json_object_new_string(""));
	else
		json_object_object_add(status, "names", json_object_new_string(names));
	
	json_object_object_add(root, "status", status);
	info("status:%s",json_object_to_json_string(status));
	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}
/* 同步本地故事请求数据 */
char *CreateSynchronizeLocalStoryReportJson(char *key, char *deviceId, int operate,char *names)
{
	char * json_str = NULL;
	json_object *root=NULL;
	json_object *status = NULL;
	info("CreateSynchronizeLocalStoryReportJson");	
	root = json_object_new_object();
	status = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(STATUS_STORY_TYPE));

	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));

	
	json_object_object_add(status, "operate",  json_object_new_int(operate));
	if(names == NULL) 
		json_object_object_add(status, "names", json_object_new_string(""));
	else
		json_object_object_add(status, "names", json_object_new_string(names));
	
	json_object_object_add(root, "status", status);
	info("status:%s",json_object_to_json_string(status));
	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}

/* 数据报告请求数据 */
char *CreateIotDataReportJson(char *key, char *deviceId)
{
	char * json_str = NULL;
	json_object *root=NULL;
	json_object *status = NULL;
	info("CreateIotDataReportJson");	
	root = json_object_new_object();

	json_object_object_add(root, "apiKey", json_object_new_string(key));

	json_object_object_add(root, "type", json_object_new_int(0));

	json_object_object_add(root, "deviceId", json_object_new_string(deviceId));

	json_str = strdup(json_object_to_json_string(root));
	info("json_str:%s",json_str);

	json_object_put(root);

	return json_str;
}


