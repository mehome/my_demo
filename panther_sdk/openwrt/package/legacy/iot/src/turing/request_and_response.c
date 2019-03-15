#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdio.h> /* printf, sprintf */
#include <dirent.h>
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "request_and_response.h"
#include "myutils/debug.h"


static char upload_head[] = 
	"POST /speech/chat HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: keep-alive\r\n"
	"Content-Length: %d\r\n"
    "Cache-Control: no-cache\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36\r\n"
	"Content-Type: multipart/form-data; boundary=%s\r\n"
    "Accept: */*\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Accept-Language: en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4,zh-TW;q=0.2,es;q=0.2\r\n"
    "\r\n";

static char tts_head[] = 
	"POST /speech/v2/tts HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: keep-alive\r\n"
	"Content-Length: %d\r\n"
    "Cache-Control: no-cache\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36\r\n"
	"Content-Type: application/json\r\n"
    "Accept: */*\r\n"
    "\r\n";


static char upload_parameters[] = 
	"Content-Disposition: form-data; name=\"parameters\"\r\n\r\n%s";

static char upload_speech[] = 
    "Content-Disposition: form-data; name=\"speech\"; filename=\"upload.wav\"\r\n"
    "Content-Type: application/octet-stream\r\n\r\n";





#define USER_SAFE_SOCKET
//#undef USER_SAFE_SOCKET
int get_socket_fd(char *host)
{
    int portno = 80;
    int sockfd, on=1;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    
	
    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { 
		error("ERROR opening socket");
		return sockfd;
	}
	setsockopt (sockfd, SOL_TCP, TCP_NODELAY, &on, sizeof (on));

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) {
		error("ERROR, no such host");
		close(sockfd);
		return -1;
    }
    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        debug("ERROR connecting");
		close(sockfd);
		return -1;
	}
	
    return sockfd;
    
}

//send(socketFd, header_data, ret,0);

int safe_send(int sockfd, void *buf, size_t total_bytes)
{
    int ret = 0;
    int send_bytes, cur_len;
    //debug("prepare to send %d bytes",total_bytes);
    for (send_bytes = 0; send_bytes < total_bytes; send_bytes += cur_len)
    {
        cur_len = send(sockfd, buf + send_bytes, total_bytes - send_bytes, 0);
        if (cur_len == 0)
        {
            error("send tcp packet error, fd=%d, %m", sockfd);
            return -1;
        }
        else if (cur_len < 0)
        {
            if (errno == EINTR)
                cur_len = 0;
            else if (errno == EAGAIN)
            {
                error("errno == EAGAIN,send_bytes=%d",send_bytes);
                return send_bytes;
            }
            else
            {
                error("send tcp packet error, fd=%d, %m", sockfd);
                return -1;
            }
        }
    }
    debug ("send: fd=%d, len=%d", sockfd, send_bytes);
    return send_bytes;
}

int safe_recv(int sockfd, void *buf, size_t nbytes)
{   
    int cur_len;
	int on = 1;
	
    debug("prepare to receive %d bytes",nbytes);
recv_again:
    cur_len = recv (sockfd, buf, nbytes, 0);
    if (cur_len == 0)
    {
        error("connection closed by peer, fd=%d", sockfd);
        return 0;
    }
    else if (cur_len < 0)
    {
        if (errno == EINTR || errno == EAGAIN)
            goto recv_again;
        else
            error("recv error, fd=%d, errno=%d %m", sockfd, errno);
    }
	setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &on, sizeof(int));  
    debug("cur_len = %d",cur_len);
    return cur_len;
}


static void get_resp_header(const char *response, struct resp_header *resp)
{    
	/*获取响应头的信息*/
    
	char *pos = strstr(response, "HTTP/");    
	if (pos) {        
		//debug("status_code: %s", pos);
		sscanf(pos, "%*s %d", &resp->status_code);//返回状态码    
	}
	else 
	{
		resp->status_code = 0;
	} 
	
	pos = strstr(response, "Content-Type:");//返回内容类型    
	if (pos) {      
		//debug("Content-Type: %s", pos);
		sscanf(pos, "%*s %s", resp->content_type);    
	}
	
	
	pos = strstr(response, "Content-Length:");//内容的长度(字节)    
	if (pos) {       
		//debug("Content-Length: %s", pos);
		sscanf(pos, "%*s %ld", &resp->content_length); 
	}
	else 
		resp->content_length = 0;

    pos = strstr(pos,"\r\n\r\n"); //内容的开始
    if(pos){
        //info("contents:%s",pos);
        pos += 4;
        resp->content_buf = pos;
    }
    else
        {info("Can not find \r\n\r\n ");}
        
  
}

#if 1
//-1:超时   0:正常结束
int getResponse(int socket_fd, char **text)
{
    char response[4096];
	char *code  = NULL;
	int length = 0,mem_size=4096;
    struct timeval tv={0};
    fd_set client_fd_set;
    struct resp_header resp;
    int ret=0;
    char *ptmp = NULL,*pstr = NULL;
    int timeout_count = 0;
    info("receiving header start");
    memset(response, 0, 4096);

    while (1)
    {
        if(IsHttpRecvCancled())
        {
            info("HttpRecvPthread cancled");
            ret = -2;
            break;
        }
            
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        
        FD_ZERO(&client_fd_set);
        FD_SET(socket_fd, &client_fd_set);
        
        ret = select(socket_fd + 1, &client_fd_set, NULL, NULL, &tv);
        if(ret == 0)
        {
            timeout_count++;
            if(timeout_count == 20)
            {
                info("超时时间到");   	//超时10s
                ret = -1;
                break;
            }
        }
        else if(ret > 0)
        {
            info("有数据可读。。。。。。。");
            if(FD_ISSET(socket_fd, &client_fd_set))
            {
                if(IsHttpRecvCancled())
                {
                    info("HttpRecvPthread cancled");
                    ret = -2;
                    break;
                }
                ret = recv (socket_fd, response+length, 4096, MSG_DONTWAIT);
                info("received %d byte",ret);
                if(ret < 0 && (errno == EINTR || errno == EAGAIN))
                    continue;
                else if(ret > 0)
                    length += ret;
                else if(ret == 0)
                {
                    error("connection closed by peer, fd=%d", socket_fd);
                    ret = -1;
                    goto exit;
                }
                ptmp = strstr(response,"\r\n\r\n");
                while(1)
                {
                    if(IsHttpRecvCancled())
                    {
                        info("HttpRecvPthread cancled");
                        ret = -2;
                        goto exit;
                    }
                    if(ptmp)
                    {
                        pstr = ptmp;
                        ptmp += strlen("\r\n\r\n");
                        ptmp =  strstr(ptmp,"\r\n\r\n");
                    }
                    else
                    {
                        ret = 0;
                        break;  
                    }
                        
            
                }
                
                if(pstr)
                {
                    pstr += strlen("\r\n\r\n");
                    get_resp_header(response,&resp);
                    if(resp.content_length < 100 || resp.status_code != 200){
                        info("resp.content_length = %d",resp.content_length);
                        ret = 0;
                        break;
                    }
                    //exec_cmd("mpg123.sh /root/iot/received_url.mp3");
                    code = (char *)calloc(resp.content_length+1,1);
                    strncpy(code,pstr,resp.content_length);
                    *text = strdup(code);
                    free(code);
                    code = NULL;
                    ret = 1;
                    break;
                }
                
            }
        }
    }    
    //info("receiving contents over.............");
    debug("contents is [%s]",*text);

exit:
	if(code)
		free(code);
	
	return ret;
}

#else
int getResponse(int socket_fd, char **text)
{
    //char response[4096];
    char *response = NULL;
	char *code  = NULL;
	int length = 0,mem_size=1024;
	response = calloc(1, 1024);
	if(response == NULL)
		return -1;
    memset(response, 0, 1024);
    
    struct resp_header resp;
    int ret=0;

	struct timeval timeout;
	timeout.tv_sec = 3;    
    timeout.tv_usec = 0;
    int retryCount = 0;
	
    //debug("");
    //人为的获取http头部信息，暂时不要数据部分。因为不知道数据长度
    info("begin receive");
    while (1)
    {
/*
	fd_set readSet;
	 FD_ZERO(&readSet);
         FD_SET(socket_fd, &readSet);
         int nRet = select(socket_fd+1, &readSet, NULL, NULL, &timeout);
         if (nRet < 0)    //出错
             return 0;
         if (nRet == 0)    //超时
         {
             if (++retryCount > 10)
                 return 0;
             continue;
         }
         retryCount = 0;
		 
		if(IsHttpRequestCancled()) {
			length = -1;
			goto exit;
		}
		*/
#ifdef USER_SAFE_SOCKET
		ret = safe_recv(socket_fd, response+length, 1);
#else
		ret = recv(socket_fd, response+length, 1,0);
#endif     
		debug("response=%s", response);

		if(ret < 0)
        {
            if(errno == EINTR ||errno == EAGAIN )
            {   
                continue;
            }
        }
        else if(ret == 0)
        {
            debug("connection closed by peer, fd=%d", socket_fd);
            break;
        }
		
		int flag = 0;	
		int i;			
		for (i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);
		if (flag == 4)			  
			break;		  
		length += ret;	
		if(length >= mem_size-1){
			break;
		}
	}
	info("response = [%s]",response);
    debug("response = [%s]",response);
	get_resp_header(response,&resp);
    
	if(resp.status_code != 200 || resp.content_length == 0){
		length = -1;
		goto exit;
	}

	code = (char *)calloc(1,resp.content_length+1);
	if(code == NULL){
		length = -1;
		goto exit;
	}
	ret=0;
	length=0;
	while(1){		
		if(IsHttpRequestCancled()){
			length = -1;
			goto exit;
		}
#ifdef USER_SAFE_SOCKET
		ret = safe_recv(socket_fd, code+length, resp.content_length-length);
#else
		ret = recv(socket_fd, code+length, resp.content_length-length, 0);
#endif		
		if(ret <= 0){
			break;
		}
		length += ret;
		
		if(length==resp.content_length)
			break;
	}
    debug("contents is [%s]",code);
    *text = code;
exit:
	if(response)
		free(response);
	
	return length;
}
#endif
static void GetRandStr(char s[])
{
    struct timeval tv = {0,0};
    gettimeofday(&tv,NULL);
    sprintf(s,"%10ld%08ld",tv.tv_sec,tv.tv_usec);
}


int turingBuildRequest(int socketFd,char *host, char *data, int len, char *jsonStr)
{    
	int ret = 0;
	char *boundary_header = "------AiWiFiBoundary";
	char* end = "\r\n"; 			//结尾换行
	char* twoHyphens = "--";		//两个连字符
	char s[20] = {0};
	GetRandStr(s);
	char *boundary = malloc(strlen(boundary_header)+strlen(s) +1);
	memset(boundary, 0, strlen(boundary_header)+strlen(s) +1);
	strcat(boundary, boundary_header);
	strcat(boundary, s);

	char firstBoundary[64]={0};
	char secondBoundary[64]={0};
	char endBoundary[128]={0};

	sprintf(firstBoundary, "%s%s%s", twoHyphens, boundary, end);
	sprintf(secondBoundary, "%s%s%s%s", end, twoHyphens, boundary, end);
	sprintf(endBoundary, "%s%s%s%s%s", end, twoHyphens, boundary, twoHyphens, end);


	char *parameter_data = malloc(strlen(jsonStr)+ strlen(upload_parameters) + strlen(boundary) + strlen(end)*2 + strlen(twoHyphens) +1);
	memset(parameter_data, 0,strlen(jsonStr)+ strlen(upload_parameters) + strlen(boundary) + strlen(end)*2 + strlen(twoHyphens) +1);
	sprintf(parameter_data, upload_parameters, jsonStr);
	strcat(parameter_data, secondBoundary);

	int content_length = len+ strlen(boundary)*2 + strlen(parameter_data) + strlen(upload_speech) + strlen(end)*3 + strlen(twoHyphens)*3;
	char *header_data= malloc(4096);
	memset(header_data, 0, 4096);

	ret = snprintf(header_data,4096, upload_head, host, content_length, boundary);


	//header_data,boundary,parameter_data,boundary,upload_speech,fileData,end,boundary,boundary_end
#ifdef USER_SAFE_SOCKET
    safe_send(socketFd, header_data, ret);
	safe_send(socketFd, firstBoundary, strlen(firstBoundary));
	safe_send(socketFd, parameter_data, strlen(parameter_data));
    safe_send(socketFd, upload_speech, strlen(upload_speech));  
#else
    send(socketFd, header_data, ret,0);
	send(socketFd, firstBoundary, strlen(firstBoundary),0);
	send(socketFd, parameter_data, strlen(parameter_data),0);
    send(socketFd, upload_speech, strlen(upload_speech),0);    
#endif
    
	


	int w_size=0,pos=0,all_Size=0;
	while(1) {
		
		if(IsHttpRequestCancled()) {
			ret = -1;
			goto exit;
		}
#ifdef USER_SAFE_SOCKET
        pos = safe_send(socketFd,data+w_size,len-w_size);
#else
		pos = send(socketFd,data+w_size,len-w_size,0);
#endif        
		if(pos < 0) {
			ret = -1;
			break;
		}
        info("pos = %d",pos);
		w_size += pos;
		all_Size += len;
		if( w_size== len ){
			w_size=0;
			break;
		}
	}
#ifdef USER_SAFE_SOCKET   
    ret= safe_send(socketFd, endBoundary, strlen(endBoundary));
#else
	ret= send(socketFd, endBoundary, strlen(endBoundary),0);
#endif
exit:
	if(header_data)
		free(header_data);
	if(boundary)
		free(boundary);
	if(parameter_data)
		free(parameter_data);
	return ret;
}


int build_turing_tts_request(int socketFd,char *host, char *jsonStr)
{
    int ret = 0;
    char * header_data = calloc(4096,1);
    int content_length = strlen(jsonStr);
    
    ret = snprintf(header_data,4096, tts_head, host,content_length);
#ifdef USER_SAFE_SOCKET 
    ret = safe_send(socketFd, header_data, ret);         //发送头部信息
    ret = safe_send(socketFd, jsonStr, content_length);  //发送body数据,数据不足16K，一次可以发送完
#else
    ret = send(socketFd, header_data, ret,0);         //发送头部信息
    ret = send(socketFd, jsonStr, content_length,0);  //发送body数据,数据不足16K，一次可以发送完
#endif
    free(header_data);

    return ret;
}
