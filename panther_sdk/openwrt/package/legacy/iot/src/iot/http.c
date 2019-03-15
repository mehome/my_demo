#include "http.h"

#include <stdio.h>
#include <curl/curl.h>
#include <json/json.h>
#include <sys/stat.h>  

#include <fcntl.h>

#include "myutils/debug.h"
#include "common.h"

/* post和uploadfile数据回调. */
static size_t ProcessData(void *buffer, size_t size, size_t nmemb, void *stream)
{	
	info("buffer:%s",buffer);
	//buffer:{"desc":"success","code":0,"payload":"UdeOVIXorbOSOxeAEx8uApxonCNCcnxBIAM4Fnn6QcLLSJ_ifBAjZCIbXSPqkMN6"}
   	info("stream:%d",stream);
	char *str = (char *)buffer;
	str[nmemb * size] = 0;
	info("str:%s",str);
	ProcessIotJson(buffer, (int)stream);
	return (nmemb * size);
}
static size_t onHeadData(void *ptr, size_t size, size_t nmemb, void *stream)  
{

	return (nmemb * size);
}


#ifndef ENABLE_HUABEI
/* 下载url到	fileName. */
int UploadFile(char *url, char * file, void *data)
{

    CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
    
    if(code != CURLE_OK)
	{
        error("init libcurl failed.");
    }
    struct curl_slist *http_headers = NULL;
    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;
    CURL *easy_handle = curl_easy_init();
    info("path:%s",file);
    http_headers = curl_slist_append(http_headers, "Content-Type:multipart/form-data");
    
    curl_formadd(&post, &last,
                 CURLFORM_COPYNAME,"speech",
                 CURLFORM_FILE, file,
                 CURLFORM_CONTENTTYPE,"application/octet-stream",
                 CURLFORM_END);

    curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, http_headers);

	curl_easy_setopt(easy_handle, CURLOPT_URL,url);
	warning("url:%s",url);
	//url:http://iot-ai.tuling123.com/resources/file/upload?apiKey=8e3941df07b14bc183766ebd51a3b8b0
    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION,  ProcessData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA,data);  
	curl_easy_setopt(easy_handle, CURLOPT_HEADERFUNCTION, onHeadData);
    curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST,post);
    curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 0); 
    code = curl_easy_perform(easy_handle);

	if(code != CURLE_OK )		
	{
		if(code != 23)
			error("Curl curl_easy_perform failed: errno=%d [%s]",code ,curl_easy_strerror(code));
	}
    curl_slist_free_all(http_headers);
    curl_formfree(post);
    curl_easy_cleanup(easy_handle);
    curl_global_cleanup();
	return code;
}


#else
/* 上传微聊录音文件接口. */
int UploadFile(char *url, char *path, char *apikey,  char *body, int type, void *data)
{

	int mqtt_connect = GetMqttConnect();
	error("mqtt_connect:%d", mqtt_connect);
	if(mqtt_connect == 0)
		return;

    CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
    char buf[4] = {0};
    if(code != CURLE_OK)
	{
        error("init libcurl failed.");
    }
    snprintf(buf, 4, "%d", type);
    struct curl_slist *http_headers = NULL;
    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;
    CURL *easy_handle = curl_easy_init();
    info("path:%s",path);
    http_headers = curl_slist_append(http_headers, "Expect:");
    http_headers = curl_slist_append(http_headers, "Content-Type:multipart/form-data;charset=UTF-8");

    curl_formadd(&post, &last,
                 CURLFORM_COPYNAME,"speech",
                 CURLFORM_FILE,path,
                 CURLFORM_CONTENTTYPE,"application/octet-stream",
                 CURLFORM_END);
    curl_formadd(&post, &last,
             CURLFORM_COPYNAME,"type",
             CURLFORM_COPYCONTENTS, buf,
             CURLFORM_CONTENTTYPE,"application/octet-stream",
             CURLFORM_END);
  	 curl_formadd(&post, &last,
     CURLFORM_COPYNAME,"apikey",
     CURLFORM_COPYCONTENTS,apikey,
     CURLFORM_CONTENTTYPE,"application/octet-stream",
     CURLFORM_END);

    curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, http_headers);


	curl_easy_setopt(easy_handle, CURLOPT_URL,url);
	warning("url:%s",url);

    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION,  ProcessData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA,data);  
	curl_easy_setopt(easy_handle, CURLOPT_HEADERFUNCTION, onHeadData);
    curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST,post);
	curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 0);
	//curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, body);
    //curl_easy_setopt(easy_handle, CURLOPT_PROXY, "localhost:8888");
     
    code = curl_easy_perform(easy_handle);

	if(code != CURLE_OK )		
	{
		if(code != 23)
			error("Curl curl_easy_perform failed: errno=%d [%s]",code ,curl_easy_strerror(code));
	}
    curl_slist_free_all(http_headers);
    curl_formfree(post);
    curl_easy_cleanup(easy_handle);
    curl_global_cleanup();
	return code;
}
#endif
/* post请求. */
int HttpPost(char *host , char* path, char* body , void *data)
{
	debug("enter...");
	CURLcode code;
#ifdef ENABLE_HUABEI
	int mqtt_connect = GetMqttConnect();
	info("mqtt_connect:%d", mqtt_connect);
	if(mqtt_connect == 0)
		return;
#endif	

	char url[512] = {0};
	
	snprintf(url, 512, "http://%s%s", host, path);
    info("url:%s", url);
	//url:http://iot-ai.tuling123.com/iot/message/send(微聊时候的url)
    code = curl_global_init(CURL_GLOBAL_DEFAULT);
  
    if(code != CURLE_OK){
        error("init libcurl failed.");
    }
    
    struct curl_slist *http_headers = NULL;
    CURL *easy_handle = curl_easy_init();
    debug("body:%s", body);
    http_headers = curl_slist_append(http_headers, "Expect:");
    http_headers = curl_slist_append(http_headers, "Content-Type:application/json;charset=UTF-8");
    
    curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, http_headers);
    curl_easy_setopt(easy_handle, CURLOPT_URL, url);
    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, ProcessData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA,data);  
	curl_easy_setopt(easy_handle, CURLOPT_HEADERFUNCTION, onHeadData);
    curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, body);
	curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 0);

    debug("curl_easy_perform");
    code = curl_easy_perform(easy_handle);
	if(code != CURLE_OK )		
	{	
		if(code != 23)
			error("Curl curl_easy_perform failed: errno=%d [%s]",code ,curl_easy_strerror(code));
	}
	if(http_headers)
   		curl_slist_free_all(http_headers);
    curl_easy_cleanup(easy_handle);
    curl_global_cleanup();
	debug("exit...");
	return code;
}

static size_t onWriteData(void *ptr, size_t size, size_t nmemb, void *stream) 
{
	ProcessHttpGetJson(ptr);
	return (nmemb * size);

}

/* Get请求. */
int HttpGet(char* url)
{
	CURLcode code;
	long length = 0;
    code = curl_global_init(CURL_GLOBAL_DEFAULT);
  
    if(code != CURLE_OK){
        error("init libcurl failed.");
    }

	char * response = NULL;
	CURL *easy_handle = curl_easy_init();
	
	curl_easy_setopt(easy_handle, CURLOPT_URL, url);
	curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 20);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, onWriteData);
	curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 0);
	
    code = curl_easy_perform(easy_handle);
	if(code != CURLE_OK )		
	{	
		if(code != 23)
			error("Curl curl_easy_perform failed: errno=%d [%s]",code ,curl_easy_strerror(code));
	}
	curl_easy_cleanup(easy_handle);
    curl_global_cleanup();
	return code;
}


static size_t DownloadCallback(void* pBuffer, size_t nSize, size_t nMemByte, void* pParam)    
{    
    FILE* fp = (FILE*)pParam;    
    size_t nWrite = fwrite(pBuffer, nSize, nMemByte, fp);    	
    return nWrite;    
}    
//收藏执行  ， dltotal：总的字节长度  			dlnow：正在下载的字节长度
static int ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)    
{    
  
    if ( dltotal > -0.1 && dltotal < 0.1 )    
    {  
        return 0;  
    }  
    int nPos = (int) ( (dlnow/dltotal)*100 );    
    
   	debug("dltotal:%d  ---- dlnow:%d",(long)dltotal ,(long)dlnow );  
  	//dltotal:7440945  ---- dlnow:183558
    return 0;    
}  
/* 下载url到	fileName. */
int    HttpDownLoadFile(char *url ,char *fileName)
{	
	int ret = -1;
	CURL *curl;
	CURLcode code;
	FILE* file = NULL;
#ifdef ENABLE_HUABEI
	int mqtt_connect = GetMqttConnect();
	info("mqtt_connect:%d", mqtt_connect);
	if(mqtt_connect == 0)
		return;
#endif	
	info("download %s ....", fileName);
	code = curl_global_init(CURL_GLOBAL_DEFAULT);
    if(code != CURLE_OK)
	{
        error("init libcurl failed.");
    }

    curl = curl_easy_init();
	file = fopen(fileName, "wb");  
	if(file == NULL) {
		error("fopen %s failed", fileName);
		perror("fopen failed");
		goto exit;
	}
	curl_easy_setopt(curl, CURLOPT_URL,url); 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadCallback);  
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,file);  
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHeadData);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);    
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);    
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);  
    code = curl_easy_perform(curl);  
	if(code != CURLE_OK) {
		ret = -1;
		if(code != 23)
			error("Curl curl_easy_perform failed: errno=%d [%s]\n",code ,curl_easy_strerror(code));
	} else {
		ret = 0;
	}
exit:
	curl_easy_cleanup(curl);  
	curl_global_cleanup();
	if(file)
		fclose(file);
	return ret;
}

static size_t readFileFunc(char *buffer, size_t size, size_t nitems, void *instream)
{
	int iRet = 0;
	int readSize = 0;

	readSize = fread( buffer, size, nitems, (FILE *)instream );

	if (0 == readSize)
	{
		//PrintSysTime("Upload End");
	}

	return readSize;
}

/* 获取要下载文件的长度    .       */
double GetDownloadFileLenth(const char *url)
{  
	double downloadFileLenth = 0;  
    CURL *handle = curl_easy_init();  
    curl_easy_setopt(handle, CURLOPT_URL, url);  
    curl_easy_setopt(handle, CURLOPT_HEADER, 1);    //只需要header头  
    curl_easy_setopt(handle, CURLOPT_NOBODY, 1);    //不需要body  
    if (curl_easy_perform(handle) == CURLE_OK) 
	{  
        curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLenth);
    } 
	else 
	{ 
        downloadFileLenth = -1;  
    }  
	if(handle)
		curl_easy_cleanup(handle);  
	info("downloadFileLenth:%f",downloadFileLenth);
    return downloadFileLenth;  
}  







