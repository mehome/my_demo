#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>  
#include <sys/un.h> 
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <mpd/client.h>
#include "uciConfig.h"
#include <assert.h>

#define DEBUG_ON 0

#if DEBUG_ON
	#define DEBUG_PRINTF(fmt, args...) do { \
		printf("Client:[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); } while(0) 
#else
	#define DEBUG_PRINTF(fmt, args...)
#endif

#define UNIX_WAKEUP "/tmp/UNIX.wakeup"
#define FIFO_BUFF_LENGTH 64

pthread_t StatusMonitor_Pthread;
sem_t sem;
int wakeup_flag = 0;
int record_flag = 1;

#define V_QUIET 0
#define V_DEFAULT 1
#define V_VERBOSE 2

struct WAV_HEADER{
	char rld[4];    		   // riff ±ê??·?o?
	int rLen;
	char wld[4];    		   // ??ê?ààDí￡¨wave￡?    
	char fld[4];               // "fmt"    
	int fLen;            	   // sizeof(wave format matex)    
	short wFormatTag;   	   // ±à????ê?    
	short wChannels;    	   // éùμàêy    
	int   nSamplesPersec ;   // 2é?ù?μ?ê    
	int   nAvgBitsPerSample; // WAVE???t2é?ù′óD?    
	short  wBlockAlign; 	   // ?é????    
	short wBitsPerSample;    // WAVE???t2é?ù′óD?    
	char dld[4];        	   // ?±data?°    
	int wSampleLength;  	   // ò??μêy?Yμ?′óD?
} wav_header;

typedef struct {
	const char *host;
	const char *port_str;
	int port;
	const char *password;
	const char *format;
	int verbosity; // 0 for quiet, 1 for default, 2 for verbose
	bool wait;

	bool custom_format;
} options_t;

options_t options = {
	.verbosity = V_DEFAULT,
	.password = NULL,
	.port_str = NULL,
	.format = "[%name%: &[%artist% - ]%title%]|%name%|[%artist% - ]%title%|%file%",
};

int printErrorAndExit(struct mpd_connection *conn)
{
	assert(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS);

	const char *message = mpd_connection_get_error_message(conn);
	if (mpd_connection_get_error(conn) == MPD_ERROR_SERVER)
		/* messages received from the server are UTF-8; the
		   rest is either US-ASCII or locale */
		//message = charset_from_utf8(message);

	fprintf(stderr, "alexa-> mpd error: %s\n", message);
	mpd_connection_free(conn);

	return -1;
}

void
send_password(const char *password, struct mpd_connection *conn)
{
	if (!mpd_run_password(conn, password))
		printErrorAndExit(conn);
}

struct mpd_connection *
setup_connection(void)
{
	struct mpd_connection *conn = mpd_connection_new(options.host, options.port, 0);
	if (conn == NULL) {
		fputs("Out of memory\n", stderr);
		//exit(EXIT_FAILURE);
		return NULL;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
		if (0 != printErrorAndExit(conn))
			return NULL;

	return conn;
}




int OnCreateSocketServer(char *ppath)
{
	int listen_fd;
	int ret = -1;

	struct sockaddr_un srv_addr;

	unlink(ppath);

	listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		DEBUG_PRINTF("cannot create communication socket");
		return -1;
	}  

	//set server addr_param
	srv_addr.sun_family = AF_UNIX;
	strncpy(srv_addr.sun_path, ppath, sizeof(srv_addr.sun_path)-1);

	ret = bind(listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if(ret < 0)
	{
		DEBUG_PRINTF("cannot bind server socket");
		close(listen_fd);
		unlink(ppath);
		return -1;
	}

	ret = listen(listen_fd, 4);
	if(ret < 0)
	{
		DEBUG_PRINTF("cannot listen the client connect request");
		close(listen_fd);
		unlink(ppath);
		return 1;
	}

	return listen_fd;
}

/*
int SocketAccept(int iSocket)
{
    int iConnectSocket = -1;

    struct sockaddr_in ClientAddr;

    struct timeval sTimeout;

    fd_set socketSet;

    unsigned long dwLen = 0;

    // socket ��Ч
    if (iSocket <= 0)
    {
        DEBUG_PRINTF("error: socket invalid error!!!\r\n");

        return -1;
    }

    memset(&ClientAddr, 0, sizeof(ClientAddr));

    // select
    sTimeout.tv_sec  = 0;
    sTimeout.tv_usec = 1000 * 100;

    FD_ZERO(&socketSet);
    FD_SET(iSocket, &socketSet);

    // select
    if (select(iSocket + 1, (fd_set*)&socketSet, (fd_set*)NULL, (fd_set*)NULL, &sTimeout ) <= 0)
    {
        return -2;
    }
    else
    {
        // accept
        dwLen = sizeof(struct sockaddr_in);

        iConnectSocket = accept(iSocket, (struct sockaddr *)&ClientAddr, (socklen_t *)&dwLen);
    }

    return iConnectSocket;
}


int SocketRecvNoWait(int iSocket, unsigned char * pByBuf, int iLen, int * piRecvLen)
{
    struct timeval sTimeout;

    fd_set socketSet;

    int iRecvCount = 0;

    // socket ��Ч
    if (iSocket <= 0)
    {
        DEBUG_PRINTF("socket invalid error!!!\r\n");

        return -1;
    }

    sTimeout.tv_sec  = 0;
    sTimeout.tv_usec = 1000 * 100;

    FD_ZERO(&socketSet);
    FD_SET(iSocket, &socketSet);

    // select
    if (select(iSocket + 1, (fd_set*)&socketSet, (fd_set*)NULL, (fd_set*)NULL, &sTimeout ) <= 0)
    {
        // Error or Timeout
        return -2;
    }
    else
    {
        if (FD_ISSET(iSocket, &socketSet) > 0)
        {
            // Receive data from the agent.
            iRecvCount = read(iSocket, (char *)pByBuf, iLen);
            if (0 < iRecvCount)
            {
                *piRecvLen = iRecvCount;
            }
			close(iSocket);
        }
    }

    return 0;
}

#define START "start"
void *StatusMonitorPthread(void *ppath)
{
	int iCount = 0;
	int iRet = -1;
	int iSocket = -1;
    int iConnect;
    int iRecvLen = 0;
	char byReadBuf[FIFO_BUFF_LENGTH] = {0};

	int maxfd = 0;

	iSocket = OnCreateSocketServer(UNIX_WAKEUP);

 	while (1)
	{
		iConnect = SocketAccept(iSocket);
		
		if (iConnect >= 0)
		{
			memset(byReadBuf, 0, FIFO_BUFF_LENGTH);
			//iRet = SocketRecvNoWait(iConnect, byReadBuf, FIFO_BUFF_LENGTH, &iRecvLen);
			if (0 == iRet)
			{
				printf("\033[1;32;40m[MSG:%s\033[0m]\n", byReadBuf);
				if (0 == strncmp(byReadBuf, START, sizeof(START) - 1))
				{
					
				}
			}
		}
	}
}*/

#define START "start"
#define RESTART "restart"
#define STARTENGINE "StartEngine"

void *StatusMonitorPthread(void *ppath)
{
	int iCount = 0;
	int iRet = -1;
	int iSocket = -1;
    int iConnect;
    int iRecvLen = 0;
	char byReadBuf[FIFO_BUFF_LENGTH] = {0};
	int val;

	int maxfd = 0;

    struct sockaddr_in ClientAddr;
    int iLen = sizeof(ClientAddr);

	iSocket = OnCreateSocketServer(UNIX_WAKEUP);

 	while (1)
	{
        iConnect = accept(iSocket, (struct sockaddr *)&ClientAddr, (socklen_t *)&iLen);
		
		if (iConnect >= 0)
		{
			memset(byReadBuf, 0, sizeof(byReadBuf));
			//iRet = SocketRecvNoWait(iConnect, byReadBuf, FIFO_BUFF_LENGTH, &iRecvLen);
            iRecvLen = read(iConnect, (char *)byReadBuf, sizeof(byReadBuf));
			
			if (0 <= iRecvLen)
			{
				DEBUG_PRINTF("\033[1;32;40m[MSG:%s\033[0m]\n", byReadBuf);
				if (0 == strncmp(byReadBuf, START, sizeof(START) - 1))
				{
					if (wakeup_flag != 0)
					{
						sem_post(&sem);
					}
					else if (record_flag != 1)
					{
						startRecord();
						record_flag = 1;
					}
				}
				else if (0 == strncmp(byReadBuf, RESTART, sizeof(RESTART) - 1))
				{
					record_flag = 0;
					//setupAlexa();
				}
				else if (0 == strncmp(byReadBuf, STARTENGINE, sizeof(STARTENGINE) - 1))
				{
					record_flag = 0;
					//stopRecord();
					//setupAlexa();
					//OnWriteMsgToAlexa("StartEngine");
					//exit(0);
				}
			}
		}
	}
}
void sigint(int sig)
{
	DEBUG_PRINTF("sigint\n");
	//sem_post(&sem);
	OnWriteMsgToWakeup(START);
}
void sigusr1(int sig)
{
	DEBUG_PRINTF("sigusr1\n");
	stopRecord();
}
void sigusr2(int sig)
{
	DEBUG_PRINTF("sigusr2\n");
	startRecord();
}
void CreateMsgPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
#if defined(__PANTHER__)
	pthread_attr_setstacksize(&a_thread_attr, 65536);
#else
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN * 10);
#endif

	iRet = pthread_create(&StatusMonitor_Pthread, &a_thread_attr, StatusMonitorPthread, NULL);
	if(iRet != 0)
	{
		DEBUG_PRINTF("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
	iRet = sem_init(&sem, 0, 0);
	if(iRet != 0)
	{
		DEBUG_PRINTF("sem_init error:%s\n", strerror(iRet));
		exit(-1);
	}
	//signal(SIGINT, sigint);
	signal(SIGUSR1, sigusr1);
	signal(SIGUSR2, sigusr2);
}

#define UNIX_DOMAIN "/tmp/UNIX.domain"

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
		DEBUG_PRINTF("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		DEBUG_PRINTF("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		DEBUG_PRINTF("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

int OnWriteMsgToWakeup(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_WAKEUP);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{
		DEBUG_PRINTF("\033[1;32;40mcannot create communication socket. \033[0m\n");
		return 1;
	}

    flags = fcntl(iConnect_fd, F_GETFL, 0);
    fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		DEBUG_PRINTF("\033[1;32;40mconnect server failed..\033[0m\n");
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));
	if (iRet != strlen(cmd))
	{
		DEBUG_PRINTF("\033[1;32;40mwrite failed..\033[0m\n");
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

static int speaker_not_wifi(void)
{
	system("uartdfifo.sh WiFiNotConnected");
	int ret1 = system("aplay /tmp/res/speaker_not_connected.wav");
	
	return 1;
}

static int amazon_unsuccessful(void)
{
	system("uartdfifo.sh AlexaNotLogin");
	int ret1 = system("aplay /tmp/res/amazon_unsuccessful.wav");
	return 1;
}
extern struct mpd_connection *setup_connection(void);
int setupAlexa()
{
	struct mpd_connection *conn = NULL;
	char *getRecordStatus;
	int status = 0;
	int ret;

	DEBUG_PRINTF("Set MPD volume start.");
	// ����mpd����������û�п���mpd�ҵ���߿��������,���Գ�ʹ�ô˺�����������Ļ�ֻ��Ҫ11ms
	conn = setup_connection();
	if (conn != NULL) {
		if (!mpd_run_set_volume(conn, 101))
			printErrorAndExit(conn);
		else
			mpd_connection_free(conn);
	}
	conn = NULL;
	DEBUG_PRINTF("Set MPD volume end.");

	// ����Ƿ������ɹ�
	getRecordStatus = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.wificonnectstatus");
	if (getRecordStatus == NULL)
	{
		ret = speaker_not_wifi();
		system("mpc volume 200");
		return ret;
	}
	else
	{
		status = atoi(getRecordStatus); 
		free(getRecordStatus);
		getRecordStatus = NULL;
		if (3 == status)
		{
			status = 0;
			// ����Ƿ����token
    	 	getRecordStatus = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.amazon_loginStatus");
			if (getRecordStatus != NULL)
			{
				int iVluae = atoi(getRecordStatus);
				//printf("getRecordStatus length:%d\n", iLen);
				if (0 == iVluae)
				{
					// ����������ʾ����ѷδ��¼
					free(getRecordStatus);
					ret = amazon_unsuccessful();
					system("mpc volume 200");
					return ret;
				}
			}
		}
		else
		{
			// û�������ɹ������������speaker_not_wifi
			ret = speaker_not_wifi();
			system("mpc volume 200");
			return ret;
		}
		ret = WIFIAudio_UciConfig_SearchAndSetValueString("xzxwifiaudio.config.recordstatus", "1");
		DEBUG_PRINTF("Start Send MSG to Alexa.");
		//system("aplay /root/res/alexa_start.wav");
		if (OnWriteMsgToAlexa("StartEngine_wakeup") == 0)
		{
			return 0;
		}
		else// ����˵������ѷû������
		{
			system("aplay /root/res/ring2.wav");
			system("mpc volume 200");
			return 1;
		}
	}
}
