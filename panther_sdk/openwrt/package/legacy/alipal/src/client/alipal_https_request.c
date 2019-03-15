#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <wchar.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <json-c/json.h>
#include <sys/wait.h>  
#include <sys/types.h> 
#include <regex.h>

#include <sys/sem.h>//包含信号量定义的头文件
#include <sys/ipc.h>

#include "alipal_https_request.h"

FILE 	*fpcurl = NULL;//这边这个定义一个curl专用的


//###############################################################################
//设置信号量的值
int WIFIAudio_Semaphore_SetSemValue(int sem_id, int value)
{
	int ret = 0;
	
	WASEM_Semun sem_union;
	sem_union.Value = value;

	semctl(sem_id,0,SETVAL,sem_union);

	return ret;
}


//根据key值，创建或获取信号量，如果是创建赋予初值
int WIFIAudio_Semaphore_CreateOrGetSem(key_t key, int value)
{
	int sem_id = -1;
	//key = ftok("./file_sem.txt", 1);
	//创建一个新的信号量或者是取得一个已有信号量的键,初始值为1
	sem_id = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL);
	if(sem_id < 0)
	{
		sem_id = semget(key, 1, 0666 | IPC_CREAT);
	}
	else
	{
		WIFIAudio_Semaphore_SetSemValue(sem_id, value);
	}
	
	return sem_id;
}

//删除信号量，如果集合已为空则直接删除
int WIFIAudio_Semaphore_DeleteSem(int sem_id)
{
	int ret = 0;
	
	WASEM_Semun sem_union;
	
	semctl(sem_id,0,IPC_RMID,sem_union);
	
	return ret;
}

//信号量P操作：对信号量进行减一操作
int WIFIAudio_Semaphore_Operation_P(int sem_id)
{
	int ret = 0;
	struct sembuf sembuff;

	sembuff.sem_num = 0;//信号量编号
	sembuff.sem_op = -1;//P操作	
	sembuff.sem_flg = SEM_UNDO;
	
	//printf("开始P操作\r\n");
	semop(sem_id,&sembuff,1);
	//printf("P操作结束\r\n");
	
	return ret;
}

//信号量V操作：对信号量进行加一操作
int WIFIAudio_Semaphore_Operation_V(int sem_id)
{
	int ret = 0;
	struct sembuf sembuff;

	sembuff.sem_num = 0;//信号量编号
	sembuff.sem_op = 1;//V操作
	sembuff.sem_flg = SEM_UNDO;
	
	//printf("开始V操作\r\n");
	semop(sem_id,&sembuff,1);
	//printf("V操作结束\r\n");
	return ret;
}


//这边是文档中标准的回调函数
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)fpcurl);
	return written;
}


//关于lcurl访问的
char * AliPal_HttpGetUrl(char *purl)
{
	char *pstring = NULL;
	CURL *curl;
	CURLcode res;
	struct stat filestat;//用来读取文件大小
	int sem_id = 0;//信号量ID
	
	if(NULL == purl)
	{
		printf("Error of Parameter : NULL == purl in WIFIAudio_SystemCommand_HttpGetUrl\r\n");
		pstring = NULL;
	}
	else
	{
		res = curl_global_init(CURL_GLOBAL_ALL);
		if(0 == res)
		{
			curl = curl_easy_init();
			if (NULL != curl)
			{
				curl_easy_setopt(curl, CURLOPT_URL, purl);
				
				sem_id = WIFIAudio_Semaphore_CreateOrGetSem(\
				(key_t)WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_SEMKEY, 1);
				WIFIAudio_Semaphore_Operation_P(sem_id);
				if(NULL == (fpcurl = fopen(\
				WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME, "w+")))//文件内容会覆盖，不存在则创建,可读
				{
					curl_easy_cleanup(curl);
					curl_global_cleanup();
					pstring = NULL;
				}
				else
				{
					//将后续的动作交给write_data回调函数处理，libcurl接收到数据后将数据保存
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
					res = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
					fclose(fpcurl);
					fpcurl = NULL;
					if(0 == res)
					{
						stat(WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME, &filestat);
						pstring = (char *)calloc(1, filestat.st_size * sizeof(char));
						if(NULL != pstring)
						{
							fpcurl = fopen(WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME, "r");
							if(NULL != fpcurl)
							{
								fread(pstring, sizeof(char), filestat.st_size, fpcurl);
								fclose(fpcurl);
								fpcurl = NULL;
							}
						}
					}					
				}
				WIFIAudio_Semaphore_Operation_V(sem_id);
			}
			curl_global_cleanup();
		}
	}
		
	return pstring;
}





