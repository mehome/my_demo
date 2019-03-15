#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <pcap.h>
#include <tapi/timerfd.h>
#include <pthread.h>
#include <wdk/cdb.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <netinet/if_ether.h>  
#include <net/if.h>  
#include <net/if.h>  
#include <netdb.h>	
#include <net/if.h>	
#include <arpa/inet.h>  
#include <sys/ioctl.h>  
#include <sys/types.h>  
#include <sys/socket.h>	 

//#include <json/.h>	 

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <myutils/ConfigParser.h>
#include "inc/airkiss.h"
#include "inc/utils.h"

#include "myutils/debug.h"

static pthread_t airkiss_rcv_thread_id;
static pthread_t rcv_thread_id;

pcap_t *handle = NULL;
static airkiss_context_t *akcontex = NULL;
#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)

unsigned int ap_found;
uint8_t lan_buf[200];
uint16_t lan_buf_len;
static char g_device_id[128]={0};
static char appid[32]={0};
char *g_appKey[65]={0};

#if defined	ENABLE_YIYA 
#define DEVICE_NAME "br0"
#elif defined ENABLE_LVMENG
#define DEVICE_NAME "br0"
#elif defined ENABLE_HUABEI
#define DEVICE_NAME "br0"
#else
#define DEVICE_NAME "br0"
#endif

#ifndef ENABLE_HUABEI
#ifdef ENABLE_VOICE_DIARY
#define DEVICE_TYPE "gh_ca69fb525566"
#else
#define DEVICE_TYPE "gh_ca1f565e168e"
#endif
#else
#define DEVICE_TYPE "gh_0f529dbca07b" //huabei
#endif

#define APIKEY "c411202498b24af8b6f3f74bf73fc46e"

static int discoverType = 0;
#define DEFAULT_LAN_PORT  12476
#define IP_SIZE 	16	

static const airkiss_config_t akconf = {
    (airkiss_memset_fn)&memset,
    (airkiss_memcpy_fn)&memcpy,
    (airkiss_memcmp_fn)&memcmp,
    (airkiss_printf_fn)&printf
};

static int get_local_ip(const char *eth_inf, char *ip)  
{  
	int sd;       struct sockaddr_in sin;  
	struct ifreq ifr;  
	char buf[32] = {0};
	sd = socket(AF_INET, SOCK_DGRAM, 0);  
	if (-1 == sd)  
	{  
		printf("socket error: %s\n", strerror(errno));  
		return -1;        
	}  

	strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);  
	ifr.ifr_name[IFNAMSIZ - 1] = 0;  

	// if error: No such device  
	if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)  
	{  
		close(sd);  
		return -1;  
	}  

	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));  
	snprintf(buf, IP_SIZE, "%s", inet_ntoa(sin.sin_addr));  
	close(sd);  
	char *tmp = strrchr(buf, '.');
	if(tmp)  {
		buf[tmp - buf] = 0;;
 		snprintf(ip, 16, "%s.255", buf);
 		return 0;
	}

	return -1;  
}  

static int send_package(char *buf, int len)
{
	int sockfd=-1;
	struct sockaddr_in broadcastAddr;
	int numbytes;
	int broadcast = 1;
	struct ifreq ifr;
	char tmp[32] = {0};
	char val;
	int i, ret = 0;
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		ret = -1;
		goto done;
	}
	
	memset(&ifr, 0, sizeof ifr);
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), VERIFY_DEVICE);

	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
		perror("setsockopt (SO_BINDTODEVICE)");
		ret = -1;
		goto done;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
		sizeof broadcast) == -1) {
		perror("setsockopt (SO_BROADCAST)");
		ret = -1;
		goto done;
	}
	
	memset(&broadcastAddr, 0, sizeof broadcastAddr);
	broadcastAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "255.255.255.255", &broadcastAddr.sin_addr);
	broadcastAddr.sin_port = htons(DEFAULT_LAN_PORT);

	if ((numbytes=sendto(sockfd, buf, len, 0,
			(struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr))) == -1) {
		perror("sendto");
		ret = -1;
		goto done;
	}

done:
	if(sockfd != -1)
		close(sockfd);
	return ret;
}

static void time_callback(void)
{
	int ret;
	lan_buf_len = sizeof(lan_buf);
	ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD, appid, g_device_id, 0, 0, lan_buf, &lan_buf_len, &akconf);
	
	if (ret != AIRKISS_LAN_PAKE_READY) {
		printf("Pack lan packet error!");
		return;
	}
    
#if 0    
        printf("lan_buf=[%s],size = %d\n",lan_buf+16,lan_buf_len);
        int i = 0;
        for(i=0;i <lan_buf_len;i++ )
        {
            printf("%02x ",lan_buf[i]);
        }
        printf("\n");
    
        unsigned char final[200] = {0};
        char *buf = "{\"deviceInfo\":{\"deviceType\":\"gh_6a59209e0881\",\"deviceId\":\"f250c4525e104fb69c0be31de60f996e_aiAA8005dfc07576\",\"test\":\"This is test\"}}";
        printf("buf=[%s],size = %d\n",buf,strlen(buf)+16);
        memcpy(final,lan_buf,16);
        strcat(final+16,buf);
        final[11] = strlen(buf)+16; //设置长度
    
        for(i=0;i <strlen(buf)+16;i++ )
        {
            printf("%02x ",final[i]);
        }
        printf("\n");
        printf("final = %p,len = %d\n",final, strlen(buf)+16);
        ret = send_package(final, strlen(buf)+16);
#endif
	ret = send_package(lan_buf, lan_buf_len);
	if (ret != 0)  
	{
		//printf("UDP send error!");
	}
	else 
	{
		//printf("Finish send notify!");
	}

}


void wifilan_recv_callbk(void *arg, char *pdata, unsigned short len)
{
	airkiss_lan_ret_t ret = airkiss_lan_recv(pdata, len, &akconf);
	airkiss_lan_ret_t packret;
	switch (ret) {
		case AIRKISS_LAN_SSDP_REQ:
			cprintf("AIRKISS_LAN_SSDP_REQ\n");
			lan_buf_len = sizeof(lan_buf);
			packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD, appid, g_device_id, 0, 0, lan_buf, &lan_buf_len, &akconf);
			if (packret != AIRKISS_LAN_PAKE_READY) {
				printf("Pack lan packet error!");
				return;
			}
			packret = send_package(lan_buf, lan_buf_len);
			if (packret != 0) {
				printf("LAN UDP Send err!");
			}
			break;
		default:
			//printf("Pack is not ssdq req!\n");
			break;
	}
}


static void airkiss_rcv_thread(void)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    printf("airkiss_rcv_thread...\n");
	handle = pcap_open_live(MONITOR_DEVICE, BUFSIZ, 1, 0, errbuf);
    if(handle == NULL) {
		printf("Couldn't open device %s: %s\n", MONITOR_DEVICE, errbuf);
		return;
	}
	
	pcap_loop(handle, -1, wifilan_recv_callbk, NULL);
	
	return;
}

void recv_thread(void) 
{
	struct sockaddr_in addrto;  
    bzero(&addrto, sizeof(struct sockaddr_in));  
    addrto.sin_family = AF_INET;  
    addrto.sin_addr.s_addr = htonl(INADDR_ANY);  

	char buf[32]= {0};
	inet_pton(AF_INET, "255.255.255.255", &addrto.sin_addr);
    addrto.sin_port = htons(12476);  
    struct sockaddr_in from;  
    bzero(&from, sizeof(struct sockaddr_in));  
    from.sin_family = AF_INET;  
    from.sin_addr.s_addr = htonl(INADDR_ANY);  
	
    from.sin_port = htons(12476);  
  
    int sock = -1;  
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)   
    {     
       printf("socket error\n");   
       	exit(-1);  
    }  
	const int opt = 1;  
  
    int nb = 0;  
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));  
    if(nb == -1)  
    {  
       printf("setsockopt error\n");   
       	exit(-1);   
    }  
    if(bind(sock,(struct sockaddr *)&(addrto), sizeof(struct sockaddr_in)) == -1)   
    {     
       printf("bind error\n");   
       	exit(-1);  
    }  
    int len = sizeof(struct sockaddr_in);  
    char smsg[200] = {0};   
	while(1)  
    {  
        int ret=recvfrom(sock, smsg, 100, 0, (struct sockaddr*)&from,(socklen_t*)&len);  
		//printf("from:%s\n",inet_ntoa(from.sin_addr));  
        if(ret<=0)  
        {  
            printf("read error....\n");  
        }  
        else  
        {         
            printf("%s\t", smsg);     
        }  
  
        sleep(1);  
    }  
   
}

int GetDeviceID(char *temp_id)
{
	char *device = DEVICE_NAME; 
	//char temp_id[128] = {0};
	unsigned char macaddr[ETH_ALEN]; 
	int i,s;  
	s = socket(AF_INET,SOCK_DGRAM,0); 
	struct ifreq req;  
	int err,rc;  
	rc	= strcpy(req.ifr_name, device); 
	err = ioctl(s, SIOCGIFHWADDR, &req); 
	close(s);  
#ifndef ENABLE_HUABEI
	sprintf(temp_id, "%s", "aiAA");
#else
	sprintf(temp_id, "%s", "draw");
#endif
	if (err != -1 )  
	{  
		memcpy(macaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); 
		for(i = 0; i < ETH_ALEN; i++)  
		{
			sprintf(&temp_id[(2+i) * 2], "%02x", macaddr[i]);
		}
        //memcpy(temp_id,"aiAA8005dfc498ea",strlen("aiAA8005dfc498ea"));
	}
	return 0;
}

static void airkiss_apply(void)
{
    int sockfd = -1;
    struct sockaddr_in broadcastAddr;
    int numbytes;
    int broadcast = 1;
    struct ifreq ifr;
    char buf[4];
    char val;
    int i;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        goto done;
    }

    memset(&ifr, 0, sizeof ifr);
    snprintf(ifr.ifr_name, sizeof (ifr.ifr_name), VERIFY_DEVICE);

    if (setsockopt
        (sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *) &ifr, sizeof (ifr)) < 0)
    {
        perror("setsockopt (SO_BINDTODEVICE)");
        goto done;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
                   sizeof broadcast) == -1)
    {
        perror("setsockopt (SO_BROADCAST)");
        goto done;
    }

    memset(&broadcastAddr, 0, sizeof broadcastAddr);
    broadcastAddr.sin_family = AF_INET;
    char tmp[32]= {0};
    inet_pton(AF_INET, "255.255.255.255", &broadcastAddr.sin_addr);
    broadcastAddr.sin_port = htons(AIRKISSPORT);

    cdb_get("$airkiss_val", &buf);
    val = atoi(buf);

    for (i = 0; i < AIRKISSTRYCOUNT; i++)
    {
        if ((numbytes = sendto(sockfd, &val, 1, 0,
                               (struct sockaddr *) &broadcastAddr,
                               sizeof (broadcastAddr))) == -1)
        {
            perror("sendto");
            goto done;
        }
    }
done:
    if (sockfd != -1)
        close(sockfd);
    cdb_set_int("$airkiss_state", AIRKISS_STANDBY);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int debug = 1;
	struct itimerspec new_value;
	struct timespec now;
	int fd;
	uint64_t exp;
	pthread_attr_t tattr;
	char *value = NULL;
	char *type = NULL; 
	char *apikey = NULL; 
	PidLock(argv[0]);
	char *deviceId[64] ={0};
	GetDeviceID(&deviceId);

	log_set_quiet(1);
	log_set_level(LOG_ERROR);
	value = ConfigParserGetValue("appID");
	if(value){
		int len = strlen(value);
		memcpy(appid ,value ,len);
		appid[len] = '\0';
		free(value);
	} else {
		int len = strlen(DEVICE_TYPE);
		memcpy(appid ,DEVICE_TYPE ,len);
		appid[len] = '\0';
	
	}
#ifndef ENABLE_HUABEI
	apikey = ConfigParserGetValue("apiKeyTuring");
	if(apikey){
		int len = strlen(apikey);
		memcpy(g_appKey,apikey ,len);
		g_appKey[len] = '\0';
		free(apikey);
	} else {
		int len = strlen(APIKEY);
		memcpy(g_appKey ,APIKEY ,len);
		g_appKey[len] = '\0';
	}
	type = ConfigParserGetValue("discoverType");
	if(type){
		discoverType = atoi(type);
		free(type);
	}
	if(discoverType == 1) {
		strcat(g_device_id,g_appKey);
		strcat(g_device_id,"_");
		strcat(g_device_id,deviceId);
	} else {
		strcat(g_device_id,deviceId);
	}
#else
	strcat(g_device_id,deviceId);
#endif	
	printf("%s:%d g_device_id:\"%s\" deviceType:\"%s\" , discoverType:%d\n\n", __func__, __LINE__,g_device_id,appid, discoverType);
	//cprintf("%s:%d g_device_id:\"%s\" deviceType:\"%s\" , discoverType:%d\n\n", __func__, __LINE__,g_device_id,appid, discoverType);
	pthread_attr_init(&tattr);
	pthread_attr_setstacksize(&tattr, PTHREAD_STACK_MIN);
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&airkiss_rcv_thread_id, &tattr, airkiss_rcv_thread, NULL);
	pthread_attr_destroy(&tattr);
	
	if(ret)
		goto done;

	/*Timer to change channel*/
	if(clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
		printf("clock_gettime");
		goto done;
	}
	daemon(0, debug);
	new_value.it_value.tv_sec = now.tv_sec + 2;
	new_value.it_value.tv_nsec = now.tv_nsec;
	
	//new_value.it_interval.tv_sec = 3;
	new_value.it_interval.tv_sec = 2;
	new_value.it_interval.tv_nsec = 300000000;
	
	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if(fd == -1) {
		printf("timerfd create fail");
		goto done;
	}
	
	if(timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1) {
		printf("timerfd set fail");
		goto done;
	}
	
	while(1) {
		ssize_t s;
		s = read(fd, &exp, sizeof(uint64_t));
		time_callback();
	
	}
	
	if(handle) {
		pcap_close(handle);
	}
	pcap_breakloop(handle);

done:
	PidUnLock();
	if(akcontex)
		free(akcontex);

	return ap_found;
}
