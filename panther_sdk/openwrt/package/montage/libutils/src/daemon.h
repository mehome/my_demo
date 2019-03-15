/*
 * =====================================================================================
 *
 *       Filename:  daemon.h
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

#ifndef __DAEMON_H__
#define __DAEMON_H__

void daemon_init();
void daemon_ready();
#if 0
void daemon_fail(const char *format, va_list arg);
#endif
void daemon_exit();

#endif /* __DAEMON_H__ */
