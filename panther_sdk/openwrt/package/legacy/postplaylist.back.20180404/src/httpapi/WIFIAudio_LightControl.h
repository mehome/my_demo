#ifndef __WIFIAUDIO_LIGHTCONTROL_H__
#define __WIFIAUDIO_LIGHTCONTROL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "WIFIAudio_LightControlFormat.h"
#include "WIFIAudio_LightControlUart.h"
#include "WIFIAudio_Debug.h"

//重命名传输当中，每个字节的类型
typedef unsigned char WFLC_Byte, *WFLC_pBuff;



//根据WIFIAudio_LightControlFormat当中定义的起始标志和结束标志
//来获取一个完整指令的buff
extern WFLC_pBuff WIFIAudio_LightControl_GetCompleteCommandBuff(WFLCU_pUartBuff pUartBuff);

//释放一整段buff
extern int WIFIAudio_LightControl_FreeppBuff(WFLC_pBuff *ppBuff);

//根据开始标志和结束标志来计算一个完整命令buff的长度
extern int WIFIAudio_LightControl_GetCommandBuffLength(WFLC_pBuff pBuff);

#ifdef __cplusplus
}
#endif

#endif
