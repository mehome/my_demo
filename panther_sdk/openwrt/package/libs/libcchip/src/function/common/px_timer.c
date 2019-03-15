#include "px_timer.h"

inline long get_tsd(struct timespec *pt2,struct timespec *pt1,char lev){	
	struct timespec tv={0};
	long tsd=get_tsv(tv,*pt2,*pt1);
	if(tsd<0)return -1;
	switch(lev){
	case 's'://秒
		tsd=tv.tv_sec;
		break;
	case 'm'://毫秒
		tsd=tv.tv_sec*1000+tv.tv_nsec/1000000;
		break;
	case 'u'://微秒
		tsd=tv.tv_sec*1000000+tv.tv_nsec/1000;
		break;
	case 'n'://纳秒
		tsd=tv.tv_sec*1000000000+tv.tv_nsec;
		break;
	default:
		return -1;
	}
	return tsd;
}

static struct timespec mts={0};
int rec_tmie_start(clockid_t cid){
	int ret=px_gettime(cid,&mts);
	if(ret<0)return -1;
	if(CLOCK_REALTIME==cid){
		char *timeStr=getTmieStr(mts);
		if(timeStr){
			raw("%s:%s\n",__func__,timeStr);
			free(timeStr);
		}		
	}	
	return 0;
}

inline long long int rec_tmie_get(clockid_t cid,char sync){
	struct timespec ts={0},tv={0};
	int ret=px_gettime(cid,&ts);
	if(ret<0)return -1;
	tv.tv_sec=ts.tv_sec-mts.tv_sec;
	tv.tv_nsec=ts.tv_nsec-mts.tv_nsec;
	if(CLOCK_REALTIME==cid){
		char *timeStr=getTmieStr(ts);
		if(timeStr){
			raw(timeStr);
			free(timeStr);
		}		
	}
	if(sync){
		memcpy(&mts,&ts,sizeof(mts));
	}
	return tv.tv_sec*1000000000+tv.tv_nsec;
}

inline char *get_fmt_time(char *tm_str,size_t size,char *fmt){
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	strftime(tm_str,size,fmt,timeinfo);
	return tm_str;
}