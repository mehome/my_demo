#include <stdio.h>
#include <wdk/cdb.h>
#include <function/common/fd_op.h>
#include "micarray.h"
#include <mon_config.h>

#define WIFIAUDIO_ASP_PATHNAME "/tmp/misc/asp.raw"
//降噪录音文件的路径

#define WIFIAUDIO_MIC_PATHNAME "/tmp/misc/mic0.raw"
//麦克录音文件的路径

#define WIFIAUDIO_REF_PATHNAME "/tmp/misc/ref0.raw"
//参考信号录音文件的路径
#define CX20921_CARD "cx20921"
#define MICARRAY_CARD "micarray"

int get_micarray_info(micarray_Info **pMicarray_info){
	micarray_Info *pInfo = NULL;
	if(pMicarray_info == NULL){
		err("pMicarray_info = NULL");
		return -1;
	}
	pInfo = realloc(pInfo, sizeof(micarray_Info));
	if(pInfo == NULL){
		err("realloc failure");
		return -1;
	}
	pInfo->mics = MICS;
	pInfo->mics_channel = MICS_CHANNEL;
	pInfo->refs = REFS;
	pInfo->refs_channel = REF_CHANNEL;
#if defined (CONFIG_MICARRY_RECOED)	//doing
	pInfo->record_dev = MICARRAY_CARD;
#else
	pInfo->record_dev = CX20921_CARD;
#endif
//	pInfo->record_dev = MICARRAY_CARD;

	*pMicarray_info = pInfo;
	return 0;
}
//开始mic阵录音
int micarray_record(int value){
	int ret = 0;
	char str[256] = {0};
//	printf("1111111");
#if defined(CONFIG_PACKAGE_duer_linux)
	sprintf(str,"cdb set duer_nocheck 1;killall duer_linux;killall micarray;micarray %d &",value);
#elif defined(CONFIG_PROJECT_K2_V1)
	sprintf(str,"cdb set iot_nocheck 1;killall -9 turingIot;killall -9 micarray;micarray %d & > /dev/null",value);
#endif
	printf("str:%s",str);
//	ret = system("cdb set duer_nocheck 1;killall duer_linux;killall micarray;micarray 10 &");
	ret = system(str);
	if (ret != 0){
		ret = -1;
	}
	return ret;		
}

//停止mic阵录音
int micarray_stop_record(){
	int ret = 0;
#if defined(CONFIG_PACKAGE_duer_linux)
	ret = system("killall micarray;cdb set duer_nocheck 0");
#elif defined(CONFIG_PROJECT_K2_V1)
	ret = system("killall -9 micarray;cdb set iot_nocheck 0 > /dev/null");
#endif

	if (ret != 0){
		ret = -1;
	}
	return ret;		
}

//获取降噪录音文件路径
char* micarray_normal_record_path(){
	return WIFIAUDIO_ASP_PATHNAME;		
}

//获取麦克录音文件路径
char* micarray_mic_record_path(){
	return WIFIAUDIO_MIC_PATHNAME;		
}

//获取参考信号录音文件路径
char* micarray_ref_record_path(){
	return WIFIAUDIO_REF_PATHNAME;		
}
