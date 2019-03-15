#ifndef MICARRAY_CFG_H_
#define MICARRAY_CFG_H_ 1

typedef struct micarray_Info{
	int mics;
	int mics_channel;
	int refs;
	int refs_channel;
	char* record_dev;
}micarray_Info;

int get_micarray_info(micarray_Info **pMicarray_info);

//开始mic阵录音
int micarray_record(int value);

//获取mic阵降噪录音文件路径
char* micarray_normal_record_path();

//获取mic阵麦克录音文件路径
char* micarray_mic_record_path();

//获取mic阵参考信号录音文件路径 
char* micarray_ref_record_path();

//停止mic阵录音
int micarray_stop_record();

#endif
