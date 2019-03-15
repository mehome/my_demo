#ifndef DUEROSFIFOCMDTRIGGER_H_
#define DUEROSFIFOCMDTRIGGER_H_ 1
extern "C" {
#include <libcchip/platform.h>
#include <libcchip/function/fifo_cmd/fifo_cmd.h>
}

namespace duerOSDcsApp {
namespace application {

int cfgnet_handle(void *args);

class DuerosFifoCmd{	
	private:
	static fifo_cmd_t fifo_cmd_tbl[];
	static cmd_args_t fifo_cmd_args;
	static void* pDcsSdkIst;
	public:
	DuerosFifoCmd(void *);
	static int setCmdItemArgs(char *cmd,void *args){
		int i=0;
		for(i=0;i<fifo_cmd_args.count;i++){
			if(0==strcmp(fifo_cmd_tbl[i].cmd,"wakeup")){
				fifo_cmd_tbl[i].args=args;
				return 0;
			}
		}
		err("cmd:%s not fund !",cmd);
		return -1;	
	}
};

}
}

#endif
