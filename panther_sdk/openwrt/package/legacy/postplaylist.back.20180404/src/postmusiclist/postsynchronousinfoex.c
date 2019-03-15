#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

#include "filepathnamesuffix.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_FifoCommunication.h"
#include "WIFIAudio_Semaphore.h"

int WIFIAudio_PostSynchronousInfo_StartSynchronizePlayEx()
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
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STATRSYNCHRONIZEPLAYEX;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
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
		
		close(fdfifo);
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
	int sem_id = 0;
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
		
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_POSTSYNCHRONOUSINFOEX_SEMKEY, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
		
		if(NULL == (fp = fopen(WIFIAUDIO_POSTSYNCHRONOUSINFOEX_FILENAME, "w+")))
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
#ifdef WIFIAUDIO_DEBUG
					syslog(LOG_INFO, "%s - %d - %s: %s\n", __FILE__, __LINE__, __FUNCTION__, poststr);
#endif
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
		}
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
		
		if(!(ret < len))
		{
			ret = WIFIAudio_PostSynchronousInfo_StartSynchronizePlayEx();
			if(0 == ret)
			{
				WIFIAudio_RetJson_retJSON("OK");
			} else {
				WIFIAudio_RetJson_retJSON("FAIL");
			}
		}
	}
	fflush(stdout);

	return 0;
}

