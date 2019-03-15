#ifndef __CLIENTFD_SET_H__
#define __CLIENTFD_SET_H__


#include <sys/types.h>
#include <stdbool.h>

#include <stdlib.h>

#define true 1
#define false 0

#define MAX_CLIENT 4
typedef struct clientfdsets
{
	unsigned int clientnum;
	int fd[MAX_CLIENT];
	fd_set cli_fdset;
	int maxfd;
}clientfds;

int clientfds_FD_ISSET(clientfds *fds);
void clientfds_add_to_listen(clientfds *fds);
void clientfds_delete(int fd, clientfds *fds);
bool clientfds_add(int fd, clientfds *fds);
void clientfds_init(clientfds *fds);


#endif
