/*
 * =====================================================================================
 *
 *       Filename:  strutil.c
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
#include "strutils.h"

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

int str2args (const char *str, char *argv[], char *delim, int max)
{
    char *p;
    int n;
    p = (char *) str;
    for (n=0; n < max; n++)
    {
        if (0==(argv[n]=strtok_r(p, delim, &p)))
            break;
    }
    return n;
}

int str2argv(char *buf, char **v_list, char c)
{
    int n;
    char del[4];

    del[0] = c;
    del[1] = 0;
    n=str2args(buf, v_list, del, MAX_ARGVS);
    v_list[n] = 0;
    return n;
}

char *str_arg(char **v_list, char *var_name)
{
    int i;
    for(i=0; i<MAX_ARGVS; i++)
    {
        if(v_list[i] == 0)
            break;
        if(strncmp(v_list[i], var_name, strlen(var_name)) == 0)
            return (v_list[i]+strlen(var_name));
    }
    return NULL;
}

int str2arglist(char *buf, int *list, char c, int max)
{
    char *idx = buf;
    int j=0;

    list[j++] = (int)buf;
    while(*idx && j<max) {
        if(*idx == c || *idx == '\n') {
            *idx = 0;
            list[j++] = (int)(idx+1);
        }
        idx++;	
    }
    if(j==1 && !(*buf)) // No args
        j = 0;

    return j;
}

int hex2str(char *pszHex, int nHexLen, char *pszString)
{
    int i;
    char ch;
    for (i = 0; i < nHexLen; i++)
    {
        ch = (pszHex[i] & 0xF0) >> 4;
        pszString[i * 2] = (ch > 9) ? (ch + 0x61 - 0x0A) : (ch + 0x30);
        ch = pszHex[i] & 0x0F;
        pszString[i * 2 + 1] = (ch > 9) ? (ch + 0x61 - 0x0A) : (ch + 0x30);
    }
    pszString[i * 2] = 0;

    return 0;
}

void strrep(char *in, char from, char to)
{
    int i;
    int slen;

    if (in) {
        slen = strlen(in);

        for (i = 0; i < slen; i++) {
            if (in[i] == from) {
                in[i] = to;
            }
        }
    }
}

