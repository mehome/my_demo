#include "FileUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "myutils/debug.h"

char *ReadData(char *filename)
{
	int readLen = 0;
	FILE* fp = NULL;
	char *data = NULL;
	struct stat filestat;

	if((access(filename,F_OK))) {
		error("%s is not exsit", filename);
		goto exit;
	}
	stat(filename, &filestat);
	if(filestat.st_size <= 0) {
		goto exit;
	}

	fp = fopen(filename, "r");
	if(fp == NULL) {
		error("%s fopen error", filename);
		goto exit;
	}
	data = (char *)calloc(1,(filestat.st_size + 1) * sizeof(char)); //yes
	if(data == NULL) {
		error("calloc error");
		goto exit;
	}
	readLen = fread(data, sizeof(char), filestat.st_size, fp);
	if(readLen !=  filestat.st_size)
	{	
		error("%s fread error", filename);
		free(data);
		data = NULL;
		goto exit;
	}
exit:

	if(fp) {
		fclose(fp);
		fp = NULL;
	}
	return data;

}
int WriteData(char *filename, char *data)
{
	FILE* fp = NULL;
	int ret = 0;
	size_t len = 0 ,writeLen = 0; 
	info("filename:%s",filename);
	fp = fopen(filename, "w+");
	if(fp == NULL) {
		error("%s fopen error", filename);
		ret = -1;
		goto exit;
	}
	len = strlen(data);
	info("len:%d",len);
	writeLen = fwrite(data, sizeof(char), len, fp);
	info("writeLen:%d",writeLen);
	if(writeLen !=  len) {	
		ret = -1;
		error("%s fwrite error", filename);
		goto exit;
	}

exit:
	if(fp) {
		fclose(fp);
		fp = NULL;
	}
	return ret;

}
