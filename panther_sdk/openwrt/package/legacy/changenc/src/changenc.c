#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>



#define SIG_PRESS_BUTTON    __SIGRTMIN+10 
#define OMNICFG_NAME "omnicfg_bc"

/*
 *功能描述：查询omnicfg_bc的进程号,并发送__SIGRTMIN+10 给该进程,改变广播信息中net_conf的值 此值为0
 */

int pid__[2] ={0,0};




int press_send_signal(int pid)
{
	int ret = -1;
	if ((ret = kill(pid, SIG_PRESS_BUTTON)) == 0)//向子进程发出SIG_PRESS_BUTTON信号  
	{  
		printf("send signal to %d\n", pid) ;  
	}  
	
	return ret;
}

int seach_omnicfg_pid_and_send(char *name )
{
	FILE *pstr;
	char cmd[128] = "";
	char buff[128] = "";
	sprintf(cmd, "ps -ef | grep %s |grep -v grep | awk '{print $1}'",name);

    pstr=popen(cmd, "r");//    
	if(pstr==NULL)
	{
		printf("popen error");
		return -1; 
	}
	while( fgets(buff, sizeof(buff), pstr))
    {
        
		 press_send_signal(atoi(buff));
         memset(buff, 0, sizeof(buff));
    }
	
	fclose(pstr);
	return 0;
}

int main (int argc, char** argv)
{
    //printf(" ------------------%d------------\n",SIG_PRESS_BUTTON ) ; 
	seach_omnicfg_pid_and_send(OMNICFG_NAME);
    return 0;
}
