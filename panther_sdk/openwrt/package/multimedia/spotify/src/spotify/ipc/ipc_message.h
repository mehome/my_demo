#ifndef SPOTIFY_IPC_MESSAGE_H
#define SPOTIFY_IPC_MESSAGE_H
#include <stdint.h>

#define SPOTIFY_SOCK_S "/tmp/spotify_s.socket"
#define SPOTIFY_SOCK_C "/tmp/spotify_c.socket"

struct spotify_ipc_param {
	  unsigned char senderType;
	  unsigned char isNeedResp;
	  unsigned int len;
	  char data[128];
} __attribute__ ((packed));

typedef void (*cmd_function_t)(struct spotify_ipc_param *msg );

struct cmd_table {
    const char* cmd;
    cmd_function_t func;
};

/**
* Start a thread to recv and deal ipc message
*/
int start_ipc_thread();

/**
* resgister cmd table to ipc control
*/
void register_ipc_handles(struct cmd_table *cmds);
#define SPOTIFY_RECV_LEN 64
#include <libcchip/platform.h>

#endif
