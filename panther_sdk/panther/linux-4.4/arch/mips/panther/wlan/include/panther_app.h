#ifndef __PANTHER_APP_H__
#define __PANTHER_APP_H__
#include <mac_ctrl.h>
struct panther_app
{
	    /* channel */
    u8 channel;
    u8 bandwidth;
    u8 active_bandwidth;
};
struct panther_app lapp_g = {
    	.channel = 1,				/* default channel */
	.bandwidth = BW40MHZ_AUTO,		/* 40MHz */
};
extern struct panther_app *lapp;
struct panther_app *lapp=&lapp_g;

#endif
