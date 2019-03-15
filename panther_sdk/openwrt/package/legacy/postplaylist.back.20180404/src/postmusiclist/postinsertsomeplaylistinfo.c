#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <json/json.h> 

#include "filepathnamesuffix.h"

#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_FifoCommunication.h"

#include <sys/time.h>//为了测试时间加的头文件


int WIFIAudio_PostInsertList_PostInsertList(void)
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
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_INSERTSOMEMUSICLIST;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
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
	FILE *fp = NULL;
	
#ifdef WIFIAUDIO_DEBUG
	printf("Content-Type:text/html\n\n");
#endif
	
	lenstr = getenv("CONTENT_LENGTH");
	if (NULL == lenstr)
	{
		WIFIAudio_RetJson_retJSON("FAIL");
	} else {
		len = atoi(lenstr);
		
		if(NULL == (fp = fopen(WIFIAUDIO_POSTINSERTLIST_PATHNAME, "w+")))
		{
			WIFIAudio_RetJson_retJSON("FAIL");
		} else {
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
					ret = fwrite(poststr, sizeof(char), len, fp);
					
					if(ret < len)
					{
						WIFIAudio_RetJson_retJSON("FAIL");
					}
				}
				free(poststr);
				poststr = NULL;
			}
			fclose(fp);
			fp = NULL;
			
			if(!(ret < len))
			{
				ret = WIFIAudio_PostInsertList_PostInsertList();
				if(0 == ret)
				{
					WIFIAudio_RetJson_retJSON("OK");
				} else {
					WIFIAudio_RetJson_retJSON("FAIL");
				}
			}
		}
	}

	fflush(stdout);

	return 0;
}

