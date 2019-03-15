/********************************************************************************************/
/* Montage Technology (Shanghai) Co., Ltd.                                                  */
/* Montage Proprietary and Confidential                                                     */
/* Copyright (c) 2014 Montage Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "symphony_spi.h"
#include "erased_page.h"
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pdma.h>
#include <asm/mach-panther/otp.h>

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
#include <asm/mach-panther/pdma.h>
#endif

/*=============================================================================+
| Extern Function/Variables                                                    |
+=============================================================================*/
#ifdef CONFIG_MTD_PDMA_SPI_FLASH
extern int efuse_read(u8 *bufp, u32 index_start, u32 element_num);
extern void otp_read_config(void);
extern int otp_parse_config(int shift_val);
#endif
u32 is_enable_secure_boot = 0;
u32 aes_enable_status = 0;

lld_spi_t gSpiLLD[3]; 
spi_cfg_t gSpiCfg[3];
struct spi_slave gSpiSlave[3];

static DEFINE_SPINLOCK(spi_lock);
static DECLARE_WAIT_QUEUE_HEAD(spi_wq);
static volatile pid_t spi_lock_holder = -1;
static volatile int spi_lock_cnt = 0;

void spi_access_lock(void)
{
    DECLARE_WAITQUEUE(wait, current);
    unsigned long flags;

    spin_lock_irqsave(&spi_lock, flags);

retry:
    if(spi_lock_cnt==0)
    {
        spi_lock_holder = task_pid_nr(current);
        spi_lock_cnt++;
    }
    else if(spi_lock_holder == task_pid_nr(current))
    {
        spi_lock_cnt++;
    }
    else
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        add_wait_queue(&spi_wq, &wait);

        spin_unlock_irqrestore(&spi_lock, flags);
        schedule();

        remove_wait_queue(&spi_wq, &wait);
        spin_lock_irqsave(&spi_lock, flags);
        goto retry;
    }

    spin_unlock_irqrestore(&spi_lock, flags);
}

void spi_access_unlock(void)
{
    unsigned long flags;

    spin_lock_irqsave(&spi_lock, flags);

    if(spi_lock_holder != task_pid_nr(current))
    {
        panic("spi_access_unlock (%d,%d)\n", spi_lock_holder, task_pid_nr(current));
    }

    spi_lock_cnt--;
    if(0==spi_lock_cnt)
    {
        spi_lock_holder = -1;
        wake_up(&spi_wq);
    }

    spin_unlock_irqrestore(&spi_lock, flags);
}


#if defined(CONFIG_PM)

static unsigned long spi_regval[13];
void symphony_spi_suspend(void)
{
    unsigned long *spireg = (unsigned long *) SPI_BASE;
    int i;

    spi_access_lock();

    for(i=0;i<13;i++)
        spi_regval[i] = spireg[i];
}

void symphony_spi_resume(void)
{
    unsigned long *spireg = (unsigned long *) SPI_BASE;
    int i;

    for(i=0;i<13;i++)
        spireg[i] = spi_regval[i];

    spi_access_unlock();
}

#endif

static void spi_concerto_ioctrl_set_ionum(u8 spi_id, u8 ionum)
{
    u32 dtmp = REG32(R_SPIN_CTRL(spi_id));
    dtmp &= ~(0x3 << 14);
    dtmp |= ((ionum - 1) & 0x3) << 14;   //該配置從模式配置移植控制寄存ん
    REG32(R_SPIN_CTRL(spi_id)) = dtmp;
}

#if 0
static void spi_concerto_ioctrl_set_pins_cfg(u8 spi_id, u8 io_num)
{
    u32 dtmp = 0;
    dtmp = 0x00e400e4;

    REG32(R_SPIN_PIN_CFG_IO(spi_id)) = dtmp;
}
#endif


#define SPI_PHY_CLK_00        330
#define SPI_PHY_CLK_01        166
#define SPI_PHY_CLK_11        24

#define R_CLK_SPI_CFG 0xBF50004C


/*!
  define SPI delay time in us under polling mode
  */
#define SPI_TIME_OUT   20000

/*!
  define SPI delay function in us
  */
//#define SPI_DELAY_US mtos_task_delay_us


//inline static void spi_concerto_ioctrl_set_pins_cfg(u8 spi_id);
static void spi_concerto_ioctrl_set_pins_dir(u8 spi_id,  u8 io_num);
static void spi_concerto_soft_reset(u8 spi_id)
{
}


inline static BOOL spi_concerto_is_txd_fifo_full(u8 spi_id)
{
    u32 dtmp = 0;
    dtmp = REG32(R_SPIN_STA(spi_id));
    if ((31 - ((dtmp >> 8) & 0x3f)) == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

inline static BOOL spi_concerto_is_rxd_fifo_empty(u8 spi_id)
{
    u32 dtmp = 0;
    dtmp = REG32(R_SPIN_STA(spi_id));
    if ((dtmp & 0x3f) > 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

inline static BOOL spi_concerto_is_trans_complete(u8 spi_id)
{
    u32 dtmp_sta = 0;
    dtmp_sta = REG32(R_SPIN_STA(spi_id));
    if (!((dtmp_sta >> 16) & 0x1))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

inline static BOOL spi_converto_is_write_register(u8 *p_cmd_buf)
{
    u8 cmd_dtmp = 0;
    u8 cmd_dtmp2 = 0;

#define SPI_FLASH_CMD_WR_SR          0x01
#define SPI_FLASH_CMD_WR_CR          0x31

    cmd_dtmp = *p_cmd_buf;

    // NOR flash
    if (cmd_dtmp == SPI_FLASH_CMD_WR_SR || cmd_dtmp == SPI_FLASH_CMD_WR_CR)
    {
        return TRUE;
    }
    

    // NAND flash
    if (cmd_dtmp == SPI_NAND_FLASH_CMD_WR_SR)
    {
        cmd_dtmp2 = *(p_cmd_buf + 1);
        if (cmd_dtmp2 == SPI_NAND_FLASH_ADDR_PT 
                || cmd_dtmp2 == SPI_NAND_FLASH_ADDR_FT
                || cmd_dtmp2 == SPI_NAND_FLASH_ADDR_ST)
        {
            return TRUE;
        }
    }

    return FALSE;
}

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
struct pdma_intr_data spi_data;
static void spi_panther_pdma_done(u32 channel, u32 intr_status)
{
    spin_lock(&spi_data.lock);
    spi_data.ch_received |= intr_status;
    spin_unlock(&spi_data.lock);
    wake_up_interruptible(&spi_data.waitq);    
}

void init_spi_data(void)
{
    init_waitqueue_head(&spi_data.waitq);
    spin_lock_init(&spi_data.lock);
    pdma_callback_register(PDMA_CH_SPI_TX, spi_panther_pdma_done);
    pdma_callback_register(PDMA_CH_SPI_RX, spi_panther_pdma_done);
}

static u32 pdma_get_spi_srstat(struct pdma_intr_data *p_data)
{
    unsigned long flags;
    u32 sr;
    spin_lock_irqsave(&p_data->lock, flags);
    sr = p_data->ch_received;
    spin_unlock_irqrestore(&p_data->lock, flags);

//	    sf_log("[%s:%d] get from WaitEvent sr %x\n", __func__, __LINE__, sr);
    return sr;
}

static RET_CODE spi_panther_wait_event(u32 channel, u32 flags, compare_func compare)
{
    unsigned sr = 0;
    int interrupted;
    int done;
    int rc = 0;

    do {
		interrupted = wait_event_interruptible_timeout (
			spi_data.waitq, (done = compare(sr = pdma_get_spi_srstat(&spi_data), flags)), HZ);

		if ((rc = translate_pdma_status(channel, sr)) < 0)
        {
			FLASH_DEBUG("[%s:%d] pdma_error %x\n", __func__, __LINE__, sr);
			return rc;
		}
        else if (!interrupted)
        {
			FLASH_DEBUG("wait event time out !!\n");
			return -ETIMEDOUT;
		}
	} while(!done);

    return rc;
}

static RET_CODE spi_panther_read_with_pdma(spi_cfg_t *p_priv, u32 data_len, u32 cmd_len,
                                    u8 *p_buf, u32 cmd_dtmp, u8 secure)
{
    pdma_descriptor descriptor;
    int ret = SUCCESS;
    
    if (data_len <= 0)
    {
        return ERR_PARAM;
    }

    if (cmd_len <= 0)
    {
        REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;

        if (secure)
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x305);
        }
        else
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x205);
        }
    }

    // clear received data before operation
    spi_data.ch_received = 0;
    
    // need to flush cache before read to avoid potential problem
    dma_cache_wback_inv((u32)p_buf, data_len);

    descriptor.channel = PDMA_CH_SPI_RX;
    descriptor.next_addr = 0;
    descriptor.src_addr = PHYSICAL_ADDR(CH7_SLV_ADDR);
    descriptor.dest_addr = PHYSICAL_ADDR(p_buf);
    descriptor.dma_total_len = data_len;
    descriptor.aes_ctrl = init_secure_settings(ACTION_READ);
    descriptor.intr_enable = 1;
    descriptor.src = 0;
    descriptor.dest = 3;
    descriptor.fifo_size = 31;
    descriptor.valid = PDMA_VALID;
    pdma_desc_set(&descriptor);

    // wait until PDMA finish work
#ifdef PDMA_INTERRUPT
    ret = spi_panther_wait_event(descriptor.channel, CH7_PDMA_ENABLE, any_bits_set);
#else
    pdma_pooling_wait();
#endif

    return ret;
}

static RET_CODE spi_panther_write_with_pdma(spi_cfg_t *p_priv, u8 *p_cmd_buf, u32 data_len,
                                    u32 cmd_len, u8 *p_buf, u32 cmd_dtmp, u8 secure)
{
    pdma_descriptor descriptor;
    int ret = SUCCESS;

    if (data_len <= 0)
    {
        return ERR_PARAM;
    }

    if (cmd_len <= 0)
    {
        REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;

        if (secure)
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x303);
        }
        else
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x203);
        }
    }

    // clear received data before operation
    spi_data.ch_received = 0;
    
    // need to flush cache before read to avoid potential problem
    dma_cache_wback_inv((u32)p_buf, data_len);

    descriptor.channel = PDMA_CH_SPI_TX;
    descriptor.next_addr = 0;
    descriptor.src_addr = PHYSICAL_ADDR(p_buf);
    descriptor.dest_addr = PHYSICAL_ADDR(CH6_SLV_ADDR);
    descriptor.dma_total_len = data_len;
    descriptor.aes_ctrl = init_secure_settings(ACTION_WRITE);
    descriptor.intr_enable = 1;
    descriptor.src = 3;
    descriptor.dest = 0;
    descriptor.fifo_size = 31;
    descriptor.valid = PDMA_VALID;
    pdma_desc_set(&descriptor);

    // wait until PDMA finish work
#ifdef PDMA_INTERRUPT
    ret = spi_panther_wait_event(descriptor.channel, CH6_PDMA_ENABLE, any_bits_set);
#else
    pdma_pooling_wait();
#endif

    return ret;
}

u32 init_secure_settings(u32 action)
{
    u32 aes_control = 0;
    PDMAREG(PDMA_AES_CTRL) = (PDMA_AES_MODE_CBC | PDMA_AES_KEYLEN_256);

    if (!is_enable_secure_boot)
    {
        aes_enable_status = DISABLE_SECURE;
    }

    switch (aes_enable_status)
    {
        case DISABLE_SECURE:
            break;
        case ENABLE_SECURE_REG:
            if (action == ACTION_WRITE)
            {
                aes_control = (AES_ENABLE | AES_OP_ENCRYPT | AES_KEYSEL_REG);
            }
            else
            {
                aes_control = (AES_ENABLE | AES_OP_DECRYPT | AES_KEYSEL_REG);
            }
            break;
        case ENABLE_SECURE_OTP:
            // need to call read otp function to let hardware fetch otp key
            read_otp_key();

            if (action == ACTION_WRITE)
            {
                aes_control = (AES_ENABLE | AES_OP_ENCRYPT | AES_KEYSEL_OTP);
            }
            else
            {
                aes_control = (AES_ENABLE | AES_OP_DECRYPT | AES_KEYSEL_OTP);
            }
            break;
        default:
            break;
    }
//	    printk("aes_control = 0x%x\n", aes_control);
    return aes_control;
}

void spi_panther_init_secure(void)
{
    otp_read_config();
    
    // is enable secure boot
    if (otp_parse_config(OTP_ENABLE_SECURE_SHIFT))
    {
        FLASH_DEBUG("enable secure boot\n");
        read_otp_key();
        is_enable_secure_boot = 1;
    }
    else
    {
        FLASH_DEBUG("disable secure boot\n");
        is_enable_secure_boot = 0;
    }

    change_aes_enable_status(ENABLE_SECURE_OTP);
}

void read_otp_key(void)
{
    u8 otpkey_r[32];
    efuse_read(otpkey_r, 0, 32);
}
#endif

void change_aes_enable_status(u32 aes_status)
{
    aes_enable_status = aes_status;
}

static void spi_panther_read_without_pdma(spi_cfg_t *p_priv, u32 data_len, u32 cmd_len,
                                       u8 *p_buf, u32 cmd_dtmp, u8 secure)
{
    u32 i = 0;
    u32 j = 0;
    u32 r = 0;
    u32 dtmp = 0;
    
    if (data_len > 0)
    {
        i = data_len / 4;
        r = data_len % 4;

        if (cmd_len <= 0)
        {
            REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;

            if (secure)
            {
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x305);
            }
            else
            {
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x205);
            }
        }

        while(i)
        {
            if (spi_concerto_is_rxd_fifo_empty(p_priv->spi_id))
            {
                continue;
            }

            dtmp = REG32(R_SPIN_RXD(p_priv->spi_id));

            memcpy(p_buf, &dtmp, 4);
            i--;
            p_buf += 4;
        }

        if (r > 0)
        {
            while(spi_concerto_is_rxd_fifo_empty(p_priv->spi_id));
            dtmp = REG32(R_SPIN_RXD(p_priv->spi_id));

            for (j = 1; j <= r; j++)
            {
                p_buf[0] = (dtmp >> (8 * (4 - j))) & 0xff;
                p_buf++;
            }
        }
    }
}

static void spi_panther_write_without_pdma(spi_cfg_t *p_priv, u8 *p_cmd_buf, u32 data_len,
                                       u32 cmd_len, u8 *p_buf, u32 cmd_dtmp, u8 secure)
{
    u32 i = 0;
    u32 j = 0;
    u32 r = 0;
    u32 dtmp = 0;
    
    if (data_len > 0)
    {
        i = data_len / 4;
        r = data_len % 4;
        
        if (cmd_len <= 0)
        {
            REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;

            if (secure)
            {
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x303);
            }
            else
            {
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x203);
            }
        }

        while (i)
        {
            if (spi_concerto_is_txd_fifo_full(p_priv->spi_id))
            {
                continue;
            }
            
            memcpy(&dtmp, p_buf, 4);
            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
            i--;
            p_buf += 4;
        }

        while (spi_concerto_is_txd_fifo_full(p_priv->spi_id));
        
        // special case about change register parameters
        if (r == 1 && spi_converto_is_write_register(p_cmd_buf))
        {
            dtmp = 0;
            
            for (j = 1; j <= 4; j++)
            {
                dtmp |= (p_buf[0] << (8 * (4 - j)));
            }
            
            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
        }
        else if (r > 0)
        {
            dtmp = 0;
            
            for (j = 1; j <= r; j++)
            {
                dtmp |= (p_buf[0] << (8 * (4 - j)));
                p_buf++;
            }
            
            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
        }
    }
}

static RET_CODE spi_concerto_read(lld_spi_t *p_lld, u8 *p_cmd_buf, u32 cmd_len,
              spi_cmd_cfg_t *p_cmd_cfg, u8 *p_data_buf, u32 data_len, u8 secure, u32 dma_sel)
{
    spi_cfg_t *p_priv = NULL;
    u32 timeout = SPI_TIME_OUT;
    u32 i = 0;
    u8 *p_buf = 0;
    u8 cmd0_ionum = 0, cmd0_len = 0, cmd1_ionum = 0, cmd1_len = 0;
    u8 data_ionum = 0;
    u32 cmd_dtmp = 0;
    u8 dummy = 0;
    int ret = SUCCESS;

    if ((p_cmd_buf == NULL) && (cmd_len != 0))
    {
        return ERR_PARAM;
    }

    if ((p_data_buf == NULL) && (data_len != 0))
    {
        return ERR_PARAM;
    }

    p_priv = (spi_cfg_t *)p_lld->p_priv;

    spi_concerto_soft_reset(p_priv->spi_id);

    data_ionum = p_priv->io_num - 1;   //數據使用的IO口數目

    if (cmd_len > 0)
    {
        if (p_cmd_cfg == NULL)
        {
            cmd0_ionum = 0;
            cmd0_len = cmd_len;
            cmd1_ionum = 0;
            cmd1_len = 0;
            dummy = 0;
        }
        else
        {
            cmd0_ionum = p_cmd_cfg->cmd0_ionum - 1;
            cmd0_len = p_cmd_cfg->cmd0_len;
            cmd1_ionum = p_cmd_cfg->cmd1_ionum - 1;
            cmd1_len = p_cmd_cfg->cmd1_len;
            dummy = p_cmd_cfg->dummy;
        }
    }
    else if (cmd_len <= 0)
    {
        cmd0_ionum = 0;
        cmd0_len = 0;
        cmd1_ionum = 0;
        cmd1_len = 0;
        dummy = 0;
    }

    cmd_dtmp = (cmd1_ionum & 0x3) << 30 | (cmd1_len & 0x3f) << 24
                    | (cmd0_ionum & 0x3) << 22 | (cmd0_len & 0x3f) << 16;
    cmd_dtmp |= data_ionum << 14; //合入數據IO的配置
    cmd_dtmp |= dummy << 3;

    if (cmd_len > 0)
    {
        // send command
        p_buf = p_cmd_buf;
        REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;

        if(secure)
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x305);
        }
        else
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x205);
        }

        // write command
        for (i = 0; i < cmd_len; i ++)
        {
            if (spi_concerto_is_txd_fifo_full(p_priv->spi_id))
            {
                continue;
            }
            REG32(R_SPIN_CMD_FIFO(p_priv->spi_id)) = p_buf[i];
        }
    }

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    if (dma_sel)
    {
        ret = spi_panther_read_with_pdma(p_priv, data_len, cmd_len, p_data_buf, cmd_dtmp, secure);
    }
    else
#endif
    {
        spi_panther_read_without_pdma(p_priv, data_len, cmd_len, p_data_buf, cmd_dtmp, secure);
    }

    if (ret)
    {
        return ret;
    }
    
    timeout = SPI_TIME_OUT;
    while (!spi_concerto_is_trans_complete(p_priv->spi_id) && timeout)
    {
        timeout--;
    }
    if (timeout == 0)
    {
        FLASH_DEBUG("spi timeout  %d  %s\n",__LINE__, __FILE__);
        return ERR_TIMEOUT;
    }
    return ret;
}

static RET_CODE spi_concerto_write(lld_spi_t *p_lld, u8 *p_cmd_buf, u32 cmd_len,
                  spi_cmd_cfg_t *p_cmd_cfg, u8 *p_data_buf, u32 data_len, u8 secure, u32 dma_sel)
{
    spi_cfg_t *p_priv = NULL;
    u32 timeout = SPI_TIME_OUT;
    u32 i  = 0;
    u8 *p_buf = NULL;
    u8 cmd0_ionum = 0, cmd0_len = 0, cmd1_ionum = 0, cmd1_len = 0;
    u8 data_ionum = 0;
    u32 cmd_dtmp = 0;
    int ret = SUCCESS;

    if ((p_cmd_buf == NULL) && (cmd_len != 0))
    {
        FLASH_DEBUG("%s %d  %s\n",__FUNCTION__,__LINE__, __FILE__);
        return ERR_PARAM;
    }

    if ((p_data_buf == NULL) && (data_len != 0))
    {
        FLASH_DEBUG("%s %d  %s\n",__FUNCTION__,__LINE__, __FILE__);
        return ERR_PARAM;
    }

    p_priv = (spi_cfg_t *)p_lld->p_priv;

    data_ionum = p_priv->io_num - 1;   //數據使用的IO口數目

    if (cmd_len > 0)
    {
        if (p_cmd_cfg == NULL)   //默認使用單線模式
        {
            cmd0_ionum = 0;
            cmd0_len = cmd_len;
            cmd1_ionum = 0;
            cmd1_len = 0;
        }
        else  //支持自定義的命令IO口數目
        {
            cmd0_ionum = p_cmd_cfg->cmd0_ionum -1;
            cmd0_len = p_cmd_cfg->cmd0_len;
            cmd1_ionum = p_cmd_cfg->cmd1_ionum -1;
            cmd1_len = p_cmd_cfg->cmd1_len;
        }
    }
    else if (cmd_len <= 0)
    {
        cmd0_ionum = 0;
        cmd0_len = 0;
        cmd1_ionum = 0;
        cmd1_len = 0;
    }

    cmd_dtmp = (cmd0_ionum & 0x3) << 30 | (cmd0_len & 0x3f) << 24
                    | (cmd1_ionum & 0x3) << 22 | (cmd1_len & 0x3f) << 16;
    cmd_dtmp |= data_ionum << 14; //合入數據IO的配置

    if (cmd_len > 0)
    {
        // send command
        p_buf = p_cmd_buf;
        REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;

        if (secure)
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x303);
        }
        else
        {
            REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x203);
        }

        // write command
        for (i = 0; i < cmd_len; i ++)
        {
            if (spi_concerto_is_txd_fifo_full(p_priv->spi_id))
            {
                continue;
            }
            REG32(R_SPIN_CMD_FIFO(p_priv->spi_id)) = p_buf[i];
        }
    }

#ifdef CONFIG_MTD_PDMA_SPI_FLASH
    if (dma_sel)
    {
        ret = spi_panther_write_with_pdma(p_priv, p_cmd_buf, data_len, cmd_len, p_data_buf, cmd_dtmp, secure);
    }
    else
#endif
    {
        spi_panther_write_without_pdma(p_priv, p_cmd_buf, data_len, cmd_len, p_data_buf, cmd_dtmp, secure);
    }

    if (ret)
    {
        return ret;
    }

    timeout = SPI_TIME_OUT;
    while (!spi_concerto_is_trans_complete(p_priv->spi_id) && timeout)
    {
        timeout--;
    }
    if (timeout == 0)
    {
        FLASH_DEBUG("spi timeout  %d  %s\n",__LINE__, __FILE__);
        return ERR_TIMEOUT;
    }
    return ret;
}

static void spi_concerto_ioctrl_set_baud_rate(u8 spi_id, u32 clk)
{
    u32 dtmp = 0;
    s16 baud_rate = 1;
    u16 phy_clk = 0;
    u16 int_delay = 0;


	dtmp = REG32(R_CLK_SPI_CFG);

	switch(spi_id)
	{
		case 0:
			if ((dtmp & 0x3) == 0)
			  phy_clk = 24;
			else if ((dtmp & 0x3) == 1)
			  phy_clk = 365;
			else if ((dtmp & 0x3) == 2)
			  phy_clk = 275;
			else if ((dtmp & 0x3) == 3)
			  phy_clk = 259;
		   break;
		case 1:
		   if (((dtmp & 0x300) >> 8) == 0)
				phy_clk = 24;
		   else if (((dtmp & 0x300) >> 8) == 1)
				 phy_clk = 365;
		   else if (((dtmp & 0x300) >> 8) == 2)
				phy_clk = 275;
		   else if (((dtmp & 0x300) >> 8) == 3)
				phy_clk = 259;
		   break;
		case 2:
		   if (((dtmp & 0x30000) >> 16) == 0)
				phy_clk = 24;
		   else if (((dtmp & 0x30000) >> 16) == 1)
				phy_clk = 365;
		   else if (((dtmp & 0x30000) >> 16) == 2)
				phy_clk = 275;
		   else if (((dtmp & 0x30000) >> 16) == 3)
				phy_clk = 259;
		   break;
		default:
		   FLASH_DEBUG("error spi_id\n");
	}

    dtmp = REG32(R_SPIN_CH0_BAUD(spi_id));

    printk("phy_clk = %d, clk=%d\n",phy_clk, clk);  
    baud_rate = phy_clk / clk;  // 硬件的計算方法改變  (phy_clk / clk) / 2 - 1;
    if((phy_clk%clk) > 0)
		baud_rate ++;
    baud_rate = baud_rate > 0 ? baud_rate : 1;

    int_delay = baud_rate/2;

    dtmp = baud_rate | (int_delay << 28);

    printk("R_SPIN_CH0_BAUD: %x\n",dtmp);
    REG32(R_SPIN_CH0_BAUD(spi_id)) = dtmp;
}

static void spi_concerto_set_baud_rate(lld_spi_t * p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    spi_concerto_ioctrl_set_baud_rate(p_priv->spi_id, p_priv->bus_clk_mhz);
}

static void spi_concerto_init_mode(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;
    u32 dtmp = 0;

    dtmp |= 0x1 << 30;   //恢復至初始狀態
    dtmp |= 0x1 << 29;   //表示為bit模式打包，1表示為byte模式打包
    dtmp |= 0x7 << 24;   //8位寬的傳輸單元
	dtmp |= p_priv->op_mode << 22;
	dtmp |= p_priv->op_mode << 20;
    // 該功能已不在此寄存器中  dtmp |= (p_priv->io_num - 1) << 20;
    dtmp |= 1 << 18;
    dtmp |= 1 << 16;
    dtmp |= p_priv->op_mode << 13;
    dtmp |= 1 << 10;
    dtmp |= 1 << 9;
    dtmp |= 1 << 8;
    dtmp |= 1 << 1;
    dtmp |= 1 << 0;
    //dtmp = 0x67050703; //  dtmp = 0x67050503;
    REG32(R_SPIN_CH0_MODE_CFG(p_priv->spi_id)) = dtmp;

}

static void spi_concerto_init_int(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    REG32(R_SPIN_INT_CFG(p_priv->spi_id)) = 0x100807;
}

#if 0
static void spi_concerto_set_pin_cfg(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    spi_concerto_ioctrl_set_pins_cfg(p_priv->spi_id);
}
#endif

static void spi_concerto_set_pin_dir(lld_spi_t *p_lld)
{
  spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

  spi_concerto_ioctrl_set_pins_dir(p_priv->spi_id, p_priv->io_num);
}

static void spi_concerto_init_mosi_ctrl(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    if(p_priv->io_num != 4)
    {
        REG32(R_SPIN_PIN_CTRL(p_priv->spi_id)) = 0xa0;
    }
    else
    {
        REG32(R_SPIN_PIN_CTRL(p_priv->spi_id)) = 0x0;
    }
}


#if 0
//此處的修改需要跟IC確認是否如此
static void spi_concerto_ioctrl_set_clk_delay(u8 spi_id, u8 delay)
{
    u8 int_delay = 0;
    u32 dtmp = 0;
    int_delay = delay / 2;
	dtmp = hal_get_u32((volatile unsigned long *) R_SPIN_BAUD(spi_id));

    dtmp &= ~0xf0000000;
    dtmp |= int_delay<<28;

	hal_put_u32((volatile unsigned long *) R_SPIN_BAUD(spi_id), dtmp);
}

static void spi_concerto_set_clk_delay(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = NULL;

    p_priv = (spi_cfg_t *)p_lld->p_priv;
    SPI_DBG_PRINTF("spi_concerto_set_clk_delay[%d]=:%x\n",p_priv->spi_id,p_priv->bus_clk_delay);
    spi_concerto_ioctrl_set_clk_delay(p_priv->spi_id, p_priv->bus_clk_delay);
}

static void spi_concerto_disable_mem_boot(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    if(p_priv->spi_id == 0)
    {
        REG32(R_SPIN_MEM_BOOT(p_priv->spi_id)) = 0x00000000;
    }
}
#endif

#if 1
void spi_concerto_init(lld_spi_t * p_lld)
#else
static void spi_concerto_init(lld_spi_t * p_lld)
#endif
{
    //spi_concerto_disable_mem_boot(p_lld);
    spi_concerto_set_baud_rate(p_lld);
    spi_concerto_init_mode(p_lld);
    spi_concerto_init_int(p_lld);
    //spi_concerto_set_pin_cfg(p_lld);
    spi_concerto_set_pin_dir(p_lld);
    spi_concerto_init_mosi_ctrl(p_lld);
}

inline static void spi_concerto_ioctrl_set_pins_dir(u8 spi_id,  u8 io_num)
{
    u32 dtmp = 0;

    dtmp = 0x5f;

    if (io_num == 1)
    {
        dtmp = 0x5f;
    }
    else if (io_num == 2)
    {
        dtmp = 0x5f;
    }
    else if (io_num == 4)
    {
        dtmp = 0xff;
    }
    REG32(R_SPIN_PIN_MODE(spi_id)) = dtmp;
}

#if 1
#if 0
void spi_flash_set_pinmux(void)
{
    //Hukun add for pinmux
    REG32(0xbf156008) = 0x00000000;
    return;
}
#endif
#else
void spi_flash_set_pinmux(void)
{
    REG32(0xbf156004) &= ~(0xFFF << 4);
    REG32(0xbf156004) |= (0x555 << 4);
    REG32(0xbf156008) &= 0xFFFFFF;
    REG32(0xbf156008) |= 0x111111;
    REG32(0xbf15601c) &= ~(0xFFFF << 8);
    REG32(0xbf15601c) |= (0x0 << 8);

    return;
}
#endif

static void spi_getDefaultCfg(spi_cfg_t *p_spiCfg)
{
    p_spiCfg->bus_clk_mhz   = 10;
    p_spiCfg->bus_clk_delay = 8;
    p_spiCfg->io_num        = 1;
    p_spiCfg->op_mode       = 0;
    p_spiCfg->spi_id        = 0;
}

//設備掛載，一般而言一條總線只掛載一個設備,故可將其當成SPI的配置API
struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
    lld_spi_t * p_lld;
    spi_cfg_t *p_spiCfg;
    if (bus > 2)
    {
        printk("Error Parameter!!!\n");
        return NULL;
    }
    p_lld = &(gSpiLLD[bus]);

    if (p_lld->used == 1)
    {
        printk("spi%d is already setup!!!\n", bus);
        return &(gSpiSlave[bus]);
    }
    
    p_spiCfg = &(gSpiCfg[bus]);
    spi_getDefaultCfg(p_spiCfg);
    p_lld->p_priv = p_spiCfg;

    p_spiCfg->bus_clk_mhz = max_hz/1000000;  //MHZ峈等弇
    p_spiCfg->op_mode = mode;
    p_spiCfg->spi_id = bus;
    
    //spi_concerto_init(p_lld);

    p_lld->used = 1;

    gSpiSlave[bus].bus = bus;
    gSpiSlave[bus].cs  = cs;
	return &(gSpiSlave[bus]);
}

//	void spi_init(void)
//	{
//	    /* Nothing to do */
//	    memset(gSpiLLD, 0, sizeof(gSpiLLD));
//	    memset(gSpiSlave, 0xff, sizeof(gSpiSlave));
//	    //spi_setup_slave(0, 0, 10000000, 0);
//	}


//設備卸載，目前一條總線只掛載一個設備，故卸載可留空
void spi_free_slave(struct spi_slave *slave)
{
    lld_spi_t * p_lld;
    
    p_lld = &(gSpiLLD[slave->bus]);

    p_lld->used = 0;
    
}


//占用總線請求，使用中保證每條總線一個設備使用，故留空
//若一條總線多個設備同時占用，則需要進行互斥
int spi_claim_bus(struct spi_slave *slave)
{
    /* Nothing to do */
    return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	/* Nothing to do */
}


static int concerto_spi_read(struct spi_slave *slave, u8 *p_cmd_buf, u32 cmd_len,
              spi_cmd_cfg_t *p_cmd_cfg, u8 *p_data_buf, u32 data_len, u8 secure, u32 dma_sel)
{
    lld_spi_t *p_lld;
    p_lld = &(gSpiLLD[slave->bus]);
    
    //SPI_DBG_PRINTF("concerto_spi_read[%d] p_data_buf[0x%x] :0x%x\n",slave->bus,p_data_buf,data_len);
    return spi_concerto_read(p_lld, p_cmd_buf, cmd_len, p_cmd_cfg, p_data_buf, data_len, secure, dma_sel);
}

static int concerto_spi_write(struct spi_slave *slave, u8 *p_cmd_buf, u32 cmd_len,
              spi_cmd_cfg_t *p_cmd_cfg, u8 *p_data_buf, u32 data_len, u8 secure, u32 dma_sel)
{
    lld_spi_t *p_lld;
    p_lld = &(gSpiLLD[slave->bus]);
    return spi_concerto_write(p_lld, p_cmd_buf, cmd_len, p_cmd_cfg, p_data_buf, data_len, secure, dma_sel);
}

void concerto_spi_set_trans_ionum(struct spi_slave *slave, u8 io_num)
{
    lld_spi_t *p_lld = &(gSpiLLD[slave->bus]);
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    p_priv->io_num = io_num;
    spi_concerto_ioctrl_set_ionum(p_priv->spi_id, io_num);
    spi_concerto_ioctrl_set_pins_dir(p_priv->spi_id, io_num);
    return ;
}


#if 1
int concerto_spi_cmd_read(struct spi_slave *spi, u8 *cmd, u32 cmd_len,
                        void *data, u32 data_len, struct spi_read_cfg* read_cfg, u32 dma_sel)
{
    spi_cmd_cfg_t cmd_cfg;
    memset(&cmd_cfg, 0, sizeof(cmd_cfg));
    
    if (read_cfg != NULL)
    {
        cmd_cfg.cmd0_ionum = 1;
        cmd_cfg.cmd0_len = cmd_len;
        cmd_cfg.cmd1_ionum = 1;
        cmd_cfg.cmd1_len = 0;
        cmd_cfg.dummy = read_cfg->read_from_cache_dummy;
        return concerto_spi_read(spi, cmd, cmd_len, &cmd_cfg, (u8*)data, data_len, 0, dma_sel);
    }
    else
    {
        return concerto_spi_read(spi, cmd, cmd_len, NULL, (u8*)data, data_len, 0, dma_sel);
    }
}

int concerto_spi_cmd_write(struct spi_slave *spi, u8 *cmd, u32 cmd_len,
		                const void *data, u32 data_len, u32 dma_sel)
{
    return concerto_spi_write(spi, cmd, cmd_len, NULL, (u8*)data, data_len, 0, dma_sel);
}

int concerto_spi_cmd(struct spi_slave *spi, u8 cmd, void *response, u32 len)
{
	return concerto_spi_read_common(spi, &cmd, 1, response, len);
}


int concerto_spi_cmd_write_enable(struct spi_slave *spi)
{
	return concerto_spi_cmd(spi, SPI_FLASH_CMD_WR_EN, NULL, 0);
}


int concerto_spi_cmd_write_disable(struct spi_slave *spi)
{
	return concerto_spi_cmd(spi, SPI_FLASH_CMD_WR_DI, NULL, 0);
}


int concerto_spi_cmd_wait_ready(struct spi_slave *spi, u32 timeout)
{
    u8 resp = 0;
    int ret = 0;
    u32 status = 0;

    status = 1;

    usleep_range(100, 100);
    while (status && timeout)
    {
        ret = concerto_spi_cmd(spi, SPI_FLASH_CMD_RD_SR, &resp, 1);
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

        usleep_range(100, 100);
    }
    if (timeout == 0)
    {
        printk("concerto_spi_cmd_wait_ready: time out! %s %d %s\n",
                __FUNCTION__, __LINE__, __FILE__);
    }
    return 0;
}


int concerto_spi_read_common(struct spi_slave *spi, u8 *cmd,
		u32 cmd_len, void *data, u32 data_len)
{
	return concerto_spi_cmd_read(spi, cmd, cmd_len, data, data_len, NULL, 0);
}

// the size need to be equal to page_size
void panther_spi_restore_erased_page(u8 *buf, u32 size)
{
    // for better performance, check four bytes first
    if((((u32)buf & 0x03)==0) && ( *((u32 *)buf) != *((u32 *)__erased_page_data_first)))
        return;

    if (is_erased_page(buf, size))
    {
        memset(buf, 0xff, size);
    }
}

#endif
