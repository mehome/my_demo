#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <myutils/pidfile.h>

#define PID_FILE "/tmp/airkissDis.pid"

void PidLock(char *exec)
{
	LockPid(exec,PID_FILE);
}
void PidUnLock()
{
	UnLockPid();
}


