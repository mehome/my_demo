#ifndef __WIFIAUDIO_CHANNELCOMPARE_H__
#define __WIFIAUDIO_CHANNELCOMPARE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned short WACC_int16;
typedef unsigned short WACC_uint16;
typedef unsigned int WACC_uint32;


int WIFIAudio_ChannelCompare_app_detect_sin_wav_running(WACC_int16 *ptr,WACC_uint16 len,WACC_uint16 cnt_thd,WACC_int16 *rzl);

void  WIFIAudio_ChannelCompare_channel_compare(char *filename, float *l_pro, float *r_pro);

int WIFIAudio_ChannelCompare_Open1(const char *pathname, int flags);

ssize_t WIFIAudio_ChannelCompare_Read(int fd, void *buf, size_t count);

void WIFIAudio_ChannelCompare_Close(int fd);

void WIFIAudio_ChannelCompare_unix_error(char *msg);



#ifdef __cplusplus
}
#endif

#endif

