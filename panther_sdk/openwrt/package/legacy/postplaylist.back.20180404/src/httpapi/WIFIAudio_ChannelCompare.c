#include "WIFIAudio_ChannelCompare.h"

#ifndef WACC_MIN
#define WACC_MIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef WACC_MAX
#define WACC_MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

#ifndef WACC_ABS
#define WACC_ABS(a)  ((a) < 0 ? (-(a)) : (a))
#endif


/*
int main(int argc, char *argv[])
{
	float l_pro = 0.0, r_pro = 0.0;
	if(2 != argc){
		printf("usage: %s filename\n", argv[0]);
		exit(0);
	}	
	
	WIFIAudio_ChannelCompare_channel_compare(argv[1], &l_pro, &r_pro);
	printf("%.2f left, %.2f right\n",l_pro, r_pro);
	return 0;

}
*/


int WIFIAudio_ChannelCompare_Open1(const char *pathname, int flags)
{
	int rc;
	
	if((rc = open(pathname, flags)) < 0)
		WIFIAudio_ChannelCompare_unix_error("WIFIAudio_ChannelCompare_Open1 error");
	return rc;	
}

ssize_t WIFIAudio_ChannelCompare_Read(int fd, void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0) 
	WIFIAudio_ChannelCompare_unix_error("WIFIAudio_ChannelCompare_Read error");
    return rc;
}

void WIFIAudio_ChannelCompare_Close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
	WIFIAudio_ChannelCompare_unix_error("WIFIAudio_ChannelCompare_Close error");
}

void WIFIAudio_ChannelCompare_unix_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void  WIFIAudio_ChannelCompare_channel_compare(char *filename, float *l_pro, float *r_pro)
{
	int fd, num, v = 0, sum = 0, l_channel = 0, r_channel = 0;
	WACC_int16 stream[64];
	WACC_int16 rzl;
	
	fd = WIFIAudio_ChannelCompare_Open1(filename, O_RDONLY);
	memset(stream, 0, 64);
	while((num = WIFIAudio_ChannelCompare_Read(fd, stream, 64)) > 0){
		v = WIFIAudio_ChannelCompare_app_detect_sin_wav_running(stream, num, 0, &rzl);
		sum++;
		if(1 == v)
			l_channel++;
		else if( -1 == v)
			r_channel++;
//		printf("%2d", v);
	} 	
	*l_pro = (float)l_channel /(float)sum;
	*r_pro = (float)r_channel / (float)sum;
//	printf("\n");
//	printf("left = %d; right = %d sum = %d\n", l_channel, r_channel, sum);
	WIFIAudio_ChannelCompare_Close(fd);
}

//cnt_thd为检测周期.如果要检测全文件,cnt_thd设为0
int WIFIAudio_ChannelCompare_app_detect_sin_wav_running(WACC_int16 *ptr,WACC_uint16 len,WACC_uint16 cnt_thd,WACC_int16 *rzl)
{
	int i,j;
	WACC_uint32 maxABS[2]={0};
	int ret=0;
	for(i=0,j=0;i<len;i+=2,j++)
	{
		maxABS[0] = WACC_MAX(abs(ptr[i+0]),maxABS[0]);
		maxABS[1] = WACC_MAX(abs(ptr[i+1]),maxABS[1]);
		if(cnt_thd&&(j >= cnt_thd))
		{
			if((maxABS[0]*10)>(maxABS[1]*14)){
				ret = 1;//Lch larger
			}else if((maxABS[1]*10)>(maxABS[0]*14)){
				ret = -1;//Rch larger
			}
			maxABS[0] = 0;	
			maxABS[1] = 0;	
			j = 0;
		}
	}
	
	if(cnt_thd==0)
	{
		if((maxABS[0]*10)>(maxABS[1]*14)){
			ret = 1;//Lch larger
		}else if((maxABS[1]*10)>(maxABS[0]*14)){
			ret = -1;//Rch larger
		}
	}

	*rzl = ret;
	return ret;
}
