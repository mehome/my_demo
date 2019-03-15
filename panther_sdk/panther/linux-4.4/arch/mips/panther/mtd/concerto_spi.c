/******************************************************************************/
/* Copyright (c) 2012 Montage Tech - All Rights Reserved                      */
/******************************************************************************/
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>

#include "concerto_spi.h"
#include "erased_page.h"

lld_spi_t gSpiLLD[3]; 
spi_cfg_t gSpiCfg[3];
struct spi_slave gSpiSlave[3];

static unsigned int concerto_is_ddr = 0, concerto_is_dma = 0;

static void spi_concerto_ioctrl_set_ionum(u8 spi_id, u8 ionum)
{
    u32 dtmp = REG32(R_SPIN_CTRL(spi_id));
    dtmp &= ~(0x3 << 14);
    dtmp |= ((ionum - 1) & 0x3) << 14;   //該配置從模式配置移植控制寄存器
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

static void spi_concerto_ioctrl_set_pins_dir(u8 spi_id,  u8 io_num)
{
    u32 dtmp = 0;
    
    dtmp =0x5f;
    
    if(io_num == 1)
    {
      dtmp = 0x5f;
    }
    else if(io_num == 2)
    {
      dtmp = 0x5f;
    }
    else if(io_num == 4)
    {
      dtmp = 0xff;
    }
    REG32(R_SPIN_PIN_MODE(spi_id)) = dtmp;
}

static void spi_concerto_soft_reset(u8 spi_id)
{
    REG32(R_SPIN_MODE_CFG(spi_id)) &= ~0x1;
    REG32(R_SPIN_MODE_CFG(spi_id)) |= 0x1;
}

inline static BOOL spi_concerto_is_txd_fifo_full(u8 spi_id)
{
    u32 dtmp = 0;
    dtmp = REG32(R_SPIN_STA(spi_id));
    if((31 - ((dtmp >> 8) & 0x3f)) == 0)
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
    if((dtmp & 0x3f) > 0)
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
    if(!((dtmp_sta >> 16) & 0x1))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#if 0
static void spi_concerto_ioctrl_set_baud_rate(u8 spi_id, u32 clk)
{
    u32 dtmp = 0;
    s16 baud_rate = 1;
    u16 phy_clk = 0;
    u16 int_delay = 0;

    //phy_clk = 259;    //phy_clk set in uboot spi_setup_slave
    phy_clk = 365;    //phy_clk set in uboot spi_setup_slave

    dtmp = REG32(R_SPIN_BAUD(spi_id));

    FLASH_DEBUG("phy_clk = %d, clk=%d\n",phy_clk, clk);
    baud_rate = phy_clk / clk;  
    if((phy_clk%clk) > 0)
		baud_rate ++;
    baud_rate = baud_rate > 0 ? baud_rate : 1;

    //int_delay = baud_rate / 2;
    int_delay = baud_rate / 2 + 2;
    
    dtmp = baud_rate | (1 << 16) | (int_delay << 28);

    REG32(R_SPIN_BAUD(spi_id)) = dtmp;
}

static void spi_concerto_set_baud_rate(lld_spi_t * p_lld)
{
    spi_cfg_t *p_priv = NULL;

    p_priv = (spi_cfg_t *)p_lld->p_priv;
    spi_concerto_ioctrl_set_baud_rate(p_priv->spi_id, p_priv->bus_clk_mhz);
}

static void spi_concerto_init_mode(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = NULL;
    u32 dtmp = 0;
    p_priv = (spi_cfg_t *)p_lld->p_priv;

    dtmp |= 0x1 << 30;    
    dtmp |= 0x1 << 29;    
    dtmp |= 0x7 << 24;  
    dtmp |= p_priv->op_mode << 22;
    dtmp |= p_priv->op_mode << 20;
    dtmp |= 1 << 18;
    dtmp |= 1 << 16;
    dtmp |= p_priv->op_mode << 13;
    dtmp |= 1 << 10;
    dtmp |= 1 << 9;
    dtmp |= 1 << 8;
    dtmp |= 1 << 1;
    dtmp |= 1 << 0;
    REG32(R_SPIN_MODE_CFG(p_priv->spi_id)) = dtmp;
}

static void spi_concerto_init_int(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    REG32(R_SPIN_INT_CFG(p_priv->spi_id)) = 0x100807;
}

static void spi_concerto_set_pin_cfg(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    spi_concerto_ioctrl_set_pins_cfg(p_priv->spi_id,  p_priv->io_num);
}

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
        REG32(R_SPIN_MOSI_CTRL(p_priv->spi_id)) = 0xa0;
    }
    else
    {
        REG32(R_SPIN_MOSI_CTRL(p_priv->spi_id)) = 0x00;
    }
}
#endif


#if 0

static void spi_concerto_disable_mem_boot(lld_spi_t *p_lld)
{
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    if(p_priv->spi_id == 0)
    {
        REG32(R_SPIN_MEM_BOOT(p_priv->spi_id)) = 0x00000000;
    }
}

static void spi_concerto_init(lld_spi_t * p_lld)
{
    spi_concerto_disable_mem_boot(p_lld);
    spi_concerto_set_baud_rate(p_lld);
    spi_concerto_init_mode(p_lld);
    spi_concerto_init_int(p_lld);
    spi_concerto_set_pin_cfg(p_lld);
    spi_concerto_set_pin_dir(p_lld);
    spi_concerto_init_mosi_ctrl(p_lld);
    //spi_concerto_set_clk_delay(p_lld);

}
#endif


static RET_CODE spi_concerto_read(lld_spi_t *p_lld, u8 *p_cmd_buf,u32 cmd_len,
                spi_cmd_cfg_t *p_cmd_cfg,u8 *p_data_buf, u32 data_len)
{
    spi_cfg_t *p_priv = NULL;
    u32 timeout = SPI_TIME_OUT;
    u32 i = 0;
    u8 r = 0;
    u32 j = 0;
    u32 dtmp = 0;
    u8 *p_buf = 0;
    u8 cmd0_ionum = 0, cmd0_len = 0, cmd1_ionum = 0, cmd1_len = 0;
    u8 data_ionum = 0;
    u32 cmd_dtmp = 0;
    u8  dummy = 0;

    if((p_cmd_buf == NULL) && (cmd_len != 0))
    {
        return ERR_PARAM;
    }

    if((p_data_buf == NULL) && (data_len != 0))
    {
        return ERR_PARAM;
    }

    p_priv = (spi_cfg_t *)p_lld->p_priv;

    spi_concerto_soft_reset(p_priv->spi_id);

    data_ionum = p_priv->io_num -1;  

    if(cmd_len >= 0)
    {
        if(p_cmd_cfg == NULL)
        {
            cmd0_ionum = 0;
            cmd0_len = cmd_len;
            cmd1_ionum = 0;
            cmd1_len = 0;
            dummy =0;
        }
        else
        {
            cmd0_ionum = p_cmd_cfg->cmd0_ionum -1;
            cmd0_len = p_cmd_cfg->cmd0_len;
            cmd1_ionum = p_cmd_cfg->cmd1_ionum -1;
            cmd1_len = p_cmd_cfg->cmd1_len;
            dummy = p_cmd_cfg->dummy;
        }
    }
    else if(cmd_len < 0)
    {
        cmd0_ionum = 0;
        cmd0_len = 0;
        cmd1_ionum = 0;
        cmd1_len = 0;
        dummy = 0;
    }

    if(concerto_is_ddr == 0)
    {
        cmd_dtmp = (cmd1_ionum & 0x3) << 30 | (cmd1_len & 0x3f) << 24 
                        | (cmd0_ionum & 0x3) << 22 | (cmd0_len & 0x3f) << 16;
    }
    else
    {
        cmd_dtmp = (cmd1_ionum & 0x3) << 30 | (0 & 0x1) << 29 | (cmd1_len & 0x3f) << 24 
                        | (cmd0_ionum & 0x3) << 22 | (1 & 0x1) << 21 
                        | (cmd0_len & 0x3f) << 16 | (1 & 0x1) << 13;
    }
    cmd_dtmp |= data_ionum << 14; 
    cmd_dtmp |= dummy << 3;

    if(cmd_len > 0)
    {
        i = cmd_len / 4;
        r = cmd_len % 4;

        p_buf = p_cmd_buf;
        REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;

        while(i)
        {
            if(spi_concerto_is_txd_fifo_full(p_priv->spi_id))
            {
                continue;
            }

            dtmp = (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
            i --;
            p_buf += 4;
        }

        while(spi_concerto_is_txd_fifo_full(p_priv->spi_id));
        if(r > 0)
        {
            dtmp = 0;
            for(j = 1; j <= r; j++)
            {
                dtmp |= (p_buf[0] << (8 * (4 - j)));
                p_buf ++;
            }
            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
        }

    REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x205);
    }

    if(concerto_is_dma == 0)
    {
        if(data_len > 0)
        {
            i = data_len / 4;
            r = data_len % 4;

            p_buf = p_data_buf;
            if(cmd_len <= 0)
            {
                REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x205);
            }


            while(i)
            {
                if(spi_concerto_is_rxd_fifo_empty(p_priv->spi_id))
                {
                    continue;
                }

                dtmp = REG32(R_SPIN_RXD(p_priv->spi_id));

                p_buf[0] = (dtmp >> 24) & 0xff ;
                p_buf[1] = (dtmp >> 16) & 0xff;
                p_buf[2] = (dtmp >> 8) & 0xff;
                p_buf[3] = dtmp & 0xff;
                i--;
                p_buf += 4;
            }

            if(r > 0)
            {
                while(spi_concerto_is_rxd_fifo_empty(p_priv->spi_id));
                dtmp = REG32(R_SPIN_RXD(p_priv->spi_id));
                for(j = 1; j <= r; j++)
                {
                    p_buf[0] = (dtmp >> (8 * (4 - j))) & 0xff;
                    p_buf ++;
                }
            }
        }
        timeout = SPI_TIME_OUT;
        while(!spi_concerto_is_trans_complete(p_priv->spi_id) && timeout)
        {
            timeout --;
        }
        if(timeout == 0)
        {
            printk("spi timeout  %d  %s\n",__LINE__, __FILE__);
        }
    }
    return SUCCESS;
}

#if 0
static RET_CODE spi_concerto_write(lld_spi_t *p_lld, u8 *p_cmd_buf, u32 cmd_len,
                    spi_cmd_cfg_t *p_cmd_cfg, u8 *p_data_buf, u32 data_len)
{
    spi_cfg_t *p_priv = NULL;
    u32 timeout = SPI_TIME_OUT;
    u32 i  = 0;
    u8 r = 0;
    u32 j = 0;
    u32 dtmp = 0;
    u8 *p_buf = NULL;
    u8 cmd0_ionum = 0, cmd0_len = 0, cmd1_ionum = 0, cmd1_len = 0;
    u8 data_ionum = 0;
    u32 cmd_dtmp = 0;

    if((p_cmd_buf == NULL) && (cmd_len != 0))
    {
        FLASH_DEBUG("%s %d  %s\n",__FUNCTION__,__LINE__, __FILE__);
        return ERR_PARAM;
    }

    if((p_data_buf == NULL) && (data_len != 0))
    {
        FLASH_DEBUG("%s %d  %s\n",__FUNCTION__,__LINE__, __FILE__);
        return ERR_PARAM;
    }

    p_priv = (spi_cfg_t *)p_lld->p_priv;

    data_ionum = p_priv->io_num -1;   

    if(cmd_len > 0)
    {
        if(p_cmd_cfg == NULL)
        {
            cmd0_ionum = 0;
            cmd0_len = cmd_len;
            cmd1_ionum = 0;
            cmd1_len = 0;
        }
        else
        {
            cmd0_ionum = p_cmd_cfg->cmd0_ionum -1;
            cmd0_len = p_cmd_cfg->cmd0_len;
            cmd1_ionum = p_cmd_cfg->cmd1_ionum -1;
            cmd1_len = p_cmd_cfg->cmd1_len;
        }
    }
    else if(cmd_len <= 0)
    {
        cmd0_ionum = 0;
        cmd0_len = 0;
        cmd1_ionum = 0;
        cmd1_len = 0;
    }

    if(concerto_is_ddr == 0)
    {
        cmd_dtmp = (cmd1_ionum & 0x3) << 30 | 
                    (cmd1_len & 0x3f) << 24 | 
                    (cmd0_ionum & 0x3) << 22 | 
                    (cmd0_len & 0x3f) << 16;
    }
    else
    {
        cmd_dtmp = (cmd1_ionum & 0x3) << 30 | 
                    (0 & 0x1) << 29 | 
                    (cmd1_len & 0x3f) << 24 | 
                    (cmd0_ionum & 0x3) << 22 | 
                    (1 & 0x1) << 21 | 
                    (cmd0_len & 0x3f) << 16 | 
                    (1 & 0x1) << 13;
    }

    cmd_dtmp |= data_ionum << 14;  
    
    if(cmd_len > 0)
    {
        i = cmd_len / 4;
        r = cmd_len % 4;

        p_buf = p_cmd_buf;
        REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;
        REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x203);

        while(i)
        {
            if(spi_concerto_is_txd_fifo_full(p_priv->spi_id))
            {
                continue;
            }
            dtmp = (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
            i --;
            p_buf += 4;
        }

        while(spi_concerto_is_txd_fifo_full(p_priv->spi_id));
        if(r > 0)
        {
            dtmp = 0;
            for(j = 1; j <= r; j++)
            {
                dtmp |= (p_buf[0] << (8 * (4 - j)));
                p_buf ++;
            }

            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
        }
    }
    if(concerto_is_dma == 0)
    {
        if(data_len > 0)
        {
            i = data_len / 4;
            r = data_len % 4;

            p_buf = p_data_buf;
            if(cmd_len <= 0)
            {
                REG32(R_SPIN_TC(p_priv->spi_id)) = data_len;
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x203);
            }

            while(i)
            {
                if(spi_concerto_is_txd_fifo_full(p_priv->spi_id))
                {
                    continue;
                }

                dtmp = (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
                REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
                i --;
                p_buf += 4;
            }

            while(spi_concerto_is_txd_fifo_full(p_priv->spi_id));
            if(r > 0)
            {
                dtmp = 0;
                for(j = 1; j <= r; j++)
                {
                    dtmp |= (p_buf[0] << (8 * (4 - j)));
                    p_buf ++;
                }
                REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
            }
        }
            timeout = SPI_TIME_OUT;
        while(!spi_concerto_is_trans_complete(p_priv->spi_id) && timeout)
        {
            timeout --;
        }
        if(timeout == 0)
        {
            printk("spi timeout  %d  %s\n",__LINE__, __FILE__);
        }
    }
    return SUCCESS;
}
#else
static RET_CODE spi_concerto_write(lld_spi_t *p_lld, u8 *p_cmd_buf, u32 cmd_len,
                    spi_cmd_cfg_t *p_cmd_cfg, u8 *p_data_buf, u32 data_len)
{
    spi_cfg_t *p_priv = NULL;
    u32 timeout = SPI_TIME_OUT;
    u32 i  = 0;
    u8 r = 0;
    u32 j = 0;
    u32 dtmp = 0;
    u8 *p_buf = NULL;
    u8 cmd0_ionum = 0, cmd0_len = 0, cmd1_ionum = 0, cmd1_len = 0;
    u8 data_ionum = 0;
    u32 cmd_dtmp = 0;

    if((p_cmd_buf == NULL) && (cmd_len != 0))
    {
        FLASH_DEBUG("%s %d  %s\n",__FUNCTION__,__LINE__, __FILE__);
        return ERR_PARAM;
    }

    if((p_data_buf == NULL) && (data_len != 0))
    {
        FLASH_DEBUG("%s %d  %s\n",__FUNCTION__,__LINE__, __FILE__);
        return ERR_PARAM;
    }

    p_priv = (spi_cfg_t *)p_lld->p_priv;

    REG32(R_SPIN_MODE_CFG(p_priv->spi_id)) &= ~(1 << 30);


    data_ionum = p_priv->io_num -1;   

    if(cmd_len > 0)
    {
        if(p_cmd_cfg == NULL)
        {
            cmd0_ionum = 0;
            cmd0_len = cmd_len;
            cmd1_ionum = 0;
            cmd1_len = 0;
        }
        else
        {
            cmd0_ionum = p_cmd_cfg->cmd0_ionum -1;
            cmd0_len = p_cmd_cfg->cmd0_len;
            cmd1_ionum = p_cmd_cfg->cmd1_ionum -1;
            cmd1_len = p_cmd_cfg->cmd1_len;
        }
    }
    else if(cmd_len <= 0)
    {
        cmd0_ionum = 0;
        cmd0_len = 0;
        cmd1_ionum = 0;
        cmd1_len = 0;
    }

    if(concerto_is_ddr == 0)
    {
        cmd_dtmp = (cmd1_ionum & 0x3) << 30 | 
                    (cmd1_len & 0x3f) << 24 | 
                    (cmd0_ionum & 0x3) << 22 | 
                    (cmd0_len & 0x3f) << 16;
    }
    else
    {
        cmd_dtmp = (cmd1_ionum & 0x3) << 30 | 
                    (0 & 0x1) << 29 | 
                    (cmd1_len & 0x3f) << 24 | 
                    (cmd0_ionum & 0x3) << 22 | 
                    (1 & 0x1) << 21 | 
                    (cmd0_len & 0x3f) << 16 | 
                    (1 & 0x1) << 13;
    }

    cmd_dtmp |= data_ionum << 14;  
    
    if(cmd_len > 0)
    {
        i = cmd_len / 4;
        r = cmd_len % 4;

        p_buf = p_cmd_buf;
        REG32(R_SPIN_TC(p_priv->spi_id)) = 0;

        while(i)
        {
            if(spi_concerto_is_txd_fifo_full(p_priv->spi_id))
            {
                continue;
            }
            dtmp = (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
            i --;
            p_buf += 4;
        }

        while(spi_concerto_is_txd_fifo_full(p_priv->spi_id));
        if(r > 0)
        {
            dtmp = 0;
            for(j = 1; j <= r; j++)
            {
                dtmp |= (p_buf[0] << (8 * (4 - j)));
                p_buf ++;
            }

            REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
        }
        REG32(R_SPIN_CTRL(p_priv->spi_id)) = (cmd_dtmp | 0x203);
        
        timeout = SPI_TIME_OUT;
        while(!spi_concerto_is_trans_complete(p_priv->spi_id) && timeout)
        {
            timeout --;
        }
        if(timeout == 0)
        {
            printk("spi timeout  %d  %s\n",__LINE__, __FILE__);
        }
    }
    if(concerto_is_dma == 0)
    {
        if(data_len > 0)
        {
            u32 snd_times = 0;
            u32 left = 0;
            u32 snd_bytes = 128;
            
            snd_times = data_len / snd_bytes;
            left = data_len % snd_bytes;
            
            cmd_dtmp = REG32(R_SPIN_CTRL(p_priv->spi_id));
            cmd_dtmp &= ~((0x3f << 24) | (0x3f << 16));
            
            p_buf = p_data_buf;
            for(i = 0; i < snd_times; i ++)
            {
                snd_bytes = 128;
                while(snd_bytes)
                {
                    dtmp = (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
                    REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
                    snd_bytes -= 4;
                    p_buf += 4;
                }
            
                REG32(R_SPIN_TC(p_priv->spi_id)) = 128;
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = cmd_dtmp | 0x203;

                timeout = SPI_TIME_OUT;
                while(!spi_concerto_is_trans_complete(p_priv->spi_id) && timeout)
                {
                    timeout --;
                }
                if(timeout == 0)
                {
                    printk("spi timeout  %d  %s\n",__LINE__, __FILE__);
                }
            }

            if(left)
            {
                i = left / 4;
                r = left % 4;
                
                while(i)
                {
                    dtmp = (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
                    REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
                    i --;
                    p_buf += 4;
                }
            
                if(r > 0)
                {
                    dtmp = 0;
                    for(j = 1; j <= r; j++)
                    {
                        dtmp |= (p_buf[0] << (8 * (4 - j)));
                        p_buf ++;
                    }
                
                    REG32(R_SPIN_TXD(p_priv->spi_id)) = dtmp;
                }
            
                REG32(R_SPIN_TC(p_priv->spi_id)) = left;
                REG32(R_SPIN_CTRL(p_priv->spi_id)) = cmd_dtmp | 0x203;

                timeout = SPI_TIME_OUT;
                while(!spi_concerto_is_trans_complete(p_priv->spi_id) && timeout)
                {
                    timeout --;
                }
                if(timeout == 0)
                {
                    printk("spi timeout  %d  %s\n",__LINE__, __FILE__);
                }
            }

        }
    }

    REG32(R_SPIN_MODE_CFG(p_priv->spi_id)) |= (1 << 30);
    
    return SUCCESS;
}

#endif



static void spi_getDefaultCfg(spi_cfg_t *p_spiCfg)
{
    p_spiCfg->bus_clk_mhz   = 10;
    p_spiCfg->bus_clk_delay = 8;
    p_spiCfg->io_num        = 1;
    p_spiCfg->op_mode       = 0;
    p_spiCfg->spi_id = 0;
}


//設備掛載，一般而言一條匯流排只掛載一個設備,故可將其當成SPI的配置API
struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
    lld_spi_t * p_lld;
    spi_cfg_t *p_spiCfg;
    if(bus>2)
    {
        FLASH_DEBUG("Error Parameter!!!\n");
        return NULL;
    }
    p_lld = &(gSpiLLD[bus]);

    if(p_lld->used == 1)
    {
        printk("spi%d is already setup!!!\n", bus);
        return &(gSpiSlave[bus]);
    }
    
    p_spiCfg = &(gSpiCfg[bus]);
    spi_getDefaultCfg(p_spiCfg);
    p_lld->p_priv = p_spiCfg;

    p_spiCfg->bus_clk_mhz = max_hz/1000000;  //MHZ為單位
    p_spiCfg->op_mode = mode;
    p_spiCfg->spi_id = bus;
    
    //spi_concerto_init(p_lld);

    p_lld->used = 1;

    gSpiSlave[bus].bus = bus;
    gSpiSlave[bus].cs  = cs;
    
	return &(gSpiSlave[bus]);
}


//	void spi_init (void)
//	{
//	    /* Nothing to do */
//	    memset(gSpiLLD,0,sizeof(gSpiLLD));
//	    memset(gSpiSlave,0xff,sizeof(gSpiSlave));
//	    //spi_setup_slave(0, 0, 10000000, 0);
//	}

//設備卸載，目前一條匯流排只掛載一個設備，故卸載可留空
void spi_free_slave(struct spi_slave *slave)
{
    lld_spi_t * p_lld;
    
    p_lld = &(gSpiLLD[slave->bus]);

    p_lld->used = 0;
    
}


//佔用匯流排請求，使用中保證每條匯流排一個設備使用，故留空
//若一條匯流排多個設備同時佔用，則需要進行互斥
int spi_claim_bus(struct spi_slave *slave)
{
    /* Nothing to do */
    return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	/* Nothing to do */
}


static int concerto_spi_read(struct spi_slave *slave, u8 *p_cmd_buf,u32 cmd_len,
              spi_cmd_cfg_t *p_cmd_cfg,u8 *p_data_buf, u32 data_len)
{
    lld_spi_t * p_lld;
    p_lld = &(gSpiLLD[slave->bus]);
    
    //OS_PRINTF("concerto_spi_read[%d] p_data_buf[0x%x] :0x%x\n",slave->bus,p_data_buf,data_len);
    return spi_concerto_read(p_lld, p_cmd_buf,cmd_len,p_cmd_cfg,p_data_buf,data_len);
}

static int concerto_spi_write(struct spi_slave *slave, u8 *p_cmd_buf,u32 cmd_len,
              spi_cmd_cfg_t *p_cmd_cfg,u8 *p_data_buf, u32 data_len)
{
    lld_spi_t * p_lld;
    p_lld = &(gSpiLLD[slave->bus]);
    return spi_concerto_write(p_lld, p_cmd_buf,cmd_len,p_cmd_cfg,p_data_buf,data_len);
}

void concerto_spi_set_trans_ionum(struct spi_slave *slave, u8 io_num)
{
    lld_spi_t * p_lld = &(gSpiLLD[slave->bus]);
    spi_cfg_t *p_priv = (spi_cfg_t *)p_lld->p_priv;

    p_priv->io_num = io_num;
    spi_concerto_ioctrl_set_ionum(p_priv->spi_id, io_num);
    spi_concerto_ioctrl_set_pins_dir(p_priv->spi_id,io_num);

    return;
}


#if 1
int concerto_spi_cmd_read(struct spi_slave *spi, u8 *cmd,
		u32 cmd_len, void *data, u32 data_len)
{
    return concerto_spi_read(spi, cmd, cmd_len, NULL, (u8*)data, data_len);
}

int concerto_spi_cmd_write(struct spi_slave *spi, u8 *cmd, u32 cmd_len,
		const void *data, u32 data_len)
{
    return concerto_spi_write(spi, cmd, cmd_len, NULL, (u8*)data, data_len);
}

int concerto_spi_cmd(struct spi_slave *spi, u8 cmd, void *response, u32 len)
{
	return concerto_spi_cmd_read(spi, &cmd, 1, response, len);
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

    while(status && timeout)
    {
        ret = concerto_spi_cmd(spi, SPI_FLASH_CMD_RD_SR, &resp, 1);
        if(ret < 0)
        {
            timeout --;
            continue;
        }
        if((resp&0x01) == 1)
        {
            status = 1;
            timeout --;
        }
        else
        {
            status = 0;
        }
    }
    if(timeout == 0)
    {
        printk("concerto_spi_cmd_wait_ready: time out! %s %d %s\n",
                __FUNCTION__, __LINE__, __FILE__);
    }
    return 0;
}


int concerto_spi_read_common(struct spi_slave *spi, u8 *cmd,
		u32 cmd_len, void *data, u32 data_len)
{
	return concerto_spi_cmd_read(spi, cmd, cmd_len, data, data_len);
}

// the size need to be equal to page_size
void panther_spi_restore_erased_page(u8 *buf, u32 size)
{
    if (is_erased_page(buf, size))
    {
        memset(buf, 0xff, size);
    }
}

#endif






