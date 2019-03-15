#ifndef __WIFIAUDIO_LIGHTCONTROLFORMAT_H__
#define __WIFIAUDIO_LIGHTCONTROLFORMAT_H__

#ifdef __cplusplus
extern "C"
{
#endif

//开始标志占用1个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG_SIZE (1)
//命令类型占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_TYPE_SIZE (2)
//数据长度占用3个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_DATE_LENGTH_SIZE (3)
//校验和占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_CHECK_SUM_SIZE (2)
//结束标志占用1个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG_SIZE (1)

//开始标志
#define WIFIAUDIO_LIGHTCONTROLFORMAT_START_FLAG (0xaa)
//结束标志
#define WIFIAUDIO_LIGHTCONTROLFORMAT_END_FLAG (0xbb)

//重命名传输当中，每个字节的类型
typedef unsigned char WFLCF_Byte, *WFLCF_pBuff;

enum WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE
{
	WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_SINGLELIGHT = 0,
	WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE_LEDMATRIXSCREEN,
};

typedef struct _LedUart_pCmdData
{
	enum WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE Type;
	int Len;
	WFLCF_pBuff pDataBuff;
}LedUart_CmdData, *LedUart_pCmdData;//命令类型和数据


//使能占用1个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_ENABLE_SIZE (1)
//亮度占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BRIGHTNESS_SIZE (2)
//Red占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_RED_SIZE (2)
//Green占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_GREEN_SIZE (2)
//Blue占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_SINGLELIGHT_BLUE_SIZE (2)

typedef struct _LedUart_SingleLight
{
	int Enable;			//使能
	int Brightness;		//亮度
	int Red;			//Red
	int Green;			//Green
	int Blue;			//Blue
}LedUart_SingleLight, *LedUart_pSingleLight;//组灯控制


//横向灯珠占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_LENGTH_SIZE (2)
//纵向灯珠占用2个字节
#define WIFIAUDIO_LIGHTCONTROLFORMAT_LEDMATRIXSCREEN_WIDTH_SIZE (2)

typedef struct _LedUart_LedMatrixScreen
{
	int Length;			//横向个数
	int Width;			//纵向个数
	char *pData;		//灯珠数据
}LedUart_LedMatrixScreen, *LedUart_pLedMatrixScreen;//点阵屏显示

typedef struct _LedUart_DataContent
{
	enum WIFIAUDIO_LIGHTCONTROLFORMAT_COMMANDTYPE Type;
	union
	{
		LedUart_pSingleLight pSingleLight;
		LedUart_pLedMatrixScreen pLedMatrixScreen;
	}Data;
}LedUart_DataContent, *LedUart_pDataContent;//原始数据结构体


//释放一整段buff
extern int WIFIAudio_LightControlFormat_FreeppBuff(WFLCF_pBuff *ppBuff);

//释放原始数据结构体
extern int WIFIAudio_LightControlFormat_FreeppDataContent(LedUart_pDataContent *ppDataContent);

//将通信格式的数据直接转换成原始数据
extern LedUart_pDataContent WIFIAudio_LightControlFormat_pBuffTopDataContent(WFLCF_pBuff pBuff);

//将原始数据转换成通信格式的数据
extern WFLCF_pBuff WIFIAudio_LightControlFormat_pDataContentTopBuff(LedUart_pDataContent pDataContent);




#ifdef __cplusplus
}
#endif

#endif
