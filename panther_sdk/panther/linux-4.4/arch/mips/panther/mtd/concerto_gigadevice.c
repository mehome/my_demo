#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     4   /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    4   /* TWO speed: 1, 4  */

/* S25FLxx-specific commands */
#define CMD_GIGADEVICE_READ	        0x03	/* Read Data Bytes */
#define CMD_GIGADEVICE_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_GIGADEVICE_DUAL_READ	0x3b	/* Read Data Bytes at Dual IO */
#define CMD_GIGADEVICE_QUAD_READ	0x6b	/* Read Data Bytes at Quad IO */
#define CMD_GIGADEVICE_READID	0x90	/* Read Manufacture ID and Device ID */
#define CMD_GIGADEVICE_WREN	    0x06	/* Write Enable */
#define CMD_GIGADEVICE_WRDI	    0x04	/* Write Disable */
#define CMD_GIGADEVICE_RDSR1    0x05	/* Read Status Register-1  */
#define CMD_GIGADEVICE_RDSR2    0x35    /* Read Status Reguster-2  */
#define CMD_GIGADEVICE_WRSR1	0x01	/* Write Status Register-1 */
#define CMD_GIGADEVICE_WRSR2    0x31    /* Write Status Register-2 */
#define CMD_GIGADEVICE_PP		0x02	/* Page Program */
#define CMD_GIGADEVICE_QUAD_PP	0x32	/* Page Program at Quad IO */
#define CMD_GIGADEVICE_SE		0x20	/* Sector Erase */
#define CMD_GIGADEVICE_BE		0xd8	/* Sector Erase */
#define CMD_GIGADEVICE_CE		0xc7	/* Bulk Erase */
#define CMD_GIGADEVICE_DP		0xb9	/* Deep Power-down */
#define CMD_GIGADEVICE_RES	    0xab	/* Release from DP, and Read Signature */

#define GIGADEVICE_ID_GD25Q128B	0x4018
#define GIGADEVICE_ID_GD25Q64B	0x4017
#define GIGADEVICE_ID_GD25Q32B	0x4016

struct gigadevice_spi_flash_params 
{
	u16 idcode1;
	u16 idcode2;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

static const struct gigadevice_spi_flash_params gigadevice_spi_flash_table[] = 
{
	{
		.idcode1 = GIGADEVICE_ID_GD25Q64B,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 128,
		.name = "GD25Q64B",
	},
	{
		.idcode1 = GIGADEVICE_ID_GD25Q32B,
		.idcode2 = 0,
		.page_size = 256,
		.pages_per_sector = 16,
        .sectors_per_block = 16,
		.nr_blocks = 64,
		.name = "GD25Q32B",
	},
    {
        .idcode1 = GIGADEVICE_ID_GD25Q128B,
        .idcode2 = 0,
        .page_size = 256,
        .pages_per_sector = 16,
        .sectors_per_block = 16,
        .nr_blocks = 256,
        .name = "GD25Q128B",
    },
};

int spi_flash_probe_gigadevice(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct gigadevice_spi_flash_params *params = NULL;
	unsigned int i;
	unsigned int jedec;
    unsigned int jedec_inv; // inverse of jecdec for endian(big/little)

	jedec = (idcode[1] << 8) | idcode[2];
    jedec_inv = idcode[1] | (idcode[2] << 8);
    printk("jedec = 0x%x\n", jedec);

	for (i = 0; i < ARRAY_SIZE(gigadevice_spi_flash_table); i++) 
    {
		params = &gigadevice_spi_flash_table[i];
		if (params->idcode1 == jedec || params->idcode1 == jedec_inv) 
        {
			break;
		}
	}

	if (i == ARRAY_SIZE(gigadevice_spi_flash_table)) 
    {
		FLASH_DEBUG("SF: Unsupported GIGADIVICE ID %04x\n", jedec);
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
    
    host->erase_cmd = CMD_GIGADEVICE_BE;

    if (host->read_ionum == 4)
    {
        host->fastr_cmd = CMD_GIGADEVICE_QUAD_READ;
    }
    else if (host->read_ionum == 2)
    {
        host->fastr_cmd = CMD_GIGADEVICE_DUAL_READ;
    }
    else
    {
        host->fastr_cmd = CMD_GIGADEVICE_FAST_READ;
    }

    if (host->write_ionum == 4)
    {
        host->write_cmd = CMD_GIGADEVICE_QUAD_PP;
    }
    else
    {
        host->write_cmd = CMD_GIGADEVICE_PP;
    }    

	mtd->_erase = concerto_spi_nor_erase;
	mtd->_read = concerto_spi_nor_read;
	mtd->_write = concerto_spi_nor_write;

    host->write_reg1_cmd = CMD_GIGADEVICE_WRSR1;
    host->write_reg2_cmd = CMD_GIGADEVICE_WRSR2;
    host->read_reg1_cmd = CMD_GIGADEVICE_RDSR1;
    host->read_reg2_cmd = CMD_GIGADEVICE_RDSR2;

	return 0;
}
