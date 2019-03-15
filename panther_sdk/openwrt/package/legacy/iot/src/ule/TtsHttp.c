#include "TtsHttp.h"

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include "myutils/debug.h"

int connect_timeout = 10000;
int socket_timeout = 10000;
int debug = 0;

static inline size_t onWriteData(void * buffer, size_t size, size_t nmemb, void * userp)
{
	mystring* str = ((mystring *)userp);
	info("len:%d",size * nmemb);
	string_append(str, buffer, size * nmemb);
	return size * nmemb;
}
static inline size_t onHeadData(void * buffer, size_t size, size_t nmemb, void * userp)
{
	info("buffer:%s",buffer);
	return size * nmemb;
}

void setConnectTimeout(int connect_timeout)
{
  connect_timeout = connect_timeout;
}

void setSocketTimeout(int socket_timeout)
{
  socket_timeout = socket_timeout;
}


int post(char* url, char *body, mystring* response) 
{
  struct curl_slist * slist = NULL;
  CURL * curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
 
  slist = curl_slist_append(slist, "Expect:");
  slist = curl_slist_append(slist, "Content-Type:application/json;charset=UTF-8");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
  curl_easy_setopt(curl, CURLOPT_POST, true);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  onWriteData);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHeadData );
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) response);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, true);
  //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 4000);
  //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
  
  int status_code = curl_easy_perform(curl);
  
  curl_easy_cleanup(curl);
  curl_slist_free_all(slist);
  
  return status_code;
}
int get(char *url ,char *ak, char *sk, mystring *str)
{
	char * key = NULL;
	char * value = NULL;
    CURL * curl = curl_easy_init();
    struct curl_slist * slist = NULL;

	
	curl_easy_setopt(curl, CURLOPT_URL, url);
    //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)str);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, true);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, socket_timeout);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    
    int status_code = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    info("exit");
    return status_code;
}
          

