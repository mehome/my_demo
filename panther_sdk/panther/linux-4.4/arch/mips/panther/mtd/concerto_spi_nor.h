/******************************************************************************/
/* Copyright (c) 2012 Montage Tech - All Rights Reserved                      */
/******************************************************************************/
#ifndef __CONCERTO_SPI_NOR_H__
#define __CONCERTO_SPI_NOR_H__

//#include "sys_types2.h"

//#include "sys_define.h"
#include <linux/slab.h>
#include <linux/mtd/nand.h>

#include "concerto_spi.h"

#define CONCERTO_SPI_FLASH_PARTITIONS

#define IDCODE_LEN 5

#define MAX_NOR_BUFFER_SIZE 256

typedef struct
{
    u8 shift;
    u8 idcode;
    int (*probe) (struct mtd_info *mtd, u8 *idcode);
}spi_flash_probe_t;


struct concerto_spi_hw_control {
	struct concerto_sfc_host *active;
};


struct concerto_sfc_host 
{
    struct mtd_info	 *mtd;
    struct device *dev;

    struct spi_slave *spi;

    const char *name;

    unsigned int read_ionum;
    unsigned int write_ionum;

    unsigned int pagesize;
    unsigned int blocksize;
    uint64_t     diesize;
    uint64_t     chipsize;

    unsigned int page_shift;
    unsigned int erase_shift;
    unsigned int chip_shift;
    unsigned int die_shift;
    u8 current_die_id;

    u8 erase_cmd;
    u8 fastr_cmd;
    u8 write_cmd;
    u8 byte_mode;

    u8 write_reg1_cmd;
    u8 write_reg2_cmd;
    u8 read_reg1_cmd;
    u8 read_reg2_cmd;

    struct concerto_spi_hw_control *controller;
    struct concerto_spi_hw_control hwcontrol;
    flstate_t state;

    int (*die_select) (struct mtd_info *mtd, loff_t *address);

    unsigned char *buffer;
};








//spi flash mamufactory id
/*!
    The device id of MX
  */
#define FLASH_MX 0xC2

/*!
    The device id of ATMEL
  */
#define FLASH_MF_ATMEL 0x1F
/*!
    The device id of ESMT
  */
#define FLASH_MF_ESMT   0x8C
/*!
    The device id of SST
  */
#define FLASH_MF_SST      0xBF

/*!
    The device id of KH
  */
#define FLASH_MF_KH       0xC2
/*!
    The device id of WINBOND
  */
#define FLASH_MF_WINB   0xEF
/*!
    The device id of SPANSION
  */
#define FLASH_MF_SPAN   0x01
/*!
    The device id of EON
  */
#define FLASH_MF_EON     0x1C
/*!
    The device id of AMIC
  */
#define FLASH_MF_AMIC   0x37
/*!
    The device id of pFlash
  */
#define FLASH_MF_PFLASH 0x7F
/*!
    The device id of GigaDevice
  */
#define FLASH_MF_GD 0xC8



extern int concerto_spi_nor_read(struct mtd_info *mtd, loff_t from, size_t len,
                size_t *retlen, u_char *buf);
extern int concerto_spi_nor_write(struct mtd_info *mtd, loff_t offset,
                size_t len, size_t *retlen, const u_char *buf);
extern int concerto_spi_nor_erase(struct mtd_info *mtd, struct erase_info *instr);


extern int spi_flash_probe_sst(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_esmt(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_macronix(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_gigadevice(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_issi(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_eon(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_spansion(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_winbond(struct mtd_info *mtd, u8 *idcode);
extern int spi_flash_probe_dosilicon(struct mtd_info *mtd, u8 *idcode);



#endif //__CONCERTO_SPI_NOR_H__


