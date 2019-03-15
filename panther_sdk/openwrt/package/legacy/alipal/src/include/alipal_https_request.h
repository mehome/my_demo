#ifndef _ALIPAL_CONVER_OPUS_H_
#define _ALIPAL_CONVER_OPUS_H_

#define WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_SEMKEY (7954)

#define WIFIAUDIO_SYSTEMCOMMAND_HTTPGETBUFF_PATHNAME  "/tmp/alipal_https_request.xml"



//联合类型semun定义
typedef union _WASEM_Semun{
	int Value;
	struct semid_ds *pBuff;
	unsigned short *pArray;
}WASEM_Semun,*pWASEM_Semun;


char * AliPal_HttpGetUrl(char *purl);


#endif