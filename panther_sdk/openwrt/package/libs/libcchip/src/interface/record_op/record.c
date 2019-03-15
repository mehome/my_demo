#include <stdio.h>
#include <function/wavrecorder/WIFIAudio_Semaphore.h>
#include  <function/wavrecorder/WIFIAudio_ChannelCompare.h>
#include "record.h"

//3.10.1. 设置录音时间				
int set_record_time(int value)
{
	int ret = 0;
	char filename[128];
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	ret = my_popen("uartdfifo.sh testtlkon &");
	if(ret == -1){
		printf("uartdfifo.sh error\n");
	}
	ret = WavRecorder(filename, value);
	if(ret == -1){
		printf("WavRecorder error\n");
	}
	ret = my_popen("uartdfifo.sh testtlkoff &");
	if(ret == -1){
		printf("uartdfifo.sh error\n");
	}
	
	return ret;
}

//3.10.2. 播放录音文件				
int play_record_audio(void)
{
	int ret = 0;	
	char filename[128];
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	
	ret = my_popen("aplay %s &", filename);
	
	return ret;
}

//3.10.3. 临时播放录音				
int play_url_audio(const char *url)
{
	int ret = 0;
	
	if(NULL == url)
	{
		printf("Error of parameter : NULL == precurl in WIFIAudio_DownloadBurn_PlayUrlRecord\r\n");
		ret = -1;
	}
	else
	{
		my_popen("mpg123 %s &", url);
	}
	
	return ret;
}

//3.10.4. 开始录音
int start_record(void)
{
	int ret = 0;	
	char filename[128];
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	ret = my_popen("uartdfifo.sh testtlkon &");
	ret = WavRecorder(filename, 20);
	ret = my_popen("uartdfifo.sh testtlkoff &");
	
	return ret;
}

//3.10.5. 停止录音
int stop_record(void)
{
	int ret = 0;
	char filename[128];
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	SetState_StopRecord(1);
	my_popen("uartdfifo.sh testtlkoff &");
	ret = my_popen("aplay %s &", filename);
	
	return ret;
}

//3.10.6. 定长录音
int start_fixed_record(void)
{
	int ret = 0;
	char filename[128];
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	
	ret=my_popen("uartdfifo.sh testtlkon &");
	if(ret == -1){
		printf("uartdfifo.sh error\n");
	}

	ret = WavRecorder(filename, 0);
	if(ret == -1){
		printf("wavrecord error\n");
	}
	ret=my_popen("uartdfifo.sh testtlkoff &");
	if(ret == -1){
		printf("uartdfifo.sh error\n");
	}
	ret = my_popen("aplay %s &", filename);
	if(ret == -1){
		printf("aplay error\n");
	}
	
	return ret;
}

//3.10.7. 获取录音文件 URL
const char *get_record_file_URL(void)
{
	FILE *fp = NULL;
	char filename[128];
	char tmp[256];
	char *url = NULL;
	char ip[32] = {0};
	
	memset(filename, 0, sizeof(filename));
	sprintf(filename, "%s%s%s%s", \
	WIFIAUDIO_RECORDINSERVER_PATH1, \
	WIFIAUDIO_RECORDINSERVER_PATH2, \
	WIFIAUDIO_RECORDINSERVER_NAME, \
	WIFIAUDIO_RECORDINSERVER_SUFFIX);
	
	//这边先只读打开music下录音文件试一下，免得不存在还返回成功就jiānjiè了
	if(NULL == (fp = fopen(filename, "r")))
	{
		printf("file not exist\n");
		return NULL;
	}
	else
	{
		fclose(fp);
		fp = NULL;
		
		WIFIAudio_SystemCommand_GetLocalWifiIp(ip);
		if(!strcmp(ip, "0.0.0.0"))
		{
			return NULL;
		}
		else
		{
			url=(char *)calloc(1,256);
			if(!url){
				printf("calloc error\n");
				return NULL;
			}
			sprintf(url, "http://%s/%s%s%s", ip, \
			WIFIAUDIO_RECORDINSERVER_PATH2, \
			WIFIAUDIO_RECORDINSERVER_NAME, \
			WIFIAUDIO_RECORDINSERVER_SUFFIX);
		}
	}
	
	return url;
}

//3.10.8. 语音活动检测					
int voice_activity_detection(int value, char *pcardname)
{
	int ret = 0;
	
	if(NULL == pcardname)
	{
		printf("Error of parameter : NULL == pcardname in WIFIAudio_DownloadBurn_VoiceActivityDetection\r\n");
		ret = -1;
	}
	else
	{
		vadthreshold(value,pcardname);
	}
	
	return ret;
}

//3.10.9. 左右声道录音对比
void voice_channel_compare(char *filename, float *l_pro, float *r_pro)
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
	} 	
	*l_pro = (float)l_channel /(float)sum;
	*r_pro = (float)r_channel / (float)sum;
	WIFIAudio_ChannelCompare_Close(fd);
}

