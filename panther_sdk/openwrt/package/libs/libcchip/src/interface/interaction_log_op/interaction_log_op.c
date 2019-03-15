#include "interaction_log_op.h"
/******************************************
 ** 2.14.1. 						
 ** 函数名: interaction_log_capture_enable
 ** 功  能: 云端交互日志捕捉使能
 ** 参  数: 无
 ** 返回值: 成功返回日志文件路径，不存在失败的情况
******************************************/ 
char* interaction_log_capture_enable(){
	if((access(INTERACTION_LOG_SAVE_PATH,F_OK))){
		plog(INTERACTION_LOG_SAVE_PATH,"interaction log has been enabled !");
	}
	return INTERACTION_LOG_SAVE_PATH;
}

/******************************************
 ** 2.14.2. 						
 ** 函数名: interaction_log_capture_disable
 ** 功  能: 云端交互日志捕捉禁止
 ** 参  数: 无
 ** 返回值: 成功返回0，失败返回-1
******************************************/ 
int interaction_log_capture_disable(){
	int ret=unlink(INTERACTION_LOG_SAVE_PATH);
	if(ret<0){
		return -1;
	}
	return 0;
}