#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     2       /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    1       /* TWO speed: 1, 4  */

/* MX25xx-specific commands */
#define CMD_MX25XX_WREN		0x06	/* Write Enable */
#define CMD_MX25XX_WRDI		0x04	/* Write Disable */
#define CMD_MX25XX_RDSR		0x05	/* Read Status Register */
#define CMD_MX25XX_WRSR		0x01	/* Write Status Register */
#define CMD_MX25XX_READ		0x03	/* Read Data Bytes */
#define CMD_MX25XX_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_MX25XX_DUAL_READ	0x3b	/* Read Data Bytes at Higher Speed */
#define CMD_MX25XX_PP		0x02	/* Page Program */
#define CMD_MX25XX_SE		0x20	/* Sector Erase */
#define CMD_MX25XX_BE		0xD8	/* Block Erase */
#define CMD_MX25XX_CE		0xc7	/* Chip Erase */
#define CMD_MX25XX_DP		0xb9	/* Deep Power-down */
#define CMD_MX25XX_RES		0xab	/* Release from DP, and Read Signature */
#define CMD_MX25XX_4BYTE	0xb7	/* enter 4 byte mode*/


#define MACRONIX_ID_MX25L25735E	0x2019
#define MACRONIX_ID_MX25L4005	0x2013
#define MACRONIX_ID_MX25L8005	0x2014
#define MACRONIX_ID_MX25L1605D	0x2015
#define MACRONIX_ID_MX25L3205D	0x2016
#define MACRONIX_ID_MX25L6405D	0x2017
#define MACRONIX_ID_MX25L12805D	0x2018
#define MACRONIX_ID_MX25L12855E	0x2618




struct macronix_spi_flash_params 
{
	u16 idcode;
	u16 addr_width;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

static const struct macronix_spi_flash_params macronix_spi_flash_table[] = 
{
	{
		.idcode = MACRONIX_ID_MX25L25735E,
                .addr_width = 4,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 512,
		.name = "MX25L25735E",
	},
	{
		.idcode = MACRONIX_ID_MX25L4005,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 8,
		.name = "MX25L4005",
	},
	{
		.idcode = MACRONIX_ID_MX25L8005,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 16,
		.name = "MX25L8005",
	},
	{
		.idcode = MACRONIX_ID_MX25L1605D,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 32,
		.name = "MX25L1605D",
	},
	{
		.idcode = MACRONIX_ID_MX25L3205D,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 64,
		.name = "MX25L3205D",
	},
	{
		.idcode = MACRONIX_ID_MX25L6405D,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 128,
		.name = "MX25L6405D",
	},
	{
		.idcode = MACRONIX_ID_MX25L12805D,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 256,
		.name = "MX25L12805D",
	},
	{
		.idcode = MACRONIX_ID_MX25L12855E,
		.page_size = 256,
		.pages_per_sector = 16,
		.sectors_per_block = 16,
		.nr_blocks = 256,
		.name = "MX25L12855E",
	},
};

static int macronix_enter_4byte_mode(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;

    int ret;

    ret = concerto_spi_cmd(host->spi, CMD_MX25XX_4BYTE, NULL, 0);
    if (ret < 0) 
    {
        FLASH_DEBUG("SF: enter 4 byte mode Failed!\n");
        return ret;
    }

    return 0;
}

int spi_flash_probe_macronix(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct macronix_spi_flash_params *params;
	unsigned int i;
	unsigned int jedec;
    unsigned int jedec_inv; // inverse of jecdec for endian(big/little)

	jedec = (idcode[1] << 8) | idcode[2];
    jedec_inv = idcode[1] | (idcode[2] << 8);

	for (i = 0; i < ARRAY_SIZE(macronix_spi_flash_table); i++) 
    {
		params = &macronix_spi_flash_table[i];
		if (params->idcode == jedec || params->idcode == jedec_inv) 
        {
			break;
		}
	}

	if (i == ARRAY_SIZE(macronix_spi_flash_table)) 
    {
		FLASH_DEBUG("SF: Unsupported Macronix ID %04x\n", jedec);
		return 1;
	}

    host->name = params->name;
    host->pagesize = params->page_size;
    host->blocksize = host->pagesize * params->pages_per_sector * params->sectors_per_block;
    host->chipsize = host->blocksize * params->nr_blocks;

    if(params->addr_width == 4)
    {
        host->byte_mode = 4;
    }
    else
    {
        host->byte_mode = 3;
    }
    
    if (host->read_ionum > MAX_READ_IO_NUM)
    {
        host->read_ionum = MAX_READ_IO_NUM;
    }
    if (host->write_ionum > MAX_WRITE_IO_NUM)
    {
        host->write_ionum = MAX_WRITE_IO_NUM;
    }

    if (host->read_ionum == 2)
    {
        host->fastr_cmd = CMD_MX25XX_DUAL_READ;
    }
    else
    {
        host->fastr_cmd = CMD_MX25XX_FAST_READ;
    }

    host->erase_cmd = CMD_MX25XX_BE;
    host->write_cmd = CMD_MX25XX_PP;

	mtd->_erase = concerto_spi_nor_erase;
	mtd->_read = concerto_spi_nor_read;
	mtd->_write = concerto_spi_nor_write;

    if(host->byte_mode == 4)
        macronix_enter_4byte_mode(mtd);

	return 0;
}
