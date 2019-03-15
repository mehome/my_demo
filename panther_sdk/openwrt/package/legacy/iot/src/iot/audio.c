#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <json/json.h>
#include "common.h"
#include "myutils/debug.h"

#include "mpdcli.h"
#include "DeviceStatus.h"
#include "TuringPlaylist.h"
#include "SearchMusic.h"

enum {	
	PLAY_STATUS_STOP = 0,  
	PLAY_STATUS_PLAY, 
	PLAY_STATUS_PAUSE, 
	PLAY_STATUS_RESUME, 
};

/* 停止播放音乐 */
int AudioPlayStop(AudioStatus *status)
{

#ifdef USE_MPDCLI
	 MPDCliStop();
#else
	exec_cmd("mpc stop");	
#endif
	return 0;
}
/* 暂停播放音乐 */
int AudioPlayPause(AudioStatus *status)
{	
#ifdef USE_MPDCLI
	 MpdPause();
#else
	 exec_cmd("mpc pause");	
#endif
	return 0;
}
/* 播放音乐 */
int AudioPlayPlay(AudioStatus *status)
{		
	int mode = 0;
	char *purl = NULL;
	cdb_set_int("$turing_stop_time",0);
	cdb_set_int("$key_request_song",0);
	cdb_set_int("$playmode",0);
	
	mode = cdb_get_int("$current_play_mode", 0);
	if(mode != 0) {
		cdb_set_int("$current_play_mode", 0);
		FreeMpdState();
	}
	FinshLastSession();		

	/* 获取当前的歌曲信息,
	 * 如果和要播放的是同一首,直接返回
	 */
	AudioStatus *audioStatus =  newAudioStatus();
	MpdGetAudioStatus(audioStatus);								
	info("audioStatus->url:%s",audioStatus->url);
	//暂停的情况下，执行该操作后，恢复播放
	if(audioStatus->url) {
		if(strcmp(audioStatus->url, status->url) == 0) {		//对比本地mpd播放设备的歌曲链接和微信端的歌曲链接
			info("status->progress:%d",status->progress);
			if(audioStatus->state == MPDS_PAUSE) {
#ifdef USE_MPDCLI
				MpdPlay(-1);		
#else
				exec_cmd("mpc play");
#endif		
			}
			freeAudioStatus(&audioStatus);
			return 0;
		}
		
	}
	cdb_set_int("$turing_mpd_play", 0);	
	freeAudioStatus(&audioStatus);
	PrintSysTime("Start play url...");
#if 1
	purl = strdup(status->url);
	MpdClear();
	MpdVolume(200);
	MpdAdd(purl);
	MpdPlay(0);
	MpdRepeat(0);
	MpdSingle(0);

	/* 将MusicItem添加到播放列表中,不播放 */
	MusicItem *item = NULL;
	item = NewMusicItem();
	cpAudioStatusToMusicItem(item, status);
	//将歌曲添加到 /usr/script/playlists/turingPlayList.json这个文件中
	Turing_list_Insert(TURING_PLAYLIST, item, 1);	
	FreeMusicItem(item);
	cdb_set_int("$turing_mpd_play", 4);		//向服务器请求歌曲的状态
#endif
	free(purl);
	info("exit");
	return 0;
}

/*将音乐添加到列表中，并播放*/
int AudioPlaylistmusic(AudioStatus *status)
{		
	int mode = 0;
	
	cdb_set_int("$turing_stop_time",0);
	cdb_set_int("$turing_mpd_play", 0);
	mode = cdb_get_int("$current_play_mode", 0);
	if(mode != 0) {
		cdb_set_int("$current_play_mode", 0);
		FreeMpdState();
	}
	FinshLastSession();

	/* 获取当前的歌曲信息,
	 * 如果和要播放的是同一首,直接返回
	 */
	AudioStatus *audioStatus =  newAudioStatus();
	MpdGetAudioStatus(audioStatus);			//获取设备的Mpd状态
	info("audioStatus->url:%s",audioStatus->url);
	//audioStatus->url:http://appfile.tuling123.com/media/audio/20180524/80a0bd7c1e034833a14ebc6026525978.mp3
	//暂停的情况下，执行该操作后，恢复播放
	if(audioStatus->url) {
		if(strcmp(audioStatus->url, status->url) == 0) {		//对比本地mpd播放设备的歌曲链接和微信端的歌曲链接
			info("status->progress:%d",status->progress);
			//status->progress:0
			if(audioStatus->state == MPDS_PAUSE) {
#ifdef USE_MPDCLI
				MpdPlay(-1);		//播放
#else
				exec_cmd("mpc play");
#endif		
			}
			freeAudioStatus(&audioStatus);
			return 0;
		}
		
	}
	freeAudioStatus(&audioStatus);
	PrintSysTime("Start play url...");
#if 0
	long startTime = getCurrentTime();
	char cmd[2048] ={0};
	snprintf(cmd, 2048, "creatPlayList iot \"%s\" \"%s\" null \"%s\" \"%s\" null null  0 symlink", status->url,status->title,status->tip, status->mediaId);
	exec_cmd(cmd);
	info("before TuringPlaylistPlay");
	TuringPlaylistPlay(status->url);
	info("after TuringPlaylistPlay");
	long endTime = getCurrentTime();
	fatal("TuringPlay %s 耗时：%d.%d",status->url, (endTime-startTime)/1000, (endTime-startTime)%1000);		
#else
	/* 将MusicItem添加到播放列表中并播放 */
	MusicItem *item = NULL;
	item = NewMusicItem();
	cpAudioStatusToMusicItem(item, status);
	//将歌曲添加到 /usr/script/playlists/turingPlayList.json这个文件中，并开始播放
	TuringPlaylistInsert(TURING_PLAYLIST, item, 1);	
	FreeMusicItem(item);
#endif
	info("exit");
	return 0;
}


int AudioPlayResume(AudioStatus *status)
{
#ifdef USE_MPDCLI
	MpdPause();
#else
	exec_cmd("mpc play");	
#endif
	return 0;
}

/* 处理图灵IOT云端向设备端推送的音乐消息. */
int ParseAudioData(json_object *message)
{
	json_object *url = NULL, *arg = NULL, *operate = NULL;
	json_object *title = NULL, *mediaId = NULL, *tip = NULL;
	char *pTip = NULL,*pMediaId = NULL;
	char *pUrl = NULL, *pArg = NULL, *pTitle = NULL;
	int iOperate = 0;
	AudioStatus *audioStatus = NULL;
	/* 创建AudioStatus,记得要释放. */
	audioStatus = newAudioStatus();
	if(audioStatus == NULL) {
		error("newAudioStatus failed");
		goto exit;
	}
	warning("message:%s",json_object_to_json_string(message));
//message:{ "mediaId": 106129, "operate": 1, "url": "http:\/\/appfile.tuling123.com\/media\/audio\/20180524\/80a0bd7c1e034833a14ebc6026525978.mp3", "arg": "下雨天", "tip": "http:\/\/turing-iot.oss-cn-beijing.aliyuncs.com\/tts\/tts-c49c9fd38a98426abd414bcc071019be.amr" }
	url =  json_object_object_get(message, "url");
	if(url == NULL) {
		error("json_object_object_get url");
		goto exit;
	}
	pUrl = json_object_get_string(url);
	
	operate =  json_object_object_get(message, "operate");
	if(operate == NULL) {
		error("json_object_object_get operate");
		goto exit;
	}
	iOperate = json_object_get_int(operate);
	info("iOperate:%d",iOperate);
	arg =  json_object_object_get(message, "arg");
	if(arg) {
		pArg = json_object_get_string(arg);
		info("arg:%s",pArg);
	}else
		pArg = "null";
	
	tip =  json_object_object_get(message, "tip");
	if(tip) {
		pTip = json_object_get_string(tip);
		info("tip:%s",pTip);
	}else
		pTip = "null";

	mediaId =  json_object_object_get(message, "mediaId");
	if(mediaId) {
		pMediaId = json_object_get_string(mediaId);
	} else {
		pMediaId = "null";
	}
	title =  json_object_object_get(message, "title");
	if(title) {
		pTitle = json_object_get_string(title);
	} else {
		pTitle = "null";
	}
	
	switch (iOperate) {
		case PLAY_STATUS_PLAY:					//播放或者下一曲
			/* 初始化AudioStatus. */
			audioStatus->title = strdup(pArg);
			audioStatus->url = strdup(pUrl);
			audioStatus->tip = strdup(pTip);
			audioStatus->mediaId = strdup(pMediaId);
			audioStatus->type = MUSIC_TYPE_PHONE;
			//AudioPlaylistmusic(audioStatus);	//添加到列表，然后播放列表的歌曲
			AudioPlayPlay(audioStatus);			
			StartTlkPlay_led();
			break;
		case PLAY_STATUS_PAUSE:					//暂停
			AudioPlayPause(audioStatus);	
			StartPowerOn_led();
			break;
		case PLAY_STATUS_STOP:					//停止
			AudioPlayStop(audioStatus);		
			StartPowerOn_led();
			break;
		case PLAY_STATUS_RESUME:				//恢复播放
			AudioPlayResume(audioStatus);
			StartTlkPlay_led();
			break;
	}

/* 报歌名 */
#ifdef REPORT_SONG_NAME	
	if(MpdGetAudioStatus(audioStatus) == 0 )
	{
		IotAudioStatusReport(audioStatus);
	}
#endif	
exit:
	/* 释放AudioStatus */
	freeAudioStatus(&audioStatus);
	return 0 ;
}
