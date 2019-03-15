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
				pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_INSERTPLAYLISTINFO, poststr);
				
				if(NULL == pstr)
				{
					WIFIAudio_RetJson_retJSON("FAIL");
				}
			}
			
			free(poststr);
			poststr = NULL;
		}
	}
	
	//上面已经解析出JSON信息，插入歌单，修改歌曲信息
	if(NULL != pstr)
	{
		memset(cmd, 0, sizeof(cmd)/sizeof(char));
		sprintf(cmd,"creatPlayList insertonesong \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
		((NULL == pstr->Str.pInsertPlaylistInfo->pMusic->pArtist)? \
		"null": pstr->Str.pInsertPlaylistInfo->pMusic->pArtist), \
		((NULL == pstr->Str.pInsertPlaylistInfo->pMusic->pTitle)? \
		"null": pstr->Str.pInsertPlaylistInfo->pMusic->pTitle), \
		((NULL == pstr->Str.pInsertPlaylistInfo->pMusic->pAlbum)? \
		"null": pstr->Str.pInsertPlaylistInfo->pMusic->pAlbum), \
		((NULL == pstr->Str.pInsertPlaylistInfo->pMusic->pContentURL)? \
		"null": pstr->Str.pInsertPlaylistInfo->pMusic->pContentURL), \
		((NULL == pstr->Str.pInsertPlaylistInfo->pMusic->pCoverURL)? \
		"null": pstr->Str.pInsertPlaylistInfo->pMusic->pCoverURL));
		//关于音乐平台、栏目ID、节目ID脚本还没有提供插入，暂时不改

		fp = popen(cmd, "r");
		if(fp == NULL)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
		} else {
			pclose(fp);
			fp = NULL;
			WIFIAudio_RetJson_retJSON("OK");
		}
		
		WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		pstr = NULL;
	}

	fflush(stdout);

	return 0;
}

