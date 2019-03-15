#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <json-c/json.h>

#include "myutils/debug.h"
#include "common.h"

#include "PlaylistJson.h"
#include "DeviceStatus.h"

/* 获取名字为filename的json中获取content_url为url的MusicItem */
MusicItem* GetMusicItemFromJsonFile(char *filename, char *url)
{
	int ret ,length, i;
	FILE* fp = NULL;
	char *data = NULL;
	MusicItem *music =  NULL;
	struct stat filestat;
	json_object *root = NULL, *musicList = NULL;
	json_object *contentUrl = NULL;
	json_object *find = NULL;
	json_object *object = NULL;
	if((access(filename,F_OK))) {
		error("%s is not exsit", filename);
		return NULL;
	}

	stat(filename, &filestat);
	if(filestat.st_size <= 0)
		return NULL;
	fp = fopen(filename, "r");
	if(fp == NULL) {
		error("%s fopen error", filename);
		return NULL;
	}

	data = (char *)calloc(1,(filestat.st_size + 1) * sizeof(char)); //yes
	if(data == NULL)
		return NULL;
	ret = fread(data, sizeof(char), filestat.st_size, fp);
	if(ret !=  filestat.st_size)
	{   
		fclose(fp);
		if(data)
			free(data);
		error("%s fread error", filename);
		return NULL;
	}
	fclose(fp);
	root = json_tokener_parse(data);
	if (is_error(root)) 
	{
		goto exit;
	}
	
	musicList = json_object_object_get(root, "musiclist");
	if(musicList == NULL)
		goto exit;
	
	length = json_object_array_length(musicList);
	if(length <= 0 )
		goto exit;
	error("length:%d", length);
	for(i = 0; i < length; i++) {
		char *tmp = NULL;
		object =json_object_array_get_idx(musicList,i);
		contentUrl = json_object_object_get(object, "content_url");
		if(NULL != contentUrl) 
		{
			tmp = json_object_get_string(contentUrl);
			
			if(strcmp(tmp, url) == 0 || strstr(url, tmp) ) {
				find = object;
				break;
			}
			
		}	
	}
	if(find == NULL) {
		error("not find %s", url);
		goto exit;
	}
	music = NewMusicItem(); 
	if(music == NULL){
		error("NewMusicItem failed");
		goto exit;
	}

	object = json_object_object_get(find, "content_url");
	if(object) {
		char *content_url = NULL;
		content_url = json_object_get_string(object);
		if(content_url && strlen(content_url) > 0) {
			music->pContentURL = strdup(content_url);
			debug("music->pContentURL%s", music->pContentURL);
		}
	}
	
	object = json_object_object_get(find, "tip");
	if(object) {
		char *tip = NULL;
		tip = json_object_get_string(object);
		if(tip && strlen(tip) > 0) {
			music->pTip = strdup(tip);
			debug("music->pTip%s", music->pTip);
		}
	}	
	
	object = json_object_object_get(find, "title");
	if(object) {
		char *title = NULL;
		title = json_object_get_string(object);
		if(title && strlen(title) > 0) {
			music->pTitle = strdup(title);
			debug("music->pTitle%s", music->pTitle);
		}	
	}
	object = json_object_object_get(find, "artist");
	if(object) {
		char *artist = NULL;
		artist = json_object_get_string(object);
		if(artist && strlen(artist) > 0) {
			music->pArtist = strdup(artist);
			debug("music->pArtist:%s", music->pArtist);
		}	
	}
	
	object = json_object_object_get(find, "id");
	if(object) {
		char *id = NULL;
		id = json_object_get_string(object);
		if(id && strlen(id) > 0) {
			music->pId = strdup(id);
			debug("music->pId:%s", music->pId);
		}	
	}
	object = json_object_object_get(find, "type");
	if(object) {
		music->type = json_object_get_int(object);
	}
exit:
	if(data) {
		free(data);
		data=NULL;
	}
	if(root) {
		json_object_put(root);
	}
	return music;
}
/* 获取CURRENT_PLAYLIST_M3U中url的行数 */
int TuringPlaylistToIndex(char *url)
{
	FILE *dp =NULL;
	int ret = 0;
	size_t len = 0;
	int num = 0;
	ssize_t read;  
	char line[1024];
	if(url != NULL)
	{
		int playmode = cdb_get_int("$playmode", 0);	
		if(playmode == 4)		//tf卡模式
			dp= fopen("/usr/script/playlists/currentplaylist.m3u", "r");
		else
			dp= fopen(TURING_PLAYLIST_M3U, "r");
		if(dp == NULL)
		{
			perror("fopen CURRENT_PLAYLIST_M3U file failed:");
			return -1;
		}
			
		//while ((read = getline(&line, &len, dp)) != -1)  
		while(!feof(dp))
	    {  
	    	char *tmp = NULL;
	   	    memset(line, 0 , 1024);
	      
			tmp = fgets(line, sizeof(line), dp);
			if(tmp == NULL)
				break;
			int len =  strlen(line) -1;
			char ch = line[len];
			if(ch  == '\n' || ch  == '\r')
				line[strlen(line)-1] = 0;
		    line[strlen(line)] = 0;
	
			num++;
			if(strcmp(line, url)==0)
			{
				info("num=%d",num);
				ret  = num;
				break;
			}

		
	    }  
		/*if(line) {
			free(line);
			line = NULL;
		}*/
		if(dp) {
			fclose(dp);
		}
	}	

	return ret;
}
/* 播放CURRENT_PLAYLIST中链接为url歌曲,从头开始 */
void TuringPlaylistPlay(char *url)
{
	
	int index;
	cdb_set_int("$turing_stop_time",0);
	info("url:%s",url);	
#ifdef USE_MPDCLI
	MpdVolume(200);
	MpdClear();
	MpdRepeat(1);
	MpdSingle(0);
	MpdLoad(CURRENT_PLAYLIST);
#else
	exec_cmd("mpc voloume 200");
	exec_cmd("mpc clear");
	exec_cmd("mpc repeat on");
	exec_cmd("mpc single off");
	eval("mpc", "load", CURRENT_PLAYLIST);
#endif

	index = TuringPlaylistToIndex(url);
	error("index:%d",index);

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
/* 播放CURRENT_PLAYLIST中链接为url歌曲,从progress开始 */
void PlaylistPlaySeek(char *url, int progress)
{
	int index;
	cdb_set_int("$turing_stop_time",0);

    /* BEGIN: Added by Frog, 2018/5/31 */
    //bug点：
    //等待提示音播放完,否则后面的操作会使提示音播放不出来
    while(2 == MpdGetPlayState()){
		usleep(1000*200);
	} 
    /* END:   Added by Frog, 2018/5/31 */
   
#ifdef USE_MPDCLI
	MpdVolume(200);
	MpdClear();
	MpdRepeat(1);
	MpdSingle(0);
    info("CURRENT_PLAYLIST = %s",TURING_PLAYLIST);
	int playmode = cdb_get_int("$playmode", 0);	
	if(playmode == 4)//tf卡模式
	{
		//MpdLoad(CURRENT_PLAYLIST);
		info("playmode is tf test");
		MpdLoad("currentplaylist");		//加载文件到播放列表
	}					
	else
		MpdLoad(TURING_PLAYLIST);
#else
	exec_cmd("mpc voloume 200");
	exec_cmd("mpc clear");
	exec_cmd("mpc repeat on");
	exec_cmd("mpc single off");
	eval("mpc", "load", CURRENT_PLAYLIST);		
#endif

	index = TuringPlaylistToIndex(url);	//去播放列表查询"url"歌曲链接
	
	info("index:%d",index);
	index--;
	MpdRunSeek(index, progress);		//播放之前保存的歌曲链接
	info("exit");
}
/* 播放CURRENT_PLAYLIST中链接为url歌曲,从progress开始,初始状态暂停 */
void PlaylistPauseSeek(char *url, int progress)
{
	int index;	
	cdb_set_int("$turing_stop_time",0);
#ifdef USE_MPDCLI
	MpdVolume(200);
	MpdClear();
	MpdLoad(CURRENT_PLAYLIST);
#else
	eval("mpc", "voloume","200");
	eval("mpc", "clear");
	eval("mpc", "load", CURRENT_PLAYLIST);
#endif

	index = TuringPlaylistToIndex(url);
	
	info("index:%d",index);
	index--;
	MpdVolume(101);
	MpdRunPauseSeek(index, progress);
	MpdVolume(200);
	info("exit");
}
/* 
 *播放CURRENT_PLAYLIST中的歌曲
 * random: 0--从第一首播放 1--随机播放
 */
void PlaylistPlayRandom(int random)
{
	int length;
	char cmd[64] = {0};
	cdb_set_int("$turing_stop_time",0);
#ifdef USE_MPDCLI
	MpdClear();
	MpdLoad(CURRENT_PLAYLIST);
#else
	exec_cmd("mpc clear");
	bzero(cmd, 64);
	snprintf(cmd, 64,"mpc load %s", CURRENT_PLAYLIST);
	exec_cmd(cmd);
#endif
	if(random) {
		length = GetFileLength(CURRENT_PLAYLIST_M3U) ;
		warning("length:%d",length);
		srand( (unsigned)time(NULL) );	 
		int count = rand()%length + 1;
		count--;
		warning("count:%d", count);
		MpdPlay(count);
	} else {
		//MpdPlay(-1);
		MpdPlay(0);
	}
	info("exit");
}
/* 播放CURRENT_PLAYLIST中链接为status->url歌曲 */
void AudioStatusPlay(AudioStatus *status)
{
	int length;
	int index ;
	char cmd[64] = {0};
	cdb_set_int("$turing_stop_time",0);
#ifdef USE_MPDCLI
	MpdClear();
	MpdLoad(CURRENT_PLAYLIST);
#else
	exec_cmd("mpc clear");
	bzero(cmd, 64);
	snprintf(cmd, 64,"mpc load %s", CURRENT_PLAYLIST);
	exec_cmd(cmd);
#endif

	MpdRepeat(status->repeat);
	MpdRandom(status->random);
	info("status->url:%s",status->url);
	index = TuringPlaylistToIndex(status->url);
	index--;
	info("index:%d  status->progress:%d",index, status->progress);
	MpdRunSeek(index, status->progress);
	info("exit");
}



