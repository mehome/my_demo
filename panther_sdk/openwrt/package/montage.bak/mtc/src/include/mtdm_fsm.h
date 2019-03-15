
/**
 * @file mtdm_fsm.h
 *
 * @author  Frank Wang
 * @date    2015-10-08
 */

#ifndef _MTDM_FSM_H_ 
#define _MTDM_FSM_H_

#include "mtdm_fsm.h"

typedef enum
{
    fsm_unknow = 0,
    fsm_boot,
    fsm_stop,
    fsm_key,

    fsm_max
}fsm_event;

typedef enum
{
    fsm_mode_unknow = 0,
    fsm_mode_idle,
    fsm_mode_running,
    fsm_mode_stop,

    fsm_mode_max
}fsm_mode;

typedef struct
{
    fsm_mode mode;
}fsm_private_t;

fsm_mode mtdm_get_fsm_mode();
void mtdm_set_fsm_mode(fsm_mode mode);
void mtdm_fsm_schedule(fsm_event event);
void mtdm_fsm_boot();
void mtdm_fsm();

#endif

