#ifndef _COMMON_H
#define _COMMON_H

#include <sys/time.h>
#include <pthread.h>
#include <openssl/rsa.h>
#include <stdint.h>
#include <sys/socket.h>
#include "audio.h"
#include "mdns.h"
#include "player.h"

// struct sockaddr_in6 is bigger than struct sockaddr. derp
#ifdef AF_INET6
#define SOCKADDR struct sockaddr_storage
#define SAFAMILY ss_family
#else
#define SOCKADDR struct sockaddr
#define SAFAMILY sa_family
#endif

//#define DEBUG
#ifndef DEBUG
#define debug
#endif

typedef struct {
	char log;
	char *password;
	char *apname;
	uint8_t hw_addr[6];
	int port;
	char *output_name;
	audio_output *output;
	char *mdns_name;
	mdns_backend *mdns;
	int buffer_start_fill;
	int daemonise;
	char *cmd_start, *cmd_stop;
	char *pidfile;
	char *logfile;
	char *errfile;
} shairport_cfg;

extern int debuglev;
void die(char *format, ...);
void warn(char *format, ...);
#ifdef DEBUG
void debug(int level, char *format, ...);
#endif
uint8_t *base64_dec(char *input, int *outlen);
char *base64_enc(uint8_t * input, int length);

#define MAX(a,b) (((a)>(b))?(a):(b))
#define RSA_MODE_AUTH (0)
#define RSA_MODE_KEY  (1)
uint8_t *rsa_apply(uint8_t * input, int inlen, int *outlen, int mode);

extern shairport_cfg config;

void shairport_shutdown(int retval);
void shairport_startup_complete(void);

typedef struct {
	int fd;
	stream_cfg stream;
	SOCKADDR remote;
	int rtsp_running;
	char *dacp_id;
	char *active_remote;
	unsigned int total;
	unsigned int current;
	unsigned int audio_socket;
	pthread_t dacp_thread;
	pthread_t thread;
} rtsp_conn_info;

//DACP CTRL
struct dacp_info {
	unsigned char start;
	int sockfd[2];
	unsigned int addr;
	unsigned short port;
	char *dacp_id;
	char *active_remote;
};
void air_syslog(const char *format, ...);

void dacp_thread_func(struct dacp_info *dacp_ctx);
void mdns_func(struct dacp_info *dacp_ctx);

typedef struct {
	char addr[32];
	int port;
} host_t;

typedef struct {
	host_t *host;
	char *dacp_id;
	char *srv_name;
	char *active_remote;
} srv_t;

enum {
	RTSP = 1,
	RTP,
	PLAY,
	MDNS,
	DACP,
	CTRL,
	WARN,
};

extern struct timeval tv;

static long timevaldiff(struct timeval *starttime, struct timeval *finishtime)
{
	long msec;
	msec = (finishtime->tv_sec - starttime->tv_sec) * 1000;
	msec += (finishtime->tv_usec - starttime->tv_usec) / 1000;
	return msec;
}

static long gettimevaldiff(struct timeval *start_tv)
{
	struct timeval tv;
	struct timeval *end_tv = &tv;
	gettimeofday(end_tv, NULL);
	return timevaldiff(start_tv, end_tv);
}

static void gettimeofbegin(struct timeval *start_tv)
{
	gettimeofday(start_tv, NULL);
}

extern srv_t dacp_srv;
//END
#endif // _COMMON_H
