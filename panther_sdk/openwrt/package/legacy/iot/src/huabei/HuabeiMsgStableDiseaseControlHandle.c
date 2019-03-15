#include "HuabeiMsgStableDiseaseControlHandle.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include "http.h"


#include "debug.h"
enum {
	OPERATE_ADD_REPLACE = 1,
	OPERATE_DELETE,
};
	
static char *suffix="tar";

int HuabeiDownloadTarFile(char *url , char *dir)
{
	int ret = -1;
	char value[16] = {0};
	char muluFile[128] = {0};
	char cmd[256] = {0};
	char data[128] = {0};
	char download[128] = {0};
	char local[128] = {0};
	info("dir:%s",dir);
	cdb_get_str("$tf_mount_point",value,16,"");
	
	snprintf(data, 128, "%s/%s",value, dir);
	snprintf(download ,128 , "%s/%s.%s", url, dir, suffix);
	snprintf(local ,128 , "/media/%s/%s.%s", value, dir, suffix);

	info("value:%s",value);
	info("data:%s",data);
	HuabeiEventNotifyStatus(0x90);
	SetSendFlag(0);
	double downloadSize = GetDownloadFileLenth(download);
	if(downloadSize <= 0) {
		return -1;
	}
	info("download:%s",download);
	info("local:%s",local);
	ret = HttpDownLoadFile(download, local);
	info("ret:%d",ret);
	if(ret == 0) 
	{	
		double fileSize = getFileSize(local);
		info("filesize:%f", fileSize);
		info("downloadSize:%f",downloadSize);
		if( downloadSize == fileSize ) {
			snprintf(muluFile ,128 , "/media/%s/mulu.ini", value);
			memset(cmd, 0, 256);
			snprintf(cmd, 256, "rm /media/%s/%s -rf", value, dir);
			info("cmd:%s",cmd);
			ret = exec_cmd(cmd);
			memset(cmd, 0, 256);
			snprintf(cmd, 256, "tar -xvf %s -C /media/%s/", local, value);
			info("cmd:%s",cmd);
			ret = exec_cmd(cmd);
			CheckMuluAndUpdate(muluFile, dir);
			info("data:%s",data);
			MpdUpdate(data);
			exec_cmd("mpc update");
			HuabeiEventNotifyStatus(0x82);
			
		} else {
			error("Download error");
		}
		unlink(local);
	}
	SetSendFlag(1);
	info("ret :%d",ret);
	return ret;
}

int HuabeiAddReplace(char *url, char * filename)
{
	char datDir[64] = {0};
	char lrcDir[64] = {0};
	char value[16] = {0};
	char lrcFile[512] = {0};
	char datFile[512] = {0};
	int tf =0;
	int ret;
	tf = cdb_get_int("$tf_status", 0);
	info("tf:%d",tf);
	if( tf == 0)
		return -1;
	ret = HuabeiDownloadTarFile(url, filename);

	info("ret:%d",ret);
	
	return ret;
	
}
int HuabeiDeleteFile(char *url, char * filename)
{
	char cmd[256] = {0};
	char value[16] = {0};
	char muluFile[128] = {0};
	int tf =0;
	int ret = -1;
	
	tf = cdb_get_int("$tf_status", 0);
	info("tf:%d",tf);
	if( tf == 0)
		return -1;
	cdb_get_str("$tf_mount_point",value,16,"");
	info("value:%s",value);

	snprintf(muluFile ,128 , "/media/%s/mulu.ini", value);
	
	memset(cmd, 0, 256);
	snprintf(cmd ,256, "rm /media/%s/%s -rf",value, filename);
	info("cmd:%s", cmd);
	exec_cmd(cmd);
	
	memset(cmd, 0, 256);
	snprintf(cmd , 256, "%s/%s", value, filename);
	MpdUpdate(cmd);
	exec_cmd("mpc update");
	
	memset(cmd, 0, 256);
	snprintf(cmd ,256, "sed -i '/%s/'d %s", filename, muluFile);
	info("cmd:%s", cmd);
	info("muluFile:%s",muluFile);
	exec_cmd(cmd);
	

	return ret;
}

int StableDiseaseControlHandler(void *arg)
{

	json_object *root = NULL, *message = NULL;;
	
	json_object *pSub = NULL;
	char *dataFileName = NULL;
	char *url  = NULL;
	int operate ;
	info("...");

	root = (json_object *)arg;
	if (is_error(root)) 
	{
		error("json_tokener_parse root failed");
		goto exit;
	}
	message = json_object_object_get(root, "message");
	if(!message) {
		error("json_object_object_get message failed");
		goto exit;
	}

	pSub = json_object_object_get(message, "operate");
	if(!pSub) {
		error("json_object_object_get operate failed");
		goto exit;
	}
	operate = json_object_get_int(pSub);
	info("operate:%d",operate);
	pSub = json_object_object_get(message, "arg");
	if(!pSub) {
		error("json_object_object_get arg failed");
		goto exit;
	}
	dataFileName = json_object_get_string(pSub);
	info("dataFileName:%s",dataFileName);
	pSub = json_object_object_get(message, "url");
	if(!pSub) {
		error("json_object_object_get url failed");
		goto exit;
	}
	url = json_object_get_string(pSub);
	info("url:%s",url);
	info("operate:%d",operate);
	switch(operate) {
		case OPERATE_ADD_REPLACE:
			HuabeiAddReplace(url, dataFileName);
			break;
		case OPERATE_DELETE:
			HuabeiDeleteFile(url, dataFileName);
			break;
		default:
			break;
	}


	return 0;
exit:
	return -1;


}


