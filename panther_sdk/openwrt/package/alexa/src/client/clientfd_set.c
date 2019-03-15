#include "clientfd_set.h"
#include <stdio.h>

//结构体初始化
void clientfds_init(clientfds *fds)
{
	int i = 0;
	for(; i < MAX_CLIENT; i++){
		fds->fd[i] = -1;
	}
	fds->clientnum = 0;
	fds->maxfd = 0;
}

//添加fd到结构体数组中，便于后面统一操作
bool clientfds_add(int fd, clientfds *fds)
{	
	int i = 0;
	for(;i < MAX_CLIENT; i++){
		if(fds->fd[i] == -1){
			fds->fd[i] = fd;
			fds->clientnum++;
			return true;
		}
	}
	return false;
}

//删除即将断开连接的fd
void clientfds_delete(int fd, clientfds *fds)
{
	int i = 0;
	for(;i < MAX_CLIENT; i++){
		if(fds->fd[i] == fd){
			fds->fd[i] = -1;
			fds->clientnum--;
			return;
		}
	}
	printf("no fd: %d\n", fd);
}

//将结构体数组中所有需要监听的fd添加到FD_SET中
void clientfds_add_to_listen(clientfds *fds)
{
	int i = 0;
	FD_ZERO(&fds->cli_fdset);	
	for(;i < MAX_CLIENT; i++){
		if(fds->fd[i] != -1){
			FD_SET(fds->fd[i], &fds->cli_fdset);
			fds->maxfd = (fds->maxfd > fds->fd[i] ? fds->maxfd: fds->fd[i]);
		}
	}	
}

//监听到某一路fd需要处理，返回需要处理的fd，后续进行数据处理
int clientfds_FD_ISSET(clientfds *fds)
{
	int i = 0;
	for(; i < MAX_CLIENT; i++){
		if(fds->fd[i] < 0)
			continue;
		if (FD_ISSET(fds->fd[i], &fds->cli_fdset)){
			return fds->fd[i];
		}			
	}
	return -1;
}
