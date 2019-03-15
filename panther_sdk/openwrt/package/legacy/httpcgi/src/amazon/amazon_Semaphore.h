#ifndef __LENOVO_SEMAPHORE_H__
#define __LENOVO_SEMAPHORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>

typedef union _Lenovo_Sem_Semun{
	int Value;
	struct semid_ds *pBuff;
	unsigned short *pArray;
}Lenovo_Sem_Semun,*pLenovo_Sem_Semun;

extern int Lenovo_Semaphore_SetSemValue(int sem_id, int value);
extern int Lenovo_Semaphore_CreateOrGetSem(key_t key, int value);
extern int Lenovo_Semaphore_DeleteSem(int sem_id);
extern int Lenovo_Semaphore_Operation_P(int sem_id);
extern int Lenovo_Semaphore_Operation_V(int sem_id);

#ifdef __cplusplus
}
#endif

#endif
