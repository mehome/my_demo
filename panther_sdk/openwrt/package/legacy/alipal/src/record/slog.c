#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "slog.h"
// \033[显示方式;前景色;背景色m
/* 	显示方式:0（默认值）、1（高亮）、22（非粗体）、4（下划线）、24（非下划线）、5（闪烁）、25（非闪烁）、7（反显）、27（非反显）
	前景色:30（黑色）、31（红色）、32（绿色）、 33（黄色）、34（蓝色）、35（洋红）、36（青色）、37（白色）
	背景色:40（黑色）、41（红色）、42（绿色）、 43（黄色）、44（蓝色）、45（洋红）、46（青色）、47（白色） */
//注：如果增减了以下项，注意在log.h中增减对应的枚举值	
log_ctrl_t log_ctrl_array[MAX_LOG_TYPE_T]={
	[_ERR]	={"ERR"		,"\033[1;31m"},
	[_WAR]	={"WAR"		,"\033[1;33m"},
	[_IPT]	={"IPT"		,"\033[1;34m"},
	[_INF]	={"INF"		,"\033[1;32m"},
};
char log_ctrl_set[MAX_LOG_TYPE_T+1]="0000";
void get_local_time_str(char *ts,int size){
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	strftime(ts,size,"%H:%M:%S",lt);
}
void get_time_ms(char *ts,int size){
	struct timeval tv={0};
	if(gettimeofday(&tv,NULL)<0){
		return;
	}
	snprintf(ts,size,"%013lu",tv.tv_sec*1000+tv.tv_usec/1000);
}
pthread_mutex_t line_lock = PTHREAD_MUTEX_INITIALIZER;
int log_init(char *ctrl_str){
#if 1
	if(ctrl_str&&ctrl_str[0]){
		if(sscanf(ctrl_str,"l=%04s",log_ctrl_set)<=0){
			printf("sscanf log_ctrl_set failure!,ctrl_str:%s,log_ctrl_set:%s\n",ctrl_str,log_ctrl_set);
			return -1;
		}
	}
	return 0;
#else
	FILE *fd=NULL;
	char ret=0;
	if(fpath==NULL){
		printf("ERR:%s %d fpath is NULL!\n",__FILE__,__LINE__);
		return -1;
	}
	if(!(fd=fopen(fpath,"a+"))){
		printf("ERR:%s line:%d:fopen %s fail,errno:%s\n",__FILE__,__LINE__,fpath,strerror(errno));
		return -1;
	}
	char buf[MAX_LOG_TYPE_T+1]="";
	if((ret=fread(buf,sizeof(char),MAX_LOG_TYPE_T,fd))<MAX_LOG_TYPE_T){
		printf("%s,%d:error:read log set fial,ret=%d\n",__FILE__,__LINE__,ret);
		goto fread_err;
	}
	strncpy(log_ctrl_set,buf,MAX_LOG_TYPE_T);
	return 0;
fread_err:	
	if(fclose(fd)){
		printf("ERR:%s%d close %s fail\n",__FILE__,__LINE__,fpath);
	}
	return -1;
#endif
}
void slog(log_type_t num_type,char *log_ctrl_set,const char *out_file,const char *ts,const char *file,const int line,const char *fmt,...){
	int ret=0;
	if((ret=pthread_mutex_lock(&line_lock))){
		printf("ERR:%s %d pthread_mutex_lock fail,ret=%d\n",ret,__FILE__,__LINE__);;
		return;
	}
	if('1'==log_ctrl_set[num_type]){
		FILE *fp=NULL;
		if(out_file){
			if(!(fp=fopen(out_file,"a"))){
				printf("fopen %s fail,errno=%d\n",out_file,errno);
				fp=stderr;
			}
		}else{
			fp=stderr;
		}
		va_list args;
		fprintf(fp,"%s %s%s\033[0m %s %d:",ts,log_ctrl_array[num_type].color,log_ctrl_array[num_type].name,file,line);			
		va_start(args,fmt);
		vfprintf(fp,fmt,args);
		va_end(args);
		fprintf(fp,"\n");		
		if(fp!=stderr){
			if(fclose(fp)){
				printf("error:close %s fail,errno=%d\n",out_file,errno);
			}
		}
	}
	if((ret=pthread_mutex_unlock(&line_lock))){
		printf("ERR:%s %d pthread_mutex_unlock fail,ret=%d\n",ret,__FILE__,__LINE__);;
		return;
	}
}
#if 0
int main(int argc,char ** argv){
	do{	
		char ts[16]="";		
		get_local_time_str(ts,sizeof(ts));
		if(update_log_set_from_file(LOG_SET_PATH)<0){
			printf("error:%s line:%d:update_log_set_from_file %s fial!\n",__FILE__,__LINE__,LOG_SET_PATH);
		}
		DEBUG_ERR("好哈哈哈哈哈");
		usleep(3000);
		DEBUG_WAR("操蛋");
		usleep(5000);
		DEBUG_INF("你是谁");
		usleep(8000);
		DEBUG_DEF("%s",__func__);
		usleep(9000);
	}while(1);
	return 0;
}
#endif