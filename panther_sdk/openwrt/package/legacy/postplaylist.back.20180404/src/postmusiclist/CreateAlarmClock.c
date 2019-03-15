#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/stat.h>
#include <json/json.h>

#include "filepathnamesuffix.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_RealTimeInterface.h"

int main(void)
{
	int len = 0;
	int ret = 0;
	FILE *fp = NULL;
	char *lenstr = NULL;
	char *poststr = NULL;
	WARTI_pStr pstr = NULL;
	char cmd[2048];
	
#ifdef WIFIAUDIO_DEBUG
	printf("Content-Type:text/html\n\n");
#endif
	//接受数据
	lenstr = getenv("CONTENT_LENGTH");
	if (lenstr == NULL)
	{
		WIFIAudio_RetJson_retJSON("FAIL");
	} else {
		len = atoi(lenstr);
		
		poststr = (char *)calloc(1, len + 1);

		if(poststr == NULL)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
		} else {
			ret = fread(poststr, sizeof(char), len, stdin);
			if(ret < len)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
			} else {
				pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_CREATEALARMCLOCK, poststr);
				
				if(pstr == NULL)
				{
					WIFIAudio_RetJson_retJSON("FAIL");
				}
			}
			free(poststr);
			poststr = NULL;
		}
	}

	//上面已经解析出JSON信息，插入歌单，修改歌曲信息
	if(pstr != NULL)
	{
		memset(cmd, 0, sizeof(cmd)/sizeof(char));
		sprintf(cmd,"alarm create %d %d %02d %02d %02x \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" %d %d", \
		pstr->Str.pAlarmClock->Num, \
		pstr->Str.pAlarmClock->Enable, \
		pstr->Str.pAlarmClock->Hour, \
		pstr->Str.pAlarmClock->Minute, \
		pstr->Str.pAlarmClock->Week, \
		((NULL == pstr->Str.pAlarmClock->pContent->pContentURL)? \
		"null": pstr->Str.pAlarmClock->pContent->pContentURL), \
		((NULL == pstr->Str.pAlarmClock->pContent->pTitle)? \
		"null": pstr->Str.pAlarmClock->pContent->pTitle), \
		((NULL == pstr->Str.pAlarmClock->pContent->pArtist)? \
		"null": pstr->Str.pAlarmClock->pContent->pArtist), \
		((NULL == pstr->Str.pAlarmClock->pContent->pAlbum)? \
		"null": pstr->Str.pAlarmClock->pContent->pAlbum), \
		((NULL == pstr->Str.pAlarmClock->pContent->pCoverURL)? \
		"null": pstr->Str.pAlarmClock->pContent->pCoverURL), \
		((NULL == pstr->Str.pAlarmClock->pContent->pPlatform)? \
		"null": pstr->Str.pAlarmClock->pContent->pPlatform), \
		pstr->Str.pAlarmClock->pContent->Column, \
		pstr->Str.pAlarmClock->pContent->Program);
		//关于音乐平台、栏目ID、节目ID脚本还没有提供插入，暂时不改

		fp = popen(cmd, "r");
		if(fp == NULL)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
		} else {
			pclose(fp);
			fp = NULL;//不关心执行之后显示什么数据
			WIFIAudio_RetJson_retJSON("OK");
		}
		
		WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		pstr = NULL;
	}

	fflush(stdout);

	return 0;
}

