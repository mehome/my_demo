#ifndef __GLOBAL_PARAM_H__
#define __GLOBAL_PARAM_H__

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <mpd/client.h>
#include <curl/curl.h>
#include <sys/wait.h>  
#include <sys/types.h>

#include "fifobuffer.h"

#include "myutils/debug.h"
#include "Curl_Https_Request.h"


#include "common.h"
#include "uciConfig.h"


/**
 * Information about MPD's current status.
 */
typedef struct CurlEasyHandler
{
	CURL *easy_handle;
	char *BodyBuff;	
	char *data;
	int len;
	struct curl_slist *httpHeaderSlist;
	struct curl_httppost *formpost;
	struct curl_httppost *lastptr;
}CurlEasyHandler;


#define MAX_CONNECT_STREM 10




typedef struct GlobalParam
{
	pthread_mutex_t Clear_easy_handler_mtx;
    CURLM *multi_handle;
	CurlEasyHandler m_RequestBoby[MAX_CONNECT_STREM];
}GlobalParam;



extern void SystemParaInit(void);



#endif /* __GLOBAL_PARAM_H__ */


