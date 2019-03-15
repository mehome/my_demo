#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h> 
#include <pthread.h>
#include <sys/wait.h>
#include <limits.h>
#include "uartd.h"
#include "uartfifo.h"
#include "montage.h"
#include "debug.h"
#include "command_parse.h"
#include <wdk/cdb.h>
#include <libcchip/platform.h>
#include <libcchip/function/back_trace/back_trace.h>
#include <myutils/msg_queue.h>

extern int alsaplay_thread(void);

#define UARTD_PID "/var/run/uartd.pid"
pthread_t thread[5];
pthread_mutex_t pmMUT;

int iUartfd;

extern int speech_or_iflytek;

extern char g_byAmazonFlag;

extern int record;


void thread_create(void)
{
	int temp;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN *4);
	memset(&thread, 0, sizeof(thread));

	if((temp = pthread_create(&thread[4], &a_thread_attr, command_parse, NULL)) != 0)
	{
		DEBUG_PERR("command_parse_thread create failed..");
	}
	else
	{
		DEBUG_TRACE("command_parse_thread create succeed..");
	}
	
	if((temp = pthread_create(&thread[0], &a_thread_attr, uartd_thread, NULL)) != 0)
	{
		DEBUG_PERR("uartd_thread create failed..");
	}

	if((temp = pthread_create(&thread[1], &a_thread_attr, uartfifo_thread, NULL)) != 0)
	{
		DEBUG_PERR("uartfifo_thread create failed..");
	}
	else
	{
		DEBUG_TRACE("uartfifo_thread create succeed..");
	}

	// if((temp = pthread_create(&thread[2], &a_thread_attr, alsaplay_thread, NULL)) != 0)
	// {
		// DEBUG_PERR("alsaplay_thread create failed..");
	// }
	// else
	// {
		// DEBUG_INFO("alsaplay_thread create succeed..");
	// }
#if 0
	if((temp = pthread_create(&thread[3], &a_thread_attr, check_usb_tf, NULL)) != 0)
	{
		DEBUG_PERR("check_usb_tf_thread create failed..");
	}
	else
	{
		DEBUG_TRACE("check_usb_tf_thread create succeed..");
	}
#endif

}

void thread_wait(void)
{

	if (thread[4] != 0) 
	{
		pthread_join(thread[4],NULL);
		DEBUG_TRACE("uartfifo_thread end...");
	}
	if (thread[0] != 0)
	{
		pthread_join(thread[0],NULL);
		DEBUG_TRACE("uartd_thread end...");
	}

	if (thread[1] != 0) 
	{
		pthread_join(thread[1],NULL);
		DEBUG_TRACE("uartfifo_thread end...");
	}
#if 0
	if (thread[2] != 0) 
	{
		pthread_join(thread[2],NULL);
		DEBUG_TRACE("uartfifo_thread end...");
	}

	if (thread[3] != 0) 
	{
		pthread_join(thread[3],NULL);
		DEBUG_TRACE("uartfifo_thread end...");
	}


#endif
	
}

void func_waitpid(int signo) {
    pid_t pid;
    int stat;
    while( (pid = waitpid(-1, &stat, WNOHANG)) > 0 ) {
        DEBUG_INFO( "child %d exit", pid );
    }
    return;
}

void sigalrm_fn(int sig)
{
    DEBUG_INFO("alarm!..");
	g_byAmazonFlag = 0;
	record = 0;

	alarm(0);

	//tlkoff();
	DEBUG_INFO("tlkoff...");
	
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK+OFF&", strlen("AXX+TLK+OFF&"));
	pthread_mutex_unlock(&pmMUT);

	system("wavplayer /root/res/ring2.wav -M");
	system("mpc volume 200");
}

int lock_pid(){
    int fd = -1; 
    // char buf[32]; 
    fd = open(UARTD_PID, O_WRONLY | O_CREAT, 0666); 
    if (fd < 0) { 
       DEBUG_PERR("Fail to open"); 
       exit(1); 
   } 
   struct flock lock; 
   bzero(&lock, sizeof(lock)); 
   if (fcntl(fd, F_GETLK, &lock) < 0) { 
      DEBUG_INFO("Fail to fcntl F_GETLK"); 
      exit(1); 
    } 
  lock.l_type = F_WRLCK; 
  lock.l_whence = SEEK_SET; 
  if (fcntl(fd, F_SETLK, &lock) < 0) { 
     DEBUG_INFO("Fail to fcntl F_SETLK"); 
     exit(1); 
  } 
  // pid_t pid = getpid(); 
  // int len = snprintf(buf, 32, "%d\n", (int)pid);
  return 0;
}

void SpeechSelect(void)
{
	char value[16]={0};

	int Speech = cdb_get_int("$speech_recognition", 3);
	speech_or_iflytek = Speech;
	
	snprintf(value,16, "%d", Speech);
	DEBUG_INFO("value:%s",value);
	cdb_set("$speech_recognition", value);
	//WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.speech_recognition", value);
}

int main(int argc,char *argv[])
{
	traceSig(SIGSEGV,SIGBUS,SIGABRT);
//	clog(Hred,"compile in [%s %s]",__DATE__,__TIME__);
	log_init(argv[1]);	//uartd l=11111
  	lock_pid();
	int ret=timer_cdb_init(__SIGRTMIN+5);
	if(ret<0)return -1;
	signal(SIGALRM, sigalrm_fn);
	pthread_mutex_init(&pmMUT, NULL);
    msg_queue_create_msg_queue(MAGIC_NUMBER);
	SpeechSelect();
    iUartfd = open_port();		//初始化的时候即打开uart设备，获取iUartfd
    set_opt(iUartfd,115200,8,'N',1);
	signal(SIGCHLD, &func_waitpid);
	if(!cdb_get_int("$vtf003_usb_tf",0))
		cdb_set_int("$playmode", 000);
	 // WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.playmode", "000");
	cmd_fifo_init();
	thread_create();
	
/* BEGIN: Modified by Frog, 2017-4-6 19:01:18 */
/*
	sleep(10);
	check_usb_tf();
*/
/* END:   Modified by Frog, 2017-4-6 19:01:36 */

	thread_wait();
  	cmd_fifo_free();	
  return 0;
}

