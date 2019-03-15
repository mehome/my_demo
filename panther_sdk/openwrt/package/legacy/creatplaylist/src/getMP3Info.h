#ifndef __GET_MP3_INFO_H__
#define __GET_MP3_INFO_H__
#include "getMusicInfo.h"


/******************************************
 ** 功  能: 获取*pmp3path路径指向mp3文件的标题、艺术家、专辑信息
 ** 返回值: 成功返回WIGMI_pMusicInformation指针，失败返回NULL,格式不正确返回 0x1000
 ******************************************/ 
WIGMI_pMusicInformation WIFIAudio_GetMP3Information_MP3PathTopMP3Information(char *pmp3path);
int WIFIAudio_GetMP3Information_freeMP3Information(WIGMI_pMusicInformation ppmp3information);



#endif
