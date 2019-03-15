#ifndef __MYUTILS_PID_FILE_H__

#define __MYUTILS_PID_FILE_H__


extern void LockPid(char *exec, char *file);
extern void UnLockPid();
extern int lock_pid(char *pidfile);

#endif

