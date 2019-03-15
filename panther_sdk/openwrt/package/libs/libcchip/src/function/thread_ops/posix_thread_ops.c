#include <pthread.h>
#include <sys/prctl.h>
#include <platform.h>
#include "posix_thread_ops.h"

void posix_thread_ops_enter_listen(posixThreadOps_t *ptr){
	if(!ptr)return;
	pthread_mutex_lock(&ptr->mtx);
	ptr->isListenSuspend=1;
	pthread_mutex_unlock(&ptr->mtx);
}

void posix_thread_ops_exit_listen(posixThreadOps_t *ptr){
	if(!ptr)return;
	pthread_mutex_lock(&ptr->mtx);
	ptr->isListenSuspend=0;
	pthread_cond_broadcast(&ptr->cond);
	pthread_mutex_unlock(&ptr->mtx);
}

void posix_thread_ops_wait(posixThreadOps_t *ptr){
	if(!ptr)return;
	pthread_mutex_lock(&ptr->mtx);
	pthread_cond_wait(&ptr->cond, &ptr->mtx);
	pthread_mutex_unlock(&ptr->mtx);
}

void posix_thread_ops_signal(posixThreadOps_t *ptr){	
	if(!ptr)return;
	pthread_mutex_lock(&ptr->mtx);
	pthread_cond_signal(&ptr->cond);
	pthread_mutex_unlock(&ptr->mtx);	
}

void posix_thread_ops_broadcast(posixThreadOps_t *ptr){	
	if(!ptr)return;
	pthread_mutex_lock(&ptr->mtx);
	pthread_cond_broadcast(&ptr->cond);
	pthread_mutex_unlock(&ptr->mtx);
}

void posix_thread_ops_resum(posixThreadOps_t *ptr){
	posix_thread_ops_exit_listen(ptr);	
	posix_thread_ops_broadcast(ptr);
}

int get_posix_thread_ops_listenstate(posixThreadOps_t *ptr)
{
	int ret = 0;
	if(!ptr)return -1;
	pthread_mutex_lock(&ptr->mtx);
	ret = ptr->isListenSuspend;
	pthread_mutex_unlock(&ptr->mtx);
	return ret;
}

posixThreadOps_t* posix_thread_ops_create(void *args){
	posixThreadOps_t *ptr=calloc(1,sizeof(posixThreadOps_t));
	if(!ptr){
		err("calloc failed");
		return NULL;
	}
	pthread_mutex_init(&(ptr->mtx),NULL);
	pthread_cond_init(&(ptr->cond),NULL);
	ptr->wait			=&posix_thread_ops_wait;		//进入睡眠等待
	ptr->signal			=&posix_thread_ops_signal;		//唤醒某一条线程
	ptr->broadcast		=&posix_thread_ops_broadcast;	//唤醒所有线程
	ptr->resum			=&posix_thread_ops_resum;		//将ptr->isListenSuspend=0;再通知所有线程
	ptr->enter_listen	=&posix_thread_ops_enter_listen;//将ptr->isListenSuspend=1;
	ptr->exit_listen	=&posix_thread_ops_exit_listen;	//将ptr->isListenSuspend=0;
	ptr->isListenSuspend=0;
	return ptr;
}

int posix_thread_set_name(char *name){
	int ret=prctl(PR_SET_NAME,name);
	if(ret<0){
		show_errno(ret,__func__);
		return -1;
	}
	return  0;
}
