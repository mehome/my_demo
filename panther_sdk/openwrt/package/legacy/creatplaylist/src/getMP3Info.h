#ifndef __GET_MP3_INFO_H__
#define __GET_MP3_INFO_H__
#include "getMusicInfo.h"


/******************************************
 ** ��  ��: ��ȡ*pmp3path·��ָ��mp3�ļ��ı��⡢�����ҡ�ר����Ϣ
 ** ����ֵ: �ɹ�����WIGMI_pMusicInformationָ�룬ʧ�ܷ���NULL,��ʽ����ȷ���� 0x1000
 ******************************************/ 
WIGMI_pMusicInformation WIFIAudio_GetMP3Information_MP3PathTopMP3Information(char *pmp3path);
int WIFIAudio_GetMP3Information_freeMP3Information(WIGMI_pMusicInformation ppmp3information);



#endif
