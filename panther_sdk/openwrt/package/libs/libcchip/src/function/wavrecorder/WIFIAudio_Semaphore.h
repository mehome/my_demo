#ifndef __WIFIAUDIO_SEMAPHORE_H__
#define __WIFIAUDIO_SEMAPHORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

//#include "debugprintf.h"

#include <sys/sem.h>//包含信号量定义的头文件
#include <sys/types.h>
#include <sys/ipc.h>

#define WIFIAUDIO_UCICONFIG_SEMKEY (8163)
//存储全局配置文件的文件，访问之前的信号量key值
//这个值是随便指定的，可以改，因为文件可能会重新创建，所以这边


//联合类型semun定义
typedef union _WASEM_Semun{
	int Value;
	struct semid_ds *pBuff;
	unsigned short *pArray;
}WASEM_Semun,*pWASEM_Semun;

//设置信号量的值
extern int WIFIAudio_Semaphore_SetSemValue(int sem_id, int value);

//根据key值，创建或获取信号量，如果是创建赋予初值
extern int WIFIAudio_Semaphore_CreateOrGetSem(key_t key, int value);

//删除信号量，如果集合已为空则直接删除
extern int WIFIAudio_Semaphore_DeleteSem(int sem_id);

//信号量P操作：对信号量进行减一操作
extern int WIFIAudio_Semaphore_Operation_P(int sem_id);

//信号量V操作：对信号量进行加一操作
extern int WIFIAudio_Semaphore_Operation_V(int sem_id);

//控制wavRecord停止录音的状态
extern void SetState_StopRecord(int status);
extern int GetState_StopRecord(void);


#ifdef __cplusplus
}
#endif

#endif
