#include "HuabeiMsgDataFileHandle.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <json-c/json.h>
#include "HuabeiJson.h"

#include "debug.h"
#include "HuabeiUart.h"
#include "http.h"

// 0 exsit;


//HuabeiDownLoadFile

int HuabeiDownLoadFile(char *url, char *filename, int *operate)
{
	char datDir[64] = {0};
	char value[16] = {0};
	char datUrl[512] = {0};
	char lrcUrl[512] = {0};
	char lrcFile[512] = {0};
	char datFile[512] = {0};
	int tf =0;
	int ret;
#if 0
	tf = cdb_get_int("$tf_status", 0);
	if(tf == 0) {
		*operate = 0;
		snprintf(datDir ,64, "/tmp/huabei", value);
	} else {
		*operate = 1;
		cdb_get_str("$tf_mount_point",value,16,"");
		snprintf(datDir ,64, "/media/%s/huabei", value);
	}
#else
	*operate = 0;
	snprintf(datDir ,64, "/tmp/huabei", value);
#endif
	ret = create_path(datDir);
	warning("ret:%d",ret);
	if(ret < 0)
		return ret;
	exec_cmd("rm  /tmp/huabei/*");
	snprintf(lrcFile ,512, "%s/%s.lrc", datDir, filename);
	snprintf(datFile ,512, "%s/%s.dat", datDir, filename);
	//snprintf(lrcFile ,512, "%s/%s/%s.lrc", datDir,filename,  filename);
	//snprintf(datFile ,512, "%s/%s/%s.dat", datDir, filename, filename);
	
	ret = access(lrcFile, F_OK);
	warning("%s access ret:%d",lrcFile, ret);
	ret |= access(datFile, F_OK);
	warning("%s access ret:%d",datFile, ret);

	if(ret != 0) {
		//snprintf(datUrl ,128, "%s.dat", url);
		//snprintf(lrcUrl ,128, "%s.lrc", url);
		snprintf(datUrl ,128, "%s/%s.dat", url, filename);
		snprintf(lrcUrl ,128, "%s/%s.lrc", url, filename);
		ret = HttpDownLoadFile(datUrl, datFile);
		ret |= HttpDownLoadFile(lrcUrl, lrcFile);
	}
	return ret;
	
}

int HandleDataFile(char*url, char*dir, int *operate)
{
	int tfStatus = 0, ret = -1;
	tfStatus = cdb_get_int("$tf_status", 0);
	warning("tf_status=%d", tfStatus);	
	if(tfStatus) 
	{	
		
		ret = CheckMuluIni(url, dir);
		exec_cmd("mpc update");
		warning("ret=%d", ret);	
		if(ret == 0) {
			*operate = 1;
		}
	} 
	if (ret != 0) {
		ret = HuabeiDownLoadFile(url, dir, operate);
		warning("ret=%d", ret);	
		SetHuabeiDataUrl(url);
	}
	
	return ret;
}

int DataFileHanlder(void *arg)
{
	json_object *root = NULL ,*message = NULL;

	json_object *pSub = NULL;
	char *url = NULL;
	char *dataFileName = NULL;
	int operate ;
	int ret;
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

	pSub = json_object_object_get(message, "url");
	if(!pSub) {
		error("json_object_object_get url failed");
		goto exit;
	}
	url = json_object_get_string(pSub);
	info("url:%s",url);
	pSub = json_object_object_get(message, "arg");
	if(!pSub) {
		error("json_object_object_get arg failed");
		goto exit;
	}
	dataFileName = json_object_get_string(pSub);
	
	SetHuabeiDataDirName(dataFileName);
#if 0
	SetHuabeiDataUrl(url);
	ret = HuabeiDownLoadFile(url, dataFileName, &operate);
	info("ret:%d",ret);
#endif
	ret = HandleDataFile(url, dataFileName, &operate);
	info("ret:%d",ret);

	if(ret != 0)
		goto exit;
	info("dataFileName:%s",dataFileName);
	return HuabeiUartSendData(dataFileName, operate);
exit:
	return -1;
}


