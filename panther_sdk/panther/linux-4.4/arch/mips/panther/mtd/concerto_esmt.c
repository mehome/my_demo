#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     2   /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    1   /* TWO speed: 1, 4  */

/* S25FLxx-specific commands */
#define CMD_ESMT_READ	0x03	/* Read Data Bytes */
#define CMD_ESMT_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_ESMT_READID	0x90	/* Read Manufacture ID and Device ID */
#define CMD_ESMT_WREN	0x06	/* Write Enable */
#define CMD_ESMT_WRDI	0x04	/* Write Disable */
#define CMD_ESMT_RDSR	0x05	/* Read Status Register */
#define CMD_ESMT_WRSR	0x01	/* Write Status Register */
#define CMD_ESMT_PP		0x02	/* Page Program */
#define CMD_ESMT_SE		0x20	/* Sector Erase */
#define CMD_ESMT_BE		0xd8	/* Bulk Erase */
#define CMD_ESMT_CE		0xc7	/* Chip Erase */
#define CMD_ESMT_DP		0xb9	/* Deep Power-down */
#define CMD_ESMT_RES	0xab	/* Release from DP, and Read Signature */

#define ESMT_ID_F25L16PA	0x2115
#define ESMT_ID_F25L32QA	0x4116
#define ESMT_ID_F25L64QA	0x4117

struct esmt_spi_flash_params 
{
	u16 idcode1;
	u16 idcode2;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

static const struct esmt_spi_flash_params esmt_spi_flash_table[] = 
{
	{
		.idcode1 = ESMT_ID_F25L16PA,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 32,
		.name = "F25L16PA",
	},
	{
		.idcode1 = ESMT_ID_F25L32QA,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 64,
		.name = "F25L32QA",
	},
    {
        .idcode1 = ESMT_ID_F25L64QA,
        .idcode2 = 0,
        .page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
        .nr_blocks = 128,
        .name = "F25L64QA",
    },
};

int spi_flash_probe_esmt(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct esmt_spi_flash_params *params;
	unsigned int i;
	unsigned short jedec;
    unsigned int jedec_inv; // inverse of jecdec for endian(big/little)

	jedec = (idcode[1] << 8) | idcode[2];
    jedec_inv = idcode[1] | (idcode[2] << 8);

	for (i = 0; i < ARRAY_SIZE(esmt_spi_flash_table); i++) 
    {
		params = &esmt_spi_flash_table[i];
		if (params->idcode1 == jedec || params->idcode1 == jedec_inv) 
        {
			break;
		}
	}

	if (i == ARRAY_SIZE(esmt_spi_flash_table)) 
    {
		FLASH_DEBUG("SF: Unsupported ESMT ID %04x\n", jedec);
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
    
    host->erase_cmd = CMD_ESMT_BE;
    if (host->read_ionum == 2)
    {
        host->fastr_cmd = CMD_ESMT_FAST_READ;
    }
    else
    {
        host->fastr_cmd = CMD_ESMT_READ;
    }
    host->write_cmd = CMD_ESMT_PP;

	mtd->_erase = concerto_spi_nor_erase;
	mtd->_read = concerto_spi_nor_read;
	mtd->_write = concerto_spi_nor_write;

	return 0;

}
