/* 
 *  Copyright (c) 2016  Montage Inc.	All rights reserved. 
 *
 *  Routines to allocate SRAM resources
 *
 */

#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/mach-panther/panther.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>

#if defined(CONFIG_PANTHER_WLAN)
extern unsigned int wlan_driver_sram_size;
extern void* wlan_driver_sram_base;
#endif

// address range of 2nd 16K of SRAM
#define SRAM_CACHED_ADDRESS_SWITCH_BASE 0x90000000UL
#define SRAM_ADDRESS_SWITCH_BASE 0xB0000000UL
#define SRAM_ADDRESS_BASE       0xB0004000UL
#define SRAM_ADDRESS_LIMIT      0xB0007F00UL    /* reserve 0x100 bytes for EJTAG & boot failure detection */
#define SRAM_BACKUP_SIZE        (SRAM_ADDRESS_LIMIT - SRAM_ADDRESS_SWITCH_BASE)

void sram_init(void)
{
    unsigned long sram_alloc_ptr;

    sram_alloc_ptr = SRAM_ADDRESS_BASE;

#if defined(CONFIG_PANTHER_WLAN)
    if(wlan_driver_sram_size)
    {
        printk("L1 cache line size %d\n", L1_CACHE_BYTES);
        printk("SRAM: %08X-%08X (%d bytes) allocated to WLAN driver\n",
                 (unsigned) sram_alloc_ptr, ((unsigned) sram_alloc_ptr + wlan_driver_sram_size - 1), wlan_driver_sram_size);

        sram_alloc_ptr += wlan_driver_sram_size;

        // re-align to cache-line size
        if(sram_alloc_ptr % L1_CACHE_BYTES)
            sram_alloc_ptr += (L1_CACHE_BYTES - sram_alloc_ptr % L1_CACHE_BYTES);
    }
#endif

    if(sram_alloc_ptr>SRAM_ADDRESS_LIMIT)
        panic("SRAM: out of sram resource\n");
}

#if defined(CONFIG_PM)
static unsigned char sram_backup[SRAM_BACKUP_SIZE];
void sram_suspend(void)
{
    dma_cache_wback_inv(SRAM_CACHED_ADDRESS_SWITCH_BASE, SRAM_BACKUP_SIZE);
    memcpy(sram_backup, (void *) SRAM_ADDRESS_SWITCH_BASE, SRAM_BACKUP_SIZE);
}

void sram_resume(void)
{
    dma_cache_wback_inv(SRAM_CACHED_ADDRESS_SWITCH_BASE, SRAM_BACKUP_SIZE);
    memcpy((void *) SRAM_ADDRESS_SWITCH_BASE, sram_backup, SRAM_BACKUP_SIZE);
}
#endif

