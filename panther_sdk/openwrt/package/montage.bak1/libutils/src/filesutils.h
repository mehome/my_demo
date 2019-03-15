/*
 * =====================================================================================
 *
 *       Filename:  filesutils.h
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

#ifndef __FILESUTILS_H__
#define __FILESUTILS_H__

#define FW_CREATE   0
#define FW_APPEND   1
#define FW_NEWLINE  2

int f_exists_and_not_zero_byte(const char *path);
int f_exists(const char *path);
int f_isdir(const char *path);

unsigned long f_size(const char *path);

int f_read(const char *path, void *buffer, int max);

int f_write(const char *path, const void *buffer, int len, unsigned flags, unsigned cmode);

int f_read_string(const char *path, char *buffer, int max);

int f_write_string(const char *path, const char *buffer, unsigned flags, unsigned cmode);

int f_write_sprintf(const char *path, unsigned flags, unsigned cmode, const char *fmt, ...);

int f_read_alloc(const char *path, char **buffer, int max);

int f_read_alloc_string(const char *path, char **buffer, int max);

int f_wait_exists(const char *name, int max);

int f_wait_notexists(const char *name, int max);

int f_wsprintf(const char *file, const char *fmt, ...);

#endif /* __FILESUTILS_H__ */
