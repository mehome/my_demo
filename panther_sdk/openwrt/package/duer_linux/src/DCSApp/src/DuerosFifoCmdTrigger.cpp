#include <DCSApp/DeviceIoWrapper.h>
#include <DeviceIo/DeviceIo.h>
#include <DCSApp/DuerosFifoCmdTrigger.h>
extern "C"{
	#include <wakeup/interface.h>
	#include <libcchip/function/user_timer/user_timer.h>
}
namespace duerOSDcsApp {
namespace application {
using namespace duerOSDcsApp::framework; 

int cfgnet_handle(void *args){
	item_arg_t *arg=(item_arg_t *)args;
//	char **argv=arg->argv;
	DeviceIoWrapper::getInstance()->callback(DeviceInput::KEY_ONE_LONG,NULL,0);
	return 0;
}
int logctrl_handle(void *args){
	item_arg_t *arg=(item_arg_t *)args;
	char **argv=(char **)arg->argv;
	do{
		assert_arg(1,-1);
		log_init(arg->argv[1]);
	}while(0);
	return 0;
}
void* DuerosFifoCmd::pDcsSdkIst=NULL;
int wakeup_handle(void *args){
	item_arg_t *arg=(item_arg_t *)args;
//	char **argv=arg->argv;
#if 1
	PortAudioMicrophoneWrapper *micWrapperPtr=(PortAudioMicrophoneWrapper *)(arg->args);
	wakeupStartArgs_t *pStartArgs=&micWrapperPtr->wakeupStartArgs;
	pStartArgs->isWakeUpSucceed=1;
	setListenStatus(1);
	struct itimerspec it={{13,0},{12,0}};
	pStartArgs->userTimerPtr->trigger(pStartArgs->userTimerPtr, &it);
#else
	DeviceIoWrapper::getInstance()->callback(DeviceInput::KEY_WAKE_UP,NULL,0);
#endif
	return 0;
}
int keyevt_handle(void *args){
	item_arg_t *arg=(item_arg_t *)args;
	char **argv=arg->argv;
	assert_arg(1,-1);
	DeviceInput key_value=(DeviceInput)atoi(argv[1]);
	if(key_value <= DeviceInput::DEVICEINPUT_MIN || key_value >= DeviceInput::DEVICEINPUT_MAX){
		return -2;
	}
	int value=argv[2]?atoi(argv[2]):0;
	int len=argv[3]?atoi(argv[3]):0;
	DeviceIoWrapper::getInstance()->callback(key_value,&value,len);
	return 0;
}

fifo_cmd_t DuerosFifoCmd::fifo_cmd_tbl[]={
	ADD_HANDLE_ITEM(cfgnet,NULL)
	ADD_HANDLE_ITEM(logctrl,NULL)
	ADD_HANDLE_ITEM(wakeup,NULL)
	ADD_HANDLE_ITEM(keyevt,NULL)	
};

cmd_args_t DuerosFifoCmd::fifo_cmd_args={
	"/tmp/duer.cmd",
	&fifo_cmd_tbl,
};

DuerosFifoCmd::DuerosFifoCmd(    void* args){	
	trc(__func__);
	fifo_cmd_args.count=getCount(fifo_cmd_tbl);	
	setCmdItemArgs("wakeup",args);
	int err=fifo_cmd_thread_create(&fifo_cmd_args);		
	if(err<0){
		err("%s failure,err=%d",__func__,err);
	}
}

}
}

