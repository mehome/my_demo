#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include <pthread.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/times.h>

#include "btup.h"

pthread_t thread[2];
//pthread_mutex_t pmMUT;

pthread_mutex_t pmMUT = PTHREAD_MUTEX_INITIALIZER;/*初始化互斥锁*/  
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;//init cond  


int Uartfd;
static char AT_UP_DATE[]		="AXX+UP+DATE&";	//������������ģʽ
#define BUFF_LENGTH 32
#define PROGRESS_DIR "/tmp/progress"
#define debugflag 1
#define MAX_DEV_NAME 256
#define GPIO_RESET_PIN     12


static char AT_UP_3266[]		="AXX+UP+3266&";	//蓝牙进入升级模式


static char gpio_name[MAX_DEV_NAME];


// GPIO functions
//
/* export the gpio to user mode space */
static int export_gpio(int gpio)
{
        int err = 0;
        int fd;
        int wr_len;
        
        /* check if gpio is already exist or not */ 
        sprintf(gpio_name, "/sys/class/gpio/gpio%d/value", gpio);
                
        fd = open(gpio_name,O_RDONLY);
        if ( fd >= 0 ) {
                /* gpio device has already exported */
                close(fd);
                return 0;
        }
        
        /* gpio device is no exported, export it */
        strcpy(gpio_name,"/sys/class/gpio/export");
                
        fd = open(gpio_name,O_WRONLY);

        if (fd < 0) {
                fprintf(stderr, "failed to open device: %s\n", gpio_name);
                return -ENOENT;
        }
        
        sprintf(gpio_name,"%d",gpio);
        wr_len = strlen(gpio_name);

        if (write(fd,gpio_name,wr_len)!=wr_len) {
                fprintf(stderr, "failed to export gpio: %d\n", gpio);
                return -EACCES;
        }

        return err;
}

static int free_gpio( int gpio)
{
        int err = 0;
        int fd;
        int wr_len;

        /* check if gpio exist */
        sprintf(gpio_name, "/sys/class/gpio/gpio%d/value", gpio);

        fd = open(gpio_name,O_RDONLY);
        if ( fd < 0 ) {
                /* gpio device doesn't exist */
                return -ENOENT;
        }
        close(fd);

        /* gpio device is no exported, export it */
        strcpy(gpio_name,"/sys/class/gpio/unexport");

        fd = open(gpio_name,O_WRONLY);

        if (fd < 0) {
                fprintf(stderr, "failed to open device: %s\n", gpio_name);
                return -ENOENT;
        }

        sprintf(gpio_name,"%d",gpio);
        wr_len = strlen(gpio_name);

        if (write(fd,gpio_name,wr_len)!=wr_len) {
                fprintf(stderr, "failed to free gpio: %d\n", gpio);
                return -EACCES;
        }

        return err;

}

static int set_gpio_dir(int gpio)
{
        int err = 0;
        int fd;
        int wr_len;
        char str_dir[4];

        err = export_gpio(gpio);

        if (err)
                return err;

        sprintf(gpio_name, "/sys/class/gpio/gpio%d/direction", gpio);

        fd = open(gpio_name,O_WRONLY);

        if (fd < 0) {
                fprintf(stderr, "failed to open device: %s\n", gpio_name);
                err = -ENOENT;
                goto exit;
        }

        strcpy(str_dir,"out");
        wr_len = strlen(str_dir);
        if (write(fd, str_dir,wr_len) != wr_len) {
                fprintf(stderr, "Failed to set gpio %s direction to %s \n", gpio_name, str_dir);
                err =  -EACCES;
                goto exit;
        }
	if (debugflag)
        	printf("set gpio %d to %s\n",gpio,str_dir);
exit:
        close(fd);
	return err;
}

int set_gpio_value(int gpio, int value)
{ 
        int err = 0; 
        int fd; 
 
//        err = export_gpio(gpio); 
 
        if (err)  
                return err; 
 
        sprintf(gpio_name, "/sys/class/gpio/gpio%d/value", gpio); 
                 
        fd = open(gpio_name,O_WRONLY); 
 
        if (fd < 0) { 
                fprintf(stderr, "failed to open device: %s\n", gpio_name); 
                err = -ENOENT; 
                goto exit; 
        } 
 
 
        if (write(fd, value? "1":"0", 1) != 1) {  
                fprintf(stderr, "Failed to set gpio %s to %d\n", gpio_name, value); 
                err =  -EACCES; 
                goto exit; 
        } 
 
	if (debugflag)
        	printf("set gpio %d to %d\n",gpio,value); 
exit: 
        close(fd); 
         
        return err; 
}

#define MAX_BUF_SIZE 256
char buf[MAX_BUF_SIZE];

#define END_CHAR 0x13

#ifdef TERMIO	
static struct termio oterm_attr;
#else
static struct termios oterm_attr;
#endif

static int uart_baudflags[] = {
     B921600, B460800, B230400, B115200, B57600, B38400,
     B19200, B9600, B4800, B2400, B1800, B1200,
     B600, B300, B150, B110, B75, B50
};
static int uart_speeds[] = {
     921600, 460800, 230400, 115200, 57600, 38400,
     19200, 9600, 4800, 2400, 1800, 1200,
     600, 300, 150, 110, 75, 50
};

int setTermios(int fd, int baud, int databits, int parity, int stopbits);
int reset_port(int fd);
int read_data(int fd, void *buf, int len);
int write_data(int fd, void *buf, int len);
void print_usage(char *program_name);



int convert_uart_baud(int baud)
{
     int i;

     for (i = 0; i < sizeof(uart_speeds)/sizeof(int); i++) {
         if (baud == uart_speeds[i]) {
             return uart_baudflags[i];
         }
     }

     fprintf(stderr, "Unsupported baudrate, use 9600 instead!\n");
     return B9600;
}

int convert_uart_baud2(int baud2)
{
     int i;

     for (i = 0; i < sizeof(uart_baudflags)/sizeof(int); i++) {
         if (baud2 == uart_baudflags[i]) {
             return uart_speeds[i];
         }
     }

     fprintf(stderr, "Unsupported baudrate, use 9600 instead!\n");
     return 9600;
}


int setTermios(int fd, int baud, int databits, int parity, int stopbits)
{
#ifdef TERMIO	
     struct termio term_attr;
#else
		 struct termios term_attr;
#endif     
     int bbaud = convert_uart_baud(baud);

     /* Get current setting */
#ifdef TERMIO     
     if (ioctl(fd, TCGETA, &term_attr) < 0) {
         return -1;
     }
#else
		 tcgetattr(fd , &term_attr);
#endif
     /* Backup old setting */
     memcpy(&oterm_attr, &term_attr, sizeof(term_attr));

     term_attr.c_iflag &= ~(INLCR | IGNCR | ICRNL | ISTRIP);
     term_attr.c_oflag &= ~(OPOST | ONLCR | OCRNL);
     term_attr.c_lflag &= ~(ISIG | ECHO | ICANON | NOFLSH);
     term_attr.c_cflag &= ~CBAUD | ~OLCUC;
     term_attr.c_cflag &= ~(IXON|IXOFF|IXANY|IGNCR|INLCR|IUCLC);
     term_attr.c_cflag |= CREAD | bbaud;

     /* Set databits */
     term_attr.c_cflag &= ~(CSIZE);

//	 term_attr.c_lflag &= ~(ICANON |ISIG);
     switch (databits) {
         case 5:
             term_attr.c_cflag |= CS5;
             break;

         case 6:
             term_attr.c_cflag |= CS6;
             break;

         case 7:
             term_attr.c_cflag |= CS7;
             break;

         case 8:
         default:
             term_attr.c_cflag |= CS8;
             break;
     }

     /* Set parity */
     switch (parity) {
         case 1: /* Odd parity */
             term_attr.c_cflag |= (PARENB | PARODD);
             break;

         case 2: /* Even parity */
             term_attr.c_cflag |= PARENB;
             term_attr.c_cflag &= ~(PARODD);
             break;

         case 0: /* None parity */
         default:
             term_attr.c_cflag &= ~(PARENB);
             break;
     }


     /* Set stopbits */
     switch (stopbits) {
         case 2: /* 2 stopbits */
             term_attr.c_cflag |= CSTOPB;
             break;

         case 1: /* 1 stopbits */
         default:
             term_attr.c_cflag &= ~CSTOPB;
             break;
     }

     term_attr.c_cc[VMIN] = 1;
     term_attr.c_cc[VTIME] = 0;

#ifdef TERMIO 
     if (ioctl(fd, TCSETAW, &term_attr) < 0) {
         return -1;
     }

     if (ioctl(fd, TCFLSH, 2) < 0) {
         return -1;
     }
#else
		 tcflush(fd,TCIFLUSH);
		 tcflush(fd,TCIOFLUSH);
		  
		 cfsetispeed(&term_attr, bbaud );
     cfsetospeed(&term_attr, bbaud );
		 /* Update the options and do it NOW */
		 if ( tcsetattr(fd,TCSANOW, &term_attr) != 0)
		 {
		  	return -1;
		 }
		 
		 tcflush(fd, TCIOFLUSH);
#endif   
     return 0;
}


int read_data(int fd, void *buf, int len)
{
     int count;
     int ret;

     ret = 0;
     count = 0;

		 while ( len > 0 ) {
	     ret = read(fd, (char*)buf + count, len);
	     if (ret < 1) {
	     //    fprintf(stderr, "Read error %s\n", strerror(errno));
	         break;
	     }
	
	     count += ret;
	     len -= ret;
   	 }

     *((char*)buf + count) = 0;
     return count;
}


int write_data(int fd, void *buf, int len)
{
	int count;
	int ret;

	ret = 0;
	count = 0;

	while (len > 0) {

	 ret = write(fd, (char*)buf + count, len);
	 if (ret < 1) {
	 //    fprintf(stderr, "Write error %s\n", strerror(errno));
	     break;
	 }

	 count += ret;
	 len -= ret;
	}
	usleep(5000);
	return count;
}


void print_usage(char *program_name)
{
     fprintf(stderr,
             "*************************************\n"
             " A Simple Serial Port Test Utility\n"
             "*************************************\n\n"
             "Usage:\n %s <device> <baud> <databits> <parity> <stopbits>\n"
             " databits: 5, 6, 7, 8\n"
             " parity: 0(None), 1(Odd), 2(Even)\n"
             " stopbits: 1, 2\n"
             "Example:\n %s /dev/ttyS0 9600 8 0 1\n\n",
             program_name, program_name
            );
}

int setopt_opentty(int baud, int databits, int parity, int stopbits)

{
	 
     int len;
     int i, n = 10;

     Uartfd = open("/dev/ttyS1", O_RDWR | O_NONBLOCK); //非阻塞
//	 Uartfd = open("/dev/ttyS1", O_RDWR);			//阻塞
     if (Uartfd < 0) {
         fprintf(stderr, "open > error %S\n", strerror(errno));
         return 1;
     }

     if (setTermios(Uartfd, baud, databits, parity, stopbits)) {
         fprintf(stderr, "setup_port error %s\n", strerror(errno));
         close(Uartfd);
         return 1;
     }
		 
		 if( 1 ) {
		 	struct termios term_attr;
		 	
		 	tcgetattr( Uartfd, &term_attr);
     	
		 	printf("ISPEED = %ld\n", convert_uart_baud2(cfgetispeed(&term_attr)));
		 	printf("OSPEED = %ld\n", convert_uart_baud2(cfgetospeed(&term_attr)));
		}
}



int BuildBK3266Cmd_LinkCheck(uint8 *buf){
	int len=1;
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_LinkCheck;
	return(len+4);
}
int BuildBK3266Cmd_ReadReg(uint8 *buf,uint32 regAddr){
	int len=1+(4);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_ReadReg;
	buf[5]=(regAddr&0xff);
	buf[6]=((regAddr>>8)&0xff);
	buf[7]=((regAddr>>16)&0xff);
	buf[8]=((regAddr>>24)&0xff);
	return(len+4);
}
int BuildBK3266Cmd_WriteReg(uint8 *buf,uint32 regAddr,uint32 val){
	int len=1+(4+4);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_WriteReg;
	buf[5]=(regAddr&0xff);
	buf[6]=((regAddr>>8)&0xff);
	buf[7]=((regAddr>>16)&0xff);
	buf[8]=((regAddr>>24)&0xff);
	buf[9]=(val&0xff);
	buf[10]=((val>>8)&0xff);
	buf[11]=((val>>16)&0xff);
	buf[12]=((val>>24)&0xff);
	return(len+4);
}
int BuildBK3266Cmd_SetBaudRate(uint8 *buf,uint32 baudrate,uint8 dly_ms){
	int len=1+(4+1);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_SetBaudRate;
	buf[5]=(baudrate&0xff);
	buf[6]=((baudrate>>8)&0xff);
	buf[7]=((baudrate>>16)&0xff);
	buf[8]=((baudrate>>24)&0xff);
	buf[9]=(dly_ms&0xff);
	return(len+4);
}
int BuildBK3266Cmd_CheckCRC(uint8 *buf,uint32 startAddr,uint32 endAddr){
	int len=1+(4+4);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_CheckCRC;
	buf[5]=(startAddr&0xff);
	buf[6]=((startAddr>>8)&0xff);
	buf[7]=((startAddr>>16)&0xff);
	buf[8]=((startAddr>>24)&0xff);
	buf[9]=(endAddr&0xff);
	buf[10]=((endAddr>>8)&0xff);
	buf[11]=((endAddr>>16)&0xff);
	buf[12]=((endAddr>>24)&0xff);
	return(len+4);
}
int BuildBK3266Cmd_Reboot(uint8 *buf){
	int len=1+(1);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_Reboot;
	buf[5]=0xa5;
	return(len+4);
}

int BuildBK3266Cmd_Reset(uint8 *buf){
	int len=1+(4);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_Reset;
	buf[5]=0x95;
	buf[6]=0x27;
	buf[7]=0x95;
	buf[8]=0x27;
	return(len+4);
}

int BuildBK3266Cmd_StayRom(uint8 *buf){
	int len=1+(1);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=len;
	buf[4]=BK3266CMD_StayRom;
	buf[5]=0x55;
	return(len+4);
}

int BuildBK3266Cmd_FlashEraseAll(uint8 *buf){
	int len=1+(1);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashEraseAll;
	buf[8]=4;
	return(len+7);
}
int BuildBK3266Cmd_FlashErase4K(uint8 *buf,uint32 addr){
	int len=1+(4);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashErase4K;
	buf[8]=(addr&0xff);
	buf[9]=((addr>>8)&0xff);
	buf[10]=((addr>>16)&0xff);
	buf[11]=((addr>>24)&0xff);
	return(len+7);
}

int BuildBK3266Cmd_FlashErase(uint8 *buf,uint32 addr,uint8 szCmd){
	int len=1+(4+1);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashErase;
	buf[8]=szCmd;
	buf[9]=(addr&0xff);
	buf[10]=((addr>>8)&0xff);
	buf[11]=((addr>>16)&0xff);
	buf[12]=((addr>>24)&0xff);
	return(len+7);
}

int BuildBK3266Cmd_FlashWrite4K(uint8 *buf,uint32 addr,uint8 *dat){
	int len=1+(4+4*1024);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashWrite4K;
	buf[8]=(addr&0xff);
	buf[9]=((addr>>8)&0xff);
	buf[10]=((addr>>16)&0xff);
	buf[11]=((addr>>24)&0xff);
	memcpy(&buf[12],dat,4*1024);
	return(len+7);
}
int BuildBK3266Cmd_FlashRead4K(uint8 *buf,uint32 addr){
	int len=1+(4+0);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashRead4K;
	buf[8]=(addr&0xff);
	buf[9]=((addr>>8)&0xff);
	buf[10]=((addr>>16)&0xff);
	buf[11]=((addr>>24)&0xff);
	return(len+7);
}
int BuildBK3266Cmd_FlashWrite(uint8 *buf,uint32 addr,uint8 *dat,int sz){
	int len=1+(4+sz);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashWrite;
	buf[8]=(addr&0xff);
	buf[9]=((addr>>8)&0xff);
	buf[10]=((addr>>16)&0xff);
	buf[11]=((addr>>24)&0xff);
	memcpy(&buf[12],dat,sz);
	return(len+7);
}
int BuildBK3266Cmd_FlashRead(uint8 *buf,uint32 addr,uint16 lenObj){
	int len=1+(4+2);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashWrite4K;
	buf[8]=(addr&0xff);
	buf[9]=((addr>>8)&0xff);
	buf[10]=((addr>>16)&0xff);
	buf[11]=((addr>>24)&0xff);
	buf[12]=(lenObj&0xff);
	buf[13]=((lenObj>>8)&0xff);
	return(len+7);
}
int BuildBK3266Cmd_FlashReadSR(uint8 *buf,uint8 regAddr){
	int len=1+(1+0);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashReadSR;
	buf[8]=(regAddr&0xff);
	return(len+7);
}

int BuildBK3266Cmd_FlashWriteSR(uint8 *buf,uint8 regAddr,uint8 val){
	int len=1+(1+1);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashWriteSR;
	buf[8]=(regAddr&0xff);
	buf[9]=((val)&0xff);
	return(len+7);
}

int BuildBK3266Cmd_FlashWriteSR2(uint8 *buf,uint8 regAddr,uint16 val){
	int len=1+(1+2);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashWriteSR;
	buf[8]=(regAddr&0xff);
	buf[9]=((val)&0xff);
	buf[10]=((val>>8)&0xff);
	return(len+7);
}

int BuildBK3266Cmd_FlashGetMID(uint8 *buf,uint8 regAddr){
	int len=(1+4);
	buf[0]=0x01;
	buf[1]=0xe0;
	buf[2]=0xfc;
	buf[3]=0xff;
	buf[4]=0xf4;
	buf[5]=(len&0xff);
	buf[6]=((len>>8)&0xff);
	buf[7]=BK3266CMD_FlashGetMID;
	buf[8]=(regAddr&0xff);
	buf[9]=0;
	buf[10]=0;
	buf[11]=0;
	return(len+7);
}

int CheckRespond_LinkCheck(uint8 *buf){
	const uint8 cBuf[]={0x04,0x0e,0x05,0x01,0xe0,0xfc,BK3266CMD_LinkCheck+1,0x00};
	if(memcmp(cBuf,buf,8) == 0){
		DEBUG_ERR("check pass");
		return (1);

	}
	return(0);
}
int CheckRespond_ReadReg(uint8 *buf,uint32 regAddr,uint32*val){
	uint8 cBuf[256]={0x04,0x0e,0x05,0x01,0xe0,0xfc,BK3266CMD_ReadReg};
	cBuf[2]=3+1+4+4;
	cBuf[7]=(regAddr&0xff);
	cBuf[8]=((regAddr>>8)&0xff);
	cBuf[9]=((regAddr>>16)&0xff);
	cBuf[10]=((regAddr>>24)&0xff);
	if(memcmp(cBuf,buf,11)==0){
		if(val){
			uint32 t;
			t=buf[14];
			t=(t<<8)+buf[13];
			t=(t<<8)+buf[12];
			t=(t<<8)+buf[11];
			*val=t;
		}
		return(1);
	}
	return(0);
}
int CheckRespond_WriteReg(uint8 *buf,uint32 regAddr,uint32 val){
	uint8 cBuf[256]={0x04,0x0e,0x05,0x01,0xe0,0xfc,BK3266CMD_WriteReg};
	cBuf[2]=3+1+4+4;
	cBuf[7]=(regAddr&0xff);
	cBuf[8]=((regAddr>>8)&0xff);
	cBuf[9]=((regAddr>>16)&0xff);
	cBuf[10]=((regAddr>>24)&0xff);
	cBuf[11]=(val&0xff);
	cBuf[12]=((val>>8)&0xff);
	cBuf[13]=((val>>16)&0xff);
	cBuf[14]=((val>>24)&0xff);

	if(memcmp(cBuf,buf,15)==0){
		return(1);
	}
	return(0);
}
int CheckRespond_SetBaudRate(uint8 *buf,uint32 baudrate,uint8 dly_ms){
	uint8 cBuf[256]={0x04,0x0e,0x05,0x01,0xe0,0xfc,BK3266CMD_SetBaudRate};
	cBuf[2]=3+1+4+1;
	cBuf[7]=(baudrate&0xff);
	cBuf[8]=((baudrate>>8)&0xff);
	cBuf[9]=((baudrate>>16)&0xff);
	cBuf[10]=((baudrate>>24)&0xff);
	cBuf[11]=(dly_ms&0xff);
	
	if(memcmp(cBuf,buf,12)==0){
		return(1);
	}
	return(0);
}
int CheckRespond_CheckCRC(uint8 *buf,uint32 startAddr,uint32 endAddr,uint32*val){
	uint8 cBuf[256]={0x04,0x0e,0x05,0x01,0xe0,0xfc,BK3266CMD_CheckCRC};
	cBuf[2]=3+1+4;
	if(memcmp(cBuf,buf,7)==0){
		if(val){
			uint32 t;
			t=buf[10];
			t=(t<<8)+buf[9];
			t=(t<<8)+buf[8];
			t=(t<<8)+buf[7];
			*val=t;
		}
		return(1);
	}
	return(0);
}
int CheckRespond_StayRom(uint8 *buf){
	uint8 cBuf[256]={0x04,0x0e,0x05,0x01,0xe0,0xfc,BK3266CMD_StayRom};
	cBuf[2]=3+1+1;
	if(memcmp(cBuf,buf,7)==0){
		if(buf[7]==0x55)return(1);
	}
//	int i =0;
//	for (i = 0;i < 16;i++)
//		if(buf[i]==0xaa && buf[i+1]==0x55)
//			return 1;
	
	return(0);
}
int CheckRespond_SysErr(uint8 *buf){
	uint8 cBuf[256]={0x04,0x0e,0x04,0x01,0xe0,0xfc};
	if(memcmp(cBuf,buf,6)==0){
		if(buf[6]==0xee)return(1);
		if(buf[6]==0xfe)return(2);
	}
	return(0);
}
int CheckRespond_FlashEraseAll(uint8 *buf,uint8*status,uint8*to_s){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
		0xe0,0xfc,0xf4,0x03,
		0x00,BK3266CMD_FlashEraseAll};
	if(memcmp(cBuf,buf,10)==0){
		if(status){
			*status=buf[10];
		}
		if(to_s){
			*to_s=buf[11];
		}
		return(1);
	}
	return(0);
}
int CheckRespond_FlashErase4K(uint8 *buf,uint32 addr,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
		0xe0,0xfc,0xf4,0x06,
		0x00,BK3266CMD_FlashErase4K};

		if(/*(memcmp(&buf[11],&addr,4)==0)&&*/(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			return(1);
		}
		return(0);
}

int CheckRespond_FlashErase(uint8 *buf,uint32 addr,uint32 szCmd,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,1+1+(1+4),
			0x00,BK3266CMD_FlashErase};

		if((buf[11]==szCmd)&&(memcmp(cBuf,buf,10)==0) /*&&(memcmp(&buf[12],&addr,4)==0)*/){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			return(1);
		}

	
		
		return(0);
}

int CheckRespond_FlashWrite4K(uint8 *buf,uint32 addr,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,1+1+(4),
			0x00,BK3266CMD_FlashWrite4K};

		int i;
		
//		DEBUG_ERR(" addr =0x%x",addr);
		for (i = 0;i < rx_repond;i++){
			if (buf[i] == cBuf[0])
				if(memcmp(cBuf,&buf[i],10)==0){
//					DEBUG_ERR("find a data");
					return 1;
				}
		}	

			
		if(/*(memcmp(&buf[11],&addr,4)==0)&&*/(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			return(1);
		}
		return(0);
}
int CheckRespond_FlashRead4K(uint8 *buf,uint32 addr,uint8**pdat,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,(1+1+(4+4*1024))&0xff,
			((1+1+(4+4*1024))>>8)&0xff,BK3266CMD_FlashRead4K};



			
		if(/*(memcmp(&buf[11],&addr,4)==0)&&*/(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			if(pdat)*pdat=&buf[15];
			return(1);
		}
		return(0);
}
int CheckRespond_FlashWrite(uint8 *buf,uint32 addr,uint8 *status,int *sz){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,(1+1+(4+1))&0xff,
			((1+1+(4+1))>>8)&0xff,BK3266CMD_FlashWrite};
	
		if(/*(memcmp(&buf[11],&addr,4)==0)&&*/(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			if(sz)*sz=buf[15];
			return(1);
		}
		return(0);
}
int CheckRespond_FlashRead(uint8 *buf,uint32 addr,uint16 lenObj,uint8**dat,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,(1+1+(4+lenObj))&0xff,
			((1+1+(4+lenObj))>>8)&0xff,BK3266CMD_FlashRead};
		if((memcmp(&buf[11],&addr,4)==0)&&(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			if(dat)*dat=&buf[15];
			return(1);
		}
		return(0);
}
int CheckRespond_FlashReadSR(uint8 *buf,uint8 regAddr,uint8*status,uint8*sr){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,(1+1+(1+1))&0xff,
			((1+1+(1+1))>>8)&0xff,BK3266CMD_FlashReadSR};
// 		uint8 tbuf[32];
// 		memcpy(tbuf,buf,13);
		if((regAddr==buf[11])&&(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			if(sr)*sr=buf[12];
			return(1);
		}
		return(0);
}

int CheckRespond_FlashWriteSR(uint8 *buf,uint8 regAddr,uint8 val,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,(1+1+(1+1))&0xff,
			((1+1+(1+1))>>8)&0xff,BK3266CMD_FlashWriteSR};
		if((val==buf[12])&&(regAddr==buf[11])&&(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			return(1);
		}
		return(0);
}

int CheckRespond_FlashWriteSR2(uint8 *buf,uint8 regAddr,uint16 val,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,(1+1+(1+2))&0xff,
			((1+1+(1+2))>>8)&0xff,BK3266CMD_FlashWriteSR};
		if(((val&0xff)==buf[12])&&(((val>>8)&0xff)==buf[13])&&(regAddr==buf[11])&&(memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			return(1);
		}
		return(0);
}

int CheckRespond_FlashGetMID(uint8 *buf,uint32*val,uint8*status){
	uint8 cBuf[256]={
		0x04,0x0e,0xff,0x01,
			0xe0,0xfc,0xf4,(1+4)&0xff,
			((1+4)>>8)&0xff,BK3266CMD_FlashGetMID};
		// 		uint8 tbuf[32];
		// 		memcpy(tbuf,buf,13);
		if((memcmp(cBuf,buf,10)==0)){
			uint32 t;
			if(status){
				*status=buf[10];
			}
			uint32*p32=(uint32*)&buf[11];
			if(val)*val=*p32;
			return(1);
		}
		return(0);
}

uint8 GetRespond_CmdType(uint8 *buf){
	if(buf[2]==0xff){
		return(buf[9]);
	}else{
		return(buf[6]);
	}
}

int CalcRxLength_LinkCheck(){
	return(3+3+1+1+0);
}
int CalcRxLength_ReadReg(){
	return(3+3+1+4+4);
}
int CalcRxLength_WriteReg(){
	return(3+3+1+4+4);
}
int CalcRxLength_SetBaudRate(){
	return(3+3+1+4+1);
}
int CalcRxLength_CheckCRC(){
	return(3+3+1+4);
}

int CalcRxLength_StayRom(){
	return(3+3+1+1);
}

int CalcRxLength_SysErr(){
	return(3+3+1);
}
int CalcRxLength_FlashEraseAll(){
	return(3+3+3+(1+1+(1+0)));
}
int CalcRxLength_FlashErase4K(){
	return(3+3+3+(1+1+(4+0)));
}

int CalcRxLength_FlashErase(){
	return(3+3+3+(1+1+(1+4)));
}

int CalcRxLength_FlashWrite4K(){
	return(3+3+3+(1+1+(4+0)));
}
int CalcRxLength_FlashRead4K(){
	return(3+3+3+(1+1+(4+4*1024)));
}
int CalcRxLength_FlashWrite(){
	return(3+3+3+(1+1+(4+1)));
}
int CalcRxLength_FlashRead(uint16 lenObj){
	return(3+3+3+(1+1+(4+lenObj)));
}
int CalcRxLength_FlashReadSR(){
	return(3+3+3+(1+1+(1+1)));
}

int CalcRxLength_FlashWriteSR(){
	return(3+3+3+(1+1+(1+1)));
}
int CalcRxLength_FlashWriteSR2(){
	return(3+3+3+(1+1+(1+2)));
}
int CalcRxLength_FlashGetID(){
	return(3+3+3+(1+1+(4)));
}




void StartCmd_CheckLink()
{
	uint8 txbuf[32]={0};
	int l=BuildBK3266Cmd_LinkCheck(txbuf);
	int i = 0;
//	for (i = 0;i < l;i++)
//		DEBUG_ERR("write 0x%x ",txbuf[i]);
	m_uart_rxLen=CalcRxLength_LinkCheck();
	m_uart_ptr=0;
	m_erase_to=3;
	write_data(Uartfd,txbuf,l);
// 	Sleep(2);
}

void StartCmd_StayRom()
{
	uint8 txbuf[32]={0};
	int l=BuildBK3266Cmd_StayRom(txbuf);
	m_uart_rxLen=CalcRxLength_StayRom();
	m_uart_ptr=0;
	m_erase_to=3;
	int ret,i= 0;
	ret = write_data(Uartfd,txbuf,l);
//	DEBUG_ERR(":%d ",ret);
//	for (i = 0;i < ret;i++)
//		DEBUG_ERR("StartCmd_StayRom 0x%x ",txbuf[i]);
// 	Sleep(2);
}

void StartCmd_Write4K(uint32 addr,uint8*dat)
{
	uint8 txbuf[5*1024]={0};
	int m_uart_baudrate=115200;
	int l=BuildBK3266Cmd_FlashWrite4K(txbuf,addr,dat);
	m_uart_rxLen=CalcRxLength_FlashWrite4K();
	m_uart_ptr=0;
	m_erase_to=1000/(4110*10*1000/m_uart_baudrate);
	int ret,i;
	
//	pthread_mutex_lock(&pmMUT);
	ret = write_data(Uartfd,txbuf,l);
//	DEBUG_ERR("l = %d ret = %d",l,ret);

//	pthread_mutex_unlock(&pmMUT);
	while(ret != l){
		ret = write_data(Uartfd,txbuf,l);
		DEBUG_ERR("l = %d ret = %d",l,ret);
		usleep(5000);
	}
}

int UartWaitRespond()
{
	int k;
	int i = 0;
	//if(m_rs232.WaitCommRxEvent()==0)return(0);
//	DEBUG_ERR("respond");
	k=read_data(Uartfd,u_uart_rxbuf,4096);
//	DEBUG_ERR("ret = %d",k);
	rx_repond = k;
//	for (i = 0;i < k;i++)
//		DEBUG_ERR("0x%x",u_uart_rxbuf[i]);
	if(k > 0)
		return (1);//���յ���Ӧ
	else
		return -1;
	return(0);//���ڵȴ�
}

unsigned char write_uart_rxbuf[16] = "";

int WriteUartWaitRespond()
{
	int k;
	int i = 0;
//	pthread_mutex_unlock(&pmMUT);
//	DEBUG_ERR("m_uart_rxLen-m_uart_ptr = %d",m_uart_rxLen-m_uart_ptr);

	k=read_data(Uartfd,write_uart_rxbuf,m_uart_rxLen-m_uart_ptr);
//	pthread_mutex_lock(&pmMUT);
//	DEBUG_ERR("ret = %d",k);
	rx_repond = k;
//	for (i = 0;i < k;i++)
//		DEBUG_ERR("0x%x",write_uart_rxbuf[i]);
	if(k > 0)
		return (1);//���յ���Ӧ
	else
		return -1;
	return(0);//���ڵȴ�
}

void Do_3266ResetCmd()
{
	uint8 txbuf[32]={0};
	int l=BuildBK3266Cmd_Reset(txbuf);
	int i;
	int k,ret;
	m_uart_rxLen=8;
	m_uart_ptr=0;	
	for(i=0;i<2;i++){
		usleep(500);
		ret = write_data(Uartfd,txbuf,l);
		
//		for (k = 0;k < ret;k++)
//			DEBUG_ERR("write 0x%x",txbuf[k]);
//		usleep(100);
	}
}

void Start3266Cmd(uint8 *buf, int txl, int rxl)
{
	m_uart_rxLen=rxl;
	m_uart_ptr=0;
	int ret;
	ret = write_data(Uartfd,buf,txl);
//	int i = 0;

//	for (i = 0;i < ret;i++)
//		DEBUG_ERR("Start3266Cmd write 0x%x",buf[i]);
}


int Write3266Reg(uint32 regAddr, uint32 val)
{
	uint8 txbuf[32];
	memset(txbuf,0,32);
	int l=BuildBK3266Cmd_WriteReg(txbuf,regAddr,val);
	m_uart_retry=NUM_RETRY_PROC;
	int i;
	for(i=0;i<m_uart_retry;i++){
		Start3266Cmd(txbuf,l,CalcRxLength_WriteReg());
		usleep(5000);
		int r=WriteUartWaitRespond();
		if(r>0){
			if(CheckRespond_WriteReg(write_uart_rxbuf,regAddr,val)>0){
				return 1;
			}
		}
	}
	return 0;
}


int Do_3266OpenWDT()
{

	if(Write3266Reg(BK3266_WDT_CONFIG,(0x5a0000|0xffff))>0){
		return(Write3266Reg(BK3266_WDT_CONFIG,0xa50000|0xffff));
	}

	return 0;
}

int Do_3266FeedWDT()
{

	if(Write3266Reg(BK3266_WDT_CONFIG,(0x5a0000|0xffff))>0){
		return(Write3266Reg(BK3266_WDT_CONFIG,0xa50000|0xffff));
	}

	return(0);

}

void StartCmd_FlashErase64K(uint32 addr)
{
	uint8 txbuf[32]={0};
	int ret;
	int l=BuildBK3266Cmd_FlashErase(txbuf,addr,0xd8);
	m_uart_rxLen=CalcRxLength_FlashErase();
	m_uart_ptr=0;
	m_erase_to=300/5;
	ret = write_data(Uartfd,txbuf,l);
	int i = 0;

//	for (i = 0;i < ret;i++)
//		DEBUG_ERR("StartCmd_FlashErase64K write 0x%x",txbuf[i]);
}

void StartCmd_FlashErase4K(uint32 addr)
{
	uint8 txbuf[32]={0};
	int l=BuildBK3266Cmd_FlashErase4K(txbuf,addr);
	m_uart_rxLen=CalcRxLength_FlashErase4K();
	m_uart_ptr=0;
	m_erase_to=50;
	int ret = write_data(Uartfd,txbuf,l);
	int i = 0;

//	for (i = 0;i < ret;i++)
//		DEBUG_ERR("StartCmd_FlashErase4K write 0x%x",txbuf[i]);
	
}

int Do_3266Erase1Sect(uint32 addr)
{
	int i,j;
	int r;
	uint8 stt;
	uint8*pbuf=NULL;
	for(i=0;i<100;i++){
		StartCmd_FlashErase4K(addr);
		for(j=0;j<100;j++){
			//usleep(150000);
			r=WriteUartWaitRespond();
			if(r > 0)break;
		}
		if(r>0){
			if(CheckRespond_FlashErase4K(write_uart_rxbuf,addr,&stt)>0){
				return(1);
			}
		}
	}
	return(0);
}

int Do_3266Erase1Block(uint32 addr)
{
	int i,j;
	int r;
	uint8 stt;
	uint8*pbuf=NULL;
#if 0
	int flags;
	flags = fcntl(Uartfd,F_GETFL,0);
	flags &= ~O_NONBLOCK;
	fcntl(Uartfd,F_SETFL,flags);
#endif
	for(i=0;i<100;i++){
		StartCmd_FlashErase64K(addr);
		for(j=0;j<4;j++){
			usleep(100000);
			r=WriteUartWaitRespond();
			if(r > 0)break;
		}
		if(r>0){
			if(CheckRespond_FlashErase(write_uart_rxbuf,addr,0xd8,&stt)>0){
				return(1);
			}
		}
	}
	return(0);
}


int Do_3266Write1Sector(uint32 addr, uint8 *buf)
{
	int i;
	int r;
	uint8 stt;
	int flags;

	for(i=0;i<10;i++){
		StartCmd_Write4K(addr,buf);		
		while(1){
			usleep(100000);
			r=WriteUartWaitRespond();
			if(r>0)break;
		}
		if(r>0){
			if(CheckRespond_FlashWrite4K(write_uart_rxbuf,addr,&stt)>0){
				return(1);
			}
		}
	}
	return(-1);
}


int Do_3266WriteSector0(uint8 *buf)
{
	int i;
	int r;
	uint8 stt;
	for(i=0;i<10;i++){
		StartCmd_Write4K(0,buf);
		while(1){
			usleep(50000);
			r=WriteUartWaitRespond();
			if(r > 0)break;
		}
		if(r>0){
			if(CheckRespond_FlashWrite4K(write_uart_rxbuf,0,&stt)>0){
				return(1);
			}
		}
	}
	return(-1);
}

void StartCmd_RebootTartget()
{
	uint8 txbuf[32]={0};
	int l=BuildBK3266Cmd_Reboot(txbuf);
	write_data(Uartfd,txbuf,l);
	usleep(10000);
}



int GetFlashMID()
{
	int i;
	int r;
	uint8 stt;

	uint8 txbuf[32]={0};
	int l=BuildBK3266Cmd_FlashGetMID(txbuf,0x9f);
	m_uart_rxLen=CalcRxLength_FlashGetID();
	//write_data(Uartfd,txbuf,l);
	uint32 id;
	for(i=0;i<3;i++){
		write_data(Uartfd,txbuf,l);
		m_uart_ptr=0;
		m_erase_to=10;
		while(1){
			usleep(1000);
			r=WriteUartWaitRespond();
			if(r!=0)break;
		}
		if(r>0){
			if(CheckRespond_FlashGetMID(write_uart_rxbuf,&id,&stt)>0){
				return(id>>8);
			}
		}
	}
	return(-1);
}


int Do_3266StopWDT()
{

	if(Write3266Reg(BK3266_WDT_CONFIG,0x5a0000|0x0000)>0){
		return(Write3266Reg(BK3266_WDT_CONFIG,0xa50000|0x0000));
	}

	return(0);
}


void flash_write_addr_05(void)
{
	size_t rt_len=0;

	int ret,i = 0;
	int len;
	char erase_read[16] = {0};

	char write_5a[13] = {0x01, 0xe0 ,0xfc, 0x09,0x01,0x00,0x80,0xf4,0x00,0xff,0xff,0x5a,0x00};
	char write_a5[13] = {0x01, 0xe0 ,0xfc, 0x09,0x01,0x00,0x80,0xf4,0x00,0xff,0xff,0xa5,0x00};
	char write_f4_06[12] = {0x01, 0xe0, 0xfc, 0xff, 0xf4, 0x05, 0x00, 0x0b,  0x00, 0x00, 0x0f, 0x00};

	for (i = 0;i  < 16;i++){
		ret = write_data(Uartfd, write_5a, sizeof(write_5a));
		UartWaitRespond();
		ret = write_data(Uartfd, write_a5, sizeof(write_a5));
		UartWaitRespond();
		write_f4_06[9] = i;
		ret = write_data(Uartfd, write_f4_06, sizeof(write_f4_06));
		UartWaitRespond();
	}

}


void flash_write_addr_06(void)
{
	size_t rt_len=0;

	int ret,i = 0;
	int len;
	char erase_read[16] = {0};

	char write_5a[13] = {0x01, 0xe0 ,0xfc, 0x09,0x01,0x00,0x80,0xf4,0x00,0xff,0xff,0x5a,0x00};
	char write_a5[13] = {0x01, 0xe0 ,0xfc, 0x09,0x01,0x00,0x80,0xf4,0x00,0xff,0xff,0xa5,0x00};
	char write_f4_06[13] = {0x01, 0xe0, 0xfc, 0xff, 0xf4, 0x06, 0x00, 0x0f,  0xd8, 0x00, 0x00, 0x00,  0x00 };

	for (i = 0;i  < 16;i++){
		ret = write_data(Uartfd, write_5a, sizeof(write_5a));
		UartWaitRespond();
		ret = write_data(Uartfd, write_a5, sizeof(write_a5));
		UartWaitRespond();
		write_f4_06[11] = i;
		ret = write_data(Uartfd, write_f4_06, sizeof(write_f4_06));
		ret = read_data(Uartfd,erase_read,16);
		DEBUG_ERR("ret = %d",ret);
	//	UartWaitRespond();
		
	}

}

void StartCmd_WriteFlashSR(uint8 regCmd,uint16 sr,int sz)
{
	uint8 buf[64]={0};
	uint8 regAddr=regCmd;
	int l=BuildBK3266Cmd_FlashWriteSR(buf,regAddr,sr&0xff);
	m_uart_rxLen=CalcRxLength_FlashWriteSR();
	if(sz==2){
		l=BuildBK3266Cmd_FlashWriteSR2(buf,regAddr,sr);
		m_uart_rxLen=CalcRxLength_FlashWriteSR2();
	}
	m_uart_ptr=0;
	m_erase_to=10;
	m_uart_retry=3;
	write_data(Uartfd,buf,l);
	int k;
	uint8 stt;
	while(1){
		usleep(10000);
		k=UartWaitRespond();
		if(k<0){
			return;
		}
		if(k>0){
			if(sz==1){
				if(CheckRespond_FlashWriteSR(m_uart_rxBuf,regAddr,sr,&stt)>0){
					return;
				}
			}else{
				if(CheckRespond_FlashWriteSR2(m_uart_rxBuf,regAddr,sr,&stt)>0){
					return;
				}
			}
		}

	}
	
}

int StartCmd_ReadFlashSR(uint8 regAddr)
{
	uint8 buf[64]={0};
	//uint8 regAddr=0x05;
	int l=BuildBK3266Cmd_FlashReadSR(buf,regAddr);
	m_uart_ptr=0;
	m_uart_rxLen=CalcRxLength_FlashReadSR();
	m_erase_to=10;
	m_uart_retry=3;
	write_data(Uartfd,buf,l);
	int k;
	uint8 stt,sr;
	while(1){
		usleep(10000);
		k=UartWaitRespond();
		if(k<0){
			return(-1);
		}
		if(k>0){
			if(CheckRespond_FlashReadSR(m_uart_rxBuf,regAddr,&stt,&sr)>0){
				return(sr);
			}else{
				return(-2);
			}
		}
	}
	return(-1);
}


int Do_XTX_UnProtect(uint16 cw)
{

	int i;
	int sr;
	for(i=0;i<20;i++){
		sr=StartCmd_ReadFlashSR(0x05);
		if(sr>=0){
			if(sr==(cw&0xff))return(1);
		}
		StartCmd_WriteFlashSR(0x01,cw&0xff,1);
	}
	return(-1);
}

int Do_WH_UnProtect(uint16 cw)
{
	int i;
	int sr;
	for(i=0;i<20;i++){
		sr=StartCmd_ReadFlashSR(0x05);
		if(sr>=0){
			if(((sr))==(cw&0xff)){
				sr=StartCmd_ReadFlashSR(0x15);
				if(sr>=0){
					if(sr==((cw>>8)&0xff))return(1);
				}
			}
		}
		StartCmd_WriteFlashSR(0x01,cw,2);
	}
	return(-1);

}

int UnProtect3266()
{
	if(m_flashID==FLASH_ID_XTX_25F08B){
		return(Do_XTX_UnProtect(0x00));
	}
	if(m_flashID==FLASH_ID_MXIC_25V8035F){
		return(Do_WH_UnProtect(0x0800));
	}
}


void RW_flashsr(int ver)
{
	int ret;
	int len,i = 0;
	char read_buf[16] = "";
	char unprotect_02[9] = {0x01, 0xe0, 0xfc, 0xff, 0xf4, 0x02, 0x00, 0x0c, 0x05};
	char unprotect_03[10] = {0x01, 0xe0, 0xfc, 0xff,0xf4, 0x03, 0x00, 0x0d,  0x01, 0x00};
	char unprotect_03_3c[10] = {0x01, 0xe0, 0xfc, 0xff,0xf4, 0x03, 0x00, 0x0d,  0x01, 0x3c};
	char crc[13] = {0x01, 0xe0, 0xfc, 0x09,  0x10, 0x00, 0x00, 0x00,  0x00, 0xff, 0xcf, 0x0f,  0x00};
	if (ver == 0){
		ret = write_data(Uartfd, unprotect_02, sizeof(unprotect_02));
		m_uart_rxLen = 13;
		m_uart_ptr = 0;
		WriteUartWaitRespond();

	//	usleep(5000);

		ret = write_data(Uartfd, unprotect_03, sizeof(unprotect_03));
		WriteUartWaitRespond();	
	//	usleep(5000);

		ret = write_data(Uartfd, unprotect_02, sizeof(unprotect_02));
		WriteUartWaitRespond();
	}else{
	//	ret = write_data(Uartfd, crc, sizeof(crc));
	//	DEBUG_ERR("ret = %d",ret);
//		m_uart_rxLen = 11;
//		m_uart_ptr = 0;
	//	WriteUartWaitRespond();
	//	usleep(5000);

		ret = write_data(Uartfd, unprotect_02, sizeof(unprotect_02));
	//	DEBUG_ERR("ret = %d",ret);
		m_uart_rxLen = 13;
		m_uart_ptr = 0;
		WriteUartWaitRespond();

	//	usleep(5000);
	
		ret = write_data(Uartfd, unprotect_03_3c, sizeof(unprotect_03_3c));
	//	DEBUG_ERR("ret = %d",ret);

		WriteUartWaitRespond();	
	//	usleep(5000);

		ret = write_data(Uartfd, unprotect_02, sizeof(unprotect_02));
		DEBUG_ERR("ret = %d",ret);

		WriteUartWaitRespond();

	}
}

void sigalrm_fn(int sig)  
{  
	DEBUG_ERR("will reset btup");
	system("killall btup;btup");	  
	return;  
}  


int Proc_Download3266()
{
	uint32 addr=0xff000;
	int fd;
	int len,ret;
	struct stat st; 
	int PacketSize = 4*1024;
	int c = 0,count  = 0,Cursor = 0;
	float Step;
	char buf[PacketSize];
	char buf0[PacketSize];
	int j = 16,k = 0;
	int i = 0;
	char read_buf[12] = {0};
	int	index = 1;
	int handle; char string[40];
	int length, res;	
	char *normal_state = "0";
	char *error_state = "1";


	if (stat(F_PATH, &st) < 0){
	   printf("Open tmp/bt_upgrade.bin failed \n");

	   return -1;
	}
	fd = open(F_PATH, O_RDONLY);
	if (fd < 0)
	{
	   printf("Can't open tmp/bt_upgrade.bin\n");

	   return -1;
	}
	printf("Bt firwmare size = %d\n",st.st_size);
	Step = (float)st.st_size/100;


	
	int sz=m_pfile_len;
	if(m_pfile_len>=0xfd000){
		sz=0xfd000;
	}

	
	DEBUG_ERR("erase begin");
	/*开始擦除flash*/		

	cdb_set("$bt_update_status",error_state);
	system("cdb commit");
//	system("cdb fsave");


	for(m_write_addr=0;m_write_addr<0x100000-0x1000;){

		Do_3266FeedWDT();	
		signal(SIGALRM, sigalrm_fn);  //后面的函数必须是带int参数的
		alarm(5); 

		if(m_write_addr<0x100000-0x10000){
			
			fprintf(stderr,"\r erase flash:0x%x",m_write_addr);
			if(Do_3266Erase1Block(m_write_addr)<1)return(ERR_CODE_EraseFlash);
			m_write_addr+=0x10000;
		}else{
			fprintf(stderr,"\r erase flash:0x%x",m_write_addr);
		//	DEBUG_ERR("Do_3266Erase1Sect");
			if(Do_3266Erase1Sect(m_write_addr)<1)return(ERR_CODE_EraseFlash);
			m_write_addr+=0x1000;
		}
	}
	
	printf("\n");
	DEBUG_ERR("erase success");
	/*从0x1000开始写入到0xfd000截止*/


	m_write_ptr=0x1000;
	m_write_addr = 0x1000;




	while((len = read_data(fd, buf, PacketSize) )> 0){
		if(index == 1){
			memcpy(buf0,buf,4096);
			index++;
			continue;	
		}
		signal(SIGALRM, sigalrm_fn);  //后面的函数必须是带int参数的
		alarm(8); 

		Do_3266FeedWDT();
	
		if(Do_3266Write1Sector(m_write_addr,buf)<1)
			return(ERR_CODE_WriteFlash);
		m_write_ptr+=0x1000;
		m_write_addr += 0x1000;
		index++;

		c = count + PacketSize;
		while(c-Step*Cursor>-0.003){    
		   Cursor++;
		}
		count+=PacketSize;
		fprintf(stderr,"\r%d.0%%",Cursor);
		sprintf(string,"\r%d.0%%",Cursor);
		length = strlen(string);
		if ((handle = open(PROGRESS_DIR, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
		{
			printf("Error opening file.\n");
		//	exit(1);
		}
		
		if ((res = write(handle, string, length)) != length)
		{
			printf("Error writing to the file.\n");
		//	exit(1);
		}
		
	}
	close(handle);

	buf0[0x190] = 0x55;			//bt up flage,will be fix  when restart bt
	buf0[0x190+1] = 0xAA;

	if(Do_3266WriteSector0(buf0)<1)return(ERR_CODE_WriteFlash);
	

	Do_3266StopWDT();
	RW_flashsr(1);
//	cdb_set("$bt_update_status",normal_state);
//	system("cdb commit");

	for( i=0;i<3;i++){
		DEBUG_ERR("reboot system");
		StartCmd_RebootTartget();
	}
	
	cdb_set("$bt_update_status","0");
	system("cdb commit");
//	system("cdb fsave");
	sleep(2);
	system("reboot");

	return(ERR_CODE_Success);
}

void up_fail(void)
{
	int bt_status;
	bt_status = cdb_get_int("$bt_update_status",0);
	if ( bt_status != 1)
		system("myPlay /root/appresources/download_failed.mp3");
	cdb_set_int("$wifi_bt_update_flage",1);
	system("killall -9 ota");
}


void get_mcu_reponed()
{
	int i;
	int ret;
	i = 100;

	while(i--){
		write_data(Uartfd, AT_UP_3266, strlen(AT_UP_3266)); 
		m_uart_rxLen=13;
		m_uart_ptr=0;
		usleep(1);
		ret = WriteUartWaitRespond();
		if ((strcmp(write_uart_rxbuf,"MCU+RES+3266&") == 0)){
			DEBUG_ERR("MCU+RES+3266&");
			break;
		}else{
		//	DEBUG_ERR("error");
		}
	}
	if (i <= 0){
		DEBUG_ERR("Mcu not responed");
	//	up_fail();
	//	return 0;
	//	exit(0);
	}
}


int stayrom = 0;
void write_to_mcu(void)
{
	
	int ret;
	int resetCnt = 100;
	int id;
	int i;
	i = 30;
	int k = 0;
	
	set_gpio_value(GPIO_RESET_PIN,1);

	
	while(i--){
#if 1
		int flags;
		flags = fcntl(Uartfd,F_GETFL,0);
		flags |= O_NONBLOCK;
		fcntl(Uartfd,F_SETFL,flags); //fei阻塞
#endif

//		StartCmd_CheckLink();
//		ret = UartWaitRespond();

		if(1){
			if(1){
				//DEBUG_ERR("link success");
				get_mcu_reponed();
			//	goto exit;			
				DEBUG_ERR("*******Enter Rom******");

				StartCmd_StayRom();
				m_uart_rxLen=1;
				m_uart_ptr=0;
				WriteUartWaitRespond();
				tcflush(Uartfd, TCIOFLUSH);
#if 1
				int flags;
				flags = fcntl(Uartfd,F_GETFL,0);
				flags &= ~O_NONBLOCK;
				fcntl(Uartfd,F_SETFL,flags); //阻塞
#endif	
				StartCmd_StayRom();
				m_uart_rxLen=8;
				m_uart_ptr=0;
			//	i = 1000;
				fd_set readSet;
				struct timeval timeOut;
			
				while(1){
					FD_ZERO(&readSet);
					FD_SET(Uartfd, &readSet);
					timeOut.tv_sec = 0;
					timeOut.tv_usec = 1;
					
					select(Uartfd + 1, &readSet, NULL, NULL, &timeOut);
					//select(fd + 1, &readSet, NULL, NULL, NULL);
					
					if(FD_ISSET(Uartfd, &readSet))
					{
						ret = WriteUartWaitRespond();
						if(ret > 0){
							if(CheckRespond_StayRom(write_uart_rxbuf)>0){
								DEBUG_ERR("Stay Rom pass");
							
								Do_3266OpenWDT();
								DEBUG_ERR("Open WDT");
							//	return 0;
								id = GetFlashMID();
								DEBUG_ERR("FlashMIDid = 0x%x",id);
							//	Do_3266StopWDT();
								RW_flashsr(0);
								if(UnProtect3266()<1){
									DEBUG_ERR("UnProtect3266  < 1");
									return(ERR_CODE_UnProtect);
							
								}
								int err=Proc_Download3266();				
			
								return 0;
							}else{
								StartCmd_StayRom();
								usleep(1000);
								stayrom++;
								fprintf(stderr,"\r1Try to connect times = %d",stayrom);
								if (stayrom >= 100){
									stayrom = 0;
									break;
								}					
							}
			
						}
					}else{
						
						StartCmd_StayRom();
						usleep(1000);
						stayrom++;
						fprintf(stderr,"\r2Try to connect times = %d",stayrom);
						if (stayrom >= 100){
							stayrom = 0;
							break;
						}
			
					}
			
				}

			}
		}				
	
	}
	if (i <= 0){
		set_gpio_value(GPIO_RESET_PIN,0);
		DEBUG_ERR("link failed");
		up_fail();
		return 0;
	}
	
exit:



	return 0;

}

void read_from_mcu()
{

	

}

void thread_create(void)
{
	int tmp;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,PTHREAD_STACK_MIN*2);
	memset(&thread,0,sizeof(thread));

	if (tmp = pthread_create(&thread[0],&attr,write_to_mcu,NULL) != 0){
		DEBUG_ERR("create write_to_mcu faile");
	}else{
		DEBUG_ERR("create write_to_mcu success");
	}
	
	if (tmp = pthread_create(&thread[1],&attr,read_from_mcu,NULL) != 0){
		DEBUG_ERR("create read_from_mcu faile");
	}else{
		DEBUG_ERR("create read_from_mcu success");
	}
}


void thread_wait(void)
{
	if (thread[1] != 0){
		pthread_join(thread[1],NULL);
		DEBUG_INFO("thread 1 read end");
	}


	if (thread[0] != 0){
		pthread_join(thread[0],NULL);
		set_gpio_value(GPIO_RESET_PIN,0);
		DEBUG_INFO("thread 0 write end");
	}

}

void main()
{
	int ret = -1;
	int err = 0;

	cdb_set("$uartd_nocheck","1");
	
	err = set_gpio_dir(GPIO_RESET_PIN);
	if(err < 0){
		DEBUG_ERR("err = %d",err);
	}

	system("killall -9 uartd");
	sleep(2);
	setopt_opentty(115200,8,0,1);

	thread_create();
	thread_wait();

	
	return 0;

}
