#include "pidfile.h"

#include <stdlib.h>
#include <unistd.h>

#include <myutils/pidfile.h>

#include "myutils/debug.h"

#define PID_FILE "/tmp/turingIot.pid"



void PidLock(char *exec)
{
	LockPid(exec, PID_FILE);
}
void PidUnLock()
{
	UnLockPid();
}



