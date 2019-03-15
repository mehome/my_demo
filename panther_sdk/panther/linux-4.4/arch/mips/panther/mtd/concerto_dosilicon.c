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

/* FM25Qxx-specific commands */
#define CMD_DOSI_RDSR	0x05	/* Read Status Register */
#define CMD_DOSI_WRSR	0x01	/* Write Status Register */
#define CMD_DOSI_PP		0x02	/* Page Program */
#define CMD_DOSI_BE		0xd8	/* Bulk Erase */
#define CMD_DOSI_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_DOSI_DUAL_READ	0x3b	/* Read Data Bytes at Higher Speed */

#define DOSI_ID_FM25Q64	    0x3217

struct dosi_spi_flash_params 
{
	u16 idcode1;
	u16 idcode2;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

static const struct dosi_spi_flash_params dosi_spi_flash_table[] = 
{
	{
		.idcode1 = DOSI_ID_FM25Q64,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 128,
		.name = "FM25Q64",
	},
};

int spi_flash_probe_dosilicon(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct dosi_spi_flash_params *params = NULL;
	unsigned int i;
	unsigned int jedec;
    unsigned int jedec_inv; // inverse of jecdec for endian(big/little)

	jedec = (idcode[1] << 8) | idcode[2];
    jedec_inv = idcode[1] | (idcode[2] << 8);

	for (i = 0; i < ARRAY_SIZE(dosi_spi_flash_table); i++) 
    {
		params = &dosi_spi_flash_table[i];
		if (params->idcode1 == jedec || params->idcode1 == jedec_inv) 
        {
			break;
		}
	}

	if (i == ARRAY_SIZE(dosi_spi_flash_table)) 
    {
		FLASH_DEBUG("SF: Unsupported DOSI ID %04x\n", jedec);
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
    host->erase_cmd = CMD_DOSI_BE;
    if (host->read_ionum == 2)
    {
        host->fastr_cmd = CMD_DOSI_DUAL_READ;
    }
    else
    {
        host->fastr_cmd = CMD_DOSI_FAST_READ;
    }
    host->write_cmd = CMD_DOSI_PP;

	mtd->_erase = concerto_spi_nor_erase;
	mtd->_read = concerto_spi_nor_read;
	mtd->_write = concerto_spi_nor_write;

	return 0;
}
