/******************************************************************************/
/* Copyright (c) 2012 Montage Tech - All Rights Reserved                      */
/******************************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include <linux/math64.h>
#include <linux/kgdb.h>
#include <linux/sched.h>

#include <linux/mtd/nand.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
#endif

#include <asm/cache.h>
#include <asm/bootinfo.h>

#include "concerto_spi_nand.h"
#include "erased_page.h"
#include <asm/mach-panther/bootinfo.h>
#include <asm/mach-panther/pdma.h>

#define CONCERTO_SPI_NAND_FLASH_NAME     "mt_snf"

#include <linux/mtd/partitions.h>

/*=============================================================================+
| Extern Function/Variables                                                    |
+=============================================================================*/
extern int otp_get_boot_type(void);
extern void spi_access_lock(void);
extern void spi_access_unlock(void);

#define READ_CACHE_SPEEDUP

#ifndef CONFIG_MTD_CMDLINE_PARTS

#define FIRMWARE_PART_INDEX    4
#define CDB_PART_INDEX         (FIRMWARE_PART_INDEX - 2)
#define CI_PART_INDEX          (FIRMWARE_PART_INDEX - 1)
#define KERNEL_PART_INDEX      (FIRMWARE_PART_INDEX + 1)
#define ROOTFS_PART_INDEX      (FIRMWARE_PART_INDEX + 2)

#define SNF_MIN_BLOCK_SIZE     128 * 1024
int part_ratio = 1;

extern u32 is_enable_secure_boot;
u32 bad_block_table[MAX_BAD_BLOCK_NUM];
u32 bad_block_length;
u32 is_sysupgrade_running;
u32 upgrade_shift_block;

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
#define UPGRADE_CDB    1
#define UPGRADE_CI     2
#define UPGRADE_KERNEL 3
#define UPGRADE_ROOTFS 4
u32 sys_upgrade_state;

#define DEFAULT_PANTHER_MTD_SIZE 0x1000000

#define SNF_BTINT_START    0x00000000                            //0x00000000
#define SNF_BTINT_LENTH    SNF_MIN_BLOCK_SIZE                    //0x00020000

#define SNF_BVAR_START     (SNF_BTINT_START + SNF_BTINT_LENTH)
#define SNF_BVAR_LENTH     SNF_MIN_BLOCK_SIZE

#define SNF_CDB_START      (SNF_BVAR_START + SNF_BVAR_LENTH)
#define SNF_CDB_LENTH      SNF_MIN_BLOCK_SIZE

#define SNF_COMBINED_START    (SNF_CDB_START + SNF_CDB_LENTH)
#define SNF_COMBINED_LENTH    SNF_MIN_BLOCK_SIZE
/*
|<-------------- whole firmware partition --------------->|
|<---- linux partition ----->|<---- rootfs partition ---->|
*/
#define SNF_LINUX_START    (SNF_COMBINED_START + SNF_COMBINED_LENTH)
#define SNF_LINUX_LENTH    (CONFIG_PANTHER_KERNEL_SIZE * 0x100000)

#define SNF_ROOTFS_START   (SNF_LINUX_START + SNF_LINUX_LENTH)
#define SNF_ROOTFS_LENTH   (DEFAULT_PANTHER_MTD_SIZE - SNF_ROOTFS_START)

#define SNF_FIRMWARE_START  SNF_LINUX_START
#define SNF_FIRMWARE_LENTH  (DEFAULT_PANTHER_MTD_SIZE - SNF_LINUX_START)

static struct mtd_partition concerto_spi_nand_partitions[] =
{
        {
            .name       = "boot_init",
            .size       = SNF_BTINT_LENTH,
            .offset     = SNF_BTINT_START,
        },
        {
            .name       = "boot_var",
            .size       = SNF_BVAR_LENTH,
            .offset     = SNF_BVAR_START,
        },
        {
            .name       = "cdb",
            .size       = SNF_CDB_LENTH,
            .offset     = SNF_CDB_START,
        },
        {
            .name       = "combined_image_header",
            .size       = SNF_COMBINED_LENTH,
            .offset     = SNF_COMBINED_START,
        },
        {
            .name       = "firmware",
            .size       = SNF_FIRMWARE_LENTH,
            .offset     = SNF_FIRMWARE_START,
        },
        {
            .name       = "kernel",
            .size       = SNF_LINUX_LENTH,
            .offset     = SNF_LINUX_START,
        },
        {
            .name       = "rootfs",
            .size       = SNF_ROOTFS_LENTH,
            .offset     = SNF_ROOTFS_START,
        },
};

#endif

#ifdef CONFIG_SYSCTL
static int panther_spi_nand_start_virtul_access(void);
static int proc_enable_virtual_access(struct ctl_table *table, int write,
                    void __user *buffer, size_t *lenp, loff_t *ppos);
static struct ctl_table spi_nand_sysctl_table[] = {
        {
            .procname       = "flash.nand.support_virtual_access",
            .data       = &support_virtual_access,
            .maxlen     = sizeof(int),
            .mode       = 0444,
            .proc_handler   = proc_dointvec
        },
        {
            .procname       = "flash.nand.enable_virtual_access",
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
        panther_spi_nand_start_virtul_access();
    }

    return proc_dointvec(table, write, buffer, lenp, ppos);
}
#endif

#if 0
static void concerto_printData(char * title, u8* buf, u32 size)
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
#endif

/*****************************************************************************/


#if 1
// panther support now
static struct nand_ecclayout nand_oob_GD5F1GQ4UCYIG = 
{
	.oobfree = {{0x10, 16}, {0x20, 16}, {0x30, 16}}
};

static struct nand_ecclayout nand_oob_GD5F2GQ4UCYIG = 
{
	.oobfree = {{0x10, 16}, {0x20, 16}, {0x30, 16}}
};


static struct nand_ecclayout nand_oob_GD5F1GQ4UAYIG = 
{
	.oobfree = {{0x4, 8}, {0x14, 8}, {0x24, 8}, {0x34, 8}}
};

static struct nand_ecclayout nand_oob_GD5F2GQ4UAYIG = 
{
	.oobfree = {{0x4, 8}, {0x14, 8}, {0x24, 8}, {0x34, 8}}
};

static struct nand_ecclayout nand_oob_F50L512M41A = 
{
	.oobfree = {{0x8, 8}, {0x18, 8}, {0x28, 8}, {0x38, 8}}
};

static struct nand_ecclayout nand_oob_F50L1G41A = 
{
	.oobfree = {{0x8, 8}, {0x18, 8}, {0x28, 8}, {0x38, 8}}
};

static struct nand_ecclayout nand_oob_W25N01GV = 
{
	.oobfree = {{0x2, 6}, {0x10, 8}, {0x20, 8}, {0x30, 8}}
};

static struct nand_ecclayout nand_oob_GD5F1GQ4UB = 
{
	.oobfree = {{0x10, 48}}
};

static struct nand_ecclayout nand_oob_GD5F2GQ4UB = 
{
	.oobfree = {{0x10, 48}}
};

static struct nand_ecclayout nand_oob_GD5F4GQ4U = 
{
	.oobfree = {{0x10, 112}}
};

static struct nand_ecclayout nand_oob_HYF1GQ4UAACAE = 
{
	.oobfree = {{0x2, 6}, {0x22, 6}, {0x42, 6}, {0x62, 6}}
};

static struct nand_ecclayout nand_oob_HYF2GQ4UAACAE = 
{
	.oobfree = {{0x2, 6}, {0x22, 6}, {0x42, 6}, {0x62, 6}}
};

static struct nand_ecclayout nand_oob_MX35LF1GE4AB =
{
    .oobfree = {{0x10, 16}, {0x20, 16}, {0x30, 16}}
};

static struct nand_ecclayout nand_oob_XT26G01A =
{
    .oobfree = {{0x08, 40}}
};

static struct nand_ecclayout nand_oob_W25M02GC =
{
    .oobfree = {{0x10, 16}, {0x20, 16}, {0x30, 16}}
};

static struct nand_ecclayout nand_oob_F50L2G41LB =
{
    .oobfree = {{0x10, 16}, {0x20, 16}, {0x30, 16}}
};


static const struct concerto_spi_nand_flash_params concerto_spi_nand_flash_table[] = 
{
    {
        .id                 = SPI_NAND_FLASH_XT26G01A,
        .page_size          = 2048,
        .oob_size           = 64,
        .pages_per_block    = 64,
        .nr_blocks          = 1024,
        .name               = "XT26G01A",
        .layout             = &nand_oob_XT26G01A,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_MX35LF1GE4AB,
        .page_size          = 2048,
        .oob_size           = 64,
        .pages_per_block    = 64,
        .nr_blocks          = 1024,
        .name               = "MX35LF1GE4AB",
        .layout             = &nand_oob_MX35LF1GE4AB,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_MX35LF1GE4BA,
        .page_size          = 2048,
        .oob_size           = 64,
        .pages_per_block    = 64,
        .nr_blocks          = 1024,
        .name               = "MX35LF1GE4AB",
        .layout             = &nand_oob_MX35LF1GE4AB,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_W25M02GC,
        .page_size          = 2048,
        .oob_size           = 64,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "W25M02GC",
        .layout             = &nand_oob_W25M02GC,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_START,
        .read_from_cache_dummy = 8,
    },
    {
        .id                 = SPI_NAND_FLASH_F50L2G41LB,
        .page_size          = 2048,
        .oob_size           = 64,
        .pages_per_block    = 64,
        .nr_blocks          = 512,
        .name               = "F50L2G41LB",
        .layout             = &nand_oob_F50L2G41LB,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_GD5F1GQ4UCYIG,
        .page_size          = 2048,
        .oob_size           = 128,
        .pages_per_block    = 64,
        .nr_blocks          = 1024,
        .name               = "GD5F1GQ4UCYIG",
        .layout             = &nand_oob_GD5F1GQ4UCYIG,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_START,
        .read_from_cache_dummy = 8,
    },
    {
        .id                 = SPI_NAND_FLASH_GD5F2GQ4UCYIG,
        .page_size          = 2048,
        .oob_size           = 128,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "GD5F2GQ4UCYIG",
        .layout             = &nand_oob_GD5F2GQ4UCYIG,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_START,
        .read_from_cache_dummy = 8,
    },
	{
		.id                 = SPI_NAND_FLASH_F50L512M41A,
		.page_size          = 2048,
        .oob_size           = 64,
		.pages_per_block    = 64,
		.nr_blocks          = 512,
		.name               = "F50L512M41A",
		.layout             = &nand_oob_F50L512M41A,
		.read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
	},
	{
		.id                 = SPI_NAND_FLASH_F50L1G41A,
		.page_size          = 2048,
        .oob_size           = 64,
		.pages_per_block    = 64,
		.nr_blocks          = 1024,
		.name               = "F50L1G41A",
		.layout             = &nand_oob_F50L1G41A,
		.read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
	},
	{
		.id                 = SPI_NAND_FLASH_GD5F1GQ4UAYIG,
		.page_size          = 2048,
        .oob_size           = 64,
		.pages_per_block    = 64,
		.nr_blocks          = 1024,
		.name               = "GD5F1GQ4UAYIG",
        .layout             = &nand_oob_GD5F1GQ4UAYIG,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
	},
    {
        .id                 = SPI_NAND_FLASH_GD5F2GQ4UAYIG,
        .page_size          = 2048,
        .oob_size           = 64,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "GD5F2GQ4UAYIG",
        .layout             = &nand_oob_GD5F2GQ4UAYIG,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_W25N01GV,
        .page_size          = 2048,
        .oob_size           = 64,
        .pages_per_block    = 64,
        .nr_blocks          = 1024,
        .name               = "W25N01GV",
        .layout             = &nand_oob_W25N01GV,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_GD5F1GQ4UB,
        .page_size          = 2048,
        .oob_size           = 128,
        .pages_per_block    = 64,
        .nr_blocks          = 1024,
        .name               = "GD5F1GQ4UB",
        .layout             = &nand_oob_GD5F1GQ4UB,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_GD5F2GQ4UB,
        .page_size          = 2048,
        .oob_size           = 128,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "GD5F2GQ4UB",
        .layout             = &nand_oob_GD5F2GQ4UB,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_GD5F2GQ4UB_LE,
        .page_size          = 2048,
        .oob_size           = 128,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "GD5F2GQ4UB",
        .layout             = &nand_oob_GD5F2GQ4UB,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_GD5F4GQ4U_A,
        .page_size          = 4096,
        .oob_size           = 256,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "GD5F4GQ4U",
        .layout             = &nand_oob_GD5F4GQ4U,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_GD5F4GQ4U_B,
        .page_size          = 4096,
        .oob_size           = 256,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "GD5F4GQ4U",
        .layout             = &nand_oob_GD5F4GQ4U,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_HYF1GQ4UAACAE,
        .page_size          = 2048,
        .oob_size           = 128,
        .pages_per_block    = 64,
        .nr_blocks          = 1024,
        .name               = "HYF1GQ4UAACAE",
        .layout             = &nand_oob_HYF1GQ4UAACAE,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
    {
        .id                 = SPI_NAND_FLASH_HYF2GQ4UAACAE,
        .page_size          = 2048,
        .oob_size           = 128,
        .pages_per_block    = 64,
        .nr_blocks          = 2048,
        .name               = "HYF2GQ4UAACAE",
        .layout             = &nand_oob_HYF2GQ4UAACAE,
        .read_cmd_dummy_type   = READ_CMD_DUMMY_END,
        .read_from_cache_dummy = 0,
    },
};

static u32 concerto_spi_nand_getJedecID(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;

    u8 cmd[4];
    u8 codeID[4];
    u32 id = 0xffffff;
    int ret;

    cmd[0] = SPI_NAND_FLASH_CMD_RD_MAN_ID;
    cmd[1] = 0x00;
    
    ret = concerto_spi_read_common(host->spi, cmd, 2, codeID, 4);

    if (ret)
    {
        id = 0xffffff;
    }
    else
    {
        id = codeID[0] | codeID[1] << 8 | (codeID[2] << 16);
    }
    
    return id;
}


static int concerto_spi_nand_wait_write_complete(struct mtd_info *mtd, u32 timeout)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;

    u8 resp = 0;
    int ret = SUCCESS;
    u32 status = 0;

    u8 cmd[2];

    status = 1;

    while (status && timeout)
    {
        cmd[0] = SPI_NAND_FLASH_CMD_RD_SR;
        cmd[1] = SPI_NAND_FLASH_ADDR_ST;
        
        ret = concerto_spi_read_common(host->spi, cmd, 2, &resp, 1);
        if (ret < 0)
        {
            timeout--;
            continue;
        }
        if ((resp & 0x01) == 1)
        {
            status = 1;
            timeout--;
        }
        else
        {
            status = 0;
        }
    }
    if (timeout == 0)
    {
        FLASH_DEBUG("concerto_spi_nand_wait_write_complete: time out! %s %d %s\n",
                __FUNCTION__, __LINE__, __FILE__);
        ret = ERR_TIMEOUT;
    }
    return ret;
}


static void concerto_spi_nand_internel_ecc_on(struct mtd_info *mtd, u8 on)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
	
    u8 value = 0;
    u8 cmd[2];
	
    cmd[0] = SPI_NAND_FLASH_CMD_RD_SR;
    cmd[1] = SPI_NAND_FLASH_ADDR_FT;
	
    concerto_spi_read_common(host->spi, cmd, 2, &value, 1);
	
    if (on)
    {
        value |= (1 << 4);
    }
    else
    {
        value &= ~(1 << 4);
    }
    
    cmd[0] = SPI_NAND_FLASH_CMD_WR_SR;
    cmd[1] = SPI_NAND_FLASH_ADDR_FT;
	
    concerto_spi_cmd_write(host->spi, cmd, 2, &value, 1, 0);
	
    concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    return;

}


static u8 concerto_spi_nand_get_ecc_result(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

    u8 resp = 0;
    u8 result = 0;
    u8 cmd[2];

    cmd[0] = SPI_NAND_FLASH_CMD_RD_SR;
    cmd[1] = SPI_NAND_FLASH_ADDR_ST;
    
    concerto_spi_read_common(host->spi, cmd, 2, &resp, 1);
    result = (resp >> 4) & 0x03;

    if (result == 0x03)
    {
        result = 0x01;
    }

    return result;
}


static void concerto_spi_nand_quad_feature_on(struct mtd_info *mtd, u8 on)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;

    u8 value = 0;
    u8 cmd[2];

    cmd[0] = SPI_NAND_FLASH_CMD_RD_SR;
    cmd[1] = SPI_NAND_FLASH_ADDR_FT;

    concerto_spi_read_common(host->spi, cmd, 2, &value, 1);

    if (on)
    {
        value |= (1 << 0);
    }
    else
    {
        value &= ~(1 << 0);
    }
    
    cmd[0] = SPI_NAND_FLASH_CMD_WR_SR;
    cmd[1] = SPI_NAND_FLASH_ADDR_FT;

    concerto_spi_cmd_write(host->spi, cmd, 2, &value, 1, 0);

    concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    return;

}


static u8 concerto_spi_nand_get_quad_feature(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

    u8 resp = 0;
    u8 result = 0;
    u8 cmd[2];

    cmd[0] = SPI_NAND_FLASH_CMD_RD_SR;
    cmd[1] = SPI_NAND_FLASH_ADDR_FT;
        
    concerto_spi_read_common(host->spi, cmd, 2, &resp, 1);

    result = resp & 0x01;

    return result;
}

static int concerto_spi_nand_set_feature(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;

    u8 value = 0;
    u8 cmd[2];
    
    cmd[0] = SPI_NAND_FLASH_CMD_WR_SR;
    cmd[1] = SPI_NAND_FLASH_ADDR_PT;
    concerto_spi_cmd_write(host->spi, cmd, 2, &value, 1, 0);

    return 0;
}

static int concerto_spi_nand_reset(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;

    u8 cmd[5];
    
    cmd[0] = SPI_NAND_FLASH_CMD_WR_RESET;
	
    ret = concerto_spi_read_common(host->spi, cmd, 1, NULL, 0);
    ret = concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    ret = concerto_spi_nand_set_feature(mtd);
    ret = concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    return ret;
}

static int concerto_spi_nand_write_enable(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;

    u8 cmd[5];
	
    cmd[0] = SPI_NAND_FLASH_CMD_WR_EN;
	
    ret = concerto_spi_read_common(host->spi, cmd, 1, NULL, 0);

    ret = concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    return ret;
}

static int concerto_spi_nand_block_erase(struct mtd_info *mtd, u32 block_addr)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;

    u8 cmd[5];
    u32 page_addr = block_addr << (host->erase_shift - host->page_shift);
    concerto_spi_nand_write_enable(mtd);

    cmd[0] = SPI_NAND_FLASH_CMD_BLK_ERASE;
    cmd[1] = page_addr >> 16;
    cmd[2] = page_addr >> 8;
    cmd[3] = page_addr >> 0;

    ret = concerto_spi_cmd_write(host->spi, cmd, 4, NULL, 0, 0);
    if (ret)
    {
        goto out;
    }

    ret = concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

out:
    return ret;
}

static int concerto_spi_nand_program_execute(struct mtd_info *mtd, u32 page_addr)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;

    u8 cmd[5];

    cmd[0] = SPI_NAND_FLASH_CMD_PG_EXC;
    cmd[1] = page_addr >> 16;
    cmd[2] = page_addr >> 8;
    cmd[3] = page_addr >> 0;

    ret = concerto_spi_cmd_write(host->spi, cmd, 4, NULL, 0, 0);

    return ret;

}

static int concerto_spi_nand_program_single_load(struct mtd_info *mtd, u32 col_addr, u8* buf, u32 size)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;
    u8 cmd[5];

    cmd[0] = SPI_NAND_FLASH_CMD_LOAD;
    cmd[1] = col_addr >> 8;
    cmd[2] = col_addr;

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    ret = concerto_spi_cmd_write(host->spi, cmd, 3, buf, size, 1);
#else
    ret = concerto_spi_cmd_write(host->spi, cmd, 3, buf, size, 0);
#endif

    return ret;
}

static int concerto_spi_nand_program_quad_load(struct mtd_info *mtd, u32 col_addr, u8 *buf, u32 size)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host = chip->priv;
    int ret = SUCCESS;
    u8 cmd[5];

    cmd[0] = SPI_NAND_FLASH_CMD_QUAD_LOAD;
    cmd[1] = col_addr >> 8;
    cmd[2] = col_addr;

    concerto_spi_set_trans_ionum(host->spi, 4);
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    ret = concerto_spi_cmd_write(host->spi, cmd, 3, buf, size, 1);
#else
    ret = concerto_spi_cmd_write(host->spi, cmd, 3, buf, size, 0);
#endif
    concerto_spi_set_trans_ionum(host->spi, 1);

    return ret;
}

static int concerto_spi_nand_single_page_read_from_cache(struct mtd_info *mtd, u32 col_addr, u8* buf, u32 size)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;    
    struct spi_read_cfg read_cfg;
    u8 cmd[4];
    int ret = SUCCESS;

    memset(&read_cfg, 0, sizeof(read_cfg));

    
    cmd[0] = SPI_NAND_FLASH_CMD_PAGE_RD_FRM_CACHE;
    if (host->read_cmd_dummy_type == READ_CMD_DUMMY_END)
    {
        cmd[1] = (col_addr >> 8);
        cmd[2] = (col_addr >> 0);
        cmd[3] = 0;
    }
    else
    {
        cmd[1] = 0;
        cmd[2] = (col_addr >> 8);
        cmd[3] = (col_addr >> 0);
    }

    // see explain about read_from_cache_dummy of concerto_spi_nand.h
    if (host->read_from_cache_dummy != 0)
    {
        read_cfg.read_from_cache_dummy = host->read_from_cache_dummy;
    }

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    ret = concerto_spi_cmd_read(host->spi, cmd, 4, buf, size, &read_cfg, 1);
#else
    ret = concerto_spi_cmd_read(host->spi, cmd, 4, buf, size, &read_cfg, 0);
#endif

    return ret;
}

static int concerto_spi_nand_dual_page_read_from_cache(struct mtd_info *mtd, u32 col_addr, u8* buf, u32 size)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    struct spi_read_cfg read_cfg;
    u8 cmd[4];
    int ret = SUCCESS;

    memset(&read_cfg, 0, sizeof(read_cfg));
    
    cmd[0] = SPI_NAND_FLASH_CMD_PAGE_RD_DUAL_FRM_CACHE;
    if (host->read_cmd_dummy_type == READ_CMD_DUMMY_END)
    {
        cmd[1] = (col_addr >> 8);
        cmd[2] = (col_addr >> 0);
        cmd[3] = 0;
    }
    else
    {
        cmd[1] = 0;
        cmd[2] = (col_addr >> 8);
        cmd[3] = (col_addr >> 0);
    }

    // see explain about read_from_cache_dummy of concerto_spi_nand.h
    if (host->read_from_cache_dummy != 0)
    {
        read_cfg.read_from_cache_dummy = host->read_from_cache_dummy;
    }

    concerto_spi_set_trans_ionum(host->spi, 2);
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    ret = concerto_spi_cmd_read(host->spi, cmd, 4, buf, size, &read_cfg, 1);
#else
    ret = concerto_spi_cmd_read(host->spi, cmd, 4, buf, size, &read_cfg, 0);
#endif
    concerto_spi_set_trans_ionum(host->spi, 1);

    return ret;
}

static int concerto_spi_nand_quad_page_read_from_cache(struct mtd_info *mtd, u32 col_addr, u8* buf, u32 size)
{
    struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    struct spi_read_cfg read_cfg;
    u8 cmd[4];
    int ret = SUCCESS;

    memset(&read_cfg, 0, sizeof(read_cfg));
    
    cmd[0] = SPI_NAND_FLASH_CMD_PAGE_RD_QUAD_FRM_CACHE;
    if (host->read_cmd_dummy_type == READ_CMD_DUMMY_END)
    {
        cmd[1] = (col_addr >> 8);
        cmd[2] = (col_addr >> 0);
        cmd[3] = 0;
    }
    else
    {
        cmd[1] = 0;
        cmd[2] = (col_addr >> 8);
        cmd[3] = (col_addr >> 0);
    }

    // see explain about read_from_cache_dummy of concerto_spi_nand.h
    if (host->read_from_cache_dummy != 0)
    {
        read_cfg.read_from_cache_dummy = host->read_from_cache_dummy;
    }

    concerto_spi_set_trans_ionum(host->spi, 4);
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    ret = concerto_spi_cmd_read(host->spi, cmd, 4, buf, size, &read_cfg, 1);
#else
    ret = concerto_spi_cmd_read(host->spi, cmd, 4, buf, size, &read_cfg, 0);
#endif
    concerto_spi_set_trans_ionum(host->spi, 1);

    return ret;
}

static int concerto_spi_nand_page_read_to_cache(struct mtd_info *mtd, u32 page_addr)
{    
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;

    u8 cmd[4];
    
    cmd[0] = SPI_NAND_FLASH_CMD_PAGE_RD_TO_CACHE;
    cmd[1] = page_addr >> 16;
    cmd[2] = page_addr >> 8;
    cmd[3] = page_addr >> 0;
    
    ret = concerto_spi_read_common(host->spi, cmd, 4, NULL, 0);

    return ret;
}

static int concerto_spi_nand_page_write(struct mtd_info *mtd, u32 page_addr, u32 col_addr, u8* buf, u32 size)
{    
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    struct mtd_partition *kernel_part = NULL;
    int b_start;
    int b_end;
    int ret = SUCCESS;
    uint64_t block_size = SNF_MIN_BLOCK_SIZE * part_ratio;

    kernel_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX + 1];
    b_start = div64_u64(kernel_part->offset, block_size);
    b_end = div64_u64(kernel_part->size, block_size) + b_start;

    // if someone wnat to write kernel partition,
    // use kgdb to break and use bt command to see that who want to do
//  if ((page_addr >= (64 * b_start)) && (page_addr < (64 * b_end)))
//  {
//      FLASH_DEBUG("Warning!, Somebody \"WRITE\" kernel partition\n");
//      kgdb_breakpoint();
//  }

    host->buffer_page_addr = BUFFER_PAGE_ADDR_INVALID;
    memset(host->buffer, 0xff, mtd->writesize);
    memcpy(host->buffer + col_addr, buf, size);

    concerto_spi_nand_write_enable(mtd);
    if (host->write_ionum == 4)
    {
        ret = concerto_spi_nand_program_quad_load(mtd, 0, host->buffer, mtd->writesize);
    }
    else
    {
        ret = concerto_spi_nand_program_single_load(mtd, 0, host->buffer, mtd->writesize);
    }
    if (ret)
        goto out;

    ret = concerto_spi_nand_program_execute(mtd, page_addr);
    if (ret)
        goto out;

    ret = concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

out:
    return ret;
}

static int concerto_spi_nand_oob_write(struct mtd_info *mtd, u32 page_addr, u8* buf)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;

    concerto_spi_nand_write_enable(mtd);
    
    change_aes_enable_status(DISABLE_SECURE);
    if (host->write_ionum == 4)
    {
        ret = concerto_spi_nand_program_quad_load(mtd, mtd->writesize, host->oob_poi, mtd->oobsize);
    }
    else
    {
        ret = concerto_spi_nand_program_single_load(mtd, mtd->writesize, host->oob_poi, mtd->oobsize);
    }
    change_aes_enable_status(ENABLE_SECURE_OTP);
    if (ret)
        goto out;

    ret = concerto_spi_nand_program_execute(mtd, page_addr);
    if (ret)
        goto out;

    ret = concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

out:
    return ret;
}

static int concerto_spi_nand_page_read(struct mtd_info *mtd, u32 page_addr, u32 col_addr, u8* buf, u32 size)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;
    u8 ecc_result;

    concerto_spi_nand_page_read_to_cache(mtd, page_addr);
    concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    if((size==mtd->writesize) && (col_addr==0) && (((u32) buf & L1_CACHE_BYTES)==0) && ((u32)buf < 0x90000000UL))
    {
        //printk(KERN_EMERG " => %d %d %p %d\n", col_addr, size, buf, size);
        if (host->read_ionum == 4)
            ret = concerto_spi_nand_quad_page_read_from_cache(mtd, 0, buf, size);
        else if (host->read_ionum == 2)
            ret = concerto_spi_nand_dual_page_read_from_cache(mtd, 0, buf, size);
        else
            ret = concerto_spi_nand_single_page_read_from_cache(mtd, 0, buf, size);

        if (is_enable_secure_boot)
        {
            // if in secure boot, need to check whether this page had been erased or not
            panther_spi_restore_erased_page((u8 *)(buf), size);
        }
        host->buffer_page_addr = BUFFER_PAGE_ADDR_INVALID;
    }
#if defined(READ_CACHE_SPEEDUP)
    else if(host->buffer_page_addr==page_addr)
    {
        //printk(KERN_EMERG "===> SPEEDUP\n");
        memcpy(buf, (void*)(host->buffer + col_addr), size);
        ecc_result = 0;
        goto Exit;
    }
#endif
    else
    {
        if (host->read_ionum == 4)
            ret = concerto_spi_nand_quad_page_read_from_cache(mtd, 0, host->buffer, mtd->writesize);
        else if (host->read_ionum == 2)
            ret = concerto_spi_nand_dual_page_read_from_cache(mtd, 0, host->buffer, mtd->writesize);
        else
            ret = concerto_spi_nand_single_page_read_from_cache(mtd, 0, host->buffer, mtd->writesize);

        if (is_enable_secure_boot)
        {
            // if in secure boot, need to check whether this page had been erased or not
            panther_spi_restore_erased_page((u8 *)(host->buffer), mtd->writesize);
        }
        host->buffer_page_addr = page_addr;
        memcpy(buf, (void*)(host->buffer + col_addr), size);
    }

    ecc_result = concerto_spi_nand_get_ecc_result(mtd);
    if (ecc_result == 0x02)
    {
        mtd->ecc_stats.failed++;
    }

#if defined(READ_CACHE_SPEEDUP)
Exit:
#endif
    return ret;
}

static int concerto_spi_nand_oob_read(struct mtd_info *mtd, u32 page_addr, u8* buf)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = SUCCESS;
    u8 ecc_result;

    concerto_spi_nand_page_read_to_cache(mtd, page_addr);
    concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    change_aes_enable_status(DISABLE_SECURE);
    if (host->read_ionum == 4)
    {
        ret = concerto_spi_nand_quad_page_read_from_cache(mtd, mtd->writesize, host->oob_poi, mtd->oobsize);
    }
    else if (host->read_ionum == 2)
    {
        ret = concerto_spi_nand_dual_page_read_from_cache(mtd, mtd->writesize, host->oob_poi, mtd->oobsize);
    }
    else
    {
        ret = concerto_spi_nand_single_page_read_from_cache(mtd, mtd->writesize, host->oob_poi, mtd->oobsize);
    }
    change_aes_enable_status(ENABLE_SECURE_OTP);

    ecc_result = concerto_spi_nand_get_ecc_result(mtd);

    if (ecc_result == 0x02)
    {
        mtd->ecc_stats.failed++;
    }

    return ret;
}

static int concerto_spi_nand_oob_badblock_flag_read(struct mtd_info *mtd, u32 page_addr, u8* buf)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    u8 ecc_result;
    int ret = SUCCESS;

    concerto_spi_nand_page_read_to_cache(mtd, page_addr);
    ret = concerto_spi_nand_wait_write_complete(mtd, SPI_FLASH_PROG_TIMEOUT);

    if (host->read_ionum == 4)
    {
        ret = concerto_spi_nand_quad_page_read_from_cache(mtd, mtd->writesize, buf, 1);
    }
    else if (host->read_ionum == 2)
    {
        ret = concerto_spi_nand_dual_page_read_from_cache(mtd, mtd->writesize, buf, 1);
    }
    else
    {
        ret = concerto_spi_nand_single_page_read_from_cache(mtd, mtd->writesize, buf, 1);
    }

    ecc_result = concerto_spi_nand_get_ecc_result(mtd);

    if (ecc_result == 0x02)
    {
        mtd->ecc_stats.failed++;
    }

    return ret;
}

static int concerto_spi_nand_block_is_bad(struct mtd_info *mtd, loff_t offs)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;
    int ret = 0;
    u32 block_addr = (u32)(offs >> host->erase_shift);    
    u8 flag = 0;

    spi_access_lock();

    concerto_spi_nand_internel_ecc_on(mtd, 0);
    change_aes_enable_status(DISABLE_SECURE);
    
    concerto_spi_nand_oob_badblock_flag_read(mtd, (block_addr << (host->erase_shift - host->page_shift)), &flag);
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    flag = *((u8 *)UNCACHED_ADDR(&flag));
#endif
    if (flag != 0xff)
    {
        ret = 1;
        goto out;
    }
    
    concerto_spi_nand_oob_badblock_flag_read(mtd, (block_addr << (host->erase_shift - host->page_shift)) + 1, &flag);
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    flag = *((u8 *)UNCACHED_ADDR(&flag));
#endif
    if (flag != 0xff)
    {
        ret = 1;
        goto out;
    }

out:
    change_aes_enable_status(ENABLE_SECURE_OTP);
    concerto_spi_nand_internel_ecc_on(mtd, 1);

    spi_access_unlock();
    return ret;
}

static int panther_spi_is_value_in_array(int val, int *arr, int size);
static int panther_shift_block_position(u32 blockno)
{
    u32 new_blockno = blockno;

    while(panther_spi_is_value_in_array(new_blockno, bad_block_table, bad_block_length))
    {
        upgrade_shift_block += 1;
        new_blockno += 1;
    }

    return new_blockno;
}

int concerto_spi_nand_virtual_access_erase(struct mtd_info *mtd, loff_t from, size_t len)
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

static int concerto_spi_nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

    u32 blockno;
    int ret;
    loff_t len;

    struct mtd_partition *cdb_part = NULL;
    struct mtd_partition *ci_part = NULL;
    struct mtd_partition *kernel_part = NULL;
    struct mtd_partition *rootfs_part = NULL;
    int b_start;
    int b_end;
    uint64_t block_size = SNF_MIN_BLOCK_SIZE * part_ratio;

    /* Start address must align on block boundary */
    if (instr->addr & ((1 << host->erase_shift) - 1)) 
    {
        FLASH_DEBUG("concerto_spi_nand_erase: Unaligned address\n");
        return -EINVAL;
    }

    /* Length must align on block boundary */
    if (instr->len & ((1 << host->erase_shift) - 1)) 
    {
        FLASH_DEBUG("concerto_spi_nand_erase: "
            "Length not block aligned\n");
        return -EINVAL;
    }

    /* Do not allow erase past end of device */
    if ((instr->len + instr->addr) > mtd->size) 
    {
        FLASH_DEBUG("concerto_spi_nand_erase: "
            "Erase past end of device\n");
        return -EINVAL;
    }

    spi_access_lock();

    blockno = (u32)(instr->addr >> host->erase_shift);

    len = instr->len;

    instr->state = MTD_ERASING;

    if(enable_virtual_access && (0 != strcmp(current->comm, "mtd")))
    {
        if(0 == concerto_spi_nand_virtual_access_erase(mtd, instr->addr, len))
        {
            goto erase_done;
        }
    }

    // CDB part
    cdb_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX - 2];
    b_start = div64_u64(cdb_part->offset, block_size);
    b_end = div64_u64(cdb_part->size, block_size) + b_start;
    if ((blockno >= b_start) && (blockno < b_end))
    {
        sys_upgrade_state = UPGRADE_CDB;
        upgrade_shift_block = 0;
    }

    // CI part
    if (sys_upgrade_state != UPGRADE_CI)
    {
        ci_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX - 1];
        b_start = div64_u64(ci_part->offset, block_size);
        b_end = div64_u64(ci_part->size, block_size) + b_start;
    
        // if sysupgrade wnat to erase combined_image_header partition,
        // disable flag is_enable_secure_boot
        if ((blockno >= b_start) && (blockno < b_end))
        {
            sys_upgrade_state = UPGRADE_CI;
            is_enable_secure_boot = 0;
            is_sysupgrade_running = 1;
            upgrade_shift_block = 0;
        }
    }

    // kernel part
    if (is_sysupgrade_running && (sys_upgrade_state == UPGRADE_CI))
    {    
        kernel_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX + 1];
        b_start = div64_u64(kernel_part->offset, block_size);
        b_end = div64_u64(kernel_part->size, block_size) + b_start;

        if ((blockno >= b_start) && (blockno < b_end))
        {
            sys_upgrade_state = UPGRADE_KERNEL;
            upgrade_shift_block = 0;
        }
    }

     // rootfs part
    if (is_sysupgrade_running && (sys_upgrade_state == UPGRADE_KERNEL))
    {    
        rootfs_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX + 2];
        b_start = div64_u64(rootfs_part->offset, block_size);
        b_end = div64_u64(rootfs_part->size, block_size) + b_start;

        if ((blockno >= b_start) && (blockno < b_end))
        {
            sys_upgrade_state = UPGRADE_ROOTFS;
        }
    }

    if (is_sysupgrade_running)
    {
        blockno += upgrade_shift_block;
    }

    while (len) 
    {
        if (is_sysupgrade_running)
        {
            printk("erase blockno = %d\n", blockno);
        }
        if(concerto_spi_nand_block_is_bad(mtd, (loff_t)(blockno << host->erase_shift)))
        {
            FLASH_DEBUG("concerto_spi_nand_erase: attempt to erase a "
                "bad block %d\n", blockno);
            instr->state = MTD_ERASE_FAILED;

            if (is_sysupgrade_running)
            {
                blockno = panther_shift_block_position(blockno);
                continue;
            }
            else if (sys_upgrade_state == UPGRADE_CDB)
            {
                sys_upgrade_state = 0;
                upgrade_shift_block = 0;
                blockno = panther_shift_block_position(blockno);
                continue;
            }
            else
            {
                goto erase_exit;
            }
        }

        ret = concerto_spi_nand_block_erase(mtd, blockno);
        if (ret)
        {
            goto erase_exit;
        }

        /* Increment page address and decrement length */
        len -= mtd->erasesize;
        blockno += 1;
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



static u8 *concerto_spi_nand_transfer_oob(struct mtd_info *mtd, u8 *oob,
				  struct mtd_oob_ops *ops, size_t len)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

	switch(ops->mode) 
    {
	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_RAW:
        memcpy(oob, host->oob_poi + ops->ooboffs, len);
		return oob + len;

	case MTD_OPS_AUTO_OOB: 
    {
		struct nand_oobfree *free = host->ecclayout->oobfree;
		u32 boffs = 0, roffs = ops->ooboffs;
		size_t bytes = 0;

		for(; free->length && len; free++, len -= bytes) 
        {
			/* Read request not from offset 0 ? */
			if (unlikely(roffs)) 
            {
				if (roffs >= free->length) 
                {
					roffs -= free->length;
					continue;
				}
				boffs = free->offset + roffs;
				bytes = min_t(size_t, len, (free->length - roffs));
				roffs = 0;
			} 
            else 
            {
				bytes = min_t(size_t, len, free->length);
				boffs = free->offset;
			}
            memcpy(oob, host->oob_poi + boffs, bytes);
			oob += bytes;
		}
		return oob;
	}
	default:
		BUG();
	}
	return NULL;
}

static u8 *concerto_spi_nand_fill_oob(struct mtd_info *mtd, uint8_t *oob, size_t len,
				  struct mtd_oob_ops *ops)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

	memset(host->oob_poi, 0xff, mtd->oobsize);

    switch(ops->mode) 
    {
        case MTD_OPS_PLACE_OOB:
        case MTD_OPS_RAW:
            memcpy(host->oob_poi + ops->ooboffs, oob, len);
            return oob + len;

        case MTD_OPS_AUTO_OOB: 
            {
                struct nand_oobfree *free = host->ecclayout->oobfree;
                uint32_t boffs = 0, woffs = ops->ooboffs;
                size_t bytes = 0;

                for(; free->length && len; free++, len -= bytes) 
                {
                    /* Write request not from offset 0 ? */
                    if (unlikely(woffs)) 
                    {
                        if (woffs >= free->length) 
                        {
                            woffs -= free->length;
                            continue;
                        }
                        boffs = free->offset + woffs;
                        bytes = min_t(size_t, len, (free->length - woffs));
                        woffs = 0;
                    } 
                    else 
                    {
                        bytes = min_t(size_t, len, free->length);
                        boffs = free->offset;
                    }
                    memcpy(host->oob_poi + boffs, oob, bytes);
                    oob += bytes;
                }
                return oob;
            }
        default:
            BUG();
    }
    return NULL;
}


static int concerto_spi_nand_do_read_ops(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

    struct mtd_partition *cdb_part = NULL;
    u32 blockno;
    int b_start;
    int b_end;
    uint64_t block_size = SNF_MIN_BLOCK_SIZE * part_ratio;

	int page, col, bytes;
	struct mtd_ecc_stats stats;

	int ret = SUCCESS;
	uint32_t readlen = ops->len;
//  uint32_t oobreadlen = ops->ooblen;
//  uint32_t max_oobsize = ops->mode == MTD_OPS_AUTO_OOB ?
//  	mtd->oobavail : mtd->oobsize;
//  uint8_t *oob;
    uint8_t *buf;
//  unsigned int max_bitflips = 0;

	stats = mtd->ecc_stats;

    blockno = (u32)(from >> host->erase_shift);
	page = (int)(from >> host->page_shift);
	col = (int)(from & (mtd->writesize - 1));

    // CDB part
    cdb_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX - 2];
    b_start = div64_u64(cdb_part->offset, block_size);
    b_end = div64_u64(cdb_part->size, block_size) + b_start;
    if ((blockno >= b_start) && (blockno < b_end))
    {
        // which means have bad block in cdb partition
        if (cdb_part->size != block_size)
        {
            blockno = panther_shift_block_position(blockno);
            page = (blockno * 0x40) + (page % 0x40);
        }
    }

	buf = ops->datbuf;
//  oob = ops->oobbuf;

//  printk("page = %d, block = %d, readlen = 0x%x\n", page, page/0x40, readlen);
    
    while(1) 
    {
        bytes = min(mtd->writesize - col, readlen);
        
        ret = concerto_spi_nand_page_read(mtd, page, col, buf, bytes);

//      max_bitflips = max_t(unsigned int, max_bitflips, ret);

        buf += bytes;

//      // use unlikely macro for optimization
//      if (unlikely(oob))
//      {
//          int toread = min(oobreadlen, max_oobsize);
//
//          if (toread)
//          {
//              oob = concerto_spi_nand_transfer_oob(mtd, oob, ops, toread);
//              oobreadlen -= toread;
//          }
//      }

        readlen -= bytes;

        if (!readlen)
        {
            break;
        }

        /* For subsequent reads align to page boundary. */
        col = 0;
        /* Increment page address */
        page++;
    }

    ops->retlen = ops->len - (size_t)readlen;
//  if (oob)
//  {
//      ops->oobretlen = ops->ooblen - oobreadlen;
//  }

    if (mtd->ecc_stats.failed - stats.failed)
    {
        return -EBADMSG;
    }

	return ret;
}

int concerto_spi_nand_virtual_access_read(struct mtd_info *mtd, loff_t from, size_t len,
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

static int concerto_spi_nand_read(struct mtd_info *mtd, loff_t from, size_t len,
		     size_t *retlen, uint8_t *buf)
{
	struct mtd_oob_ops ops;
	int ret = SUCCESS;

	spi_access_lock();

    if(enable_virtual_access && (0 != strcmp(current->comm, "mtd")))
    {
        if(0 == concerto_spi_nand_virtual_access_read(mtd, from, len, retlen, buf))
        {
            spi_access_unlock();
            return ret;
        }
    }

	ops.len = len;
	ops.datbuf = buf;
	ops.oobbuf = NULL;
	ops.mode = 0;

	ret = concerto_spi_nand_do_read_ops(mtd, from, &ops);
	*retlen = ops.retlen;

    // tansform local error code to system error code
    if (ret)
    {
        FLASH_DEBUG("SNF DRV ERROR: read failed\n");
        ret = -EIO;
    }

	spi_access_unlock();
	return ret;
}


static int concerto_spi_nand_do_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

    int page;
    
	struct mtd_ecc_stats stats;

    int readlen = ops->ooblen;
    int len;
    u8 *buf = ops->oobbuf;

    // FLASH_DEBUG("%s: from = 0x%08Lx, len = %i\n",
	//		__func__, (unsigned long long)from, readlen);

    stats = mtd->ecc_stats;

    if (ops->mode == MTD_OPS_AUTO_OOB)
    {
        len = host->ecclayout->oobavail;
    }
    else
    {
        len = mtd->oobsize;
    }

    if (unlikely(ops->ooboffs >= len)) 
    {
        FLASH_DEBUG("%s: attempt to start read outside oob\n",
				__func__);
        return -EINVAL;
    }

    /* Do not allow reads past end of device */
    if (unlikely(from >= mtd->size ||
        ops->ooboffs + readlen > ((mtd->size >> host->page_shift) -
            (from >> host->page_shift)) * len)) 
    {
        FLASH_DEBUG("%s: attempt to read beyond end of device\n",
				__func__);
        return -EINVAL;
    }

    /* Shift to get page */
    page = (int)(from >> host->page_shift);
    while(1) 
    {
        concerto_spi_nand_oob_read(mtd, page, buf);

        len = min(len, readlen);
        buf = concerto_spi_nand_transfer_oob(mtd, buf, ops, len);

        readlen -= len;
        if (!readlen)
            break;

        /* Increment page address */
        page++;
    }

	ops->oobretlen = ops->ooblen - readlen;
	if (mtd->ecc_stats.failed - stats.failed)
    {
        return -EBADMSG;
    }

	return  mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0;
}


static int concerto_spi_nand_read_oob(struct mtd_info *mtd, loff_t from,
			 struct mtd_oob_ops *ops)
{
    int ret = -ENOTSUPP;

    ops->retlen = 0;

    /* Do not allow reads past end of device */
    if (ops->datbuf && (from + ops->len) > mtd->size) 
    {
        FLASH_DEBUG("%s: attempt to read beyond end of device\n",
            __func__);
        return -EINVAL;
    }

    spi_access_lock();

    switch(ops->mode) 
    {
        case MTD_OPS_PLACE_OOB:
        case MTD_OPS_AUTO_OOB:
        case MTD_OPS_RAW:
            break;

        default:
            goto out;
    }

    if (!ops->datbuf)
    {
        ret = concerto_spi_nand_do_read_oob(mtd, from, ops);
    }
    else
    {
        ret = concerto_spi_nand_do_read_ops(mtd, from, ops);
    }
out:
    // tansform local error code to system error code
    if (ret)
    {
        FLASH_DEBUG("SNF DRV ERROR: read oob failed\n");
        ret = -EIO;
    }

    spi_access_unlock();
    return ret;
}


static int concerto_spi_nand_do_write_ops(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

    struct mtd_partition *cdb_part = NULL;
    u32 blockno;
    int b_start;
    int b_end;
    uint64_t block_size = SNF_MIN_BLOCK_SIZE * part_ratio;

    int page;
    int ret = SUCCESS;
    uint32_t writelen = ops->len;

	uint32_t oobwritelen = ops->ooblen;
	uint32_t oobmaxlen = ops->mode == MTD_OPS_AUTO_OOB ? mtd->oobavail : mtd->oobsize;

    uint8_t *oob = ops->oobbuf;
    uint8_t *buf = ops->datbuf;

    ops->retlen = 0;
    if (!writelen)
    {
        return 0;
    }

    if((to & (mtd->writesize - 1)) || 
        (ops->len & (mtd->writesize - 1)))
    {
        FLASH_DEBUG("concerto_spi_nand_do_write_ops: "
            "Attempt to write not page aligned data\n");
        return -EINVAL;
    }

    page = (int)(to >> host->page_shift);
    blockno = (page/0x40);

	/* Don't allow multipage oob writes with offset */
	if (oob && ops->ooboffs && (ops->ooboffs + ops->ooblen > oobmaxlen))
		return -EINVAL;

    if (is_sysupgrade_running)
    {
        page = (blockno + upgrade_shift_block) * 0x40;
        printk("writelen = 0x%x, blockno = %d\n", writelen, blockno);
    }

    // CDB part
    cdb_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX - 2];
    b_start = div64_u64(cdb_part->offset, block_size);
    b_end = div64_u64(cdb_part->size, block_size) + b_start;
    if ((blockno >= b_start) && (blockno < b_end))
    {
        printk("==============================================\n");
        printk("page = %d, blockno = %d\n", page, blockno);
        blockno = panther_shift_block_position(blockno);
        page = (blockno * 0x40);
        printk("page = %d, blockno = %d\n", page, blockno);
    }

    while(1) 
    {
        uint8_t *wbuf = buf;
        if (unlikely(oob))
        {
			size_t len = min(oobwritelen, oobmaxlen);
            oob = concerto_spi_nand_fill_oob(mtd, oob, len, ops);
			oobwritelen -= len;
        }
        else
        {
            memset(host->oob_poi, 0xff, mtd->oobsize);
        }

        ret = concerto_spi_nand_page_write(mtd, page, 0, wbuf, mtd->writesize);
        if (ret)
        {
            goto out;
        }

        writelen -= mtd->writesize;
        if (!writelen)
        {
            break;
        }

        buf += mtd->writesize;
        page++;
    }

    ops->retlen = ops->len - writelen;
    if (unlikely(oob))
    {
        ops->oobretlen = ops->ooblen;
    }

out:
    return ret;
}

int concerto_spi_nand_virtual_access_write(struct mtd_info *mtd, loff_t from, size_t len,
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

static int concerto_spi_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
			  size_t *retlen, const uint8_t *buf)
{
	int ret = SUCCESS;
    struct mtd_oob_ops ops;

	/* Do not allow reads past end of device */
	if ((to + len) > mtd->size)
    {   
		return -EINVAL;
    }
	if (!len)
    {   
		return 0;
    }

	spi_access_lock();

    if(enable_virtual_access && (0 != strcmp(current->comm, "mtd")))
    {
        if(0 == concerto_spi_nand_virtual_access_write(mtd, to, len, retlen, buf))
        {
            spi_access_unlock();
            return ret;
        }
    }

	ops.len = len;
	ops.datbuf = (uint8_t *)buf;
	ops.oobbuf = NULL;
	ops.mode = 0;

	ret = concerto_spi_nand_do_write_ops(mtd, to, &ops);

	*retlen = ops.retlen;

    // tansform local error code to system error code
    if (ret)
    {
        FLASH_DEBUG("SNF DRV ERROR: write failed\n");
        ret = -EIO;
    }

	spi_access_unlock();
	return ret;
}

static int concerto_spi_nand_do_write_oob(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;
	int  page, len;
    int ret = SUCCESS;

	if (ops->mode == MTD_OPS_AUTO_OOB)
    {   
		len = host->ecclayout->oobavail;
    }
	else
    {   
		len = mtd->oobsize;
    }

	/* Do not allow write past end of page */
	if ((ops->ooboffs + ops->ooblen) > len)
    {
		FLASH_DEBUG("concerto_spi_nand_do_write_oob: "
		      "Attempt to write past end of page\n");
		return -EINVAL;
	}

	if (unlikely(ops->ooboffs >= len))
    {
		FLASH_DEBUG("concerto_spi_nand_do_write_oob: "
			"Attempt to start write outside oob\n");
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(to >= mtd->size ||
		     ops->ooboffs + ops->ooblen >
			((mtd->size >> host->page_shift) -
			 (to >> host->page_shift)) * len))
	{
		FLASH_DEBUG("concerto_spi_nand_do_write_oob: "
			"Attempt write beyond end of device\n");
		return -EINVAL;
	}

	/* Shift to get page */
	page = (int)(to >> host->page_shift);


	memset(host->oob_poi, 0xff, mtd->oobsize);
	concerto_spi_nand_fill_oob(mtd, ops->oobbuf, ops->ooblen, ops);

    ret = concerto_spi_nand_oob_write(mtd, page, NULL);
	memset(host->oob_poi, 0xff, mtd->oobsize);

	ops->oobretlen = ops->ooblen;

	return ret;
}

static int concerto_spi_nand_write_oob(struct mtd_info *mtd, loff_t to,
			  struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	ops->retlen = 0;

	/* Do not allow writes past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->size) 
    {
		FLASH_DEBUG("nand_read_oob: "
		      "Attempt read beyond end of device\n");
		return -EINVAL;
	}

    spi_access_lock();

	switch(ops->mode) 
    {
    	case MTD_OPS_PLACE_OOB:
    	case MTD_OPS_AUTO_OOB:
    	case MTD_OPS_RAW:
    		break;
    	default:
    		goto out;
	}

	if (!ops->datbuf)
    {   
		ret = concerto_spi_nand_do_write_oob(mtd, to, ops);
    }
	else
    {   
		ret = concerto_spi_nand_do_write_ops(mtd, to, ops);
    }
out:
    // tansform local error code to system error code
    if (ret)
    {
        FLASH_DEBUG("SNF DRV ERROR: write oob failed\n");
        ret = -EIO;
    }

    spi_access_unlock();
    return ret;
}

static int concerto_spi_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	struct concerto_snfc_host *host	= chip->priv;

	struct mtd_oob_ops ops;
	struct erase_info einfo;
    
	uint8_t buf[2] = { 0, 0 };
	int block, ret = 0;

	spi_access_lock();

	memset(&einfo, 0, sizeof(einfo));
	einfo.mtd = mtd;
	einfo.addr = ofs;
	einfo.len = 1 << host->erase_shift;
	concerto_spi_nand_erase(mtd, &einfo);

	/* Get block number */
	block = (int)(ofs >> host->erase_shift);

	ops.datbuf = NULL;
	ops.oobbuf = buf;
	ops.ooboffs = host->badblockpos;
	ops.len = ops.ooblen = 1;
	ops.mode = MTD_OPS_PLACE_OOB;

	ret = concerto_spi_nand_do_write_oob(mtd, ofs, &ops);
	if (ret)
    {
        mtd->ecc_stats.badblocks++;
    }

	spi_access_unlock();
	return ret;
}

static u32 panther_convert_to_dummy_id(u32 id)
{
    u32 dummy_id = 0xffffff;
    u8 codeID[3];

    codeID[0] = (id >> 8) & 0xFF;
    codeID[1] = id & 0xFF;
    codeID[2] = (id >> 16) & 0xFF;
    dummy_id = (codeID[0] << 16) | (codeID[1] << 8) | codeID[2];  // special ID

    return dummy_id;
}

static u32 panther_convert_to_inverse_id(u32 id)
{
    u32 inverse_id = 0xffffff;
    u8 codeID[3];

    codeID[0] = (id >> 16) & 0xFF;
    codeID[1] = id & 0xFF;
    codeID[2] = (id >> 8) & 0xFF;
    inverse_id = (codeID[0] << 16) | (codeID[1] << 8) | codeID[2];  // special ID

    return inverse_id;
}

static int concerto_spi_nand_flash_probe_common(struct mtd_info *mtd, u32 id)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;
    const struct concerto_spi_nand_flash_params *params;
    u32 dummy_id;   // special case with some chip, need to check
    unsigned int i;

    dummy_id = panther_convert_to_dummy_id(id);
    
    for (i = 0; i < ARRAY_SIZE(concerto_spi_nand_flash_table); i++) 
    {
        params = &concerto_spi_nand_flash_table[i];
        if (params->id == id) 
        {
            break;
        }

        if (params->id == dummy_id) 
        {
            id = dummy_id;
            break;
        }
    }

    // trying compare id table with inverse_id,
    // because big & little endian
    if (i == ARRAY_SIZE(concerto_spi_nand_flash_table)) 
    {
        FLASH_DEBUG("SNF: Unsupported nand flash ID = 0x%x\n", id);
        FLASH_DEBUG("SNF: Unsupported nand flash dummy = ID 0x%x\n", dummy_id);

        id = panther_convert_to_inverse_id(id);
        dummy_id = panther_convert_to_inverse_id(dummy_id);
        FLASH_DEBUG("SNF: inverse ID = 0x%x\n", id);
        FLASH_DEBUG("SNF: inverse dummy ID = 0x%x\n", dummy_id);
    }

    for (i = 0; i < ARRAY_SIZE(concerto_spi_nand_flash_table); i++) 
    {
        params = &concerto_spi_nand_flash_table[i];
        if (params->id == id) 
        {
            break;
        }
        
        if (params->id == dummy_id) 
        {
            id = dummy_id;
            break;
        }
    }
	
    if (i == ARRAY_SIZE(concerto_spi_nand_flash_table)) 
    {
        FLASH_DEBUG("SNF: Unsupported inverse nand flash ID 0x%x\n", id);
        FLASH_DEBUG("SNF: Unsupported inverse nand flash dummy ID 0x%x\n", dummy_id);
        return 1;
    }

    host->name = params->name;
    host->pagesize = mtd->writesize = params->page_size;
    host->oobsize = mtd->oobsize = params->oob_size;
    host->blocksize = mtd->erasesize = params->page_size * params->pages_per_block;
    host->chipsize = mtd->size = host->blocksize * params->nr_blocks;
    mtd->writebufsize = mtd->writesize;   // added for UBI/UBIFS back-port

    host->page_shift = ffs(mtd->writesize) - 1;
    host->erase_shift = ffs(mtd->erasesize) - 1;
    if (host->chipsize & 0xffffffff)
    {
        host->chip_shift = ffs((unsigned)host->chipsize) - 1;
    }
    else
    {
        host->chip_shift = ffs((unsigned)(host->chipsize >> 32)) + 32 - 1;
    }

    host->badblockpos = 0;
    host->oob_poi = host->buffer + host->pagesize;

    host->ecclayout = params->layout;

    host->ecclayout->oobavail = 0;
    for (i = 0; host->ecclayout->oobfree[i].length; i++)
    {
        host->ecclayout->oobavail += host->ecclayout->oobfree[i].length;
    }
    mtd->oobavail = host->ecclayout->oobavail;

    host->read_cmd_dummy_type = params->read_cmd_dummy_type;
    host->read_from_cache_dummy = params->read_from_cache_dummy;

    return 0;
}




/*****************************************************************************/


static int concerto_spi_nand_flash_probe(struct mtd_info *mtd, unsigned int bus, 
        unsigned int cs, unsigned int max_hz, unsigned int spi_mode)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;
    struct spi_slave *spi;
    int ret;
    u32 id = 0xffffff;

    spi = spi_setup_slave(bus, cs, max_hz, spi_mode);
    if(!spi) 
    {
        FLASH_DEBUG("SNF: Failed to set up slave\n");
        return 1;
    }

    ret = spi_claim_bus(spi);
    if(ret) 
    {
        FLASH_DEBUG("SNF: Failed to claim SPI bus: %d\n", ret);
        goto err_claim_bus;
    }

    host->spi = spi;

    ret = concerto_spi_nand_reset(mtd);
    if (ret) 
    {
        FLASH_DEBUG("SNF: cannot reset spi nand flash!!!\n");
        goto err_manufacturer_probe;
    }

    /* Read the ID codes */
    id = concerto_spi_nand_getJedecID(mtd);

    if (id == 0xffffff)
    {
        FLASH_DEBUG("SNF: get JedecID error!!!\n");
        goto err_read_id;
    }

    ret = concerto_spi_nand_flash_probe_common(mtd, id);

    if (ret) 
    {
        FLASH_DEBUG("SNF: Unsupported manufacturer 0x%x\n", id);
        goto err_manufacturer_probe;
    }

    // initial with ecc on
    concerto_spi_nand_internel_ecc_on(mtd, 1);
    // initial config about read/write speed
    if (host->read_ionum == 4 || host->write_ionum == 4)
    {
        concerto_spi_nand_quad_feature_on(mtd, 1);
    }
    else
    {
        concerto_spi_nand_quad_feature_on(mtd, 0);
    }
    FLASH_DEBUG("QE feature status = %d\n", concerto_spi_nand_get_quad_feature(mtd));

    FLASH_DEBUG("SNF: Got idcodes: 0x%06x\n", id);

    FLASH_DEBUG("SNF DRV: Detected %s with page size: %d, oob size: %d, total %d M bytes\n", 
        host->name, host->pagesize, host->oobsize, (u32)(host->chipsize >> 20));

    spi_release_bus(spi);

    return 0;

err_manufacturer_probe:
err_read_id:
    spi_release_bus(spi);
err_claim_bus:
    spi_free_slave(spi);
    return 1;
}



#endif





/*****************************************************************************/

static int concerto_spi_nand_init(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;

    host->buffer_page_addr = BUFFER_PAGE_ADDR_INVALID;
    host->buffer = kmalloc((NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE), GFP_KERNEL);
    if (!host->buffer)
    {
        return 1;
    }
    memset(host->buffer, 0xff, (NAND_MAX_PAGESIZE +	NAND_MAX_OOBSIZE));

    host->read_ionum = 4;
    host->write_ionum = 4;

    host->controller = &host->hwcontrol;

    host->read_cmd_dummy_type = READ_CMD_DUMMY_END;
    host->read_from_cache_dummy = 0;

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    spi_panther_init_secure();
#endif

    return 0;
}
/*****************************************************************************/

#if 0
static void concerto_spi_nand_set_pinmux(void)
{
    REG32(0xbf156008) = 0;
}
#endif

static void concerto_spi_nand_inithw(struct mtd_info *mtd)
{
    //concerto_spi_nand_set_pinmux();
}

static int __init get_ci_offset(char *p)
{
    //get ci_offset first
    ci_offset = simple_strtol(p, NULL, 16);

    //set the partition offet
    if(ci_offset != 0)
    {
        concerto_spi_nand_partitions[CI_PART_INDEX].offset = ci_offset;
        concerto_spi_nand_partitions[CDB_PART_INDEX].offset = ci_offset - SNF_CDB_LENTH;
        concerto_spi_nand_partitions[FIRMWARE_PART_INDEX].offset = ci_offset + SNF_COMBINED_LENTH;
        concerto_spi_nand_partitions[KERNEL_PART_INDEX].offset = concerto_spi_nand_partitions[FIRMWARE_PART_INDEX].offset;
        concerto_spi_nand_partitions[ROOTFS_PART_INDEX].offset = concerto_spi_nand_partitions[KERNEL_PART_INDEX].offset + SNF_LINUX_LENTH;
    }
    else
    {
        printk("!!!!ci_offset = 0x%x\n", ci_offset);
    }
}
early_param("ci_offset", get_ci_offset);

static char* sp = NULL; /* the start position of the string */
static char* panther_spi_nand_strtok(char *str, char *delim)
{
    int i = 0;
    char *p_start = sp;
    int len = strlen(delim);
 
    /* check in the delimiters */
    if (len == 0)
    {
        FLASH_DEBUG("Delimiters are empty...\n");
    }
 
    /* if the original string has nothing left */
    if (!str && !sp)
    {
        return NULL;
    }
 
    /* initialize the sp during the first call */
    if (str && !sp)
    {
        sp = str;
        p_start = sp;
    }

    /* find the start of the substring, skip delimiters */
    while (true)
    {
        for (i = 0; i < len; i++)
        {
            if (*p_start == delim[i])
            {
                p_start++;
                break;
            }
        }
 
        if (i == len)
        {
            sp = p_start;
            break;
        }
    }

    /* return NULL if nothing left */
    if (*sp == '\0')
    {
        sp = NULL;
        return sp;
    }

    /* find the end of the substring, and
        replace the delimiter with null */
    while (*sp != '\0')
    {
        for(i = 0; i < len; i ++)
        {
            if (*sp == delim[i])
            {
                *sp = '\0';
                break;
            }
        }
        sp++;
        
        if (i < len)
        {
            break;
        }
    }

    return p_start;
}

static int panther_spi_is_value_in_array(int val, int *arr, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (arr[i] == val)
        {
            return 1;
        }
    }

    return 0;
}

static void panther_spi_nand_fit_page_size(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;
    struct mtd_partition *parts = NULL;
    uint64_t block_size = SNF_MIN_BLOCK_SIZE;
    uint64_t part_shift = 0;
    //uint64_t size = 0;
    //uint64_t offset = 0;
    int i;
	
    // put boot and cdb partition into correct place,
    // because different NAND flash have different page size
    part_ratio = (host->blocksize / block_size);

    if (part_ratio == 1)
    {
        return;
    }

    for (i = 0; i < ARRAY_SIZE(concerto_spi_nand_partitions); i++)
    {
        parts = &concerto_spi_nand_partitions[i];

        if (i < FIRMWARE_PART_INDEX)
        {
            part_shift += (part_ratio - 1) * parts->size;        
            parts->offset *= part_ratio;
            parts->size *= part_ratio;
        }
        else
        {
            parts->offset += part_shift;
        }
    }
    
    parts = &concerto_spi_nand_partitions[i - 1];
    parts->size = (mtd->size - parts->offset);
    parts = &concerto_spi_nand_partitions[i - 3];
    parts->size = (mtd->size - parts->offset);
}

static void panther_spi_nand_adjust_partitions(struct mtd_info *mtd)
{
    int i, j, blk_idx, bbt_idx;
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;
    struct mtd_partition *parts = NULL;
    uint64_t chip_size = mtd->size;
    uint64_t block_size = SNF_MIN_BLOCK_SIZE * part_ratio;

    for (i = 0; i < ARRAY_SIZE(concerto_spi_nand_partitions); i++)
    {
        parts = &concerto_spi_nand_partitions[i];

        FLASH_DEBUG("===============================\n");
        FLASH_DEBUG("parts->name = %s\n", parts->name);
        FLASH_DEBUG("Before Shift.................\n");
        FLASH_DEBUG("parts->offset = 0x%08llx\n", parts->offset);
        FLASH_DEBUG("parts->size = 0x%08llx\n", parts->size);
        blk_idx = (u32)(parts->offset >> host->erase_shift);

        bbt_idx = 0;
        for(j = 0; j <= blk_idx; ++j)
        {
            if(bad_block_table[bbt_idx] == j)
            {
                blk_idx++;
                FLASH_DEBUG("find bad_block = %d, %d\n", j, bad_block_table[bbt_idx]);
                bbt_idx++;
            }
        }
        parts->offset += bbt_idx * block_size;
        if ((parts->size + parts->offset) > chip_size)
        {
            //FLASH_DEBUG("....adjust size\n");
            parts->size = (chip_size - parts->offset);
        }
        FLASH_DEBUG("After Shift..................\n");
        FLASH_DEBUG("parts->offset = 0x%08llx\n", parts->offset);
        FLASH_DEBUG("parts->size = 0x%08llx\n", parts->size);
    }
}

static void panther_spi_nand_adapt_combined_partition(struct mtd_info *mtd)
{
    struct nand_chip *chip = mtd->priv;
    struct concerto_snfc_host *host	= chip->priv;
    struct mtd_partition *parts = NULL;
    struct mtd_partition *ci_part = NULL;
    uint64_t offset = 0;
    u8 hdr_buf[0x40];
    u16 ci_magic;
    int i = 0, j = 0;
    int hdr_blk_idx = 0;

    spi_access_lock();

    parts = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX - 1];
    hdr_blk_idx = (u32)(parts->offset >> host->erase_shift);
    // need to handle bad block here,
    // read bad block will get wrong magic number, and will cause functions get failed after this function
    for(i = 0; i <= hdr_blk_idx; ++i)
    {
        if(bad_block_table[j] == i)
        {
            hdr_blk_idx++;
            FLASH_DEBUG("find bad_block = %d, %d\n", i, bad_block_table[j]);
            j++;
        }
    }

    change_aes_enable_status(DISABLE_SECURE);
    concerto_spi_nand_page_read(mtd, 64 * hdr_blk_idx, 0, hdr_buf, 0x40);
    change_aes_enable_status(ENABLE_SECURE_OTP);
    ci_magic = cpu_to_be16(*((u16 *)hdr_buf));
    FLASH_DEBUG("hdr_blk_idx = %d\n", hdr_blk_idx);
    FLASH_DEBUG("CI MAGIC = 0x%x\n", ci_magic);
    if (ci_magic == CI_MAGIC)
    {
        spi_access_unlock();
        return;
    }

    FLASH_DEBUG("firmware type is not combined image, adjust partitions\n");
    ci_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX - 1];
    for (i = FIRMWARE_PART_INDEX; i < ARRAY_SIZE(concerto_spi_nand_partitions); i++)
    {
        parts = &concerto_spi_nand_partitions[i];

        if (i == FIRMWARE_PART_INDEX)
        {
            parts->offset = offset = ci_part->offset;
            continue;
        }

        parts->offset = offset;
        offset = (parts->offset + parts->size);
    }

    FLASH_DEBUG("after function: %s\n", __FUNCTION__);
    for (i = 0; i < ARRAY_SIZE(concerto_spi_nand_partitions); i++)
    {
        parts = &concerto_spi_nand_partitions[i];
        FLASH_DEBUG("parts->name = %s\n", parts->name);
        FLASH_DEBUG("parts->offset = 0x%08llx\n", parts->offset);
        FLASH_DEBUG("parts->size = 0x%08llx\n", parts->size);
    }

    spi_access_unlock();
}

#ifdef CONFIG_MTD_SPLIT_UIMAGE_FW
static void panther_spi_nand_adapt_kernel_partition(struct mtd_info *mtd)
{
    struct mtd_partition *parts = NULL;
    u32 block_size = (SNF_MIN_BLOCK_SIZE * part_ratio);
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

    for (i = (FIRMWARE_PART_INDEX + 1); i < ARRAY_SIZE(concerto_spi_nand_partitions); i++)
    {
        parts = &concerto_spi_nand_partitions[i];

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

    parts = &concerto_spi_nand_partitions[i - 1];   //rootfs
    parts->size = (mtd->size - parts->offset);
    parts = &concerto_spi_nand_partitions[i - 3];   // firmware
    parts->size = (mtd->size - parts->offset);

    FLASH_DEBUG("after function: %s\n", __FUNCTION__);
    for (i = 0; i < ARRAY_SIZE(concerto_spi_nand_partitions); i++)
    {
        parts = &concerto_spi_nand_partitions[i];
        FLASH_DEBUG("parts->name = %s\n", parts->name);
        FLASH_DEBUG("parts->offset = 0x%08llx\n", parts->offset);
        FLASH_DEBUG("parts->size = 0x%08llx\n", parts->size);
    }
}
#endif

static void panther_spi_nand_rename_rootfs(struct mtd_info *mtd)
{
    struct mtd_partition *rootfs_part = NULL;
    char *init_info;

    rootfs_part = &concerto_spi_nand_partitions[FIRMWARE_PART_INDEX + 2];

    init_info = strstr(arcs_cmdline, "ubi.mtd=");
    if (init_info != NULL)
    {
        rootfs_part->name = "ubi";
        FLASH_DEBUG("%s: Rename rootfs to ubi\n", __FUNCTION__);
    }
}


static void panther_spi_nand_parser_badblock_list(void)
{
    char *token;
    char *delim = ",";
    char *init_info;
    char list_bad_block_str[BAD_BLOCK_STR_SIZE];
    u32 i;
    for(i = 0; i < MAX_BAD_BLOCK_NUM; ++i)
    {
        // initialize bad_block_table
        bad_block_table[i] = 0xFFFFFFFF;
    }

    init_info = strstr(arcs_cmdline, "bad_list=");
    if (init_info == NULL)
    {
        FLASH_DEBUG("bad_list = NULL, function: %s return!!\n", __FUNCTION__);
        return;
    }
    sscanf(&init_info[9], "%s", list_bad_block_str);
    FLASH_DEBUG("list_bad_block_str = %s\n", list_bad_block_str);

    bad_block_length = 0;
    token = panther_spi_nand_strtok(list_bad_block_str, delim);
    while ((token != NULL) && strcmp(token, "end"))
    {
        sscanf(token, "%d", &bad_block_table[bad_block_length]);
        FLASH_DEBUG("bad_block_table[%d] = %d\n", bad_block_length, bad_block_table[bad_block_length]);
        bad_block_length++;
        // check whether the bad block list stringhave next token
        token = panther_spi_nand_strtok(NULL, delim);
    }
}

static struct mtd_info *mtd_global = NULL;
static int panther_spi_nand_start_virtul_access(void)
{
    struct mtd_info *mtd = mtd_global;
    struct mtd_partition *ci_part = NULL;
    uint64_t size, actual_size, offset;
    u8 *buf;
    size_t retlen;

    ci_part = &concerto_spi_nand_partitions[6];

    size = ci_part->size;
    offset = ci_part->offset;

    printk("panther_spi_nand_start_virtul_access 0x%08llx@0x%08llx\n", ci_part->size, ci_part->offset);

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
        printk(KERN_EMERG "panther_spi_nand_start_virtul_access vmalloc failed\n");
        return 0;
    }

    virtual_access_buf = buf;

    spi_access_lock();

    while(size > 0)
    {
        concerto_spi_nand_read(mtd, offset, mtd->writesize, &retlen, buf);
        size -= mtd->writesize;
        offset += mtd->writesize;
        buf += mtd->writesize;
    }

    virtual_access_actual_size = actual_size;
    virtual_access_size = ci_part->size;
    virtual_access_offset = ci_part->offset;

    spi_access_unlock();

    printk("panther_spi_nand_start_virtul_access done\n");

    return 0;
}

static int concerto_spi_nand_probe(struct platform_device * pltdev)
{
	int	result = 0;
    u32 boot_type;

	struct concerto_snfc_host *host;
	struct nand_chip  *chip;
	struct mtd_info	  *mtd;
	struct mtd_part_parser_data ppdata = {};

#ifdef CONFIG_MTD_CMDLINE_PARTS
    struct mtd_partition *parts = NULL;
    int nr_parts = 0;
    static const char *part_probes[] = {"cmdlinepart", NULL,};
#endif

	int	size = sizeof(struct concerto_snfc_host) + sizeof(struct nand_chip)
		+ sizeof(struct	mtd_info);

    // check boot from NAND or NOR
    boot_type = otp_get_boot_type();
    if (boot_type==BOOT_FROM_NOR)
        return -1;

    FLASH_DEBUG("concerto_spi_nand_init start.\n");
    
	host = kmalloc(size, GFP_KERNEL);
	if (!host)
	{
		dev_err(&pltdev->dev, "failed to allocate device structure.\n");
		return -ENOMEM;
	}

	memset((char *)host, 0,	size);
	platform_set_drvdata(pltdev, host);

	host->dev  = &pltdev->dev;
	host->chip = chip =	(struct	nand_chip *)&host[1];
	host->mtd  = mtd  =	(struct	mtd_info *)&chip[1];


	mtd->priv  = chip;
	mtd->owner = THIS_MODULE;
	mtd->name  = (char*)(pltdev->name);
	mtd->dev.parent = &pltdev->dev;

    chip->priv = host;

    load_erased_page_data_from_boot_cmdline();

    concerto_spi_nand_inithw(mtd);
    panther_spi_nand_parser_badblock_list();

	if (concerto_spi_nand_init(mtd))
	{
		dev_err(&pltdev->dev, "failed to allocate device buffer.\n");
		result = -ENOMEM;
		goto err;
	}

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    pdma_init();
    init_spi_data();
#endif
    
    if (concerto_spi_nand_flash_probe(mtd, CONFIG_SF_DEFAULT_BUS, CONFIG_SF_DEFAULT_CS, 
                                CONFIG_SNF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE))
    {
		dev_err(&pltdev->dev, "failed to spi nand probe.\n");
		result = -ENXIO;
		goto err;
    }

	host->state = FL_READY;

	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->_erase = concerto_spi_nand_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = concerto_spi_nand_read;
	mtd->_write = concerto_spi_nand_write;
	mtd->_read_oob = concerto_spi_nand_read_oob;
	mtd->_write_oob = concerto_spi_nand_write_oob;
	mtd->_sync = NULL;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_suspend = NULL;
	mtd->_resume = NULL;
	mtd->_block_isbad = concerto_spi_nand_block_is_bad;
	mtd->_block_markbad = concerto_spi_nand_block_markbad;
    mtd->ecclayout = host->ecclayout;
    mtd->name = CONCERTO_SPI_NAND_FLASH_NAME;

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

    panther_spi_nand_fit_page_size(mtd);    // about page size
    // Don't resize in recovery mode.
#ifndef CONFIG_RAMFS_RECOVERY_IMAGE
    panther_spi_nand_adapt_combined_partition(mtd);   // about combined image partition
#endif
#ifdef CONFIG_MTD_SPLIT_UIMAGE_FW
    panther_spi_nand_adapt_kernel_partition(mtd);   // about kernel real size
#endif
    panther_spi_nand_adjust_partitions(mtd);    // about bad block
    panther_spi_nand_rename_rootfs(mtd);
    ppdata.of_node = pltdev->dev.of_node;
	result = mtd_device_parse_register(mtd, NULL, &ppdata,
					concerto_spi_nand_partitions, ARRAY_SIZE(concerto_spi_nand_partitions));
#endif

    FLASH_DEBUG("concerto_spi_nand_init end.\n");

    mtd_global = mtd;
    /* nand flash support virtual access */
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


void concerto_spi_nand_release(struct mtd_info *mtd)
{
	//del_mtd_partitions(mtd);
	//del_mtd_device(mtd);
    mtd_device_unregister(mtd);
}


/*****************************************************************************/
int	concerto_spi_nand_remove(struct platform_device *pltdev)
{
	struct concerto_snfc_host *host	= platform_get_drvdata(pltdev);

	concerto_spi_nand_release(host->mtd);
	kfree(host);
	platform_set_drvdata(pltdev, NULL);

	return 0;
}
/*****************************************************************************/
static void	concerto_spi_nand_pltdev_release(struct device *dev)
{
}
/*****************************************************************************/
static struct platform_driver concerto_spi_nand_pltdrv =
{
	.driver.name   = "concerto spi nand",
	.probe	= concerto_spi_nand_probe,
	.remove	= concerto_spi_nand_remove,
};
/*****************************************************************************/
static struct platform_device concerto_spi_nand_pltdev =
{
	.name			= "concerto spi nand",
	.id				= -1,

	.dev.platform_data	   = NULL,
	.dev.release		   = concerto_spi_nand_pltdev_release,

	.num_resources	= 0,
	.resource		= NULL,
};

/*****************************************************************************/

#ifdef CONFIG_SYSCTL
static struct ctl_table_header *spi_nand_sysctl_header;
#endif
static int __init concerto_spi_nand_module_init(void)
{
	int	result = 0;
	
	//FLASH_DEBUG("\nMT concerto Spi Nand Flash Controller Device Driver, Version 1.00\n");

	result = platform_driver_register(&concerto_spi_nand_pltdrv);
	if (result < 0)
	{
		return result;
	}

	result = platform_device_register(&concerto_spi_nand_pltdev);
	if (result < 0)
	{
		platform_driver_unregister(&concerto_spi_nand_pltdrv);
		return result;
	}

#ifdef CONFIG_SYSCTL
	spi_nand_sysctl_header = register_sysctl_table(spi_nand_sysctl_table);
	if(spi_nand_sysctl_header  == NULL)
		return -ENOMEM;
#endif
	return result;
}

/*****************************************************************************/

static void	__exit concerto_spi_nand_module_exit	(void)
{
	platform_driver_unregister(&concerto_spi_nand_pltdrv);
	platform_device_unregister(&concerto_spi_nand_pltdev);

#ifdef CONFIG_SYSCTL
	unregister_sysctl_table(spi_nand_sysctl_header);
	spi_nand_sysctl_header = NULL;
#endif
}
/*****************************************************************************/

module_init(concerto_spi_nand_module_init);
module_exit(concerto_spi_nand_module_exit);


/*****************************************************************************/






















