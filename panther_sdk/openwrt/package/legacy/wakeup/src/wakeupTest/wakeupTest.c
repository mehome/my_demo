#include <pthread.h>
#include "../interface.h"

int main(int argc,char *argv[]){
		wakeupStartArgs_t wakeupStartArgs={0};
		wakeupStartArgs.wakeup_waitctl = posix_thread_ops_create(NULL);
		if(!wakeupStartArgs.wakeup_waitctl){
			err("posix_thread_ops_create failure");
			return -1;
		}		
		#if defined(KWD_SENSORY)
	sensory_init();
#endif
		if(argv[1] && atoi(argv[1])){
			wakeupStartArgs.wakeup_mode=1;
		}
		struct sched_param param;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		param.sched_priority = 60;
		pthread_attr_setschedpolicy (&attr, SCHED_RR);
		pthread_attr_setschedparam(&attr, &param);
		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		wakeupStartArgs.pAttr=&attr;
	    int err  = wakeupStart(&wakeupStartArgs);
		if(err<0){
			err("wakeupStart failure,err=%d",err);
			return -2;
		}
		inf("recorder thread is OK!");
		if(argv[1]){
			if(atoi(argv[1])>0){
				sleep(atoi(argv[1]));
				exit(0);
			}
		}
		while(1)pause();
		return 0;

}
