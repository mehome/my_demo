/*
 * =====================================================================================
 *
 *       Filename:  logger.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/16/2016 10:38:05 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef LOGGER_H
#define LOGGER_H

#ifdef __GNUC__
#  define _PRINTF_LIKE(_one, _two)  __attribute__ ((__format__ (__printf__, _one, _two)))
#endif

#include <syslog.h>

int logtolevel (const char *priority);
void setloglevel (int level);
void setlogprefix (const char *prefix);
void openlogger(const char *ident, int err2console);
void logger (int level, const char *fmt, ...) _PRINTF_LIKE (2, 3);
void closelogger(void);

#endif
