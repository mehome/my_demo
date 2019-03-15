#ifndef __UARTD_H__
#define __UARTD_H__

extern void *uartd_thread(void *arg);
extern int open_port(void);
extern int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop);


#endif


