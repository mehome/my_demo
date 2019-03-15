/*
 * =====================================================================================
 *
 *       Filename:  strutil.h
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

#ifndef __STRUTILS_H__
#define __STRUTILS_H__

#define MAX_ARGVS    20

int str2args (const char *str, char *argv[], char *delim, int max);
int str2argv(char *buf, char **v_list, char c);
char *str_arg(char **v_list, char *var_name);
int str2arglist(char *buf, int *list, char c, int max);
int hex2str(char *pszHex, int nHexLen, char *pszString);
void strrep(char *in, char from, char to);

#endif /* __STRUTILS_H__ */
