#ifndef __GET_M4A_INFO_H__
#define __GET_M4A_INFO_H__

/******************************************
 ** ��  ��: ��ȡ*pm4apath·��ָ��m4a�ļ��ı��⡢�����ҡ�ר����Ϣ
 ** ����ֵ: �ɹ�����WIGMI_pMusicInformationָ�룬ʧ�ܷ���NULL,��ʽ����ȷ���� 0x1000
 ******************************************/ 
WIGMI_pMusicInformation WIFIAudio_GetM4AInformation_M4APathTopM4AInformation(char *pm4apath);
int WIFIAudio_GetM4AInformation_freeM4AInformation(WIGMI_pMusicInformation pm4ainformation);

#endif
