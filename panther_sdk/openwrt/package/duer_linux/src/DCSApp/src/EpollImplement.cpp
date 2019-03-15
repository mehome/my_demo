#include "EpollImplement.h"
#include <DCSApp/DuerosFifoCmdTrigger.h>
#include <DeviceIo/DeviceIo.h>
#include "DCSApp/DuerLinkCchipInstance.h"

namespace duerOSDcsApp {
namespace application {

#define UNIX_DOMAIN "/tmp/UNIX.baidu"

#define DELIMITER '`'
#define TERMINATOR '\n'

enum INSTRUCTION_TYPE{
	BUTTON,
	URL
};

epoll_server::epoll_server()
{	
	init(10);
#if 1
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,PTHREAD_STACK_MIN * 10);
	pthread_create(&m_epoll_thread, &attr,run, this); 
	pthread_attr_destroy(&attr);
#else
	pthread_create(&m_epoll_thread,nullptr,run, this); 
#endif
}

  
epoll_server::~epoll_server()  
{  
	if (m_listen_sock > 0)	
	{  
		close(m_listen_sock);  
	}  
  
	if (m_epoll_fd > 0)  
	{  
		close(m_epoll_fd);	
	}  
}  
  
  
bool epoll_server::init(int sock_count)	
{  
	m_listen_sock = 0;	
	m_epoll_fd = 0;  
	m_max_count = sock_count;	  
	struct sockaddr_un server_addr;  
	remove(UNIX_DOMAIN);
	memset(&server_addr, 0, sizeof(&server_addr));	
	server_addr.sun_family = AF_UNIX;  
	strncpy(server_addr.sun_path, UNIX_DOMAIN, sizeof(server_addr.sun_path) - 1);
  
	m_listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);  
	if(m_listen_sock == -1)  
	{  
		exit(1);  
	}  
	  
	if(bind(m_listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)	
	{  
		exit(1);  
	}  
	  
	if(listen(m_listen_sock, 5) == -1)	
	{  
		exit(1);  
	}  
  
	m_epoll_events = new struct epoll_event[sock_count];  
	if (m_epoll_events == NULL)  
	{  
		exit(1);  
	}  
  
	m_epoll_fd = epoll_create(sock_count);	
	struct epoll_event ev;	
	ev.data.fd = m_listen_sock;  
	ev.events  = EPOLLIN;  
	epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_sock, &ev);  
}  
  
bool epoll_server::init(const char* ip, int port , int sock_count)	
{	  
	m_max_count = sock_count;  
	struct sockaddr_in server_addr;  
	memset(&server_addr, 0, sizeof(&server_addr));	
	server_addr.sin_family = AF_INET;  
	server_addr.sin_port = htons(port);  
	server_addr.sin_addr.s_addr = inet_addr(ip);		  
  
	m_listen_sock = socket(AF_INET, SOCK_STREAM, 0);  
	if(m_listen_sock == -1)  
	{  
		exit(1);  
	}  
	  
	if(bind(m_listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)	
	{  
		exit(1);  
	}  
	  
	if(listen(m_listen_sock, 5) == -1)	
	{  
		exit(1);  
	}  
  
	m_epoll_events = new struct epoll_event[sock_count];  
	if (m_epoll_events == NULL)  
	{  
		exit(1);  
	}  
  
	m_epoll_fd = epoll_create(sock_count);	
	struct epoll_event ev;	
	ev.data.fd = m_listen_sock;  
	ev.events  = EPOLLIN;  
	epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_sock, &ev);  
}  
  
int epoll_server::accept_new_client()  
{  
	sockaddr_in client_addr;  
	memset(&client_addr, 0, sizeof(client_addr));  
	socklen_t clilen = sizeof(struct sockaddr);   
	int new_sock = accept(m_listen_sock, (struct sockaddr*)&client_addr, &clilen);	
	struct epoll_event ev;	
	ev.data.fd = new_sock;	
	ev.events  = EPOLLIN;  
	epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, new_sock, &ev);  
	return new_sock;  
}  
  
int epoll_server::recv_data(int sock, char* recv_buf)  
{  
	char buf[1024] = {0};  
	int len = 0;  
	int ret = 0;  
	while(ret >= 0)  
	{  
		ret = recv(sock, buf, sizeof(buf), 0);	
		if(ret <= 0)  
		{  
			struct epoll_event ev;	
			ev.data.fd = sock;	
			ev.events  = EPOLLERR;	
			epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, sock, &ev);  
			close(sock);  
			break;	
		}  
		else if(ret < 1024)  
		{  
			memcpy(recv_buf, buf, ret);  
			len += ret;   
			struct epoll_event ev;	
			ev.data.fd = sock;	
			ev.events  = EPOLLOUT;	
			epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, sock, &ev);  
			break;	
		}  
		else  
		{  
			memcpy(recv_buf, buf, sizeof(buf));  
			len += ret;  
		}  
	}  
  
	return ret <= 0 ? ret : len;  
}  
  
int epoll_server::send_data(int sock, char* send_buf, int buf_len)	
{  
	int len = 0;  
	int ret = 0;  
	while(len < buf_len)  
	{  
		ret = send(sock, send_buf + len, 1024, 0);	
		if(ret <= 0)  
		{  
			struct epoll_event ev;	
			ev.data.fd = sock;	
			ev.events  = EPOLLERR;	
			epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, sock, &ev);  
			close(sock);  
			break;	
		}  
		else  
		{  
			len += ret;  
		}  
  
		if(ret < 1024)	
		{  
			break;	
		}  
	}  
  
	if(ret > 0)  
	{  
		struct epoll_event ev;	
		ev.data.fd = sock;	
		ev.events  = EPOLLIN;  
		epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, sock, &ev);  
	}  
  
	return ret <= 0 ? ret : len;  
}  
  
int epoll_server::epoll_server_wait(int time_out)  
{  
	int nfds = epoll_wait(m_epoll_fd, m_epoll_events, m_max_count, time_out);  
	return nfds;
}  

void epoll_server::parse_and_handle(char *data, int len)
{
	dbg("len: %d", len);
	int i = 0;
	framework::DeviceInput *key = NULL;
	void *para = NULL;
	int para_len = 0;
#if 1
	if(len < 4){
		err("data err: %s", data);
		return;
	}
	switch(*(int *)(&data[i])){
		case BUTTON:{
			dbg("button comes");
			i += sizeof(int);
			while(data[i] != TERMINATOR){
//				dbg("data[%d]: %x", i, data[i]);
				if(data[i++] == DELIMITER){
//					dbg("data[%d]: %x", i, data[i]);
					if(key == NULL){
						key = (framework::DeviceInput *)&data[i];
						i += sizeof(framework::DeviceInput);
					}else{
						para = (void *)&data[i];
						para_len = len - i - 1 ;
						break;
					}
				}
			}
			inf("key: %d, para_len: %d", *key, para_len);
			DeviceIoWrapper::getInstance()->callback(*key, para, para_len);
				
			break;
		}
		case URL:{
			i += sizeof(int);
			while(data[i] != TERMINATOR){
//				dbg("%d: %hhx ", i, data[i]);
				if(data[i++] == DELIMITER){
					para = (void *)&data[i];
					para_len = len - i - 1 ;
//					dbg("%d: %s, len: %d", i, &data[i], para_len);
					//duerLink::duerLinkSdk::get_instance()->ble_recv_data(para, para_len);
					DuerLinkCchipInstance::get_instance()->ble_recv_data(para, para_len);
				}
			}
			break;
		}
		default:{
			dbg("wrong instruction");
			break;
		}
	}
#else
	for(i = 0; i < sizeof(epoll_handler)/sizeof(epoll_direct_handle); i++){
		if(!strncmp(epoll_handler[i].direct, data, len)){
			DeviceIoWrapper::getInstance()->callback(epoll_handler[i].event, NULL, 0);
		}
	}
#endif
}


#define RECVBUF_SIZE 1024
void *run(void *arg)  
{  
	showProcessThreadId("epoll_server");
	epoll_server *server = (epoll_server *)arg;
	int time_out =2000;
	char *recv_buf = new char[RECVBUF_SIZE];  
	char *send_buf = new char[RECVBUF_SIZE];  
	while(1)  
	{  
		int ret = server->epoll_server_wait(time_out);	
		if(ret == 0)  
		{  
//			dbg("time out");  
			continue;  
		}  
		else if(ret == -1)	
		{  
			dbg("error");  
			err("error");
		}  
		else  
		{  
			for(int i = 0; i < ret; i++)  
			{  
				if(server->m_epoll_events[i].data.fd == server->m_listen_sock)	
				{  
					if(server->m_epoll_events[i].events & EPOLLIN)	
					{  
						int new_sock = server->accept_new_client();  
					}  
				}  
				else  
				{  
					if(server->m_epoll_events[i].events & EPOLLIN)	
					{  
						memset(recv_buf, 0, RECVBUF_SIZE);	
						int recv_count = server->recv_data(server->m_epoll_events[i].data.fd, recv_buf);  
						if(recv_count > 0){
							dbg(recv_buf);  
							server->parse_and_handle(recv_buf, recv_count);
						}
//						m_epoll_events[i].events = EPOLLOUT;
//						epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, m_epoll_events[i].data.fd, &m_epoll_events[i]);
//						strcpy(send_buf, recv_buf);  
//						memset(recv_buf, 0, sizeof(recv_buf));	
					}  
					else if(server->m_epoll_events[i].events & EPOLLOUT)  
					{  
						int send_count = server->send_data(server->m_epoll_events[i].data.fd, send_buf, strlen(send_buf));	
						memset(send_buf, 0, sizeof(send_buf));	
						server->m_epoll_events[i].events = EPOLLIN;
						epoll_ctl(server->m_epoll_fd, EPOLL_CTL_MOD, server->m_epoll_events[i].data.fd, &server->m_epoll_events[i]);
					}  
					  
				}  
			}  
		}  
	}  
}  

void *epoll_server::join_pthread()
{
    pthread_join(m_epoll_thread, NULL);
}


/*******************************************test code*******************************************/
#define CLIENT 0
#if CLIENT
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_DOMAIN "/tmp/UNIX.baidu"

int main(void)
{
  int connect_fd;
  struct sockaddr_un srv_addr;
  char snd_buf[256];
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

  memset(rcv_buf, 0, 256);
  int rcv_num = read(connect_fd, rcv_buf, sizeof(rcv_buf));
  printf("receive message from server (%d) :%s\n", rcv_num, rcv_buf);

  memset(snd_buf, 0, 256);
  strcpy(snd_buf, "message from client");
  printf("sizeof(snd_buf): %d\n", sizeof(snd_buf));

  printf("send data to server... ...\n");
  for(i = 0; i < 4; i++)
  {
    write(connect_fd, snd_buf, sizeof(snd_buf));
  }
  printf("send end!\n");
  close(connect_fd);
  return 0;

}
#endif

}
}
