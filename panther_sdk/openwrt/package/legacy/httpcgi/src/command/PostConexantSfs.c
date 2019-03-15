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

#include "WIFIAudio_SystemCommand.h"
#include "WIFIAudio_RetJson.h"

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
				
				sprintf(info.path_phys, WIFIAUDIO_CONEXANTSFS_PATHNAME);
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
		WIFIAudio_RetJson_retJSON("OK");
	} else {
		printf(errorBuf,"%s","Upload error!");
		WIFIAudio_RetJson_retJSON("FAIL");
		goto error;
	}
	
error:
	if(errorBuf[0]!='\0')
	{
		printf("<script type='text/javascript' language='javascript'>parent.Conexant_sys_my_Error('%s');</script>",errorBuf);
	} else {
		printf("<script type='text/javascript' language='javascript'>parent.Conexant_sys_my_OK();</script>");
	}
	fflush(stdout);

	return 0;
}


