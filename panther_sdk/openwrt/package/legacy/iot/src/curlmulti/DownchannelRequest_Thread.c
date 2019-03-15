#include "globalParam.h"
#include <curl/curl.h>





size_t DownChannelHeaderCallback(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{


	return (nmemb * size);
}

size_t DownChannelChunkDataCallback(void *buffer,size_t size,size_t count,void **response)  
{
	char * ptr = buffer;

	ProcessIotJson(ptr);

	return count;  
}


#define STATUS_REPORT_URL "http://iot.ai.tuling123.com/iot/status/notify"


bool DownChannelCurlEasyInit(void)
{

}



void MultiInit()
{

	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);
	warning("g:%#x", g);

	g->multi_handle = curl_multi_init();
	if(NULL == g->multi_handle) {
		error("curl_multi_init failed");

		exit(-1);
	}
		
	debug("multi_handle:%#x, curl_version:%s", g->multi_handle, curl_version());

	curl_multi_setopt(g->multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
	curl_multi_setopt(g->multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);


}

void MultiFree()
{

	GlobalParam *g = GetGlobalParam();
	assert(g != NULL);
	warning("g:%#x", g);

	if(NULL == g->multi_handle) {
		error("curl_multi_init failed");

		exit(-1);
	}

	curl_multi_cleanup(g->multi_handle);
	g->multi_handle = NULL;


}


void *DownchannelRequestThread(char *ppath)
{
	while (1)
	{
	
		DownChannelCurlEasyInit();

		CurlSendRequest();


		info("DownchannelRequestThread over..\n");

		exec_cmd("uartdfifo.sh tlkoff && mpc volume 200");

	}
}
 


