#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/time.h>
#include <unistd.h>

#include <sys/time.h>

#ifdef WIFIAUDIO_DEBUG
#include <syslog.h>
#endif

#include "WIFIAudio_FifoCommunication.h"


//将前n个字节删除，后面len个字节长度的buff向前移动，含有当前长度维护
int WIFIAudio_FifoCommunication_MoveBuff(WAFC_pFifoBuff pfifobuff, int start, int len)
{
	int ret = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(len > pfifobuff->CurrentLen)
		{
			len = pfifobuff->CurrentLen;
		}
		memmove(pfifobuff->pBuff, pfifobuff->pBuff + start, len);
		memset(&((pfifobuff->pBuff)[len]), 0, start);
		
		pfifobuff->CurrentLen = len;
	}
	
	return ret;
}

//将前n个字节删除，后面len个字节长度的buff向前移动，含有当前长度维护
int WIFIAudio_FifoCommunication_ClearBuff(WAFC_pFifoBuff pfifobuff, int start, int len)
{
	int ret = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(len > pfifobuff->CurrentLen)
		{
			len = pfifobuff->CurrentLen;
		}
		memset(&((pfifobuff->pBuff)[start]), 0, len);
		
		pfifobuff->CurrentLen -= len;
	}
	
	return ret;
}

//调整buff，根据0xAA，0xBB
int WIFIAudio_FifoCommunication_AdjustReadBuff(WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	int start = 0;
	int len = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		while(1)
		{
			if((WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG1 != (unsigned char)(pfifobuff->pBuff)[start]) \
			|| (WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG2 != (unsigned char)(pfifobuff->pBuff)[start + 1]))
			{
				start++;
				if(start > pfifobuff->CurrentLen)
				{
					//没有找到开始标志0xAA和0xBB，将前面一整段删除
					memset(pfifobuff->pBuff, 0, pfifobuff->CurrentLen);
					pfifobuff->CurrentLen = 0;
					break;
				}
			} else {
				len = pfifobuff->CurrentLen - start;
				
				WIFIAudio_FifoCommunication_MoveBuff(pfifobuff, start, len);
				
				break;
			}
		}
		
	}
	
	return ret;
}

//释放FifoBuff
int WIFIAudio_FifoCommunication_FreeFifoBuff(WAFC_pFifoBuff *ppfifobuff)
{
	int ret = 0;
	
	if ((NULL == ppfifobuff) || (NULL == (*ppfifobuff)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else  {
		if(NULL != (*ppfifobuff)->pBuff)
		{
			free((*ppfifobuff)->pBuff);
			(*ppfifobuff)->pBuff = NULL;
		}
		
		free(*ppfifobuff);
		*ppfifobuff = NULL;
	}
	
	return ret;
}

//创建一个空的FifoBuff结构体
WAFC_pFifoBuff WIFIAudio_FifoCommunication_NewBlankFifoBuff(int bufflen)
{
	WAFC_pFifoBuff pfifobuff = NULL;
	
	pfifobuff = (WAFC_pFifoBuff)calloc(1, sizeof(WAFC_FifoBuff));
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pfifobuff = NULL;
	} else {
		pfifobuff->BuffLen = bufflen;
		pfifobuff->CurrentLen = 0;
		pfifobuff->pBuff = (char *)calloc(1, bufflen * sizeof(char));
		if(NULL == pfifobuff->pBuff)
		{
			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		}
	}
	
	return pfifobuff;
}

//轮询从管道当中读取一段数据
int WIFIAudio_FifoCommunication_ReadFifoToFifoBuff(int fdfifo, WAFC_pFifoBuff pfifobuff, int readlen)
{
	int remainderlen = 0;
	int actuallyreadlen = 0;
	fd_set readset;//轮询的可读集合
	struct timeval timeout;
	
	if((fdfifo < 0) || (NULL == pfifobuff) || (readlen < 0))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		actuallyreadlen = -1;
	} else {
		remainderlen = pfifobuff->BuffLen - pfifobuff->CurrentLen;
		if(readlen > remainderlen)//读取的长度大于剩余的长度，进行边界处理
		{
			readlen = remainderlen;
		}
		
		//这边会阻塞，这边已经可以不用select了，不过既然写了就放在这吧
		FD_ZERO(&readset);
		FD_SET(fdfifo, &readset);
		timeout.tv_sec = 1;//阻塞5秒
		timeout.tv_usec = 0;
		//这边是select阻塞开始
		select(fdfifo + 1, &readset, NULL, NULL, &timeout);//这边不关心写，错误，等待1秒
		//这边是select阻塞结束
		if(FD_ISSET(fdfifo, &readset))
		{
			//select返回的时候，会将没有等待到的时间对应位清零
			//所以这个时候只要来判断一下一开始加入的文件描述符现在还是否为置位状态就可以知道是否有等待的时间发生
			actuallyreadlen = read(fdfifo, (pfifobuff->pBuff) + pfifobuff->CurrentLen, readlen);
			pfifobuff->CurrentLen += actuallyreadlen;
		}
	}
	
	return actuallyreadlen;
}

//写一段数据到管道当中
int WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(int fdfifo, WAFC_pFifoBuff pfifobuff, int writelen)
{
	int actuallywritelen = 0;
	
	if((fdfifo < 0) || (NULL == pfifobuff) || (writelen < 0))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		actuallywritelen = -1;
	} else {
		if(writelen > pfifobuff->CurrentLen)//写的长度大于剩余的长度，进行边界处理
		{
			writelen = pfifobuff->CurrentLen;
		}
		
		actuallywritelen = write(fdfifo, pfifobuff->pBuff, writelen);
		
		pfifobuff->CurrentLen -= actuallywritelen;
	}
	
	return actuallywritelen;
}

//创建并打开一个只读的管道
int WIFIAudio_FifoCommunication_CreatFifo(char *ppath)
{
	int ret = 0;
	
	if(NULL == ppath)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		//这边进来的时候先创建管道
		unlink(ppath);
		mkfifo(ppath, 0666);
	}
	
	return ret;
}

//打开一个可读可写的管道
int WIFIAudio_FifoCommunication_OpenReadAndWriteFifo(char *ppath)
{
	int fdfifo = -1;
	
	if(NULL == ppath)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		fdfifo = -1;
	} else {
		fdfifo = open(ppath, O_RDWR);//会在这边阻塞住
	}
	
	return fdfifo;
}

//打开一个只写的管道
int WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(char *ppath)
{
	int fdfifo = -1;
	
	if(NULL == ppath)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		fdfifo = -1;
	} else {
		fdfifo = open(ppath, O_WRONLY);
	}
	
	return fdfifo;
}

int WIFIAudio_FifoCommunication_FreeConnectAp(WAFC_pConnectAp *ppconnectap)
{
	int ret = 0;
	
	if ((NULL == ppconnectap) || (NULL == (*ppconnectap)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppconnectap)->pSSID)
		{
			free((*ppconnectap)->pSSID);
			(*ppconnectap)->pSSID = NULL;
		}
		
		if(NULL != (*ppconnectap)->pPassword)
		{
			free((*ppconnectap)->pPassword);
			(*ppconnectap)->pPassword = NULL;
		}
		
		free(*ppconnectap);
		*ppconnectap = NULL;
	}
	
	return ret;
}


int WIFIAudio_FifoCommunication_FreePlayIndex(WAFC_pPlayIndex *ppplayindex)
{
	int ret = 0;
	
	if ((NULL == ppplayindex) || (NULL == (*ppplayindex)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		free(*ppplayindex);
		*ppplayindex = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeSetSSID(WAFC_psetSSID *ppsetssid)
{
	int ret = 0;
	
	if ((NULL == ppsetssid) || (NULL == (*ppsetssid)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppsetssid)->pSSID)
		{
			free((*ppsetssid)->pSSID);
			(*ppsetssid)->pSSID = NULL;
		}
		
		free(*ppsetssid);
		*ppsetssid = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeSetNetwork(WAFC_psetNetwork *ppsetnetwork)
{
	int ret = 0;
	
	if ((NULL == ppsetnetwork) || (NULL == (*ppsetnetwork)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppsetnetwork)->pPassword)
		{
			free((*ppsetnetwork)->pPassword);
			(*ppsetnetwork)->pPassword = NULL;
		}
		
		free(*ppsetnetwork);
		*ppsetnetwork = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeName(WAFC_pName *ppname)
{
	int ret = 0;
	
	if ((NULL == ppname) || (NULL == (*ppname)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppname)->pName)
		{
			free((*ppname)->pName);
			(*ppname)->pName = NULL;
		}
		
		free(*ppname);
		*ppname = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeUpdate(WAFC_pUpdate *ppupdate)
{
	int ret = 0;
	
	if ((NULL == ppupdate) || (NULL == (*ppupdate)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppupdate)->pURL)
		{
			free((*ppupdate)->pURL);
			(*ppupdate)->pURL = NULL;
		}
		
		free(*ppupdate);
		*ppupdate = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeHtmlUpdate(WAFC_pHtmlUpdate *pphtmlupdate)
{
	int ret = 0;
	
	if ((NULL == pphtmlupdate) || (NULL == (*pphtmlupdate)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*pphtmlupdate)->pPath)
		{
			free((*pphtmlupdate)->pPath);
			(*pphtmlupdate)->pPath = NULL;
		}
		
		free(*pphtmlupdate);
		*pphtmlupdate = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeSetRecord(WAFC_pSetRecord *ppsetrecord)
{
	int ret = 0;
	
	if ((NULL == ppsetrecord) || (NULL == (*ppsetrecord)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		free(*ppsetrecord);
		*ppsetrecord = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreePlayTfCard(WAFC_pPlayTfCard *ppplaytfcard)
{
	int ret = 0;
	
	if ((NULL == ppplaytfcard) || (NULL == (*ppplaytfcard)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		free(*ppplaytfcard);
		*ppplaytfcard = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreePlayUsbDisk(WAFC_pPlayUsbDisk *ppplayusbdisk)
{
	int ret = 0;
	
	if ((NULL == ppplayusbdisk) || (NULL == (*ppplayusbdisk)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		free(*ppplayusbdisk);
		*ppplayusbdisk = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeShutdownSec(WAFC_pShutdownSec *ppshutdownsec)
{
	int ret = 0;
	
	if ((NULL == ppshutdownsec) || (NULL == (*ppshutdownsec)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		free(*ppshutdownsec);
		*ppshutdownsec = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreePlayUrlRecord(WAFC_pPlayUrlRecord *ppplayurlrecord)
{
	int ret = 0;
	
	if ((NULL == ppplayurlrecord) || (NULL == (*ppplayurlrecord)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppplayurlrecord)->pUrl)
		{
			free((*ppplayurlrecord)->pUrl);
			(*ppplayurlrecord)->pUrl = NULL;
		}
		
		free(*ppplayurlrecord);
		*ppplayurlrecord = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeVoiceActivityDetection(WAFC_pVoiceActivityDetection *ppvoiceactivitydetection)
{
	int ret = 0;
	
	if ((NULL == ppvoiceactivitydetection) || (NULL == (*ppvoiceactivitydetection)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppvoiceactivitydetection)->pCardName)
		{
			free((*ppvoiceactivitydetection)->pCardName);
			(*ppvoiceactivitydetection)->pCardName = NULL;
		}
		
		free(*ppvoiceactivitydetection);
		*ppvoiceactivitydetection = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeSetMultiVolume(WAFC_pSetMultiVolume *ppsetmultivolume)
{
	int ret = 0;
	
	if ((NULL == ppsetmultivolume) || (NULL == (*ppsetmultivolume)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppsetmultivolume)->pMac)
		{
			free((*ppsetmultivolume)->pMac);
			(*ppsetmultivolume)->pMac = NULL;
		}
		
		free(*ppsetmultivolume);
		*ppsetmultivolume = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeConnectBluetooth(WAFC_pConnectBluetooth *ppconnectbluetooth)
{
	int ret = 0;
	
	if ((NULL == ppconnectbluetooth) || (NULL == (*ppconnectbluetooth)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		if(NULL != (*ppconnectbluetooth)->pAddress)
		{
			free((*ppconnectbluetooth)->pAddress);
			(*ppconnectbluetooth)->pAddress = NULL;
		}
		
		free(*ppconnectbluetooth);
		*ppconnectbluetooth = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeLedMatrixScreen(WAFC_pLedMatrixScreen *ppledmatrixscreen)
{
	int ret = 0;
	
	if ((NULL == ppledmatrixscreen) || (NULL == (*ppledmatrixscreen)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		free(*ppledmatrixscreen);
		*ppledmatrixscreen = NULL;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FreeCmdParm(WAFC_pCmdParm *ppcmdparm)
{
	int ret = 0;
	
	if ((NULL == ppcmdparm) || (NULL == (*ppcmdparm)))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch((*ppcmdparm)->Command)
		{
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTAP:
				if(NULL != (*ppcmdparm)->Parameter.pConnectAp)
				{
					WIFIAudio_FifoCommunication_FreeConnectAp(&((*ppcmdparm)->Parameter.pConnectAp));
					(*ppcmdparm)->Parameter.pConnectAp = NULL;
				}
				break;
			
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYINDEX:
				if(NULL != (*ppcmdparm)->Parameter.pPlayIndex)
				{
					WIFIAudio_FifoCommunication_FreePlayIndex(&((*ppcmdparm)->Parameter.pPlayIndex));
					(*ppcmdparm)->Parameter.pPlayIndex = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETSSID:
				if(NULL != (*ppcmdparm)->Parameter.pSetSSID)
				{
					WIFIAudio_FifoCommunication_FreeSetSSID(&((*ppcmdparm)->Parameter.pSetSSID));
					(*ppcmdparm)->Parameter.pSetSSID = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETNETWORK:
				if(NULL != (*ppcmdparm)->Parameter.pSetNetwork)
				{
					WIFIAudio_FifoCommunication_FreeSetNetwork(&((*ppcmdparm)->Parameter.pSetNetwork));
					(*ppcmdparm)->Parameter.pSetNetwork = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETDEVICENAME:
				if(NULL != (*ppcmdparm)->Parameter.pName)
				{
					WIFIAudio_FifoCommunication_FreeName(&((*ppcmdparm)->Parameter.pName));
					(*ppcmdparm)->Parameter.pName = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_GETMVREMOTEUPDATE:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REMOTEBLUETOOTHUPDATE:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPGRADEWIFIANDBLUETOOTH:
				if(NULL != (*ppcmdparm)->Parameter.pUpdate)
				{
					WIFIAudio_FifoCommunication_FreeUpdate(&((*ppcmdparm)->Parameter.pUpdate));
					(*ppcmdparm)->Parameter.pUpdate = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_HTMLUPDATE:
				if(NULL != (*ppcmdparm)->Parameter.pHtmlUpdate)
				{
					WIFIAudio_FifoCommunication_FreeHtmlUpdate(&((*ppcmdparm)->Parameter.pHtmlUpdate));
					(*ppcmdparm)->Parameter.pHtmlUpdate = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETRECORD:
				if(NULL != (*ppcmdparm)->Parameter.pSetRecord)
				{
					WIFIAudio_FifoCommunication_FreeSetRecord(&((*ppcmdparm)->Parameter.pSetRecord));
					(*ppcmdparm)->Parameter.pSetRecord = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYTFCARD:
				if(NULL != (*ppcmdparm)->Parameter.pPlayTfCard)
				{
					WIFIAudio_FifoCommunication_FreePlayTfCard(&((*ppcmdparm)->Parameter.pPlayTfCard));
					(*ppcmdparm)->Parameter.pPlayTfCard = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYUSBDISK:
				if(NULL != (*ppcmdparm)->Parameter.pPlayUsbDisk)
				{
					WIFIAudio_FifoCommunication_FreePlayUsbDisk(&((*ppcmdparm)->Parameter.pPlayUsbDisk));
					(*ppcmdparm)->Parameter.pPlayUsbDisk = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SHUTDOWN:
				if(NULL != (*ppcmdparm)->Parameter.pShutdownSec)
				{
					WIFIAudio_FifoCommunication_FreeShutdownSec(&((*ppcmdparm)->Parameter.pShutdownSec));
					(*ppcmdparm)->Parameter.pShutdownSec = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYURLRECORD:
				if(NULL != (*ppcmdparm)->Parameter.pPlayUrlRecord)
				{
					WIFIAudio_FifoCommunication_FreePlayUrlRecord(&((*ppcmdparm)->Parameter.pPlayUrlRecord));
					(*ppcmdparm)->Parameter.pPlayUrlRecord = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_VOICEACTIVITYDETECTION:
				if(NULL != (*ppcmdparm)->Parameter.pVoiceActivityDetection)
				{
					WIFIAudio_FifoCommunication_FreeVoiceActivityDetection(&((*ppcmdparm)->Parameter.pVoiceActivityDetection));
					(*ppcmdparm)->Parameter.pVoiceActivityDetection = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETMULTIVOLUME:
				if(NULL != (*ppcmdparm)->Parameter.pSetMultiVolume)
				{
					WIFIAudio_FifoCommunication_FreeSetMultiVolume(&((*ppcmdparm)->Parameter.pSetMultiVolume));
					(*ppcmdparm)->Parameter.pSetMultiVolume = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTBLUETOOTH:
				if(NULL != (*ppcmdparm)->Parameter.pConnectBluetooth)
				{
					WIFIAudio_FifoCommunication_FreeConnectBluetooth(&((*ppcmdparm)->Parameter.pConnectBluetooth));
					(*ppcmdparm)->Parameter.pConnectBluetooth = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LEDMATRIXSCREEN:
				if(NULL != (*ppcmdparm)->Parameter.pLedMatrixScreen)
				{
					WIFIAudio_FifoCommunication_FreeLedMatrixScreen(&((*ppcmdparm)->Parameter.pLedMatrixScreen));
					(*ppcmdparm)->Parameter.pLedMatrixScreen = NULL;
				}
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_NORMALRECORD:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MICRECORD:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REFRECORD:
				if(NULL != (*ppcmdparm)->Parameter.pSetRecord)
				{
					WIFIAudio_FifoCommunication_FreeSetRecord(&((*ppcmdparm)->Parameter.pSetRecord));
					(*ppcmdparm)->Parameter.pSetRecord = NULL;
				}
				break;
				
			default:
				break;
		}
		
		free(*ppcmdparm);
		*ppcmdparm = NULL;
	}
	
	return ret;
}



int WIFIAudio_FifoCommunication_GetCompleteCommandLength(WAFC_pFifoBuff pfifobuff)
{
	int i = 0;
	int len = 0;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		len = -1;
	} else {
		WIFIAudio_FifoCommunication_AdjustReadBuff(pfifobuff);
		if((WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG1 == (unsigned char)(pfifobuff->pBuff)[0]) \
		&& (WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG2 == (unsigned char)(pfifobuff->pBuff)[1]))
		{
			//前两个字节刚好是开头0xAA,0xBB
			i = 2;
			memset(temp, 0, sizeof(temp));
			while(1)
			{
				if('\0' == (pfifobuff->pBuff)[i])
				{
					len = i;
					break;
				}
				
				if(WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR != (pfifobuff->pBuff)[i])
				{
					temp[i - 2] = (pfifobuff->pBuff)[i];
					i++;
				} else {
					temp[i] = '\0';
					
					len = atoi(temp);
					break;
				}
			}
		} else {
			len = 0;
		}
	}
	
	return len;
}

//判断是否在结束标志出现之前，又出现了一个开始标志，说明这是有问题的，如果出现，将这一段直接移除掉
int WIFIAudio_FifoCommunication_IsErrorCommand(WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	int i = 0;
	int start = 0;
	int end = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		WIFIAudio_FifoCommunication_AdjustReadBuff(pfifobuff);
		if((WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG1 == (unsigned char)(pfifobuff->pBuff)[0]) \
		&& (WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG2 == (unsigned char)(pfifobuff->pBuff)[1]))
		{
			//前两个字节刚好是开头0xAA,0xBB
			i = 2;
			while(1)
			{
				if(i > pfifobuff->CurrentLen)
				{
					//没有找到开始标志0xAA和0xBB或者结束标志0xCC和0xDD，还没有出现明显问题
					break;
				}
				
				if((WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG1 == (unsigned char)(pfifobuff->pBuff)[i]) \
				&& (WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG2 == (unsigned char)(pfifobuff->pBuff)[i + 1]))
				{
					end = i;
				}
				
				if((WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG1 == (unsigned char)(pfifobuff->pBuff)[i]) \
				&& (WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG2 == (unsigned char)(pfifobuff->pBuff)[i + 1]))
				{
					start = i;
				}
				
				if((0 != start) && (0 == end))
				{
					//在找到结束标志之前又出现了一个结束标志
					ret = 1;
					(pfifobuff->pBuff)[0] = 0x31;//把第一个开头的开始标志直接破坏掉
					WIFIAudio_FifoCommunication_AdjustReadBuff(pfifobuff);
					break;
				} else if((0 == start) && (0 != end)) {
					//先找到结束标志
					ret = 0;
					break;
				}
				
				i++;
			}
		}
	}
	
	return ret;
}

//这边此时有可能读到下一条指令的内容，所以这边要在做处理
int WIFIAudio_FifoCommunication_IsCompleteCommand(WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	int len = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		WIFIAudio_FifoCommunication_IsErrorCommand(pfifobuff);//如果出现明显错误，调整一下
		
		len = WIFIAudio_FifoCommunication_GetCompleteCommandLength(pfifobuff);//printf("Len: %d\r\n", len);
		if(len > (2 + 5 + 2))//最少的长度，0xAA，0xBB，0xCC，0xDD，长度又固定为5个字节
		{
			if((WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG1 == (unsigned char)(pfifobuff->pBuff)[len - 2]) \
			&& (WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG2 == (unsigned char)(pfifobuff->pBuff)[len - 1]))
			{
				ret = 1;
			}
		}
	}
	
	return ret;
}

//在使用之前记得把dst置零一下
int WIFIAudio_FifoCommunication_GetOneParameterString(char *dst, WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	int i = 0;
	
	if((NULL == dst) || (NULL == pfifobuff))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		while(1)
		{
			if(WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR != (pfifobuff->pBuff)[i])
			{
				dst[i] = (pfifobuff->pBuff)[i];
				i++;
			} else {
				dst[i] = '\0';
				
				while(WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR == (pfifobuff->pBuff)[i+1])//第一个'\n'之后如果还有'\n'一起去掉
				{
					i++;
				}
				
				WIFIAudio_FifoCommunication_MoveBuff(pfifobuff, i + 1, \
				pfifobuff->CurrentLen - (i + 1));
				
				break;
			}
		}
	}
	
	return ret;
}



WAFC_pConnectAp WIFIAudio_FifoCommunication_GetConnectAp(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pConnectAp pconnectap = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pconnectap = NULL;
	} else {
		pconnectap = (WAFC_pConnectAp)calloc(1, sizeof(WAFC_ConnectAp));
		if(NULL == pconnectap)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pconnectap->pSSID = strdup(temp);
			
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pconnectap->Encode = atoi(temp);
			
			if(0 != pconnectap->Encode)
			{
				memset(temp, 0, sizeof(temp));
				WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
				pconnectap->pPassword = strdup(temp);
			}
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pconnectap;
}

WAFC_pPlayIndex WIFIAudio_FifoCommunication_GetPlayIndex(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pPlayIndex pplayindex = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pplayindex = NULL;
	} else {
		pplayindex = (WAFC_pPlayIndex)calloc(1, sizeof(WAFC_PlayIndex));
		if(NULL == pplayindex)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pplayindex->Index = atoi(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pplayindex;
}

WAFC_psetSSID WIFIAudio_FifoCommunication_GetSetSSID(WAFC_pFifoBuff pfifobuff)
{
	WAFC_psetSSID psetssid = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		psetssid = NULL;
	} else {
		psetssid = (WAFC_psetSSID)calloc(1, sizeof(WAFC_setSSID));
		if(NULL == psetssid)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			psetssid->pSSID = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return psetssid;
}

WAFC_psetNetwork WIFIAudio_FifoCommunication_GetSetNetwork(WAFC_pFifoBuff pfifobuff)
{
	WAFC_psetNetwork psetnetwork = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		psetnetwork = NULL;
	} else {
		psetnetwork = (WAFC_psetNetwork)calloc(1, sizeof(WAFC_setNetwork));
		if(NULL == psetnetwork)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			psetnetwork->Flag = atoi(temp);
			
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			psetnetwork->pPassword = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return psetnetwork;
}

WAFC_pName WIFIAudio_FifoCommunication_GetName(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pName pname = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pname = NULL;
	} else {
		pname = (WAFC_pName)calloc(1, sizeof(WAFC_Name));
		if(NULL == pname)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pname->pName = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pname;
}

WAFC_pUpdate WIFIAudio_FifoCommunication_GetUpdate(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pUpdate pupdate = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pupdate = NULL;
	} else {
		pupdate = (WAFC_pUpdate)calloc(1, sizeof(WAFC_Update));
		if(NULL == pupdate)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pupdate->pURL = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pupdate;
}

WAFC_pHtmlUpdate WIFIAudio_FifoCommunication_GetHtmlUpdate(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pHtmlUpdate phtmlupdate = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		phtmlupdate = NULL;
	} else {
		phtmlupdate = (WAFC_pHtmlUpdate)calloc(1, sizeof(WAFC_HtmlUpdate));
		if(NULL == phtmlupdate)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			phtmlupdate->pPath = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return phtmlupdate;
}

WAFC_pSetRecord WIFIAudio_FifoCommunication_GetSetRecord(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pSetRecord psetrecord = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		psetrecord = NULL;
	} else {
		psetrecord = (WAFC_pSetRecord)calloc(1, sizeof(WAFC_SetRecord));
		if(NULL == psetrecord)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			psetrecord->Value = atoi(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return psetrecord;
}

WAFC_pPlayTfCard WIFIAudio_FifoCommunication_GetPlayTfCard(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pPlayTfCard pplaytfcard = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pplaytfcard = NULL;
	} else {
		pplaytfcard = (WAFC_pPlayTfCard)calloc(1, sizeof(WAFC_PlayTfCard));
		if(NULL == pplaytfcard)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pplaytfcard->Index = atoi(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pplaytfcard;
}

WAFC_pPlayUsbDisk WIFIAudio_FifoCommunication_GetPlayUsbDisk(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pPlayUsbDisk pplayusbdisk = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pplayusbdisk = NULL;
	} else {
		pplayusbdisk = (WAFC_pPlayUsbDisk)calloc(1, sizeof(WAFC_PlayUsbDisk));
		if(NULL == pplayusbdisk)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pplayusbdisk->Index = atoi(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pplayusbdisk;
}

WAFC_pShutdownSec WIFIAudio_FifoCommunication_GetShutdownSec(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pShutdownSec pshutdownsec = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pshutdownsec = NULL;
	} else {
		pshutdownsec = (WAFC_pShutdownSec)calloc(1, sizeof(WAFC_ShutdownSec));
		if(NULL == pshutdownsec)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pshutdownsec->Second = atoi(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pshutdownsec;
}

WAFC_pPlayUrlRecord WIFIAudio_FifoCommunication_PlayUrlRecord(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pPlayUrlRecord pplayurlrecord = NULL;
	char temp[256];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pplayurlrecord = NULL;
	} else {
		pplayurlrecord = (WAFC_pPlayUrlRecord)calloc(1, sizeof(WAFC_PlayUrlRecord));
		if(NULL == pplayurlrecord)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pplayurlrecord->pUrl = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pplayurlrecord;
}

WAFC_pVoiceActivityDetection WIFIAudio_FifoCommunication_VoiceActivityDetection(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pVoiceActivityDetection pvoiceactivitydetection = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pvoiceactivitydetection = NULL;
	} else {
		pvoiceactivitydetection = (WAFC_pVoiceActivityDetection)calloc(1, sizeof(WAFC_VoiceActivityDetection));
		if(NULL == pvoiceactivitydetection)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pvoiceactivitydetection->Value = atoi(temp);
			
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pvoiceactivitydetection->pCardName = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pvoiceactivitydetection;
}

WAFC_pSetMultiVolume WIFIAudio_FifoCommunication_SetMultiVolume(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pSetMultiVolume psetmultivolume = NULL;
	char temp[128];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		psetmultivolume = NULL;
	} else {
		psetmultivolume = (WAFC_pSetMultiVolume)calloc(1, sizeof(WAFC_SetMultiVolume));
		if(NULL == psetmultivolume)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			psetmultivolume->Value = atoi(temp);
			
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			psetmultivolume->pMac = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return psetmultivolume;
}

WAFC_pConnectBluetooth WIFIAudio_FifoCommunication_ConnectBluetooth(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pConnectBluetooth pconnectbluetooth = NULL;
	char temp[32];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pconnectbluetooth = NULL;
	} else {
		pconnectbluetooth = (WAFC_pConnectBluetooth)calloc(1, sizeof(WAFC_ConnectBluetooth));
		if(NULL == pconnectbluetooth)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pconnectbluetooth->pAddress = strdup(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pconnectbluetooth;
}

WAFC_pLedMatrixScreen WIFIAudio_FifoCommunication_LedMatrixScreen(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pLedMatrixScreen pledmatrixscreen = NULL;
	char temp[32];
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pledmatrixscreen = NULL;
	} else {
		pledmatrixscreen = (WAFC_pLedMatrixScreen)calloc(1, sizeof(WAFC_LedMatrixScreen));
		if(NULL == pledmatrixscreen)
		{
#ifdef WIFIAUDIO_DEBUG
			syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pledmatrixscreen->Animation = atoi(temp);
			
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pledmatrixscreen->Num = atoi(temp);
			
			memset(temp, 0, sizeof(temp));
			WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
			pledmatrixscreen->Interval = atoi(temp);
			
			WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
		}
	}
	
	return pledmatrixscreen;
}

WAFC_pCmdParm WIFIAudio_FifoCommunication_FifoBuffToCmdParm(WAFC_pFifoBuff pfifobuff)
{
	WAFC_pCmdParm pcmdparm = NULL;
	char temp[128];
	int len = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		pcmdparm = NULL;
	} else {
		if(1 == WIFIAudio_FifoCommunication_IsCompleteCommand(pfifobuff))
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL == pcmdparm)
			{
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
			} else {
				//去掉开头0xAA，0xBB
				WIFIAudio_FifoCommunication_MoveBuff(pfifobuff, 2, pfifobuff->CurrentLen - 2);
				
				//获取长度
				memset(temp, 0, sizeof(temp));
				WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
				len = atoi(temp);
				
				len -= 2;
				len -= strlen(temp) - 1;
				
				//获取指令类型
				memset(temp, 0, sizeof(temp));
				WIFIAudio_FifoCommunication_GetOneParameterString(temp, pfifobuff);
				pcmdparm->Command = atoi(temp);
				
				len -= strlen(temp) - 1;
				
				switch(pcmdparm->Command)
				{
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTAP:
						pcmdparm->Parameter.pConnectAp = WIFIAudio_FifoCommunication_GetConnectAp(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYINDEX:
						pcmdparm->Parameter.pPlayIndex = WIFIAudio_FifoCommunication_GetPlayIndex(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETSSID:
						pcmdparm->Parameter.pSetSSID = WIFIAudio_FifoCommunication_GetSetSSID(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETNETWORK:
						pcmdparm->Parameter.pSetNetwork = WIFIAudio_FifoCommunication_GetSetNetwork(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_RESTORETODEFAULT:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						//虽然这边不带参，不过还是要做分支判断，否则在default就会被当做错误的命令释放掉
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REBOOT:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						//虽然这边不带参，不过还是要做分支判断，否则在default就会被当做错误的命令释放掉
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETDEVICENAME:
						pcmdparm->Parameter.pName = WIFIAudio_FifoCommunication_GetName(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_GETMVREMOTEUPDATE:
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REMOTEBLUETOOTHUPDATE:
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPGRADEWIFIANDBLUETOOTH:
						pcmdparm->Parameter.pUpdate = WIFIAudio_FifoCommunication_GetUpdate(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_HTMLUPDATE:
						pcmdparm->Parameter.pHtmlUpdate = WIFIAudio_FifoCommunication_GetHtmlUpdate(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALY:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALYEX:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_INSERTSOMEMUSICLIST:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LOGINAMAZON:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETRECORD:
						pcmdparm->Parameter.pSetRecord = WIFIAudio_FifoCommunication_GetSetRecord(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYRECORD:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYTFCARD:
						pcmdparm->Parameter.pPlayTfCard = WIFIAudio_FifoCommunication_GetPlayTfCard(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYUSBDISK:
						pcmdparm->Parameter.pPlayUsbDisk = WIFIAudio_FifoCommunication_GetPlayUsbDisk(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STATRSYNCHRONIZEPLAYEX:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						//虽然这边不带参，不过还是要做分支判断，否则在default就会被当做错误的命令释放掉
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SHUTDOWN:
						pcmdparm->Parameter.pShutdownSec = WIFIAudio_FifoCommunication_GetShutdownSec(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYURLRECORD:
						pcmdparm->Parameter.pPlayUrlRecord = WIFIAudio_FifoCommunication_PlayUrlRecord(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STARTRECORD:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_FIXEDRECORD:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_VOICEACTIVITYDETECTION:
						pcmdparm->Parameter.pVoiceActivityDetection = WIFIAudio_FifoCommunication_VoiceActivityDetection(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETMULTIVOLUME:
						pcmdparm->Parameter.pSetMultiVolume = WIFIAudio_FifoCommunication_SetMultiVolume(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SEARCHBLUETOOTH:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTBLUETOOTH:
						pcmdparm->Parameter.pConnectBluetooth = WIFIAudio_FifoCommunication_ConnectBluetooth(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATEBLUETOOTH:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LEDMATRIXSCREEN:
						pcmdparm->Parameter.pLedMatrixScreen = WIFIAudio_FifoCommunication_LedMatrixScreen(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATACONEXANT:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_NORMALRECORD:
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MICRECORD:
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REFRECORD:
						pcmdparm->Parameter.pSetRecord = WIFIAudio_FifoCommunication_GetSetRecord(pfifobuff);
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STOPMODERECORD:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_DISCONNECTWIFI:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CREATESHORTCUTKEYLIST:
						WIFIAudio_FifoCommunication_ClearBuff(pfifobuff, 0, 2);//跳过0xCC，0xDD结尾
						break;
						
					default:
#ifdef WIFIAUDIO_DEBUG
						syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
						WIFIAudio_FifoCommunication_MoveBuff(pfifobuff, len, \
						pfifobuff->CurrentLen - len);
						WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
						pcmdparm = NULL;
						break;
				}
			}
		}
	}
	
	return pcmdparm;
}

int WIFIAudio_FifoCommunication_GetCompleteCommand(int fdfifo, WAFC_pFifoBuff pfifobuff, int readlen)
{
	int ret = 0;
	fd_set readset;//轮询的可读集合
	struct timeval timeout;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		while(1)
		{
			if(1 == WIFIAudio_FifoCommunication_IsCompleteCommand(pfifobuff))
			{
				//获取到一个完整的指令长度buff就退出
				break;
			} else {
				FD_ZERO(&readset);
				FD_SET(fdfifo, &readset);
				timeout.tv_sec = 1;//阻塞1秒
				timeout.tv_usec = 0;
				select(fdfifo + 1, &readset, NULL, NULL, &timeout);//这边不关心写，错误，等待时间1秒
				if(FD_ISSET(fdfifo, &readset))
				{
					WIFIAudio_FifoCommunication_ReadFifoToFifoBuff(fdfifo, pfifobuff, readlen);
				} else {
					ret = -1;
					break;//不是一个完整的指令，等待1秒读取，如果1秒之后没数据就直接退出了
				}
			}
		}
	}
	
	return ret;
}


int WIFIAudio_FifoCommunication_RemoveStartFlag(WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		WIFIAudio_FifoCommunication_AdjustReadBuff(pfifobuff);
		WIFIAudio_FifoCommunication_MoveBuff(pfifobuff, 0, 2);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_AddStartFlag(WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		(pfifobuff->pBuff)[pfifobuff->CurrentLen] = WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG1;
		(pfifobuff->CurrentLen)++;
		(pfifobuff->pBuff)[pfifobuff->CurrentLen] = WIFIAUDIO_FIFOCOMMUNICATION_STARTFLAG2;
		(pfifobuff->CurrentLen)++;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_RemoveEndFlag(WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		WIFIAudio_FifoCommunication_AdjustReadBuff(pfifobuff);//这边直接将下一个指令的开始挪到最开始
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_AddEndFlag(WAFC_pFifoBuff pfifobuff)
{
	int ret = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		(pfifobuff->pBuff)[pfifobuff->CurrentLen] = WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG1;
		(pfifobuff->CurrentLen)++;
		(pfifobuff->pBuff)[pfifobuff->CurrentLen] = WIFIAUDIO_FIFOCOMMUNICATION_ENDFLAG2;
		(pfifobuff->CurrentLen)++;
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_PutOneParameterString(WAFC_pFifoBuff pfifobuff, char *src)
{
	int ret = 0;
	int i = 0;
	
	if((NULL == src) || (NULL == pfifobuff))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		while(1)
		{
			if('\0' != src[i])
			{
				(pfifobuff->pBuff)[pfifobuff->CurrentLen] = src[i];
				(pfifobuff->CurrentLen)++;
				i++;
			} else {
				(pfifobuff->pBuff)[pfifobuff->CurrentLen] = WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR;
				(pfifobuff->CurrentLen)++;
				break;
			}
		}
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_PutLengthString(WAFC_pFifoBuff pfifobuff, int putlen, int length)
{
	int ret = 0;
	char temp[128];
	int currentlen = 0;
	
	if(NULL == pfifobuff)
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		currentlen = pfifobuff->CurrentLen;//记录下当前长度
		pfifobuff->CurrentLen = putlen;//将当前长度暂时赋值为插入长度的位置
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%c%c%c%c", \
		WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR, \
		WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR, \
		WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR, \
		WIFIAUDIO_FIFOCOMMUNICATION_SEPARATOR);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);//先填充4个字节作为len
		
		pfifobuff->CurrentLen = putlen;//将当前长度暂时赋值为插入长度的位置
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", length);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);//先填充4个字节作为len
		
		pfifobuff->CurrentLen = currentlen;//插入长度之后，将当前长度还原
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_ConnectApToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pConnectAp->pSSID);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pConnectAp->Encode);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		if(0 != pcmdparm->Parameter.pConnectAp->Encode)
		{
			WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pConnectAp->pPassword);
		}
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_PlayIndexToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pPlayIndex->Index);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_SetSSIDToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pSetSSID->pSSID);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_SetNetworkToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pSetNetwork->Flag);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pSetNetwork->pPassword);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_RestoreDefaultToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_RebootToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_NameToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pName->pName);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_UpdateToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pUpdate->pURL);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_HtmlUpdateToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pHtmlUpdate->pPath);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_MusicListPlay(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_MusicListPlayEx(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_InsertSomeMusicList(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_LoginAmazon(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_SetRecord(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pSetRecord->Value);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_PlayRecord(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_PlayTfCard(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pPlayTfCard->Index);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_PlayUsbDisk(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pPlayUsbDisk->Index);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_ShutdownSec(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pShutdownSec->Second);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_StartSynchronizePlayExToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_PlayUrlRecordToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pPlayUrlRecord->pUrl);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_StartRecordToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_FixedRecordToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_VoiceActivityDetectionToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pVoiceActivityDetection->Value);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pVoiceActivityDetection->pCardName);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_SetMultiVolumeToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pSetMultiVolume->Value);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pSetMultiVolume->pMac);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_SearchBluetoothToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_ConnectBluetoothToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, pcmdparm->Parameter.pConnectBluetooth->pAddress);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_UpdateBluetoothToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_LedMatrixScreenToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pLedMatrixScreen->Animation);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pLedMatrixScreen->Num);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Parameter.pLedMatrixScreen->Interval);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_UpdataConexantToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_StopModeRecordToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_DisconnectWiFiToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_CreateShortcutKeyListToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int putlen = 0;
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		len = pfifobuff->CurrentLen;//先记录下起始位置,待会方便写入完整计算指令的长度
		
		WIFIAudio_FifoCommunication_AddStartFlag(pfifobuff);
		
		putlen = pfifobuff->CurrentLen;//记录下写长度的位置
		
		pfifobuff->CurrentLen = pfifobuff->CurrentLen + 5;//先跳过5个字节，这边是用来填充Len的
		
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "%d", pcmdparm->Command);
		WIFIAudio_FifoCommunication_PutOneParameterString(pfifobuff, temp);
		
		WIFIAudio_FifoCommunication_AddEndFlag(pfifobuff);
	
		len = pfifobuff->CurrentLen - len;//计算出总长度
		
		WIFIAudio_FifoCommunication_PutLengthString(pfifobuff, putlen, len);
	}
	
	return ret;
}

int WIFIAudio_FifoCommunication_CmdParmToFifoBuff(WAFC_pFifoBuff pfifobuff, WAFC_pCmdParm pcmdparm)
{
	int ret = 0;
	char temp[128];
	int len = 0;
	
	if((NULL == pfifobuff) || (NULL == pcmdparm))
	{
#ifdef WIFIAUDIO_DEBUG
		syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
		ret = -1;
	} else {
		switch(pcmdparm->Command)
		{
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTAP:
				WIFIAudio_FifoCommunication_ConnectApToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYINDEX:
				WIFIAudio_FifoCommunication_PlayIndexToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETSSID:
				WIFIAudio_FifoCommunication_SetSSIDToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETNETWORK:
				WIFIAudio_FifoCommunication_SetNetworkToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_RESTORETODEFAULT:
				WIFIAudio_FifoCommunication_RestoreDefaultToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REBOOT:
				WIFIAudio_FifoCommunication_RebootToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETDEVICENAME:
				WIFIAudio_FifoCommunication_NameToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_GETMVREMOTEUPDATE:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REMOTEBLUETOOTHUPDATE:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPGRADEWIFIANDBLUETOOTH:
				WIFIAudio_FifoCommunication_UpdateToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_HTMLUPDATE:
				WIFIAudio_FifoCommunication_HtmlUpdateToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALY:
				WIFIAudio_FifoCommunication_MusicListPlay(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MUSICLISTPALYEX:
				WIFIAudio_FifoCommunication_MusicListPlayEx(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_INSERTSOMEMUSICLIST:
				WIFIAudio_FifoCommunication_InsertSomeMusicList(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LOGINAMAZON:
				WIFIAudio_FifoCommunication_LoginAmazon(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETRECORD:
				WIFIAudio_FifoCommunication_SetRecord(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYRECORD:
				WIFIAudio_FifoCommunication_PlayRecord(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYTFCARD:
				WIFIAudio_FifoCommunication_PlayTfCard(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYUSBDISK:
				WIFIAudio_FifoCommunication_PlayUsbDisk(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STATRSYNCHRONIZEPLAYEX:
				WIFIAudio_FifoCommunication_StartSynchronizePlayExToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SHUTDOWN:
				WIFIAudio_FifoCommunication_ShutdownSec(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_PLAYURLRECORD:
				WIFIAudio_FifoCommunication_PlayUrlRecordToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STARTRECORD:
				WIFIAudio_FifoCommunication_StartRecordToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_FIXEDRECORD:
				WIFIAudio_FifoCommunication_FixedRecordToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_VOICEACTIVITYDETECTION:
				WIFIAudio_FifoCommunication_VoiceActivityDetectionToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SETMULTIVOLUME:
				WIFIAudio_FifoCommunication_SetMultiVolumeToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_SEARCHBLUETOOTH:
				WIFIAudio_FifoCommunication_SearchBluetoothToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CONNECTBLUETOOTH:
				WIFIAudio_FifoCommunication_ConnectBluetoothToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATEBLUETOOTH:
				WIFIAudio_FifoCommunication_UpdateBluetoothToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_LEDMATRIXSCREEN:
				WIFIAudio_FifoCommunication_LedMatrixScreenToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATACONEXANT:
				WIFIAudio_FifoCommunication_UpdataConexantToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_NORMALRECORD:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_MICRECORD:
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_REFRECORD:
				WIFIAudio_FifoCommunication_SetRecord(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_STOPMODERECORD:
				WIFIAudio_FifoCommunication_StopModeRecordToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_DISCONNECTWIFI:
				WIFIAudio_FifoCommunication_DisconnectWiFiToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			case WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_CREATESHORTCUTKEYLIST:
				WIFIAudio_FifoCommunication_CreateShortcutKeyListToFifoBuff(pfifobuff, pcmdparm);
				break;
				
			default:
#ifdef WIFIAUDIO_DEBUG
				syslog(LOG_INFO, "%s - %d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#endif
				break;
		}
	}
	
	return ret;
}

