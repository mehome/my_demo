#include "dueros_socket_client.h"
#include "function/log/slog.h"

#define DIRECT_URL_BUFSIZE 1024
#define DIRECT_BUTTON_BUFSIZE 64
#define DELIMITER '`'
#define TERMINATOR '\n'

enum INSTRUCTION_TYPE{
	BUTTON,
	URL
};

//para instruction:
//	int sum: 参数个数(不算sum.从1开始数)
//只能传int型参数
int send_button_direction_to_dueros(int sum, int src, ...)
{
  int para;
  int argno = 1, i = 0, type = BUTTON;  
  char *dest = (char *)malloc(DIRECT_BUTTON_BUFSIZE);
  if(dest){
	  memset(dest, 0, DIRECT_BUTTON_BUFSIZE);
  }else{
	  err("malloc error");
	  return -1;
  }
  memcpy(&dest[i], &type, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  va_list args;
  va_start(args,src);
  inf("fmt: %d", src);
  memcpy(&dest[i], &src, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  inf("contex: %s", dest);
  while(argno < sum){
    para = va_arg(args, int );
    inf("para: %d, i: %d", para, i);
    memcpy(&dest[i], &para, sizeof(int));
    i += sizeof(int);
    dest[i++] = DELIMITER;
    argno++;
  }
  va_end(args);
  dest[i-1] = TERMINATOR;
  
  socket_write(UNIX_DOMAIN, dest, i);
  free(dest);
  return 0;
}

//para instruction:
//	int sum: 参数个数(不算sum.从1开始数)
//只能传char *型参数
int send_player_url_to_dueros(int sum, const char *src, ...)
{
  char *para;
  int argno = 1, i = 0, type = URL;
  char *dest = (char *)malloc(DIRECT_URL_BUFSIZE);
  if(dest){
	  memset(dest, 0, DIRECT_URL_BUFSIZE);
  }else{
	  err("malloc error");
	  return -1;
  }
  memcpy(&dest[i], &type, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  
  va_list args;
  va_start(args,src);
  vsprintf(&dest[i],src,args);
  i += strlen(src);
  dest[i++] = DELIMITER;
  inf("contex: %s", dest);
  while(argno < sum){
	para = va_arg(args, char *);
	inf("para: %s, i: %d", para, i);
	vsprintf(&dest[i],para,args);
	i += strlen(para);
	dest[i++] = DELIMITER;
	argno++;
  }
  va_end(args);
  dest[i-1] = TERMINATOR;
  socket_write(UNIX_DOMAIN, dest, i);
  free(dest);
  return 0;
}

int send_data_to_dueros(const void *data, unsigned int len)
{
  int i = 0, type = URL;
  char *dest = (char *)malloc(DIRECT_URL_BUFSIZE);
  if(dest){
	  memset(dest, 0, DIRECT_URL_BUFSIZE);
  }else{
	  err("malloc error");
	  return -1;
  }
  memcpy(&dest[i], &type, sizeof(int));
  i += sizeof(int);
  dest[i++] = DELIMITER;
  memcpy(&dest[i], data, len);
  i += len;
  dest[i++] = TERMINATOR;
  socket_write(UNIX_DOMAIN, dest, i);
  free(dest);
  return 0;
}

int send_to_dueros(const void *data, int len)
{
	if(data == NULL || len <= 0){
		return -1;
	}
	int ret = socket_write(UNIX_DOMAIN, data, len);
	return ret;
}

