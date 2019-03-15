/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 *
 * Alibaba Group retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have
 * obtained a separate written license from Alibaba Group., you are not
 * authorized to utilize all or a part of this computer program for any
 * purpose (including reproduction, distribution, modification, and
 * compilation into object code), and you must immediately destroy or
 * return to Alibaba Group all copies of this computer program.  If you
 * are licensed by Alibaba Group, your rights to utilize this computer
 * program are limited by the terms of that license.  To obtain a license,
 * please contact Alibaba Group.
 *
 * This computer program contains trade secrets owned by Alibaba Group.
 * and, unless unauthorized by Alibaba Group in writing, you agree to
 * maintain the confidentiality of this computer program and related
 * information and to not disclose this computer program and related
 * information to any other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND
 * Alibaba Group EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <net/if.h>	      // struct ifreq
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <net/ethernet.h>     // ETH_P_ALL
#include <linux/if_packet.h>
#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <sys/time.h>
#include <net/ethernet.h>     // ETH_P_ALL
#include <net/if.h>	      // struct ifreq
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>

#include <libutils/wlutils.h>
#include <libutils/nl80211.h>
#include <errno.h>
#include <openssl/aes.h>


//#include <pcap/pcap.h>
#include <unistd.h>
#include "montage_api.h"
//#include "aes.h"
//#include "crypto.h"
#include "platform.h"
#include "platform_config.h"
#define IFNAME			"br1"	//TODO: wlan0
#define info(format, ...)	printf(format, ##__VA_ARGS__)
#define MONITOR_DEVICE "mon"
/* socket handler to recv 80211 frame */
//static int sock_fd;
/* buffer to hold the 80211 frame */
//static char *ether_frame;
//pcap_t *handle = NULL;
pthread_t thread_id;
platform_awss_recv_80211_frame_cb_t p_cb =NULL;
//extern int is_wifi_scan;
//wifi信道切换，信道1-13
#define HAL_PRIME_CHNL_OFFSET_DONT_CARE	0
#define HAL_PRIME_CHNL_OFFSET_LOWER	1
#define HAL_PRIME_CHNL_OFFSET_UPPER	2
int is_wifi_rev_data;

#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_evalpid(argv, ">/dev/console", 0, NULL); \
})



//一键配置超时时间, 建议超时时间1-3min, APP侧一键配置1min超时
int platform_awss_get_timeout_interval_ms(void)
{
    return 1 * 60 * 1000;
}

//默认热点配网超时时间
int platform_awss_get_connect_default_ssid_timeout_interval_ms( void )
{
    return 0;
}

//一键配置每个信道停留时间, 建议200ms-400ms
int platform_awss_get_channelscan_interval_ms(void)
{
    return 200;
}


#if 1
static int set_if_flags(char *ifname, short flags)
{
    int sockfd;
    struct ifreq ifr;
    int res = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
        return -1;

    memset(&ifr, 0, sizeof ifr);

    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    res = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    if (res < 0)
    {
        cprintf("Interface '%s': Error: SIOCSIFFLAGS failed: %s\n",
                ifname, strerror(errno));
    }

    close(sockfd);

    return res;
}

static int set_if_up(char *ifname, short flags)
{
    return set_if_flags(ifname, flags | IFF_UP);
}

static int nl80211_init(struct nl80211_state *state)
{
    int err;

    state->nl_sock = nl_socket_alloc();
    if (!state->nl_sock)
    {
        cprintf("Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    if (genl_connect(state->nl_sock))
    {
        cprintf("Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
    if (state->nl80211_id < 0)
    {
        cprintf("nl80211 not found.\n");
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

out_handle_destroy:
    nl_socket_free(state->nl_sock);
    return err;
}

#endif

static void monitor_interface_add(void)
{
    struct nl_msg *msg = NULL;
    struct nl80211_state nlstate;
    int err = -1;
    unsigned int devidx = 0;

    /* get device index of "wlan0" */
    devidx = if_nametoindex(SCAN_DEVICE);
    if (devidx == 0)
    {
        cprintf("cannot find wlan0\n");
        devidx = -1;
	return;
    }

    err = nl80211_init(&nlstate);
    if (err)
    {
        cprintf("nl80211 init fail\n");
        return;
    }

    msg = nlmsg_alloc();
    if (!msg)
    {
        cprintf("failed to allocate netlink message\n");
        goto nla_put_failure;
    }
    genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, 0, NL80211_CMD_NEW_INTERFACE,
                0);

    /* put data in netlink message */
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

    NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, MONITOR_DEVICE);
    NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);

    err = nl_send_auto_complete(nlstate.nl_sock, msg);
    if (err < 0)
    {
        cprintf("Send netlink message fail\n");
        goto out_free_msg;
    }

    set_if_up(MONITOR_DEVICE, 0);

out_free_msg:
    if (msg)
        nlmsg_free(msg);
nla_put_failure:
    nl_socket_free(nlstate.nl_sock);
    return;
}

static void monitor_interface_del(void)
{
    struct nl_msg *msg = NULL;
    struct nl80211_state nlstate;
    int err = -1;
    unsigned int devidx = 0;

    /* get devide index of "mon" */
    devidx = if_nametoindex(MONITOR_DEVICE);
    if (devidx == 0)
    {
        cprintf("cannot find mon\n");
        devidx = -1;
	return;
    }

    err = nl80211_init(&nlstate);
    if (err)
    {
        cprintf("nl80211 init fail\n");
        return;
    }

    msg = nlmsg_alloc();
    if (!msg)
    {
        cprintf("failed to allocate netlink message\n");
        goto nla_put_failure;
    }
    genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, 0, NL80211_CMD_DEL_INTERFACE,
                0);

    /* put data in netlink message */
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

    err = nl_send_auto_complete(nlstate.nl_sock, msg);
    if (err < 0)
    {
        cprintf("Send netlink message fail\n");
        goto out_free_msg;
    }

out_free_msg:
    if (msg)
        nlmsg_free(msg);
nla_put_failure:
    nl_socket_free(nlstate.nl_sock);
    return;
}

#if 0
static void mon_srv_stop(void)
{
    exec_cmd("mtc ra stop");
    exec_cmd("mtc wl stop");
    sleep(2);
}
#endif
#define SPEEDUP_WLAN_CONF

#if 1
static void mon_srv_stop(void)
{
    #ifdef SPEEDUP_WLAN_CONF
   cdb_set_int("$op_work_mode", 1);
    cdb_set_int("$wanif_state", 0);
    exec_cmd("mtc wlhup");
    sleep(1); 
    #else
    exec_cmd("mtc wl stop");
    sleep(2);
    #endif
}
#endif

void wifi_promiscuous_enable(bool is_open)
{
    if (is_open)
    {
        monitor_interface_add();
        exec_cmd("wd smrtcfg start usr");
    }
    else
    {
        exec_cmd("wd smrtcfg stop usr");
        monitor_interface_del();
    }
}


void platform_awss_switch_channel(char primary_channel,
		char secondary_channel, uint8_t bssid[ETH_ALEN])
{
	static char oldchan=0;
//	printf("-----%s------%d---CHAN--%d\n",__FUNCTION__,__LINE__,primary_channel);

	if(oldchan != primary_channel)
	{
		oldchan = primary_channel;
		char cmd[128];
		snprintf(cmd, 128, "wd smrtcfg chan %d", primary_channel);
		exec_cmd(cmd);
	}


}

uint8_t ascii_to_num(uint8_t num)
{
    uint8_t ret = 0;

    if(num >= 0x30 && num <= 0x39) //0-9
    {
        ret = num - 0x30;
    }

    if(num >= 0x41 && num <= 0x46) //A-F
    {
        ret = num - 0x41 + 10;
    }

    if(num >= 0x61 && num <= 0x66) //a-f
    {
        ret = num - 0x61 + 10;
    }
    return ret;
}


#define WIFIOFFSET	18
#if 1

static void processpkt(unsigned char *args, char *packet,
                      unsigned int len)
{
    int ret = 0;
	int i = 0;

    if (len > WIFIOFFSET)
    {
#ifdef DUMPBUF
        dump_buf((unsigned int *) packet, len);
#endif
        packet += WIFIOFFSET;
        len -= WIFIOFFSET;

	/*

	char *r_n = "\n";

	FILE *fp = fopen("/root/log.txt","ab+"); 
	if (NULL == fp)
		printf("!fp NULL|\n");
	for(i = 0; i <len; i++){
	//	printf("%x", packet[i]);
		fprintf(fp,"%x",packet[i]);
	}

	fwrite(r_n,1,sizeof(r_n),fp);  
	printf("\n");

	fclose(fp);
	fp = NULL;

	*/

	if(p_cb)
		p_cb(packet,len,AWSS_LINK_TYPE_80211_RADIO,0);



    }

}


void* open_socket(void* ptr)
{
	int saddr_size , data_size;
	struct sockaddr saddr;
	unsigned char *buffer = (unsigned char *)malloc(4096);
	int sock_raw;
	struct ifreq ifr;
	struct sockaddr_ll sll;
	
	sock_raw = socket(PF_PACKET , SOCK_RAW , ETH_P_ALL);

	if(sock_raw < 0)
	{
	    cprintf("Socket Error\n");
	    free(buffer);
	    return 1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, MONITOR_DEVICE, IFNAMSIZ);

	if (ioctl(sock_raw, SIOCGIFINDEX, &ifr ) < 0) {
	    goto fail;
	}

	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;
	sll.sll_protocol = htons(ETH_P_ALL);

	if(bind(sock_raw, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
	    goto fail;
	}

	setsockopt(sock_raw, SOL_SOCKET, SO_BINDTODEVICE, MONITOR_DEVICE, 3);
	while(1)
	{
		if (is_wifi_rev_data == 1)
			break;
		saddr_size = sizeof(saddr);
		data_size = recvfrom(sock_raw , buffer , 4096 , 0 , &saddr , &saddr_size);
		if(data_size < 0 ){
			printf("Recvfrom error , failed to get packets\n");
			goto fail;
		}
		processpkt(NULL, buffer, data_size);
	}
fail:
    close(sock_raw);
    free(buffer);
}

#endif

int open_socket_1(void)
{
	int fd;
#if 0
	if (getuid() != 0)
		err("root privilege needed!\n");
#endif
	//create a raw socket that shall sniff
	fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	assert(fd >= 0);

	struct ifreq ifr;
	int sockopt = 1;

	memset(&ifr, 0, sizeof(ifr));

	/* set interface to promiscuous mode */
	strncpy(ifr.ifr_name, WLAN_IFNAME, sizeof(ifr.ifr_name));
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("ioctl(SIOCGIFFLAGS)");
		goto exit;
	}
	ifr.ifr_flags |= IFF_PROMISC;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		perror("ioctl(SIOCSIFFLAGS)");
		goto exit;
	}

	/* allow the socket to be reused */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                &sockopt, sizeof(sockopt)) < 0) {
		perror("setsockopt(SO_REUSEADDR)");
		goto exit;
	}

	/* bind to device */
	struct sockaddr_ll ll;

	memset(&ll, 0, sizeof(ll));
	ll.sll_family = PF_PACKET;
	ll.sll_protocol = htons(ETH_P_ALL);
	ll.sll_ifindex = if_nametoindex(WLAN_IFNAME);
	if (bind(fd, (struct sockaddr *)&ll, sizeof(ll)) < 0) {
		perror("bind[PF_PACKET] failed");
		goto exit;
	}

	return fd;
exit:
	close(fd);
	exit(EXIT_FAILURE);
}

pthread_t monitor_thread;
char monitor_running;

void *monitor_thread_func(void *arg)
{
    platform_awss_recv_80211_frame_cb_t ieee80211_handler = arg;
    /* buffer to hold the 80211 frame */
    char *ether_frame = malloc(IP_MAXPACKET);
	assert(ether_frame);

    int fd = open_socket_1();
    int len, ret;
    fd_set rfds;
    struct timeval tv;

    while (monitor_running) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;//100ms

        ret = select(fd + 1, &rfds, NULL, NULL, &tv);
        assert(ret >= 0);

        if (!ret)
            continue;

        //memset(ether_frame, 0, IP_MAXPACKET);
        len = recv(fd, ether_frame, IP_MAXPACKET, 0);
        if (len < 0) {
            perror ("recv() failed:");
            //Something weird happened
            continue;
        }

        /*
         * Note: use tcpdump -i wlan0 -w file.pacp to check link type and FCS
         */

        /* rtl8188: include 80211 FCS field(4 byte) */
        int with_fcs = 1;
        /* rtl8188: link-type IEEE802_11_RADIO (802.11 plus radiotap header) */
        int link_type = AWSS_LINK_TYPE_80211_RADIO;

        (*ieee80211_handler)(ether_frame, len, link_type, with_fcs);
    }

    free(ether_frame);
    close(fd);

    return NULL;
}


//进入monitor模式, 并做好一些准备工作，如
//设置wifi工作在默认信道6
//若是linux平台，初始化socket句柄，绑定网卡，准备收包
//若是rtos的平台，注册收包回调函数aws_80211_frame_handler()到系统接口
void platform_awss_open_monitor(platform_awss_recv_80211_frame_cb_t cb)
{
	int err = -1;

	mon_srv_stop();

	wifi_promiscuous_enable(true);
	is_wifi_rev_data = 0;
//	

//	exec_cmd("wd smrtcfg start fixed");	
	printf("wd smrtcfg start fixed done -------------------------\n");
//	exec_cmd("wd smrtcfg start usr");

	p_cb = cb;

	pthread_attr_t tattr;
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	err = pthread_create(&thread_id, &tattr, open_socket, NULL);
	
	if(err!=0)
		printf("create thread error\n");
	pthread_attr_destroy(&tattr);
#if 0
out_free_msg:
	if(msg)
		nlmsg_free(msg);
nla_put_failure:
	nl_socket_free(nlstate.nl_sock);
	return; 
#endif
	return; 
}

//退出monitor模式，回到station模式, 其他资源回收
void platform_awss_close_monitor(void)
{
		struct nl_msg *msg = NULL;
		struct nl80211_state nlstate;
		int err = -1;
		unsigned int devidx = 0;
//		is_wifi_scan = 1;
		is_wifi_rev_data = 1;
		exec_cmd("wd smrtcfg stop");	
		wifi_promiscuous_enable(false);
		printf("-----%s------%d---------------------------is_wifi_rev_data = %d\n",__FUNCTION__,__LINE__,is_wifi_rev_data);
#if 1
		/* get devide index of "mon" */
		devidx = if_nametoindex(MONITOR_DEVICE);
		if (devidx == 0) {
			printf("cannot find mon\n");
			devidx = -1;
		}
	
		err = nl80211_init(&nlstate);
		if (err) {
			printf("nl80211 init fail\n");
			return;
		}
		
		msg = nlmsg_alloc();
		if (!msg) {
			printf("failed to allocate netlink message\n");
			return;
		}
		genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, 0, NL80211_CMD_DEL_INTERFACE, 0);
		
		/* put data in netlink message */
		NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);
	
		err = nl_send_auto_complete(nlstate.nl_sock, msg);
		if (err < 0) {
			printf("Send netlink message fail\n");
			goto out_free_msg;
		}
	
	out_free_msg:
		if(msg)
			nlmsg_free(msg);
	nla_put_failure:
		nl_socket_free(nlstate.nl_sock);
		return; 
#endif
}


void swapspace(char *in ,char* out)  
{  
	int count = 0;

	if(in && out)
	{
		while (*in != '\0' )  
		{  
			printf("*in ======0x%x\n",*in);
	#if 1
			if (*in == 0x20)  
			{  
				*(out) = '\\';
				printf("*out ======0x%x\n",*out);

				out++;
				*(out) = *in;
				printf("*out ======0x%x\n",*out);

			}
			else
			{
				*(out) = *in;
					printf("*out ======0x%x\n",*out);

			}  
	#endif
			out++;
			in++;  
		}  
	}

}

int wifi_connect;


void *Timer_PowerOff_Handle(void)
{
	int timeEnable = 0;
	int setTime = 0;
       char ip_str[PLATFORM_IP_LEN];
            uint32_t time_start, time_end;
    struct timeval tv = { 0 };
    struct ifreq ifreq;
        int sock = -1;
        char ifname_buff[IFNAMSIZ] = {0};
	  gettimeofday(&tv, NULL);
          time_start = tv.tv_usec;
          printf("------------time_start = %d \n",time_start);
	while(1)
	{
          if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
        }

    ifreq.ifr_addr.sa_family = AF_INET; //ipv4 address
    strncpy(ifreq.ifr_name, "br1", IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFADDR, &ifreq) < 0) {
        close(sock);
        perror("ioctl");
        return -1;
    }

    close(sock);

    strncpy(ip_str,
            inet_ntoa(((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr),
            PLATFORM_IP_LEN);
    if(NULL != strstr(ip_str, "192.168.7."))
    {

        gettimeofday(&tv, NULL);
          time_end = tv.tv_usec;
          int i =time_end-time_start ;
          printf("------------time_end = %d \n",time_end);
          printf("------------dhcp time = %d \n",i);
          break;
                
    }
       
         usleep(1000 * 100);
           
           }
           
 }
int Timer_PowerOff_Pthread_Create(void)
{
	int ret = 0;
	pthread_t timerThread;
	
    ret = pthread_create(&timerThread,NULL,(void *)Timer_PowerOff_Handle,NULL);
    if(0 != ret)
    {
    	printf("pthread_create fail\n");
		return -1;
    }
	
 	//pthread_join(timerThread, NULL);

	return ret;
}

char tempssid[32*2+1] = {0}; //32 space 


#ifdef SPEEDUP_WLAN_CONF
void set_ssid_pwd(char *ssid, char *pass)
{
    if (strlen(ssid) > 32)
    {
        printf("critical error, ssid two long : %s\n", ssid);
        return;
    }

    if (pass)
    {
        if (strlen(pass) > 64)
        {
            printf("critical error, pass two long : pass:%s\n", pass);
            return;
        }
    }

    cdb_set("$wl_bss_ssid2", ssid);
    cdb_set("$wl_bss_bssid2", "");

    if (!pass)
    {
        cdb_set("$wl_bss_sec_type2", "0");
        cdb_set("$wl_bss_cipher2", "0");
        cdb_set("$wl_bss_wpa_psk2", "");
        cdb_set("$wl_bss_wep_1key2", "");
        cdb_set("$wl_bss_wep_2key2", "");
        cdb_set("$wl_bss_wep_3key2", "");
        cdb_set("$wl_bss_wep_4key2", "");
        cdb_set("$wl_passphase2", "");
        return;
    }

    cdb_set("$wl_bss_wep_1key2", pass);
    cdb_set("$wl_bss_wep_2key2", pass);
    cdb_set("$wl_bss_wep_3key2", pass);
    cdb_set("$wl_bss_wep_4key2", pass);
    cdb_set("$wl_passphase2", pass);
    cdb_set("$wl_bss_sec_type2", "4");
}
#endif


int platform_awss_connect_ap(
        _IN_ uint32_t connection_timeout_ms,
        _IN_ char ssid[PLATFORM_MAX_SSID_LEN],
        _IN_ char passwd[PLATFORM_MAX_PASSWD_LEN],
        _IN_OPT_ enum AWSS_AUTH_TYPE auth,
        _IN_OPT_ enum AWSS_ENC_TYPE encry,
        _IN_OPT_ uint8_t bssid[ETH_ALEN],
        _IN_OPT_ uint8_t channel)
{
/*
	char buf[256];
	int ret;

	snprintf(buf, sizeof(buf), "ifconfig %s up", WLAN_IFNAME);
	ret = system(buf);

	snprintf(buf, sizeof(buf), "iwconfig %s mode managed", WLAN_IFNAME);
	ret = system(buf);

	snprintf(buf, sizeof(buf), "wpa_cli -i %s status | grep 'wpa_state=COMPLETED'", WLAN_IFNAME);
	do {
		ret = system(buf);
		usleep(100 * 1000);
	} while (ret);

	snprintf(buf, sizeof(buf), "udhcpc -i %s", WLAN_IFNAME);
	ret = system(buf);
*/

//	is_wifi_scan = 1;
	is_wifi_rev_data = 1;
	mon_srv_stop();

	swapspace(ssid,tempssid);
   	 unsigned char buffer[256]={0};
#if 0
	if(strlen(passwd) != 0){
      //     sprintf(buffer, "elian.sh connect %s %s", tempssid, passwd);
           play_notice_voice(m_get_passwd_start_connect_router);
		sprintf(buffer, "/lib/wdk/omnicfg_apply 3 %s %s", tempssid, passwd);
	}else{
            sprintf(buffer, "/lib/wdk/omnicfg_apply 9 %s", tempssid);
       }
    printf("-----%s------%d---ssid=%s  passwd = %s\n",__FUNCTION__,__LINE__,tempssid,passwd);
	
	exec_cmd(buffer);
//	Timer_PowerOff_Pthread_Create();
	//system(buffer);
	 printf("-----%s------%d---ssid=%s  passwd = %s\n",__FUNCTION__,__LINE__,tempssid,passwd);
	//TODO: wait dhcp ready here
#endif	
#ifdef SPEEDUP_WLAN_CONF
//	exec_cmd("wd smrtcfg stop");	
	wifi_promiscuous_enable(false);
	set_ssid_pwd(ssid, (strlen(passwd) != 0) ? passwd : NULL);
	if(strlen(passwd) != 0){
		play_notice_voice(m_get_passwd_start_connect_router);
		printf("-----%s------%d---ssid=%s  passwd = %s\n",__FUNCTION__,__LINE__,tempssid,passwd);
	}
	cdb_set("$op_work_mode", "3");
	cdb_set_int("$omi_wpa_supp_retry", 2);
	exec_cmd("mtc wlhup");
//	 system("mtc wlhup");

	 char ip_str[PLATFORM_IP_LEN] = {0};
	 int k = 0;
	 int wifi_state;
        while(1){
		wifi_state = cdb_get_int("$wanif_state", 0);
		if (2 == wifi_state)
			break;
		k++;
        }
	printf("k = %d\n",k);
#endif
    return 0;
}

int platform_wifi_scan(platform_wifi_scan_result_cb_t cb)
{
	printf("-----%s------%d---\n",__FUNCTION__,__LINE__);

	return 0;
}

static AES_KEY encks, decks;

uint8_t *rec_iv = NULL;
p_aes128_t platform_aes128_init(
    const uint8_t* key,
    const uint8_t* iv,
    AES_DIR_t dir)
{
	int i = 0;
	
    if (AES_set_decrypt_key(key, 128, &decks) < 0){
		printf("-----%s------%d---AES_set_decrypt_key return 0\n",__FUNCTION__,__LINE__);
	}else{
		printf("------------AES_set_decrypt_key > 0---\n");
	}
	rec_iv = malloc(16);
	memcpy(rec_iv, iv, 16);

	return 0;
}

int platform_aes128_destroy(
    p_aes128_t aes)
{
	free(aes);	 

	return 0;
}

int platform_aes128_cbc_encrypt(
    p_aes128_t aes,
    const void *src,
    size_t blockNum,
    void *dst )
{

	return 0;
}

int platform_aes128_cbc_decrypt(
    p_aes128_t aes,
    const void *src,
    size_t blockNum,
    void *dst )
{
	int i = 0,k = 0;


//	for (k = 0;k < 64;k++)
//		printf(" %x ",(((uint8_t *)src)[k]));

	for (i = 0; i < 8; ++i) {
	   AES_cbc_encrypt(src,dst,strlen(src),&decks,rec_iv,AES_DECRYPT);
	   src = (uint8_t *)src + strlen(src);
	   dst = (uint8_t *)dst + 16;
	  

	}
	return 0;
}

int platform_wifi_get_ap_info(
    char ssid[PLATFORM_MAX_SSID_LEN],
    char passwd[PLATFORM_MAX_PASSWD_LEN],
    uint8_t bssid[ETH_ALEN])
{
	strncpy(ssid, tempssid,strlen(tempssid));	
	printf("-----%s------%d---ssid = %s\n",__FUNCTION__,__LINE__,ssid);

	if (NULL != ssid){
		strcpy(ssid, "NO_WIFI");
	}
	return 0;
}


int platform_wifi_low_power(int timeout_ms)
{
    //wifi_enter_power_saving_mode();
    usleep(timeout_ms);
    //wifi_exit_power_saving_mode();

    return 0;
}

int platform_wifi_enable_mgnt_frame_filter(
            _IN_ uint32_t filter_mask,
            _IN_OPT_ uint8_t vendor_oui[3],
            _IN_ platform_wifi_mgnt_frame_cb_t callback)
{

    return -2;
}

int platform_wifi_send_80211_raw_frame(_IN_ enum platform_awss_frame_type type,
        _IN_ uint8_t *buffer, _IN_ int len)
{

    return -2;
}
