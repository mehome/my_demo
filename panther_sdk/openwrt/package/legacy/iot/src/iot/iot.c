//
//  testIot.c
//  ai-wifi
//
//  Created by sunlei on 2017/4/8.
//  Copyright © 2017年 sunlei. All rights reserved.
//
#include "iot.h"

#include <stdio.h>
#include <curl/curl.h>
#include <json/json.h>

#include "http.h"
#include "device.h"
#include "DeviceStatus.h"

#include "queue.h"
#include "PlaylistJson.h"



static pthread_t iotPthread = 0;

static char *g_mediaId = NULL;
static char *g_topic = NULL;  
static char      *g_deviceId = NULL;
static char      *g_key = NULL;

/* 低电量状态报告接口 */
void IotLowPowerReport()
{
	char *body = NULL;
	IotEvent *event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	body = CreateEmergStatusReportJson(key, deviceId);
	info("LowPowerReport:%s",body);
	
	event = newIotEvent(body);
	
	if(event) {
		EventQueuePut(EVENT_REPROT_IOT_STATUS, event);
	}	
}

/* 设备状态报告接口 */
void    IotDeviceStatusReport()
{
	char *body = NULL;
	IotEvent *event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	
	body = CreateDeviceStatusReportJson(key,deviceId);
	info("DeviceStatus:%s",body);
	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_REPROT_IOT_STATUS, event);
	}	
}
/* 设备授权接口 */
void IotDeviceVendorAuthorize()
{
	char *body = NULL;
	IotEvent *event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	char mac[13] = {0};
    char *productId = NULL;
    
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
    productId= GetTuringProductId();

	memcpy(mac , deviceId+4, 12);
	
	mac[12]= 0;
	info("deviceId+3:%s",deviceId+4);
	info("mac:%s",mac);
    
#if defined(ENABLE_YIYA)
	body = CreateDeviceVendorAuthorizeJson(key,"43416", deviceId, mac);
#elif defined(ENABLE_LVMENG)
    body = CreateDeviceVendorAuthorizeJson(key,productId, deviceId, mac);
#endif
	info("VendorAuthorizeJson:%s",body);
	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_VENDOR_AUTHORIZE, event);
		info("QueuePut EVENT_VENDOR_AUTHORIZE");
	}	
}
/* 设备授权状态查询接口 */
void IotDeviceVendorStatusQuery()
{
	char *body = NULL;
	IotEvent *event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	
	body = CreateDeviceVendorStatusQueryJson(key, deviceId);
	info("VendorStatusQueryJson:%s",body);
	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_QUERY_DEVICE_STATUS, event);
		info("QueuePut EVENT_QUERY_DEVICE_STATUS");
	}	
}

/* 音频状态报告接口 */
void IotAudioStatusReport()
{
	char *body = NULL;
	AudioStatus *audioStatus = NULL;
	IotEvent *event = NULL;
	MusicItem *music = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	int type = 0;
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	audioStatus = newAudioStatus();
	MpdGetAudioStatus(audioStatus);
	if(audioStatus->url){
		error("before GetMusicItemFromJsonFile");
		music = GetMusicItemFromJsonFile(TURING_PLAYLIST_JSON, audioStatus->url);
		error("after GetMusicItemFromJsonFile");
		if(music) {
			if(music->pId)	//pid号
				audioStatus->mediaId = strdup(music->pId);
			if(music->pTitle)	//歌曲名字
				audioStatus->title =  strdup(music->pTitle);
			type = music->type;
			FreeMusicItem(music);
		}
	}
	info("type:%d", type);
	if(type <= 1) {
		if(audioStatus->mediaId) {
			info("audioStatus->mediaId:%s",audioStatus->mediaId);
			body = CreateAudioStatusReportJson(key,deviceId, audioStatus);
			info("AudioStatus:%s",body);
			event = newIotEvent(body);
			if(event) {
				EventQueuePut(EVENT_REPROT_IOT_STATUS, event);
			}
		}
	}

	freeAudioStatus(&audioStatus);
	info("exit");
}
/* 获取数据接口 */
void IotGetDataReport()
{
	char *body = NULL;
	IotEvent * event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	
	body = CreateGetDataReportJson(key,deviceId, 2);
	info("DeviceStatus:%s",body);
	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_GET_DATA, (void *)event);
		info("QueuePut EVENT_GET_DATA");
	}

}
/* 获取音频资源接口 */
void IotGetAudio(int type)
{
	char *body = NULL;
	IotEvent * event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	
	body = CreateAudioGetJson(key,deviceId, type);
	info("audio:%s",body);
	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_GET_AUDIO, (void *)event);
		info("QueuePut EVENT_GET_DATA");
	}
}
/* 设备收藏接口 */
void IotCollectSong(int id)
{
	char *body = NULL;
	IotEvent * event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	
	body = CreateCollecSongJson(key,deviceId, id);
	info("DeviceStatus:%s",body);
	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_COLLECT_SONG, (void *)event);
		info("QueuePut EVENT_GET_DATA");
	}
}

/* 获取topic和clientid接口 */
void    IotTopicReport()
{
	char *body = NULL;
	IotEvent * event = NULL;
	
	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	body = CreateTopicJson(key, deviceId);
	info("TopicReport:%s",body);


	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_REQUEST_TOPIC, (void *)event);
		info("QueuePut EVENT_REQUEST_TOPIC");
	}
	
}
/* 获取mqtt用户名和密码接口 */
void IotGetMqttInfo()
{
	EventQueuePut(EVENT_GET_MQTT_INFO, NULL);
	info("QueuePut EVENT_GET_MQTT_INFO");
}

/* 同步本地音乐接口 */
void IotSynchronizeLocalMusicReport(int operate,char *names)
{	
	char *body = NULL;
	IotEvent * event = NULL;

	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	body = CreateSynchronizeLocalMusicReportJson(key, deviceId, operate, names);
	info("SynchronizeLocalMusic:%s",body);

	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_REPROT_IOT_STATUS, event);
		info("QueuePut EVENT_REQUEST_TOPIC");
	}
}
/* 同步本地故事接口 */
void IotSynchronizeLocalStoryReport(int operate,char *names)
{	
	char *body = NULL;
	IotEvent * event = NULL;
	char *deviceId = NULL;
	char *key = NULL;
	
	deviceId = GetTuringDeviceId();
	key 	 = GetTuringApiKey();
	
	body = CreateSynchronizeLocalStoryReportJson(key, deviceId, operate, names);
	info("SynchronizeLocalStory:%s",body);

	event = newIotEvent(body);
	if(event) {
		EventQueuePut(EVENT_REPROT_IOT_STATUS, event);
		info("QueuePut EVENT_REQUEST_TOPIC");
	}
}
/* 处理IotGetMqttInfo()返回的数据 */
int ProcessHttpGetJson(char *json)
{
	json_object *root = NULL, *code = NULL, *payload = NULL;
	char *mediaId = NULL;
	char * clientId=NULL, *pTopic= NULL;

	warning("json:%s",json);
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		return -1;
	}

	code = json_object_object_get(root, "code");
	if(NULL != code) {
		int iCode ;
		iCode = json_object_get_int(code);
		debug("iCode:%d",iCode);
		if(iCode == 0) 
		{
			;//info()
		}
		else if (iCode == -1) 
		{
			struct json_object *desc =NULL;
				
			desc = 	json_object_object_get(root, "desc");
			if(NULL !=  desc) 
			{
				error("desc:%s",json_object_get_string(desc));
			}
			json_object_put(root);
			return -1;
		}
	}

	payload = json_object_object_get(root, "payload");
	if( NULL != payload) {
		char *tmp;
		json_object *mqttUser = NULL;
		json_object *mqttPassword=NULL, *topic= NULL;
		mqttUser = json_object_object_get(payload, "mqttUser");
		if(mqttUser) {
			tmp = json_object_get_string(mqttUser);
			SetTuringMqttMqttUser(tmp);
			ConfigParserSetValue("mqttUser",tmp);
			info("mqttUser:%s",tmp);
		}
		mqttPassword = json_object_object_get(payload, "mqttPassword");
		if(mqttPassword) {
			tmp = json_object_get_string(mqttPassword);
			SetTuringMqttMqttPassword(tmp);
			ConfigParserSetValue("mqttPassword",tmp);
			info("mqttPassword:%s",tmp);
		}
	}
	json_object_put(root);
	return 0;
}

/* 处理mqtt相关post的返回的数据 */
int ProcessIotJson(char *json, int type)
{
	json_object *root = NULL, *code = NULL, *payload = NULL;
	char *mediaId = NULL;
	char * clientId=NULL, *pTopic= NULL;
	int ret = -1;
	info("json:%s",json);
	info("type:%d",type);
	
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		return -1;
	}

	if( NULL != root) 
	{
		info("NULL != root");
#ifdef ENABLE_HUABEI
	ret = ExecHuabeiPostCallback(type, root);
#else
	ret = ExecPostCallback(type, root);
#endif
	}
	
	json_object_put(root);
	return ret;
}

/* 检查设备是否授权 $device_status==1:授权           */  
int CheckDeviceStatus()
{
	int deviceStatus = cdb_get_int("$device_status", 0);
	info("deviceStatus:%d",deviceStatus);
	if(deviceStatus) 
		return deviceStatus;
	IotDeviceVendorAuthorize();
}

/* 获取topic,启动mqtt线程		   */  
void IotInit(void)
{
	char *body = NULL;
	int ret = 9;
	
	g_deviceId  = GetTuringDeviceId();
	g_key 		= GetTuringApiKey();

	fatal("g_deviceId:%s",g_deviceId);
	fatal("g_key:%s",g_key);
#if defined (ENABLE_YIYA) || defined (ENABLE_LVMENG)
//#if defined (ENABLE_YIYA)
	IotDeviceVendorStatusQuery();
	CheckDeviceStatus();
	IotGetMqttInfo();
#endif
	IotTopicReport();
	IotDeviceStatusReport();
	IotGetDataReport();

}







