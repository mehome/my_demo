#include "testmode.h"
#include <wdk/cdb.h>
#include <stdio.h>
#include <libcchip.h>
#include <mon_config.h>

#define TEST_MODE_ON		"testmodeon"	// 通知蓝牙进入测试模式
#define TEST_MODE_OFF		"testmodeoff"	// 通知蓝牙退出测试模式


int set_test_mode(int iMode) {
	//cdb_set_int("$test_mode", iMode);
	//cdb_save();
	if (1 == iMode) {
		FILE *fp = fopen("/tmp/test_mode", "w");
#if defined(CONFIG_PACKAGE_duer_linux)
		system("cdb set duer_nocheck 1;killall duer_linux");
#elif defined (CONFIG_PROJECT_K2_V1)
		system("cdb set iot_nocheck 1 > /dev/null;killall -9 turingIot > /dev/null");
#endif
		fclose(fp);
#if defined(CONFIG_PACKAGE_duer_linux)
		uartd_fifo_write(TEST_MODE_ON);		//打开测试模式
#endif
	} else if(access("/tmp/test_mode", 0) == 0){
		remove("/tmp/test_mode");
#if defined(CONFIG_PACKAGE_duer_linux)
		uartd_fifo_write(TEST_MODE_OFF);
#endif
	}

	return 0;
}


int get_test_mode() {
	int iRet = 0;

	if(!access("/tmp/test_mode",0)) {
		iRet = 1;
	}

	return iRet;
}

int play_test_music(char *pURL) {
	char byBuffer[1024] = {0};
	int ret = 0;
	sprintf(byBuffer, "myPlay %s &", pURL);
	ret = system(byBuffer);

	if (ret != 0){
		ret = -1;
	}

	return ret;
}

int Sd_Is_Exist(void)
{
	int ret = 0;
	if(cdb_get_int("$tf_status", 0) == 0){
		ret = -1;
	}else if(cdb_get_int("$tf_status", 0) == 1){
		ret =  0;
	}
		return ret;
}

int Usb_Is_Exist(void)
{
	int ret = 0;
#if 0
	if(cdb_get_int("$usb_status", 0) == 0){
		ret = -1;
	}else if(cdb_get_int("$usb_status", 0) == 1){
		ret =  0;
	}
#else
	ret = 0;
#endif
		return ret;
}


int stop_test_music() {
	int ret = 0;
	ret = system("killall myPlay");

	if (ret != 0){
		ret = -1;
	}

	return ret;
}


