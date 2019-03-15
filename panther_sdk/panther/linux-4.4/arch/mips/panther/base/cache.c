/*
 *  Copyright (c) 2017  Montage Inc.	All rights reserved. 
 *
 *  Test code to manipulate cache and check performance
 *
 */
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/mach-panther/panther.h>

#include "cache.h"

int cache_test_thread(void *data)
{
    volatile unsigned long *icache_shrink;
    volatile unsigned long *dcache_shrink;

    icache_shrink = (volatile unsigned long *) 0xA0000080;
    dcache_shrink = (volatile unsigned long *) 0xA0000084;

    while(1)
    {
        if(*icache_shrink)
            HAL_ICACHE_LOCK(0x83F00000, *icache_shrink * 1024);
        if(*dcache_shrink)
            HAL_DCACHE_LOCK(0x83F00000, *dcache_shrink * 1024);
        schedule_timeout_interruptible(1);
    };

    return 0;
}

static int __init panther_cache_init(void)
{
    printk("Shrinking cache size for testing...\n");

    //HAL_ICACHE_LOCK(0x80000000, 48 * 1024);
    //HAL_DCACHE_LOCK(0x80000000, 16 * 1024);

    //HAL_ICACHE_LOCK(0x83F00000, 64 * 1024);
    //HAL_DCACHE_LOCK(0x83F00000, 32 * 1024);

    kthread_run(cache_test_thread, (void *) 0, "cache test");

    return 0;
}

late_initcall(panther_cache_init);

