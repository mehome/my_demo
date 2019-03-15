
/*
 * Copyright (C) 2009 Freescale Semiconductor, Inc.
 *
 * Author: Mingkai Hu (Mingkai.hu@freescale.com)
 * Based on stmicro.c by Wolfgang Denk (wd@denx.de),
 * TsiChung Liew (Tsi-Chung.Liew@freescale.com),
 * and  Jason McMullan (mcmullan@netapp.com)
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     2   /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    1   /* TWO speed: 1, 4  */

/* S25FLxx-specific commands */
#define CMD_S25FLXX_READ	0x03	/* Read Data Bytes */
#define CMD_S25FLXX_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_S25FLXX_4FAST_READ	0x0c	/* Read Data Bytes at Higher Speed */
#define CMD_S25FLXX_READID	0x90	/* Read Manufacture ID and Device ID */
#define CMD_S25FLXX_WREN	0x06	/* Write Enable */
#define CMD_S25FLXX_WRDI	0x04	/* Write Disable */
#define CMD_S25FLXX_RDSR	0x05	/* Read Status Register */
#define CMD_S25FLXX_WRSR	0x01	/* Write Status Register */
#define CMD_S25FLXX_PP		0x02	/* Page Program */
#define CMD_S25FLXX_4PP		0x12	/* Page Program */
#define CMD_S25FLXX_SE		0x20	/* Sector Erase */
#define CMD_S25FLXX_BE		0xd8	/* Sector Erase */
#define CMD_S25FLXX_4BE		0xdc	/* Sector Erase */
#define CMD_S25FLXX_CE		0xc7	/* Bulk Erase */
#define CMD_S25FLXX_DP		0xb9	/* Deep Power-down */
#define CMD_S25FLXX_RES		0xab	/* Release from DP, and Read Signature */

#define SPSN_ID_S25FL008A	0x0213
#define SPSN_ID_S25FL016A	0x0214
#define SPSN_ID_S25FL032A	0x0215
#define SPSN_ID_S25FL064A	0x0216
#define SPSN_ID_S25FL128P	0x2018
#define SPSN_ID_S25FL256S   0x0219
#define SPSN_EXT_ID_S25FL128P_256KB	0x0300
#define SPSN_EXT_ID_S25FL128P_64KB	0x0301
#define SPSN_EXT_ID_S25FL032P		0x4d00
#define SPSN_EXT_ID_S25FL129P		0x4d01
#define SPSN_EXT_ID_S25FL256S       0x4d01

struct spansion_spi_flash_params 
{
	u16 idcode1;
	u16 idcode2;
	u16 addr_width;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

static const struct spansion_spi_flash_params spansion_spi_flash_table[] = 
{
    {
		.idcode1 = SPSN_ID_S25FL008A,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 16,
		.name = "S25FL008A",
	},
	{
		.idcode1 = SPSN_ID_S25FL016A,
		.idcode2 = 0,
		.page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 32,
		.name = "S25FL016A",
	},
	{
		.idcode1 = SPSN_ID_S25FL032A,
		.idcode2 = 0,
		.page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 64,
		.name = "S25FL032A",
	},
	{
		.idcode1 = SPSN_ID_S25FL064A,
		.idcode2 = 0,
		.page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 128,
		.name = "S25FL064A",
	},
    {
        .idcode1 = SPSN_ID_S25FL256S,
        .idcode2 = SPSN_EXT_ID_S25FL256S,
        .addr_width = 4,
        .page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
        .nr_blocks = 512,
        .name = "S25FL256S",
    },
	{
		.idcode1 = SPSN_ID_S25FL128P,
		.idcode2 = SPSN_EXT_ID_S25FL128P_64KB,
		.page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 256,
		.name = "S25FL128P_64K",
	},
	{
		.idcode1 = SPSN_ID_S25FL128P,
		.idcode2 = SPSN_EXT_ID_S25FL128P_256KB,
		.page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 64,
		.nr_blocks = 64,
		.name = "S25FL128P_256K",
	},
	{
		.idcode1 = SPSN_ID_S25FL032A,
		.idcode2 = SPSN_EXT_ID_S25FL032P,
		.page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 64,
		.name = "S25FL032P",
	},
	{
		.idcode1 = SPSN_ID_S25FL128P,
		.idcode2 = SPSN_EXT_ID_S25FL129P,
		.page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 256,
		.name = "S25FL129P_64K",
	},
};

int spi_flash_probe_spansion(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct spansion_spi_flash_params *params;
	unsigned int i;
	unsigned short jedec, ext_jedec;
    unsigned short jedec_inv, ext_jedec_inv;

	jedec = idcode[1] << 8 | idcode[2];
    jedec_inv = idcode[1] | idcode[2] << 8;
	ext_jedec = idcode[3] << 8 | idcode[4];
    ext_jedec_inv = idcode[3] | idcode[4] << 8;

    for (i = 0; i < ARRAY_SIZE(spansion_spi_flash_table); i++) 
    {
        params = &spansion_spi_flash_table[i];
        if (params->idcode1 == jedec && params->idcode2 == ext_jedec) 
        {
            break;
        }

        if (params->idcode1 == jedec_inv && params->idcode2 == ext_jedec_inv)
        {
            break;
        }
    }

    if (i == ARRAY_SIZE(spansion_spi_flash_table)) 
    {
        FLASH_DEBUG("SF: Unsupported SPANSION ID %04x %04x\n", jedec, ext_jedec);
		return 1;
	}

    host->name = params->name;
    host->pagesize = params->page_size;
    host->blocksize = params->page_size * params->pages_per_sector * params->sectors_per_block;
    host->chipsize = host->blocksize * params->nr_blocks;

    if (host->read_ionum > MAX_READ_IO_NUM)
    {
        host->read_ionum = MAX_READ_IO_NUM;
    }
    if (host->write_ionum > MAX_WRITE_IO_NUM)
    {
        host->write_ionum = MAX_WRITE_IO_NUM;
    }

    // TODO by Hsuan
//	    if (host->read_ionum == 2)
//	    {
//	        host->fastr_cmd = CMD_S25FLXX_FAST_READ;
//	    }
//	    else
//	    {
//	        host->fastr_cmd = CMD_S25FLXX_READ;
//	    }
    
    if(params->addr_width == 4)
    {
        host->byte_mode = 4;
        host->erase_cmd = CMD_S25FLXX_4BE;
        host->fastr_cmd = CMD_S25FLXX_4FAST_READ;
        host->write_cmd = CMD_S25FLXX_4PP;
    }
    else
    {
        host->byte_mode = 3;
        host->erase_cmd = CMD_S25FLXX_BE;
        host->fastr_cmd = CMD_S25FLXX_FAST_READ;
        host->write_cmd = CMD_S25FLXX_PP;
    }

	mtd->_erase = concerto_spi_nor_erase;
	mtd->_read = concerto_spi_nor_read;
	mtd->_write = concerto_spi_nor_write;

	return 0;

}


