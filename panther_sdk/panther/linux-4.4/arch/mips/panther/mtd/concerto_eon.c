/*
 * (C) Copyright 2010, ucRobotics Inc.
 * Author: Chong Huang <chuang@ucrobotics.com>
 * Licensed under the GPL-2 or later.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     2   /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    1   /* TWO speed: 1, 4  */

/* EN25Q128-specific commands */
#define CMD_EN25Q128_WREN	0x06    /* Write Enable */
#define CMD_EN25Q128_WRDI	0x04    /* Write Disable */
#define CMD_EN25Q128_RDSR	0x05    /* Read Status Register */
#define CMD_EN25Q128_WRSR	0x01    /* Write Status Register */
#define CMD_EN25Q128_READ	0x03    /* Read Data Bytes */
#define CMD_EN25Q128_FAST_READ	0x0b    /* Read Data Bytes at Higher Speed */
#define CMD_EN25Q128_PP		0x02    /* Page Program */
#define CMD_EN25Q128_SE		0x20    /* Sector Erase */
#define CMD_EN25Q128_BE		0xd8    /* Block Erase */
#define CMD_EN25Q128_DP		0xb9    /* Deep Power-down */
#define CMD_EN25Q128_RES	0xab    /* Release from DP, and Read Signature */

#define ENSN_ID_EN25Q32B	0x3016
#define ENSN_ID_EN25Q64B	0x3017
#define ENSN_ID_EN25QH128	0x7018
#define ENSN_ID_EN25QH256	0x7019



struct eon_spi_flash_params 
{
	u16 idcode1;
	u16 idcode2;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

static const struct eon_spi_flash_params eon_spi_flash_table[] = 
{
    {
		.idcode1 = ENSN_ID_EN25Q32B,
        .idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 64,
		.name = "EN25Q32B",
	},
    {
		.idcode1 = ENSN_ID_EN25Q64B,
        .idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 128,
		.name = "EN25Q64B",
	},
	{
		.idcode1 = ENSN_ID_EN25QH128,
        .idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 256,
		.name = "EN25QH128",
	},
    {
        .idcode1 = ENSN_ID_EN25QH256,
        .idcode2 = 0,
        .page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
        .nr_blocks = 512,
        .name = "EN25QH256",
    },
};

int spi_flash_probe_eon(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct eon_spi_flash_params *params;
	unsigned int i;
	unsigned int jedec;
    unsigned int jedec_inv; // inverse of jecdec for endian(big/little)

	jedec = (idcode[1] << 8) | idcode[2];
    jedec_inv = idcode[1] | (idcode[2] << 8);

	for (i = 0; i < ARRAY_SIZE(eon_spi_flash_table); i++) 
    {
		params = &eon_spi_flash_table[i];
		if (params->idcode1 == jedec || params->idcode1 == jedec_inv) 
        {
			break;
		}
	}

    if (i == ARRAY_SIZE(eon_spi_flash_table)) 
    {
        FLASH_DEBUG("SF: Unsupported EON ID %04x\n", jedec);
		return 1;
	}

    host->name = params->name;
    host->pagesize = params->page_size;
    host->blocksize = params->page_size * params->pages_per_sector * params->sectors_per_block;
    host->chipsize = host->blocksize * params->nr_blocks;

    host->byte_mode = 3;
    if (host->read_ionum > MAX_READ_IO_NUM)
    {
        host->read_ionum = MAX_READ_IO_NUM;
    }
    if (host->write_ionum > MAX_WRITE_IO_NUM)
    {
        host->write_ionum = MAX_WRITE_IO_NUM;
    }
    
    host->erase_cmd = CMD_EN25Q128_BE;
    if (host->read_ionum == 2)
    {
        host->fastr_cmd = CMD_EN25Q128_FAST_READ;
    }
    else
    {
        host->fastr_cmd = CMD_EN25Q128_READ;
    }
    host->write_cmd = CMD_EN25Q128_PP;

	mtd->_erase = concerto_spi_nor_erase;
	mtd->_read = concerto_spi_nor_read;
	mtd->_write = concerto_spi_nor_write;

	return 0;
}
