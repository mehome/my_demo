
#include  "column.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <fcntl.h>

#include "PlaylistJson.h"
#ifdef USE_MPDCLI
#include  "mpdcli.h"
#endif

static AudioStatus **columnAudio = NULL;

static ColumeType columnType = COLUMN_TYPE_NURSERY_RHYMES;

static pthread_mutex_t columnMtx = PTHREAD_MUTEX_INITIALIZER;


static char *gColumnDir[] = {
	COLUMN_NURSERY_RHYMES_DIR,
	COLUMN_SINOLOGY_CLASSIC_DIR,
	COLUMN_BEDTIME_STORIES_DIR,
	COLUMN_FAIRY_TALES_DIR,
	COLUMN_ENGLISH_ENLIGHTTENMENT_DIR,
	COLUMN_COLLECTION_DIR
};
static char *gColumnPlaylist[] = {
	COLUMN_NURSERY_RHYMES_PLAYLIST,
	COLUMN_SINOLOGY_CLASSIC_PLAYLIST,
	COLUMN_BEDTIME_STORIES_PLAYLIST,
	COLUMN_FAIRY_TALES_PLAYLIST,
	COLUMN_ENGLISH_ENLIGHTTENMENT_PLAYLIST,
	COLUMN_COLLECTION_PLAYLIST
};
static char *gColumnSpeech[] = {
	COLUMN_NURSERY_RHYMES_SPEECH,
	COLUMN_SINOLOGY_CLASSIC_SPEECH,
	COLUMN_BEDTIME_STORIES_SPEECH,
	COLUMN_FAIRY_TALES_SPEECH,
	COLUMN_ENGLISH_ENLIGHTTENMENT_SPEECH,
	COLUMN_COLLECTION_SPEECH
};
		
static int gColumnLength = sizeof(gColumnDir)/sizeof(gColumnDir[0]); 

void SetColumnType(int type) 
{
	pthread_mutex_lock(&columnMtx);
	columnType = (type%COLUMN_TYPE_COLLECTION);
	pthread_mutex_unlock(&columnMtx);
}

int GetColumnType() 
{
	int ret;
	pthread_mutex_lock(&columnMtx);
	ret = columnType;
	pthread_mutex_unlock(&columnMtx);
	return ret;
}
/* 在/usr/script/playlists/下生成m3u文件和json文件. */
int CreateColumnPlaylistFile(char *columnDir, char *file)
{
	int ret;
	char columnRootDir[256] = {0};
	char updateDir[256] = {0};
	char value[16] = {0};
	char cmd[1024] = {0};
	char playlistFileDir[128] = {0};
	char *playlistFile=NULL;
	
	cdb_get_str("$tf_mount_point",value,16,"");

	snprintf(columnRootDir ,24, "/media/%s/%s", value, columnDir);
	snprintf(updateDir ,24, "%s/%s", value, columnDir);
	error("updateDir:%s",updateDir);
	ret = create_path(columnRootDir);
	warning("ret:%d",ret);
	if(ret < 0)
		return ret;
	//snprintf(playlistFile, 64, "%splaylist",file);
	snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u", file);
	error("playlistFileDir:%s", playlistFileDir);
	unlink(playlistFileDir);
	eval("mpc", "update", updateDir);
	eval("mpc", "listall");
	bzero(cmd, 1024);
	snprintf(cmd, 1024, "mpc listall | grep \"%s\" > /tmp/tmp.txt && cp /tmp/tmp.txt %s",columnDir, playlistFileDir);
	exec_cmd(cmd);
	bzero(cmd, 1024);
	snprintf(cmd, 1024,"creatPlayList column \"%s\"", file);
	exec_cmd(cmd);
	return 0;
}
/* 删除/usr/script/playlists/下的m3u和json文件. */
void RemovePlayListFile()
{
	char playlistFileDir[128] = {0};
	char playlistJsonDir[128] = {0};
	int i = 0;
	for(i = 0; i< gColumnLength; i++) 
	{
		snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u",gColumnPlaylist[i]);
		snprintf(playlistJsonDir, 128, "/usr/script/playlists/%s.json",gColumnPlaylist[i]);
		unlink(playlistFileDir);
		unlink(playlistJsonDir);
	}
}
/* 
 * 播放type对应的栏目中的音乐
 * 1. /usr/script/playlists/有对应栏目的m3u文件，则直接播放
 * 2. /usr/script/playlists/没有对应栏目的m3u文件,则先生成m3u文件,再播放
 */
int PlayColumnDirMusic(int type)
{
	char *speech = "地球档案文件夹还没有内容";
	int  ret, length; 
	AudioStatus *status =NULL;
	char *columnDir = NULL;
	char columnRootDir[128] = {0};
	char updateDir[128] = {0};
	char value[16] = {0};
	char cmd[1024] = {0};
	
	char playlistFileDir[128] = {0};
	char playlistJsonDir[128] = {0};
	char linkFile[128] = {0};
	SaveColumnAudio();						//保存之前的状态
	info("ColumnTextToSound:%s",gColumnSpeech[type]);
	//播放对应栏目的提示音
	ColumnTextToSound(gColumnSpeech[type]);
	cdb_get_str("$tf_mount_point",value,16,"");
	
	warning("type:%d", type);
	columnDir = gColumnDir[type];
	snprintf(columnRootDir ,128, "/media/%s/%s", value, columnDir);
	// /media/mmcblk0p1/收藏
	snprintf(updateDir ,128, "%s/%s", value, columnDir);
	// /mmcblk0p1/收藏
	ret = create_path(columnRootDir);
	warning("ret:%d",ret);
	if(ret < 0)
		return ret;

	snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u",gColumnPlaylist[type]);
	snprintf(playlistJsonDir, 128, "/usr/script/playlists/%s.json",gColumnPlaylist[type]);

	//当有"playlistFileDir"文件时，直接goto到后面，若没有则顺序执行
	if( access(playlistFileDir, F_OK) ==0)	//成功返回0 
	{
		error("	current playlist link to %s", playlistFileDir);
		goto link;
	}
	warning("columnDir:%s", columnDir);
	unlink(playlistFileDir);
	unlink(playlistJsonDir);

#ifdef USE_MPDCLI
	MpdUpdate(updateDir);		//mpd更新
#else
	eval("mpc", "update", updateDir);
#endif

	error("	create  %s", playlistJsonDir);
	snprintf(cmd, 1024, "mpc listall | grep \"%s\" > /tmp/tmp.txt && cp /tmp/tmp.txt %s",columnDir, playlistFileDir);
	//mpc listall | grep /media/mmcblk0p1/收藏 > /tmp/tmp.txt && cp /tmp/tmp.txt /usr/script/playlists/sinologyplaylist.m3u
	exec_cmd(cmd);
	
	if(access(playlistFileDir,F_OK) != 0) {	//失败返回-1
		ColumnTextToSound(speech);
		StartPowerOn_led();
		return -1;
	}
	bzero(cmd, 1024);
	snprintf(cmd, 1024,"creatPlayList column \"%s\"",  gColumnPlaylist[type]);
	exec_cmd(cmd);
link:

	unlink(CURRENT_PLAYLIST_JSON);
	unlink(CURRENT_PLAYLIST_M3U);
	symlink(playlistFileDir, CURRENT_PLAYLIST_M3U);
	symlink(playlistJsonDir, CURRENT_PLAYLIST_JSON);
play:	
#if 0	
#ifdef USE_MPDCLI
	MpdClear();
	MpdRepeat(1);
	MpdLoad(CURRENT_PLAYLIST);
#else
	exec_cmd("mpc clear");
	exec_cmd("mpc repeat on");
	bzero(cmd, 1024);
	snprintf(cmd, 1024,"mpc load %s", CURRENT_PLAYLIST);
	exec_cmd(cmd);
#endif
	length = GetFileLength(CURRENT_PLAYLIST_M3U) ;
	warning("length:%d",length);
	srand( (unsigned)time(NULL) );   
	int count = rand()%length + 1;
	count--;
	warning("count:%d",count);
	MpdPlay(count);
#endif
	cdb_set_int("$turing_mpd_play", 0);	//播放歌曲
	StartTlkPlay_led();
#ifdef USE_MPDCLI
	MpdRepeat(1);
	MpdSingle(0);
#else
	exec_cmd("mpc single off");
	exec_cmd("mpc repeat off");
#endif
	/*
	 * 1.当columnAudio[type]为NULL时, 播放列表中的第一首
	 * 2.当columnAudio[type]不为NULL, 播放columnAudio[type]中保存的url
	 */
	status = columnAudio[type];
	if(status) 
	{
		info("status !== NULL [%p]",status);
		//if(status->state == MPDS_PLAY || status->state == MPDS_PAUSE) {
		if(status->url != NULL) {
			info("status->url:%s",status->url);
			AudioStatusPlay(status);	//播放CURRENT_PLAYLIST中链接为status->url的歌曲
		} 
		else{
			PlaylistPlayRandom(0);	//播放歌曲，从头开始
		}
	} 
	else 
	{
		info("status == NULL");
		PlaylistPlayRandom(0);	//播放歌曲，从头开始
	}
	return 0;
}


/* 依次播放栏目文件夹里的音乐. */
int    ScanColumnDir()
{
	int ret = 0, type;
	int state = MpdGetPlayState();
	info("state:%d",state);
	MpdPause();
	type = GetColumnType();	//判断栏目类型(第一次为儿歌精选)
	info("type:%d",type);
	ret = PlayColumnDirMusic(type);
	if(ret != 0) {
		if(state == MPDS_PLAY) {
			warning("resume play...");
			MpdPlay(-1);
		}
	}
	SetColumnType(type+1);		//每一次将栏目类型+1
	return ret;
}

/* 播放sd卡内收藏夹里的内容 */
int   ScanCollectionDir()
{
	int ret = 0;
	int state = MpdGetPlayState();
	MpdPause();
	ret = PlayColumnDirMusic(COLUMN_TYPE_COLLECTION);//播放收藏目录下的内容
	if(ret != 0) {
		if(state == MPDS_PLAY) {
			warning("resume play...");
			MpdPlay(-1);
		}
	}

	return ret;

}
/* 释放columnAudio数组元素 */
int ColumnAudioFree()
{
	int i;
	for(i = 0; i< gColumnLength; i++)  
	{
		if(columnAudio[i]) {
			freeAudioStatus(&columnAudio[i]);
		}
	}
	return 0;
}
/*
 * 当栏目切换时先保存当前栏目的mpd状态
 *
 */
int SaveColumnAudio()
{
	int i ,ret;
	int index = -1;
	AudioStatus *status = NULL;
	char linkFile[128] = {0};
	char buf[64]={0};
	ret = readlink(CURRENT_PLAYLIST_M3U, linkFile ,128);
	if(ret <= 0)
		return -1;
	info("linkFile:%s",linkFile);
	//	linkFile:/usr/script/playlists/turingPlayList.m3u
	sscanf(linkFile, "/usr/script/playlists/%[^.]",buf);
	info("buf:%s",buf);
	//buf:turingPlayList
	for(i = 0; i< gColumnLength; i++)  {
		if(strncmp(buf, gColumnPlaylist[i], strlen(gColumnPlaylist[i])) == 0) {
			index = i;
			break;
		}
	}
	info("index:%d",index);
	if(index < 0)
		return index;
	
	if(columnAudio[index] == NULL) {
		columnAudio[index] = newAudioStatus();
	}
	if(columnAudio[index] == NULL)
		return -1;
	status = newAudioStatus();
	if(status) {
		MpdGetAudioStatus(status);
		info("status:%d",status->state);
		if(status->state == MPDS_PLAY || status->state == MPDS_PAUSE) 
		{
			if(status->url) {
				int idx = TuringPlaylistToIndex(status->url);
				if(idx > 0) {
					deepFreeAudioStatus(&columnAudio[index]);
					dupAudioStatus(columnAudio[index], status);
				}
			}
		}
		freeAudioStatus(&status);
	}

	return 0;
	
}
	
/* 开启栏目功能. */
int StartColumn()
{
	int ret;
	int tfStatus =0;

	char *speechNoTf = "请先插入tf卡";
	char *speech2 = "添加探索档案";
	char *speech1 = "开启探索收藏夹的内容";
	char *speechGoing = "正在收藏中";
	
	tfStatus = cdb_get_int("$tf_status", 0);
	warning("tf_status=%d", tfStatus);

	if(tfStatus == 1) 
	{	
		ret = ScanColumnDir();
	}
	else 
	{
		ret = text_to_sound(speechNoTf);
		StartPowerOn_led();
	}	
exit:
	return ret;	
	
}



/* 分配AudioStatus[],用于保存每个栏目下的mpd状态. */
int   ColumnAudioInit()
{
	int len;
	columnAudio = (AudioStatus*)calloc(gColumnLength * sizeof(AudioStatus), 1);
	if(columnAudio == NULL) {
		error("calloc for columnAudio failed");
		return -1;
	}

	return 0;
}

/* 释放AudioStatus[]. */
void ColumnAudioDeinit()
{
	int len, i;
	
	len = sizeof(columnAudio) / sizeof(columnAudio[0]);
	if(columnAudio) {
		for ( i = 0; i < len; i++) {
			freeAudioStatus(&columnAudio[i]);
		}
		free(columnAudio);
	}

	
}









