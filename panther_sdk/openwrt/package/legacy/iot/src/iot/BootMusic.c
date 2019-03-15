

#include <pthread.h>
#include <limits.h>

#include "myutils/debug.h"
#include "common.h"
static pthread_t bootMusicThread = -1;

static int bootMusicHasRun = 0;
static int stopBootMusic = 0;

static void BootMusic(void *arg)
{
	int i = 0;
	bootMusicHasRun = 1;
	int pid =-1;
	while(1) {
		if(stopBootMusic)
			break;
		pid = get_pid("wavplayer");
		info("pid:%d",pid);
		if(pid <=0)
			break;
		usleep(200*1000);
	}
	//sleep(2);
	for( i =0; i < 5; i++) {
		int mode = 0;
		if(stopBootMusic)
			break;
		IotGetAudio(mode);
		sleep(2);
		//mode = !mode;
	}
	stopBootMusic = 0;
	pthread_exit(0);
}
void StartBootMusic(void )
{

	if (bootMusicHasRun)   
        return ;
	if (stopBootMusic)
		return ;
	pthread_t a_thread;
    pthread_attr_t a_thread_attr;
    PTHREAD_SET_UNATTACH_ATTR(a_thread_attr);
	
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);
	pthread_create(&bootMusicThread, &a_thread_attr, BootMusic, NULL);
	pthread_attr_destroy(&a_thread_attr);
}
void StopBootMusic()
{
	info("stop BOOT MUSIC");
	stopBootMusic = 1;
	if(bootMusicHasRun) {
		if(bootMusicThread != -1)
			pthread_join(bootMusicThread, NULL);
	}
	bootMusicHasRun = 0;	
	bootMusicThread = -1;
	//}
}

