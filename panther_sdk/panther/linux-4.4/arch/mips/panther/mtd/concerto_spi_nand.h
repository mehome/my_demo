#ifndef __CONCERTO_SPI_NAND_H__
#define __CONCERTO_SPI_NAND_H__



#include "concerto_spi.h"

#define CONCERTO_SPI_NAND_FLASH_PARTITIONS


#define KB  1024
#define MB  (1024*KB)
#define GB  (1024*MB)

#define BOOT_CFG         0xBF140020

///////////////////////////////////////////////////
// concerto used
#define SPI_NAND_FLASH_GD5F1GQ4UAYIG    0xc8f100
#define SPI_NAND_FLASH_GD5F2GQ4UAYIG    0xc8f200

#define SPI_NAND_FLASH_GD5F1GQ4UB       0xc8d100
#define SPI_NAND_FLASH_GD5F2GQ4UB       0xc8d2c8
#define SPI_NAND_FLASH_GD5F2GQ4UB_LE    0xc8d2d2

#define SPI_NAND_FLASH_GD5F4GQ4U_A      0xc8d4c8
#define SPI_NAND_FLASH_GD5F4GQ4U_B      0xc8d4d4

#define SPI_NAND_FLASH_F50L512M41A      0xc82000
#define SPI_NAND_FLASH_F50L1G41A        0xc82100
#define SPI_NAND_FLASH_W25N01GV         0xefaa00

#define SPI_NAND_FLASH_HYF1GQ4UAACAE    0xc95100
#define SPI_NAND_FLASH_HYF2GQ4UAACAE    0xc95200
///////////////////////////////////////////////////
// panther used now
#define SPI_NAND_FLASH_GD5F1GQ4UCYIG    0xc8b148
#define SPI_NAND_FLASH_GD5F2GQ4UCYIG    0xc8b248
#define SPI_NAND_FLASH_MX35LF1GE4AB	0xc212c2
#define SPI_NAND_FLASH_MX35LF1GE4BA	0xc21212
#define SPI_NAND_FLASH_W25M02GC         0xefab21
#define SPI_NAND_FLASH_F50L2G41LB       0x7f0ac8
#define SPI_NAND_FLASH_XT26G01A         0xe10be1
///////////////////////////////////////////////////

#define READ_CMD_DUMMY_END 0
#define READ_CMD_DUMMY_START 1

struct concerto_spi_nand_flash_params 
{
	u32 id;
	u32 page_size;
	u32 oob_size;
	u32 pages_per_block;
	u32 nr_blocks;
	const char *name;
	struct nand_ecclayout *layout;
    u32 read_cmd_dummy_type;    // because read command of some manufacturer's chip have different format
    u32 read_from_cache_dummy;  // same as above
};

struct concerto_spi_nand_hw_control {
	struct concerto_snfc_host *active;
};

struct concerto_snfc_host 
{
    struct nand_chip *chip;
    struct mtd_info	 *mtd;
    struct device *dev;

    struct spi_slave *spi;

    const char *name;

    unsigned int read_ionum;
    unsigned int write_ionum;

    unsigned int pageno;
    unsigned int command;
    unsigned int command2;

    int	chipselect;

    unsigned int freq_div;

    unsigned int pagesize;
    unsigned int oobsize;
    unsigned int blocksize;
    uint64_t     chipsize;

    unsigned int page_shift;
    unsigned int erase_shift;
    unsigned int chip_shift;

    unsigned int badblockpos;
    
	struct nand_ecclayout	*ecclayout;

    unsigned char *buffer;
#define BUFFER_PAGE_ADDR_INVALID 0xFFFFFFFFUL
    unsigned int buffer_page_addr;
    unsigned char *oob_poi;

    struct concerto_spi_nand_hw_control *controller;
    struct concerto_spi_nand_hw_control hwcontrol;
    flstate_t state;

    u32 read_cmd_dummy_type;    // because read command of some manufacturer's chip have different format
    u32 read_from_cache_dummy;  // same as above
};


#endif

