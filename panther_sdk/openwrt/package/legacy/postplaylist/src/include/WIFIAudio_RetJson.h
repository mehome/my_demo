#ifndef __WIFIAUDIO_COMMANDINFORMATION_H__
#define __WIFIAUDIO_COMMANDINFORMATION_H__

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************
 ** 函数名: WIFIAudio_RetJson_retJSON
 ** 功  能: 打印JSON格式相关的信息
 ** 参  数: pstring为指向返回JSON相关ret字段内容的字符串指针
 ** 返回值: 成功返回0，失败返回-1
 ******************************************/ 
extern int WIFIAudio_RetJson_retJSON(char *pstring);


#ifdef __cplusplus
}
#endif

#endif
