#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>

#include "myutils/debug.h"
#include "common.h"
#include "init.h"
#include "thread.h"

#include "libcchip/function/back_trace/back_trace.h"


int main(int argc , char **argv)
{
	micarray_mode_check(argc,argv);

    traceSig(SIGSEGV);
    init(argc, argv);

    thread_init();
    exec_cmd("mpc update");
    thread_deinit();
    deinit();
    return 0;
}




