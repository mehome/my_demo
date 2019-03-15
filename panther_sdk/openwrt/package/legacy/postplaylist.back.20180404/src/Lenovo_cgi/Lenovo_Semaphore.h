#ifndef __LENOVO_SEMAPHORE_H__
#define __LENOVO_SEMAPHORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

//包含信号量定义的头文件
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>

//联合类型semun定义
typedef union _Lenovo_Sem_Semun{
	int Value;
	struct semid_ds *pBuff;
	unsigned short *pArray;
}Lenovo_Sem_Semun,*pLenovo_Sem_Semun;

//设置信号量的值
extern int Lenovo_Semaphore_SetSemValue(int sem_id, int value);

//根据key值，创建或获取信号量，如果是创建赋予初值
extern int Lenovo_Semaphore_CreateOrGetSem(key_t key, int value);

//删除信号量，如果集合已为空则直接删除
extern int Lenovo_Semaphore_DeleteSem(int sem_id);

//信号量P操作：对信号量进行减一操作
extern int Lenovo_Semaphore_Operation_P(int sem_id);

//信号量V操作：对信号量进行加一操作
extern int Lenovo_Semaphore_Operation_V(int sem_id);


#ifdef __cplusplus
}
#endif

#endif
