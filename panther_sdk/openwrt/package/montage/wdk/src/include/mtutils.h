#ifndef __MTUTILS_H
#define __MTUTILS_H

#include <sys/syscall.h>
#include <unistd.h>
#include "mtutils.h"

#define __NR_Linux	4000

#define __NR_print	(__NR_Linux + 360)
#define __NR_panic	(__NR_Linux + 361)

#define LOGLEVEL_EMERG		0	/* system is unusable */
#define LOGLEVEL_ALERT		1	/* action must be taken immediately */
#define LOGLEVEL_CRIT		2	/* critical conditions */
#define LOGLEVEL_ERR		3	/* error conditions */
#define LOGLEVEL_WARNING	4	/* warning conditions */
#define LOGLEVEL_NOTICE 	5	/* normal but significant condition */
#define LOGLEVEL_INFO		6	/* informational */
#define LOGLEVEL_DEBUG		7	/* debug-level messages */

static inline void mt_printk(int level, char *buf, int len)
{
	syscall(__NR_print, level, buf, len);
}

static inline void mt_panic(char *buf, int len)
{
	syscall(__NR_panic, buf, len);
}
#endif /* __MTUTILS_H */
