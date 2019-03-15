#include "uartd_fifo.h"
static int uartd_fifo_fd=0;
int uartd_fifo_write(char *contex){
	int err=0;
	int uartd_fifo_fd=un_fifo_init(UARTD_FIFO_FILE,"+");
	if(uartd_fifo_fd<0){
		err=-1;
		goto exit;
	}
	err=un_fifo_write_str(uartd_fifo_fd,contex);
	if(err<0){
		err=-2;
		goto exit;
	}
exit:
	un_fifo_deinit(uartd_fifo_fd);
	return err;
}


int uartd_fifo_write_data(char *contex, unsigned int len){
	int err=0;
	int uartd_fifo_fd=un_fifo_init(UARTD_FIFO_FILE,"+");
	if(uartd_fifo_fd<0){
		err=-1;
		goto exit;
	}
	
	err=un_fifo_write(uartd_fifo_fd,contex, len);
	if(err<0){
		err=-2;
		goto exit;
	}
exit:
	un_fifo_deinit(uartd_fifo_fd);
	return err;
}
#define FMT_SUBFFIX "\n"
int uartd_fifo_write_fmt(const char *fmt,...){
	char contex[1024]="";
	va_list args;
	va_start(args,fmt);
	ssize_t ret=vsnprintf(contex,sizeof(contex)-sizeof(FMT_SUBFFIX),fmt,args);
	if(ret<0){
		err("vsnprintf failed!");
		return -1;
	}
	va_end(args);
	strcat(contex,FMT_SUBFFIX);
	ssize_t wRet= uartd_fifo_write(contex);
	if(wRet<ret){
		return -3;
	}
	return 0;
}	
