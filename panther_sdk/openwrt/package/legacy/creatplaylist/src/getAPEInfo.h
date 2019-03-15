#ifndef __GET_APE_INFO_H__
#define __GET_APE_INFO_H__

typedef struct {
	unsigned char buf[4];
}ape_flag_t;

/******************************************
 ** 功  能: 获取*papepath路径指向ape文件的标题、艺术家、专辑信息
 ** 返回值: 成功返回WIGMI_pMusicInformation指针，失败返回NULL,格式不正确返回 0x1000
 ******************************************/ 
WIGMI_pMusicInformation WIFIAudio_GetAPEInformation_APEPathTopAPEInformation(char *papepath);
int WIFIAudio_GetAPEInformation_freeAPEInformation(WIGMI_pMusicInformation papeinformation);


#endif
