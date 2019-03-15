#include "monitor.h"
#include "common.h"

#include <stdio.h>

#include <sys/socket.h>  
#include <sys/un.h> 

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cdb.h>
#include <json-c/json.h>

#define TURING_DOMAIN "/tmp/UNIX.turing"
#define HUABEI_DOMAIN "/tmp/UNIX.huabei"



void PrintUsage(char *exec, int err)
{
	FILE* o = stdout;
	if(err)
	{
  		o = stderr; 
	}
	fprintf(o,"usage: %s <chat|etoc|ctoe|rtsn|coll|coln|lock | low | normal> \n",exec);
	fprintf(o," chat: start Micro chat \n");
	fprintf(o," etoc: english translate to chinese \n");
	fprintf(o," etoc: chinese translate to english \n");
	fprintf(o," rtsn: report song name \n");	
	fprintf(o," pctm: play chat message\n");
	fprintf(o," coll: coll current song \n");
	fprintf(o," coln: switch column \n");
	fprintf(o," lock: locked  \n");
	fprintf(o," shut: shutdown \n");
	fprintf(o," start: starting up \n");
	fprintf(o," low : low power \n");
	fprintf(o," normal: normal power \n");
}


int OnWriteMsg(char *path, void* cmd, int len)
{
	int iRet = -1;
	int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, path);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

	flags = fcntl(iConnect_fd, F_GETFL, 0);
	fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, len);
	if (iRet != len)
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		error("write failed..",iRet);
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}




typedef struct turing_music
{	
	char *pArtist;
	char *pTitle;
	char *pTip;
	char *pContentURL;
	char *pId;	
	int type;
}TuringMusic;


static TuringMusic *GetTuringMusicFromJsonFile(char *filename,char *url)
{
	int ret ,length, i;
	FILE* fp = NULL;
	char *data = NULL;
	TuringMusic *music =  NULL;
	struct stat filestat;
	json_object *root = NULL, *musicList = NULL;
	json_object *contentUrl = NULL;
	json_object *find = NULL;
	json_object *object = NULL;
	error("enter");
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

	musicList = json_object_object_get(root,"musiclist");
	if(musicList == NULL)
		goto exit;
	
	length = json_object_array_length(musicList);
	if(length <= 0 )
		goto exit;

	for(i = 0; i < length; i++) {
		char *tmp = NULL;
		object =json_object_array_get_idx(musicList,i);
		contentUrl = json_object_object_get(object, "content_url");
		if(NULL != contentUrl) 
		{
			tmp = json_object_get_string(contentUrl);
			
			if(strcmp(tmp, url) == 0) {
				find = object;
				error("free tmp");
				//free(tmp);
				error("after free tmp");
				break;
			}
			
		}
		
	}
	if(find == NULL) {
		error("not find %s", url);
		goto exit;
	}

	music = calloc(1,sizeof(TuringMusic)); 
	if(music == NULL){
		error("calloc TuringMusic failed");
		goto exit;
	}

	object = json_object_object_get(find, "content_url");
	if(object) {
		music->pContentURL = strdup(json_object_get_string(object));
		error("music->pContentURL%s", music->pContentURL);
	}
	
	object = json_object_object_get(find, "tip");
	if(object) {
		music->pTip = strdup(json_object_get_string(object));
		error("music->pTip:%s", music->pTip);
	}	
	
	object = json_object_object_get(find, "title");
	if(object) {
		music->pTitle = strdup(json_object_get_string(object));
		error("music->pTitle:%s", music->pTitle);
	}
	object = json_object_object_get(find, "artist");
	if(object) {
		music->pArtist = strdup(json_object_get_string(object));
		error("music->pArtist:%s", music->pArtist);
	}
	
	object = json_object_object_get(find, "id");
	if(object) {
		music->pId = strdup(json_object_get_string(object));
	}
	
exit:
	if(data) {
		free(data);
		data=NULL;
	}
	if(root) {
		error(" json_object_put root");
		json_object_put(root);
		error("after json_object_put root");
	}
	error("exit");
	return music;
}

#define SINVOICE "/tmp/UNIX.sinvoice"

int main(int    argc, char **argv)
{
	
	int ret = -1;
	int sleep;
	int turingPowerState ;
	log_set_level(LOG_INFO);
	if(strcmp(argv[1], "turing") == 0) 
		ret = OnWriteMsg(TURING_DOMAIN, argv[2], strlen(argv[2]));
	else if(strcmp(argv[1], "sinvoice") == 0) 
		ret = OnWriteMsg(SINVOICE, argv[2], strlen(argv[2]));
	else if(strcmp(argv[1], "huabei") == 0) 
		ret = OnWriteMsg(HUABEI_DOMAIN, argv[2], strlen(argv[2]));
	else if(strcmp(argv[1], "aiwifi") == 0) {
		if(argc != 4) {
			fprintf(stderr, "usage: %s type[0|1|2|3|4] file\n", argv[0]);
		    return -1;
		}
		int type = atoi(argv[2]);
		
		ret = aiwifi(type , argv[3]);
	}
#ifdef ENABLE_OPUS
	else if(strcmp(argv[1], "opus") == 0) {
		if(argc != 4) {
			fprintf(stderr, "usage: trivial_example input.pcm output.pcm\n");
		    fprintf(stderr, "input and output are 16-bit little-endian raw files\n");
		    return -1;
		}
		ret = opus_test(argv[2], argv[3]);
	}
#endif

	return ret;
}


