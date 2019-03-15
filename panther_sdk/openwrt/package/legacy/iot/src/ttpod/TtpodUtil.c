#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h> 


static int is_uri_code(char ch)
{		
	char *uri="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.!~*'()=&";
	int i ,len;		
	len = strlen(uri);	
	//DEBUG_INFO("len=%d",len);		
	for ( i=0; i<len; i++ ) {	
		if(uri[i] == ch)				
			return 1;		
	}	
	return 0;	
}
int uri_encode(char *dst, char *str )
{    
	char * src = str;    
	char hex[] = "0123456789ABCDEF"; 	
	size_t i;	
	size_t len;
	len = strlen(str);  
	for ( i = 0; i < len; ++i)    {
		unsigned char cc = src[i];        
   
		if (is_uri_code(cc))        { 
			*dst += cc;             
			dst++;       
		} else if(cc == ' ') {
			continue;
		} else {
			unsigned char c = (unsigned char)(src[i]);
			*dst += '%';           
			dst++;            
			*dst = hex[c / 16];   
			dst++;            
			*dst += hex[c % 16];   
			dst++;        
		}    
	}   
	return 0;
}


unsigned long getTimeStamp() 
{
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}



void genRandomString(char *string, int length)    
{    
    int flag, i;     
    srand((unsigned) time(NULL ));    
    
    for (i = 0; i < length; i++)    
    {    
        flag = rand() % 3;    
        switch (flag)    
        {    
            case 0:    
                string[i] = 'A' + rand() % 26;    
                break;    
            case 1:    
                string[i] = 'a' + rand() % 26;    
                break;    
            case 2:    
                string[i] = '0' + rand() % 10;    
                break;    
            default:    
                string[i] = 'x';    
                break;    
        }    
    }    
    string[length] = 0;     
}   



#if 0
void sysUsecTime(char * usec,int len)
{
    struct timeval    tv;
    struct timezone tz;
    
    struct tm         *p;
    
    gettimeofday(&tv, &tz);
    printf("tv_sec:%ld\n",tv.tv_sec);
    printf("tv_usec:%ld\n",tv.tv_usec);
    printf("tz_minuteswest:%d\n",tz.tz_minuteswest);
    printf("tz_dsttime:%d\n",tz.tz_dsttime);
    
    p = localtime(&tv.tv_sec);
   
    snprintf(usec,len,"%d%d%d%d%d%d%ld", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec);

}


char *rand_str(char *str,const int len)
{
  	int i, nu;

    char *buffer;
    int RAND_MAX =2147483647;
    buffer[len] = '\0' ;
    bzero(buffer, len);
    
    for (i= 0; i< len; i++)
        buffer[i] = 'a' + ( 0+ (int)(26.0 *rand()/(RAND_MAX + 1.0)));
 	DEBUG_INFO("The randm String is [ %s ]\n", buffer);

    return buffer;
}
#endif



int get_mac(char * mac, int len_limit)    
{
    struct ifreq ifreq;
    int sock;

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, "eth0");    //Currently, only get eth0

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        return -1;
    }
    
    return snprintf (mac, len_limit, "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned char) ifreq.ifr_hwaddr.sa_data[0],
     (unsigned char) ifreq.ifr_hwaddr.sa_data[1], (unsigned char) ifreq.ifr_hwaddr.sa_data[2], 
     (unsigned char) ifreq.ifr_hwaddr.sa_data[3], (unsigned char) ifreq.ifr_hwaddr.sa_data[4],
     (unsigned char) ifreq.ifr_hwaddr.sa_data[5]);
}






