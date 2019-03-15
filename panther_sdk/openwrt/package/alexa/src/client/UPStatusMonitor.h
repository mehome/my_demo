#ifndef __UPSTATUSMONIROT_H__
#define __UPSTATUSMONIROT_H__

#define UNIX_DOMAIN "/tmp/UNIX.alexa"

#define CONFIG_PATH  "/etc/config/AlexaClientConfig.json"

typedef struct __data_proc_t {
    const char *name;
    int (*exec_proc)(int fd, char *cmd);
} data_proc_t;

extern void CreateUPStatusMonitorPthread(void);
extern int OnWriteMsgToWakeup(char* cmd);

#endif /* __UPSTATUSMONIROT_H__ */

