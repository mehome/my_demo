#include <stdio.h>
#include <curl/curl.h>
#include <json/json.h>
#include <sys/stat.h>  
#include <myutils/mystring.h>

#include <fcntl.h>
#include "myutils/debug.h"
#include "param.h"

static char g_byFlag = 0;

static size_t onWriteData(void *buffer, size_t size, size_t nmemb, void *stream)
{	
	mystring* str = ((mystring *)stream);
	string_append(str, buffer, size * nmemb);
	return size * nmemb;
	return (nmemb * size);
}

static size_t onHeadData(void *ptr, size_t size, size_t nmemb, void *stream)  
{

	return (nmemb * size);
}


static size_t onReadData(char *buffer, size_t size, size_t nitems, void *instream)
{
	int iRet = 0;
	int readSize = 0;

	if (0 == g_byFlag)
	{
		g_byFlag = 1;
		info("Start Upload");
	}

	readSize = fread(buffer, size, nitems, (FILE *)instream);


	if (0 == readSize)
	{
		info("Upload End");
	}

	return readSize;
}

int PostFile(char *url , char*body ,char *file, void *arg)
{
	debug("enter...");
	CURLcode code;
	FILE *fp = NULL;
	struct curl_slist *http_headers = NULL;
	CURL *easy_handle = NULL;
	int ret = 0;
    info("url:%s", url);
	struct curl_httppost *postFirst = NULL, *postLast = NULL;	
    code = curl_global_init(CURL_GLOBAL_DEFAULT);
    if(code != CURLE_OK){
        error("init libcurl failed.");
    }
	long fsize = getFileSize(file);
	char *data = malloc(fsize + 10);
	memset(data , 0, fsize+10);
    fp = fopen(file, "rb");  
	if(fp == NULL) {
		error("fopen %s failed", file);
		perror("fopen failed");
		goto exit;
	}
	size_t read = fread(data, 1, fsize, fp);
	info("read:%d",read);
   
 	easy_handle = curl_easy_init();
    info("body:%s", body);
    http_headers = curl_slist_append(http_headers, "Expect:");
    //http_headers = curl_slist_append(http_headers, "Content-Type:application/x-www-form-urlencoded;charset=utf-8");
    //	http_headers = curl_slist_append(http_headers, "Expect:");
	http_headers = curl_slist_append(http_headers, "Content-Type:multipart/form-data;charset=utf-8");
 
	curl_formadd(&postFirst, &postLast,
				CURLFORM_COPYNAME, "parameters",
				CURLFORM_COPYCONTENTS, body,
				CURLFORM_CONTENTTYPE,"application/x-www-form-urlencoded",
				CURLFORM_END);
	//curl_formadd(&postFirst, &postLast,
	//			CURLFORM_COPYNAME, "speech",
	//			CURLFORM_BUFFER, file,
	///			CURLFORM_BUFFERPTR, data,				// 该参数是指向要上传的缓冲区的指针
	//			CURLFORM_BUFFERLENGTH, read,			// 缓冲区的长度
	//			CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
	//			CURLFORM_END);
    curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, http_headers);
    curl_easy_setopt(easy_handle, CURLOPT_URL, url);
    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, onWriteData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, arg);    
	curl_easy_setopt(easy_handle, CURLOPT_HEADERFUNCTION, onHeadData);
   // curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, body);
	//curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 1);
    //curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT_MS, 1000);
    curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT_MS, 500);
	curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST, postFirst);
    debug("curl_easy_perform");
    code = curl_easy_perform(easy_handle);
	info("code:%d",code);
	if(code != CURLE_OK )		
	{	
		ret = -1;
		if(code != 23)
			error("Curl curl_easy_perform failed: errno=%d [%s]" ,code ,curl_easy_strerror(code));
	}
exit:
	if(fp) {
		fclose(fp);
	}
	if(postFirst) {
		curl_formfree(postFirst);
	}
	if(http_headers)
   		curl_slist_free_all(http_headers);
	if(easy_handle)
    	curl_easy_cleanup(easy_handle);
    curl_global_cleanup();
	info("exit");
	info("ret:%d",ret);
	return ret;
}

int PostQuestion(char *url , char*body ,char *file, void *data)
{
	debug("enter...");
	CURLcode code;
	int ret = 0;
    info("url:%s", url);
    code = curl_global_init(CURL_GLOBAL_DEFAULT);
  
    if(code != CURLE_OK){
        error("init libcurl failed.");
    }
    struct curl_slist *http_headers = NULL;
    CURL *easy_handle = curl_easy_init();
    info("body:%s", body);
    http_headers = curl_slist_append(http_headers, "Expect:");
    http_headers = curl_slist_append(http_headers, "Content-Type:application/x-www-form-urlencoded;charset=utf-8");
    
    curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, http_headers);
    curl_easy_setopt(easy_handle, CURLOPT_URL, url);
    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, onWriteData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, data);  
	curl_easy_setopt(easy_handle, CURLOPT_HEADERFUNCTION, onHeadData);
    curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, body);
	//curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 1);
	curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT_MS, 1000);
    debug("curl_easy_perform");
    code = curl_easy_perform(easy_handle);

exit:

	if(http_headers)
   		curl_slist_free_all(http_headers);
	if(easy_handle)
    	curl_easy_cleanup(easy_handle);
    curl_global_cleanup();
	info("exit");
	info("ret:%d",ret);
	return code;
}









