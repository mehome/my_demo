#include <stdio.h>
#include <wdk/cdb.h>
#include <function/common/fd_op.h>

#define WIFIAUDIO_NORMALINSYSTEM_PATHNAME "/www/misc/recoder_normal.wav"
//普通录音文件在系统上的路径
#define WIFIAUDIO_NORMALINSERVER_PATHNAME "/misc/recoder_normal.wav"
//普通录音文件在web server的路径

#define WIFIAUDIO_MICINSYSTEM_PATHNAME "/www/misc/recoder_mic.wav"
//麦克录音文件在系统上的路径
#define WIFIAUDIO_MICINSERVER_PATHNAME "/misc/recoder_mic.wav"
//麦克录音文件在web server的路径

#define WIFIAUDIO_REFINSYSTEM_PATHNAME "/www/misc/recoder_ref.wav"
//参考信号录音文件在系统上的路径
#define WIFIAUDIO_REFINSERVER_PATHNAME "/misc/recoder_ref.wav"
//参考信号录音文件在web server的路径

#if 1
#define WAKEUPPRG "AlexaRequest"
#else
#define WAKEUPPRG "wakeup"
#endif

//正常模式录音
int dot_normal_record(int value){
	int ret = 0;
	char str[256] = {0};
	sprintf(str, "(cxdish set-mode ZSH2;killall "WAKEUPPRG";arecord -D plughw:1,0 -f s16_le -c 2 -d %d -r 16000 %s) &", \
	value,WIFIAUDIO_NORMALINSYSTEM_PATHNAME);
	ret = system(str);
	return ret;		
}

//获取正常模式录音文件URL 
char* dot_normal_record_web_path(){
	return WIFIAUDIO_NORMALINSERVER_PATHNAME;		
}

//麦克录音 
int dot_mic_record(int value){
	int ret = 0;
	char str[256] = {0};
	sprintf(str, "(cxdish set-mode ZSH3;killall "WAKEUPPRG";arecord -D plughw:1,0 -f s16_le -c 2 -d %d -r 16000 %s;cxdish set-mode ZSH2) &", \
	value,WIFIAUDIO_MICINSYSTEM_PATHNAME);
	ret = system(str);
	return ret;		
}

//获取麦克录音文件URL
char* dot_mic_record_web_path(){
	return WIFIAUDIO_MICINSERVER_PATHNAME;		
}

//参考信号录音
int dot_ref_record(int value){
	int ret = 0;
	char str[256] = {0};
	sprintf(str, "(cxdish set-mode ZSH4;killall "WAKEUPPRG";arecord -D plughw:1,0 -f s16_le -c 2 -d %d -r 16000 %s;cxdish set-mode ZSH2) &", \
	value,WIFIAUDIO_REFINSYSTEM_PATHNAME);
	ret = system(str);
	return ret;		
}

//获取参考信号录音文件URL 
char* dot_ref_record_web_path(){
	return WIFIAUDIO_REFINSERVER_PATHNAME;		
}

//停止模式录音
int dot_stop_record(){
	int ret = 0;
	ret = system("killall arecord");
	return ret;		
}
