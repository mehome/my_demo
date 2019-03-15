/* 
 *  Copyright (c) 2013	Montage Inc.	All rights reserved. 
 */
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/pfn.h>
#include <linux/string.h>

#include <asm/bootinfo.h>
#include <asm/page.h>
#include <asm/sections.h>

#define AES_BLOCK_SIZE 16

/* store erased_page data for NAND/NOR flash driver here */
/* The default value is for AES key with all 0's */
unsigned char __erased_page_data_first[] = {
0x12,0x9F,0xA9,0x46,0x7C,0x53,0x77,0x10,0xB3,0x64,0x5E,0xAC,0x4D,0x3E,0xDF,0xFB
};
unsigned char __erased_page_data_next[] = {
0xED,0x60,0x56,0xB9,0x83,0xAC,0x88,0xEF,0x4C,0x9B,0xA1,0x53,0xB2,0xC1,0x20,0x04
};

void load_erased_page_data_from_boot_cmdline(void)
{
    char *str;

    if((str = strstr(arcs_cmdline, "erased_page=")))
    {
        str += 12;
        if(0 > hex2bin(__erased_page_data_first, str, AES_BLOCK_SIZE))
            return;
        if(0 > hex2bin(__erased_page_data_next, &str[AES_BLOCK_SIZE * 2], AES_BLOCK_SIZE))
            return;
    }
}


extern  void panther_serial_outc(unsigned char c);

void __init prom_meminit(void)
{

}

void __init prom_free_prom_memory(void)
{

}

int prom_putchar(char c)
{
	panther_serial_outc(c);
    return 1;
}

