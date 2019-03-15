#ifndef __TTPOD_UTIL_H__
#define __TTPOD_UTIL_H__


extern char* join(char *s1, char *s2) ;
extern void genRandomString(char *string, int length) ;
extern long getTimeStamp() ;
extern int uri_encode(char *dst, char *str );

extern int get_mac(char * mac, int len_limit)  ;


#endif


