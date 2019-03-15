#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <json/json.h> 

#include "filepathnamesuffix.h"

#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_FifoCommunication.h"
#include "WIFIAudio_RealTimeInterface.h"


int WIFIAudio_RemoteBluetoothUpdate_SendURL(char *purl)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;
	
	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);
	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REMOTEBLUETOOTHUPDATE;
				pcmdparm->Parameter.pUpdate = (WAFC_pUpdate)calloc(1, sizeof(WAFC_Update));
				if(NULL != pcmdparm->Parameter.pUpdate)
				{
					if((NULL == purl) || (!strcmp(purl, "")))
					{
						//URL为空，传一个空格下去，这样对脚本也不会有影响
						pcmdparm->Parameter.pUpdate->pURL = strdup(" ");
					}
					else
					{
						pcmdparm->Parameter.pUpdate->pURL = strdup(purl);
					}
					
					WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
					
					//写入长度小于等于0
					if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
					{
						ret = -1;
					}
				}
				else
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			}
			else
			{
				ret = -1;
			}
			
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		}
		else
		{
			ret = -1;
		}
		
		close(fdfifo);//将管道关闭
		fdfifo = 0;
	}
	else
	{
		ret = -1;
	}
	
	return ret;
}

int main(void)
{
	int len = 0;
	int ret = 0;
	char *lenstr = NULL;
	char *poststr = NULL;
	WARTI_pStr pstr = NULL;
	
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
				pstr = WIFIAudio_RealTimeInterface_newStr(WARTI_STR_TYPE_REMOTEBLUETOOTHUPDATE, poststr);
				
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
		WIFIAudio_RemoteBluetoothUpdate_SendURL(pstr->Str.pRemoteBluetoothUpdate->pURL);
		WIFIAudio_RetJson_retJSON("OK");
		
		WIFIAudio_RealTimeInterface_pStrFree(&pstr);
		pstr = NULL;
	}

	fflush(stdout);

	return 0;
}

