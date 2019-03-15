#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>

#define SEND_TIME_GAP   1
#define DEST_PORT       5354
#define BROADCAST_ADDR 	0xFFFFFFFFul
//#define SUBNET_MASK     0xFFFFFF00ul
#define QUERY_SERV_BUFSIZE  128
#define QUERY_STRING    "{\"omi_query\",\"1\"}"
#define IFNAME0         "br0"
#define IFNAME1         "br1"

#define SIG_PRESS_BUTTON    __SIGRTMIN+10 


int sockfd = -1;
int button_status = 0;//press is 1
struct sockaddr_in dest_address;

void debug_log(const char *format, ...)
{
    char buf[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    syslog(LOG_DEBUG, "%s", buf);
}

#if 1
int create_socket(char* ifname)
{
    int b_opt_val = 1;
    struct ifreq ifr;
    int ret = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd)
    {
        debug_log("fail to create socket");
        ret = -1;
        goto RETURN;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &b_opt_val, sizeof(int)) == -1)
    {
        debug_log("fail to setopt broadcast");
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
            debug_log("setsockopt %s SO_BINDTODEVICE: %m", ifname);
            ret = -1;
            goto CLOSE_FD;
        }
        else
        {
            //debug_log("bind interface %s to socket", ifname);
        }
    }
    else
    {
        debug_log("bind interface %s fail", ifname);
        ret = -1;
        goto CLOSE_FD;
    } 

    memset(&dest_address, 0, sizeof(dest_address));
    dest_address.sin_family      = AF_INET;
    dest_address.sin_addr.s_addr = BROADCAST_ADDR;
    dest_address.sin_port        = htons(DEST_PORT);

    return ret;

CLOSE_FD:
    close(sockfd);
    sockfd = -1;

RETURN:
    return ret;
}

int send_broadcast_packet_to_interface(char* buffer, char* ifname)
{
    ssize_t cnt = 0;
    int ret = 0;

    if (-1 == sockfd)
    {
        if (0 != create_socket(ifname))
        {
            debug_log("create_socket %s fail", ifname);
            ret = -1;
            goto RETURN;
        }
    }

    cnt = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&dest_address, (int)sizeof(dest_address));

    
    if (0>cnt) {
        debug_log("sendto fail");
        ret = -1;
    } else {
        //debug_log("%s broadcast cnt %d, buffer <%d>%s", ifname, cnt, strlen(buffer), buffer); 
    }

    close(sockfd);
    sockfd=-1;

RETURN:
    return ret;
}
#endif
#if 0
int create_socket(char* ip_addr)
{
    int b_opt_val = 1;
    int ret = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd)
    {
        debug_log("fail to create socket");
        ret = -1;
        goto RETURN;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &b_opt_val, sizeof(int)) == -1)
    {
        debug_log("fail to setopt broadcast");
        ret = -1;
        goto CLOSE_FD;
    }

    memset(&dest_address, 0, sizeof(dest_address));
    dest_address.sin_family      = AF_INET;
    dest_address.sin_addr.s_addr = (inet_addr(ip_addr) & SUBNET_MASK) | 0xFFUL;
    dest_address.sin_port        = htons(DEST_PORT);

    goto RETURN;

    CLOSE_FD:
    close(sockfd);
    sockfd = -1;

    RETURN:
    return ret;
}

int send_broadcast_packet_to_ip(char* buffer, char* ip_addr)
{
    ssize_t cnt = 0;
    int ret = 0;

    if (-1 == sockfd)
    {
        if (0!=create_socket(ip_addr))
        {
            debug_log("create_socket fail");
            ret = -1;
            goto RETURN;
        }
    }

    cnt = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&dest_address, (int)sizeof(dest_address));
    //debug_log("cnt %d, buffer <%d>%s",cnt, strlen(buffer), buffer);
    if (0>cnt)
    {
        debug_log("sendto fail");
        ret = -1;
    }

    close(sockfd);
    sockfd=-1;

    RETURN:
    return ret;
}
#endif
int send_broadcast_packet(char* buffer)
{
    int ret=-1;
#if 1 // bind socket by interface name
    if (0==send_broadcast_packet_to_interface(buffer, IFNAME0))
    {
        ret=0;
    }
    if (0==send_broadcast_packet_to_interface(buffer, IFNAME1))
    {
        ret=0;
    }
#endif
#if 0 // bind socket by subnet broadcast ip
    if ((0<strlen(br0_ip))
        &&(0==send_broadcast_packet_to_ip(buffer, br0_ip)))
    {
        ret=0;
    }
    if ((0<strlen(br1_ip))
        &&(0==send_broadcast_packet_to_ip(buffer, br1_ip)))
    {
        ret=0;
    }
#endif
    return ret;
}

static void handle_broadcast_stop(int signo)
{
    debug_log("normal termination");
    exit(0);
}

//int button_status = 0; //press is 1;
  
static void    sigactionprocess(int nsig)  
{  
    //================================================================  
    //TODO:ADD liaoyong CODE  
  
    //printf("son process sigactionprocess end !pid is:%d\n", getpid()) ;  
	button_status = 1;
    //================================================================  
 
}  
//button press signal processing function registration 
void    sigactionreg(void)  
{  
    struct sigaction act,oldact;  
  
    act.sa_handler  = sigactionprocess;  
    act.sa_flags = 0;  
  
    sigaction(SIG_PRESS_BUTTON,&act,&oldact);  
}  

static struct sockaddr_in client_addr;
int omnicfg_send_query_response(char *resp, int length)
{
    int socket_fd = -1;
    struct sockaddr_in myaddr;
    int nbytes;

    //printf("omnicfg_send_query_response\n");

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) <0)
        goto error;

    bzero ((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);

    if(bind(socket_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) <0)
        goto error;

    client_addr.sin_port = htons(DEST_PORT);

    nbytes = sendto(socket_fd, resp, length, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
    if(nbytes!=length)
    {
        //printf("Send error\n");
        goto error;
    }

    close(socket_fd);
    return 0;

error:
    if(socket_fd>=0)
        close(socket_fd);

    return -1;
}

int omnicfg_poll_query_request(void)
{
    static int socket_fd = -1;
    int length;
    int nbytes;
    char buf[QUERY_SERV_BUFSIZE];
    struct sockaddr_in myaddr;
    int ret = 0;
    //struct timeval timeout = {1,0};

    if(0 > socket_fd)
    {
        if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) <0)
            goto error;

        bzero ((char *)&myaddr, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myaddr.sin_port = htons(DEST_PORT);

        if(bind(socket_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) <0)
            goto error;

        #if 0
        if(0 > setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)))
            goto error;
        #endif
    }

    length = sizeof(client_addr);
    nbytes = recvfrom(socket_fd, &buf, QUERY_SERV_BUFSIZE, 0, (struct sockaddr*)&client_addr, (socklen_t *)&length);
    if(0>nbytes)
    {
        goto error;
    #if 0
        if(errno==EAGAIN)
            printf("Timeout\n");
    #endif
    }
    else
    {
        if(!strncmp(QUERY_STRING, buf, strlen(QUERY_STRING)))
        {
            //printf("Received data form %s : %d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
            //printf("%s\n", buf);
            ret = 1;
        }
    }

    return ret;

error:
    if(socket_fd >= 0)
    {
        close(socket_fd);
        socket_fd = -1;
    }
    return -1;
}

void omnicfg_handle_query(int transition_time, char *resp1, char *resp2)
{
    struct timespec start;
    struct timespec current;
    char *resp;
    int ret;

    clock_gettime(CLOCK_MONOTONIC, &start);

    while(1)
    {
        ret = omnicfg_poll_query_request();
        if(ret < 0)
        {
            sleep(1);
            continue;
        }

        if(ret > 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &current);
            //printf(" %d %d\n", start.tv_sec, current.tv_sec);

            if((current.tv_sec - start.tv_sec) > transition_time)
                resp = resp2;
            else
                resp = resp1;

            omnicfg_send_query_response(resp, strlen(resp));
        }
    }
}
int main (int argc, char** argv)
{
    int cnt;
	int unicast_mode = 0;
    openlog("BC_OCFG", LOG_NDELAY, LOG_USER);
    debug_log("omnicfg_broadcast start");

    if(argc==4)
    {

    }
    else if(argc==5)
    {
        if(!strcmp(argv[4],"-u"))
        {
            unicast_mode = 1;
        }
    }
    else
    {
        debug_log("usage: %s <text_after_change> <txt_in_working> <change_period> <-u>",argv[0]);
        return 0;
    }

    signal(SIGHUP, handle_broadcast_stop);
	//debug_log("  handle_broadcast_netwoke_status start");
    //signal(SIGPWR, handle_broadcast_netwoke_status);
	sigactionreg(); 
	
    cnt=atoi(argv[3]);

    if(unicast_mode)
        omnicfg_handle_query(cnt, argv[1], argv[2]);
    while (1)
    {
        if (cnt>0)
        {
            if (0!=send_broadcast_packet(argv[1]))
            {
                debug_log("broadcast string fail");
            }
            cnt--;
        }
        else
        {
            if (0!=send_broadcast_packet(argv[2]))
            {
                debug_log("broadcast string fail");
            }
        }
        sleep(SEND_TIME_GAP);
    }

    return 0;
}
