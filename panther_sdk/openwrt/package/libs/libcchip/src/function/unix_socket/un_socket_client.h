#ifndef UN_SOCKET_CLIENT_H_
#define UN_SOCKET_CLIENT_H_


int socket_write(const char *filename, const void *data, unsigned int len);
int socket_write_format(const char *filename, const char *direction);

#endif
