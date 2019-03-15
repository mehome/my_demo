#include "TuringPlaylist.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <json-c/json.h>
#include "DeviceStatus.h"


#include "myutils/debug.h"
#include "list.h"
#include "mpdcli.h"
#include "common.h"

#define TURING_PLAYLIST_JSON  "/usr/script/playlists/turingPlayList.json"
#define TURING_PLAYLIST_M3U   "/usr/script/playlists/turingPlayList.m3u"
#define CURRENT_PLAYLIST_JSON "/usr/script/playlists/currentPlaylist.json"
#define CURRENT_PLAYLIST_M3U  "/usr/script/playlists/currentPlaylist.m3u"


/* 将music添加到file播放列表中 */
int TuringPlaylist(char *file, MusicItem *music)
{
	int ret = 0;
	int play = 0;
	char *tmp = "/usr/script/playlists/tmp.m3u";
	char playlistFileDir[128] = {0};
	char playlistJsonDir[128] = {0};
	char currPlaylistDir[128] = {0};
	MusicItem *prev  = NULL;
	AudioStatus *audioStatus =  NULL;
	snprintf(playlistJsonDir, 128, "/usr/script/playlists/%s.json",file);
	snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u",file);
	readlink(CURRENT_PLAYLIST_M3U,currPlaylistDir, 128);
	info("currPlaylistJsonDir:%s",currPlaylistDir);
	if(strcmp(currPlaylistDir, playlistFileDir)) {
		info("MpdLoad:%s",file);
		unlink(CURRENT_PLAYLIST_JSON);
		unlink(CURRENT_PLAYLIST_M3U);
		MpdClear();
		MpdLoad(file);
		symlink(playlistFileDir, CURRENT_PLAYLIST_M3U);
		symlink(playlistJsonDir, CURRENT_PLAYLIST_JSON);
	}
	audioStatus = newAudioStatus();
	if(audioStatus == NULL) {
		error("calloc for newAudioStatus failed ");
		ret = -1;
		goto exit;
	}
	MpdGetAudioStatus(audioStatus);
	if(audioStatus->state == MPDS_PLAY || audioStatus->state == MPDS_PAUSE) {
		play = 1;
	} else {
		MpdClear();
		MpdLoad(file);
	}
	if(audioStatus->url) {
		prev = GetMusicItemFromJsonFile(playlistJsonDir, audioStatus->url);
		error("prev:%s", prev->pTitle);
	}
	ret = MusicItemlistInsert(playlistJsonDir, playlistFileDir, prev, music);
	
	error("music:%s",music->pContentURL);
	//MpdInset(music->pContentURL);
	eval("mpc", "insert", music->pContentURL);
	if(play) 
	{
		error("MpdNext");
		MpdNext();
	} 
	else 
	{	
		error("MpdPlay");
		MpdPlay(-1);
	}
	
	symlink(playlistFileDir, CURRENT_PLAYLIST_M3U);
	symlink(playlistJsonDir, CURRENT_PLAYLIST_JSON);
exit:
	freeAudioStatus(&audioStatus);
	if(prev)
		FreeMusicItem(prev);
	info("exit");
	return ret;
}


/*
 * 将music添加到file播放列表中 
 * play: 0只添加不播放 1添加并且播放列表中的第一首
 */
int TuringPlaylistAdd(char *file, MusicItem *music, int play)
{
	int ret = 0;
	char *tmp = "/usr/script/playlists/tmp.m3u";
	char playlistFileDir[128] = {0};
	char playlistJsonDir[128] = {0};
	char currPlaylistDir[128] = {0};
	MusicItem *prev  = NULL;
	AudioStatus *audioStatus =  NULL;
	snprintf(playlistJsonDir, 128, "/usr/script/playlists/%s.json",file);
	snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u",file);
	/* 将MusicItem插入jsonFile中的尾部,同时将url插入到m3uFile. */
	ret = MusicItemlistAdd(playlistJsonDir, playlistFileDir, prev, music);
	
	error("music:%s",music->pContentURL);


	if(play) 
	{
		unlink(CURRENT_PLAYLIST_JSON);
		unlink(CURRENT_PLAYLIST_M3U);
		symlink(playlistFileDir, CURRENT_PLAYLIST_M3U);
		symlink(playlistJsonDir, CURRENT_PLAYLIST_JSON);
#ifdef USE_MPDCLI
		MpdClear();
		MpdVolume(200);
		MpdRepeat(1);
		MpdSingle(0);
		MpdLoad(CURRENT_PLAYLIST);
		MpdPlay(0);
#else	
		exec_cmd("mpc clear");
		exec_cmd("mpc voloume 200");
		exec_cmd("mpc repeat on");
		exec_cmd("mpc single off");
		eval("mpc", "load", CURRENT_PLAYLIST);
		exec_cmd("mpc play 1");	
#endif
	}
	else 
	{
		MpdAdd(music->pContentURL);
	}
exit:
	freeAudioStatus(&audioStatus);
	return ret;
}


/*只是将music添加到file播放列表中,不播放*/
int Turing_list_Insert(char *file, MusicItem *music, int play)
{
	int ret = 0;
	int index ;
	char playlistFileDir[128] = {0};
	char playlistJsonDir[128] = {0};
	MusicItem *prev  = NULL;
	AudioStatus *audioStatus =  NULL;
	snprintf(playlistJsonDir, 128, "/usr/script/playlists/%s.json",file);// /usr/script/playlists/turingPlayList.json
	snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u",file);
	
	/* 将MusicItem插入jsonFile的头部,同时键url插入到m3uFile. */
	ret = MusicItemlistInsert(playlistJsonDir, playlistFileDir, prev, music);
//	ret = MusicItemlistAdd(playlistJsonDir,playlistFileDir,prev,music);
	info("music:%s",music->pContentURL);
	if(play == 1) 
	{
		unlink(CURRENT_PLAYLIST_JSON);
		unlink(CURRENT_PLAYLIST_M3U);
		symlink(playlistFileDir, CURRENT_PLAYLIST_M3U);
		symlink(playlistJsonDir, CURRENT_PLAYLIST_JSON);	//将"TURING_PLAYLIST"生成软连接"CURRENT_PLAYLIST"

		index = TuringPlaylistToIndex(music->pContentURL);	//去播放列表查询"url"歌曲链接
		info("index:%d",index);
		
	}
	freeAudioStatus(&audioStatus);
	info("exit");
	return ret;
}





/*
 * 将music添加到file播放列表第一首中 
 * play: 0 只添加不播放(有个问题?为什么播放列表里面有三首相同的歌曲链接)，mpd列表播放完成即暂停 
 * play: 1 添加并且播放列表中的第一首
 * play: 2  
 */
int TuringPlaylistInsert(char *file, MusicItem *music, int play)
{
	int ret = 0;
	int index ;
	char playlistFileDir[128] = {0};
	char playlistJsonDir[128] = {0};
	MusicItem *prev  = NULL;
	AudioStatus *audioStatus =  NULL;
	snprintf(playlistJsonDir, 128, "/usr/script/playlists/%s.json",file);// /usr/script/playlists/turingPlayList.json
	snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u",file);
	
	/* 将MusicItem插入jsonFile的头部,同时键url插入到m3uFile. */
	ret = MusicItemlistInsert(playlistJsonDir, playlistFileDir, prev, music);
//	ret = MusicItemlistAdd(playlistJsonDir,playlistFileDir,prev,music);
	while(!IsMpg123Finshed()){usleep(100*1000);}		//等待mpg123语音播完
	StartTlkPlay_led();
	info("music:%s",music->pContentURL);
	if(play == 1) 
	{
		unlink(CURRENT_PLAYLIST_JSON);
		unlink(CURRENT_PLAYLIST_M3U);
		symlink(playlistFileDir, CURRENT_PLAYLIST_M3U);
		symlink(playlistJsonDir, CURRENT_PLAYLIST_JSON);	//将"TURING_PLAYLIST"生成软连接"CURRENT_PLAYLIST"
#ifdef USE_MPDCLI
		MpdClear();					//清除当前播放列表信息
		MpdVolume(200);
		MpdRepeat(1);
		MpdSingle(0);
		MpdLoad(CURRENT_PLAYLIST);	//加载CURRENT_PLAYLIST文件作为播放列表
#else	
		exec_cmd("mpc clear");
		exec_cmd("mpc voloume 200");
		exec_cmd("mpc repeat on");
		exec_cmd("mpc single off");
		eval("mpc", "load", CURRENT_PLAYLIST);
	//	exec_cmd("mpc play 1");	
#endif
		index = TuringPlaylistToIndex(music->pContentURL);	//去播放列表查询"url"歌曲链接
		info("index:%d",index);
		
#ifdef USE_MPDCLI
		MpdVolume(200);
		index--;
		MpdRunSeek(index, 0);
#else
		char buf[32]={0};
		snprintf(buf, 128,"mpc play %d", index);
		exec_cmd(buf);
#endif

	}
	else if(play == 0)
	{
		MpdInset(music->pContentURL, 0);
		
	} 
	else if (play == 2)
    {
        if(MpdTttsPlayOver())
        {
            MpdClear();
            MpdVolume(200);
            MpdRepeat(1);
            MpdSingle(0);
            MpdLoad("turingPlayList");
            MpdPlay(0);
        }
	}
exit:
	freeAudioStatus(&audioStatus);
	info("exit");
	return ret;
}







