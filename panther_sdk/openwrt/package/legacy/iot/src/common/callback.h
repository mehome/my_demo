#ifndef __CALLBACK_H__

#define __CALLBACK_H__

typedef int (*proc)(void *arg);

typedef struct Callback {
    int type;
    proc callback;
}Callback;


#endif

