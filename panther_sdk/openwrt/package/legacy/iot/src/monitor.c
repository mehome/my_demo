

#include "monitor.h"


#include <sys/socket.h>  
#include <sys/un.h> 
#include <fcntl.h>

#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>

static pthread_t monitorPthread = 0;
static pthread_t keyPthread = 0;
#define FIFO_BUFF_LENGTH 256


int write_msg_to_wakeup(char* cmd)
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

int creat_socket_server(char *ppath)
{
	int listen_fd;
	int ret = -1;

	struct sockaddr_un srv_addr;

	unlink(ppath);

	listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		error("cannot create communication socket");
		return -1;
	}  

	//set server addr_param
	srv_addr.sun_family = AF_UNIX;
	strncpy(srv_addr.sun_path, ppath, sizeof(srv_addr.sun_path)-1);

	ret = bind(listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if(ret < 0)
	{
		error("cannot bind server socket");
		close(listen_fd);
		unlink(ppath);
		return -1;
	}

	ret = listen(listen_fd, 4);
	if(ret < 0)
	{
		error("cannot listen the client connect request");
		close(listen_fd);
		unlink(ppath);
		return 1;
	}

	return listen_fd;
}

#if 0
int socket_accept(int iSocket)
{
    int iConnectSocket = -1;

    struct sockaddr_in ClientAddr;

    struct timeval sTimeout;

    fd_set socketSet;

    unsigned long dwLen = 0;

    // socket
    if (iSocket <= 0)
    {
        error("error: socket invalid error!");

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
#else

int socket_accept(int iSocket)
{
    int iConnectSocket = -1;

    struct sockaddr_in ClientAddr;

    struct timeval sTimeout;

    fd_set socketSet;

    unsigned long dwLen = 0;

    // socket
    if (iSocket <= 0)
    {
        error("error: socket invalid error!");

        return -1;
    }

    memset(&ClientAddr, 0, sizeof(ClientAddr));


    dwLen = sizeof(struct sockaddr_in);

    iConnectSocket = accept(iSocket, (struct sockaddr *)&ClientAddr, (socklen_t *)&dwLen);

    return iConnectSocket;
}

#endif

int socket_recv_nowait(int iSocket, unsigned char * pByBuf, int iLen, int * piRecvLen)
{
    struct timeval sTimeout;

    fd_set socketSet;

    int iRecvCount = 0;

    // socket
    if (iSocket <= 0)
    {
        error("socket invalid error!!!\r\n");
		
        return -1;
    }

    //sTimeout.tv_sec  = 0;
    //sTimeout.tv_usec = 1000 * 100;

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

void *MonitorPthread(char *ppath)
{

	int iCount = 0;
	int iRet = -1;
	int turingSocket = -1;
#ifdef ENABLE_HUABEI
	int huabeiSocket = -1;
#endif
    int iConnect;
    int iRecvLen = 0;
	char byReadBuf[FIFO_BUFF_LENGTH] = {0};
	
    struct timeval sTimeout;
    fd_set socketSet;

	int maxfd = 0;
	
	turingSocket = creat_socket_server(TURING_DOMAIN);
#ifdef ENABLE_HUABEI
	huabeiSocket = creat_socket_server(HUABEI_DOMAIN);
#endif
 	while (1)
	{

	    // select
	    sTimeout.tv_sec  = 0;
	    sTimeout.tv_usec = 1000 * 100;

	    FD_ZERO(&socketSet);
	    FD_SET(turingSocket, &socketSet);
#ifdef ENABLE_HUABEI
		FD_SET(huabeiSocket, &socketSet);
#endif
		maxfd = turingSocket;
#ifdef ENABLE_HUABEI		
		maxfd = maxfd > huabeiSocket ? maxfd : huabeiSocket;
#endif
		maxfd += 1;
		memset(byReadBuf, 0, FIFO_BUFF_LENGTH);
		if (select(maxfd, (fd_set*)&socketSet, (fd_set*)NULL, (fd_set*)NULL, &sTimeout) <= 0)
	    {
	        continue;
	    } 
		else if (FD_ISSET(turingSocket, &socketSet)) 
		{
			
			iConnect = socket_accept(turingSocket);
			if (iConnect >= 0)
			{
				
				iRet = socket_recv_nowait(iConnect, byReadBuf, FIFO_BUFF_LENGTH, &iRecvLen);
				byReadBuf[iRecvLen]=0;
				if(iRet == 0) 
				{
					info("iot_socket_finished befor......................");
				    cdb_set_int("$iot_socket_finished",0);
					TuringMonitorControl(byReadBuf);
                    cdb_set_int("$iot_socket_finished",1);
					info("iot_socket_finished after......................");
				}
			}
		} 
#ifdef ENABLE_HUABEI
		else if (FD_ISSET(huabeiSocket, &socketSet)) 
		{
			iConnect = socket_accept(huabeiSocket);
			if (iConnect >= 0) 
			{
				iRet = socket_recv_nowait(iConnect, byReadBuf, FIFO_BUFF_LENGTH, &iRecvLen);
				byReadBuf[iRecvLen]=0;
				if(iRet == 0) 
				{
					HuabeiMonitorControl(byReadBuf);
				}
			}
		}
#endif
		usleep(1000 * 200);
	}   
}


void    CreateMonitorPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);


	iRet = pthread_create(&monitorPthread, &a_thread_attr, MonitorPthread, NULL);
	//iRet = pthread_create(&monitorPthread, NULL, MonitorPthread, NULL);
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}
	
}
void *TuringKeyPthread(void *arg)
{
	int iCount = 0;
	int iRet = -1;
	int turingSocket = -1;
	int iConnect;
	int iRecvLen = 0;
	char byReadBuf[FIFO_BUFF_LENGTH] = {0};
	
	struct timeval sTimeout;
	fd_set socketSet;
	int maxfd = 0;
	turingSocket = creat_socket_server(TURINGKEYCONTROL_DOMAIN);
	while (1)
	{
			// select
			sTimeout.tv_sec  = 0;
			sTimeout.tv_usec = 1000 * 100;
	
			FD_ZERO(&socketSet);
			FD_SET(turingSocket, &socketSet);
			maxfd = turingSocket;
			maxfd += 1;
			memset(byReadBuf, 0, FIFO_BUFF_LENGTH);
			if (select(maxfd, (fd_set*)&socketSet, (fd_set*)NULL, (fd_set*)NULL, &sTimeout) <= 0)
			{
				continue;
			} 
			else if (FD_ISSET(turingSocket, &socketSet)) 
			{
				
				iConnect = socket_accept(turingSocket);	//成功返回值 >0
				if (iConnect >= 0)
				{
					iRet = socket_recv_nowait(iConnect, byReadBuf, FIFO_BUFF_LENGTH, &iRecvLen);
					byReadBuf[iRecvLen]=0;
					if(iRet == 0) 
					{
				//		printf("11111111111111111111111111\n");
						cdb_set_int("$iot_turingkey_finished",0);
						TuringKeyControl(byReadBuf);
						cdb_set_int("$iot_turingkey_finished",1);
				//		printf("22222222222222222222222222\n");
					}
				}
			} 
			usleep(1000 * 200);
		}	
}

void CreateKeyControlPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*4);

	iRet = pthread_create(&keyPthread,&a_thread_attr,TuringKeyPthread,NULL);
	if(iRet != 0)
	{
		error("CreateKeyControlPthread error:%s", strerror(iRet));
		exit(-1);
	}
}

int   DestoryKeyControlPthread(void)
{
	if (keyPthread != 0)
	{
		pthread_join(keyPthread, NULL);
		info("DestoryKeyControlPthread end...\n");
	}	
}


int   DestoryMonitorPthread(void)
{
	if (monitorPthread != 0)
	{
		pthread_join(monitorPthread, NULL);
		info("capture_destroy end...\n");
	}	
}



