#ifndef __GET_APE_INFO_H__
#define __GET_APE_INFO_H__

typedef struct {
	unsigned char buf[4];
}ape_flag_t;

/******************************************
 ** ��  ��: ��ȡ*papepath·��ָ��ape�ļ��ı��⡢�����ҡ�ר����Ϣ
 ** ����ֵ: �ɹ�����WIGMI_pMusicInformationָ�룬ʧ�ܷ���NULL,��ʽ����ȷ���� 0x1000
 ******************************************/ 
WIGMI_pMusicInformation WIFIAudio_GetAPEInformation_APEPathTopAPEInformation(char *papepath);
int WIFIAudio_GetAPEInformation_freeAPEInformation(WIGMI_pMusicInformation papeinformation);


#endif
