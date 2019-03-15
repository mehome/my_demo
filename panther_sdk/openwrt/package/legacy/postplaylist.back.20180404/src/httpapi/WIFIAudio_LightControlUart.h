#ifndef __WIFIAUDIO_LIGHTCONTROLUART_H__
#define __WIFIAUDIO_LIGHTCONTROLUART_H__

#ifdef __cplusplus
extern "C"
{
#endif

//这个库只管接受就可以了，取命令和解析不管

//创建一片专门读串口的内存
#define WIFIAUDIO_LIGHTCONTROLUART_UATRBUFFSIZE (4096)

//一次读取串口的长度
#define WIFIAUDIO_LIGHTCONTROLUART_ONCEREADSIZE (16)

//串口的写设备节点
#define WIFIAUDIO_LIGHTCONTROLUART_WRITESERIAL "/dev/ttyS1"

//串口的读设备节点
#define WIFIAUDIO_LIGHTCONTROLUART_READSERIAL "/dev/ttyS1"

//重命名传输当中，每个字节的类型
typedef unsigned char WFLCU_Byte, *WFLCU_pBuff;

typedef struct _WFLCU_pUartBuff
{
	WFLCU_pBuff pBuff;
	int BuffLen;
	int CurrentLen;
}WFLCU_UartBuff, *WFLCU_pUartBuff;//串口读取缓存


//打开串口，不想调用那么多函数，全部封装在一起
extern int WIFIAudio_LightControlUart_OpenUart(char *pDevice, int Speed, int DataBits, int StopBits, int Parity);


//创建一个新的空的串口buff
extern WFLCU_pUartBuff WIFIAudio_LightControlUart_NewUartBuff(int Bufflen);


//从串口读取一段内容存入结构体当中，返回负值为读取出错
extern int WIFIAudio_LightControlUart_ReadUart(int fd, WFLCU_pUartBuff pUartBuff, int Len);


//从串口写入一段内容存入结构体当中，返回大于0为写入成功
extern int WIFIAudio_LightControlUart_WriteUart(int fd, WFLCU_pUartBuff pUartBuff);

//释放一片空的串口buff
extern int WIFIAudio_LightControlUart_FreeppUartBuff(WFLCU_pUartBuff *ppUartBuff);

#ifdef __cplusplus
}
#endif

#endif
