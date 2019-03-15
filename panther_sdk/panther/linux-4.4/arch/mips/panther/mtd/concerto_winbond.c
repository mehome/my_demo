/*
 * Copyright 2008, Network Appliance Inc.
 * Author: Jason McMullan <mcmullan <at> netapp.com>
 * Licensed under the GPL-2 or later.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     4   /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    4   /* TWO speed: 1, 4  */

/* M25Pxx-specific commands */
#define CMD_W25_WREN		0x06	/* Write Enable */
#define CMD_W25_WRDI		0x04	/* Write Disable */
#define CMD_W25_RDSR		0x05	/* Read Status Register */
#define CMD_W25_WRSR		0x01	/* Write Status Register */
#define CMD_W25_READ		0x03	/* Read Data Bytes */
#define CMD_W25_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_W25_4FAST_READ	0x0c	/* Read Data Bytes at Higher Speed with 4-Byte Address */
#define CMD_W25_DUAL_READ	0x3b	/* Read Data Bytes at dual Speed */
#define CMD_W25_4DUAL_READ	0x3c	/* Read Data Bytes at dual Speed with 4-Byte Address */
#define CMD_W25_QUAD_READ	0x6b	/* Read Data Bytes at quad Speed */
#define CMD_W25_4QUAD_READ	0x6c	/* Read Data Bytes at quad Speed with 4-Byte Address */
#define CMD_W25_PP		0x02	/* Page Program */
#define CMD_W25_4PP		0x12	/* Page Program */
#define CMD_W25_QUAD_PP 	0x32    /* Page Program at quad speed */
#define CMD_W25_4QUAD_PP	0x34	/* Page Program at quad speed with 4-Byte Address */
#define CMD_W25_SE		0x20	/* Sector (4K) Erase */
#define CMD_W25_BE		0xd8	/* Block (64K) Erase */
#define CMD_W25_4BE		0xdc	/* Block (64K) Erase with 4-Byte Address */
#define CMD_W25_CE		0xc7	/* Chip Erase */
#define CMD_W25_DP		0xb9	/* Deep Power-down */
#define CMD_W25_RES		0xab	/* Release from DP, and Read Signature */
#define CMD_W25_4BYTE		0xb7	/* enter 4 byte mode*/
#define CMD_W25_4BYTE_EXIT  0xe9	/* exit 4 byte mode*/
#define CMD_W25_SW_DIE_SEL     0xc2    /* Software Die Select */

struct winbond_spi_flash_params 
{
	u16	id;
	u16 addr_width;
	/* Log2 of page size in power-of-two mode */
	u8 l2_page_size;
	u16	pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	u16 nr_dies;
	const char	*name;
};

static const struct winbond_spi_flash_params winbond_spi_flash_table[] = 
{
	{
		.id			= 0x3013,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 8,
		.name			= "W25X40",
	},
	{
		.id			= 0x3015,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 32,
		.name			= "W25X16",
	},
	{
		.id			= 0x3016,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 64,
		.name			= "W25X32",
	},
	{
		.id			= 0x3017,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 128,
		.name			= "W25X64",
	},
	{
		.id			= 0x4015,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 32,
		.name			= "W25Q16",
	},
	{
		.id			= 0x4016,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 64,
		.name			= "W25Q32",
	},
	{
		.id			= 0x4017,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 128,
		.name			= "W25Q64",
	},
	{
		.id			= 0x4018,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 256,
		.name			= "W25Q128",
	},
	{
		.id                 = 0x4019,
		.addr_width         = 4,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 512,
		.name			= "W25Q256",
	},
	{
		.id			= 0x6019,
		.addr_width		= 4,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 512,
		.name			= "W25Q256",
	},
#if 0 // turn off 2 dies & 4 address mode for IPL/BOOT1/BOOT2 compatibility
	{
		.id			= 0x7119,
		//.addr_width		= 4,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 256,
		//.nr_dies 		= 2,
		.name			= "W25M512",
	},
#else
	{
		.id			= 0x7119,
		.addr_width		= 4,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 512,
		.nr_dies 		= 2,
		.name			= "W25M512",
	},
#endif
};

static int winbond_enter_4byte_mode(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;

    int ret;

    ret = concerto_spi_cmd(host->spi, CMD_W25_4BYTE, NULL, 0);
    if (ret < 0)
    {
        FLASH_DEBUG("SF: enter 4 byte mode Failed!\n");
        return ret;
    }

    return 0;
}

#if 0
static int winbond_exit_4byte_mode(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;

    int ret;

    ret = concerto_spi_cmd(host->spi, CMD_W25_4BYTE_EXIT, NULL, 0);
    if (ret < 0)
    {
        FLASH_DEBUG("SF: exit 4 byte mode Failed!\n");
        return ret;
    }

    return 0;
}
#endif

static int winbond_die_select(struct mtd_info *mtd, loff_t *address)
{
    struct concerto_sfc_host *host	= mtd->priv;
    int ret;
    u8 cmd[2];
    u8 value = 0;
    u8 die_id;

    if(0==host->die_shift)
        return 0;

    die_id = *address >> host->die_shift;
    *address = *address & (host->diesize - 1);

    if(host->current_die_id==die_id)
        return 0;

    cmd[0] = CMD_W25_SW_DIE_SEL;
    cmd[1] = die_id;

    ret = concerto_spi_cmd_write(host->spi, cmd, 2, &value, 1, 0);
    if (ret < 0)
    {
        FLASH_DEBUG("SF: software die selection failed!\n");
        return ret;
    }

    host->current_die_id = die_id;

    return 0;
}

int spi_flash_probe_winbond(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

    const struct winbond_spi_flash_params *params = NULL;
    unsigned int i;
    unsigned int jedec;
    unsigned int jedec_inv; // inverse of jecdec for endian(big/little)
    loff_t address = 0;

    jedec = (idcode[1] << 8) | idcode[2];
    jedec_inv = idcode[1] | (idcode[2] << 8);

    for (i = 0; i < ARRAY_SIZE(winbond_spi_flash_table); i++) 
    {
	params = &winbond_spi_flash_table[i];
	if (params->id == jedec || params->id == jedec_inv) 
        {
	    break;
	}
    }

    if (i == ARRAY_SIZE(winbond_spi_flash_table)) 
    {
	FLASH_DEBUG("SF DRV ERROR: Unsupported Winbond ID %02x%02x\n",
			idcode[1], idcode[2]);
	return 1;
    }

    host->name = params->name;
    host->pagesize = 1 << params->l2_page_size;
    host->blocksize = host->pagesize * params->pages_per_sector * params->sectors_per_block;
    host->chipsize = host->blocksize * params->nr_blocks;
    if(params->nr_dies>=2)
    {
        host->diesize = host->chipsize;
        host->die_shift = ffs(host->diesize) - 1;
        host->chipsize = host->chipsize * params->nr_dies;
    }

    if(params->addr_width == 4)
    {
        host->byte_mode = 4;

        if (host->read_ionum > MAX_READ_IO_NUM)
            host->read_ionum = MAX_READ_IO_NUM;

        if (host->write_ionum > MAX_WRITE_IO_NUM)
            host->write_ionum = MAX_WRITE_IO_NUM;

        host->erase_cmd = CMD_W25_4BE;

        if (host->read_ionum == 4)
            host->fastr_cmd = CMD_W25_4QUAD_READ;
        else if (host->read_ionum == 2)
            host->fastr_cmd = CMD_W25_4DUAL_READ;
        else
            host->fastr_cmd = CMD_W25_4FAST_READ;
    
        if (host->write_ionum == 4)
            host->write_cmd = CMD_W25_4QUAD_PP;
        else
            host->write_cmd = CMD_W25_4PP;

        winbond_enter_4byte_mode(mtd);

        if(params->nr_dies>=2)
        {
            winbond_die_select(mtd, &address);
            host->die_select = winbond_die_select;
        }
    }
    else
    {
        host->byte_mode = 3;
        if (host->read_ionum > MAX_READ_IO_NUM)
        {
            host->read_ionum = MAX_READ_IO_NUM;
        }
        if (host->write_ionum > MAX_WRITE_IO_NUM)
        {
            host->write_ionum = MAX_WRITE_IO_NUM;
        }

        host->erase_cmd = CMD_W25_BE;

        if (host->read_ionum == 4)
        {
            host->fastr_cmd = CMD_W25_QUAD_READ;
        }
        else if (host->read_ionum == 2)
        {
            host->fastr_cmd = CMD_W25_DUAL_READ;
        }
        else
        {
            host->fastr_cmd = CMD_W25_FAST_READ;
        }

        if (host->write_ionum == 4)
        {
            host->write_cmd = CMD_W25_QUAD_PP;
        }
        else
        {
            host->write_cmd = CMD_W25_PP;
        }
    }

    mtd->_erase = concerto_spi_nor_erase;
    mtd->_read = concerto_spi_nor_read;
    mtd->_write = concerto_spi_nor_write;

    return 0;
}
