#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>  
#include <string.h>

#include "amazon_Semaphore.h"

int Lenovo_Semaphore_SetSemValue(int sem_id, int value)
{
	int ret = 0;
	
	Lenovo_Sem_Semun sem_union;
	sem_union.Value = value;

	semctl(sem_id,0,SETVAL,sem_union);

	return ret;
}

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

int Lenovo_Semaphore_DeleteSem(int sem_id)
{
	int ret = 0;
	Lenovo_Sem_Semun sem_union;
	
	semctl(sem_id,0,IPC_RMID,sem_union);
	
	return ret;
}

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

