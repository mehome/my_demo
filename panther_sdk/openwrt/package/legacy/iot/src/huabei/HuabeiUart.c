#include "HuabeiUart.h"

#include <sys/socket.h>  
#include <sys/un.h> 
#include <fcntl.h>

#include <netinet/in.h>
#include "debug.h"

#define HUABEI_UARTD "/tmp/uartd.huabei"

int NotifyToHuabei(HuabeiUartMsg *msg)
{
	int iRet = -1;
    int iConnect_fd = -1;
	int flags = -1;
	int len ; 
	struct sockaddr_un srv_addr;
	
	char *sendData = NULL;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, HUABEI_UARTD);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd < 0)
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
	
	len = sizeof(HuabeiUartMsg) - 1 + msg->len;
	debug("len:%d", len);
	iRet = write(iConnect_fd, msg , len);
	debug("iRet:%d", iRet);
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

int HuabeiUartSendData(char *filename, int operate)
{
    HuabeiUartMsg msg;
	int ret;
	memset(&msg, 0, sizeof(HuabeiUartMsg));

	msg.type = HUABEI_TYPE_DATA;
	memcpy(&msg.data, filename, strlen(filename));
	msg.len = strlen(filename);
	msg.operate = operate;
	info("msg.data:%s",msg.data);
	ret = NotifyToHuabei(&msg);
	return ret;
}


int HuabeiUartSendCommand(int operate)
{

	HuabeiUartMsg msg;

	memset(&msg, 0, sizeof(HuabeiUartMsg));

	msg.type = HUABEI_TYPE_OPERATE_COMMAND;
	msg.len = 0;
	msg.operate = operate;
	
	return NotifyToHuabei(&msg);
}

int HuabeiUartSendPlay(int operate)
{

	HuabeiUartMsg msg;

	memset(&msg, 0, sizeof(HuabeiUartMsg));

	msg.type = HUABEI_TYPE_OPERATE_PLAY;
	msg.len = 0;
	msg.operate = operate;
	
	return NotifyToHuabei(&msg);
}




