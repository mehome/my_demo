#include "HuabeiPlaylist.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <fcntl.h>

#include "list.h"
#include "debug.h"

enum {
PLAYLIST_STATE_NONE = 0,
PLAYLIST_STATE_DONE ,
PLAYLIST_STATE_UPDATE,
};

typedef struct 	HuabeiPlaylist {
	int index;
	int state;
	int length ;
	char *url;
	list_t *list;
}HuabeiPlaylist;

static HuabeiPlaylist     *huabeiPlaylist = NULL;

HuabeiPlaylist           *newHuabeiPlaylist()
{
	HuabeiPlaylist *playlist = NULL;
	playlist = calloc(1, sizeof(HuabeiPlaylist));
	if(playlist == NULL) {
		error("calloc huabeiPlaylist error ");
		goto exit;
	}
	playlist->index  = -1;
	playlist->length = 0;
	playlist->state  = PLAYLIST_STATE_NONE;
	playlist->list   = NULL;
	playlist->url    = NULL;
	//playlist->list = list_new_free(free);
	//if(playlist == NULL) { 
	//	error("calloc huabeiPlaylist list_new_free");
	//	free(playlist);
	//	playlist  = NULL;
	//}
exit:
	return playlist;
}

int freeHuabeiPlaylist(HuabeiPlaylist **playlist)
{
	if(*playlist) {
		if((*playlist)->list) {
			list_destroy((*playlist)->list);
		}
		if((*playlist)->url) {
			free((*playlist)->url);
		}
		free((*playlist));
		*playlist= NULL;
	}
	return 0;
}

static int HuabeiMuluPlaylistCreate(HuabeiPlaylist *playlist)
{
	int ret = -1;
	int length = 0;
	FILE *fp =NULL;
	int num = 0;
	ssize_t read;  
	char line[1024] = {0};
	char file[128] = {0};
	char datDir[64] = {0};
	char value[16] = {0};
	list_t * list  = NULL;
	assert(playlist != NULL);

	exec_cmd("mpc update");
	
	if(playlist->list) {
		list_destroy(playlist->list);
	} 
	playlist->list = list_new_free(free);
	if(playlist->list == NULL) {
		error("list_new_free failed");
		goto exit;
	}
	list = playlist->list;
	
	if(playlist->url != NULL)
		free(playlist->url);

	cdb_get_str("$tf_mount_point" ,value ,16 ," ");
	snprintf(datDir ,64, "/media/%s", value);
	
	playlist->url = strdup(value);
	
	snprintf(file ,128 , "/media/%s/mulu.ini", value);
	fp= fopen(file, "r");
	if(fp == NULL) {
		ret = -1;
		goto exit;
		
	}
	info("playlist->url:%s" ,playlist->url);
	
	while(!feof(fp)) {
		char *tmp = NULL;
		char buf[256] = {0};
   	    memset(line, 0 , 1024);
		tmp = fgets(line, sizeof(line), fp);
		if(tmp == NULL)
			break;
		int len =  strlen(line) -1;
		char ch = line[len];
		if(ch  == '\n' || ch  == '\r')
			line[strlen(line)-1] = 0;
	    line[strlen(line)] = 0;
		snprintf(buf, 256, "%s/%s/%s.lrc", datDir,line, line);
		if(access(buf, F_OK) == 0) {
			char *dirname = NULL;
			length++;
			dirname = strdup(line);
			info("buf:%s", buf);
			info("dirname:%s", dirname);
			list_add(list, dirname);
		}
		
	}

	int listLen = list_length(list);
	info("length:%d len:%d" ,listLen, length);
	playlist->length = listLen ;
	playlist->state = PLAYLIST_STATE_DONE;
	
	if(fp)
		fclose(fp);
	return 0;
exit:
	if(fp)
		fclose(fp);
	if(playlist->list) {
		list_destroy(playlist->list);
	} 
	return -1;

}

static int HuabeiPlaylistCreate(HuabeiPlaylist *playlist)
{
	int 	ret = -1;
	int 	len = 0;
	DIR     *dir = NULL;
	struct dirent   *next = NULL;
	char datDir[64] = {0};
	char value[16] = {0};
	list_t * list  = NULL;
	assert(playlist != NULL);

	exec_cmd("mpc update");
	
	if(playlist->list) {
		list_destroy(playlist->list);
	} 
	playlist->list = list_new_free(free);
	if(playlist->list == NULL) {
		error("list_new_free failed");
		goto exit;
	}
	list = playlist->list;
	
	if(playlist->url != NULL)
		free(playlist->url);
	
	cdb_get_str("$tf_mount_point" ,value ,16 ," ");
	snprintf(datDir ,64, "/media/%s", value);
	
	if ((dir = opendir(datDir)) == NULL) {
		error("calloc huabeiPlaylist list_new_free ");
 		goto exit;
	}
	playlist->url = strdup(value);
	info("playlist->url:%s" ,playlist->url);
	while ((next = readdir(dir)) != NULL) {
	
		char buf[256] = {0};
		snprintf(buf, 256, "%s/%s/%s.lrc",datDir, next->d_name, next->d_name);
		if(access(buf, F_OK) == 0) {
			DEBUG_INFO("buf:%s",buf);
			char *dirname = NULL;
			len++;
			dirname = strdup(next->d_name);
			info("buf:%s", buf);
			info("dirname:%s", dirname);
			list_add(list, dirname);
		}
	}
	int length = list_length(list);
	info("length:%d len:%d" ,length, len);
	playlist->length = length ;
	closedir(dir);
	playlist->state = PLAYLIST_STATE_DONE;
	return 0;
exit:
	if(playlist->list) {
		list_destroy(playlist->list);
	} 
	return -1;

}

static int HuabeiPlaylistPlayIndex(HuabeiPlaylist *playlist, int mode) 
{
	list_t *list = playlist->list;
	int index = playlist->index;
	int length = playlist->length; 
	char *url = playlist->url;
	char *dataUrl = NULL; 
	int order;
	char value[128] = {0};
	char muluFile[128] = {0};
	if(length <= 0)
		return -1;
	if(mode == PLAYLIST_MODE_PREV ) {
		if(index > 0)
			order = index - 1;
		else if(index == 0) 
			order = length - 1;
		else 
			order = 0;
	} else if(mode == PLAYLIST_MODE_NEXT) {
		order = (index+1)%length;
	}
	playlist->index = order;
	list_node_t * node = list_at(list, order);
	if(node == NULL)
		return -1;	
	if(url == NULL)
		return -1;
	char *dirname = node->val;
	SetHuabeiDataDirName(dirname);
	snprintf(value, 128, "%s/%s",url,  dirname);
	snprintf(muluFile ,128 , "/media/%s/mulu.ini", url);
	info("muluFile:%s",muluFile);
	CheckMuluAndUpdate(muluFile, dirname);
	SetHuabeiDataUrl(value);
	//SetHuabeiDataUrl(playlist->url);
	//return HuabeiPlayHttp("mc");
	return 0;
}

int HuabeiPlaylistInit()
{
	int ret = -1;
	if(huabeiPlaylist  == NULL)
		huabeiPlaylist = newHuabeiPlaylist();
	if(huabeiPlaylist == NULL)
		return -1;
	return HuabeiMuluPlaylistCreate(huabeiPlaylist);
}
int HuabeiPlaylistDeinit()
{
	freeHuabeiPlaylist(&huabeiPlaylist);
}


int HuabeiPlaylistPrevNext(int mode)
{
	int ret = 0;
	ret = HuabeiPlaylistInit();
	if(ret < 0)
		return ret;
	ret = HuabeiPlaylistPlayIndex(huabeiPlaylist, mode);
	if(ret < 0)
		return ret;
	return HuabeiPlayHttp("mc");
}

int HuabeiPlaylistPrev()
{
	return HuabeiPlaylistPrevNext(PLAYLIST_MODE_PREV);
}

int HuabeiPlaylistNext() 
{
	return HuabeiPlaylistPrevNext(PLAYLIST_MODE_NEXT);
	
}





