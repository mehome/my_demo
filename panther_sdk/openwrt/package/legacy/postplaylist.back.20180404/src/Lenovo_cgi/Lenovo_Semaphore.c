#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>  
#include <string.h>

#include "Lenovo_Semaphore.h"

//设置信号量的值
int Lenovo_Semaphore_SetSemValue(int sem_id, int value)
{
	int ret = 0;
	
	Lenovo_Sem_Semun sem_union;
	sem_union.Value = value;

	semctl(sem_id,0,SETVAL,sem_union);

	return ret;
}

//根据key值，创建或获取信号量，如果是创建赋予初值
int Lenovo_Semaphore_CreateOrGetSem(key_t key, int value)
{
	int sem_id = -1;
	
	sem_id = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL);
	if(sem_id < 0)
	{
		sem_id = semget(key, 1, 0666 | IPC_CREAT);
	} else {
		Lenovo_Semaphore_SetSemValue(sem_id, value);
	}
	
	return sem_id;
}

//删除信号量，如果集合已为空则直接删除
int Lenovo_Semaphore_DeleteSem(int sem_id)
{
	int ret = 0;
	Lenovo_Sem_Semun sem_union;
	
	semctl(sem_id,0,IPC_RMID,sem_union);
	
	return ret;
}

//信号量P操作：对信号量进行减一操作
int Lenovo_Semaphore_Operation_P(int sem_id)
{
	int ret = 0;
	struct sembuf sembuff;

	sembuff.sem_num = 0;
	sembuff.sem_op = -1;
	sembuff.sem_flg = SEM_UNDO;
	
	semop(sem_id,&sembuff,1);
	
	return ret;
}

//信号量V操作：对信号量进行加一操作
int Lenovo_Semaphore_Operation_V(int sem_id)
{
	int ret = 0;
	struct sembuf sembuff;

	sembuff.sem_num = 0;
	sembuff.sem_op = 1;
	sembuff.sem_flg = SEM_UNDO;
	
	semop(sem_id,&sembuff,1);
	
	return ret;
}

