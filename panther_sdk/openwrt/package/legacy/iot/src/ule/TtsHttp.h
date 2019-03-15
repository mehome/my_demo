#ifndef __TTS_HTTP_H__
#define __TTS_HTTP_H__

#include <myutils/mystring.h>

int post(char* url,char *body, mystring* response) ; 
int get(char *url ,char *ak, char *sk, mystring *str);
      

#endif