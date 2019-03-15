#include <stdio.h>
#include <sys/reboot.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>

#include <curl/curl.h>
#include <curl/easy.h> 
#include <wdk/cdb.h>
#include <time.h>
#include <signal.h>
#include <json/json.h>

#include "dueros_ota.h"

#define DEBUG_ON         0

#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/console", 0, NULL); \
})
//#define DIRECTIVES_PATH "http://test.tsi.ppqa.com/api/romPlugins"
//#define DIRECTIVES_PATH "http://tms.api.cp61.ott.cibntv.net/api/romPlugins"
//#define DIRECTIVES_PATH "http://117.135.159.40:8082/api/romUpdate?"
//#define DIRECTIVES_PATH "http://117.135.159.40:8082/api/romUpdate?sv=FW_panther_20170626205633&modeId=DOSS1212_DS_1639&appplt=sam&macAddress=8C:8A:4B:00:00:16"
//api/romPlugins?sv=FW_panther_20180626205633&modeId=DOSS1212_DS_1639&appplt=sam&macAddress=8C:8A:4B:00:00:16

#define DIRECTIVES_PATH "http://tms.api.cp61.ott.cibntv.net/api/romUpdate?"
#define PLAYER "myPlay"
char host_header[1024] = {0};
int voice_bt_flage = 0;
void submit_token(int is_voice_bt)
{
	char sv[256] = {0};
	char VOICE_VER[16] = {0};
	char BT_VER[16] = {0};
	char appplt[16] = {0};
	char modeId[32] = {0};
	char mode_name[32] = {0};
	char mac_addr[32] = {0};
	char macAddress[32] = {0};
	char OTA_PATH[128] = {0};

	memset(host_header,0,1024);
	cdb_get("$ota_path",OTA_PATH);
	cdb_get("$sw_build",VOICE_VER);
	cdb_get("$tmp_bt_ver",BT_VER);
	cdb_get("$model_name",mode_name);
	cdb_get("$wanif_mac",mac_addr);
	if (is_voice_bt == 0){
		sprintf(sv,"sv=FW_panther_%s&",VOICE_VER);
		voice_bt_flage = 0;
	}else{
		voice_bt_flage = 1;
		sprintf(sv,"sv=BT_FIRMWAR_%s&",BT_VER);
	}
	sprintf(modeId,"modeId=%s&",mode_name);
	sprintf(macAddress,"macAddress=%s",mac_addr);
	strncpy(appplt, "appplt=sam&", 16);

	strcat(host_header,OTA_PATH);
	strcat(host_header,sv);
	strcat(host_header,modeId);
	strcat(host_header,appplt);
	strcat(host_header,macAddress);
//	DEBUG_ERROR("%s",host_header);
	

}

size_t getFileSize(char * file)    
{   size_t size = -1;
    FILE * fp = fopen(file, "r");   
    if(fp == NULL) {
    	return -1;
    }
    fseek(fp, 0L, SEEK_END);   
    size = ftell(fp);   
    fclose(fp);   
    return size;   
} 

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    int len = size * nmemb;
    int written = len;
    FILE *fp = NULL;
    if (written == 0) 
    	return 0;
		//DEBUG_INFO("ptr:%s",(char *)ptr);
    if (access((char*) stream, 0) == -1) {
        fp = fopen((char*) stream, "wb");
    } else {
        fp = fopen((char*) stream, "ab");
    }
    if (fp) {
        fwrite(ptr, size, nmemb, fp);
    }

    fclose(fp);
    return written;
}


int progress_func(char *progress_data,  
                     double dltotal, double dlnow,
                     double ultotal, double ulnow)  
{  
	fprintf(stderr,"\r%g%%", dlnow*100.0/dltotal);
	char string[8] = "";
	int length;
//	fprintf(stderr,"\r%d.0%%",dlnow*100.0/dltotal);
	sprintf(string,"\r%g%%",dlnow*100.0/dltotal);
	length = strlen(string);

	FILE *filp = NULL;  


	filp = fopen(DOWNLOAD_PROGRESS_DIR,"w+");  /* 可读可写，不存在则创建 */  
	int writeCnt = fwrite(string,length,1,filp);  /* 返回值为1 */  

	fclose(filp);  
	
  return 0;  
} 


size_t Header_callback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	return (nmemb * size);
}


size_t DownChannel_ChunkData_callback(void *buffer,size_t size,size_t count,void *response)  
{
	char * ptr = buffer;

	DEBUG_ERROR("rec: %s",ptr);  
	//pthread_mutex_lock(&g.Handle_mtc);
	size_t written = fwrite(ptr, size, count, (FILE *)response);
	return written;

//	ProcessAmazonDirective((char *)ptr, (int)(count * size));
	//pthread_mutex_unlock(&g.Handle_mtc);

//	return count;  
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}




static char *curl_get(char* url,int flage)
{
	CURL *curl;
	CURLcode res;
	 long length = 0;
	
	char * response = NULL;
	char *progress_data = "* ";  

	curl = curl_easy_init();
  	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

		if (flage == 0){
			unlink(VOICE_FW_FILE);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, VOICE_FW_FILE);
		}else{
			unlink(BT_FW_FILE);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, BT_FW_FILE);
		}
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);  
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);  
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data); 	 

		
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
    	if (CURLE_OK == res) {
			length = getFileSize(TMP_FILE);
			printf("length:%ld",length);
			response = malloc(length);
			if(response == NULL) {
			//	printf("calloc failed");
				return NULL;
			}
			FILE *fp = NULL;
      		fp = fopen(TMP_FILE, "r");
      		if (fp) {
        		int read=0;
          	if( (read=fread(response, 1, length, fp)) != length){
          	  printf("fread failed read:%d",read);
          	  fclose(fp);
          	  return NULL;
          	}
          	fclose(fp);
         	 printf("fread ok read:%d",read);
      } else {
      	printf("fopen failed");
      }  
		}
	}
	
	return response;
}


int Panther_voice_bt_upgrade(struct json_object *json_obj)
{
	int fail = 0;
	char *pmethod = NULL;
	char *url_str = NULL;
	struct json_object *asr_method_obj = NULL; 
	struct json_object *array_params_obj = NULL, *array_url_obj = NULL;


	array_params_obj = json_object_object_get(json_obj, "files");
	if(array_params_obj == NULL)
	{
		printf("array_params_obj is null\n");
		goto JS_OBJ_ERROR;
	}

	array_url_obj = json_object_array_get_idx(array_params_obj,0);
//	DEBUG_ERROR("array_url_obj = %s",json_object_to_json_string(array_url_obj));

	asr_method_obj = json_object_object_get(array_url_obj, "update_url");
	if(asr_method_obj == NULL)
	{
		printf("asr_method_obj is null\n");
		goto JS_OBJ_ERROR;
	}else{
		url_str = strdup(json_object_get_string(asr_method_obj));
		DEBUG_ERROR("url_str = %s",url_str);
		if (voice_bt_flage == 0){
			system("cdb set duer_nocheck 1 && killall duer_linux");
			system("echo 3 > /proc/sys/vm/drop_caches");
			system(PLAYER "/root/appresources/upgrading.mp3");
			curl_get(url_str,voice_bt_flage);
			system("/sbin/sysupgrade /tmp/firmware");
			sleep(5);
			exit(0);
		}else{
			system("killall uartd");
			system("cdb set duer_nocheck 1 && killall duer_linux");
			system(PLAYER "/root/appresources/bluetooth_upgrade.mp3");
			curl_get(url_str,voice_bt_flage);
			system("/usr/bin/btup &");
		}
		//eval("/sbin/sysupgrade", "/tmp/firmware");
	}


JS_OBJ_ERROR:
	json_object_put(json_obj); 	////对总对象释放
		
OTHER_ERROR:
	if(fail)return -1;	

	return 0;

}



int analysis_json(const char *asr_cmd)
{	
	int fail = 1;
	char *pmethod = NULL;
	
	struct json_object *asr_root_obj = NULL, *asr_method_obj = NULL; 
	struct json_object *array_params_obj = NULL, *array_url_obj = NULL;
	struct json_object *array_title_obj = NULL, *array_artist_obj = NULL;
	struct json_object *array_image_obj = NULL, *array_vendor_obj = NULL;

	
	if(asr_cmd == NULL)
	{
		printf("asr_cmd is null\n");
		goto OTHER_ERROR;
	}

	asr_root_obj = json_tokener_parse(asr_cmd);
	if(is_error(asr_root_obj))
	{
		printf("oh, my god. json_tokener_parse err\n");  
		goto JS_OBJ_ERROR;
	}
	asr_method_obj = json_object_object_get(asr_root_obj, "data"); //获取目标对象
	if(asr_method_obj == NULL)
	{
		printf("asr_method_obj is null\n");
		goto JS_OBJ_ERROR;
	}	

	array_params_obj = json_object_object_get(asr_method_obj, "isupdate"); //获取目标对象
	if(array_params_obj == NULL)
	{
		printf("asr_method_obj is null\n");
		goto JS_OBJ_ERROR;
	}

	pmethod = strdup(json_object_get_string(array_params_obj));

	if(strcmp(pmethod, "true") == 0)
	{
		if(Panther_voice_bt_upgrade(asr_method_obj) < 0)
			goto JS_OBJ_ERROR;
	}else{
		if (voice_bt_flage == 0){
			printf("Voice Isupdate is false,devices No upgrades required\n");
		}else{
			printf("BT Isupdate is false,devices No upgrades required\n");
		}
		goto JS_OBJ_ERROR;
	}

	

JS_OBJ_ERROR:
		json_object_put(asr_root_obj);	////对总对象释放
	
		if(pmethod != NULL)
			free(pmethod);
		
OTHER_ERROR:
		if(fail)return -1;	
	
		return 0;


}



int down_channel_curl_easy_init(void)
{
	CURLcode code;
	CURL *curl;
	CURLMcode mcode;
//	struct data config;
	char *progress_data = "* ";  
	struct curl_slist *httpHeaderSlist;

	CURL *curl_handle;
	CURLcode res;
	
	struct MemoryStruct chunk;
	
	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	curl = curl_easy_init();
	if (curl == NULL) 
	{
		DEBUG_ERROR("curl_easy_init() failed\n");
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, host_header);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

#if 0
	httpHeaderSlist = NULL;
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, sv);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, sv);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, mode_name);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, appplt);
	httpHeaderSlist = curl_slist_append(httpHeaderSlist, mac_addr);
	if (httpHeaderSlist == NULL)
 		return -1;

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeaderSlist);
#endif
	//curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Header_callback);

	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);  
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data); 


	//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	if (DEBUG_ON)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	}

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300);
	
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
  /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	res = curl_easy_perform(curl);
	/* check for errors */ 
	if(res != CURLE_OK) {
	  fprintf(stderr, "curl_easy_perform() failed: %s\n",
			  curl_easy_strerror(res));
	}
	else {	
	  printf("%lu bytes retrieved\n", chunk.size);
	//  DEBUG_ERROR("%s", chunk.memory);
	}

	curl_easy_cleanup(curl);


//	printf(LIGHT_RED "handler:%p\n"NONE, curl);

	int re = analysis_json(chunk.memory);

	return 0;
}


int main(int argc, char *argv[])
{
	/*******************************
	*submit_token:0 check voice url*
	*						 *
	*submit_token:1 check bt url   *	
	*******************************/
	
	submit_token(0);
	down_channel_curl_easy_init();

	DEBUG_ERROR("************************");
	submit_token(1);
	down_channel_curl_easy_init();
	

	return 0;		
}
