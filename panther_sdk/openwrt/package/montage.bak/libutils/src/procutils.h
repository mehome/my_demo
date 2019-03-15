/*
 * =====================================================================================
 *
 *       Filename:  procutils.h
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

#ifndef __PROCUTILS_H__
#define __PROCUTILS_H__

char *psname(int pid, char *buffer, int maxlen);

int pidof(const char *name);

int killall(const char *name, int sig);

#endif /* __PROCUTILS_H__ */
