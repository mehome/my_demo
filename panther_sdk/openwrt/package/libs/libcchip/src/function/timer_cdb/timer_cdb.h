#ifndef TIMER_CDB_H_
#define TIMER_CDB_H_ 1
#include <wdk/cdb.h>
#include "../common/px_timer.h"
int timer_cdb_save(void);
int timer_cdb_init(int signo);

#endif
