/********************************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd.                                                  */
/* Montage Proprietary and Confidential                                                     */
/* Copyright (c) 2014 Montage Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#ifndef __SYMPHONY_SPI_H__
#define __SYMPHONY_SPI_H__


#include <linux/mtd/mtd.h>
#include <linux/slab.h>
#include <linux/sched.h>

#define FLS_DBG
#ifdef FLS_DBG
#define FLASH_DEBUG(x...) printk(x)
#else
#define FLASH_DEBUG(x...) do{}while(0);
#endif

#define CI_MAGIC    0x4349     /* { 'C', 'I' }  */ 
#define UBI_EC_HDR_MAGIC  0x55424923

typedef int BOOL;
typedef void * handle_t;

#ifndef  FALSE
#define FALSE (0)             
#endif

#ifndef  TRUE
#define TRUE (1)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define RET_CODE s32
#define SUCCESS ((s32)0)       
#define ERR_FAILURE ((s32)-1)  
#define ERR_TIMEOUT ((s32)-2)      
#define ERR_PARAM ((s32)-3) 
#define ERR_STATUS ((s32)-4)  
#define ERR_BUSY ((s32)-5)
#define ERR_NO_MEM ((s32)-6)  
#define ERR_NO_RSRC ((s32)-7)
#define ERR_HARDWARE ((s32)-8) 
#define ERR_NOFEATURE ((s32)-9)       


#define CONFIG_SF_DEFAULT_BUS       0
#define CONFIG_SF_DEFAULT_CS        0
#define CONFIG_SF_DEFAULT_MODE      0

#if defined(CONFIG_PANTHER_FPGA)
#define CONFIG_SF_DEFAULT_SPEED     10000000
#define CONFIG_SNF_DEFAULT_SPEED    10000000
#else
#define CONFIG_SF_DEFAULT_SPEED     50000000
#define CONFIG_SNF_DEFAULT_SPEED    80000000
#endif



#define KBYTES    1024
#define MBYTES    (KBYTES * KBYTES)


#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define malloc(a) kmalloc(a, GFP_KERNEL)
#define free(a)   kfree(a)
 
#define BOOT_CFG         0xBF140020

#define REG32(x)    *((volatile u32 *) (x))

#define CONFIG_SYS_HZ				10000
#define SPI_TIME_OUT   20000
#define SPI_FLASH_PROG_TIMEOUT		(2 * CONFIG_SYS_HZ)
#define SPI_FLASH_PAGE_ERASE_TIMEOUT	(5 * CONFIG_SYS_HZ * 100)



#define SPI_FLASH_CMD_RD_MAN_ID      0x9F
#define SPI_FLASH_CMD_RD_DEV_ID      0x90
#define SPI_FLASH_CMD_WR_EN          0x06
#define SPI_FLASH_CMD_WR_DI          0x04
#define SPI_FLASH_CMD_RD_SR          0x05
#define SPI_FLASH_CMD_RD_CR          0x35
#define SPI_FLASH_CMD_WR_SR          0x01
#define SPI_FLASH_CMD_WR_CR          0x31
#define SPI_FLASH_CMD_SINGLE_WR      0x02
#define SPI_FLASH_CMD_NR_RD          0x03
#define SPI_FLASH_CMD_FAST_RD        0x0B
#define SPI_FLASH_CMD_SEC_ERASE      0x20
#define SPI_FLASH_CMD_BLK_ERASE      0xD8

#define SPI_NAND_FLASH_CMD_RD_MAN_ID                0x9F
#define SPI_NAND_FLASH_CMD_WR_EN                    0x06
#define SPI_NAND_FLASH_CMD_WR_DI                    0x04
#define SPI_NAND_FLASH_CMD_RD_SR                    0x0F
#define SPI_NAND_FLASH_CMD_WR_SR                    0x1F
#define SPI_NAND_FLASH_CMD_WR_RESET                 0xFF
#define SPI_NAND_FLASH_ADDR_PT                      0xA0
#define SPI_NAND_FLASH_ADDR_FT                      0xB0
#define SPI_NAND_FLASH_ADDR_ST                      0xC0
#define SPI_NAND_FLASH_CMD_BLK_ERASE                0xD8
#define SPI_NAND_FLASH_CMD_PG_EXC                   0x10
#define SPI_NAND_FLASH_CMD_LOAD                     0x02
#define SPI_NAND_FLASH_CMD_QUAD_LOAD                0x32
#define SPI_NAND_FLASH_CMD_PAGE_RD_FRM_CACHE        0x0B
#define SPI_NAND_FLASH_CMD_PAGE_RD_DUAL_FRM_CACHE   0x3B
#define SPI_NAND_FLASH_CMD_PAGE_RD_QUAD_FRM_CACHE   0x6B
#define SPI_NAND_FLASH_CMD_PAGE_RD_TO_CACHE         0x13


typedef struct spi_pins_cfg
{
    /*!
        miso1 --> spiio(n) n=0 or 1
      */
  u8 miso1_src;
    /*!
        miso0 --> spiio(n) n=0 or 1
      */    
  u8 miso0_src;
    /*!
        spiio1 <-- mosi(n) n=0 or 1
      */    
  u8 spiio1_src;
    /*!
        spiio0 <-- mosi(n) n=0 or 1
      */    
  u8 spiio0_src;

  u8 miso2_src;
  u8 miso3_src;
  u8 spiio2_src;
  u8 spiio3_src;
} spi_pins_cfg_t;

/*!
   The structure is spi pins direction
*/
typedef struct spi_pins_dir
{
    /*!
        00:input, 01:output, other:bidirectional
      */
    u8 spiio1_dir;
    /*!
        00:input, 01:output, other:bidirectional
      */
    u8 spiio0_dir;

    u8 spiio2_dir;
    u8 spiio3_dir;
} spi_pins_dir_t;


typedef struct spi_cfg
{
    u16 bus_clk_mhz;
    u8 bus_clk_delay;
    u8 lock_mode;
    u8 spi_id;
    u8 io_num;
    u8 op_mode;
    spi_pins_cfg_t pins_cfg[3];
    spi_pins_dir_t  pins_dir[3];
} spi_cfg_t;

struct spi_bus
{
    void *p_base;
    void *p_priv;
};

struct spi_slave 
{
	unsigned int bus;
	unsigned int cs;
};

typedef struct spi_cmd_cfg
{
    u8 cmd0_ionum;
    u8 cmd0_len;
    u8 cmd1_ionum;
    u8 cmd1_len;
    u8 dummy;
} spi_cmd_cfg_t;

struct spi_read_cfg
{
    u32 read_cmd_dummy_type;    // because read command of some manufacturer's chip have different format
    u32 read_from_cache_dummy;  // same as above
};


/*!
    The structure is used to configure the 2-wire bus driver.    
  */
typedef struct lld_spi {
    
  u32 used;
  /*!
      The private data of low level driver.
    */
  void *p_priv;
  /*!
      The low level function to implement the high level interface "i2c_write".
    */
  RET_CODE (*read)(struct lld_spi *p_lld, u8 *cmd_buf, u32 cmd_len, 
    spi_cmd_cfg_t * p_cmd_cfg, u8 *p_data_buf, u32 data_len);
  /*!
      The low level function to implement the high level interface "i2c_read".
    */  
  RET_CODE (*write)(struct lld_spi *p_lld, u8 *cmd_buf, u32 cmd_len, 
    spi_cmd_cfg_t * p_cmd_cfg, u8 *p_data_buf, u32 data_len);

  /*!
      The low level function to implement the high level interface "i2c_read".
    */
  RET_CODE (*reset)(struct lld_spi *p_lld);
  /*!
      The low level function to implement the high level interface "i2c_read".
    */
  RET_CODE (*soft_reset)(struct lld_spi *p_lld);
}lld_spi_t;



/************************************************************************
 * Defination of Reset register
 ************************************************************************/
/*!
  comments
  */
#define R_RST_REQ(n)                              (0xBF510000 + (n) * (0x4))
/*!
  comments
  */
#define R_RST_CTRL(n)                             (0xBF510010 + (n) * (0x4))
/*!
  comments
  */
#define R_RST_ALLOW(n)                            (0xBF510020 + (n) * (0x4))

/************************************************************************ 
 * SPI Controller
 ************************************************************************/
#define SPIN_BASE_ADDR                          0xBF002000 
#define R_SPIN_TXD(i)                           ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x0))
#define R_SPIN_RXD(i)                           ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x0))
#define R_SPIN_CH0_BAUD(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x100))
#define R_SPIN_CH0_MODE_CFG(i)                  ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x104))
#define R_SPIN_CH1_BAUD(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x108))
#define R_SPIN_CH1_MODE_CFG(i)                  ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x10C))
#define R_SPIN_CH2_BAUD(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x110))
#define R_SPIN_CH2_MODE_CFG(i)                  ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x114))
#define R_SPIN_CH3_BAUD(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x118))
#define R_SPIN_CH3_MODE_CFG(i)                  ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x11C))
#define R_SPIN_TC(i)                            ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x120))
#define R_SPIN_CTRL(i)                          ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x124))
#define R_SPIN_INT_CFG(i)                       ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x128))
#define R_SPIN_SMPL_MODE(i)                     ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x12C))
#define R_SPIN_PIN_MODE(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x130))
#define R_SPIN_PIN_CTRL(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x134))
#define R_SPIN_CH_MUX(i)                        ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x138))
#define R_SPIN_RDY_DLY(i)                       ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x13C))
#define R_SPIN_STA(i)                           ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x140))
#define R_SPIN_INT_STA(i)                       ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x144))
#define R_SPIN_CMD_FIFO(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x148))
#define R_SPIN_ACCCTRL_RAM(i)                   ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x200))
#define R_SPIN_ACC_CTRL(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x300))
#define R_SPIN_ENC_KEY(i)                       ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x304))
#define R_SPIN_ADDR_BOUNDARY0(i)                ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x310))
#define R_SPIN_ADDR_BOUNDARY1(i)                ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x314))
#define R_SPIN_ADDR_BOUNDARY2(i)                ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0x318))
#define R_SPIN_MEM_CMD(i)                       ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0xC004))
#define R_SPIN_MEM_CTRL(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0xC008))
#define R_SPIN_MEMMODE1(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0xC00C))
#define R_SPIN_MEMMODE2(i)                      ((SPIN_BASE_ADDR) + (i) * (0x10000) + (0xC010))

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode);

//	void spi_init (void);

void spi_free_slave(struct spi_slave *slave);

int spi_claim_bus(struct spi_slave *slave);

void spi_release_bus(struct spi_slave *slave);

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
extern void init_spi_data(void);
#endif

extern int concerto_spi_cmd_read(struct spi_slave *spi, u8 *cmd, u32 cmd_len,
                        void *data, u32 data_len, struct spi_read_cfg* read_cfg, u32 dma_sel);

extern int concerto_spi_cmd_write(struct spi_slave *spi, u8 *cmd, u32 cmd_len,
                        const void *data, u32 data_len, u32 dma_sel);

extern int concerto_spi_cmd(struct spi_slave *spi, u8 cmd, void *response, u32 len);
extern int concerto_spi_cmd_write_enable(struct spi_slave *spi);
extern int concerto_spi_cmd_write_disable(struct spi_slave *spi);
extern int concerto_spi_cmd_wait_ready(struct spi_slave *spi, u32 timeout);
extern int concerto_spi_read_common(struct spi_slave *spi, u8 *cmd, u32 cmd_len, void *data, u32 data_len);
extern void concerto_spi_set_trans_ionum(struct spi_slave *slave, u8 io_num);

extern void panther_spi_restore_erased_page(u8 *buf, u32 size);

extern void spi_panther_init_secure(void);
extern u32 init_secure_settings(u32 action);
extern void read_otp_key(void);
extern void change_aes_enable_status(u32 aes_status);

#endif //__CONCERTO_SPI_H__

