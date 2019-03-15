
/**
 * @file mtdm_event.h
 *
 * @author  Frank Wang
 * @date    2015-10-12
 */

#ifndef _MTDM_EVENT_H_ 
#define _MTDM_EVENT_H_

typedef enum 
{
    EventKey = 0,

    EventMax
}EventCode;

typedef struct
{
    EventCode code;
    int scode;
    int size;
}EventBlock;

typedef struct
{
    OpCodeKey key;
    int data;
}KeyCB;

void sendEvent(EventCode code, int scode, void *data, int size, unsigned char blockmode);

#endif

