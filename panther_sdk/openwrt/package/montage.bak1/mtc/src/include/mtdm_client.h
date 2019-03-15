/*
 * =====================================================================================
 *
 *       Filename:  mtdm_client.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/15/2016 10:17:56 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef _MTDM_CLIENT_H_
#define _MTDM_CLIENT_H_
#include <sys/types.h>
#include "include/mtdm_types.h"

typedef struct {
    int op;
    u8 args;
    u8 module;
    char *name;
}Mtc;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

typedef int (*client_callback)(char *rbuf, int rlen);

int mtdm_client_call(MtdmPkt *packet, client_callback ccb);
int mtdm_client_simply_call(int op, int arg, client_callback ccb);

#endif
