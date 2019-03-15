#include <function/my_socket/my_socket.h>
#include <function/common/fd_op.h>
#define UNIX_DOMAIN "/tmp/UNIX.baidu"

enum DIRECTIONS 
{
	START_RECORDER 		= 0,
	END_RECORDER,
	CONNECTING
};

void data_uniform(char *contex, const char *fmt, ...)
{
	va_list args;  
	va_start(args,fmt);
	vsprintf(contex,fmt,args);	
	va_end(args);
}

int socket_write(const char *filename, const void *data, unsigned int len)
{
	int connect_fd = un_tcp_cli_create(filename);
	if(connect_fd < 0){
		return -1;
	}

	int flags = fcntl(connect_fd, F_GETFL, 0);
	fcntl(connect_fd, F_SETFL, flags | O_NONBLOCK);

	int size=un_write(connect_fd, data, len);
	if( size != len){
		return -2;
	};
	un_close(connect_fd);
	return 0;
}


int socket_write_format(const char *filename, const char *direction)
{
	int ret = 0;
	int len = strlen(direction);
	char *data = (char *)malloc(len + 2);
	data_uniform(data, "%s&", direction);
	ret = socket_write(filename, data, strlen(data));
	
	free(data);
	return ret;
#if 0
  int connect_fd;
//  struct sockaddr_un srv_addr;
//  char snd_buf[16];
//  char rcv_buf[256];
  int ret;
  connect_fd = un_tcp_cli_create(filename);
/*  connect_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  srv_addr.sun_family = AF_UNIX;
  strcpy(srv_addr.sun_path, filename);
  ret = connect(connect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));

  if(ret == -1)
  {
	perror("connect to server failed!");
	close(connect_fd);
	unlink(UNIX_DOMAIN);
	return 1;
  }
*/
	  memset(snd_buf, 0, sizeof(snd_buf));
	  data_uniform(snd_buf, "%s&", );
	//  sprintf(snd_buf, "%d", START_RECORDER);
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
#endif

}

