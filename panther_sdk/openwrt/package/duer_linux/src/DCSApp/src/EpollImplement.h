#include  <stdlib.h>
#include  <stdio.h>
#include  <sys/un.h>
#include  <iostream>
#include  <string.h>
#include  <unistd.h>
#include  <fcntl.h>
#include  <errno.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include  <sys/epoll.h>
#include  <netinet/in.h>
#include  <pthread.h>
#include  <arpa/inet.h>
#include  <memory>
#include  "DcsSdk/DcsSdk.h"
#include "LoggerUtils/DcsSdkLogger.h"
#include <DCSApp/DeviceIoWrapper.h>
//duerOSDcsSDK::sdkInterfaces::DcsSdk;
namespace duerOSDcsApp {
namespace application {

typedef struct {
	const char *direct;
	framework::DeviceInput event;
}epoll_direct_handle;

void *run(void *argv);

class epoll_server {  
public:

//	epoll_server(std::shared_ptr<duerOSDcsSDK::sdkInterfaces::DcsSdk> dcsSdk);
	epoll_server();
	virtual ~epoll_server();  
//	void deviceIO_handler(framework::DeviceInput event, void *data, int len);
	bool init(int sock_count);  
	bool init(const char* ip, int port, int sock_count);  
	int epoll_server_wait(int time_out);  
	int accept_new_client();  
	int recv_data(int sock, char* recv_buf);  
	int send_data(int sock, char* send_buf, int buf_len);  
	void parse_and_handle(char *data, int len);
	void *join_pthread();
	
	struct epoll_event *m_epoll_events;  
	int m_listen_sock;	
	int m_epoll_fd;  

private:  
		
	pthread_t m_epoll_thread;
	int m_max_count;  
//	std::shared_ptr<duerOSDcsSDK::sdkInterfaces::DcsSdk> m_epoll_dcsSdk;
};	

}
}
