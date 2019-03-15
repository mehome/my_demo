
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <json-c/json.h>
#include "HuabeiJson.h"

#include "debug.h"
#include "HuabeiUart.h"
#include "http.h"

static char *suffix = "tar";
#define MULU_INI_FILE "mulu.ini"

int FileUpdate(char *file, char *value)
{
	int writeLen;
	int len, ret = 0;
	FILE *dp =NULL;
	len = strlen(value);
	printf("len:%d\n", len);
	dp= fopen(file, "a+");
	if(dp == NULL)
		return -1;
	writeLen = fwrite(value, sizeof(char), len, dp);
	if(writeLen !=  len) {	
		ret = -1;
		printf("%s fwrite error\n", file);
		goto exit;
	}
	
	//fwrite("\n", sizeof(char), strlen("\n"), dp);
exit:
	if(dp)
		fclose(dp);
	return ret;
}

int CheckMuluAndUpdate(char *file, char *dir)
{
	int ret = 0;
	ret = isInFile(file, dir);
	info("ret:%d, %s is %s",ret,dir,  ret == 1 ? "in" : "not in");
	if(ret == 0) 
	{
		ret = FileUpdate(file, dir);
		FileUpdate(file, "\n");
	} 
	return ret;
}

int isInFile(char *file, char *dir)
{
	FILE *dp =NULL;
	int ret = 0;
	size_t len = 0;
	int num = 0, enter = 0;
	char line[1024];
	if( access(file, F_OK) !=0)
	{
		umask(0);
		creat(file, 0666);
		return 0;
	}
	dp= fopen(file, "r");
	if(dp == NULL)
		return 0;
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
		if(ch  == '\n' || ch  == '\r') {
			enter++;
			line[strlen(line)-1] = 0;
		}
	    line[strlen(line)] = 0;
		info("line:<%s>",line);
		num++;
		if(strcmp(line, dir)==0)
		{
			info("num=%d",num);
			ret  = 1;
			//break;
		}

    }  

	if(dp) {
		fclose(dp);
	}
	if(num != enter) {
		FileUpdate(file, "\n");
	}
		
	info("num=%d enter:%d",num,enter);
	return ret;
}

int CheckMuluIni(char *url , char *dir)
{
 	int ret = -1;
	char value[16] = {0};
	char muluFile[128] = {0};
	char cmd[128] = {0};
	char data[128] = {0};
	info("dir:%s",dir);
	cdb_get_str("$tf_mount_point",value,16,"");
	snprintf(muluFile ,128 , "/media/%s/%s", value, MULU_INI_FILE);

	snprintf(data, 128, "%s/%s",value, dir);

	info("muluFile:%s",muluFile);
	if(isInFile(muluFile, dir)) 
	{	
		memset(cmd, 0, 128);
		snprintf(cmd, 128, "/media/%s/%s/%s.dat", value, dir, dir);
		info("cmd :%s",cmd);
		ret = access(cmd, F_OK);
		info("ret :%d",ret);
		if(ret == 0) {
			SetHuabeiDataUrl(data);
		}
	} 
	else 
	{
		char download[128] = {0};
		char local[128] = {0};
		HuabeiEventNotifyStatus(0x90);
		SetSendFlag(0);
		snprintf(download ,128 , "%s/%s.%s", url, dir, suffix);
		snprintf(local ,128 , "/media/%s/%s.%s", value, dir, suffix);
		info("download:%s",download);
		info("local:%s",local);
		double downloadSize = GetDownloadFileLenth(download);
		if(downloadSize <= 0) {
			SetSendFlag(1);
			return -1;
		}
		ret = HttpDownLoadFile(download, local);
		info("ret:%d",ret);
		if(ret == 0) 
		{	
			double fileSize = getFileSize(local);
			info("downloadSize:%f", downloadSize);
			info("fileSize:%f", fileSize);
			if(downloadSize == fileSize) {
				snprintf(cmd, 256, "tar -xvf %s -C /media/%s/", local, value);
				info("local:%s",local);
				ret = exec_cmd(cmd);
				memset(cmd, 0, 128);
				snprintf(cmd, 128, "/media/%s/%s/%s.dat", value, dir, dir);
				ret =  access(cmd, F_OK);
				info("ret :%d",ret);
				if(ret == 0) {
					CheckMuluAndUpdate(muluFile, dir);
					SetHuabeiDataUrl(data);
					HuabeiEventNotifyStatus(0x82);
				}
				
			}	else {
				error("Download error");
			}
			unlink(local);
		}
		SetSendFlag(1);
	}
	MpdUpdate(data);
	exec_cmd("mpc update");
	info("ret :%d",ret);
	return ret;
}












