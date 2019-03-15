#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "filepathnamesuffix.h"
#include "WIFIAudio_Semaphore.h"
#include "WIFIAudio_Debug.h"

#include "WIFIAudio_LightControl.h"
#include "WIFIAudio_LightControlFormat.h"
#include "WIFIAudio_LightControlUart.h"


WFLCU_pUartBuff WIFIAudio_LightControl_LedMatrixScreen_LoadFileTopUartBuff(char *pFilePathName)
{
	WFLCU_pUartBuff pUartBuff = NULL;
	struct stat FileStat;
	FILE *fp = NULL;
	WFLC_pBuff pBuff = NULL;
	
	if(NULL == pFilePathName)
	{
		
	}
	else
	{
		pUartBuff = WIFIAudio_LightControlUart_NewUartBuff(WIFIAUDIO_LIGHTCONTROLUART_UATRBUFFSIZE);
		if(NULL != pUartBuff)
		{
			stat(pFilePathName, &FileStat);
			if(NULL == (fp = fopen(pFilePathName, "r")))
			{
				WIFIAudio_LightControlUart_FreeppUartBuff(&pUartBuff);
			}
			else
			{
				pBuff = (WFLC_pBuff)calloc(FileStat.st_size, sizeof(WFLC_Byte));
				if(NULL == pBuff)
				{
					WIFIAudio_LightControlUart_FreeppUartBuff(&pUartBuff);
				}
				else
				{
					fread(pBuff, sizeof(WFLC_Byte), FileStat.st_size, fp);
					
					pUartBuff->CurrentLen = WIFIAudio_LightControl_GetCommandBuffLength(pBuff);
					memcpy(pUartBuff->pBuff, pBuff, pUartBuff->CurrentLen);
					
					WIFIAudio_LightControl_FreeppBuff(&pBuff);
				}
				
				fclose(fp);
				fp = NULL;
			}
		}
	}
	
	return pUartBuff;
}


int WIFIAudio_LightControl_LedMatrixScreen_Fixed(void)
{
	int ret = 0;
	int fd = 0;
	int fifoFd = 0;
	WFLCU_pUartBuff pUartBuff = NULL;
	int sem_id = 0;//信号量ID
	
	fd = WIFIAudio_LightControlUart_OpenUart(WIFIAUDIO_LIGHTCONTROLUART_WRITESERIAL, 115200, 8, 1, 'N');
	if(fd > 0)
	{
		pUartBuff = WIFIAudio_LightControl_LedMatrixScreen_LoadFileTopUartBuff(\
		WIFIAUDIO_LEDMATRIXSCREENDATA_PATHNAME);
		
		if(NULL != pUartBuff)
		{
			if(0 == access(WIFIAUDIO_LEDFIFO_PATHNAME, F_OK))
			{
				
			}
			else
			{
				mkfifo(WIFIAUDIO_LEDFIFO_PATHNAME, 0666);
			}
			
			fifoFd = open(WIFIAUDIO_LEDFIFO_PATHNAME, O_WRONLY);
			
			if(fifoFd > 0)
			{
				sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_LEDMATRIXSCREENDATAWRITE_SEMKEY, 1);
				WIFIAudio_Semaphore_Operation_P(sem_id);
				if(write(fifoFd, pUartBuff->pBuff, pUartBuff->CurrentLen) > 0)
				{
					
				}
				else
				{
					ret = -1;
				}
				WIFIAudio_Semaphore_Operation_V(sem_id);
				close(fifoFd);
				fifoFd = -1;
			}
			/*
			sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_LEDMATRIXSCREENDATAWRITE_SEMKEY, 1);
			WIFIAudio_Semaphore_Operation_P(sem_id);
			
			WIFIAudio_LightControlUart_WriteUart(fd, pUartBuff);
			
			WIFIAudio_Semaphore_Operation_V(sem_id);
			*/
		}
		
		close(fd);
		fd = 0;
	}
	return ret;
}

//动屏个数，切换时间间隔
int WIFIAudio_LightControl_LedMatrixScreen_Animation(int Num, int Interval)
{
	int ret = 0;
	int fd = 0;
	int fifoFd = 0;
	int i = 0;
	char FileName[128];
	WFLCU_pUartBuff pUartBuffArray[Num];
	WFLCU_pUartBuff pUartBuffTemp = NULL;
	int sem_id = 0;//信号量ID
	
	fd = WIFIAudio_LightControlUart_OpenUart(WIFIAUDIO_LIGHTCONTROLUART_WRITESERIAL, 115200, 8, 1, 'N');
	if(fd > 0)
	{
		//先将文件中的内容统一读取出来
		//再进行循环，省得每次发送都得读取文件
		memset(pUartBuffArray, 0, Num * sizeof(WFLCU_pUartBuff));
		for(i = 0; i < Num; i++)
		{
			sprintf(FileName, "%s%d", WIFIAUDIO_LEDMATRIXSCREENDATA_PATHNAME, i);
			pUartBuffArray[i] = WIFIAudio_LightControl_LedMatrixScreen_LoadFileTopUartBuff(FileName);
		}
		
		Interval *= 1000;
		
		pUartBuffTemp = WIFIAudio_LightControlUart_NewUartBuff(WIFIAUDIO_LIGHTCONTROLUART_UATRBUFFSIZE);
		if(NULL != pUartBuffTemp)
		{
			if(0 == access(WIFIAUDIO_LEDFIFO_PATHNAME, F_OK))
			{
				
			}
			else
			{
				mkfifo(WIFIAUDIO_LEDFIFO_PATHNAME, 0666);
			}
			
			fifoFd = open(WIFIAUDIO_LEDFIFO_PATHNAME, O_WRONLY);
			
			if(fifoFd > 0)
			{
				while(1)
				{
					for(i = 0; i < Num; i++)
					{
						//这边如果直接使用pUartBuffArray[i]进行写入操作的话
						//写函数会将WFLCU_pUartBuff中的buff取出
						//并将占用的字节数删除
						//这边先将内容拷贝到临时发送的串口buff当中
						pUartBuffTemp->CurrentLen = pUartBuffArray[i]->CurrentLen;
						memcpy(pUartBuffTemp->pBuff, pUartBuffArray[i]->pBuff, pUartBuffTemp->CurrentLen);
						
						sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_LEDMATRIXSCREENDATAWRITE_SEMKEY, 1);
						WIFIAudio_Semaphore_Operation_P(sem_id);
						if(write(fifoFd, pUartBuffTemp->pBuff, pUartBuffTemp->CurrentLen) > 0)
						{
							
						}
						else
						{
							ret = -1;
						}
						WIFIAudio_Semaphore_Operation_V(sem_id);
						
						/*
						sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)WIFIAUDIO_LEDMATRIXSCREENDATAWRITE_SEMKEY, 1);
						WIFIAudio_Semaphore_Operation_P(sem_id);
						
						WIFIAudio_LightControlUart_WriteUart(fd, pUartBuffTemp);
						
						WIFIAudio_Semaphore_Operation_V(sem_id);
						*/
						usleep(Interval);//等待时间间隔别在上锁的时候，这是大忌，上锁了在里面睡大觉
					}
				}
				WIFIAudio_LightControlUart_FreeppUartBuff(&pUartBuffTemp);
				for(i = 0; i < Num; i++)
				{
					WIFIAudio_LightControlUart_FreeppUartBuff(&(pUartBuffArray[i]));
				}
				close(fifoFd);
				fifoFd = -1;
			}
		}
		
		close(fd);
		fd = 0;
	}
	return ret;
}

int main(int argc, char **argv)
{
	//动屏标志，几个屏，时间间隔，这边参数个数是要算上程序名的
	if(4 != argc)
	{
		return 0;
	}
	else
	{
		if(0 == atoi(argv[1]))
		{
			//单屏
			WIFIAudio_LightControl_LedMatrixScreen_Fixed();
		}
		else if(1 == atoi(argv[1]))
		{
			//多屏
			WIFIAudio_LightControl_LedMatrixScreen_Animation(atoi(argv[2]), atoi(argv[3]));
		}
	}
	
	return 0;
}

