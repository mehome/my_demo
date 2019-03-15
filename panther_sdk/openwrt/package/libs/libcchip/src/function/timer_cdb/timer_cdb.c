#include "timer_cdb.h"

static timer_t tm_id={0};

static struct sigevent evp={
	.sigev_signo = __SIGRTMIN,
	.sigev_notify = SIGEV_SIGNAL,
};

static void cdb_save_cb(int sig){
	inf_nl("time out stop timer!");
	cdb_save();
	struct itimerspec it = {0};
	px_timer_set(tm_id,0,&it,NULL);
}

int timer_cdb_init(int signo){
	if(signo) evp.sigev_signo=signo;
	signal(signo,&cdb_save_cb);
	int ret=px_timer_create(CLOCK_MONOTONIC,&evp,&tm_id);
	if(ret<0) return -1;
	return 0;
}

int timer_cdb_save(void){
	struct itimerspec it={{6,0},{5,0}};
	return px_timer_set(tm_id,0,&it,NULL);
}