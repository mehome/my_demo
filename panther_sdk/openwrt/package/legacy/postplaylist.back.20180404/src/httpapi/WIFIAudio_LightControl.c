#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WIFIAudio_LightControl.h"

//移除开始标志之前的数据，可能是之前残留下来的
int WIFIAudio_LightControl_RemoveBeforeStartFlag(WFLCU_pUartBuff pUartBuff)
{
	int ret = 0;
	WFLC_pBuff pStart = NULL;
	int RemoveLen = 0;
	
	if(NULL == pUartBuff)
	{
		DEBUG("Error of Parameter");
		ret = -1;
	} 
	else 
	{
		//当前长度为0就不用那么麻烦了，毕竟这个函数可能为空的时候也会被调用到
		if(pUartBuff->CurrentLen > 0)
		{
			//查找开始标志的位置
			pStart = memchr(pUartBuff->pBuff, WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG, pUartBuff->CurrentLen);
			
			if((pStart >= (pUartBuff->pBuff)) && (pStart < (pUartBuff->pBuff + pUartBuff->CurrentLen)))
			{
				//计算，即将被覆盖移除的长度
				RemoveLen = pStart - pUartBuff->pBuff;
				//计算剩余内容的长度
				pUartBuff->CurrentLen = pUartBuff->CurrentLen - RemoveLen;
				//将内容往前移动
				memmove(pUartBuff->pBuff, pStart, pUartBuff->CurrentLen);
				//移动之后，将之前无用的数据清零
				memset(pUartBuff->pBuff + pUartBuff->CurrentLen, 0, RemoveLen);
			}
			else
			{
				//返回0了，则在已知长度下，没有下一个开始标志，但是却有东西，全部丢弃掉
				memset(pUartBuff->pBuff, 0, pUartBuff->CurrentLen);
				pUartBuff->CurrentLen = 0;
			}
		}
	}
	
	return ret;
}

//根据开始和结束标志来判断串口buff当中是否拥有一个完整的命令，含有完整指令返回1
int WIFIAudio_LightControl_IsHaveCompleteCommandBuff(WFLCU_pUartBuff pUartBuff)
{
	int ret = 0;
	int BaseLen = 0;
	WFLC_pBuff pStart = NULL;
	WFLC_pBuff pEnd = NULL;
	
	if(NULL == pUartBuff)
	{
		DEBUG("Error of Parameter");
		ret = 0;
	}
	else
	{
		//最基本的长度 = 开始标志 + 命令类型 + 数据长度 + 校验和 + 结束标志
		BaseLen = WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG_SIZE \
		+ WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_TYPE_SIZE \
		+ WIFIAUDIO_LIGHTCONTROLFORMAT_DATE_LENGTH_SIZE \
		+ WIFIAUDIO_LIGHTCONTROLFORMAT_CHECK_SUM_SIZE \
		+ WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG_SIZE;
		
		if(BaseLen >= pUartBuff->CurrentLen)
		{
			//收到的长度还不足一个命令最基本的长度
			ret = 0;
		}
		else
		{
			//查找开始标志的位置
			pStart = memchr(pUartBuff->pBuff, WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG, pUartBuff->CurrentLen);
			if((pUartBuff->pBuff > pStart) || (pStart > (pUartBuff->pBuff + pUartBuff->CurrentLen)))
			{
				//找不到开始标志或者找到的标志反而在寻找的buff之前，再或者找到的标志
				ret = 0;
			}
			else
			{
				//查找结束标志的位置
				pEnd = memchr(pUartBuff->pBuff, WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG, pUartBuff->CurrentLen);
				
				//结束标志跑到开始标志前面去了
				if((NULL != pEnd) && (pEnd < pStart))
				{
					WIFIAudio_LightControl_RemoveBeforeStartFlag(pUartBuff);
				}
				else
				{
					if((pEnd > pStart) && (pEnd < (pUartBuff->pBuff + pUartBuff->CurrentLen)))
					{
						//这边判定为含有一个完整的指令，至于取出来对不对，要再做判断
						ret = 1;
					}
				}
			}
		}
	}
	return ret;
}

//根据开始标志和结束标志来计算一个完整命令buff的长度
int WIFIAudio_LightControl_GetCommandBuffLength(WFLC_pBuff pBuff)
{
	int Len = 0;
	WFLC_pBuff pStart = NULL;
	WFLC_pBuff pEnd = NULL;
	
	if(NULL == pBuff)
	{
		DEBUG("Error of Parameter");
		Len = -1;
	}
	else
	{
		pStart = memchr(pBuff, WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG, WIFIAUDIO_LIGHTCONTROLUART_UATRBUFFSIZE);
		pEnd = memchr(pBuff, WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG, WIFIAUDIO_LIGHTCONTROLUART_UATRBUFFSIZE);
		if((NULL != pStart) && (NULL != pEnd) && (pStart < pEnd))
		{
			Len = pEnd - pStart + 1;
		}
	}
	
	return Len;
}

//根据WIFIAudio_LightControlFormat当中定义的起始标志和结束标志
//来获取一个完整指令的buff
WFLC_pBuff WIFIAudio_LightControl_GetCompleteCommandBuff(WFLCU_pUartBuff pUartBuff)
{
	int ret = 0;
	WFLC_pBuff pBuff = NULL;
	WFLC_pBuff pStart = NULL;
	WFLC_pBuff pEnd = NULL;
	int Len = 0;
	
	if(NULL == pUartBuff)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		//判定是否拥有一个开始标志和结束标志
		ret = WIFIAudio_LightControl_IsHaveCompleteCommandBuff(pUartBuff);
		if(1 == ret)
		{
			//移除开始标志之前的全部数据
			WIFIAudio_LightControl_RemoveBeforeStartFlag(pUartBuff);
			
			pStart = pUartBuff->pBuff;
			pEnd = memchr(pUartBuff->pBuff, WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG, pUartBuff->CurrentLen);
			
			Len = pEnd - pStart + 1;
			
			pBuff = (WFLC_pBuff)calloc(Len, sizeof(WFLC_Byte));
			if(NULL != pBuff)
			{
				memcpy(pBuff, pStart, Len);
				
				pUartBuff->CurrentLen = pUartBuff->CurrentLen - Len;
				
				memmove(pStart, pEnd + 1, pUartBuff->CurrentLen);
				
				memset(pUartBuff->pBuff + pUartBuff->CurrentLen, 0, Len);
			}
		}
	}
	
	return pBuff;
}


//释放一整段buff
int WIFIAudio_LightControl_FreeppBuff(WFLC_pBuff *ppBuff)
{
	int Ret = 0;
	
	if(NULL == ppBuff)
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		free(*ppBuff);
		*ppBuff = NULL;
	}
	
	return Ret;
}






