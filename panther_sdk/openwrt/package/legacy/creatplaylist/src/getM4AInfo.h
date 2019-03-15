#ifndef __GET_M4A_INFO_H__
#define __GET_M4A_INFO_H__

/******************************************
 ** 功  能: 获取*pm4apath路径指向m4a文件的标题、艺术家、专辑信息
 ** 返回值: 成功返回WIGMI_pMusicInformation指针，失败返回NULL,格式不正确返回 0x1000
 ******************************************/ 
WIGMI_pMusicInformation WIFIAudio_GetM4AInformation_M4APathTopM4AInformation(char *pm4apath);
int WIFIAudio_GetM4AInformation_freeM4AInformation(WIGMI_pMusicInformation pm4ainformation);

#endif
