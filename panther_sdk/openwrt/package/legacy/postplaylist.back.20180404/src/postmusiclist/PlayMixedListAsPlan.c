#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/stat.h>

#include "filepathnamesuffix.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_RealTimeInterface.h"
#include <json/json.h>

extern char * WIFIAudio_RealTimeInterface_pMixedContentTopString(WARTI_pMixedContent p);

int main(void)
{
	int len = 0;
	int ret = 0;
	FILE *fp = NULL;
	char *lenstr = NULL;
	char *poststr = NULL;
	char *liststr = NULL;
	WARTI_pStr pstr = NULL;
	char tmp[128];
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
				pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_PLAYMIXEDLISTASPLAN, poststr);
				
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
		if(pstr->Str.pPlanMixed->pContent == NULL)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
		} else {
			pstr->Str.pPlanMixed->pContent->NowPlay = 0;
			
			liststr = WIFIAudio_RealTimeInterface_pMixedContentTopString(pstr->Str.pPlanMixed->pContent);
			if(liststr == NULL)
			{
				WIFIAudio_RetJson_retJSON("FAIL");
			} else {
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, WIFIAUDIO_TMPPLAYMIXEDLISTASPLAN_PATHNAME, pstr->Str.pPlanMixed->Num);
				
				if((fp = fopen(tmp, "w+")) == NULL)
				{
					WIFIAudio_RetJson_retJSON("FAIL");
				} else {
					ret = fwrite(liststr, sizeof(char), strrchr(liststr, '}') - liststr + 1, fp);
					
					fclose(fp);
					fp = NULL;
				}
				free(liststr);
				liststr = NULL;
				
				if(ret > 0)
				{
					memset(cmd, 0, sizeof(cmd)/sizeof(char));
					sprintf(cmd,"alarm mixedmusic %d %d %02d %02d %02x \"%s\"", \
					pstr->Str.pAlarmClock->Num, \
					pstr->Str.pAlarmClock->Enable, \
					pstr->Str.pAlarmClock->Hour, \
					pstr->Str.pAlarmClock->Minute, \
					pstr->Str.pAlarmClock->Week, \
					tmp);
					
					fp = popen(cmd, "r");
					if(NULL == fp)
					{
						WIFIAudio_RetJson_retJSON("FAIL");
					} else {
						pclose(fp);
						fp = NULL;
						WIFIAudio_RetJson_retJSON("OK");
					}
				}
			}
		}
				
		WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		pstr = NULL;
	}

	fflush(stdout);

	return 0;
}

