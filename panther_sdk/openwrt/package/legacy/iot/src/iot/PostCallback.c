
#include "PostCallback.h"
#include <json/json.h>

#include "DeviceStatus.h"
#include "myutils/debug.h"
#include "callback.h"
#include "PlaylistStruct.h"
#include "common.h"
#include "mpdcli.h"


static int ReportIotStatusCallback(void *arg) ;
static int ReportAudioStatusCallback(void *arg) ;
static int ReportDeviceStatusCallback(void *arg) ;
static int RequestTopicCallback(void *arg);
static int VendorAuthorizeCallback(void *arg) ;
static int QueryDeviceStatusCallback(void *arg) ;
static int UploadFileCallback(void *arg) ;
static int GetAudioCallback(void *arg) ;
static int CollectSongCallback(void *arg);

/* 响应数据的处理函数数组 */
static Callback postCallbackTable[] = {
	{EVENT_REPROT_IOT_STATUS, NULL},
	{EVENT_REPROT_AUDIO_STATUS, ReportAudioStatusCallback},
	{EVENT_REPROT_DEVICE_STATUS, NULL},
	{EVENT_REQUEST_TOPIC, RequestTopicCallback},
	{EVENT_VENDOR_AUTHORIZE, VendorAuthorizeCallback},
	{EVENT_QUERY_DEVICE_STATUS, QueryDeviceStatusCallback},
	{EVENT_UPLOAD_FILE, UploadFileCallback},
	{EVENT_GET_AUDIO, GetAudioCallback},
	{EVENT_COLLECT_SONG, CollectSongCallback},
	{EVENT_SEND_MESSAGE, NULL},
	
};
/* 设备收藏歌曲响应数据的处理函数 */
static int CollectSongCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	char *mediaId = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);

	return ret;
}
/* 获取音频资源响应数据处理函数 */
static int GetAudioCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	char *mediaId = NULL;
	int ret = -1;
	cdb_set_int("$turing_mpd_play", 0);
	cdb_set_int("$key_request_song",4);		//防止stop事件里面，提前清掉这个标志状态
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	if(ret != 0)
		return ret;
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
		json_object *name = NULL, *id = NULL, *tip =NULL, *url = NULL;
		char *nameStr = NULL;
		char *tipStr = NULL;
		char *urlStr = NULL;
		int intId = 0;		
		url = json_object_object_get(payload, "url");
		if(url == NULL)
			return -1;

		urlStr = json_object_get_string(url);
		info("urlStr:%s",urlStr);
		
		
		name = json_object_object_get(payload, "name");
		if(name) {
			nameStr = json_object_get_string(name);
		}
		
		id = json_object_object_get(payload, "id");
		if(id) {
			intId = json_object_get_int(id);
		}

		tip = json_object_object_get(payload, "tip");
		if(tip) {
			tipStr = json_object_get_string(tip);
		}
		
		MusicItem *item = NewMusicItem();
		if(item) 
		{
			char mediaId[128] = {0};
			snprintf(mediaId, 128, "%d", intId);
			info("mediaId:%s",mediaId);
			if(urlStr && strlen(urlStr) > 0)
				item->pContentURL = strdup(urlStr);

			if(nameStr && strlen(nameStr) > 0)
				item->pTitle = strdup(nameStr);
			info("item->pTitle:%s",item->pTitle);
			if(tipStr && strlen(tipStr) > 0)
				item->pTip = strdup(tipStr);
			if(id && strlen(id) > 0)
				item->pId = strdup(mediaId);
			item->type = 0;
#if 1
			Turing_list_Insert(TURING_PLAYLIST,item,1);
#else
			int play = MpdGetPlayState();
			if(play == MPDS_PLAY) {
				TuringPlaylistInsert(TURING_PLAYLIST, item, 0);		
			} else {
				TuringPlaylistInsert(TURING_PLAYLIST, item, 1);		
			}
#endif
			MpdClear();
			MpdVolume(200);
			MpdAdd(urlStr);
			MpdPlay(0);
			MpdRepeat(0);
			MpdSingle(0);
//			usleep(1000*1000);
			FreeMusicItem(item);
		}
		//char cmd[2048] ={0};
		//snprintf(cmd, 2048, "creatPlayList iot \"%s\" \"%s\" null \"%s\" \"%s\" null null  0 symlink", urlStr,nameStr,tipStr, idStr);
		//exec_cmd(cmd);
		if(cdb_get_int("$turing_mpd_play", 0) != 4){
			cdb_set_int("$turing_mpd_play", 4);
		}	
		info("GetAudioCallback done 1111111111111111111111");
	}
	return ret;
}


/* 文件上传接口响应数据处理函数 */
static int UploadFileCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	char *mediaId = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	//root:{ "desc": "success", "code": 0, "payload": "UdeOVIXorbOSOxeAEx8uApxonCNCcnxBIAM4Fnn6QcLLSJ_ifBAjZCIbXSPqkMN6" }
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
		mediaId = json_object_get_string(payload);
		if(mediaId) {
			SetTuringMqttMediaId(mediaId);
			EventQueuePut(EVENT_SEND_MESSAGE, NULL);
		}
	}
	StartPowerOn_led();
	return ret;
}

/* 上报状态响应数据处理函数 */
static int ReportIotStatusCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL;
	json_object *val = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
	}
	return ret;
}
/* 上报音频状态响应数据处理函数 */
static int ReportAudioStatusCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
		//char *mediaId = json_object_get_string(payload);
		//debug("MediaId:%s", mediaId);
		//SetTuringMqttMediaId(mediaId);
	}
	
	return ret;
}
/* 上报设备状态响应数据处理函数 */
static int ReportDeviceStatusCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
	}
	return ret;
}
/* 获取topic响应数据处理函数 */
static int RequestTopicCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	json_object *clientId = NULL, *topic = NULL;
	char *pClientId = NULL, *pTopic = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
		clientId = json_object_object_get(payload, "clientId");
		if(NULL !=  clientId) 
		{
			pClientId = json_object_get_string(clientId);
			info("clientId:%s",pClientId );
			SetTuringMqttClientId(pClientId);
		}
		topic = json_object_object_get(payload, "topic");
		if(NULL !=  topic) 
		{
			
			pTopic = json_object_get_string(topic);
			info("topic:%s",pTopic );
			SetTuringMqttTopic(pTopic);

		}
		if(pTopic && pClientId) {
			StartMqttThread();
		}
	}


	return ret;
}
/* 设备授权响应数据处理函数 */
static int VendorAuthorizeCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *payload = NULL, *val = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
	}

	return ret;
}
/* 查询设备状态响应数据处理函数 */
static int QueryDeviceStatusCallback(void *arg) 
{
	json_object *root = (json_object *)arg;
	json_object *status = NULL;
	json_object *payload = NULL, *val = NULL;
	int ret = -1;
	info("root:%s",json_object_to_json_string(root));
	val = json_object_object_get(root, "code");
	if(val) 
		ret =  json_object_get_int(val);
	payload = json_object_object_get(root, "payload");
	if(payload) {
		info("payload:%s",json_object_to_json_string(payload));
		status = json_object_object_get(payload, "status");
		if(NULL != status) 
		{		
			int deviceStatus = json_object_get_int(status);
			info("deviceStatus:%d",deviceStatus );
			cdb_set_int("$device_status", deviceStatus);
		}
	}

	return ret;
}


/* post请求数据处理函数 */
int ExecPostCallback(int type, void *arg)
{
	int ret = -1;
	int i = 0;
		
	int tbl_len = sizeof(postCallbackTable) / sizeof(postCallbackTable[0]);
	debug("type:%d" ,type);
	debug("tbl_len:%d" ,tbl_len);
	for(i = 0; i < tbl_len; i++) 
	{
		if( postCallbackTable[i].type == type)
		{
			
			if(postCallbackTable[i].callback != NULL) {
				ret = postCallbackTable[i].callback(arg);	
			}
			break;
			
		}
	}

	return ret;
}



