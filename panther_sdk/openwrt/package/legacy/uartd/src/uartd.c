#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include <pthread.h>
#include <sys/reboot.h>
#include "uciConfig.h"
#include "debug.h"
#include <sys/file.h>
#include <sys/socket.h>  
#include <sys/un.h> 
#include <sys/time.h>  
#include <time.h>
#include <semaphore.h>
#include <mpd/client.h>
#include <signal.h>
#include <libcchip/function/fifobuffer/fifobuffer.h>
#include "uartd_test_mode.h"
#include <wdk/cdb.h>
#include <dirent.h>
#include "myutils/msg_queue.h"

extern pthread_mutex_t pmMUT;
extern int iUartfd;
extern int bt_flage;
// speech is 1,iflytek is 2,alexa is 3
int speech_or_iflytek = 4;

char g_byAmazonFlag = 0;



extern FT_FIFO * command_fifo;


extern pthread_mutex_t mtx ;  
extern pthread_cond_t cond ;  
extern int uartd_pthread_cond_wait;

#define UNIX_DOMAIN "/tmp/UNIX.baidu"


int open_port(void)
{
   int fd;

    //fd = open("/dev/ttyS0", O_RDWR|O_NOCTTY|O_NDELAY);
    fd = open("/dev/ttyS1", O_RDWR|O_NOCTTY|O_NDELAY);
    if (-1 == fd)
    {
        perror("Can't Open Serial Port");
        return(-1);
    }
    return fd;
}


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;

    if ( tcgetattr( fd,&oldtio) != 0)
    {
        DEBUG_INFO("SetupSerial 1");
        return -1;
    }
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch( nEvent )
    {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }
    switch( nSpeed )
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    if ( nStop == 1 )
    {
        newtio.c_cflag &= ~CSTOPB;
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |= CSTOPB;
    }
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN]  = 1;
    tcflush(fd,TCIFLUSH);
    if ((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        DEBUG_INFO("com set error");
        return -1;
    }

    return 0;
}





//#define UNIX_DOMAIN "/tmp/UNIX.domain"


#define SPEECH_SPEECH    1
#define SPEECH_IFLYTEK   2
#define SPEECH_ALEXA	   3
#define SPEECH_TURING 	 4

int record = 0;
int iflytek_file = 0;

extern sem_t playwav_sem;
extern struct mpd_connection *setup_connection(void);

static int speaker_not_wifi(void)
{
	system("wavplayer /tmp/res/speaker_not_connected.wav");
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+NET+ERR&", strlen("AXX+NET+ERR&"));
	printf("AXX+NET++ERR&..\n");
	write(iUartfd, "AXX+TLK++OFF&", strlen("AXX+TLK++OFF&"));
	pthread_mutex_unlock(&pmMUT);
	record = 0;
	return 1;
}

static int amazon_unsuccessful(void)
{
	system("wavplayer /tmp/res/amazon_unsuccessful.wav");
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+ALE+ERR&", strlen("AXX+NET+ERR&"));
	printf("AXX+NET++ERR&..\n");
	write(iUartfd, "AXX+TLK++OFF&", strlen("AXX+TLK++OFF&"));
	pthread_mutex_unlock(&pmMUT);
	record = 0;
	return 1;
}

#if 0

int OnWriteMsgToAlexa(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		printf("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		printf("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

#endif

void PrintSysTime(char *pSrc)
{

	struct timeval start;
    gettimeofday( &start, NULL );
   	printf("\033[1;31;40m [%s]---sys_time:%d.%d \033[0m\n", pSrc, start.tv_sec, start.tv_usec);
}

static int iflytek_tlkon() 
{
	int ret ;
	//char *getRecordStatus;					
	//int status = 0;
	
	
	//ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.recordstatus", "1");
    cdb_set_int("$recordstatus", 1);
	DEBUG_INFO("iflytek_file = %d",iflytek_file);
	system("mpc  pause");	
	system("ps | grep iflytek | grep -v grep && killall -9 iflytek");

	int wificonnectstatus = cdb_get_int("$wificonnectstatus",0);
	/*if (getRecordStatus == NULL)
	{
		ret = speaker_not_wifi();
		my_system("mpc volume 200");
		return ret;
	} else {
		status = atoi(getRecordStatus); 
		free(getRecordStatus);
		getRecordStatus = NULL;
		*/
		if (3 != wificonnectstatus) {
			ret = speaker_not_wifi();
			my_system("mpc volume 200");
			return ret;	
		}
		DEBUG_INFO("sem_post(&playwav_sem)");
		sem_post(&playwav_sem);
		
		pthread_mutex_lock(&pmMUT);
		write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));
		pthread_mutex_unlock(&pmMUT);	
	//}
	
	if(iflytek_file == 1) 
	{
		if (vfork() == 0)
		{
			ret = execl("/usr/bin/iflytek", "iflytek","upload", NULL);
			exit(-1);
		}
	}
	else 
	{
		if (vfork() == 0)
		{
				  ret = execl("/usr/bin/iflytek", "iflytek", NULL);
				  exit(-1);
		}
	}
	
	return ret;

}

static int Turing_tlkon() 
{
	int ret ;
	//char *getRecordStatus;					
//	int status = 0;
	
	system("mpc  pause");
	system("killall mpg123");
	int wificonnectstatus = cdb_get_int("$wanif_state",0);

    if (2 != wificonnectstatus) {
        ret = speaker_not_wifi();
        my_system("mpc volume 200");
        return ret;	
    }
    DEBUG_INFO("sem_post(&playwav_sem)");
    sem_post(&playwav_sem);

    pthread_mutex_lock(&pmMUT);
    write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));
    pthread_mutex_unlock(&pmMUT);	
	
    //发送消息给turing进程，让其执行相应动作
    msg_queue_send("StartSpeech", MSG_UARTD_TO_IOT);

	return ret;
}


static int Speech_tlkon(void)
{
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));
	pthread_mutex_unlock(&pmMUT);
	my_system("speechfifo.sh StartEngine");
	return 0;
}

static int Alexa_tlkon(void)
{
	int ret = 0;
	struct mpd_connection *conn = NULL;

	if (1 == g_byAmazonFlag)
	{
		printf("tlkon, g_byAmazonFlag:%d\n", g_byAmazonFlag);
		g_byAmazonFlag = 0;
		alarm(0);
		record = 0;
	}


	conn = setup_connection();
	if (conn != NULL) 
	{
		if (!mpd_run_set_volume(conn, 101))
			printErrorAndExit(conn);
		else
			mpd_connection_free(conn);
	}
	conn = NULL;

	int wificonnectstatus = cdb_get_int("$wanif_state",0);
	if (2 == wificonnectstatus)
	{
		int amazon_loginStatus = OnGetAVSSigninStatus();
		if (0 == OnGetAVSSigninStatus)
		{
			ret = amazon_unsuccessful();
			my_system("mpc volume 200");
			return ret;
		}
		else
		{
			sem_post(&playwav_sem);

			pthread_mutex_lock(&pmMUT);
			write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));
			pthread_mutex_unlock(&pmMUT);

			PrintSysTime("Start Send MSG to Alexa.");
			if (OnSendCommandToAmazonAlexa("StartEngine") != 0)
			{

				g_byAmazonFlag = 1;

				alarm(4);// 4S
			}
		}
	}
	else
	{
		ret = speaker_not_wifi();
		my_system("mpc volume 200");
		return ret;
	}


	return ret;
}

enum DIRECTIONS 
{
	START_RECORDER 		= 0,
	END_RECORDER,
	CONNECTING
};


int tlkon()
{
  int ret = socket_write(UNIX_DOMAIN, "MCU+TLK++ON&", strlen("MCU+TLK++ON&"));
  return ret;
  
#if 0
  int connect_fd;
  struct sockaddr_un srv_addr;
  char snd_buf[16];
  char rcv_buf[256];
  int ret;
  int i;
  connect_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if(connect_fd < 0)
  {
	perror("client create socket failed");
	return 1;
  }
  srv_addr.sun_family = AF_UNIX;
  strcpy(srv_addr.sun_path, UNIX_DOMAIN);
  ret = connect(connect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));

  if(ret == -1)
  {
	perror("connect to server failed!");
	close(connect_fd);
	unlink(UNIX_DOMAIN);
	return 1;
  }

  memset(snd_buf, 0, sizeof(snd_buf));
  sprintf(snd_buf, "%d", START_RECORDER);
  printf("sizeof(snd_buf): %d\n", strlen(snd_buf));

  printf("send data to server... ...\n");
  write(connect_fd, snd_buf, strlen(snd_buf));
  printf("send end!\n");

/*  
  memset(rcv_buf, 0, 256);
  int rcv_num = read(connect_fd, rcv_buf, sizeof(rcv_buf));
  printf("receive message from server (%d) :%s\n", rcv_num, rcv_buf);
*/
  close(connect_fd);
  return 0;
#endif
}

/*int tlkoff_test()
{
    int ret = 0;
	if (record == 0)
	{
		return 0;	
	}
	 DEBUG_INFO("tlkoff test ...");
	 pthread_mutex_lock(&pmMUT);
	 write(iUartfd, "AXX+TLK+OFF&", strlen("AXX+TLK+OFF&"));
	 pthread_mutex_unlock(&pmMUT);
   	 record = 0;	 
	 system("killall -9 wavRecorder");
	 system("wavplayer /www/music/recoder_test.wav");
	 return 0;
}*/

static pid_t get_pid_by_name(char *pidName)
{
    char filename[256];
    char name[128];
    struct dirent *next;
    FILE *file;
    DIR *dir;
    pid_t retval = -1;

    dir = opendir("/proc");
    if (!dir) {
        perror("Cannot open /proc\n");
        return 0;
    }

    while ((next = readdir(dir)) != NULL) {
        /* Must skip ".." since that is outside /proc */
        if (strcmp(next->d_name, "..") == 0)
            continue;

        /* If it isn't a number, we don't want it */
        if (!isdigit(*next->d_name))
            continue;

	  		memset(filename, 0, sizeof(filename));
        sprintf(filename, "/proc/%s/status", next->d_name);
        if (! (file = fopen(filename, "r")) )
            continue;

	  		memset(filename, 0, sizeof(filename));
        if (fgets(filename, sizeof(filename)-1, file) != NULL) {
            /* Buffer should contain a string like "Name:   binary_name" */
            sscanf(filename, "%*s %s", name);
						/* Notice: the max size of stat is 15 */
            if (!strcmp(name, pidName) ) {
                retval = strtol(next->d_name, NULL, 0);
            }
        }
        fclose(file);

        if (retval > 0) {
            break;
        }
    }
    closedir(dir);

    return retval;
}


#define BIG_ENDIAN 0
int vad(short samples[], int len)
{
  int i;
#if BIG_ENDIAN
  unsigned char *p;
  unsigned char temp;
#endif
  long long sum = 0, sum2 = 0;
  for(i = 0; i < len; i++) {
#if BIG_ENDIAN
	p = &samples[i];
	temp = p[0];
	p[0] = p[1];
	p[1] = temp;
#endif
	sum += (samples[i] * samples[i]);
	sum2 += (800000);
	//printf("%d\t%lld\t%d\n", samples[i], sum, i);
  }
  //printf("sum=%lld, sum2=%lld\n", sum, sum2);
  if (sum > sum2)
	  return 1;
  else
	  return 0;
}


// 判断录音文件是否有声音
static int IsVoice(void)
{
	int i = 0;
	int j = 0;
	int k = 0;
	char *pTemp = NULL;
	char *pData = NULL;
	int iLengt = 0;
	FILE * fp = NULL;
	char bystatus = 0;

    if(access("/tmp/test_mode",0))
    {
        return -1;  //no test signal
    }

	//unsigned long dwfile_size = get_file_size("/tmp/recoder_test.wav");

	//printf("dwfile_size:%d\n", dwfile_size);

	// 读取文件，采样三个位置的数据，如果有声音，则认为是录音成功。
	fp = fopen("/tmp/recoder_test.wav", "rb");
	if (NULL == pTemp)
		pTemp = malloc(4096);

	if (NULL == pData)
		pData = malloc(2048);

	while (1)
	{
		iLengt = fread(pTemp, 1, 4096, fp);
		// 因为是双声道的，需要转换为单声道
		for (i = 0; i < 1024; i++)
		{
			pData[j++] = pTemp[k++];
			pData[j++] = pTemp[k++];
			k += 2;
		}

		//printf("iLengt:%d\n", iLengt);
		if (iLengt > 0)
		{
			if (1 == vad(pData, iLengt/4))
			{
				printf("is voice..iLengt,:%d\n", iLengt);
				bystatus = 1;
				break;
			}
		}
		else
		{
			break;
		}
	}

	fclose(fp);

	if (pTemp)
		free(pTemp);

	if (pData)
		free(pData);

	if (1 == bystatus)
		return 1;
	else
		return 0;
}
int tlkoff_test()
{
    int ret = 0;
	if (record == 0)
	{
		return 0;	
	}
	DEBUG_INFO("tlkoff test ...");
	system("killall -9 wavRecorder");
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK+OFF&", strlen("AXX+TLK+OFF&"));
	pthread_mutex_unlock(&pmMUT);
	record = 0;	 
//	system("wavplayer /www/music/recoder_test.wav");
	return 0;
}
int tlkon_test()
{
	int CAPURE_TIMEOUT =20;
	int ret;
	if (record == 1)
	{
		printf("record == 1, return ..");
		return 1;
	}
	else
	{
		record = 1;
	}
	printf("tlkon_test ...\n");
	 if((access("/www/music/recoder_test.wav",F_OK))==0){
		printf("/www/music/recoder_test.wav is exist");	 
		system("rm /www/music/recoder_test.wav");
}
	 printf("/www/music/recoder_test.wav is not exist");	
	//system("mpc volume 101");
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));	
	pthread_mutex_unlock(&pmMUT);	
//	sleep(1);
	//system("arecord -f s16_le -c 2 -r 44100 /www/music/recoder_test.wav &");
//	system("killall wakeup;arecord -D plughw:1,0 -f s16_le -c 2 -r 16000 /www/music/recoder_test.wav &");
//  system("wavRecorder /www/music/recoder_test.wav &");

//	signal(SIGALRM,tlkoff_test);
//    alarm(CAPURE_TIMEOUT);
/*
	while(1){
			if(record ==0){
			   break;
			}else{
			 endTime = tv.tv_sec;
			if((endTime-startTime) >= CAPURE_TIMEOUT){
				tlkoff_test();
				break;
			   }
			}
			sleep(1);
		}
*/		
	return 0;
}


int tlkon_test_time()
{
    struct timeval tv; 
	gettimeofday(&tv,NULL); 
	long int startTime = 0;
	long int endTime = 0;
    int CAPURE_TIMEOUT =5;
	int ret;
	if (record == 1)
	{
		printf("record == 1, return ..");
		return 1;
	}
	else
	{
		record = 1;
	}
	printf("tlkon_test ...\n");
	system("rm /www/music/recoder_test.wav");
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));
	pthread_mutex_unlock(&pmMUT);	
	//system("arecord -f s16_le -c 2 -r 44100 -T 5000 /www/music/recoder_test.wav &");	
//	system("wavRecorder /www/music/recoder_test.wav &");	
//	signal(SIGALRM,tlkoff_test);
//    alarm(CAPURE_TIMEOUT);
	/*
	startTime = tv.tv_sec;
   
    while(1){
		if(record ==0){
           break;
		}else{
		 gettimeofday(&tv, NULL); 
	     endTime = tv.tv_sec;	
		 printf("endTime-startTime = %d \n",endTime-startTime);
	    if((endTime-startTime) >= CAPURE_TIMEOUT){
			tlkoff_test();
			break;
	       }
	    }
		sleep(1);
    }
   */ 
	return 0;
}


static void IsFactory(int mode)
{
    if(access("/tmp/test_mode",0))
    {
        return -1;  //no test signal
    }
	if (0 == mode)
		system("wavplayer /root/res/recode.wav &");
	else
		system("wavplayer /root/res/test_1KHz.wav &");
}
#if 0
int tlkon_test_time()
{
    struct timeval tv; 
	gettimeofday(&tv,NULL); 
	long int startTime = 0;
	long int endTime = 0;
    int CAPURE_TIMEOUT =5;
	int ret;
	if (record == 1)
	{
		printf("record == 1, return ..");
		return 1;
	}
	else
	{
		record = 1;
	}
	printf("tlkon_test time...\n");
	system("rm /www/music/recoder_test.wav");
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK++ON&", strlen("AXX+TLK++ON&"));
	pthread_mutex_unlock(&pmMUT);	
	IsFactory(1);
	system("killall wakeup;arecord -D plughw:1,0 -f s16_le -c 2  -d 5 -r 44100 /tmp/recoder_test.wav &");	
//	system("arecord  -f s16_le -c 2  -T 5 -r 44100 /tmp/recoder_test.wav &");	

	signal(SIGALRM,tlkoff_test);
	alarm(CAPURE_TIMEOUT);

	
	return 0;
}
#endif

int tlkoff()
{
    int ret = 0;
	if (record == 0)
	{
		//return 1;	
	}

	DEBUG_INFO("tlkoff...");
	
	pthread_mutex_lock(&pmMUT);
	write(iUartfd, "AXX+TLK+OFF&", strlen("AXX+TLK+OFF&"));
	pthread_mutex_unlock(&pmMUT);

	if((speech_or_iflytek ==1) || (speech_or_iflytek ==3))
	{
		 int value = 2;
		// char tempbuff[16];
		// memset(tempbuff, 0, sizeof(tempbuff)/sizeof(char));
		 
		// sprintf(tempbuff, "%d", value);
		 //ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.recordstatus", tempbuff);
		 cdb_set_int("$recordstatus",value);
  	}
  	record = 0; 
	return 0;
}

ssize_t whole_read(int fd, void *buf, size_t count)
{
	ssize_t n = 0, c = 0;
	size_t maxlen = count;	//maxlen=1024
/* BEGIN: Modified by Frog, 2018-7-25 09:46:18 */
//	char *tmp_buf[1024] = {0};
    char tmp_buf[1024] = {0};
/* END:   Modified by Frog, 2018-7-25 9:46:50 */
	do {
		memset(tmp_buf, 0, sizeof(tmp_buf));
		n = read(fd, tmp_buf, sizeof(tmp_buf));
		if(n > 0)
		{
		//	DEBUG_INFO("11111111111:%d,2222:%d\n",n,c);
			if(c+n >= maxlen)
			{
				break;
			}
			memcpy((char *)&buf[c], tmp_buf, n);
			c += n;
			usleep(20*1000);		//第一次接收和下一次接收的间隔时间
		}
		else
		{
			DEBUG_INFO("read data failed\n");
			break;
		}
	} while (1);

	return c;
}


#define UART_DATA_CHECK 1
#define DUREOS

void *uartd_thread(void *arg)
{
    int ret=0, maxfd=0, size=0, i=0;
	unsigned int count =0;
	char cmd_complete_flag = 0;
    char buf[1024]={0}, command[1024] = {0};
#ifdef DUREOS
	char *duerhead = NULL;
	int tmp_ducmdlen = 0;
	int duerlen = 0;
#endif
    fd_set rd;
//	struct timeval timeout;
//	timeout.tv_sec = 1;
//	timeout.tv_usec = 0;

	while(1)
    {
    #if 1
		FD_ZERO(&rd);
		FD_SET(iUartfd,&rd);
		maxfd = iUartfd+1;
		ret = select(maxfd,&rd,NULL,NULL,NULL);
    	switch(ret)
    	{
     	 	case -1:
	      			perror("select");
	      			break;
			case 0:
		      		perror("time is over!");
		      		break;
     	 	default:
     	 			if (1 == bt_flage)
					{
     	 				break;
     	 			}
					else
     	 			{
			      		if(FD_ISSET(iUartfd,&rd))   //uart_revecedata
			      		{
	#endif
							memset(buf, 0, sizeof(buf));		
					        ret = whole_read(iUartfd,buf,sizeof(buf));//MCU+PLY+PUS&
							if(ret > 0)
							{
								DEBUG_INFO("buf:%s ret=%d",buf, ret);
								
//								buf[ret] = 0;
								if(count + ret < 64)
								{
									for(i = 0; i < ret; i++)//13
									{
										command[count + i] = buf[i];	//将buf数据赋给command
#if UART_DATA_CHECK
										printf("%hhx ", buf[i]);	//打印出它的16进制
#endif
									}
#if UART_DATA_CHECK
									printf("\n");
#endif
								}
								else
								{			//接收数据错误
									printf("uartd receive command error\n");
									count = 0;
									memset(command, 0, sizeof(command));
									continue;
								}
								//以&判定接收到的命令是否完整
								for(i = 0; i < ret;)	//13
								{
									if((buf[i++] == '&') && (i +2 >= ret))		//判断以'&'结尾，是完整的数据
									{
										cmd_complete_flag = 1;
										count += i;
#if UART_DATA_CHECK
										printf("===============i = %d, ret = %d, count: %d===============\n", i, ret, count);//执行
#endif
//										DEBUG_INFO("count: %d, i: %d", count, i);
										break;
									} 
								}
								
#ifdef DUREOS
								if(!cmd_complete_flag)		
								{
									tmp_ducmdlen = count + ret;
								}
								else
								{
									tmp_ducmdlen = count;//12
								}
								duerhead = memmem(command, tmp_ducmdlen, "MCU+DUE", 7);
								if(duerhead != NULL){
									if(tmp_ducmdlen > 9)
									{		// 9 == strlen("MCU+DUE+&")
										if(command[tmp_ducmdlen -1] == '&')
										{
									#if 1
											printf("duer data is OK\n");
									#else
											pthread_mutex_lock(&pmMUT);
											write(iUartfd, "01", 2);
											pthread_mutex_unlock(&pmMUT);
									#endif
										}
									else
										{
									#if 0
											pthread_mutex_lock(&pmMUT);
											write(iUartfd, "00", 2);
											pthread_mutex_unlock(&pmMUT);
									#endif
//											send_button_direction_to_dueros(1, KEY_ONE_LONG);
											printf("uartd receive duer data from bt error\n");
											count = 0;
											memset(command, 0, sizeof(command));
											continue;
										}
									}
								}
#endif
								if(cmd_complete_flag)	//命令接收完成的标志
								{	
									#if 0
									pthread_mutex_lock(&pmMUT);
//									write(iUartfd, "1111", 4);
									printf("cmd complete\n");
									pthread_mutex_unlock(&pmMUT);
									#endif
									pthread_mutex_lock(&mtx);  
								#if 0
									for(i = 0; i < count; i++){
										printf("%hhx ", command[i]);
									}
									printf("\n");
								#endif
									ft_fifo_put(command_fifo,command,count);
									DEBUG_INFO("instruction finish len: %d, cond : %d", count, uartd_pthread_cond_wait);//执行
									if(uartd_pthread_cond_wait)
									{
										pthread_cond_signal(&cond);  
										DEBUG_INFO("ret:%d, pthread_cond_signal", ret);
									}
									pthread_mutex_unlock(&mtx); 
									//发送完一个完整的命令，清空命令拼接区域
									memset(command, 0, sizeof(command));
									cmd_complete_flag = 0;
									count = 0;
								}
								else
								{
									DEBUG_INFO("count: %d, ret: %d", count, ret);
									count += ret;
									#if 0
									if(count > 20){										
										pthread_mutex_lock(&pmMUT);
//										write(iUartfd, "0000", 4);
										pthread_mutex_unlock(&pmMUT);
										count = 0;
										memset(command, 0, sizeof(command));
										printf("receive data from bt ERROR\n");
									}
									#endif
								}
							}
							
						}
						usleep(10*1000);
						break;
					}
         }
    }
	close(iUartfd);
}


