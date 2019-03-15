#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <json/json.h>


#include "column.h"

enum{
	OPERATE_SYNC = 0,
	OPERATE_DELETE,			
};


static char *gColumnStoryDir[] = {
	COLUMN_BEDTIME_STORIES_DIR,
	COLUMN_FAIRY_TALES_DIR
};
	
static char *gColumnStoryPlaylist[] = {
	COLUMN_BEDTIME_STORIES_PLAYLIST,
	COLUMN_FAIRY_TALES_PLAYLIST
};	

static char *gColumnMusicDir[] = {
	COLUMN_NURSERY_RHYMES_DIR
};

static char *gColumnMusicPlaylist[] = {
	COLUMN_NURSERY_RHYMES_PLAYLIST,
};
		
	


static int storyLength = sizeof(gColumnStoryDir)/sizeof(gColumnStoryDir[0]);
static int musicLength = sizeof(gColumnMusicDir)/sizeof(gColumnMusicDir[0]);

char *GetJsonName(char *json)
{
	json_object *root = NULL;
    json_object *object,*getnumobject,*getlistobject,*musiclistobj,*urlobject;
	int num, length;
	char *names = NULL;
	int size = 0 ,namesLength; 
	namesLength = 128;
	root = json_tokener_parse(json);
	if (is_error(root)) 
	{
		return NULL;
	}
	names = calloc(namesLength, sizeof(char));
	if(names == NULL) {
		error("calloc failed");
		goto failed;
	}

	getnumobject = json_object_object_get(root,"num");
	if(getnumobject != NULL)
	{
		 num = json_object_get_int(getnumobject);
	}

	getlistobject = json_object_object_get(root,"musiclist");
	if(getlistobject == NULL) {
		goto  failed;
	}
	
	length = json_object_array_length(getlistobject);
	info("length:%d",length);
	if(length == 0)
		goto  failed;
	int i ;
	
	for(i = 0; i < length; i++) 
	{
		const char *title = NULL;
		const char *url = NULL;
		
		musiclistobj =json_object_array_get_idx(getlistobject,i);
		object = json_object_object_get(musiclistobj, "content_url");
		if(NULL != object) 
		{
			url = json_object_get_string(object);
		}
		
		object = json_object_object_get(musiclistobj, "title");
		if(NULL != object) 
		{
			title = json_object_get_string(object);
		}
		if(url) {
			if(title != NULL) {
				error("title:%s",title);
				size += strlen(title);
				if(size + 2 > namesLength) {
					namesLength += 32;
					names == safe_realloc(names, namesLength);	
				}
				strcat(names, title);
				if(i != length - 1) {
					strcat(names, "&");
				}
			}
			
		}

	}
	json_object_put(object);
	return names;
failed:		
	if(names) {
		free(names);
	}
	json_object_put(object);
	return NULL;
}

char *GetPlaylistName(char *filename)
{
	struct stat filestat;
	FILE *fp = NULL;
	int ret;
	char *json = NULL;
	char *names = NULL;
	
	if((access(filename,F_OK))!=0) 
		return  NULL;

	fp = fopen(filename, "r");
	if(fp == NULL)
		return NULL;

	stat(filename, &filestat);
	if(filestat.st_size > 0) 
	{
		debug("filestat.st_size:%ld",filestat.st_size);
		json = (char *)calloc(1,(filestat.st_size + 1) * sizeof(char)); //yes
		if(NULL != json)
		{    
			ret = fread(json, sizeof(char), filestat.st_size, fp);
			if(ret > 0)
			{   info("ret:%d pjsonstring:%s",ret,json);
				names = GetJsonName(json);
				
			}
			free(json);
			json=NULL;
		}
		
	}
	fclose(fp);
	return names;
}

int CreatePlaylistFile(char *columnDir, char *file)
{
	int ret;
	char columnRootDir[256] = {0};
	char updateDir[256] = {0};
	char value[16] = {0};
	char cmd[1024] = {0};
	char playlistFileDir[128] = {0};
	char playlistJsonDir[128] = {0};
	
	cdb_get_str("$tf_mount_point",value,16,"");

	snprintf(columnRootDir ,256, "/media/%s/%s", value, columnDir);
	snprintf(updateDir ,256, "%s/%s", value, columnDir);
	error("updateDir:%s",updateDir);
	
	ret = create_path(columnRootDir);
	warning("ret:%d",ret);
	if(ret < 0)
		return 1;
	
	snprintf(playlistFileDir, 128, "/usr/script/playlists/%s.m3u",file);
	snprintf(playlistJsonDir, 128, "/usr/script/playlists/%s.json",file);
	error("playlistFileDir:%s", playlistFileDir);
	//playlistFileDir:/usr/script/playlists/collplaylist.m3u
	unlink(playlistFileDir);
	unlink(playlistJsonDir);
#if 0
//#ifdef USE_MPDCLI
	MpdUpdate(value);
#else
	exec_cmd("mpc update");
	bzero(cmd, 1024);
	snprintf(cmd, 1024, "mpc update %s",updateDir);
	exec_cmd(cmd);
#endif
	sleep(1);
	bzero(cmd, 1024);
	//snprintf(cmd, 1024, "m mpc listall |  grep \"%s\" > 1.txt", updateDir);
	//exec_cmd(cmd);
	//error("cmd:%s", cmd);
	snprintf(cmd, 1024, "mpc listall | grep \"%s\" > /tmp/tmp.txt && cp /tmp/tmp.txt %s", updateDir, playlistFileDir);
	warning("cmd:%s",cmd);
	exec_cmd(cmd);
	bzero(cmd, 1024);
	snprintf(cmd, 1024, "creatPlayList column %s", file);
	exec_cmd(cmd);
	return 0;
}


void SynCreatePlaylistFile( char *dir[] ,  char * file[], int length)
{
	int i;
	char playlistFileName[128] = {0};
	for(i = 0; i < length; i++) 
	{
		snprintf(playlistFileName, 128, "/usr/script/playlists/%s.json",file[i]);
		CreatePlaylistFile(dir[i], file[i]);	
	}
}
char *GetLocalName( char *dir[] ,  char * file[],int length, int type) 
{	
	char playlistFileName[128] = {0};
	int i = 0;
	char *names = NULL; 
	char *name = NULL;
	int namesLength = 256;
	int size = 0;
	names = calloc(namesLength, sizeof(char));
	if(names == NULL) {
		error("calloc failed");
		return NULL;
	}
	for(i = 0; i < length; i++) 
	{
		snprintf(playlistFileName, 128, "/usr/script/playlists/%s.json",file[i]);
		if(type == 0)
			CreatePlaylistFile(dir[i],file[i]);
		
		name = GetPlaylistName(playlistFileName);
		if(name != NULL) 
		{
			error("name:%s",name);
			if(size + 2 > namesLength ) {
				namesLength += 128; 
				size += strlen(name);
				names = safe_realloc(names,namesLength);	
			}
			strcat(names, name);
			if(i != length - 1) {
				strcat(names, "&");
			}
			free(name);
			name = NULL;
		}
	}
	return names;
}



void SynchronizeLocalStory()
{
	char *name = NULL;
	char *speech = "请插入地球TF卡";
	int tfStatus; 
	
	tfStatus = cdb_get_int("$tf_status", 0);
	
	if(tfStatus == 0) 
	{
		name = GetLocalName(gColumnStoryDir,gColumnStoryPlaylist ,storyLength, OPERATE_DELETE);
		IotSynchronizeLocalStoryReport(OPERATE_DELETE, name);
	} else 
	{
		name = GetLocalName(gColumnStoryDir,gColumnStoryPlaylist ,storyLength, OPERATE_SYNC);
		IotSynchronizeLocalStoryReport(OPERATE_SYNC,name);

	}

	if(name) {
		free(name);
	}
}



void SynchronizeLocalMusic()
{
	
	char *name = NULL;

	int tfStatus; 

	tfStatus = cdb_get_int("$tf_status", 0);
	error("tfStatus:%d",tfStatus);
	
	if(tfStatus == 0) 
	{
		name = GetLocalName(gColumnMusicDir,gColumnMusicPlaylist ,musicLength, OPERATE_DELETE);
		IotSynchronizeLocalMusicReport(OPERATE_DELETE, name);
	} 
	else 
	{
		name = GetLocalName(gColumnMusicDir,gColumnMusicPlaylist ,musicLength, OPERATE_SYNC);
		IotSynchronizeLocalMusicReport(OPERATE_SYNC,name);
	}
	if(name) {
		free(name);
	}
}



void SyncLocalResource()
{
	SynchronizeLocalStory();
	SynchronizeLocalMusic();
}


void SyncCollectResource()
{
	CreatePlaylistFile(COLUMN_COLLECTION_DIR, COLUMN_COLLECTION_PLAYLIST);
}





