#include "fifo_cmd.h"

#define BUF_SIZE FIFO_BUF_SIZE
static char cmd_buf[BUF_SIZE]="";

static int fifo_cmd_fd=0;

static int cmd_init(char *path){
	unlink(path);
	fifo_cmd_fd=un_fifo_init(path,"+");
	if(fifo_cmd_fd<0)return -1;
	return 0;
}

static int cmd_wait(void){
	memset(cmd_buf,0,sizeof(cmd_buf));
	size_t ret_size=un_fifo_read(fifo_cmd_fd,cmd_buf,sizeof(cmd_buf));
	if(ret_size<1) return -1;	
	return 0;
}

static int cmd_proc(fifo_cmd_t *tbl,size_t count){
	int ret=-2;
	#define CMD_DELIM FIFO_CMD_DELIM
	char **argv=argl_to_argv(cmd_buf,CMD_DELIM);	
	if(!argv[0])return -1;
	size_t i=0;
	for(i=0;i<count;i++){
		ret=strncmp(tbl[i].cmd,argv[0],sizeof(tbl[i].cmd));
		if(!ret){
			item_arg_t item_arg={0};	
			item_arg.argv=argv;
			item_arg.args=tbl[i].args;
			ret=tbl[i].handle(&item_arg);
			break;
		}
		ret=-2;
	}
	if(0==ret)		{inf("%s excute succeed!",argv[0]);}
	else if(-1==ret){err("%s excute failure!",argv[0]);}
	else if(-2==ret){err("%s not found!"	 ,argv[0]);}
	FREE(argv);
	return ret;
}

void fifo_cmd_proc(char* path,void *tbl,size_t count){
	int ret=cmd_init(path);
	if(ret<0)exit(-1);
	showProcessThreadId("");
	do{
		cmd_wait();
		inf("%05d/%016ld get cmd:[%s]",
					getpid(),pthread_self(),cmd_buf);
		cmd_proc(tbl,count);
	}while(1);
}

static pthread_t pid={0};
void *fifo_cmd_thread_routine(void *pCmdArgs){
	cmd_args_t *arg=(cmd_args_t *)pCmdArgs;
	fifo_cmd_proc(arg->path,arg->tbl,arg->count);
}
int fifo_cmd_thread_create(cmd_args_t *pCmdArgs){
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,PTHREAD_STACK_MIN*20);	
	int ret=pthread_create(&pid,&attr,&fifo_cmd_thread_routine,pCmdArgs);
	if(ret){
		show_errno(ret,"pthread_create");
		return -1;
	}
	return 0;
}