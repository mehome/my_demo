#include <stdio.h>
#include <wdk/cdb.h>
#include <function/common/fd_op.h>


//搜索周边蓝牙设备
int bt_search(){
	int ret = 0;
	ret = system("uartdfifo.sh btsearch");
	return ret;		
}

//获取蓝牙设备信息  
int bt_list(char **bt_list){
	int ret = 0;
	ret=fd_read_file(bt_list,"/tmp/BT_INFO");
	return ret;		
}

//连接蓝牙设备 
int bt_connect(char *bt_addr){
	int ret = 0;
	char str[64] = {0};
	sprintf(str, "uartdfifo.sh in %s", bt_addr);
	ret = system(str);
	return ret;		
}

//断开蓝牙设备 
int bt_disconnect(){
	int ret = 0;
	ret = system("uartdfifo.sh btdiscon");
	return ret;		
}
