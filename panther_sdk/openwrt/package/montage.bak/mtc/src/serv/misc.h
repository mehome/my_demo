
/**
 * @file misc.h
 *
 * @author
 * @date   
 */

#ifndef _MISC_H_
#define _MISC_H_

int system_update_hosts(void);
#define _STR(s)     #s 
#define STR(s)      _STR(s)
#define LOG_OUT_PATH "/dev/console"
//open/close  log
#define OPEN_CSLOG_CTRL 1

#if OPEN_CSLOG_CTRL  
#define cslog(fmt, args...) ({ \
	FILE *fp = fopen(LOG_OUT_PATH, "w"); \
	if(fp){\
		fprintf(fp,"\033[1;35m["__FILE__" %d]:\033[0m%s:",__LINE__,__func__); \
		fprintf(fp,fmt,##args); \
		fprintf(fp,"\n"); \
		fclose(fp); \
	}\
})
#else
#define cslog(fmt, args...)  
#endif
#endif

