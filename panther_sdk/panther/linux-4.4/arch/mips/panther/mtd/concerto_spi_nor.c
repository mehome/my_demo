/******************************************************************************/
/* Copyright (c) 2012 Montage Tech - All Rights Reserved                      */
/******************************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
#endif

#include <asm/bootinfo.h>

#include "concerto_spi_nor.h"
#include "erased_page.h"
#include <asm/mach-panther/bootinfo.h>
#include <asm/mach-panther/pdma.h>

#define CONCERTO_SPI_FLASH_NAME     "mt_sf"

#include <linux/mtd/partitions.h>

/*=============================================================================+
| Extern Function/Variables                                                    |
+=============================================================================*/
extern int otp_get_boot_type(void);
extern void spi_access_lock(void);
extern void spi_access_unlock(void);

//#define SPI_NOR_PREFETCH_TEST    /* you shall limit kernel to use 48Mbytes memory (by boot cmdline) to test pre-fetch feature */

#ifndef CONFIG_MTD_CMDLINE_PARTS

#define FIRMWARE_PART_INDEX    4
#define CDB_PART_INDEX         (FIRMWARE_PART_INDEX - 2)
#define CI_PART_INDEX          (FIRMWARE_PART_INDEX - 1)
#define KERNEL_PART_INDEX      (FIRMWARE_PART_INDEX + 1)
#define ROOTFS_PART_INDEX      (FIRMWARE_PART_INDEX + 2)
#define DUERCFG_PART_INDEX     (FIRMWARE_PART_INDEX + 3)
#define SF_MIN_BLOCK_SIZE     64 * 1024

extern u32 is_enable_secure_boot;

u32 is_upgrade;

#define VIRTUAL_ACCESS_BUF_SIZE (8 * 1024 * 1024) // 8M
static int support_virtual_access = 0;
static int enable_virtual_access = 0;
static uint64_t virtual_access_offset;
static uint64_t virtual_access_size;
static uint64_t virtual_access_actual_size;
static u8* virtual_access_buf = NULL;
static long ci_offset;

#if !defined(CONFIG_PANTHER_KERNEL_SIZE)
#define CONFIG_PANTHER_KERNEL_SIZE 2
#endif
#define DEFAULT_PANTHER_MTD_SIZE 0x1000000

#define SF_BTINT_START    0x00000000                           //0x00000000
#define SF_BTINT_LENTH    (SF_MIN_BLOCK_SIZE * 2)              //0x00020000

#define SF_BVAR_START     (SF_BTINT_START + SF_BTINT_LENTH)
#define SF_BVAR_LENTH     SF_MIN_BLOCK_SIZE

#define SF_CDB_START      (SF_BVAR_START + SF_BVAR_LENTH)
#define SF_CDB_LENTH      SF_MIN_BLOCK_SIZE

#define SF_COMBINED_START    (SF_CDB_START + SF_CDB_LENTH)
#define SF_COMBINED_LENTH    (SF_MIN_BLOCK_SIZE * 2)

#define SF_DUERCFG_START    (DEFAULT_PANTHER_MTD_SIZE - SF_MIN_BLOCK_SIZE )
#define SF_DUERCFG_LENTH    SF_MIN_BLOCK_SIZE * 2

/*
|<-------------- whole firmware partition --------------->|
|<---- linux partition ----->|<---- rootfs partition ---->|
*/
#define SF_LINUX_START    (SF_COMBINED_START + SF_COMBINED_LENTH)
#define SF_LINUX_LENTH    (CONFIG_PANTHER_KERNEL_SIZE * 0x100000)

#define SF_ROOTFS_START   (SF_LINUX_START + SF_LINUX_LENTH)
#define SF_ROOTFS_LENTH   (DEFAULT_PANTHER_MTD_SIZE - SF_ROOTFS_START - SF_DUERCFG_LENTH )

#define SF_FIRMWARE_START  SF_LINUX_START
#define SF_FIRMWARE_LENTH  (DEFAULT_PANTHER_MTD_SIZE - SF_LINUX_START - SF_DUERCFG_LENTH )

static struct mtd_partition concerto_spi_nor_partitions[] = 
{
        {
            .name       = "boot_init",
            .size       = SF_BTINT_LENTH,
            .offset     = SF_BTINT_START,
        },
        {
            .name       = "boot_var",
            .size       = SF_BVAR_LENTH,
            .offset     = SF_BVAR_START,
        },
        {
            .name       = "cdb",
            .size       = SF_CDB_LENTH,
            .offset     = SF_CDB_START,
        },
        {
            .name       = "combined_image_header",
            .size       = SF_COMBINED_LENTH,
            .offset     = SF_COMBINED_START,
        },
        {
            .name       = "firmware",
            .size       = SF_FIRMWARE_LENTH,
            .offset     = SF_FIRMWARE_START,
        },
        {
            .name       = "kernel",
            .size       = SF_LINUX_LENTH,
            .offset     = SF_LINUX_START,
        },
        {
            .name       = "rootfs",
            .size       = SF_ROOTFS_LENTH,
            .offset     = SF_ROOTFS_START,
        },
        {
            .name       = "duercfg",
            .size       = SF_DUERCFG_LENTH,
            .offset     = SF_DUERCFG_START,
        },
};

#endif

#ifdef CONFIG_SYSCTL
static int panther_spi_nor_start_virtul_access(void);
static int proc_enable_virtual_access(struct ctl_table *table, int write,
		     void __user *buffer, size_t *lenp, loff_t *ppos);
static struct ctl_table spi_nor_sysctl_table[] = {
        {
            .procname       = "flash.nor.support_virtual_access",
            .data       = &support_virtual_access,
            .maxlen     = sizeof(int),
            .mode       = 0444,
            .proc_handler   = proc_dointvec
        },
        {
            .procname       = "flash.nor.enable_virtual_access",
            .data       = &enable_virtual_access,
            .maxlen     = sizeof(int),
            .mode       = 0644,
            .proc_handler   = proc_enable_virtual_access
        },
        {
        }
};

static int proc_enable_virtual_access(struct ctl_table *table, int write,
		     void __user *buffer, size_t *lenp, loff_t *ppos)
{
    if(support_virtual_access && write)
    {
        panther_spi_nor_start_virtul_access();
    }

    return proc_dointvec(table, write, buffer, lenp, ppos);
}
#endif

#if 0

void flash_printData(char * title, u8* buf, u32 size)
{
    u32 i;

    if(buf == NULL || size == 0)
    {
        return;
    }

    if(title != NULL)
    {
        FLASH_DEBUG("%s:\n",title);
    }
    for(i=0; i<size; i++)
    {
        FLASH_DEBUG("0x%02x ", buf[i]);
        if(i % 16 == 15)
        {
            FLASH_DEBUG("\n");
        }
    }
    FLASH_DEBUG("\n");
}

void printRegister(void)
{
    u32 i;

    for(i = 0; i < 0x50; i = i + 4)
    {
        FLASH_DEBUG("0x%08x: 0x%08x\n", (0xBF010000 + i), REG32((0xBF010000 + i)));
    }
    FLASH_DEBUG("\n");
}
#endif

static int concerto_spi_nor_getJedecID(struct mtd_info *mtd, u8 *idcode, u32 idlen)
{
	struct concerto_sfc_host *host	= mtd->priv;

    u8 cmd[4];

    cmd[0] = SPI_FLASH_CMD_RD_MAN_ID;
    
    return concerto_spi_read_common(host->spi, cmd, 1, idcode, idlen);
}

int concerto_spi_nor_cmd_erase(struct mtd_info *mtd, u32 offset)
{
    struct concerto_sfc_host *host = mtd->priv;

    int ret;
    u8 cmd[5];

    cmd[0] = host->erase_cmd;

    if(host->byte_mode == 4)
    {
        cmd[1] = offset >> 24;
        cmd[2] = offset >> 16;
        cmd[3] = offset >> 8;
        cmd[4] = offset >> 0;
    }
    else
    {
        cmd[1] = offset >> 16;
        cmd[2] = offset >> 8;
        cmd[3] = offset >> 0;
    }
    
    ret = concerto_spi_cmd_write_enable(host->spi);
    if (ret)
    {
        goto out;
    }

    if(host->byte_mode == 4)
    {
        ret = concerto_spi_cmd_write(host->spi, cmd, 5, NULL, 0, 0);
    }
    else
    {
        ret = concerto_spi_cmd_write(host->spi, cmd, 4, NULL, 0, 0);
    }
    if (ret)
    {
        goto out;
    }

    ret = concerto_spi_cmd_wait_ready(host->spi, SPI_FLASH_PAGE_ERASE_TIMEOUT);
    if (ret)
    {
        goto out;
    }

    ret = concerto_spi_cmd_write_disable(host->spi);
        
    //FLASH_DEBUG("SF DRV: Successfully erased %zu bytes @ %#x\n", mtd->erasesize, offset);
    
out:
    return ret;
}

int panther_spi_nor_read(struct concerto_sfc_host *host, loff_t from,
                            size_t len, size_t *retlen, u_char *buf, u32 dma_sel)
{
    u8 cmd[6];
    u32 data_len;
    int ret = ERR_FAILURE;
    u32 offset = 0;

    cmd[0] = host->fastr_cmd;    
    while (len > 0)
    {
        if (len >= MAX_NOR_BUFFER_SIZE)
        {
            data_len = MAX_NOR_BUFFER_SIZE;
        }
        else
        {
            data_len = len;
        }
    
        if (host->byte_mode == 4)
        {
            cmd[1] = from >> 24;
            cmd[2] = from >> 16;
            cmd[3] = from >> 8;
            cmd[4] = from >> 0;
            cmd[5] = 0x00;
            ret = concerto_spi_cmd_read(host->spi, cmd, 6, host->buffer, data_len, NULL, dma_sel);
        }
        else
        {
            cmd[1] = from >> 16;
            cmd[2] = from >> 8;
            cmd[3] = from >> 0;
            cmd[4] = 0x00;
            ret = concerto_spi_cmd_read(host->spi, cmd, 5, host->buffer, data_len, NULL, dma_sel);
        }

        if (ret != SUCCESS)
        {
            return ret;
        }
        
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
        memcpy(buf + offset, (void*)UNCACHED_ADDR(host->buffer), data_len);
#else
        memcpy(buf + offset, (void*)host->buffer, data_len);
#endif

        len -= data_len;
        offset += data_len;
        from += data_len;
    }

    return ret;
}

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
void decrypt_empty_page(u8 *buf)
{
    if (is_enable_secure_boot)
    {
        // if in secure boot, need to check whether this page had been erased or not
        panther_spi_restore_erased_page(buf, MAX_NOR_BUFFER_SIZE);
    }
}
#endif

#if defined(SPI_NOR_PREFETCH_TEST)
static uint64_t prefetch_offset;
static uint64_t prefetch_size;
static u8* prefetch_buf = (u8 *)0x83000000UL;
int concerto_spi_nor_prefetch_read(struct mtd_info *mtd, loff_t from, size_t len,
		                    size_t *retlen, u_char *buf)
{
    if((from >= prefetch_offset) && ((from + len) < (prefetch_offset + prefetch_size)))
    {
        memcpy(buf, &prefetch_buf[from - prefetch_offset], len);
        if(retlen)
            *retlen = len;

        return SUCCESS;
    }

    return -1;
}
#endif

int concerto_spi_nor_virtual_access_read(struct mtd_info *mtd, loff_t from, size_t len,
		                    size_t *retlen, u_char *buf)
{
    if((from >= virtual_access_offset) && ((from + len - 1) < (virtual_access_offset + virtual_access_size)))
    {
        if((from + len - 1) < (virtual_access_offset + virtual_access_actual_size))
        {
            memcpy(buf, &virtual_access_buf[from - virtual_access_offset], len);
        }
        else
        {
            memset(buf, 0xff, len);
        }
        if(retlen)
            *retlen = len;

        return SUCCESS;
    }

    return -1;
}

int concerto_spi_nor_read(struct mtd_info *mtd, loff_t from, size_t len,
		                    size_t *retlen, u_char *buf)
{
    struct concerto_sfc_host *host	= mtd->priv;
    int ret = SUCCESS;
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    u32 dma_sel = 1;
#else
    u32 dma_sel = 0;
#endif
    u32 inner_len;
    u32 start_offset;
//  printk("read from = 0x%x, len = 0x%x, buf = 0x%x\n", (u32) from, len, (u32) buf);

#if defined(SPI_NOR_PREFETCH_TEST)
    ret = concerto_spi_nor_prefetch_read(mtd, from, len, retlen, buf);
    if(ret==SUCCESS)
        return ret;
#endif

    spi_access_lock();

    if(enable_virtual_access && (0 != strcmp(current->comm, "mtd")))
    {
        if(0 == concerto_spi_nor_virtual_access_read(mtd, from, len, retlen, buf))
        {
            spi_access_unlock();
            return ret;
        }
    }

    if(host->die_select)
    {
        ret = host->die_select(mtd, &from);
        if (ret)
            goto out;
    }

    *retlen = len;
    concerto_spi_set_trans_ionum(host->spi, host->read_ionum);
#if 0
    ret = concerto_spi_cmd_wait_ready(host->spi, SPI_FLASH_PROG_TIMEOUT);
    if (ret)
    {
        goto out;
    }
#endif
    // first read for align with page size
    start_offset = (from % MAX_NOR_BUFFER_SIZE);
    if (start_offset != 0)
    {
        inner_len = (len < (MAX_NOR_BUFFER_SIZE - start_offset)) ? len : (MAX_NOR_BUFFER_SIZE - start_offset);
        ret = panther_spi_nor_read(
                host, (from - start_offset), MAX_NOR_BUFFER_SIZE, retlen, host->buffer, dma_sel);

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
        decrypt_empty_page((u8 *)(host->buffer));
#endif
        memcpy(buf, (u8 *)(host->buffer + start_offset), inner_len);

        len -= inner_len;
        from += inner_len;
        buf += inner_len;
    }

    while (len > 0)
    {
        inner_len = (len > MAX_NOR_BUFFER_SIZE) ? MAX_NOR_BUFFER_SIZE : len;

        ret = panther_spi_nor_read(host, from, MAX_NOR_BUFFER_SIZE, retlen, host->buffer, dma_sel);
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
        decrypt_empty_page(host->buffer);
#endif
        memcpy(buf, host->buffer, inner_len);

        len -= inner_len;
        from += inner_len;
        buf += inner_len;
    }

out:
    concerto_spi_set_trans_ionum(host->spi, 1);

    // tansform local error code to system error code
    if (ret)
    {
        FLASH_DEBUG("SF DRV ERROR: read failed\n");
        ret = -EIO;
    }

    spi_access_unlock();
    return ret;
}

int concerto_spi_nor_virtual_access_write(struct mtd_info *mtd, loff_t from, size_t len,
		                    size_t *retlen, const u_char *buf)
{
    if((from >= virtual_access_offset) && ((from + len - 1) < (virtual_access_offset + virtual_access_size)))
    {
        if((from + len - 1) < (virtual_access_offset + virtual_access_actual_size))
        {
            memcpy(&virtual_access_buf[from - virtual_access_offset], buf, len);
        }
        if(retlen)
            *retlen = len;

        return SUCCESS;
    }

    return -1;
}

int concerto_spi_nor_write(struct mtd_info *mtd, loff_t offset,
                                        size_t len, size_t *retlen, const u_char *buf)
{
    struct concerto_sfc_host *host	= mtd->priv;
    int page_addr, byte_addr;
	size_t chunk_len, actual;
	int ret = SUCCESS;
	u8 cmd[5];
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    u32 dma_sel = 1;
#else
    u32 dma_sel = 0;
#endif

    spi_access_lock();

    if(enable_virtual_access && (0 != strcmp(current->comm, "mtd")))
    {
        if(0 == concerto_spi_nor_virtual_access_write(mtd, offset, len, retlen, buf))
        {
            spi_access_unlock();
            return ret;
        }
    }

    if(host->die_select)
    {
        ret = host->die_select(mtd, &offset);
        if (ret)
            goto out;
    }

	page_addr = (int)(offset >> host->page_shift);
	byte_addr = (int)(offset & ((1 << host->page_shift) - 1));
    if (is_upgrade)
    {
        printk("write page = 0x%x, len = 0x%x, buf = 0x%x\n", (u32) page_addr, len, (u32) buf);
    }
    *retlen = 0;

	cmd[0] = host->write_cmd;
	for (actual = 0; actual < len; actual += chunk_len) 
    {
		chunk_len = MIN(len - actual, host->pagesize - byte_addr);

        if(host->byte_mode == 4)
        {
            cmd[1] = page_addr >> 16;
            cmd[2] = page_addr >> 8;
            cmd[3] = page_addr;
            cmd[4] = byte_addr;
        }
        else
        {
            cmd[1] = page_addr >> 8;
            cmd[2] = page_addr;
            cmd[3] = byte_addr;
        }

		ret = concerto_spi_cmd_write_enable(host->spi);
		if (ret) 
        {
			FLASH_DEBUG("SF DRV ERROR: enabling write failed!!!\n");
			goto out;
		}

        concerto_spi_set_trans_ionum(host->spi, host->write_ionum);

        memset(host->buffer, 0xff, MAX_NOR_BUFFER_SIZE);
        memcpy(host->buffer, buf + actual, chunk_len);

        if(host->byte_mode == 4)
        {
            ret = concerto_spi_cmd_write(host->spi, cmd, 5, host->buffer, MAX_NOR_BUFFER_SIZE, dma_sel);
        }
        else
        {
            ret = concerto_spi_cmd_write(host->spi, cmd, 4, host->buffer, MAX_NOR_BUFFER_SIZE, dma_sel);
        }

        concerto_spi_set_trans_ionum(host->spi, 1);
        
		if (ret) 
        {
			goto out;
		}

		ret = concerto_spi_cmd_wait_ready(host->spi, SPI_FLASH_PROG_TIMEOUT);
		if (ret)
        {
			goto out;
        }

		page_addr++;
		byte_addr = 0;

        *retlen += chunk_len;

        ret = concerto_spi_cmd_write_disable(host->spi);
        if (ret)
        {
            goto out;
        }
	}
    
	//FLASH_DEBUG("SF DRV: program %s %zu bytes @ %#x\n", ret ? "failure" : "success", len, (unsigned int)offset);

out:
    // tansform local error code to system error code
    if (ret) 
    {
        FLASH_DEBUG("SF DRV ERROR: write failed\n");
        ret = -EIO;
    }

    spi_access_unlock();
    return ret;
}

int concerto_spi_nor_virtual_access_erase(struct mtd_info *mtd, loff_t from, size_t len)
{
    if((from >= virtual_access_offset) && ((from + len - 1) < (virtual_access_offset + virtual_access_size)))
    {
        if((from + len - 1) < (virtual_access_offset + virtual_access_actual_size))
        {
            memset(&virtual_access_buf[from - virtual_access_offset], 0xff, len);
        }

        return SUCCESS;
    }

    return -1;
}

int concerto_spi_nor_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct concerto_sfc_host *host	= mtd->priv;
    int ret = SUCCESS;
    loff_t len;
    struct mtd_partition *ci_part = NULL;

    ci_part = &concerto_spi_nor_partitions[FIRMWARE_PART_INDEX - 1];
    // if sysupgrade wnat to erase combined_image_header partition,
    // disable flag is_enable_secure_boot
    if (is_enable_secure_boot
        && (instr->addr >= ci_part->offset)
        && (instr->addr < (ci_part->offset + ci_part->size)))
    {
        is_enable_secure_boot = 0;
        is_upgrade = 1;
    }

    /* Start address must align on block boundary */
    if (instr->addr & ((1 << host->erase_shift) - 1)) 
    {
        FLASH_DEBUG("concerto_spi_erase: Unaligned address\n");
        return -EINVAL;
    }

    /* Length must align on block boundary */
    if (instr->len & ((1 << host->erase_shift) - 1)) 
    {
        FLASH_DEBUG("concerto_spi_erase: "
            "Length not block aligned\n");
        return -EINVAL;
    }

    /* Do not allow erase past end of device */
    if ((instr->len + instr->addr) > mtd->size) 
    {
        FLASH_DEBUG("concerto_spi_erase: "
            "Erase past end of device\n");
        return -EINVAL;
    }

    spi_access_lock();

    len = instr->len;

    instr->state = MTD_ERASING;

    if(enable_virtual_access && (0 != strcmp(current->comm, "mtd")))
    {
        if(0 == concerto_spi_nor_virtual_access_erase(mtd, instr->addr, len))
        {
            goto erase_done;
        }
    }

    if(host->die_select)
    {
        ret = host->die_select(mtd, &instr->addr);
        if (ret)
            goto erase_exit;
    }

    while (len) 
    {
        if (is_upgrade)
        {
            printk("erase addr = 0x%llx, blockno = %d\n", instr->addr, (int)(instr->addr >> host->erase_shift));
        }

        ret = concerto_spi_nor_cmd_erase(mtd, instr->addr);
        if (ret)
        {
            goto erase_exit;
        }

        len -= mtd->erasesize;
        instr->addr += mtd->erasesize;
    }
erase_done:
    instr->state = MTD_ERASE_DONE;

erase_exit:

    // tansform local error code to system error code
    ret = instr->state == MTD_ERASE_DONE ? 0 : -EBADMSG;

    /* Do call back function */
    if (!ret)
    {
        mtd_erase_callback(instr);
    }
    else
    {
        FLASH_DEBUG("SF DRV ERROR: erase failed\n");
    }

    spi_access_unlock();
    return ret;
}




const spi_flash_probe_t spi_nor_flashes[] = 
{
#if 0
    /* Keep it sorted by define name */
    { 0, 0x1f, spi_flash_probe_atmel, },
    { 0, 0x20, spi_flash_probe_stmicro, },
#endif
    { 0, 0xbf, spi_flash_probe_sst, },
	{ 0, 0x8c, spi_flash_probe_esmt, },
    { 0, 0xc2, spi_flash_probe_macronix, },
	{ 0, 0xc8, spi_flash_probe_gigadevice, },
	{ 0, 0x9d, spi_flash_probe_issi, },
    { 0, 0x1c, spi_flash_probe_eon, },
    { 0, 0x01, spi_flash_probe_spansion, },
    { 0, 0xef, spi_flash_probe_winbond, },
    { 0, 0xf8, spi_flash_probe_dosilicon, },
};

#if 0
static void concerto_spi_set_pinmux(void)
{
    REG32(0xbf156008) = 0;
    return;
}
#endif

static int __init get_ci_offset(char *p)
{
    //get ci_offset first
    ci_offset = simple_strtol(p, NULL, 16);

    //set the partition offet
    if(ci_offset != 0)
    {
        concerto_spi_nor_partitions[CI_PART_INDEX].offset = ci_offset;
        concerto_spi_nor_partitions[CDB_PART_INDEX].offset = ci_offset - SF_CDB_LENTH;
        concerto_spi_nor_partitions[FIRMWARE_PART_INDEX].offset = ci_offset + SF_COMBINED_LENTH;
        concerto_spi_nor_partitions[KERNEL_PART_INDEX].offset = concerto_spi_nor_partitions[FIRMWARE_PART_INDEX].offset;
        concerto_spi_nor_partitions[ROOTFS_PART_INDEX].offset = concerto_spi_nor_partitions[KERNEL_PART_INDEX].offset + SF_LINUX_LENTH;
	concerto_spi_nor_partitions[DUERCFG_PART_INDEX].offset = concerto_spi_nor_partitions[FIRMWARE_PART_INDEX].offset + concerto_spi_nor_partitions[FIRMWARE_PART_INDEX].size;
    }
    else
	{
		printk("!!!!ci_offset = %d\n", ci_offset);
	}

	return 0;
}
early_param("ci_offset", get_ci_offset);

static int concerto_spi_nor_init(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;

    //concerto_spi_set_pinmux();

    host->buffer = kmalloc(MAX_NOR_BUFFER_SIZE, GFP_KERNEL);
	if (!host->buffer)
    {
		return 1;
    }
	memset(host->buffer, 0xff, MAX_NOR_BUFFER_SIZE);
    
    host->controller = &host->hwcontrol;

    host->read_ionum = 4;
    host->write_ionum = 4;

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    spi_panther_init_secure();
#endif

    return 0;
}

static int concerto_spi_flash_probe(struct mtd_info *mtd, unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int spi_mode)
{
    struct concerto_sfc_host *host	= mtd->priv;

    struct spi_slave *spi;
	int ret, i;
	u8 idcode[IDCODE_LEN];

	spi = spi_setup_slave(bus, cs, max_hz, spi_mode);

	if (!spi) {
		FLASH_DEBUG("SF DRV: Failed to set up slave\n");
		return 1;
	}

	ret = spi_claim_bus(spi);
	if (ret) 
    {
		FLASH_DEBUG("SF DRV: Failed to claim SPI bus: %d\n", ret);
		goto err_claim_bus;
	}

    host->spi = spi;

	/* Read the ID codes */
    ret = concerto_spi_nor_getJedecID(mtd, idcode, sizeof(idcode));
	if (ret)
    {
		goto err_read_id;
    }

    //flash_printData("Got idcodes", idcode, sizeof(idcode));

    ret = 1;

	/* search the table for matches in shift and id */
	for (i = 0; i < ARRAY_SIZE(spi_nor_flashes); ++i)
    {
        // to support dual-endian, check LSB and MSB
		if (spi_nor_flashes[i].idcode == idcode[0]
            || spi_nor_flashes[i].idcode == idcode[3]) 
        {
			/* we have a match, call probe */
			ret = spi_nor_flashes[i].probe(mtd, idcode);
			if (!ret)
            {
				break;
            }
		}
    }

	if (ret)
    {
		FLASH_DEBUG("SF DRV ERROR: Unsupported manufacturer %02x\n", idcode[0]);
		goto err_manufacturer_probe;
	}


	spi_release_bus(host->spi);

	return 0;

err_manufacturer_probe:
err_read_id:
	spi_release_bus(host->spi);
err_claim_bus:
	spi_free_slave(host->spi);
	return 1;
}

static int concerto_spi_nor_init_qe_status(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host = mtd->priv;
    int ret = 0;
    u8 value = 0;
    u8 cmd[1];

    ret = concerto_spi_cmd_write_enable(host->spi);
	if (ret < 0) 
    {
		FLASH_DEBUG("SF DRV ERROR: enabling write failed!!!\n");
		goto err_init_qe;
	}
	
    cmd[0] = host->read_reg2_cmd;
	
    concerto_spi_read_common(host->spi, cmd, 1, &value, 1);

    // if use quad speed to IO, need to enable QE bit
    if (host->read_ionum == 4 || host->write_ionum == 4)
    {
        value |= (1 << 1);
    }
    else
    {
        value &= ~(1 << 1);
    }
	
    cmd[0] = host->write_reg2_cmd;

    // for support big & little endian
    concerto_spi_cmd_write(host->spi, cmd, 1, &value, 1, 0);
	
    ret = concerto_spi_cmd_wait_ready(host->spi, SPI_FLASH_PROG_TIMEOUT);
	if (ret)
    {
		goto err_init_qe;
    }

    concerto_spi_cmd_write_disable(host->spi);
    
    return 0;

err_init_qe:
    return 1;
}

static void panther_spi_nor_adapt_combined_partition(struct mtd_info *mtd)
{
    struct mtd_partition *parts = NULL;
    struct mtd_partition *ci_part = NULL;
    uint64_t offset = 0;
    u8 hdr_buf[0x40];
    u16 ci_magic;
    int i = 0;
    size_t retlen;

    spi_access_lock();

    ci_part = &concerto_spi_nor_partitions[FIRMWARE_PART_INDEX - 1];

    change_aes_enable_status(DISABLE_SECURE);
    concerto_spi_nor_read(mtd, ci_part->offset, 0x40, &retlen, hdr_buf);
    change_aes_enable_status(ENABLE_SECURE_OTP);
    ci_magic = cpu_to_be16(*((u16 *)hdr_buf));
    FLASH_DEBUG("ci_magic = 0x%x\n", ci_magic);
    if (ci_magic == CI_MAGIC)
    {
        spi_access_unlock();
        return;
    }

    FLASH_DEBUG("firmware type is not combined image, adjust partitions\n");
    for (i = FIRMWARE_PART_INDEX; i < ARRAY_SIZE(concerto_spi_nor_partitions); i++)
    {
        parts = &concerto_spi_nor_partitions[i];

        if (i == FIRMWARE_PART_INDEX)
        {
            parts->offset = offset = ci_part->offset;
            continue;
        }
				FLASH_DEBUG("%d : offset = 0x%08llx\n", i, offset);
        parts->offset = offset;
        offset = (parts->offset + parts->size);
    }

    FLASH_DEBUG("after function: %s\n", __FUNCTION__);
    for (i = 0; i < ARRAY_SIZE(concerto_spi_nor_partitions); i++)
    {
        parts = &concerto_spi_nor_partitions[i];
        FLASH_DEBUG("%d:parts->name = %s\n", i, parts->name);
        FLASH_DEBUG("%d:parts->offset = 0x%08llx\n", i, parts->offset);
        FLASH_DEBUG("%d:parts->size = 0x%08llx\n", i, parts->size);
    }

    spi_access_unlock();
}

#ifdef CONFIG_MTD_SPLIT_UIMAGE_FW
static void panther_spi_nor_adapt_kernel_partition(struct mtd_info *mtd)
{
    struct mtd_partition *parts = NULL;
    u32 block_size = (SF_MIN_BLOCK_SIZE * 2);   // use 128k for match 128k align when image was built
    u32 img_size;
    uint64_t offset = 0;
    char *init_info;
    int i;

    // get kernel image size first
    init_info = strstr(arcs_cmdline, "img_size=");
    if (init_info == NULL)
    {
        FLASH_DEBUG("img_size = NULL, function: %s return!!\n", __FUNCTION__);
        return;
    }
    sscanf(&init_info[9], "%08x", &img_size);

    for (i = (FIRMWARE_PART_INDEX + 1); i < ARRAY_SIZE(concerto_spi_nor_partitions); i++)
    {
        parts = &concerto_spi_nor_partitions[i];

        if (i == (FIRMWARE_PART_INDEX + 1))
        {
            img_size = ((img_size + block_size - 1) / block_size) * block_size;
            FLASH_DEBUG("image size = 0x%x\n", img_size);
            parts->size = (uint64_t)(img_size);
        }
        else
        {
            parts->offset = offset;
        }

        offset = (parts->offset + parts->size);
    }

		parts = &concerto_spi_nor_partitions[DUERCFG_PART_INDEX];
    parts->offset = (mtd->size - parts->size);
    
    parts = &concerto_spi_nor_partitions[ROOTFS_PART_INDEX];
    parts->size = (mtd->size - parts->offset - SF_DUERCFG_LENTH);
    parts = &concerto_spi_nor_partitions[FIRMWARE_PART_INDEX];
    parts->size = (mtd->size - parts->offset - SF_DUERCFG_LENTH);
    
    

    FLASH_DEBUG("after function: %s\n", __FUNCTION__);
    for (i = 0; i < ARRAY_SIZE(concerto_spi_nor_partitions); i++)
    {
        parts = &concerto_spi_nor_partitions[i];
        FLASH_DEBUG("%d parts->name = %s\n", i,parts->name);
        FLASH_DEBUG("%d parts->offset = 0x%08llx\n", i,parts->offset);
        FLASH_DEBUG("%d parts->size = 0x%08llx\n", i,parts->size);
    }
}
#endif

static void panther_spi_nor_rename_rootfs(struct mtd_info *mtd)
{
    struct mtd_partition *rootfs_part = NULL;
    char *init_info;

    rootfs_part = &concerto_spi_nor_partitions[FIRMWARE_PART_INDEX + 2];

    init_info = strstr(arcs_cmdline, "ubi.mtd=");
    if (init_info != NULL)
    {
        rootfs_part->name = "ubi";
        FLASH_DEBUG("%s: Rename rootfs to ubi\n", __FUNCTION__);
    }
}

static void panther_spi_nor_align_rootfs_data(struct mtd_info *mtd)
{
    struct concerto_sfc_host *host	= mtd->priv;
    struct mtd_partition *parts = NULL;
    struct mtd_partition *rootfs_part = NULL;
    int i = 0;

    rootfs_part = &concerto_spi_nor_partitions[FIRMWARE_PART_INDEX + 2];

    // align to block size
    if ((rootfs_part->offset >> host->erase_shift)%2 != 0)
    {
        rootfs_part->offset += SF_MIN_BLOCK_SIZE;
        rootfs_part->size -= SF_MIN_BLOCK_SIZE;
    }

    FLASH_DEBUG("after function: %s\n", __FUNCTION__);
    for (i = 0; i < ARRAY_SIZE(concerto_spi_nor_partitions); i++)
    {
        parts = &concerto_spi_nor_partitions[i];
        FLASH_DEBUG("%d,parts->name = %s\n", i,parts->name);
        FLASH_DEBUG("%d,parts->offset = 0x%08llx\n", i,parts->offset);
        FLASH_DEBUG("%d,parts->size = 0x%08llx\n", i,parts->size);
    }
}

static struct mtd_info *mtd_global = NULL;
static int panther_spi_nor_start_virtul_access(void)
{
    struct mtd_info *mtd = mtd_global;
    struct mtd_partition *ci_part = NULL;
    uint64_t size, actual_size, offset;
    u8 *buf;
    size_t retlen;

    ci_part = &concerto_spi_nor_partitions[6];

    size = ci_part->size;
    offset = ci_part->offset;

    printk("panther_spi_nor_start_virtul_access 0x%08llx@0x%08llx\n", ci_part->size, ci_part->offset);

    if(virtual_access_buf)
    {
        return 0;
    }

    if(size > VIRTUAL_ACCESS_BUF_SIZE)
    {
        size = VIRTUAL_ACCESS_BUF_SIZE;
    }
    actual_size = size;

    buf = vmalloc(actual_size);
    if(!buf)
    {
        printk(KERN_EMERG "panther_spi_nor_start_virtul_access vmalloc failed\n");
        return 0;
    }

    virtual_access_buf = buf;

    spi_access_lock();

    while(size > 0)
    {
        concerto_spi_nor_read(mtd, offset, MAX_NOR_BUFFER_SIZE, &retlen, buf);
        size -= MAX_NOR_BUFFER_SIZE;
        offset += MAX_NOR_BUFFER_SIZE;
        buf += MAX_NOR_BUFFER_SIZE;
    }

    virtual_access_actual_size = actual_size;
    virtual_access_size = ci_part->size;
    virtual_access_offset = ci_part->offset;

    spi_access_unlock();

    printk("panther_spi_nor_start_virtul_access done\n");

    return 0;
}
#if defined(SPI_NOR_PREFETCH_TEST)
static int panther_spi_nor_prefetch(void *arg)
{
    struct mtd_info	*mtd = (struct mtd_info	*) arg;
    struct mtd_partition *ci_part = NULL;
    uint64_t size, offset;
    u8 *buf;
    size_t retlen;

    ci_part = &concerto_spi_nor_partitions[6];

    size = ci_part->size;
    offset = ci_part->offset;
    buf = prefetch_buf;

    printk(KERN_EMERG "panther_spi_nor_prefetch 0x%08llx@0x%08llx\n", ci_part->size, ci_part->offset);

    spi_access_lock();

    while(size > 0)
    {
        concerto_spi_nor_read(mtd, offset, MAX_NOR_BUFFER_SIZE, &retlen, buf);
        size -= MAX_NOR_BUFFER_SIZE;
        offset += MAX_NOR_BUFFER_SIZE;
        buf += MAX_NOR_BUFFER_SIZE;
    }

    prefetch_size = ci_part->size;
    prefetch_offset = ci_part->offset;

    spi_access_unlock();

    printk(KERN_EMERG "panther_spi_nor_prefetch done\n");
    return 0;
}
#endif

static int concerto_spi_nor_probe(struct platform_device * pltdev)
{
	int	result = 0;
    u32 boot_type;

	struct concerto_sfc_host *host;
	struct mtd_info	  *mtd;
	struct mtd_part_parser_data ppdata = {};

	int	size = sizeof(struct concerto_sfc_host)	+ sizeof(struct	mtd_info);

#ifdef CONFIG_MTD_CMDLINE_PARTS
    int nr_parts = 0;
    struct mtd_partition *parts = NULL;

    static const char *part_probes[] = {"cmdlinepart", NULL,};
#endif

    // check boot from NAND or NOR
    boot_type = otp_get_boot_type();
    if ((boot_type==BOOT_FROM_NAND) || (boot_type==BOOT_FROM_NAND_WITH_OTP))
        return -1;

    FLASH_DEBUG("concerto_spi_init start.\n");
    
	host = kmalloc(size, GFP_KERNEL);
	if (!host)
	{
		dev_err(&pltdev->dev, "failed to allocate device structure.\n");
		return -ENOMEM;
	}

	memset((char *)host, 0,	size);
	platform_set_drvdata(pltdev, host);

	host->dev  = &pltdev->dev;
	host->mtd  = mtd  =	(struct	mtd_info *)&host[1];


	mtd->priv  = host;
	mtd->owner = THIS_MODULE;
	mtd->name  = (char*)(pltdev->name);
	mtd->dev.parent = &pltdev->dev;

    load_erased_page_data_from_boot_cmdline();

    if (concerto_spi_nor_init(mtd))
    {
        dev_err(&pltdev->dev, "failed to spi nor init.\n");
        result = -EIO;
        goto err;
    }

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    pdma_init();
    init_spi_data();
#endif

    if (concerto_spi_flash_probe(mtd, CONFIG_SF_DEFAULT_BUS, CONFIG_SF_DEFAULT_CS, 
                                CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE))
    {
		dev_err(&pltdev->dev, "failed to spi probe.\n");
		result = -ENXIO;
		goto err;
    }

    if (concerto_spi_nor_init_qe_status(mtd))
    {
        dev_err(&pltdev->dev, "failed to initial qe status.\n");
        result = -ENXIO;
		goto err;
    }

    host->page_shift = ffs(host->pagesize) - 1;
    host->erase_shift = ffs(host->blocksize) - 1;
    if (host->chipsize & 0xffffffff)
    {
        host->chip_shift = ffs((unsigned)host->chipsize) - 1;
    }
    else
    {
        host->chip_shift = ffs((unsigned)(host->chipsize >> 32)) + 32 - 1;
    }

    mtd->erasesize = host->blocksize;
    mtd->size = host->chipsize;
    mtd->writesize = 256;
    mtd->writebufsize = 2048;  // added for UBI/UBIFS back-port, not tested yet!

    FLASH_DEBUG("SF DRV: Detected %s with page size: %d, total %d M bytes\n", 
        host->name, host->pagesize, (u32)(host->chipsize >> 20));

	host->state = FL_READY;

	mtd->type = MTD_NORFLASH;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->owner = THIS_MODULE;
 	mtd->_point = NULL;
 	mtd->_unpoint = NULL;
	mtd->_sync = NULL;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_suspend = NULL;
	mtd->_resume = NULL;

    mtd->name = CONCERTO_SPI_FLASH_NAME;

#ifdef CONFIG_MTD_CMDLINE_PARTS
    nr_parts = parse_mtd_partitions(mtd, part_probes, &parts, 0);

    if (!nr_parts)
    {
        FLASH_DEBUG(("cmdlinepart has no any partitions!!!\n"));
        return -1;
    }

    if (nr_parts > 0) 
    {
        result = add_mtd_partitions(mtd, parts, nr_parts);
    }
#else
    // Don't resize in recovery mode.
#ifndef CONFIG_RAMFS_RECOVERY_IMAGE
    panther_spi_nor_adapt_combined_partition(mtd);   // about combined image partition
#endif
#ifdef CONFIG_MTD_SPLIT_UIMAGE_FW
    panther_spi_nor_adapt_kernel_partition(mtd);     // about kernel real size
#endif
    panther_spi_nor_rename_rootfs(mtd);
    ppdata.of_node = pltdev->dev.of_node;
	result = mtd_device_parse_register(mtd, NULL, &ppdata,
					concerto_spi_nor_partitions, ARRAY_SIZE(concerto_spi_nor_partitions));
    panther_spi_nor_align_rootfs_data(mtd);
#endif

    FLASH_DEBUG("concerto_spi_init end.\n");

#if defined(SPI_NOR_PREFETCH_TEST)
    kthread_run(panther_spi_nor_prefetch, (void *) mtd, "nor_prefetch");
#endif
    mtd_global = mtd;
    /* nor flash support virtual access */
    support_virtual_access = 1;

    return result;

err:
    if (host->buffer)
	{
		kfree(host->buffer);
		host->buffer = NULL;
	}
	kfree(host);
	platform_set_drvdata(pltdev, NULL);

	return result;
}


void concerto_spi_nor_release(struct mtd_info *mtd)
{
	//del_mtd_partitions(mtd);
	//del_mtd_device(mtd);
    mtd_device_unregister(mtd);
}


/*****************************************************************************/
int	concerto_spi_nor_remove(struct platform_device *pltdev)
{
	struct concerto_sfc_host *host	= platform_get_drvdata(pltdev);

	concerto_spi_nor_release(host->mtd);
	kfree(host);
	platform_set_drvdata(pltdev, NULL);

	return 0;
}
/*****************************************************************************/
static void	concerto_spi_nor_pltdev_release(struct device *dev)
{
}
/*****************************************************************************/
static struct platform_driver concerto_spi_nor_pltdrv =
{
	.driver.name   = "concerto spi nor",
	.probe	= concerto_spi_nor_probe,
	.remove	= concerto_spi_nor_remove,
};
/*****************************************************************************/
static struct platform_device concerto_spi_nor_pltdev =
{
	.name			= "concerto spi nor",
	.id				= -1,

	.dev.platform_data	   = NULL,
	.dev.release		   = concerto_spi_nor_pltdev_release,

	.num_resources	= 0,
	.resource		= NULL,
};

/*****************************************************************************/

#ifdef CONFIG_SYSCTL
static struct ctl_table_header *spi_nor_sysctl_header;
#endif
static int __init concerto_spi_nor_module_init(void)
{
	int	result = 0;
	
	//FLASH_DEBUG("\nMT concerto Spi Nor Flash Controller Device Driver, Version 1.00\n");

	result = platform_driver_register(&concerto_spi_nor_pltdrv);
	if (result < 0)
	{
		return result;
	}

	result = platform_device_register(&concerto_spi_nor_pltdev);
	if (result < 0)
	{
		platform_driver_unregister(&concerto_spi_nor_pltdrv);
		return result;
	}

#ifdef CONFIG_SYSCTL
	spi_nor_sysctl_header = register_sysctl_table(spi_nor_sysctl_table);
	if(spi_nor_sysctl_header  == NULL)
		return -ENOMEM;
#endif
	return result;
}

/*****************************************************************************/

static void	__exit concerto_spi_nor_module_exit	(void)
{
	platform_driver_unregister(&concerto_spi_nor_pltdrv);
	platform_device_unregister(&concerto_spi_nor_pltdev);

#ifdef CONFIG_SYSCTL
	unregister_sysctl_table(spi_nor_sysctl_header);
	spi_nor_sysctl_header = NULL;
#endif
}
/*****************************************************************************/

module_init(concerto_spi_nor_module_init);
module_exit(concerto_spi_nor_module_exit);


/*****************************************************************************/







