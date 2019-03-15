#ifdef __cplusplus
extern "C" {
#endif

#ifndef POSIX_THREAD_OPS_H_
#define POSIX_THREAD_OPS_H_

typedef struct posixThreadOps_t{
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	volatile char isListenSuspend;
	void (*enter_listen)(struct posixThreadOps_t* ptr);
	void (*exit_listen)(struct posixThreadOps_t* ptr);		
	void (*wait)(struct posixThreadOps_t* ptr);
	void (*signal)(struct posixThreadOps_t* ptr);
	void (*broadcast)(struct posixThreadOps_t* ptr);
	void (*resum)(struct posixThreadOps_t* ptr);
	void (*pause)(struct posixThreadOps_t* ptr);
}posixThreadOps_t;

posixThreadOps_t* posix_thread_ops_create(void *args);
int get_posix_thread_ops_listenstate(posixThreadOps_t *ptr);

#define posix_thread_ops_listen_wait(ptr) ({\
	if(ptr){\
		int isListenSuspend=0;\
		pthread_mutex_lock(&ptr->mtx);\
		isListenSuspend=ptr->isListenSuspend;\
		pthread_mutex_unlock(&ptr->mtx);\
		if(isListenSuspend){\
			trc("isListenSuspend");\
			posix_thread_ops_wait(ptr);\
		}\
	}\
})

int posix_thread_set_name(char *name);//name limit 16 bytes

#endif

#ifdef __cplusplus
}
#endif
