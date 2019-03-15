#include <stdio.h>
#include <json/json.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "globalParam.h"
#include "alert/AlertHead.h"
#include <sys/stat.h>


extern GLOBALPRARM_STRU g;
extern AlertManager *alertManager;

static unsigned long get_JsonFile_size(const char *path)
{
	int iRet = -1;
	struct stat buf;  

	if (0 == stat(path, &buf))
	{
		return buf.st_size;
	}

	return iRet;
}

// AudioPlayer
json_object * Create_AudioPlayerJsonStr(char *pToken, int iOffsetMS, char *pPlaystatus)
{
	json_object *root, *header, *payload;
	root = json_object_new_object();
	
	json_object_object_add(root, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("AudioPlayer"));
	json_object_object_add(header, "name", json_object_new_string("PlaybackState"));
	
	json_object_object_add(root, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "token", json_object_new_string(pToken));
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(iOffsetMS));	
	json_object_object_add(payload, "playerActivity", json_object_new_string(pPlaystatus));
	return root;
}

// SpeechSynthesizer
json_object * Create_SpeechSynthesizerJsonStr(char *pToken, int iOffsetMS, char *pPlaystatus)
{
	json_object *root, *header, *payload;
	root = json_object_new_object();

	json_object_object_add(root, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("SpeechSynthesizer"));
	json_object_object_add(header, "name", json_object_new_string("SpeechState"));
	
	json_object_object_add(root, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "token", json_object_new_string(pToken));
	json_object_object_add(payload, "offsetInMilliseconds", json_object_new_int(iOffsetMS));
	json_object_object_add(payload, "playerActivity", json_object_new_string(pPlaystatus));

	return root;
}

// Alerts
json_object * Create_AlertsJsonStr(void)
{
	json_object *root, *header, *payload, *allAlerts, *activeAlerts;
	root = json_object_new_object();

	AlertsStatePayload * statepayload = alertmanager_get_state(alertManager);
	allAlerts = alertsstatepayload_to_allalerts_update(statepayload);
	activeAlerts = alertsstatepayload_to_activealerts(statepayload);

	json_object_object_add(root, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("Alerts"));
	json_object_object_add(header, "name", json_object_new_string("AlertsState"));

	// 默认为空
	json_object_object_add(root, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "allAlerts", allAlerts);	
	json_object_object_add(payload, "activeAlerts", activeAlerts);
	
	return root;
}

// Speaker VolumeState
json_object * Create_SpeakerVolumeStateJsonStr(int volume, char muted)
{
	json_object *root, *header, *payload;
	root = json_object_new_object();

	json_object_object_add(root, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("Speaker"));
	json_object_object_add(header, "name", json_object_new_string("VolumeState"));
	
	json_object_object_add(root, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "volume", json_object_new_int(volume));
	json_object_object_add(payload, "muted", json_object_new_boolean(muted));

	return root;
}

// Settings Updated
json_object * Create_SettingsUpdatedJsonStr(char *pKey, char *pValue)
{
	json_object *root;
	root = json_object_new_object();

	json_object_object_add(root, "key", json_object_new_string(pKey));
	json_object_object_add(root, "value", json_object_new_string(pValue));

	return root;
}

// Notifications.IndicatorState 
json_object * Create_NotificationsIndicatorStateJsonStr(char isEnabled, char isVisualIndicatorPersisted)
{
	json_object *root, *header, *payload;
	root = json_object_new_object();

	json_object_object_add(root, "header", header = json_object_new_object());
	json_object_object_add(header, "namespace", json_object_new_string("Notifications"));
	json_object_object_add(header, "name", json_object_new_string("IndicatorState"));
	
	json_object_object_add(root, "payload", payload = json_object_new_object());
	json_object_object_add(payload, "isEnabled", json_object_new_boolean(isEnabled));
	json_object_object_add(payload, "isVisualIndicatorPersisted", json_object_new_boolean(isVisualIndicatorPersisted));

	return root;
}

#define XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#if 0
char * ReadStrFromFile(char *pFilePath)
{
	char * json_str = NULL;

	if (0 == access(pFilePath, 0))
	{
		printf("Read %s jsonStr\n", pFilePath);
		unsigned long wFilesize = get_JsonFile_size(pFilePath);
		if (wFilesize != 0)
		{
			int from_fd = open(pFilePath, O_RDONLY);
			json_str = malloc(wFilesize);
			int iReadSize = read(from_fd, json_str, wFilesize);
			close(from_fd);
			if (iReadSize < 0)
			{
				free(json_str);
				return NULL;
			}
			else
			{
				return json_str;
			}
		}
	}

	return json_str;
}
#endif
#if 0
void CreateCommonJsonStr(void)
{
	char * json_str = NULL;

	//json_str = ReadStrFromFile(ALEXA_JASON_PATH);

	//if (NULL == json_str)
	{
		// 获取数据失败，说明当前播放的不是亚马逊的音乐，那么只更新声音
		if (0 != QueryMpdStatus())
		{
			g.m_mpd_status.volume = GetMpcVolume();
		}

		json_str = UpDateAlexaCommonJsonStr();
	}
	//else
	//{
		//free(json_str);
	//}

	return json_str;
}
#endif

char * UpDateAlexaCommonJsonStr(void)
{
	//printf("Update json String\n");

	char *json_str = NULL;

	//pthread_mutex_lock(&g.RWJsonStr_lock);

	json_object *root, *root1, *payload, *context, *header, *event, *allAlerts, *activeAlerts;

	root = json_object_new_object();

	json_object_object_add(root, "context", context = json_object_new_array());

	if (g.CurrentPlayDirective.m_PlayToken != NULL)
	{
		//MpdInit();
		int elapsed_ms = MpdGetCurrentElapsed_ms();
		//MpdDeinit();
		json_object_array_add(context, root1 = Create_AudioPlayerJsonStr(g.CurrentPlayDirective.m_PlayToken, elapsed_ms, g.m_mpd_status.mpd_state));
	}
	else
		json_object_array_add(context, root1 = Create_AudioPlayerJsonStr("", 0, "IDLE"));

	if (g.m_SpeechToken != NULL)
		json_object_array_add(context, root1 = Create_SpeechSynthesizerJsonStr(g.m_SpeechToken, 0, "FINISHED"));
	else
		json_object_array_add(context, root1 = Create_SpeechSynthesizerJsonStr("", 0, "FINISHED"));

	json_object_array_add(context, root1 = Create_AlertsJsonStr()); 
	json_object_array_add(context, root1 = Create_SpeakerVolumeStateJsonStr(g.m_mpd_status.volume, g.m_AmazonMuteStatus));
	json_object_array_add(context, root1 = Create_NotificationsIndicatorStateJsonStr(g.m_Notifications.m_IsEnabel, g.m_Notifications.m_isVisualIndicatorPersisted));

	// event 默认为空，需要使用时再填写
	json_object_object_add(root, "event", event = json_object_new_object());

	json_str = strdup(json_object_to_json_string(root));
	json_object_to_file(ALEXA_JASON_PATH, root);

	json_object_put(event);
	json_object_put(root1);
	json_object_put(root);

	//pthread_mutex_unlock(&g.RWJsonStr_lock);

	return json_str;
}

// 这里只是修改context array里面的数据，enevt数据由每个流自由管理
#if 0
char * Json_Data_Modify(char *pChangeStr, struct json_object *object)
{
	int i;
	json_object *root, *context, *temp_obj;

	char *Process_jsonstr = ReadStrFromFile(ALEXA_JASON_PATH);
	
	root = json_tokener_parse(Process_jsonstr);
	if (is_error(root))
	{
		printf("json_tokener_parse failed\n");
		return NULL;
	}
	context = json_object_object_get(root, "context");

	int iLength = json_object_array_length(context); 
	for (i = 0; i < iLength; i++)
	{
		temp_obj = json_object_array_get_idx(context, i);
		if (NULL != strstr(json_object_to_json_string(temp_obj), pChangeStr))
		{
			json_object_array_put_idx(context, i, object);
			break;
		}
	}

	char *json_str = strdup(json_object_to_json_string(root));
	json_object_to_file(ALEXA_JASON_PATH, root);

	json_object_put(temp_obj);
	json_object_put(context);
	json_object_put(root);

	if (Process_jsonstr != NULL)
		free(Process_jsonstr);
	
	return json_str;
}
#endif
/*
int main(void)
{
	Alexa_JsonData = CreateJsonStr();
	printf("Alexa_JsonData:%s\n", Alexa_JsonData);
	
	printf("--------------------------------\n");
	Json_Data_Modify("Speaker", Create_SpeakerVolumeStateJsonStr(100, 0));
}*/

