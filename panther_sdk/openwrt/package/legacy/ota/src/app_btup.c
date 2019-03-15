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


#include <curl/curl.h>
#include <curl/easy.h> 

#include "app_btup.h"
#include "ota.h"
pthread_t thread;
pthread_mutex_t pmMUT;
extern int iUartfd;
static char AT_UP_DATE[]		="AXX+UP+DATE&";	//������������ģʽ
#define BUFF_LENGTH 32
#define PROGRESS_DIR "/tmp/progress"


const uint16_t wCRC_Table[256] =
{
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};



unsigned int CaculateCRC(unsigned char *byte, int count)
{
	unsigned int CRC = 0;
	int i;
	unsigned char j;
	unsigned char temp;
	for(i = 0;i < count;i++) 
	{
		temp = byte[i+3]; 
		for(j = 0x80;j != 0;j /= 2)
		{
			if((CRC & 0x8000) != 0){CRC <<= 1; CRC ^= 0x1021;}
			else CRC <<= 1;
			if((temp & j) != 0) CRC ^= 0x1021;
		}
	}	
	return CRC;
}

#define CAL_CRC(__CRCVAL,__NEWCHAR)             \
     {\
        (__CRCVAL) = ((uint16_t)(__CRCVAL) >> 8) \
            ^ wCRC_Table[((uint16_t)(__CRCVAL) ^ (uint16_t)(__NEWCHAR)) & 0x00ff]; \
     }

#define F_PATH "/tmp/bt_upgrade.bin"
uint8_t bt_mac_frame[6]={0};

uint16_t cal_crc(uint8_t *p,uint32_t len,uint16_t crc)
{
    if (p == NULL)
    {
        return 0xFFFF;
    }

    while(len--)
    {
        uint8_t chData = *p++;
        CAL_CRC(crc, chData);
    }

    return crc;
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


int reset_port(int fd)
{
#ifdef TERMIO	
    if (ioctl(fd, TCSETAW, &oterm_attr) < 0) {
       return -1;
    }
#else
		if ( tcsetattr(fd,TCSANOW, &oterm_attr) != 0)
		{
		   return -1;
		}
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
	         fprintf(stderr, "Read error %s\n", strerror(errno));
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
             fprintf(stderr, "Write error %s\n", strerror(errno));
             break;
         }

         count += ret;
         len -= ret;
     }

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



int set_opt(int baud, int databits, int parity, int stopbits,char *url)

{
     int iUartfd;	 
     int len;
     int i, n = 10;

     iUartfd = open("/dev/ttyS1", O_RDWR );
     if (iUartfd < 0) {
         fprintf(stderr, "open > error %S\n", strerror(errno));
         return 1;
     }

     if (setTermios(iUartfd, baud, databits, parity, stopbits)) {
         fprintf(stderr, "setup_port error %s\n", strerror(errno));
         close(iUartfd);
         return 1;
     }
		 
		 if( 1 ) {
		 	struct termios term_attr;
		 	
		 	tcgetattr( iUartfd, &term_attr);
     	
		 	printf("ISPEED = %ld\n", convert_uart_baud2(cfgetispeed(&term_attr)));
		 	printf("OSPEED = %ld\n", convert_uart_baud2(cfgetospeed(&term_attr)));
		}
/*
	   while ( n-- > 0 ) {
	   	  memset(buf, 0 ,sizeof(buf));
	   	  write_data(fd, "Hello, hello, ...", 17 );
	   	  printf("Write Successfully\n");
	   		len = read_data(fd, buf, 16 );
	   		
	      fprintf(stderr, "Recv %d bytes = %s\n", len, buf );
	      sleep(1);
	   }
*/
	
	   int BufLen;
	   int fd2,Length; 						   
	   int PacketSize = 4*1024;
	   char buf[3+PacketSize+2];
	   int index;
	   unsigned int CRC;
	   int j = 0;
	   int ret = 0;
	   int GetInBufferCount= -1;
	   unsigned char dataPtr[1] = {0};
	   struct stat st; 
	   FILE *fp;
	   int handle; 
	   char string[40];
	   int length, res;
	   int normal_state = 0;
	   int error_state = 1;
   
	   if (stat(url, &st) < 0)
	   {
		   printf("Open url:%s failed \n",url);
   
		   return -1;
	   }
	   fd2 = open(url, O_RDONLY);
	   if (fd2 < 0)
	   {
		   printf("Can't open url:%s bin\n",url);
   
		   return -1;
	   }
	   printf("Bt firwmare size = %d\n",st.st_size);
	   i = st.st_size%PacketSize;
	   if(i != 0)  BufLen = ((st.st_size/PacketSize) + 1) * PacketSize;
	   else BufLen = st.st_size;
	   if (st.st_size % (512*1024) != 0)//鍒ゆ柇钃濈墮鍥轰欢鏄惁姝ｇ‘
	   {
		   printf("Firmware size must be 512K integer multiples faile\n");
		   return -1;
	   }
	   unsigned char *Ptr;
	   uint16_t crc_val = 0;
	   char bufmac[18] = {0};
	   memset(buf,0,sizeof(buf));
	   index = 0;
	   int Tick =0;
	   char bufflash[1];
	   char eabuf[3];
	   char bufflash1[4*1024];
	   i = 0;
	   ret = write_data(iUartfd, AT_UP_DATE, strlen(AT_UP_DATE));
	   //	   printf("ret = %d",ret);
	   //		   sleep(2);
	   while(++Tick < 650*10000){
		   pthread_mutex_lock(&pmMUT);
		   ret = 0;
		   memset(bufmac,0,sizeof(bufmac));
		   ret = read_data(iUartfd,bufmac,sizeof(bufmac));
		   pthread_mutex_unlock(&pmMUT);
	   //	   DEBUG_INFO(" read data from mcu ret=%d",ret);	   
		   if (ret >= 18){
			   Ptr = bufmac;
			   ret = 0;
			   while((*Ptr) != 'C' && ret < 18){
					   Ptr++;
					   ret++;
			   }														   
			   if( (*Ptr) != 'C'){//67-->'C'
				   continue;
				}else {
			   /*  crc_val = cal_crc(Ptr,7,0x55aa);
				   if(((crc_val&0x0ff)==Ptr[7])&&((crc_val>>8)==Ptr[8])){
					   Ptr++;
					   memcpy(bt_mac_frame,Ptr,6);
					}*/
				   for ( i = 0;i < 6;i ++){
					   bt_mac_frame[i] = *(++Ptr);
					   printf(" %02x ",bt_mac_frame[i]);
						   
				   }
				   printf("\n get bt mac  success.begin erase flash \n");
					break;
				   }		   
		   }
	   }
	   if(Tick >= 650*10000){
		   printf("Get bt addr timeout\n");
		   cdb_set_int("$bt_update_status",normal_state);
		   cdb_set_int("$wifi_bt_update_flage",1);
		   return -1;
	   }
   
	   for(;;){
		   cdb_set_int("$bt_update_status",error_state);
		   eabuf[0] = 'E';
		   eabuf[1] = (st.st_size/1024)&0xff;
		   eabuf[2] = ((st.st_size/1024)>>8)&0xff;
		   pthread_mutex_lock(&pmMUT);
		   ret = 0;
		   ret = write_data(iUartfd,eabuf,3);
		   pthread_mutex_unlock(&pmMUT);
		   Tick =0;
   
		   GetInBufferCount = -1;
		   pthread_mutex_lock(&pmMUT);
		   while(++Tick <65*10000) 
		   {	   
			   usleep(1);
			   GetInBufferCount = read_data(iUartfd,bufflash,sizeof(bufflash));
			   Ptr = bufflash;
				if(*Ptr == 'E') {//69--> 'E'
				break;
				}
		   }
   
		   pthread_mutex_unlock(&pmMUT);
		   if(Tick >= 65*10000)    {
			   printf("erase flash timeout .try again\n");
			   cdb_set_int("$bt_update_status",normal_state);
			   cdb_set_int("$wifi_bt_update_flage",1);
			   return 0;
		   }
   
			if(*Ptr == 'E') //69--> 'E'
				break;
   
	   }   
   
	   printf("erase flash success \n");
	   index = 1;
	   int flage = 1,k;
	   float Step;
	   int Cursor = 0;
	   unsigned char bufrev[1] = {0};
	   Step = (float)BufLen/100;
	   uint16_t tf_crc_val;
	   int index_cnt = 1;
	   int write_num = 0;
	   int write_size = 0;
	   while((len = read_data(fd2, &buf[3], PacketSize) )> 0){
		   if (index >=5){//前16k为蓝牙BootLoader，直接跳过不写
	   /*  if (index_cnt == 252){  //进行最后的CRC校验
				tf_crc_val = cal_crc(&buf[3],PacketSize-2,0x55aa);
			   buf[PacketSize + 1] = (uint8_t)tf_crc_val;
			   buf[PacketSize + 2] = (uint8_t)(tf_crc_val>>8);
		   }else{
				tf_crc_val = cal_crc(&buf[3],PacketSize,0x55aa);
		   }*/
		   if (index == 66){   //在0x41063处写入获取的蓝牙地址，保持蓝牙地址与升级前一致
			   memcpy(&buf[3 + 96 +3],bt_mac_frame,6);
		   }
		   buf[0] = 0x01;//SOH
		   buf[1] = index_cnt;
		   buf[2] = ~index_cnt;
		   CRC = CaculateCRC(buf, PacketSize);
		   buf[3+PacketSize] = CRC>>8;
		   buf[3+PacketSize+1] = CRC;
		   pthread_mutex_lock(&pmMUT);
		   ret = write_data(iUartfd, buf, sizeof(buf)); 
		//   printf("11write:%d\n",ret);
		   pthread_mutex_unlock(&pmMUT);
   
		   GetInBufferCount = -1;
		   memset(bufrev,0,sizeof(bufrev));
		   Tick = 0;
		   pthread_mutex_lock(&pmMUT);
		   while(++Tick < 65*10000) {						   
			   usleep(1);
			   GetInBufferCount = read_data(iUartfd,bufrev,sizeof(bufrev));											   
			   Ptr = bufrev;
				if((*Ptr == 0x06) ||(*Ptr == 0x18)) {//ACK-->0x06  0x18-->CAN
					break;
				}
		   }
		   pthread_mutex_unlock(&pmMUT);
		   if(Tick >= 65*10000){
			   printf("*Ptr = %x  Transfer to the  %d frame timeout failed\n",*Ptr,index);
		   //  goto ack;
			   break;				   
		   }
		   if (GetInBufferCount > 0){
			   if(*Ptr != 0x06 && *Ptr != 0x18) //ACK-->0x06  0x18-->CAN
				{
				   printf("*Ptr = 0x%x Transfer to the	%d frame  failed\n",*Ptr ,index);
				   break;
				}
			   else if(*Ptr++ == 0x18 || *Ptr== 0x18)
				{
				   printf("CRC faile \n"); 
				}
		   }else{
			   printf("don't konw the faile\n");
		   }
   
	   k = i + PacketSize;
		while(k-Step*Cursor>-0.003){    
		   Cursor++;
	   /* Create a file named "TEST.$$$" in the current directory and write a string to it. If "TEST.$$$" already exists, it will be overwritten. */
	   if ((handle = open(PROGRESS_DIR, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
	   {
		   printf("Error opening file.\n");
		   exit(1);
	   }
	   sprintf(string,"%d.0%%",Cursor);
	   fprintf(stderr,"\r%s", string);
   //  printf("\n%s ",string); 
	   length = strlen(string);
	   if ((res = write_data(handle, string, length)) != length)
	   {
		   printf("Error writing to the file.\n");
		   exit(1);
	   }
	   //	printf("Wrote %d bytes to the file.\n", res);
	   close(handle);			   
	   }
		i += PacketSize;
		 memset(buf,0,sizeof(buf));
		 *Ptr = 0;
		 index_cnt++;
	   }
   
	   index++;
	   }
	   cdb_set_int("$bt_update_status",normal_state);
	   buf[0] = 0x04;//EOT
	   i = 0;
	   pthread_mutex_lock(&pmMUT);
	   write_data(iUartfd, buf, 1);
	   pthread_mutex_unlock(&pmMUT);
	   close(fd2);
	   dataPtr[0] = 100;
		   if ((handle = open(PROGRESS_DIR, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
	   {
		   printf("Error opening file.\n");
		   exit(1);
	   }
	   sprintf(string,"%d.0%%",dataPtr[0]);
	   printf("\n%s bt up success\n",string);  
	   length = strlen(string);
	   if ((res = write_data(handle, string, length)) != length)
	   {
		   printf("Error writing to the file.\n");
		   exit(1);
	   }
	   //	printf("Wrote %d bytes to the file.\n", res);
	   close(handle); 
	   

     //reset_port(fd);
     close(iUartfd);

     return 0;
}



