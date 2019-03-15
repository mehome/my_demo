#include "ipc_message.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>


void receive_and_handle_key_cmd(int unix_sock);

static pthread_t ipc_thread;

struct cmd_table *cmds;

#define cprintf(fmt, args...) do { \
    FILE *fp = fopen("/dev/console", "w"); \
    if (fp) { \
        fprintf(fp, fmt , ## args); \
        fclose(fp); \
        } \
    } while (0)

//#define DEBUG

void Console_log(const char *format, ...) {
#ifdef IPC_DEBUG
    char buf[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    cprintf("[%lu] %s", gettimevalmsec(NULL), buf);
#endif
}

void *ipc_handle_thread(void *unused) {
    int unix_sock = create_unix_sock();
    if (unix_sock < 0) {
        Console_log("[%s:%d]create_unix_sock failed", __func__, __LINE__);
        return;
    }

    Console_log("[%s:%d]", __func__, __LINE__);
    while(1) {
        receive_and_handle_key_cmd(unix_sock);
        usleep(500000);
    }
}

int start_ipc_thread() {
    if (pthread_create(&ipc_thread, NULL, ipc_handle_thread, NULL) < 0) {
        Console_log("ipc_create thread failed");
        return -1;
    }
    return 0;
}

int create_unix_sock() {
    Console_log("[%s:%d]", __func__, __LINE__);
    int fd, len;
    struct sockaddr_un addr;
    struct ctrl_param *ctrl_arg;

    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        Console_log("[%s:%d]create socket failed", __func__, __LINE__);
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SPOTIFY_SOCK_S);
    unlink(addr.sun_path);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        Console_log("[%s:%d]bind unix socket failed", __func__, __LINE__);
        close(fd);
        return -1;
    }
    return fd;
}

void receive_and_handle_key_cmd(int unix_sock)
{
    struct spotify_ipc_param eParam={0};
    struct sockaddr_un from;
    socklen_t fromlen = sizeof(from);
    ssize_t recvsize;

    recvsize =
        recvfrom(unix_sock, &eParam, sizeof(eParam)-1, 0, (struct sockaddr *)&from,
        &fromlen);
    if (recvsize <= 0) {
        Console_log("[%s:%d]recvsize <= 0", __func__, __LINE__);
        return;
    }
    cmds[eParam.senderType].func(&eParam);
}

void register_ipc_handles(struct cmd_table *handle_table) {
    cmds = handle_table;
}

//static void handle_key_cmd(char *key_cmd)
//{
//    int i = 0;

//    Console_log("Received key command is %s\n", key_cmd);

//    for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
//        if (strcmp(key_cmd, cmds[i].cmd) == 0) {
//            cmds[i].func();
//            break;
//        }
//    }
//}

#ifdef SPOTIFY_IPC
void notify_spotify_ipc()
{
    ctrl_spotify_handler();
}
#endif

