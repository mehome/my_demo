#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>

#include "libcchip/libcchip.h"
#include "WIFIAudio_RetJson.h"
#include "WIFIAudio_SystemCommand.h"

#define MAX_ARGVS    20
#define MAX_COMMAND_LEN		1024

#define READ_BUFFER_SIZE    4096
#define BOUNDRY_STRING      "boundary="

#define CHAR_LF 0xa
#define CHAR_CR 0xd

struct request_info
{
	char name_info[PATH_MAX];
	char path_phys[PATH_MAX];
	char boundary[PATH_MAX];
	unsigned long blen;
	unsigned long len;
};

char *httpd_check_pattern(char *data, int len, char *pattern)
{
	int offset=0;
	
	while(offset < len)
	{
		if(memcmp( data+offset, pattern, strlen(pattern)) == 0)
		{
			return data+offset;
		} else {
			offset++;
		}
	}

	return 0;
}

char * httpd_get_filename(char *buf, int len, struct request_info *info)
{
	char null_line[] = { CHAR_CR, CHAR_LF, CHAR_CR, CHAR_LF, 0 };
	char *ptr = 0;
	char *start = buf;
	char *next;
	char *var;
	
	if(!(next = httpd_check_pattern(start, len, info->boundary)))
	{
		return ptr;
	}
	
	next += info->blen;
	if((*next == '-') && *(next+1)=='-')
	{
		return ptr;
	}
	
	next += 2;
	len -= (next - start);
	start = next;

	if((var = httpd_check_pattern(start, len, "filename=\"")) != NULL )
	{
		if((ptr = httpd_check_pattern(start, len, null_line)) != NULL)
		{
			ptr = ptr + strlen(null_line);
		}
		
		var += 10;
		if((next = strchr( var, '"')) != 0)
		{
			*next = '\0';
			strcpy(info->name_info, var);
		}
	}

	return ptr;
}

void truncate_file(char *path, int writelen)
{
	FILE *fp = NULL;
	
	if( (fp = fopen(path, "r+")) )
	{
		ftruncate(fileno(fp), writelen);
		fflush(fp);
		fsync(fileno(fp));
		fclose(fp);
	}
}

int WIFIAudio_PostHtmlUpdate_StartUpdateWifi(char *ppath)
{
	int ret = 0;
	
	if (ppath == NULL)
	{
		ret = -1;
	} else {	
		ret = xzxhtml_wifiup(ppath);
	}
	
	return ret;
}

int WIFIAudio_PostHtmlUpdate_WriteProgress(char *pFileName, char progress)
{
	int ret = 0;
	FILE *fp = NULL;
	
	if((fp = fopen(pFileName, "a+")) == NULL)
	{
		ret = -1;
	} else {
		if (flock(fileno(fp), LOCK_EX) == 0)
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
			
			flock(fileno(fp), LOCK_UN);
			fclose(fp);
			fp = NULL;					
		}
	} 
	
	return ret;
}

int writeline(char *path, char* buffer)
{
	FILE * fp = NULL;
	fp = fopen(path, "w");

	if (fp == NULL) {
		return -1;
	}
	
	if((fputs(buffer, fp)) >= 0) {
		fclose(fp);
		return 0;
	} else {
		fclose(fp);
		return -1;
	}
}

int str2args (const char *str, char *argv[], char *delim, int max)
{
	char *p;
	int n;
	p = (char *) str;
	
	for (n = 0; n < max; n++)
	{
		if ((argv[n]=strtok_r(p, delim, &p)) == 0)
			break;
	}
	
	return n;
}

int str2argv(char *buf, char **v_list, char c)
{
	int n;
	char del[4];
	
	del[0] = c;
	del[1] = 0;
	n=str2args(buf, v_list, del, MAX_ARGVS);
	v_list[n] = 0;
	
	return n;
}

int exec_cmd(const char *cmd)
{
	char buf[MAX_COMMAND_LEN];
	FILE *pfile;
	int status = -2;
	
	if ((pfile = popen(cmd, "r"))) {
		fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
		while(!feof(pfile)) {
			fgets(buf, sizeof buf, pfile);
		}
		
		status = pclose(pfile);
	}
	
	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}

	return -1;
}


int exec_cmd2(const char *cmd, ...)
{
	char buf[MAX_COMMAND_LEN];
	va_list args;

	va_start(args, (char *)cmd);
	vsnprintf(buf, sizeof(buf), cmd, args);
	va_end(args);

	return exec_cmd(buf);
}

static void free_memory(int all)
{
	char alive_list[200] = { 0 };
	char *WDKUPGRADE_KEEP_ALIVE = NULL;
	
	if (all) {
		strcat(alive_list, "init\\|ash\\|watchdog\\|wdkupgrade\\|ps\\|$$");
	} else {
		strcat(alive_list, "init\\|uhttpd\\|hostapd\\|omnicfg_bc\\|ash\\|watchdog\\|wdkupgrade\\|uartd\\|ota\\|telnetd\\|posthtmlupdate.cgi\\|http\\|ps\\|$$");
	}
	
	if ((WDKUPGRADE_KEEP_ALIVE = getenv("WDKUPGRADE_KEEP_ALIVE")) != NULL) {
		char *argv[10];
		int num = str2argv(WDKUPGRADE_KEEP_ALIVE, argv, ' ');
		while (num-- > 0) {
			strcat(alive_list, "\\|");
			strcat(alive_list, argv[num]);
		}
	}
	
	exec_cmd2("ps | grep -v '%s' | awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill -9 > /dev/null 2>&1", alive_list);
	
	writeline("/proc/sys/vm/drop_caches", "3");
	sleep(1);
	writeline("/proc/sys/vm/drop_caches", "2");
	sync();
}

int WIFIAudio_PostHtmlUpdate_IsWiFiAndBluetoothUpgrade()
{
	int ret = 0;
	int State = 0;
	
	State = get_wifi_bt_update_flage();
	
	if((State >= 4) && (State <= 9))
	{
		ret = 1;
	} else {
		ret = 0;
	}
	
	return ret;
}

int WIFIAudio_PostHtmlUpdate_SetWiFiAndBluetoothUpgradeState(int state)
{
	int ret = 0;
	
	ret = set_wifi_bt_update_flage(state);
	
	return ret;
}

int main(int argc, char **argv)
{
	char read_buf[READ_BUFFER_SIZE+1];
	struct request_info info;
	FILE *output_fp = NULL;
	char *buf;
	unsigned long len;
	unsigned long readlen = 0;
	unsigned long writelen = 0;
	int short_file = 0;
	char errorBuf[200]="";
	
	free_memory(0);
	
	if(WIFIAudio_PostHtmlUpdate_IsWiFiAndBluetoothUpgrade() == 1)
	{
		printf(errorBuf,"%s","Upload error!");
		WIFIAudio_RetJson_retJSON("FAIL");
		goto error;
	}
	
	read_buf[READ_BUFFER_SIZE] = '\0';
	memset(&info, 0, sizeof(struct request_info));
	buf = getenv("CONTENT_LENGTH");
	if(buf)
	{
		info.len = atoi(buf);
		
		buf = getenv("CONTENT_TYPE");
		if(buf == NULL)
		{
			printf(errorBuf,"%s","Upload error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		
		if(!strstr(buf, "multipart/form-data"))
		{
			printf(errorBuf,"%s","Upload error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		
		if(NULL == (buf = strstr(buf, BOUNDRY_STRING)))
		{
			printf(errorBuf,"%s","Upload error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		} else {
			buf += strlen(BOUNDRY_STRING);
		}
		
		strcpy(info.boundary, buf);
		info.blen = strlen(buf);
		
		while(1)
		{
			len = fread(read_buf, 1, READ_BUFFER_SIZE, stdin);
			if(len < 0)
			{
				sprintf(errorBuf,"%s","Save error!");
				WIFIAudio_RetJson_retJSON("FAIL");
				goto error;
			}
			
			if(len == 0)
				break;
			
			if(readlen == 0)
			{
				if(NULL == (buf = httpd_get_filename(read_buf, len, &info)))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				
				sprintf(info.path_phys, "%s%s", "/tmp/", info.name_info);
				if(NULL == (output_fp = fopen(info.path_phys, "w")))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				
				if(0 == fwrite(buf, len - (buf - read_buf), 1, output_fp))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				writelen += len - (buf - read_buf);
			} else {
				if(0 == fwrite(read_buf, len, 1, output_fp))
				{
					sprintf(errorBuf,"%s","Save error!");
					WIFIAudio_RetJson_retJSON("FAIL");
					goto error;
				}
				writelen += len;
			}
			
			readlen += len;
		}

		if(NULL != output_fp)
		{
			fclose(output_fp);
			output_fp = NULL;
		}

		if(NULL == (output_fp = fopen(info.path_phys, "r")))
		{
			sprintf(errorBuf,"%s","Save error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		
		if(0 > fseek(output_fp, -READ_BUFFER_SIZE, SEEK_END))
		{
			if(0 > fseek(output_fp, 0, SEEK_SET))
			{
				sprintf(errorBuf,"%s","Save error!");
				WIFIAudio_RetJson_retJSON("FAIL");
				goto error;
			} else {
				short_file = 1;
			}
		}

		if(0 > (len = fread(read_buf, 1, READ_BUFFER_SIZE, output_fp)))
		{
			sprintf(errorBuf,"%s","Save error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}

		if(NULL == (buf = httpd_check_pattern(read_buf, len, info.boundary)))
		{
			sprintf(errorBuf,"%s","Save error!");
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		}
		buf -= 4;
		
		if(short_file)
		{
			writelen = (buf - read_buf);
		} else {
			writelen -= (READ_BUFFER_SIZE - (buf - read_buf));
		}

		if(NULL != output_fp)
		{
			fclose(output_fp);
			output_fp = NULL;
		}

		truncate_file(info.path_phys, writelen);
		WIFIAudio_PostHtmlUpdate_WriteProgress(WIFIAUDIO_WIFI_UP_PROGRESS, (char)100);
		
		WIFIAudio_PostHtmlUpdate_SetWiFiAndBluetoothUpgradeState(8);
		if (WIFIAudio_PostHtmlUpdate_StartUpdateWifi(info.path_phys) == -1)
		{
			WIFIAudio_RetJson_retJSON("FAIL");
			goto error;
		} else {
			WIFIAudio_RetJson_retJSON("OK");
		}
	} else {
		printf(errorBuf,"%s","Upload error!");
		WIFIAudio_RetJson_retJSON("FAIL");
		goto error;
	}
	
error:
	if(errorBuf[0]!='\0')
	{
		printf("<script type='text/javascript' language='javascript'>parent.my_Error('%s');</script>",errorBuf);
	} else {
		printf("<script type='text/javascript' language='javascript'>parent.my_OK();</script>");
	}
	fflush(stdout);

	return 0;
}

