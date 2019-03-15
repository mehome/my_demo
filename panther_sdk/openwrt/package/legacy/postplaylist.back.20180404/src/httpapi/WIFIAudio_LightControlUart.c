#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//波特率
#include <termios.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "WIFIAudio_LightControlUart.h"
#include "WIFIAudio_Debug.h"


int Speed_Array[] = 
{
	B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
};

int Name_Array[] = 
{
	115200, 38400,  19200,  9600,  4800,  2400,  1200,  300,
	115200, 38400,  19200,  9600, 4800, 2400, 1200,  300,
};

void WIFIAudio_LightControlUart_SetSpeed(int fd, int Speed)
{
	int i = 0;
	//len计算一次就好了，不要放在for循环里面，增加耗时
	int Len = sizeof(Speed_Array) / sizeof(int);
	int status = 0;
	struct termios Opt;
	//获取终端相关参数，具体的我不管，我只要修改输入输出波特率就可以了
	tcgetattr(fd, &Opt);
	for(i = 0; i < Len; i++)
	{
		if(Speed == Name_Array[i])
		{
			//清空终端未完成的输入\输出请求数据
			tcflush(fd, TCIOFLUSH);
			//设置需要的输入波特率
			cfsetispeed(&Opt, Speed_Array[i]);
			//设置需要的输出波特率
			cfsetospeed(&Opt, Speed_Array[i]);
			//设置终端参数，不等数据传输完毕就立即生效
			status = tcsetattr(fd, TCSANOW, &Opt);
			if(status != 0)
			{
				DEBUG("tcsetattr fd1");
			}
			return;
		}
		
		//设置波特率期间，不允许传输数据
		tcflush(fd, TCIOFLUSH);
	}
}

int WIFIAudio_LightControlUart_SetParity(int fd, int DataBits, int StopBits, int Parity)
{
	struct termios options;
	//获取终端相关参数
	if(tcgetattr(fd,&options) != 0)
	{
		DEBUG("SetupSerial 1");
		return -1;
	}
	//字符长度，取值范围为CS5，CS6，CS7，CS8
	//先取消字符长度设置
	options.c_cflag &= ~CSIZE;
	switch(DataBits)/*设置数据位数*/
	{
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr,"Unsupported data size\n");
			return -1;
	}
	
	switch(Parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;		//取消奇偶校验
			options.c_iflag &= ~INPCK;		//取消输入奇偶校验校验使能
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);	//使用奇偶校验，对输入使用奇偶校验，对输出使用偶校验
			options.c_iflag |= INPCK;				//输入奇偶校验使能
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;		//使用奇偶校验
			options.c_cflag &= ~PARODD;		//取消对输入奇偶校验，对输出使用偶校验
			options.c_iflag |= INPCK;		//语序输入奇偶校验
			break;
		case 'S':
		case 's':
			options.c_cflag &= ~PARENB;		//取消奇偶校验
			options.c_cflag &= ~CSTOPB;		//取消两个停止位
			break;
		default:
			fprintf(stderr,"Unsupported parity\n");
			return -1;
	}

	switch(StopBits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;		//取消设置两个停止位
			break;
		case 2:
			options.c_cflag |= CSTOPB;		//设置另个停止位
			break;
		default:
			fprintf(stderr,"Unsupported stop bits\n");
			return -1;
	}

	if((Parity != 'n') || (Parity != 'N'))
	{
		//使用输入奇偶校验
		options.c_iflag |= INPCK;
	}
	
	options.c_iflag &= ~(IXON|IXOFF|IXANY|INLCR|IGNCR|ICRNL|ISTRIP);
	options.c_oflag |= OPOST;
	options.c_oflag &= ~(ONLCR|OCRNL);
	options.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
	
	//非规范模式读取时的超时时间
	options.c_cc[VTIME] = 150; // 15 seconds
	//非规范呢读取时的最小字符数
	options.c_cc[VMIN] = 0;

	//清除正收到的数据
	tcflush(fd, TCIFLUSH);
	//将刚才设置在结构体当中的参数再设置会终端当中
	if(tcsetattr(fd, TCSANOW, &options) != 0)
	{
		DEBUG("SetupSerial 3");
		return -1;
	}
	
	return 0;
}

int WIFIAudio_LightControlUart_OpenDevice(char *pDevice)
{
	int	fd = -1;
	
	if(NULL == pDevice)
	{
		
	}
	else
	{
		//可读可写
		//如果打开文件为终端机设备时，不会将该终端机当做进程控制终端机
		//无阻塞打开，无论有无数据读取或等待，会立即返回
		fd = open(pDevice, O_RDWR | O_NOCTTY | O_NDELAY);
		
		if(-1 == fd)
		{
			DEBUG("Can't Open Serial Port");
		}
		else
		{
			
		}
	}
	
	return fd;
}

//打开串口，不想调用那么多函数，全部封装在一起
int WIFIAudio_LightControlUart_OpenUart(char *pDevice, int Speed, int DataBits, int StopBits, int Parity)
{
	int	fd = -1;
	
	if(NULL == pDevice)
	{
		
	}
	else
	{
		fd = WIFIAudio_LightControlUart_OpenDevice(pDevice);
		
		if(0 >= fd)
		{
			DEBUG("Can't Open Serial Port");
		}
		else
		{
			//设置波特率
			WIFIAudio_LightControlUart_SetSpeed(fd, Speed);
			
			//设置结束位
			if(-1 == WIFIAudio_LightControlUart_SetParity(fd, DataBits, StopBits, Parity))
			{
				DEBUG("Set Parity Error");
				close(fd);
				fd = -1;
			}
			else
			{
				
			}
		}
	}
	
	return fd;
}



//释放一片空的串口buff
int WIFIAudio_LightControlUart_FreeppUartBuff(WFLCU_pUartBuff *ppUartBuff)
{
	int ret = 0;
	
	if ((NULL == ppUartBuff) || (NULL == (*ppUartBuff)))
	{
		DEBUG("Error of Parameter");
		ret = -1;
	} 
	else 
	{
		if(NULL != (*ppUartBuff)->pBuff)
		{
			free((*ppUartBuff)->pBuff);
			(*ppUartBuff)->pBuff = NULL;
		}
		
		free(*ppUartBuff);
		*ppUartBuff = NULL;
	}
	
	return ret;
}

//创建一个新的空的串口buff
WFLCU_pUartBuff WIFIAudio_LightControlUart_NewUartBuff(int Bufflen)
{
	WFLCU_pUartBuff pUartBuff = NULL;
	
	pUartBuff = (WFLCU_pUartBuff)calloc(1, sizeof(WFLCU_UartBuff));
	
	if(NULL == pUartBuff)
	{
		DEBUG("Error of Calloc");
	}
	else
	{
		pUartBuff->BuffLen = Bufflen;
		pUartBuff->CurrentLen = 0;
		pUartBuff->pBuff = (WFLCU_pBuff)calloc(Bufflen, sizeof(WFLCU_Byte));
		if(NULL == pUartBuff->pBuff)
		{
			WIFIAudio_LightControlUart_FreeppUartBuff(&pUartBuff);
		}
		else
		{
			//保险起见，将这个重置一下数据
			memset(pUartBuff->pBuff, 0, Bufflen);
		}
	}
	
	return pUartBuff;
}

//是否还有足够的空间可以写入，返回0就是有，返回其他值就是没有
int WIFIAudio_LightControlUart_IsHaveEnoughBuff(WFLCU_pUartBuff pUartBuff, int Len)
{
	int ret = 0;
	
	if(NULL == pUartBuff)
	{
		DEBUG("Error of Parameter");
		ret = -1;
	} 
	else 
	{
		if(Len > ((pUartBuff->BuffLen) - (pUartBuff->CurrentLen)))
		{
			//即将写入的长度大于空闲的长度
			ret = -1;
		}
		else
		{
			//小于等于空闲的长度
			ret = 0;
		}
	}
	
	return ret;
}

//从串口读取一段内容存入结构体当中，返回负值为读取出错
int WIFIAudio_LightControlUart_ReadUart(int fd, WFLCU_pUartBuff pUartBuff, int Len)
{
	int ret = 0;
	WFLCU_Byte Buff[Len];
	
	if(NULL == pUartBuff)
	{
		DEBUG("Error of Parameter");
		ret = -1;
	} 
	else 
	{
		//这边要先判断是否有空闲空间再读，免得从串口当中把数据读出来之后
		//又存不到结构体当中，直接丢失数据
		ret = WIFIAudio_LightControlUart_IsHaveEnoughBuff(pUartBuff, Len);
		if(0 == ret)
		{
			memset(Buff, 0, Len);
			ret = read(fd, Buff, Len);
			if(ret < 0)
			{
				DEBUG("Error of Read");
				ret = -1;
			}
			else
			{
				//上面已经判断有空间了，这边就大胆的写入就可以了
				memcpy((pUartBuff->pBuff) + (pUartBuff->CurrentLen), Buff, ret);
				pUartBuff->CurrentLen += ret;
			}
		}
	}
	
	return ret;
}

//从串口写入一段内容存入结构体当中，返回大于0为写入成功
int WIFIAudio_LightControlUart_WriteUart(int fd, WFLCU_pUartBuff pUartBuff)
{
	int ret = -1;
	
	if(NULL == pUartBuff)
	{
		DEBUG("Error of Parameter");
		ret = -1;
	} 
	else 
	{
		if((0 < pUartBuff->CurrentLen) && (NULL != pUartBuff->pBuff))
		{
			ret = write(fd, pUartBuff->pBuff, pUartBuff->CurrentLen);
			if(0 < ret)
			{
				memset(pUartBuff->pBuff, 0, pUartBuff->CurrentLen);
				pUartBuff->CurrentLen = 0;
			}
		}
	}
	
	return ret;
}



