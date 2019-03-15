#include <cpp_box.h>
#include "wakeup.h"
#include <audioRec/audioRec.h>
#include "audioRec/AlsaRecorder.h"
#include "snowboy-detect.h"
#include <unistd.h>
#include <cpp_box.h>

#define RESOURE_FILE_NAME   "./ttt/universal_detect.res"
#define RESOURE_MODEL_NAME  "./ttt/hotword.umdl"
#define SENSITIVITY         "0.48"

using namespace duerOSDcsApp::application;

enum class pipeFd{
	READ,
	WRITE,
	MAX
};

int recDatePipe[(unsigned)pipeFd::MAX]={0};

void writeRecDateCallback(char* buffer, unsigned long size, void *userdata) {
//	inf("recDatePipe[pipeFd::WRITE]=%d",recDatePipe[(int)pipeFd::WRITE]);
	int ret=un_write(recDatePipe[(int)pipeFd::WRITE],buffer,size);
	if(ret != size){
		err("exit process!");
		exit(-1) ;
	}
}

int wakeup_handle(char *argv[]){
	clog(Hred,"compile in [%s %s]",__DATE__,__TIME__);
	traceSig(SIGSEGV,SIGBUS,SIGFPE,SIGABRT);
	int err=0;

#if 1 //修改当前工作目录
	#define WORKING_DIRECTORY "/mnt"
		err=chdir(WORKING_DIRECTORY);
		if(err){
			show_errno(0,"chdir");
			return -2;
		}
		clog(Hred,"current working directory is "WORKING_DIRECTORY" !");
#endif

	err=pipe(recDatePipe);
	if(err){
		show_errno(0,"pipe");
		return -3;
	}
	inf("recDatePipe:[%d][%d]",recDatePipe[0],recDatePipe[1])	;

    AudioRecorder* _m_rec = new AudioRecorder();
    if (_m_rec == nullptr) {
		err("Failed to new AudioRecorder");
		return -4;
	}
	uartd_fifo_write("tlkon");
    int ret = _m_rec->recorderOpen("default", writeRecDateCallback, NULL);
    if (0 != ret) {
        err("Failed to open recorder");
        return -5;
    }

	short g_buffer[1600];
	bool is_end = DUER_FALSE;
	SnowboyInit (RESOURE_FILE_NAME, RESOURE_MODEL_NAME, DUER_TRUE);
	SnowboySetSensitivity (SENSITIVITY);
	do{
//		inf("wait for wakeup!");
		size_t count=un_read(recDatePipe[(int)pipeFd::READ],g_buffer,sizeof(g_buffer));
		if(count <0 ){
			show_errno(0,"read");
			return -6;
		}
		int hotword = SnowboyRunDetection(g_buffer, count/2, is_end);
		if (hotword > 0) {
			inf("YES, hotword %d detected.\n", hotword);
			uartd_fifo_write("wakeup");
			SnowboyReset();
		}		
	}while(1);
	err("reached EOF!");
	return 0;
}
