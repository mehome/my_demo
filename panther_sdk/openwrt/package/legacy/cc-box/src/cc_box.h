#ifndef CC_BOX_H_
#define CC_BOX_H_ 1
#include <signal.h>
#include <libcchip/function/common/fd_op.h>
#include <libcchip/function/common/misc.h>
#include <libcchip/function/my_socket/my_socket.h>

typedef struct handle_item_t{
	const char *name;
	int (*handle)(char *argv[]);
	void *args;
}handle_item_t;


//------------------------------------------------------------------------
#define SRV_ADDR 		"/tmp/cmd.sock1"
#define CLI_ADDR 		"/tmp/cmd.sock4"
#define SERVER_IP 		"192.168.1.1"
// #define SERVER_IP 	"192.168.1.6"
#define LISTEN_IP		"0.0.0.0"
#define PORT_NUM		50000

#endif