#include "audioRec.h"
#include "../cpp_box.h"
#include "AlsaRecorder.h"
#include <iostream>
#include <fstream>

using namespace duerOSDcsApp::application;

AudioRecorder* main_rec=NULL;
std::ofstream out("/tmp/ttt.pcm");

void INT_handle(int sig){
	trc(__func__);
	if(main_rec){
		uartd_fifo_write("tlkoff");
//		main_rec->recorderClose();		
	}
	exit(0);
}

void recordDataInputCallback(char* buffer, long unsigned int size, void *userdata) {
	out.write(buffer,size);
}

int audioRec_handle(char *argv[]) {
    int ret  = 0;
	clog(Hred,"compile in [%s %s]",__DATE__,__TIME__);
	traceSig(SIGSEGV,SIGBUS,SIGABRT,SIGFPE);
	signal(SIGINT, INT_handle);
    AudioRecorder* _m_rec = new AudioRecorder();
    if (_m_rec != nullptr) {
		out.clear();
		uartd_fifo_write("tlkon");
		main_rec=_m_rec;
        ret = _m_rec->recorderOpen("default", recordDataInputCallback, NULL);
        if (0 != ret) {
            err("Failed to open recorder");
            return -1;
        }
    } else {
            err("Failed to new AudioRecorder");
            return -2;
    }
	do{
		pause();
	}while(1);
	return 0;
}

