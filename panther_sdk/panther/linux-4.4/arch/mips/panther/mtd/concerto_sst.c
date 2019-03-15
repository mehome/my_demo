#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "concerto_spi_nor.h"

#define MAX_READ_IO_NUM     2       /* three speed: 1, 2, 4  */
#define MAX_WRITE_IO_NUM    1       /* TWO speed: 1, 4  */

#define CMD_SST_WREN		0x06	/* Write Enable */
#define CMD_SST_WRDI		0x04	/* Write Disable */
#define CMD_SST_RDSR		0x05	/* Read Status Register */
#define CMD_SST_WRSR		0x01	/* Write Status Register */
#define CMD_SST_READ		0x03	/* Read Data Bytes */
#define CMD_SST_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_SST_BP		    0x02	/* Byte Program */
#define CMD_SST_AAI_WP		0xAD	/* Auto Address Increment Word Program */
#define CMD_SST_SE		    0x20	/* Sector Erase */

#define SST_SR_WIP		(1 << 0)	/* Write-in-Progress */
#define SST_SR_WEL		(1 << 1)	/* Write enable */
#define SST_SR_BP0		(1 << 2)	/* Block Protection 0 */
#define SST_SR_BP1		(1 << 3)	/* Block Protection 1 */
#define SST_SR_BP2		(1 << 4)	/* Block Protection 2 */
#define SST_SR_AAI		(1 << 6)	/* Addressing mode */
#define SST_SR_BPL		(1 << 7)	/* BP bits lock */

#define SST_FEAT_WP		(1 << 0)	/* Supports AAI word program */
#define SST_FEAT_MBP		(1 << 1)	/* Supports multibyte program */

struct sst_spi_flash_params 
{
    u8 idcode1;
    u8 flags;
    u16 nr_sectors;
    const char *name;
};


#define SST_SECTOR_SIZE (4 * 1024)
#define SST_PAGE_SIZE   256
static const struct sst_spi_flash_params sst_spi_flash_table[] = {
	{
		.idcode1 = 0x8d,
		.flags = SST_FEAT_WP,
		.nr_sectors = 128,
		.name = "SST25VF040B",
	},{
		.idcode1 = 0x8e,
		.flags = SST_FEAT_WP,
		.nr_sectors = 256,
		.name = "SST25VF080B",
	},{
		.idcode1 = 0x41,
		.flags = SST_FEAT_WP,
		.nr_sectors = 512,
		.name = "SST25VF016B",
	},{
		.idcode1 = 0x4a,
		.flags = SST_FEAT_WP,
		.nr_sectors = 1024,
		.name = "SST25VF032B",
	},{
		.idcode1 = 0x4b,
		.flags = SST_FEAT_MBP,
		.nr_sectors = 2048,
		.name = "SST25VF064C",
	},{
		.idcode1 = 0x01,
		.flags = SST_FEAT_WP,
		.nr_sectors = 16,
		.name = "SST25WF512",
	},{
		.idcode1 = 0x02,
		.flags = SST_FEAT_WP,
		.nr_sectors = 32,
		.name = "SST25WF010",
	},{
		.idcode1 = 0x03,
		.flags = SST_FEAT_WP,
		.nr_sectors = 64,
		.name = "SST25WF020",
	},{
		.idcode1 = 0x04,
		.flags = SST_FEAT_WP,
		.nr_sectors = 128,
		.name = "SST25WF040",
	},
};

static int sst_enable_writing(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;

	int ret = concerto_spi_cmd_write_enable(host->spi);
	if (ret)
		FLASH_DEBUG("SF: Enabling Write failed\n");
	return ret;
}

static int sst_disable_writing(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;

	int ret = concerto_spi_cmd_write_disable(host->spi);
	if (ret)
		FLASH_DEBUG("SF: Disabling Write failed\n");
	return ret;
}

static int sst_byte_write(struct mtd_info *mtd, u32 offset, const void *buf)
{
    struct concerto_sfc_host *host	= mtd->priv;

	int ret;
	u8 cmd[4] = {
		CMD_SST_BP,
		offset >> 16,
		offset >> 8,
		offset,
	};
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    u32 dma_sel = 1;
#else
    u32 dma_sel = 0;
#endif

	//FLASH_DEBUG("BP[%02x]: 0x%p => cmd = { 0x%02x 0x%06x }\n",
	//	spi_w8r8(flash->spi, CMD_SST_RDSR), buf, cmd[0], offset);

	ret = sst_enable_writing(mtd);
	if (ret)
		return ret;

	ret = concerto_spi_cmd_write(host->spi, cmd, sizeof(cmd), buf, 1, dma_sel);
	if (ret)
		return ret;

	return concerto_spi_cmd_wait_ready(host->spi, SPI_FLASH_PROG_TIMEOUT);
}

static int sst_write_wp(struct mtd_info *mtd, loff_t offset,
                            size_t len, size_t *retlen, const u_char *buf)

{
    struct concerto_sfc_host *host	= mtd->priv;

	size_t actual, cmd_len;
	int ret;
	u8 cmd[4];
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    u32 dma_sel = 1;
#else
    u32 dma_sel = 0;
#endif

	ret = spi_claim_bus(host->spi);
	if (ret) {
		FLASH_DEBUG("SF: Unable to claim SPI bus\n");
		return ret;
	}

	/* If the data is not word aligned, write out leading single byte */
	actual = offset % 2;
	if (actual) {
		ret = sst_byte_write(mtd, offset, buf);
		if (ret)
			goto done;
	}
	offset += actual;

	ret = sst_enable_writing(mtd);
	if (ret)
		goto done;

	cmd_len = 4;
	cmd[0] = host->write_cmd;
	cmd[1] = offset >> 16;
	cmd[2] = offset >> 8;
	cmd[3] = offset;

	for (; actual < len - 1; actual += 2) {
		//FLASH_DEBUG("WP[%02x]: 0x%p => cmd = { 0x%02x 0x%06x }\n",
		//     spi_w8r8(flash->spi, CMD_SST_RDSR), buf + actual, cmd[0],
		//     offset);

		ret = concerto_spi_cmd_write(host->spi, cmd, cmd_len,
		                          buf + actual, 2, dma_sel);
		if (ret) {
			FLASH_DEBUG("SF: sst word program failed\n");
			break;
		}

		ret = concerto_spi_cmd_wait_ready(host->spi, SPI_FLASH_PROG_TIMEOUT);
		if (ret)
			break;

		cmd_len = 1;
		offset += 2;
	}

	if (!ret)
		ret = sst_disable_writing(mtd);

	/* If there is a single trailing byte, write it out */
	if (!ret && actual != len)
		ret = sst_byte_write(mtd, offset, buf + actual);

 done:
	FLASH_DEBUG("SF: sst: program %s %zu bytes @ 0x%zx\n",
	      ret ? "failure" : "success", len, (size_t) offset - actual);

    *retlen = len;

	spi_release_bus(host->spi);
	return ret;
}

static int sst_unlock(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;

	int ret;
	u8 cmd, status;

	ret = sst_enable_writing(mtd);
	if (ret)
		return ret;

	cmd = CMD_SST_WRSR;
	status = 0;
	ret = concerto_spi_cmd_write(host->spi, &cmd, 1, &status, 1, 0);
	if (ret)
		FLASH_DEBUG("SF: Unable to set status byte\n");

	//FLASH_DEBUG("SF: sst: status = %x\n", spi_w8r8(flash->spi, CMD_SST_RDSR));

	return ret;
}

int spi_flash_probe_sst(struct mtd_info *mtd, u8 *idcode)
{
    struct concerto_sfc_host *host	= mtd->priv;

	const struct sst_spi_flash_params *params;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(sst_spi_flash_table); ++i) {
		params = &sst_spi_flash_table[i];
		if (params->idcode1 == idcode[2] || params->idcode1 == idcode[1])
			break;
	}

	if (i == ARRAY_SIZE(sst_spi_flash_table)) {
		FLASH_DEBUG("SF: Unsupported SST ID %02x\n", idcode[1]);
		return 1;
	}


    host->name = params->name;
    host->pagesize = SST_PAGE_SIZE;
    host->blocksize = SST_SECTOR_SIZE;
    host->chipsize = host->blocksize * params->nr_sectors;

    host->byte_mode = 3;
    host->erase_cmd = CMD_SST_SE;
    mtd->_erase = concerto_spi_nor_erase;
    
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
        host->fastr_cmd = CMD_SST_FAST_READ;
    }
    else
    {
        host->fastr_cmd = CMD_SST_READ;
    }

	mtd->_read = concerto_spi_nor_read;

    if(params->flags & SST_FEAT_WP)
    {
        host->write_cmd = CMD_SST_AAI_WP;
        mtd->_write = sst_write_wp;
    }
    else
    {
        host->write_cmd = CMD_SST_BP;
        mtd->_write = concerto_spi_nor_write;
    }

	/* Flash powers up read-only, so clear BP# bits */
	sst_unlock(mtd);

	return 0;
}
