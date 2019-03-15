#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <errno.h>

#include "filepathnamesuffix.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_FifoCommunication.h"
#include "WIFIAudio_Semaphore.h"


#define READ_BUFFER_SIZE    4096
#define BOUNDRY_STRING      "boundary="

#define CHAR_LF 0xa
#define CHAR_CR 0xd

struct request_info
{
	//filename
	char name_info[PATH_MAX];
	char path_phys[PATH_MAX];
	
	//分隔符
	char boundary[PATH_MAX];
	//分隔符长度
	unsigned long blen;
	//传输内容长度
	unsigned long len;
};

//查找字符串起点，开始的指针，从指针指向开始的长度，字符串
//没找到返回0，找到返回大于0的正整数，为分隔符开始的指针
char *httpd_check_pattern(char *data, int len, char *pattern)
{
	//游标
	int offset=0;

	//游标还小于需要处理的长度内
	while(offset < len)
	{
		//memcmp的效率比strcmp的高
		//这边虽然是在寻找分隔符，但是也不能直接使用strstr，主要是这个不是个字符串，万一遇到个'\0'
		if(memcmp( data+offset, pattern, strlen(pattern)) == 0)
			return data+offset;
		else
			offset++;
	}

	return 0;
}

//失败返回NULL，成功返回字段数据开始指针
char * httpd_get_filename(char *buf, int len, struct request_info *info)
{
	//\r\n\r\n，5个字节长度
	char null_line[] = { CHAR_CR, CHAR_LF, CHAR_CR, CHAR_LF, 0 };
	char *ptr = 0;
	char *start = buf;
	char *next;
	char *var;
	
	//检查分隔符起点
	if(!(next = httpd_check_pattern(start, len, info->boundary)))
	{
		//没找到返回0，分隔符直接跳出大循环
		//找到了，则next为分隔符开始的位置
		return ptr;
	}

	//指针指向分隔符后
	next += info->blen;
	//如果分隔符后面连着两个--，说明为结束符，字节退出循环，返回0
	if((*next == '-') && *(next+1)=='-')
	{
		return ptr;
	}

	//跳过两个字节，\r\n，在分隔符后后有这两个字符
	next += 2;
	//计算剩余处理长度
	len -= (next - start);
	//重置开始的位置，指向分隔符\r\n后
	start = next;

	if((var = httpd_check_pattern(start, len, "filename=\"")) != NULL )
	{
		if((ptr = httpd_check_pattern(start, len, null_line)) != NULL)
		{
			//\r\n\r\n用来分割字段信息和字段数据
			//ptr指向字段数据开始
			ptr = ptr + strlen(null_line);
		}
		
		//跳过filename="的长度，指向filename的内容开始的地方
		var += 10;
		if((next = strchr( var, '"')) != 0)
		{
			//将filename内容后"调换成\0
			*next = '\0';
			strcpy(info->name_info, var);
		}
	}

	return ptr;
}

//截断文件
void truncate_file(char *path, int writelen)
{
	FILE *fp = NULL;
	//可读可写的打开下载的文件
	if( (fp = fopen(path, "r+")) )
	{
		//改变文件大小
		ftruncate(fileno(fp), writelen);
		fflush(fp);
		fsync(fileno(fp));
		fclose(fp);
	}
}

int WIFIAudio_PostHtmlUpdate_StartUpdateBluetooth(void)
{
	int ret = 0;
	int fdfifo = 0;
	WAFC_pCmdParm pcmdparm = NULL;
	WAFC_pFifoBuff pfifobuff = NULL;


	fdfifo = WIFIAudio_FifoCommunication_OpenWriteOnlyFifo(WIFIAUDIO_FIFO_PATHNAME);

	if(fdfifo > 0)
	{
		pfifobuff = WIFIAudio_FifoCommunication_NewBlankFifoBuff(1024);
		if(NULL != pfifobuff)
		{
			pcmdparm = (WAFC_pCmdParm)calloc(1, sizeof(WAFC_CmdParm));
			if(NULL != pcmdparm)
			{
				pcmdparm->Command = WIFIAUDIO_FIFOCOMMUNICATION_COMMAND_UPDATEBLUETOOTH;
				WIFIAudio_FifoCommunication_CmdParmToFifoBuff(pfifobuff, pcmdparm);
				
				//写入长度小于等于0
				if(WIFIAudio_FifoCommunication_WriteFifoBuffToFifo(fdfifo, pfifobuff, 1024) <= 0)
				{
					ret = -1;
				}
				
				WIFIAudio_FifoCommunication_FreeCmdParm(&pcmdparm);
			}

			WIFIAudio_FifoCommunication_FreeFifoBuff(&pfifobuff);
			pfifobuff = NULL;
		}

		close(fdfifo);//将管道关闭
		fdfifo = 0;
	}

	return ret;
}

int WIFIAudio_PostHtmlUpdate_WriteProgress(char *pFileName, key_t key, char progress)
{
	int ret = 0;
	int sem_id = 0;
	FILE *fp = NULL;
	
	if(NULL == pFileName)
	{
		ret = -1;
	}
	else
	{
		sem_id = WIFIAudio_Semaphore_CreateOrGetSem((key_t)key, 1);
		WIFIAudio_Semaphore_Operation_P(sem_id);
		
		if(NULL == (fp = fopen(pFileName, "a+")))//文件内容会覆盖，不存在则创建,可读
		{
			ret = -1;
		}
		else
		{
			if(progress < 0)
			{
				progress = 0;
			}
			
			if(progress > 100)
			{
				progress = 100;
			}
			
			fseek(fp, 0, SEEK_SET);
			fwrite(&progress, 1, 1, fp);
			
			fclose(fp);
			fp = NULL;					
		}
		
		WIFIAudio_Semaphore_Operation_V(sem_id);
	}

	return ret;
}

int WIFIAudio_PostHtmlUpdate_GetWiFiAndBluetoothUpgradeState(void)
{
	int State = 0;
	char TempBuff[128];
	FILE *fp = NULL;
	
	fp = popen("cdb get wifi_bt_update_flage", "r");
	if(NULL == fp)
	{
		State = -1;
	}
	else
	{
		memset(TempBuff, 0, sizeof(TempBuff));
		if(NULL != fgets(TempBuff, sizeof(TempBuff), fp))
		{
			State = atoi(strtok(TempBuff, "\r\n"));
		}
		pclose(fp);
		fp = NULL;
	}
	
	return State;
}

int WIFIAudio_PostHtmlUpdate_IsWiFiAndBluetoothUpgrade()
{
	int ret = 0;
	int State = 0;
	char TempBuff[128];
	FILE *fp = NULL;
	
	State = WIFIAudio_PostHtmlUpdate_GetWiFiAndBluetoothUpgradeState();
	
	if((4 <= State) && (State <= 9))
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	
	return ret;
}

void WIFIAudio_PostHtmlUpdate_SetWiFiAndBluetoothUpgradeState(int state)
{
	char TempBuff[128];
	
	memset(TempBuff, 0, sizeof(TempBuff));
	sprintf(TempBuff, "cdb set wifi_bt_update_flage %d", state);
	
	system(TempBuff);
}

int main(int argc, char **argv)
{
	char read_buf[READ_BUFFER_SIZE+1];//4096字节
	struct request_info info;
	FILE *output_fp = NULL;
	char *buf;
	unsigned long len;
	unsigned long readlen = 0;
	unsigned long writelen = 0;
	int short_file = 0;
	char errorBuf[200]="";

#ifdef WIFIAUDIO_DEBUG
	printf("Content-Type:text/html\n\n");
#endif
	
	//这里无需上锁，整个上传过程中
	//web是被占用的，不会同时触发
	//只要在上传之后，将标志位写上就好
	if(1 == WIFIAudio_PostHtmlUpdate_IsWiFiAndBluetoothUpgrade())
	{
		printf(errorBuf,"%s","Upload error!");
		WIFIAudio_RetJson_retJSON("FAIL");
		goto error;
	}

	//给读buff最后一个字节写0，这样每次都4096个字节都不需要管最后面个结束符的事情了
	read_buf[READ_BUFFER_SIZE] = '\0';

	//自定义结构体置零
	memset(&info, 0, sizeof(struct request_info));

	//获取内容长度
	buf = getenv("CONTENT_LENGTH");
	if(buf)
	{
		//转换内容长度为整型
		info.len = atoi(buf);

		//获取传递信息的MIME类型
		buf = getenv("CONTENT_TYPE");
		if(buf == NULL)
		{
			printf(errorBuf,"%s","Upload error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		
		//如果找不到"multipart/form-data"，即不是二进制数据，就直接退出
		if(!strstr(buf, "multipart/form-data"))
		{
			printf(errorBuf,"%s","Upload error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		//如果找不到分隔符"boundary="，就直接退出
		if(NULL == (buf = strstr(buf, BOUNDRY_STRING)))
		{
			printf(errorBuf,"%s","Upload error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		else
		{
			//另buf指针指向"boundary="的'='后面那个位置
			buf += strlen(BOUNDRY_STRING);
		}
		//将分隔符记录下来
		strcpy(info.boundary, buf);
		//将分隔符长度记录下来
		info.blen = strlen(buf);


		while(1)
		{
			//从标准输入当中读取4096字节到读缓存当中
			len = fread(read_buf, 1, READ_BUFFER_SIZE, stdin);

			if(len < 0)
			{
				sprintf(errorBuf,"%s","Save error!");
				WIFIAudio_RetJson_retJSON("FAIL");
				goto error;
			}
			//已经读取完了，或者标准输入当中已经没有数据了
			if(len == 0)
				break;

			//一开始初始化的时候就是为0，也只有这一次为0
			//循环当中，第一次进来的时候，进入这个判断分支
			if(readlen == 0)
			{
				if(NULL == (buf = httpd_get_filename(read_buf, len, &info)))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				
				sprintf(info.path_phys, WIFIAUDIO_UPDATEBLUETOOTH_PATHNAME);
				//sprintf(info.path_phys, "%s%s", TEMP_PATH, info.name_info);
				//只写的打开文件，终于要开始写文件了
				if(NULL == (output_fp = fopen(info.path_phys, "w")))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				//buf在经过上面httpd_get_filename的内部遍历之后
				//已经指向name="files"段的数据了
				//将数据的开始部分直接写入文件
				if(0 == fwrite(buf, len - (buf - read_buf), 1, output_fp))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				writelen += len - (buf - read_buf);
			}
			else
			{
				if(0 == fwrite(read_buf, len, 1, output_fp))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				writelen += len;
			}
			//处理过的总长度，加上本次读取处理的长度
			readlen += len;
		}

		if(NULL != output_fp)
		{
			fclose(output_fp);
			output_fp = NULL;
		}

		/* trim the output file to proper boundary */
		if(NULL == (output_fp = fopen(info.path_phys, "r")))
		{
			sprintf(errorBuf,"%s","Save error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}

		//移动到文件末尾4096个字节
		if(0 > fseek(output_fp, -READ_BUFFER_SIZE, SEEK_END))
		{
			//移动到开头
			if(0 > fseek(output_fp, 0, SEEK_SET))
			{
				sprintf(errorBuf,"%s","Save error!");
				WIFIAudio_RetJson_retJSON("FAIL");
				goto error;
			}
			else
			{
				//不能移动到末尾，字节移动到开头，记录为短文件
				short_file = 1;
			}
		}

		if(0 > (len = fread(read_buf, 1, READ_BUFFER_SIZE, output_fp)))
		{
			sprintf(errorBuf,"%s","Save error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}

		//因为写入文件的分隔符，有且只有末尾一个
		//前面都分隔符都不曾写入
		if(NULL == (buf = httpd_check_pattern(read_buf, len, info.boundary)))
		{
			sprintf(errorBuf,"%s","Save error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		buf -= 4;
		//查找最后一个分隔符，分隔符前面有\r\n--

		if(short_file)
		{
			//短文件，分隔符前面的长度即为文件所有长度
			writelen = (buf - read_buf);
		}
		else
		{
			//长文件，将文件实际内容后面的长度减掉
			writelen -= (READ_BUFFER_SIZE - (buf - read_buf));
		}

		if(NULL != output_fp)
		{
			fclose(output_fp);
			output_fp = NULL;
		}

		//将文件末尾多写入，多余的分隔符截断
		truncate_file(info.path_phys, writelen);
		
		WIFIAudio_PostHtmlUpdate_WriteProgress(\
		WIFIAUDIO_BLUETOOTHDOWNLOADPROGRESS_PATHNAME, \
		WIFIAUDIO_BLUETOOTHDOWNLOADPROGRESS_SEMKEY, (char)100);
		
		//真正上传完了才来，设置正在升级状态值
		//如果在开始的时候就设置值，中途文件断了
		//直接GG掉
		WIFIAudio_PostHtmlUpdate_SetWiFiAndBluetoothUpgradeState(9);
		
		if (-1 == WIFIAudio_PostHtmlUpdate_StartUpdateBluetooth())
		{
			sprintf(errorBuf,"%s","Updata error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		else
		{
			WIFIAudio_RetJson_retJSON("OK");
		}
	}
	else
	{
		//长度为0
		printf(errorBuf,"%s","Upload error!");
		WIFIAudio_RetJson_retJSON("FAIL");
		goto error;
	}
	
error:
	if(errorBuf[0]!='\0')
		//打印信息到网页的隐藏的iframe中
		//printf("<script type='text/javascript' language='javascript'>alert('%s');parent.init();</script>",errorBuf);
		printf("<script type='text/javascript' language='javascript'>parent.bluetooth_my_Error('%s');</script>",errorBuf);
	else  //printf("file upload success !<br>");
	{
		//printf("<script type='text/javascript' language='javascript'>alert('File upload success!');parent.init();</script>");
		printf("<script type='text/javascript' language='javascript'>parent.bluetooth_my_OK();</script>");
	}
	fflush(stdout);

	return 0;
}


