#ifndef WAKEUP_INTERFACE_H_
#define WAKEUP_INTERFACE_H_
#include <libcchip/platform.h>
#include <libcchip/function/user_timer/user_timer.h>

typedef void *(*pStart_routine_t) (void *);
int sensory_init(void);

void setMutMicStatus(char status);
char getMutMicStatus(void);

typedef struct wakeupStartArgs_t{	
	void *PortAudioMicrophoneWrapper_ptr;
	void (*pRecordDataInputCallback)(char* buffer, long unsigned int size, void *userdata);
	pthread_t wakeupPthreadId;	//pid
	pStart_routine_t pStart_routine;
	pthread_attr_t *pAttr;		
	char isWakeUpSucceed;	//是否唤醒成功
	userTimer_t* userTimerPtr;
	volatile char wakeup_mode;	//模式
	volatile char isAspDataClearedFlag;
    
    //isMutiVadListening :Used as a mark for distinguishing recording purposes
    //0:waiting 
    //1:speech interact
    //2:wechat
    //3:translate
    //4:sinvoice
	volatile char isMutiVadListening;
	posixThreadOps_t *wakeup_waitctl;
	unsigned long long timestamp;
	volatile char isSensoryRecgnizeClosed;	
    void (*pCancelupload)(void);
    void (*pCleanBufferCallback)(void);

    void (*pStopAllAudio)(void);
	
}wakeupStartArgs_t;

extern wakeupStartArgs_t *wakeupStartArgsPtr;
int wakeupStart(wakeupStartArgs_t *pArgs);
int sensory_init(void);

#endif
