#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>

#include "filepathnamesuffix.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_RealTimeInterface.h"
#include "WIFIAudio_Semaphore.h"


WARTI_pStr WIFIAudio_PostAlarmClock_GetLastAlarmClockpStr(int n)
{
	char filename[128];
	struct stat filestat;//用来读取文件大小
	int sem_id = 0;//信号量ID
	FILE *fp = NULL;
	WARTI_pStr pstr = NULL;
	char *pjsonstring = NULL;
	int ret = 0;
	
	//这边要进去读取之前的文件
	memset(filename, 0, sizeof(filename)/sizeof(char));
	sprintf(filename, "%s", WIFIAUDIO_POSTALARMCLOCK_PATHNAME);

	sprintf(filename, filename, n);
	stat(filename, &filestat);

	sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_POSTALARMCLOCK_SEMKEY, 1);
	WIFIAudio_Semaphore_Operation_P(sem_id);

	if(NULL == (fp = fopen(filename, "r")))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pstr = NULL;
	} else {
		pjsonstring = (char *)calloc(1, (filestat.st_size) * sizeof(char));
		if(NULL == pjsonstring)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pstr = NULL;
		} else {
			ret = fread(pjsonstring, sizeof(char), filestat.st_size, fp);
		}
		
		fclose(fp);
		fp = NULL;
	}

	WIFIAudio_Semaphore_Operation_V(sem_id);

	if(ret > 0)
	{
		pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_GETALARMCLOCK, pjsonstring);
		
		free(pjsonstring);
		pjsonstring = NULL;
		
		if(NULL == pstr)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			pstr = NULL;
		} else {
			
		}
	}
	
	return pstr;
}

int WIFIAudio_PostAlarmClock_AddAlarmClockNumToCmd(char *pcmd, WARTI_pStr pstr)
{
	int ret = 0;
	
	if((NULL == pcmd) || (NULL == pstr))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if((pstr->Str.pGetAlarmClock->N < 0) || (pstr->Str.pGetAlarmClock->N > 1))
		{
			ret = -1;
		} else {
			sprintf(pcmd, "%s%d", pcmd, pstr->Str.pGetAlarmClock->N);
		}
	}
	
	return ret;
}

int WIFIAudio_PostAlarmClock_AddAlarmClockEnableToCmd(char *pcmd, WARTI_pStr pstr)
{
	int ret = 0;
	
	if((NULL == pcmd) || (NULL == pstr))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(WIFIAUDIO_REALTIMEINTERFACE_INTERRORCODE == pstr->Str.pGetAlarmClock->Enable)
		{
			//在没有填enable为null的情况下，填使能 1
			sprintf(pcmd, "%s %d", pcmd, 1);
		} else if((pstr->Str.pGetAlarmClock->Enable >= 0) && (pstr->Str.pGetAlarmClock->Enable <= 1)) {
			sprintf(pcmd, "%s %d", pcmd, pstr->Str.pGetAlarmClock->Enable);
		} else {
			ret = -1;
		}
	}
	
	return ret;
}

int WIFIAudio_PostAlarmClock_AddEveryDayAlarmClockToCmd(char *pcmd, WARTI_pStr pstr)
{
	int ret = 0;
	
	if((NULL == pcmd) || (NULL == pstr))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if((pstr->Str.pGetAlarmClock->Minute < 0) || (pstr->Str.pGetAlarmClock->Minute > 60) \
		|| (pstr->Str.pGetAlarmClock->Hour < 0) || (pstr->Str.pGetAlarmClock->Hour > 24))
		{
			ret = -1;
		} else {
			sprintf(pcmd, "%s%02d%02d%02x", pcmd, pstr->Str.pGetAlarmClock->Minute, \
			pstr->Str.pGetAlarmClock->Hour, (unsigned char)0x7f);
		}
	}
	
	return ret;
}

int WIFIAudio_PostAlarmClock_AddEveryWeekAlarmClockToCmd(char *pcmd, WARTI_pStr pstr)
{
	int ret = 0;
	
	if((NULL == pcmd) || (NULL == pstr))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if((pstr->Str.pGetAlarmClock->Minute < 0) || (pstr->Str.pGetAlarmClock->Minute > 60) \
		|| (pstr->Str.pGetAlarmClock->Hour < 0) || (pstr->Str.pGetAlarmClock->Hour > 24) \
		|| (pstr->Str.pGetAlarmClock->WeekDay < 0) || (pstr->Str.pGetAlarmClock->WeekDay > 6))
		{
			ret = -1;
		} else {
			sprintf(pcmd, "%s%02d%02d%02x", pcmd, pstr->Str.pGetAlarmClock->Minute, \
			pstr->Str.pGetAlarmClock->Hour, (unsigned char)((0x01)<<(pstr->Str.pGetAlarmClock->WeekDay)));
		}
	}
	
	return ret;
}

int WIFIAudio_PostAlarmClock_AddEveryWeekExAlarmClockToCmd(char *pcmd, WARTI_pStr pstr)
{
	int ret = 0;
	
	if((NULL == pcmd) || (NULL == pstr))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if((pstr->Str.pGetAlarmClock->Minute < 0) || (pstr->Str.pGetAlarmClock->Minute > 60) \
		|| (pstr->Str.pGetAlarmClock->Hour < 0) || (pstr->Str.pGetAlarmClock->Hour > 24) \
		|| (pstr->Str.pGetAlarmClock->WeekDay < 0) || (pstr->Str.pGetAlarmClock->WeekDay > (unsigned char)0x7f))
		{
			ret = -1;
		} else {
			sprintf(pcmd, "%s%02d%02d%02x", pcmd, pstr->Str.pGetAlarmClock->Minute, \
			pstr->Str.pGetAlarmClock->Hour, (unsigned char)pstr->Str.pGetAlarmClock->WeekDay);
		}
	}
	
	return ret;
}

int WIFIAudio_PostAlarmClock_AddAlarmClockTimeInfoToCmd(char *pcmd, WARTI_pStr pstr)
{
	int ret = 0;
	
	if((NULL == pcmd) || (NULL == pstr))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pstr->Str.pGetAlarmClock->Trigger)
		{
			case 0:
				//取消,删除闹钟
				//这边进来不应该为取消
				ret = -1;
				break;
				
			case 1:
				//指定日期
				ret = -1;//暂时没有指定日期的格式
				break;
				
			case 2:
				//每天
				ret = WIFIAudio_PostAlarmClock_AddEveryDayAlarmClockToCmd(pcmd, pstr);
				break;
				
			case 3:
				//每星期
				ret = WIFIAudio_PostAlarmClock_AddEveryWeekAlarmClockToCmd(pcmd, pstr);
				break;
				
			case 4:
				//每星期扩展
				ret = WIFIAudio_PostAlarmClock_AddEveryWeekExAlarmClockToCmd(pcmd, pstr);
				break;
				
			case 5:
				//每个月几号
				ret = -1;//暂时没有指定每个月几号的格式
				break;
				
			default:
				
				break;
		}
	}
	
	return ret;
}

int WIFIAudio_PostAlarmClock_AddAlarmClockPlayUrlToCmd(char *pcmd, WARTI_pStr pstr)
{
	int ret = 0;
	
	if((NULL == pcmd) || (NULL == pstr))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL == pstr->Str.pGetAlarmClock->pMusic)
		{
			ret = -1;
		} else {
			if(NULL == pstr->Str.pGetAlarmClock->pMusic->pContentURL)
			{
				ret = -1;
			} else {
				sprintf(pcmd, "%s \"%s\"", pcmd, pstr->Str.pGetAlarmClock->pMusic->pContentURL);
			}
		}
	}
	
	return ret;
}

char * WIFIAudio_PostAlarmClock_GetAlarmClockCmd(WARTI_pStr pstr)
{
	char *pcmd = NULL;
	WARTI_pStr pstrlast = NULL;
	char cmd[1024];
	int ret = 0;
	
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pcmd = NULL;
	} else {
		memset(cmd, 0, sizeof(cmd)/sizeof(char));
		
		ret = WIFIAudio_PostAlarmClock_AddAlarmClockNumToCmd(cmd, pstr);
		
		if(0 == ret)
		{
			ret = WIFIAudio_PostAlarmClock_AddAlarmClockEnableToCmd(cmd, pstr);
			
			if(0 == ret)
			{
				ret = WIFIAudio_PostAlarmClock_AddAlarmClockTimeInfoToCmd(cmd, pstr);
				
				if(0 == ret)
				{
					ret = WIFIAudio_PostAlarmClock_AddAlarmClockPlayUrlToCmd(cmd, pstr);
				}
			}
		}
		
		if(0 == ret)
		{
			pcmd = strdup(cmd);
		}
	}
	
	return pcmd;
}

int WIFIAudio_PostAlarmClock_SetAlarmClock(WARTI_pStr pstr)
{
	int ret = 0;
	char *pcmd = NULL;
	char cmd[1024];
	FILE *fp = NULL;
	WARTI_pStr pstrlast = NULL;
	
	if(NULL == pstr)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		memset(cmd, 0, sizeof(cmd)/sizeof(char));
		
		if(0 == pstr->Str.pGetAlarmClock->Trigger)
		{
			//删除闹钟
			if((pstr->Str.pGetAlarmClock->N < 0) || (pstr->Str.pGetAlarmClock->N >1))
			{
				ret = -1;
			} else {
				sprintf(cmd, "/usr/script/settimetask.sh deletealarm %d &", pstr->Str.pGetAlarmClock->N);
			}
		} else {
			//设置闹钟
			pcmd = WIFIAudio_PostAlarmClock_GetAlarmClockCmd(pstr);
		
			if(NULL != pcmd)
			{
				sprintf(cmd, "/usr/script/settimetask.sh setalarm %s &", pcmd);
			}
		}
		
		if(NULL != pcmd)
		{
			free(pcmd);
			pcmd = NULL;
		}
				
		if(strlen(cmd) > 0)
		{
			fp = popen(cmd, "r");
			if(NULL == fp)
			{
				ret = -1;
			} else {
				pclose(fp);
				fp = NULL;	//不关心执行之后显示什么数据
			}
		} else {
			ret = -1;
		}
	}
	
	return ret;
}

int main(void)
{
	int len = 0;
	int ret = 0;
	int keynum = 0;
	int i = 0;
	char *lenstr = NULL;
	char *poststr = NULL;
	FILE *fp = NULL;
	char filename[128];
	char *pjsonstring = NULL;
	WARTI_pStr pstr = NULL;
	int sem_id = 0;//信号量ID
	
#ifdef WIFIAUDIO_DEBUG
	printf("Content-Type:text/html\n\n");
#endif

	lenstr = getenv("CONTENT_LENGTH");
	if (NULL == lenstr)
	{
		WIFIAudio_RetJson_retJSON("FAIL");
	} else {
		len = atoi(lenstr);
		
		poststr = (char *)calloc(1, len + 1);
	
		if(NULL == poststr)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
		} else {
			ret = fread(poststr, sizeof(char), len, stdin);
			if(ret < len)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
			} else {
				ret = 0;
				//这边先解析出闹钟信息，几号闹钟
				pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_GETALARMCLOCK, poststr);
				if(NULL == pstr)
				{
					WIFIAudio_RetJson_retJSON("FAIL");
				} else {
					keynum = pstr->Str.pGetAlarmClock->N;
					
					ret = WIFIAudio_PostAlarmClock_SetAlarmClock(pstr);//处理闹钟
					
					if(ret == 0)
					{
						WIFIAudio_RetJson_retJSON("OK");
					} else {
						WIFIAudio_RetJson_retJSON("FAIL");
					}
					
					WIFIAudio_RealTimeInterface_pStrFree(&pstr);
					pstr = NULL;
				}
			}
			free(poststr);
			poststr = NULL;
		}
	}

	fflush(stdout);

	return 0;
}

