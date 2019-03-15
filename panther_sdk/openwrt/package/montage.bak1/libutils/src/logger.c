/*
 * =====================================================================================
 *
 *       Filename:  logger.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/16/2016 10:36:05 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "shutils.h"
#include "logger.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/

static int loglevel = LOG_WARNING;
static char logprefix[12] = {0};

static const char *syslog_level_msg[] = {
	[LOG_EMERG]     = "EMERGENCY!",
	[LOG_ALERT]     = "ALERT!",
	[LOG_CRIT]      = "Critical!",
	[LOG_ERR]       = "Error",
	[LOG_WARNING]   = "Warning",
	[LOG_NOTICE]    = "Notice",
	[LOG_INFO]      = "Info",
	[LOG_DEBUG]     = "Debug",
	[LOG_DEBUG + 1] = NULL
};

static const char *syslog_level[] = {
	[LOG_EMERG]     = "LOG_EMERG",
	[LOG_ALERT]     = "LOG_ALERT",
	[LOG_CRIT]      = "LOG_CRIT",
	[LOG_ERR]       = "LOG_ERR",
	[LOG_WARNING]   = "LOG_WARNING",
	[LOG_NOTICE]    = "LOG_NOTICE",
	[LOG_INFO]      = "LOG_INFO",
	[LOG_DEBUG]     = "LOG_DEBUG",
	[LOG_DEBUG + 1]     = NULL
};

int logtolevel (const char *priority)
{
	int i = 0;

	while (syslog_level[i]) {
		if (!strcmp (priority, syslog_level[i]))
			return i;
		i++;
	}
	return -1;
}

void setloglevel (int level)
{
	loglevel = level;
}

void setlogprefix (const char *prefix)
{
	snprintf (logprefix, sizeof (logprefix), "%s", prefix);
}

void openlogger(const char *ident, int err2console)
{
    int option = LOG_PID;
    if (err2console) {
        option |= LOG_CONS;
    }
    openlog(ident, option, LOG_USER);
}

void logger(int level, const char *fmt, ...)
{
	va_list p;
	va_list p2;
	FILE *f = stderr;

	va_start (p, fmt);
	va_copy (p2, p);

	if (level <= LOG_ERR || level <= loglevel) {
		if (level == LOG_DEBUG || level == LOG_INFO)
			f = stdout;
		fprintf (f, "%s, %s", syslog_level_msg[level], logprefix);
		vfprintf (f, fmt, p);
		fputc ('\n', f);
	}

	if (level < LOG_DEBUG || level <= loglevel) {
		int len = strlen (logprefix);
		int fmt2len = strlen (fmt) + len + 1;
		char *fmt2 = xmalloc (sizeof (char *) * fmt2len);
		char *pf = fmt2;
		memcpy (pf, logprefix, len);
		pf += len;
		strlcpy (pf, fmt, fmt2len - len);
		vsyslog (level, fmt2, p2);
		free (fmt2);
	}

	va_end (p2);
	va_end (p);
}

void closelogger(void)
{
    closelog();
}
