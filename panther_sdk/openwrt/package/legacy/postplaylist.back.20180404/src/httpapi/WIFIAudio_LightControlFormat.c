#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WIFIAudio_LightControlFormat.h"
#include "WIFIAudio_Debug.h"

//Ascii转化为16进制数值
int WIFIAudio_LightControlFormat_AsciiToHex(WFLCF_pBuff pAscii)
{
	int Hex = 0;
	
	if(NULL == pAscii)
	{
		DEBUG("Error of Parameter");
		Hex = 0;
	} 
	else 
	{
		//这里不要使用sscanf，之前澜起平台可能会出问题
		Hex = strtol(pAscii, NULL, 16);
	}
	
	return Hex;
}

//16进制数值转换字符串，字符串需要释放
WFLCF_pBuff WIFIAudio_LightControlFormat_HexTopAscii(int Size, int Hex)
{
	WFLCF_pBuff pAscii = NULL;
	WFLCF_Byte Tmp[16];
	
	sprintf(Tmp, "%%0%dx", Size);
	
	pAscii = (WFLCF_pBuff)calloc(Size + 1, sizeof(WFLCF_Byte));
	if(NULL == pAscii)
	{
		DEBUG("Error of calloc");
		pAscii = NULL;
	}
	else
	{
		sprintf(pAscii, Tmp, Hex);
	}
	
	return pAscii;
}

//将内存当中的16进制Ascii字符串取出，设置指定的整数
int WIFIAudio_LightControlFormat_CopypAsciiTopHex(int *pHex, int *pCurrent, int Size, WFLCF_pBuff pBuff)
{
	int Ret = 0;
	WFLCF_Byte Tmp[16];
	
	if((NULL == pHex) || (NULL == pCurrent) || (NULL == pBuff))
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		memset(Tmp, 0, sizeof(Tmp));
		memcpy(Tmp, &(pBuff[*pCurrent]), Size);
		*pHex = WIFIAudio_LightControlFormat_AsciiToHex(Tmp);
		*pCurrent += Size;
	}
	
	return Ret;
}

//将一个16进制数据转换成字符串，并且拷贝到指定内存当中
int WIFIAudio_LightControlFormat_CopyHexTopAscii(WFLCF_pBuff pBuff, int *pCurrent, int Size, int Hex)
{
	int Ret = 0;
	WFLCF_pBuff pTmp = NULL;
	
	if((NULL == pBuff) || (NULL == pCurrent))
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		pTmp = WIFIAudio_LightControlFormat_HexTopAscii(Size, Hex);
		if(NULL != pTmp)
		{
			memcpy(&(pBuff[*pCurrent]), pTmp, Size);
			WIFIAudio_LightControlFormat_FreeppBuff(&pTmp);
		}
		*pCurrent += Size;
	}
	
	return Ret;
}

//释放一整段buff
int WIFIAudio_LightControlFormat_FreeppBuff(WFLCF_pBuff *ppBuff)
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

//将一个数据当中的数据全部压缩进buff
WFLCF_pBuff WIFIAudio_LightControlFormat_pCmdDataTopBuff(LedUart_pCmdData pCmdData)
{
	WFLCF_pBuff pBuff = NULL;
	int Current = 0;
	int Length = 0;
	WFLCF_pBuff pTmp = NULL;
	int i = 0;
	int Sum = 0;
	
	if(NULL == pCmdData)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		if(NULL == pCmdData->pDataBuff)
		{
			DEBUG("Error of Parameter");
		}
		else
		{
			//这边分配的一段内存，不是为了放字符串
			//数据中间本身就有可能出现\0字符，无需考虑结束符
			Length = WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_TYPE_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_DATE_LENGTH_SIZE \
			+ pCmdData->Len \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_CHECK_SUM_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG_SIZE;
			
			pBuff = (WFLCF_pBuff)calloc(Length, sizeof(WFLCF_Byte));
			
			if(NULL == pBuff)
			{
				DEBUG("Error of Calloc");
			}
			else
			{
				Current = 0;
				//开始标记
				pBuff[Current] = WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG;
				Current += WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG_SIZE;
				
				//指令类型
				WIFIAudio_LightControlFormat_CopyHexTopAscii(pBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_TYPE_SIZE, \
				pCmdData->Type);
				
				//数据长度
				WIFIAudio_LightControlFormat_CopyHexTopAscii(pBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_DATE_LENGTH_SIZE, \
				pCmdData->Len);
				
				//数据部分
				memcpy(&(pBuff[Current]), pCmdData->pDataBuff, pCmdData->Len);
				Current += pCmdData->Len;
				
				//校验和
				for(i = WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG_SIZE, Sum = 0; i < Current; i++)
				{
					Sum += pBuff[i];
					//为了求单字节和位操作只取低八位，位速度很快，不会影响
					Sum &= 0xff;
				}
				WIFIAudio_LightControlFormat_CopyHexTopAscii(pBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_CHECK_SUM_SIZE, \
				0x100 - Sum);
				
				//结束标记
				pBuff[Current] = WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG;
				Current += WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG_SIZE;
			}
		}
	}
	
	return pBuff;
}

//释放命令数据结构体
int WIFIAudio_LightControlFormat_FreeppCmdData(LedUart_pCmdData *ppCmdData)
{
	int Ret = 0;
	
	if(NULL == ppCmdData)
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		if(NULL != (*ppCmdData)->pDataBuff)
		{
			WIFIAudio_LightControlFormat_FreeppBuff(&((*ppCmdData)->pDataBuff));
		}
		
		free(*ppCmdData);
		*ppCmdData = NULL;
	}
	
	return Ret;
}

//将buff当中的数据，解析到命令数据结构体当中
LedUart_pCmdData WIFIAudio_LightControlFormat_pBuffTopCmdData(WFLCF_pBuff pBuff)
{
	LedUart_pCmdData pCmdData = NULL;
	WFLCF_Byte Tmp[16];
	int Current = 0;
	int i = 0;
	int Sum = 0;
	int Check = 0;
	
	if(NULL == pBuff)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		//这边不考虑buff当中有没有其他东西，或者指令不完全的情况
		//这边只解析，其他情况在别地方处理，这边就当做buff当中已经有一个完整的指令了
		pCmdData = (LedUart_pCmdData)calloc(1, sizeof(LedUart_CmdData));
		if(NULL == pBuff)
		{
			DEBUG("Error of Calloc");
		}
		else
		{
			Current = WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG_SIZE;
			
			//指令类型
			WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
			(int *)(&(pCmdData->Type)), &Current, \
			WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_TYPE_SIZE, 
			pBuff);

			//数据长度
			WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
			&(pCmdData->Len), &Current, \
			WIFIAUDIO_LIGHTCONTROLFORMAT_DATE_LENGTH_SIZE, 
			pBuff);
			
			pCmdData->pDataBuff = (WFLCF_pBuff)calloc(pCmdData->Len, sizeof(WFLCF_Byte));
			if(NULL == pCmdData->pDataBuff)
			{
				WIFIAudio_LightControlFormat_FreeppCmdData(&pCmdData);
			}
			else
			{
				memcpy(pCmdData->pDataBuff, &(pBuff[Current]), pCmdData->Len);
				Current += pCmdData->Len;
			}
			
			//校验和验证
			Sum = 0;
			for(i = WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG_SIZE; i < Current; i++)
			{
				Sum += pBuff[i];
				Sum &= 0xff;
			}
			
			WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
			&Check, &Current, \
			WIFIAUDIO_LIGHTCONTROLFORMAT_CHECK_SUM_SIZE, 
			pBuff);
			
			if(0x100 != Check + Sum)
			{
				//校验和错误，舍弃这个数据
				WIFIAudio_LightControlFormat_FreeppCmdData(&pCmdData);
			}
		}
	}
	
	return pCmdData;
}

LedUart_pCmdData WIFIAudio_LightControlFormat_pDataContentTopCmdData_SingleLight(LedUart_pDataContent pDataContent)
{
	LedUart_pCmdData pCmdData = NULL;
	int Current = 0;
	
	if(NULL == pDataContent)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		pCmdData = (LedUart_pCmdData)calloc(1, sizeof(LedUart_CmdData));
		if(NULL == pCmdData)
		{
			DEBUG("Error of Calloc");
		}
		else
		{
			//命令类型
			pCmdData->Type = pDataContent->Type;
			
			//数据长度
			pCmdData->Len = WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_ENABLE_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BRIGHTNESS_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_RED_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_GREEN_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BLUE_SIZE;
			
			//数据部分
			pCmdData->pDataBuff = (WFLCF_pBuff)calloc(pCmdData->Len, sizeof(WFLCF_Byte));
			if(NULL == pCmdData->pDataBuff)
			{
				WIFIAudio_LightControlFormat_FreeppCmdData(&pCmdData);
			}
			else
			{
				Current = 0;
				
				//使能
				WIFIAudio_LightControlFormat_CopyHexTopAscii(\
				pCmdData->pDataBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_ENABLE_SIZE, \
				pDataContent->Data.pSingleLight->Enable);
				
				//亮度
				WIFIAudio_LightControlFormat_CopyHexTopAscii(\
				pCmdData->pDataBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BRIGHTNESS_SIZE, \
				pDataContent->Data.pSingleLight->Brightness);
				
				//红
				WIFIAudio_LightControlFormat_CopyHexTopAscii(\
				pCmdData->pDataBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_RED_SIZE, \
				pDataContent->Data.pSingleLight->Red);
				
				//绿
				WIFIAudio_LightControlFormat_CopyHexTopAscii(\
				pCmdData->pDataBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_GREEN_SIZE, \
				pDataContent->Data.pSingleLight->Green);
				
				//蓝
				WIFIAudio_LightControlFormat_CopyHexTopAscii(\
				pCmdData->pDataBuff, &Current, \
				WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BLUE_SIZE, \
				pDataContent->Data.pSingleLight->Blue);
			}
		}
	}
		
	return pCmdData;
}

int WIFIAudio_LightControlFormat_pStringDataTopBcdData(WFLCF_pBuff pBuff, int *pCurrent, int Size, char *pData)
{
	int Ret = 0;
	int i = 0;
	WFLCF_Byte Tmp[2];
	WFLCF_Byte Value = 0;
	
	if((NULL == pBuff) || (NULL == pCurrent) || (NULL == pData))
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		//因为每次只取一个值，所以不用每次都重新置0
		memset(Tmp, 0, sizeof(Tmp));
		for(i =0; i < Size; i++)
		{
			Tmp[0] = pData[i];
			Value = WIFIAudio_LightControlFormat_AsciiToHex(Tmp);
			if(0 == (i & 0x01))
			{
				//偶数
				pBuff[*pCurrent] = (Value << 4) & 0xf0;
				//这边这样处理，如果灯珠个数是奇数个，就在低位自动补零了
			}
			else
			{
				//奇数
				pBuff[*pCurrent] |= Value & 0x0f;
				(*pCurrent)++;
			}
		}
		(*pCurrent)++;
	}
	
	return Ret;
}

LedUart_pCmdData WIFIAudio_LightControlFormat_pDataContentTopCmdData_LedMatrixScreen(LedUart_pDataContent pDataContent)
{
	LedUart_pCmdData pCmdData = NULL;
	int Current = 0;
	int Product = 0;
	
	if(NULL == pDataContent)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		pCmdData = (LedUart_pCmdData)calloc(1, sizeof(LedUart_CmdData));
		if(NULL == pCmdData)
		{
			DEBUG("Error of Calloc");
		}
		else
		{
			//命令类型
			pCmdData->Type = pDataContent->Type;
			
			//数据长度
			pCmdData->Len = WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_LENGTH_SIZE \
			+ WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_WIDTH_SIZE;
			//计算灯珠个数
			Product = pDataContent->Data.pLedMatrixScreen->Length \
			* pDataContent->Data.pLedMatrixScreen->Width;
			
			if(1 == (Product & 0x01))
			{
				//灯珠个数为奇数
				pCmdData->Len += (Product >> 1) + 1;
			}
			else
			{
				//灯珠个数为偶数
				pCmdData->Len += (Product >> 1);
			}
			
			//数据部分
			pCmdData->pDataBuff = (WFLCF_pBuff)calloc(pCmdData->Len, sizeof(WFLCF_Byte));
			if(NULL == pCmdData->pDataBuff)
			{
				WIFIAudio_LightControlFormat_FreeppCmdData(&pCmdData);
			}
			else
			{
				if(strlen(pDataContent->Data.pLedMatrixScreen->pData) != Product)
				{
					DEBUG("Error of Size");
					WIFIAudio_LightControlFormat_FreeppCmdData(&pCmdData);
				}
				else
				{
					Current = 0;
					//横向灯珠
					WIFIAudio_LightControlFormat_CopyHexTopAscii(\
					pCmdData->pDataBuff, &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_LENGTH_SIZE, \
					pDataContent->Data.pLedMatrixScreen->Length);
					
					//纵向灯珠
					WIFIAudio_LightControlFormat_CopyHexTopAscii(\
					pCmdData->pDataBuff, &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_WIDTH_SIZE, \
					pDataContent->Data.pLedMatrixScreen->Width);
					
					//灯珠色值数据
					WIFIAudio_LightControlFormat_pStringDataTopBcdData(\
					pCmdData->pDataBuff, &Current, Product, \
					pDataContent->Data.pLedMatrixScreen->pData);
				}
			}
		}
	}
	
	return pCmdData;
}

//将原始数据，一个个按照协议格式压缩到数据结构体当中数据字段中
LedUart_pCmdData WIFIAudio_LightControlFormat_pDataContentTopCmdData(LedUart_pDataContent pDataContent)
{
	LedUart_pCmdData pCmdData = NULL;
	
	if(NULL == pDataContent)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		switch(pDataContent->Type)
		{
			case WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_SINGLELIGHT:
				pCmdData = WIFIAudio_LightControlFormat_pDataContentTopCmdData_SingleLight(pDataContent);
				break;
				
			case WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_LEDMATRIXSCREEN:
				pCmdData = WIFIAudio_LightControlFormat_pDataContentTopCmdData_LedMatrixScreen(pDataContent);
				break;
				
			default:
				break;
		}
	}
	
	return pCmdData;
}

int WIFIAudio_LightControlFormat_FreeppSingleLight(LedUart_pSingleLight *ppSingleLight)
{
	int Ret = 0;
	
	if(NULL == ppSingleLight)
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		free(*ppSingleLight);
		*ppSingleLight = NULL;
	}
	
	return Ret;
}

int WIFIAudio_LightControlFormat_FreeppLedMatrixScreen(LedUart_pLedMatrixScreen *ppLedMatrixScreen)
{
	int Ret = 0;
	
	if(NULL == ppLedMatrixScreen)
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		if(NULL != (*ppLedMatrixScreen)->pData)
		{
			free((*ppLedMatrixScreen)->pData);
			*ppLedMatrixScreen = NULL;
		}
		
		free(*ppLedMatrixScreen);
		*ppLedMatrixScreen = NULL;
	}
	
	return Ret;
}

//释放原始数据结构体
int WIFIAudio_LightControlFormat_FreeppDataContent(LedUart_pDataContent *ppDataContent)
{
	int Ret = 0;
	
	if(NULL == ppDataContent)
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		switch((*ppDataContent)->Type)
		{
			case WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_SINGLELIGHT:
				if(NULL != (*ppDataContent)->Data.pSingleLight)
				{
					WIFIAudio_LightControlFormat_FreeppSingleLight(&((*ppDataContent)->Data.pSingleLight));
				}
				break;
				
			case WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_LEDMATRIXSCREEN:
				if(NULL != (*ppDataContent)->Data.pLedMatrixScreen)
				{
					WIFIAudio_LightControlFormat_FreeppLedMatrixScreen(&((*ppDataContent)->Data.pLedMatrixScreen));
				}
				break;
				
			default:
				break;
		}
		
		free(*ppDataContent);
		*ppDataContent = NULL;
	}
	
	return Ret;
}

LedUart_pDataContent WIFIAudio_LightControlFormat_pCmdDataTopDataContent_SingleLight(LedUart_pCmdData pCmdData)
{
	LedUart_pDataContent pDataContent = NULL;
	int Current = 0;
	
	if(NULL == pCmdData)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		if(NULL == pCmdData->pDataBuff)
		{
			DEBUG("Error of Parameter");
		}
		else
		{
			pDataContent = (LedUart_pDataContent)calloc(1, sizeof(LedUart_DataContent));
			
			if(NULL == pDataContent)
			{
				DEBUG("Error of Calloc");
			}
			else
			{
				pDataContent->Type = pCmdData->Type;
				
				pDataContent->Data.pSingleLight = (LedUart_pSingleLight)calloc(1, sizeof(LedUart_SingleLight));
				if(NULL == pDataContent->Data.pSingleLight)
				{
					DEBUG("Error of Calloc");
					WIFIAudio_LightControlFormat_FreeppDataContent(&pDataContent);
				}
				else
				{
					Current = 0;
					
					//使能
					WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
					&(pDataContent->Data.pSingleLight->Enable), &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_ENABLE_SIZE, \
					pCmdData->pDataBuff);
					
					//亮度
					WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
					&(pDataContent->Data.pSingleLight->Brightness), &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BRIGHTNESS_SIZE, \
					pCmdData->pDataBuff);
					
					//红
					WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
					&(pDataContent->Data.pSingleLight->Red), &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_RED_SIZE, \
					pCmdData->pDataBuff);
					
					//绿
					WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
					&(pDataContent->Data.pSingleLight->Green), &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_GREEN_SIZE, \
					pCmdData->pDataBuff);
					
					//蓝
					WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
					&(pDataContent->Data.pSingleLight->Blue), &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BLUE_SIZE, \
					pCmdData->pDataBuff);
					
				}
			}
		}
	}
	
	return pDataContent;
}

int WIFIAudio_LightControlFormat_pBcdDataTopStringData(char **ppData, int *pCurrent, int Size, WFLCF_pBuff pBuff)
{
	int Ret = 0;
	int i = 0;
	int Len = 0;
	WFLCF_Byte High = 0;
	WFLCF_Byte Low = 0;
	char Tmp[3];
	
	if((NULL == pCurrent) || (NULL == pBuff))
	{
		DEBUG("Error of Parameter");
		Ret = -1;
	}
	else
	{
		*ppData = (char *)calloc(Size + 1, sizeof(char));
		if(NULL == *ppData)
		{
			DEBUG("Error of Parameter");
			Ret = -1;
		}
		else
		{
			//保险点，先置零
			memset(*ppData, 0, Size + 1);
			
			//先当做是偶数个灯珠
			Len = Size >> 1;
			
			for(i = 0; i < Len; i++)
			{
				High = (pBuff[*pCurrent] & 0xf0) >> 4;
				Low = pBuff[*pCurrent] & 0x0f;
				sprintf(Tmp, "%d%d", High, Low);
				strcat(*ppData, Tmp);
				(*pCurrent)++;
			}
			
			//做奇数个灯珠的特殊处理
			if(1 == (Size & 0x01))
			{
				High = (pBuff[*pCurrent] & 0xf0) >> 4;
				sprintf(Tmp, "%d", High);
				strcat(*ppData, Tmp);
				(*pCurrent)++;
			}
		}
	}
	
	return Ret;
}

LedUart_pDataContent WIFIAudio_LightControlFormat_pCmdDataTopDataContent_LedMatrixScreen(LedUart_pCmdData pCmdData)
{
	LedUart_pDataContent pDataContent = NULL;
	int Current = 0;
	int Product = 0;
	
	if(NULL == pCmdData)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		if(NULL == pCmdData->pDataBuff)
		{
			DEBUG("Error of Parameter");
		}
		else
		{
			pDataContent = (LedUart_pDataContent)calloc(1, sizeof(LedUart_DataContent));
			
			if(NULL == pDataContent)
			{
				DEBUG("Error of Calloc");
			}
			else
			{
				pDataContent->Type = pCmdData->Type;
				
				pDataContent->Data.pLedMatrixScreen = (LedUart_pLedMatrixScreen)calloc(1, sizeof(LedUart_LedMatrixScreen));
				if(NULL == pDataContent->Data.pLedMatrixScreen)
				{
					DEBUG("Error of Calloc");
					WIFIAudio_LightControlFormat_FreeppDataContent(&pDataContent);
				}
				else
				{
					Current = 0;
					
					//横向灯珠
					WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
					&(pDataContent->Data.pLedMatrixScreen->Length), &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_LENGTH_SIZE, \
					pCmdData->pDataBuff);
					
					//纵向灯珠
					WIFIAudio_LightControlFormat_CopypAsciiTopHex(\
					&(pDataContent->Data.pLedMatrixScreen->Width), &Current, \
					WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_WIDTH_SIZE, \
					pCmdData->pDataBuff);
					
					//计算等级个数
					Product = pDataContent->Data.pLedMatrixScreen->Length \
					* pDataContent->Data.pLedMatrixScreen->Width;
					
					//灯珠色值数据
					WIFIAudio_LightControlFormat_pBcdDataTopStringData(\
					&(pDataContent->Data.pLedMatrixScreen->pData), \
					&Current, Product, pCmdData->pDataBuff);
				}
			}
		}
	}
	
	return pDataContent;
}

//将已经格式化的命令数据结构体当中的数据，还原成原始数据
LedUart_pDataContent WIFIAudio_LightControlFormat_pCmdDataTopDataContent(LedUart_pCmdData pCmdData)
{
	LedUart_pDataContent pDataContent = NULL;
	
	if(NULL == pCmdData)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		switch(pCmdData->Type)
		{
			case WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_SINGLELIGHT:
				pDataContent = WIFIAudio_LightControlFormat_pCmdDataTopDataContent_SingleLight(pCmdData);
				break;
				
			case WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_LEDMATRIXSCREEN:
				pDataContent = WIFIAudio_LightControlFormat_pCmdDataTopDataContent_LedMatrixScreen(pCmdData);
				break;
				
			default:
				break;
		}
	}
	
	return pDataContent;
}


//将通信格式的数据直接转换成原始数据
LedUart_pDataContent WIFIAudio_LightControlFormat_pBuffTopDataContent(WFLCF_pBuff pBuff)
{
	LedUart_pDataContent pDataContent = NULL;
	LedUart_pCmdData pCmdData = NULL;
	
	if(NULL == pBuff)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		pCmdData = WIFIAudio_LightControlFormat_pBuffTopCmdData(pBuff);
		
		if(NULL != pCmdData)
		{
			pDataContent = WIFIAudio_LightControlFormat_pCmdDataTopDataContent(pCmdData);
			WIFIAudio_LightControlFormat_FreeppCmdData(&pCmdData);
		}
	}
	
	return pDataContent;
}


WFLCF_pBuff WIFIAudio_LightControlFormat_pDataContentTopBuff(LedUart_pDataContent pDataContent)
{
	WFLCF_pBuff pBuff = NULL;
	LedUart_pCmdData pCmdData = NULL;
	
	if(NULL == pDataContent)
	{
		DEBUG("Error of Parameter");
	}
	else
	{
		pCmdData = WIFIAudio_LightControlFormat_pDataContentTopCmdData(pDataContent);
		
		if(NULL != pCmdData)
		{
			pBuff = WIFIAudio_LightControlFormat_pCmdDataTopBuff(pCmdData);
			WIFIAudio_LightControlFormat_FreeppCmdData(&pCmdData);
		}
	}
	
	return pBuff;
}






