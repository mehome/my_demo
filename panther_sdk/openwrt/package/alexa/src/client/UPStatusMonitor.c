#include "globalParam.h"
#include "clientfd_set.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>  
#include <sys/un.h> 
#include <netinet/in.h>
#include "alert/AlertHead.h"

//#define ENABLE_NTP_LOG 
#define FIFO_BUFF_LENGTH 512

#ifdef ENABLE_NTP_LOG
int ntpSuccess  = 0;
#endif
extern 	char g_byWakeUpFlag;
pthread_t StatusMonitor_Pthread;
extern GLOBALPRARM_STRU g;
extern FT_FIFO * g_DecodeAudioFifo;
extern FT_FIFO * g_PlayAudioFifo;
extern FT_FIFO * g_CaptureFifo;
extern AlertManager *alertManager;

extern char g_playFlag;

int OnWriteMsgToWakeup(char* cmd)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	#define UNIX_WAKEUP "/tmp/UNIX.wakeup"
	
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
        }
    }

    return 0;
}


static int OnProcessSignin(int fd, char *pData)
{
	int iRet = -1;
	struct json_object *root, *authorizationCodeObj, *redirectUriObj, *clientIdObj, *codeVerifierObj;
	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		DEBUG_PRINTF("json_tokener_parse root failed");
		return iRet;
	}

	authorizationCodeObj = json_object_object_get(root, "authorizationCode");
	redirectUriObj = json_object_object_get(root, "redirectUri");
	clientIdObj = json_object_object_get(root, "clientId");
	codeVerifierObj = json_object_object_get(root, "codeVerifier");

	char *codeVerifier = strdup(json_object_get_string(codeVerifierObj));
	char *authorizationCode = strdup(json_object_get_string(authorizationCodeObj));
	char *redirectUri = strdup(json_object_get_string(redirectUriObj));
	char *clientId = strdup(json_object_get_string(clientIdObj));

	DEBUG_PRINTF("[codeVerifier     ]:%s", codeVerifier);
	DEBUG_PRINTF("[authorizationCode]:%s", authorizationCode);
	DEBUG_PRINTF("[redirectUri      ]:%s", redirectUri);
	DEBUG_PRINTF("[clientId         ]:%s", clientId);

	json_object_put(root);

	OnSetAuthDelegateParam(codeVerifier, authorizationCode, redirectUri, clientId);

	iRet = OnAuthDelegate();

	extern AUTHDELEGATE_STRUCT *g_AuthDelegateParam;
	OnFreeAuthDelegateParam(g_AuthDelegateParam);

	return iRet;
}

//Alexa sign up needs three steps: status = 0, reset AlexaClientConfig.json , delete alarm file
static int OnProcessSignUp(int fd, char *pData)
{
	int iRet = -1;
	
	/*********these points need to be freed***********/
	char *pvalue = NULL, *alexa_conf_data = NULL;
	struct json_object *root = NULL, *value = NULL;
	/*********these points need to be freed***********/

	//step 1:
	SetAmazonLogInStatus(0);

	//step 2:
	remove(ALARM_FILE);

	//step 3:
	alexa_conf_data = getConfigJsonData(CONFIG_PATH);
	if(!alexa_conf_data){
		printf("can't get config data\n");
		return -1;
	}
	if(strstr(alexa_conf_data, "productId")){
		char init_alexa_conf_data[512] = {0};
		root = json_tokener_parse(alexa_conf_data);	
		if (is_error(root)) 
		{
			printf("json_tokener_parse root failed");
			return NULL;
		}
		value = json_object_object_get(root, "productId");
		pvalue = strdup(json_object_get_string(value));
		sprintf(init_alexa_conf_data,  "{\"productId\":\"%s\",\"clientId\":\"\",\"locale\":\"en-US\",\"endpoint\":\"https://avs-alexa-na.amazon.com\",\"vad_Threshold\":1000000,\"Notification_Enable\":0,\"Notification_VisualIndicatorPersisted\":0,\"displayCardsSupported\":false,\"accessToken\":\"\",\"refreshToken\":\"\"}",	pvalue);
		iRet = writeConfigJsonData(init_alexa_conf_data);
		
		if(value){
			json_object_put(value);
		}		
		if(root){
			json_object_put(root);
		}		
		if(pvalue){
			free(pvalue);
		}
	}else{
		printf("alexa config file error\n");
	}

	if(alexa_conf_data){
		free(alexa_conf_data);
	}
	return iRet;
}

static int OnProcessStartEngine(int fd, char *pData)
{
	g.m_byAudioRequestFlag = AUDIO_REQUEST_START;

	SetVADFlag(RECORD_VAD_END);
	sem_post(&g.Semaphore.Capture_sem);
	return 0;
}
/* 
 * 按键处理
 */
static int OnProcessButtonEvent(int fd, char *pData)
{
	int iRet = 0;
	struct json_object *root, *button;
	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		printf("json_tokener_parse root failed");
		return iRet;
	}
	button = json_object_object_get(root, "buttonEvent");
	char *exent = strdup(json_object_get_string(button));
	json_object_put(root);

	// 播放暂停按键
	if (0 == strcmp(exent, "MpcPlayEvent"))
	{
		Queue_Put(ALEXA_EVENT_MPCPLAYSTOP);
	}
	// Mute按键
	else if (0 == strcmp(exent, "Mute"))
	{
		SetMuteFlag(UMUTE_STATUS);
		sem_post(&g.Semaphore.WakeUp_sem);
	}
	// UnMute按键
	else if (0 == strcmp(exent, "UnMute"))
	{
		SetMuteFlag(MUTE_STATUS);
	}
	// 上一曲按键
	else if (0 == strcmp(exent, "MpcPrevEvent"))
	{
		Queue_Put(ALEXA_EVENT_MPCPREV);
	}
	// 下一曲按键
	else if (0 == strcmp(exent, "MpcNextEvent"))
	{
		Queue_Put(ALEXA_EVENT_MPCNEXT);
	}
	// 声音加减按键
	else if ((0 == strcmp(exent, "VolumeUp")) || (0 == strcmp(exent, "VolumeDown")))
	{
		Queue_Put(ALEXA_EVENT_VOLUME_CHANGED);
	}
	// 停止闹钟按键
	else if (0 == strcmp(exent, "KeyStopAler"))
	{
		alertmanager_stop_active_alert(alertManager);
		OnWriteMsgToUartd(ALEXA_ALER_END);
		MpdVolume(200);	
	}
	// keyevent，按下所以按键取消闹铃
	else if (0 == strcmp(exent, "KeyEvent"))
	{
		alerthandler_set_state(alertmanager_get_alerthandler(alertManager), ALERT_FINISHED);
	}
	else
	{
		DEBUG_PRINTF("Unsupported keys");
		iRet = -1;
	}

	return iRet;
}

/* 需要完善代码*/
static int OnProcessSetLanguage(int fd, char *pData)
{
	Queue_Put(ALEXA_EVENT_SETTING);
	return 0;
}

static int OnProcessGetTimerStatus(int fd, char *pData)
{
	if (alerthandler_get_state(alertManager->handler) != ALERT_FINISHED)
	{
		OnWriteMsgToUartd(ALEXA_ALER_START);
	}
	return 0;
}

static int OnProcessNtpSucceed(int fd, char *pData)
{
#ifdef ENABLE_NTP_LOG
	ntpSuccess = 1;
	exec_cmd("echo \"NtpClienSucceed\" > /tmp/ntp.log");
	struct tm *timenow; 
	time_t timep;
	char buf[1024] = {0};

	time(&timep);
	timenow = localtime(&timep);	
	char *str = asctime(timenow);
	str[strlen(str)-1] = 0;
	memset(buf, 0, 1024);
	snprintf(buf, 1024, "echo \"now:<%s> :NtpClienSucceed\"  >> /tmp/ntp.log " , str);
	exec_cmd(buf);
	memset(buf, 0, 1024);
	snprintf(buf, 1024, "echo \"now:<%s> :NtpClienSucceed\"  >> /tmp/alert.txt ",  str);
	exec_cmd(buf);
#endif					
	Alert_Reload();

	return 0;
}

static int OnProcessPowerOff(int fd, char *pData)
{
	Queue_Put(ALEXA_EVENT_PLAYBACK_STOP);
	return 0;
}

static int OnProcessSigninStatus(int fd, char *pData)
{
	int iRet = -1;
//	char str[4] = {0};
	struct json_object *retData = json_object_new_object();
	// 获取当前登陆状态，将登陆状态返回
	char tmp_amazonlogin_status = GetAmazonLogInStatus();
//	sprintf(str, "%c", tmp_amazonlogin_status + 48);
	
//	json_object_object_add(retData, "command", json_object_new_string("GetSigninStatus"));
	json_object_object_add(retData, "amazonLogInStatus", json_object_new_int(tmp_amazonlogin_status));
	
	char *pJsonData = strdup(json_object_get_string(retData));
	json_object_put(retData);
	
	iRet = send(fd, pJsonData, strlen(pJsonData), MSG_NOSIGNAL);
	if(iRet != strlen(pJsonData)){
		printf("iRet: %d, len: %d, write error!\n", iRet, strlen(pJsonData));
	}
	if(pJsonData){
		free(pJsonData);
	}
	
	return iRet;
}


static  data_proc_t cmd_tbl[] = {
	{"SignIn", OnProcessSignin},
	{"SignUp", OnProcessSignUp},
	{"StartEngine", OnProcessStartEngine},
	{"ButtonEvent", OnProcessButtonEvent},
	{"SetLanguage", OnProcessSetLanguage},
	{"GetTimerStatus", OnProcessGetTimerStatus},
	{"NtpClienSucceed", OnProcessNtpSucceed},
	{"PowerOff", OnProcessPowerOff},
	{"GetSigninStatus", OnProcessSigninStatus}
};

static int OnProcessJsonData(int fd, char *pData)
{
	int i = 0;
	int iRet = -1;

	struct json_object *root, *cmd;
	root = json_tokener_parse(pData);
	if (is_error(root)) 
	{
		printf("json_tokener_parse root failed\n");
		return iRet;
	}
	cmd = json_object_object_get(root, "command");
	char *command = strdup(json_object_get_string(cmd));
	json_object_put(root);

	if (command) 
	{
		printf("command: %s\n", command);
		int iLength = sizeof(cmd_tbl) / sizeof(data_proc_t);
		for (i = 0; i < iLength; i++) 
		{
			if (0 == strncmp(command, cmd_tbl[i].name, strlen(cmd_tbl[i].name))) 
			{
				if (cmd_tbl[i].exec_proc != NULL) 
				{
					iRet = cmd_tbl[i].exec_proc(fd, pData);
				}
				break;
			}
		}
	}

	if (command)
	{
		free(command);
	}

	return iRet;
}


int SocketDataHandle(int listen_fd)
{
	struct timeval tv;	
	struct sockaddr_in clientAddr;
	int clientfd = 0;  
  	int nRet = 0;
	
	clientfds *cli_fds = (clientfds *)malloc(sizeof(clientfds));
	clientfds_init(cli_fds);
	char byReadBuf[FIFO_BUFF_LENGTH] = {0};
	int iRecvCount = 0;
	unsigned long dwLen = 0;
	while(1)  
	{  
//		printf("connecting.........\n");
		clientfds_add_to_listen(cli_fds);
		FD_SET(listen_fd, &cli_fds->cli_fdset);  
		tv.tv_sec=10;	// select超时时间  
		tv.tv_usec=0;  
		cli_fds->maxfd = (cli_fds->maxfd > listen_fd ? cli_fds->maxfd : listen_fd);
		nRet=select(cli_fds->maxfd+1, &cli_fds->cli_fdset, NULL, NULL, &tv);	
		if(nRet > 0)  
		{	
			if(FD_ISSET(listen_fd, &cli_fds->cli_fdset))	
			{	 
				dwLen = sizeof(struct sockaddr_in);
				clientfd = accept(listen_fd, (struct sockaddr*)&clientAddr, (socklen_t *)&dwLen);	
				if(clientfd > 0){
					clientfds_add(clientfd, cli_fds);
				}else{
					printf("%s\n",strerror(errno));
				}
//				printf("accept fd2: %d\n", clientfd);
			}else{	
				clientfd = clientfds_FD_ISSET(cli_fds);
//				printf("accept fd1: %d\n", clientfd);
				if(clientfd < 0){
					continue;
				}
				
				iRecvCount = 0;
				memset(byReadBuf, 0, FIFO_BUFF_LENGTH);
				iRecvCount = recv(clientfd, (char *)byReadBuf, FIFO_BUFF_LENGTH, MSG_DONTWAIT);
				printf("recv:%s\n", byReadBuf);
				OnProcessJsonData(clientfd, byReadBuf);
				clientfds_delete(clientfd, cli_fds);
				close(clientfd);  
			}	
		}	
	}  
	free(cli_fds);
	return 0;  
}




void *StatusMonitorPthread(char *ppath)
{
	int iCount = 0;
	int iRet = -1;
	int iSocket = -1;
    int iConnect;
    int iRecvLen = 0;

	char byReadBuf[FIFO_BUFF_LENGTH] = {0};

	int maxfd = 0;

	iSocket = OnCreateSocketServer(UNIX_DOMAIN);

	SocketDataHandle(iSocket);

	return;
}

void CreateUPStatusMonitorPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN);

	iRet = pthread_create(&StatusMonitor_Pthread, &a_thread_attr, StatusMonitorPthread, NULL);
	if(iRet != 0)
	{
		DEBUG_PRINTF("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}


