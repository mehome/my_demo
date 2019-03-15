/********************************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd.                                                  */
/* Montage Proprietary and Confidential                                                     */
/* Copyright (c) 2014 Montage Technology Group Limited and its affiliated companies         */
/********************************************************************************************/                

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     2   /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    1   /* TWO speed: 1, 4  */

/* S25FLxx-specific commands */
#define CMD_ISSI_RDSR	0x05	/* Read Status Register */
#define CMD_ISSI_WRSR	0x01	/* Write Status Register */
#define CMD_ISSI_PP		0x02	/* Page Program */
#define CMD_ISSI_BE		0xd8	/* Bulk Erase */
#define CMD_ISSI_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_ISSI_DUAL_READ	0x3b	/* Read Data Bytes at Higher Speed */

#define ISSI_ID_IS25LP032	0x6016
#define ISSI_ID_IS25LP064	0x6017
#define ISSI_ID_IS25LP128	0x6018

struct issi_spi_flash_params 
{
	u16 idcode1;
	u16 idcode2;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

static const struct issi_spi_flash_params issi_spi_flash_table[] = 
{
	{
		.idcode1 = ISSI_ID_IS25LP032,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 64,
		.name = "IS25LP032",
	},
	{
		.idcode1 = ISSI_ID_IS25LP064,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 128,
		.name = "IS25LP064",
	},
    {
        .idcode1 = ISSI_ID_IS25LP128,
        .idcode2 = 0,
        .page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
        .nr_blocks = 256,
        .name = "IS25LP128",
    },
};

int spi_flash_probe_issi(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct issi_spi_flash_params *params = NULL;
	unsigned int i;
	unsigned int jedec;
    unsigned int jedec_inv; // inverse of jecdec for endian(big/little)

	jedec = (idcode[1] << 8) | idcode[2];
    jedec_inv = idcode[1] | (idcode[2] << 8);

	for (i = 0; i < ARRAY_SIZE(issi_spi_flash_table); i++) 
    {
		params = &issi_spi_flash_table[i];
		if (params->idcode1 == jedec || params->idcode1 == jedec_inv) 
        {
			break;
		}
	}

	if (i == ARRAY_SIZE(issi_spi_flash_table)) 
    {
		FLASH_DEBUG("SF: Unsupported ISSI ID %04x\n", jedec);
		return 1;
	}

    host->name = params->name;
    host->pagesize = params->page_size;
    host->blocksize = host->pagesize * params->pages_per_sector * params->sectors_per_block;
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
    
    host->erase_cmd = CMD_ISSI_BE;
    if (host->read_ionum == 2)
    {
        host->fastr_cmd = CMD_ISSI_DUAL_READ;
    }
    else
    {
        host->fastr_cmd = CMD_ISSI_FAST_READ;
    }
    host->write_cmd = CMD_ISSI_PP;

	mtd->_erase = concerto_spi_nor_erase;
	mtd->_read = concerto_spi_nor_read;
	mtd->_write = concerto_spi_nor_write;

	return 0;
}
