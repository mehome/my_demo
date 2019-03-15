#include "slog.h"

log_ctrl_t log_ctrl_array[MAX_TYPE]={
	[_ERR]	={"ERR"		,Hred		},
	[_WAR]	={"WAR"		,Hyellow	},
	[_INF]	={"INF"		,Hgreen		},
	[_DBG]	={"DBG"		,Hblue		},
	[_TRC]	={"TRC"		,Hpurple	},	
};
char log_ctrl_set[MAX_TYPE+1]="111";
char out_path_set[MAX_PNL]="";

void get_time_ms(char *ts,int size){
	struct timeval tv={0};
	if(gettimeofday(&tv,NULL)<0){
		return;
	}
	snprintf(ts,size,"%013lu",tv.tv_sec*1000+tv.tv_usec/1000);
}
//cfg_str : l=11111
int log_init(const char *cfg_str){
	if(!cfg_str)return 0;
	char *idx=strstr(cfg_str,"l=");
	if(idx)sscanf(idx,"l=%s",log_ctrl_set);//log_ctrl_set:11111
	idx=strstr(cfg_str,"op=");
	if(idx)sscanf(idx,"op=%s",out_path_set);
	if('1'==log_ctrl_set[_INF]){
		printf("log_ctrl_set:%s\n",log_ctrl_set);
		if(idx)printf("out_path_set:%s\n",out_path_set);
	}
	return 0;
}
pthread_mutex_t line_lock = PTHREAD_MUTEX_INITIALIZER;
inline void slog(log_type_t n,char lock,char *log_ctrl_set,const char *out_path_set,const char *ts,\
					const char *file,const int line,const char *fmt,...){						
	if('1'==log_ctrl_set[n]){	
		FILE *fp=NULL;
		if(lock)pthread_mutex_lock(&line_lock);	
		if(out_path_set[0]){
			if(!(fp=fopen(out_path_set,"a"))){
				printf("fopen %s fail,errno=%d\n",out_path_set,errno);
				fp=stdout;
			}
		}else{
			fp=stdout;
		}
		va_list args;
		fprintf(fp,"[%s%s%s\033[0m %s:%d]:",ts,log_ctrl_array[n].color,log_ctrl_array[n].name,get_last_name(file),line);			
		va_start(args,fmt);
		vfprintf(fp,fmt,args);
		va_end(args);
		fprintf(fp,"\n");		
		if(fp!=stdout){
			if(fclose(fp)){
				printf("error:close %s fail,errno=%d\n",out_path_set,errno);
			}
		}
		if(lock)pthread_mutex_unlock(&line_lock);		
	}
}
inline void raw_log(log_type_t n,char lock,char *log_ctrl_set,const char *out_path_set,const char *fmt,...){
	if('1'==log_ctrl_set[n]){
		FILE *fp=NULL;
		if(lock)pthread_mutex_lock(&line_lock);
		if(out_path_set[0]){
			if(!(fp=fopen(out_path_set,"a"))){
				printf("fopen %s fail,errno=%d\n",out_path_set,errno);
				fp=stdout;
			}
		}else{
			fp=stdout;
		}
		va_list args;
		va_start(args,fmt);
		vfprintf(fp,fmt,args);
		va_end(args);
		if(fp!=stdout){
			if(fclose(fp)){
				printf("error:close %s fail,errno=%d\n",out_path_set,errno);
			}
		}
		if(lock)pthread_mutex_unlock(&line_lock);		
	}
}