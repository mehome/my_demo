#include "myPlay.h"
#include "SimpleAudioPlayerImpl.h"
#include "AudioPlayerInterface.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/time.h>


using namespace duerOSDcsApp::mediaPlayer; 
int myPlayProxyInit(SimpleAudioPlayerImpl *playerIstPtr){
	do{
		pause();
	}while(1);
	return 0;
}

#define SEND_PORT 8060

int sockfd = -1;
struct sockaddr_in dest_address;

static int create_broadcast_socket(char* ifname)
{
    int b_opt_val = 1;
    struct ifreq ifr;
    int ret = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd)
    {
        printf("fail to create socket");
        ret = -1;
        goto RETURN;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &b_opt_val, sizeof(int)) == -1)
    {
        printf("fail to setopt broadcast");
        ret = -1;
        goto CLOSE_FD;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_ifrn.ifrn_name, ifname, strlen(ifname));
    ifr.ifr_addr.sa_family = AF_INET;

    if ((ioctl(sockfd, SIOCGIFFLAGS, &ifr)>= 0)&&(ifr.ifr_flags&IFF_UP))
    {
        if (setsockopt(sockfd,SOL_SOCKET,SO_BINDTODEVICE,ifname,strlen(ifname)+1) == -1 )
        {
            printf("setsockopt %s SO_BINDTODEVICE: %m", ifname);
            ret = -1;
            goto CLOSE_FD;
        }
    }
    else
    {
        printf("bind interface %s fail", ifname);
        ret = -1;
        goto CLOSE_FD;
    } 

    memset(&dest_address, 0, sizeof(dest_address));
    dest_address.sin_family      = AF_INET;
    dest_address.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
    dest_address.sin_port        = htons(SEND_PORT);

    return ret;

CLOSE_FD:
    close(sockfd);
    sockfd = -1;

RETURN:
    return ret;
}

static int send_broadcast_to_interface(char* buffer, char* ifname)
{
	int i;
    ssize_t cnt = 0;
    int ret = 0;

    if (-1 == sockfd)
    {
        if (0 != create_broadcast_socket(ifname))
        {
            printf("create_socket %s fail", ifname);
            ret = -1;
            goto RETURN;
        }
    }

	for (i = 0; i < 10; i++)
	{
    	cnt = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&dest_address, (int)sizeof(dest_address));
		usleep(1000 * 100);
	}

    
    if (0>cnt) 
    {
        printf("sendto fail");
        ret = -1;
    } 
    else 
    {
        printf("%s send broadcast %s\n", ifname, buffer); 
    }

    close(sockfd);
    sockfd=-1;

RETURN:

    return ret;
}


int sendToTestTool(long int startTime) {
	int ret = -1;
    if(access("/tmp/test_mode",0)) {
        return -1;  //no test signal
    }

	long int endTime = 0;
	struct timeval tv; 
	gettimeofday(&tv,NULL);	
	endTime = tv.tv_sec;
    char byData[20]={0};

	long int processTime = endTime - startTime;
	sprintf(byData,"myPlay_%02d", processTime);

    if (0 == send_broadcast_to_interface(byData, "br0")) {
        ret = 0;
    }

    if (0 == send_broadcast_to_interface(byData, "br1")) {
        ret = 0;
    }

	return ret;
}


int main(int argc, char *argv[]){
	long int startTime = 0;

	myPlayLogIint("l=111111");
	traceSig(SIGSEGV);
	int ret=0;
	if(!argv[1] || !strcmp(argv[1],"--help")){
		myPlayRaw("             [file path] : play file!\n");
		myPlayRaw("--device default [file path] : user alsa device:All to play!\n");
		return 0;
	}
	ret=strcmp(argv[1],"--device");
	if(!ret){
		assert_arg(3,-1);
		std::string device=argv[2];
		SimpleAudioPlayerImpl playerIst(device);
		std::string path=argv[3];
		myPlayInf("%s palying %s ...",device.c_str(),path.c_str());
		playerIst.audioPlay(path.c_str(),RES_FORMAT::AUDIO_MP3, 0, 15000);
		ret=myPlayProxyInit(&playerIst);
		if(ret<0){
			myPlayInf("palying %s ...",path.c_str());
			return -1;
		}
		return 0;
	}

    if (!access("/tmp/test_mode",0)) {
		struct timeval tv; 
		gettimeofday(&tv,NULL);	
		startTime = tv.tv_sec;
    }

	assert_arg(1,-1);
	SimpleAudioPlayerImpl playerIst("common");
	std::string path=argv[1];
	myPlayInf("palying %s ...",path.c_str());
	playerIst.audioPlay(path.c_str(),RES_FORMAT::AUDIO_MP3, 0, 15000);
	ret=myPlayProxyInit(&playerIst);
	if(ret<0){
		myPlayErr("myPlayProxyInit failure!");
		sendToTestTool(startTime);
		return -1;
	}
	sendToTestTool(startTime);
	return 0;
}
