#ifndef __APP_BTUP_H__
#define __APP_BTUP_H__


//extern int  iUartfd;
extern int app_bt_up(void);
extern int open_port(void);
//extern int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop);
extern int set_opt(int baud, int databits, int parity, int stopbits,char *url);



#endif
