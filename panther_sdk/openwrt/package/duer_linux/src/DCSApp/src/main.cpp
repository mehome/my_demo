/*
 * Copyright (c) 2017 Baidu, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <signal.h>
#include <cstdlib>
#include <string>
//#include <execinfo.h>
#include "LoggerUtils/DcsSdkLogger.h"
#include "DCSApp/DCSApplication.h"
#include "DCSApp/DeviceIoWrapper.h"
#include "EpollImplement.h"
#include <DeviceTools/SingleApplication.h>
#include <DeviceTools/PrintTickCount.h>
extern "C" {
//	#include <libcchip/platform.h>
	#include <libcchip/function/back_trace/back_trace.h>	
}


#ifndef __SAMPLEAPP_VERSION__
#define __SAMPLEAPP_VERSION__ "Unknown SampleApp Version"
#endif

#ifndef __DCSSDK_VERSION__
#define __DCSSDK_VERSION__ "Unknown DcsSdk Version"
#endif

#ifndef __DUER_LINK_VERSION__
#define __DUER_LINK_VERSION__ "Unknown DuerLink Version"
#endif

 
int main(int argc, char** argv) {
	clog(Hred,"compile in [%s %s]",__DATE__,__TIME__);
	log_init(argv[1]);
	//栈回溯追踪信号
	traceSig(SIGSEGV,SIGBUS,SIGABRT,SIGFPE,SIGTRAP);
	if(!strcmp(get_last_name(argv[0]),"micarray")){
		clog(Hred,"starting %s ...",argv[0]);
		wakeupStartArgs_t wakeupStartArgs={0};
		sensory_init();
		wakeupStartArgs.wakeup_mode=1;
		struct sched_param param;
		pthread_attr_t attr; 
		pthread_attr_init(&attr);
		param.sched_priority = 60;
		pthread_attr_setschedpolicy (&attr, SCHED_RR);
		pthread_attr_setschedparam(&attr, &param);
		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		wakeupStartArgs.pAttr=&attr;
	    int err  = wakeupStart(&wakeupStartArgs);
		if(err<0){
			err("wakeupStart failure,err=%u",err);
			return -2;
		}
		inf("recorder thread is OK!");
		if(argv[1]){
			sleep(atoi(argv[1]));
		}else{
			sleep(20);
		}
		exit(0);
		return 0;
	}
	clog(Hred,"starting %s ...",argv[0]);	
#if 1//修改当前工作目录
	#define WORKING_DIRECTORY "/root"
	int err=chdir(WORKING_DIRECTORY);
	if(err){
		show_errno(0,"chdir");
		return -1;
	}
	clog(Hred,"current working directory is "WORKING_DIRECTORY" !");
#endif
#ifdef MTK8516
    if (geteuid() != 0) {
        APP_ERROR("This program must run as root, such as \"sudo ./duer_linux\"");
        return 1;
    }
#endif	
    if (deviceCommonLib::deviceTools::SingleApplication::is_running()) {
        APP_ERROR("duer_linux is already running");
        return 1;
    }	
	sensory_init();
    deviceCommonLib::deviceTools::printTickCount("duer_linux main begin");
    /// print current version
    APP_INFO("SampleApp Version: [%s]", __SAMPLEAPP_VERSION__);
    APP_INFO("DcsSdk Version: [%s]", __DCSSDK_VERSION__);
    APP_INFO("DuerLink Version: [%s]", __DUER_LINK_VERSION__);

    auto dcsApplication = duerOSDcsApp::application::DCSApplication::create();
    if (!dcsApplication) {
        APP_ERROR("Failed to create to SampleApplication!");
        duerOSDcsApp::application::SoundController::getInstance()->release();
        duerOSDcsApp::application::DeviceIoWrapper::getInstance()->release();
        return EXIT_FAILURE;
    }

    // This will run until application quit.
    dcsApplication->run();

    return EXIT_SUCCESS;
}
