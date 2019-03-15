#ifndef UARTD_FIFO_H_
#define UARTD_FIFO_H_ 1
#include "../common/un_fifo.h"
#define UARTD_FIFO_FILE "/tmp/UartFIFO"

extern int uartd_fifo_write(char *contex);
extern int uartd_fifo_write_fmt(const char *fmt,...);
#endif