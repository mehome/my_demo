#include "commsat.h"

#include <sys/socket.h>  
#include <sys/un.h> 
#include <fcntl.h>

#include <netinet/in.h>

#define COMMSAT_DOMAIN "/tmp/COMMSAT.domain"

int OnWriteMsgToCommsat(char* cmd, char *data)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	int len ; 
	struct sockaddr_un srv_addr;
	
	char *sendData = NULL;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, COMMSAT_DOMAIN);

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
	
	if(data == NULL) {
		len = strlen(cmd);
		iRet = write(iConnect_fd, cmd, len);
	}else {
		int sendLen  = 0;
		sendLen = strlen(cmd) + strlen(data) + 2 + 4 ;// ('%s%s',cmd,data)
		sendData = calloc(sendLen, 1);
		if(sendData) {
			snprintf(sendData, sendLen, "%s%s&", cmd, data);
			len = strlen(sendData);
			iRet = write(iConnect_fd, sendData, len);
			free(sendData);
		}
		
	}

	if (iRet != len)
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











