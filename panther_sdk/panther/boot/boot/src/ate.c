/*=============================================================================+
|                                                                              |
| Copyright 2016                                                               |
| Montage Technology, Inc. All rights reserved.                                |
|                                                                              |
+=============================================================================*/
/*!
*   \file ate.c
*   \brief
*   \author Montage
*/
/*=============================================================================+
| Included Files                                                               |
+=============================================================================*/
#include <arch/chip.h>
#include <common.h>
#include <mt_types.h>
#include <pmu.h>
#include <reg.h>
#include <lib.h>
#include <cm_mac.h>
#include "ate.h"
#include "ddr_config.h"
#include <sched.h>
#include "otp.h"
#ifdef WT_TEST
#include "wtest.h"
#endif

#ifdef PHYSICAL_ADDR
#undef PHYSICAL_ADDR
#endif
#include "gdma.h"

#ifdef CONFIG_ATE
#define REG32(addr)            (*((volatile u32 *)(addr)))
#define addr_read(addr, data)  (*data = REG32(addr))
#define addr_write(addr, data) (REG32(addr) = (data))

int gpio_ids[] = {2,3,4,5,6,7,8,
                  9,10,11,12,13,14,15,16,
                  17,18,19,20,23,24,25,26,27,
                  31,-1};
unsigned long gpio_funcs[] = {0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,
                              3,3,3,3,3,3,0,0,0,
                              1,-1};
unsigned int ddr2_size;

#ifdef WT_TEST
#define WLA_TEST_THREAD_STACK_SIZE  (1024*1024)
unsigned char ate_test_thread_stack[WLA_TEST_THREAD_STACK_SIZE];
extern wla_test_cfg *acfg_p;
extern void wla_test(void);
volatile unsigned int ate_rssi;
volatile unsigned int ate_cnt;
#endif
unsigned int gpio_func_0_7 = 0;
unsigned int gpio_func_8_15 = 0;
unsigned int gpio_func_16_23 = 0;
unsigned int gpio_func_24_31 = 0;


extern void ate_set_chan(unsigned char chan, unsigned char band);

static u8 bb_register_read(int group,u32 bb_reg)
{
    u8 value=0;
    u32 data, dst;

    if(group == 0)
    {
        dst = (bb_reg/4) * 4;
    }
    else if(group == 1)
    {
        dst = ((bb_reg/4) * 4) + 0x100;
    }
    else if(group ==2)
    {
        dst = ((bb_reg/4) * 4) + 0x200;
    }
    else if(group==3)
    {
        dst = ((bb_reg/4) * 4) + 0x300;
    }
    else
    {
        printf("XXX: UnKonwn group %d\n", group);
        return 0;
    }
    data = BBREG_READ32(dst);

    value = (data >> 8*(bb_reg % 4)) & 0xffUL;

    //printf("\t\t\t\tread %d\t%08x\t%02x\n", group, bb_reg, value);
    return value;
}
static void bb_register_write(int group, u32 bb_reg, u8 value)
{
    u32 data, dst, mask=0;

    if(group==0)
    {
        dst = (bb_reg/4) * 4;
    }
    else if(group==1)
    {
        dst = ((bb_reg/4) * 4) + 0x100;
    }
    else if(group==2)
    {
        dst = ((bb_reg/4) * 4) + 0x200;
    }
    else if(group==3)
    {
        dst = ((bb_reg/4) * 4) + 0x300;
    }
    else
    {
        printf("XXX: UnKonwn group %d\n", group);
        return;
    }
    data =BBREG_READ32(dst);

//    printf("read %08x\t%08x ", dst, data);
    if(bb_reg%4==0)
    {
        mask |= 0x000000ffUL;
    }
    else if(bb_reg%4==1)
    {
        mask |= 0x0000ff00UL;
    }
    else if(bb_reg%4==2)
    {
        mask |= 0x00ff0000UL;
    }
    else if(bb_reg%4==3)
    {
        mask |= 0xff000000UL;
    }
    data = ( (data & ~(mask)) | ((value << 8*(bb_reg % 4)) & (mask)) );
    // reg0 will reset BB by write any value except zero
    // write the reg1,2,3 with the LSByte==0, so the BB won't be reset
    if(((dst==0) && (group==0)) && (bb_reg!=0))
    {
        mask = 0xffffff00UL;
        data = data & mask;
    }

//    printf("write %08x\t%08x\n", dst ,data);

    BBREG_WRITE32(dst, data);
}
/*-------------- Utils ---------------*/
void restore_pin_func(void)
{
    REG_WRITE32(PMU_GPIO_FUNC_0_7, gpio_func_0_7);
    REG_WRITE32(PMU_GPIO_FUNC_8_15, gpio_func_8_15);
    REG_WRITE32(PMU_GPIO_FUNC_16_23, gpio_func_16_23);
    REG_WRITE32(PMU_GPIO_FUNC_24_31, gpio_func_24_31);
}

void backup_pin_func(void)
{
    gpio_func_0_7   = REG_READ32(PMU_GPIO_FUNC_0_7);
    gpio_func_8_15  = REG_READ32(PMU_GPIO_FUNC_8_15);
    gpio_func_16_23 = REG_READ32(PMU_GPIO_FUNC_16_23);
    gpio_func_24_31 = REG_READ32(PMU_GPIO_FUNC_24_31);
}

void pinmux_set_pin_func_gpio(void)
{
    pmu_set_gpio_function(gpio_ids, gpio_funcs);
    return;
}

void gpio_func_direction_val(int input, int high)
{
    if(input)
    {
        PMUREG_WRITE32(GPIO_FUNC_EN, 0x8F9FFFFC);
        PMUREG_WRITE32(GPIO_OEN, 0x8F9FFFFC);
    }
    else
    {
        PMUREG_WRITE32(GPIO_FUNC_EN, 0x8F9FFFFC);
        PMUREG_WRITE32(GPIO_OEN, 0x0);
        if(high)
            PMUREG_WRITE32(GPIO_ODS, 0x8F9FFFFC);
        else
            PMUREG_WRITE32(GPIO_ODC, 0x8F9FFFFC);
    }
}

void pull_down_set(void)
{
    int idx, pin;
    unsigned int data = 0;

    for(idx=0; idx<(sizeof(gpio_ids)/sizeof(int));idx++)
    {
        pin = gpio_ids[idx];
        if(pin == -1)
            break;
        if(pin < 8)
        {
            REG_UPDATE32(GPIO_DRIV10, 3<<(pin*4), 0xf<<(pin*4));
        }
        else if((pin >= 8) && (pin<16))
        {
            pin-=8;
            REG_UPDATE32(GPIO_DRIV11, 3<<(pin*4), 0xf<<(pin*4));
        }
        else if((pin >= 16) && (pin<24))
        {
            pin-=16;
            REG_UPDATE32(GPIO_DRIV12, 3<<(pin*4), 0xf<<(pin*4));
        }
        else if (pin == 24)
        {
            pin-=24;
            REG_UPDATE32(GPIO_DRIV13, 3<<(pin*4), 0xf<<(pin*4));
        }
    }
}

void driving_str(void)
{
    int idx, pin, val1=0, val2=0;

    for(idx=0; idx<(sizeof(gpio_ids)/sizeof(int)); idx++)
    {
        pin = gpio_ids[idx];
        if(pin == -1)
            break;
        if(pin < 16)
        {
            val1 |= 3<<(pin*2);
        }
        else if (pin < 32)
        {
            pin-=16;
            val2 |= 3<<(pin*2);
        }
    }
    if(val1 != 0)
        REG_WRITE32(GPIO_DRIV01, val1);
    if(val2 != 0)
        REG_WRITE32(GPIO_DRIV02, val2);
}

/*-------------- Test Item ---------------*/
static void pin_input_init(void)
{
    backup_pin_func();
    pinmux_set_pin_func_gpio();
    gpio_func_direction_val(1,0);
    pull_down_set();
}

static void dc_tst(void)
{
    printf("%s\n",__func__);
}

static void ip_tst(void)
{
    printf("%s\n",__func__);
}

static void wrt(void)
{
    REG_WRITE32(RESULT3, PMUREG(GPVAL));
}

static void vol(void)
{
    /*Set goip output direction and value low*/
    driving_str();
    gpio_func_direction_val(0, 0);
}

static void voh(void)
{
    /*Set goip output direction and value high*/
    driving_str();
    gpio_func_direction_val(0, 1);
}

static void ts_md(void)
{
    printf("%s\n",__func__);
}

/******************************************************
 * USB / EPHY / DDR
 * DDR Testing on power on. 
 *
*******************************************************/
#ifdef USB_TEST
#if 0
static void std_pkt_usb(void)
{
    int i;

    //only loopback mode test
    //operation
    USB0REGW(USB_TEST_MODE_REG, 0x6);
    USB1REGW(USB_TEST_MODE_REG, 0x6);

    for(i=HIGH_SPEED; i<=LOW_SPEED; i++)
    {
        //set HIGH/FULL/LOW mode bit[5:4]
        USB0_UPDATE32(USB_TEST_PKT_CFG_REG, (i<<4), 0x30);
        USB1_UPDATE32(USB_TEST_PKT_CFG_REG, (i<<4), 0x30);
        //case4
        USB0_UPDATE32(USB_PHY_DIG_CFG, 0x1000, 0x1000);
        USB1_UPDATE32(USB_PHY_DIG_CFG, 0x1000, 0x1000);
        //LoopBack
        USB0_UPDATE32(USB_TEST_PKT_CFG_REG, 0x1, 0x1);
        USB1_UPDATE32(USB_TEST_PKT_CFG_REG, 0x1, 0x1);

        //check loopback complete bit[0]
        while(!(USB0REGR(USB_TEST_PKT_STS_REG)&0x01) &&
                !(USB1REGR(USB_TEST_PKT_STS_REG)&0x01))
            ;
        //check status bit[2]
        printf("USB0 Status:%x\n",USB0REGR(USB_TEST_PKT_STS_REG)&0x4);
        printf("USB1 Status:%x\n",USB1REGR(USB_TEST_PKT_STS_REG)&0x4);
    }
}
#endif
static void psudo_case4(unsigned char mode, char ebit)
{
    unsigned int status;

    //USB0
    //loopback bit[12]
    //USB0_UPDATE32(USB_PHY_DIG_CFG, 0x1000, 0x1000);
    USB0REGW(USB_PHY_DIG_CFG, 0x1F30CC);

    //set HIGH/FULL/LOW speed mode bit[6:5], and enable/disable error bit bit[4], RX/TX bit[1:0]
    USB0_UPDATE32(USB_BIST_CFG_REG, (mode<<5)|(ebit<<4)|0x3, 0x73);

    udelay(1000);
    status = USB0REGR(USB_BIST_BIT_ERR_DETECTED_REG)&0x1;

    //save result
    if(!ebit)
        REG_UPDATE32(RESULT2, REG_READ32(RESULT2)|(status<<(mode*2)), 0x3F);
    else
        REG_UPDATE32(RESULT2, REG_READ32(RESULT2)|(status<<(mode*2+1)), 0x3F);
/*
    if(ebit && status)
        printf("USB0 Pass: Psudo ERR Dected:%x ebit:%d\n",status, ebit);
    else if (!ebit && !status)
        printf("USB0 Pass: Psudo ERR Dected:%x ebit:%d\n",status, ebit);
    else
        printf("USB0 Fail: Psudo ERR Dected:%x ebit:%d\n",status, ebit);
*/
    USB0_UPDATE32(USB_PHY_DIG_CFG, 0x0, 0x1000);

    //USB1
    //loopback bit[12]
    //USB1_UPDATE32(USB_PHY_DIG_CFG, 0x1000, 0x1000);
    USB1REGW(USB_PHY_DIG_CFG, 0x1F30CC);

    //set HIGH/FULL/LOW speed mode bit[6:5], and enable/disable error bit bit[4], RX/TX bit[1:0]
    USB1_UPDATE32(USB_BIST_CFG_REG, (mode<<5)|(ebit<<4)|0x3, 0x73);

    udelay(1000);
    status = USB1REGR(USB_BIST_BIT_ERR_DETECTED_REG)&0x1;

    //save result
    if(!ebit)
        REG_UPDATE32(RESULT2, REG_READ32(RESULT2)|(status<<(mode*2))<<8, 0x3F00);
    else
        REG_UPDATE32(RESULT2, REG_READ32(RESULT2)|(status<<(mode*2+1))<<8, 0x3F00);
/*
    if(ebit && status)
        printf("USB1 Pass: Psudo ERR Dected:%x ebit:%d\n",status, ebit);
    else if (!ebit && !status)
        printf("USB1 Pass: Psudo ERR Dected:%x ebit:%d\n",status, ebit);
    else
        printf("USB1 Fail: Psudo ERR Dected:%x ebit:%d\n",status, ebit);
*/
    USB1_UPDATE32(USB_PHY_DIG_CFG, 0x0, 0x1000);
}

static void psudo_pkt_usb(int start_mode)
{
    int i;
    int start, end;

    if(start_mode == HIGH_SPEED)
    {
        start = HIGH_SPEED;
        end = HIGH_SPEED;
    }
    else if(start_mode == LOW_SPEED)
    {
        start = LOW_SPEED;
        end = LOW_SPEED;
    }
    else
    {
        start = FULL_SPEED;
        end = FULL_SPEED;
    }

    for(i=start; i<=end; i++)
    {
        USB0REGW(USB_TEST_MODE_REG, 0x0);
        USB1REGW(USB_TEST_MODE_REG, 0x0);

        USB0REGW(USB_BIST_CFG_REG, 0x0);
        USB0REGW(USB_BIST_PKT_LEN_REG, 0x40);
        USB0REGW(USB_BIST_CFG_REG, 0x2);

        USB1REGW(USB_BIST_CFG_REG, 0x0);
        USB1REGW(USB_BIST_PKT_LEN_REG, 0x40);
        USB1REGW(USB_BIST_CFG_REG, 0x2);

        USB0REGW(USB_BIST_CFG_REG, 0x0);
        USB0REGW(USB_BIST_PKT_LEN_REG, 0x40);
        USB0REGW(USB_BIST_CFG_REG, 0x1);

        USB1REGW(USB_BIST_CFG_REG, 0x0);
        USB1REGW(USB_BIST_PKT_LEN_REG, 0x40);
        USB1REGW(USB_BIST_CFG_REG, 0x1);

        USB0REGW(USB_BIST_CFG_REG, 0x0);
        USB1REGW(USB_BIST_CFG_REG, 0x0);

        USB0REGW(USB_TEST_MODE_REG, 0x7);
        USB1REGW(USB_TEST_MODE_REG, 0x7);
        udelay(10000);

        //disable error bit
        psudo_case4(i,0);

        //enable error bit
        psudo_case4(i,1);
    }
    USB0REGW(USB_TEST_MODE_REG, 0x0);
    USB0REGW(USB_BIST_CFG_REG, 0x0);
    USB1REGW(USB_TEST_MODE_REG, 0x0);
    USB1REGW(USB_BIST_CFG_REG, 0x0);
}

static void usb_test_low(void)
{
    //Set to normal operation
    USB0REGW(USB_TEST_MODE_REG, 0x0);
    USB1REGW(USB_TEST_MODE_REG, 0x0);

    //Start test
    //std_pkt_usb();
    psudo_pkt_usb(LOW_SPEED);
}

static void usb_test_full(void)
{
    //Set to normal operation
    USB0REGW(USB_TEST_MODE_REG, 0x0);
    USB1REGW(USB_TEST_MODE_REG, 0x0);

    //Start test
    //std_pkt_usb();
    psudo_pkt_usb(FULL_SPEED);
}

static void usb_test_high(void)
{
    //Set to normal operation
    USB0REGW(USB_TEST_MODE_REG, 0x0);
    USB1REGW(USB_TEST_MODE_REG, 0x0);

    //Start test
    //std_pkt_usb();
    psudo_pkt_usb(HIGH_SPEED);
}
#endif

#ifdef EPHY_TEST
#define ETHPHY 2
static void eth_test(void)
{
#define UDELAY 1000
    int val1, val2;

    REG_WRITE32(RESULT1, 0);
    REG_WRITE32(0xbf004a58, 0x13f98047);

    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x14, 0x6000);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x1f, 0x2);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x11, 0x8059);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x12, 0x8975);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x13, 0x6a60);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x1f, 0x4);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x12, 0x5a40);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x1b, 0xc0);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x10, 0xb5a0);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x11, 0xa528);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x12, 0x0f90);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x17, 0x6bbd);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x17, 0x6bbc);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x18, 0xf400);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x13, 0xa4d8);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x14, 0x3780);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x15, 0xb600);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x14, 0xb100);
    udelay(UDELAY);

    //ETH bist
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x13, 0xa000);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x19, 0x400);    //analog front end loopback
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x0, 0xa000);
    udelay(UDELAY);

    cm_mdio_wr(ETHPHY, 0x1f, 0x4);
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x10, 0x4);      //AFE loopback
    //cm_mdio_wr(ETHPHY, 0x10, 0x1);    //RJ45 loopback
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x11, 0x200); //Start bist
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x11, 0x8000);
    udelay(UDELAY);

    //verify
    val1 = cm_mdio_rd(ETHPHY, 0x11) & 0xffff;
    REG_WRITE32(RESULT1, val1);

    //force error
    cm_mdio_wr(ETHPHY, 0x11, 0xa00); //Force bist error and start
    udelay(UDELAY);
    cm_mdio_wr(ETHPHY, 0x11, 0x8000);
    udelay(UDELAY);

    //verify
    val2 = cm_mdio_rd(ETHPHY, 0x11) & 0xffff;
    REG_WRITE32(RESULT1, val1|val2<<16);
}
#endif

static void ip_bk(void)
{
#ifdef EPHY_TEST
    eth_test();
#endif
}

static void usb_ini(void)
{
#ifdef USB_TEST
    REG_WRITE32(RESULT2, 0);
#endif
}

static void usb_lo(void)
{
#ifdef USB_TEST
    usb_test_low();
#endif
}

static void usb_fu(void)
{
#ifdef USB_TEST
    usb_test_full();
#endif
}

static void usb_hi(void)
{
#ifdef USB_TEST
    usb_test_high();
#endif
}

static void rf_tst(char flag)
{
#ifdef WT_TEST
    if(acfg_p)
    {
        //tx
        if(flag == WIFI_TX)
        {
            if(acfg_p->start)
            {
                printf("Already start\n");
                return;
            }
            acfg_p->rx_echo = 0;

            acfg_p->tx_repeat = -1;
            acfg_p->start = 1;
            // clear counter
            bb_register_write(0, 0x80, 0xc0);
            // enable counter
            bb_register_write(0, 0x80, 0x80);

            thread_create(wla_test, (void *) 0, &ate_test_thread_stack[WLA_TEST_THREAD_STACK_SIZE], WLA_TEST_THREAD_STACK_SIZE);
            printf("TX Start\n");
        }
        //rx
        else if(flag == WIFI_RX)
        {
            if(acfg_p->start)
            {
                printf("Already start\n");
                return;
            }
            REG_WRITE32(RESULT4, 0);
            acfg_p->tx_repeat = 0;
            acfg_p->iteration = 0;

            acfg_p->rx_echo = 0;
            acfg_p->rx_drop = 1;
            acfg_p->start = 1;
            acfg_p->expect_count = 100;
            // clear counter
            bb_register_write(0, 0x80, 0xc0);
            ate_cnt = 0;
            ate_rssi = 0;
            // enable counter
            bb_register_write(0, 0x80, 0x80);
            thread_create(wla_test, (void *) 0, &ate_test_thread_stack[WLA_TEST_THREAD_STACK_SIZE], WLA_TEST_THREAD_STACK_SIZE);
            printf("RX Start\n");
        }
        else if(flag == WIFI_STOP)
        {
            unsigned long value;

            // disable counter
            bb_register_write(0, 0x80, 0x0);

            acfg_p->start = 0;
            acfg_p->indicate_stop = 1;

            // read ok counter high byte
            value = (bb_register_read(0, 0x89) << 8);
            // read ok counter low byte
            value |= bb_register_read(0, 0x8a);

            if(ate_cnt == 0)
                printf("No pkt recv\n");
            else
                REG_WRITE32(RESULT4, ((ate_rssi/ate_cnt)<<16)|ate_cnt);
            ate_rssi=0;
            ate_cnt=0;
        }
    }
#endif
}

extern int ai_test_ladda(void);
static void aud_tst(void)
{
#ifdef AUD_TEST
    ai_test_ladda();
#if 0
    //CH0_OP8
    REG_WRITE32(CLK_EN_CTRL, 0xffffffff);
    REG_WRITE32(ANA_TEST_CTRL, 0x15);
    REG_WRITE32(CPLL_REG, 0x30000f70);
    REG_WRITE32(AUDIO_ADC, 0x40000);
    REG_WRITE32(0xBF00704C, 0xc);

    //CH1_OP8
    REG_WRITE32(CLK_EN_CTRL, 0xffffffff);
    REG_WRITE32(ANA_TEST_CTRL, 0x15);
    REG_WRITE32(CPLL_REG, 0x30000f70);
    REG_WRITE32(AUDIO_ADC, 0x80000);
    REG_WRITE32(0xBF00704C, 0xc);

    //CH0_OP2
    REG_WRITE32(CLK_EN_CTRL, 0xffffffff);
    REG_WRITE32(ANA_TEST_CTRL, 0x15);
    REG_WRITE32(CPLL_REG, 0x30000f70);
    REG_WRITE32(AUDIO_ADC, 0x40010);
    REG_WRITE32(0xBF00704C, 0xc);

    //CH1_OP2
    REG_WRITE32(CLK_EN_CTRL, 0xffffffff);
    REG_WRITE32(ANA_TEST_CTRL, 0x15);
    REG_WRITE32(CPLL_REG, 0x30000f70);
    REG_WRITE32(AUDIO_ADC, 0x81000);
    REG_WRITE32(0xBF00704C, 0xc);
#endif
#endif
}

#define PKG_MODE_CTRL_REG 0xBF004AFC
#define ATE_ADC_CTRL_REG 0xBF004C2C
static void mad_tst_chani(void)
{
    //Chan I
    REG_WRITE32(PKG_MODE_CTRL_REG, 6<<4);
    REG_WRITE32(ATE_ADC_CTRL_REG, 1<<3);
}
static void mad_tst_chanq(void)
{
    //Chan Q
    REG_WRITE32(PKG_MODE_CTRL_REG, 6<<4);
    REG_WRITE32(ATE_ADC_CTRL_REG, 0<<3);
}

#if 0
static void reg_tst(void)
{
    unsigned int reg_offset[]=
    {
        0xbf0048FC,
        0xbf005510,
        0xbf005514,
        0xbf005520,
        0xbf005524,
        /*
        0x18,
        0x1c,
        0x24,
        0x28,
        0x2c,
        */
    };
    int i, j;
    for(i=0; i<sizeof(reg_offset)/4; i++)
    {
        REG_WRITE32(reg_offset[i],0);
        for(j=0; j<32; j++)
        {
            unsigned int val;
            REG_UPDATE32(reg_offset[i],1<<j,1<<j);
            //verify
            val = REG_READ32(reg_offset[i]);
            if((1<<j)&val)
                printf("Success: val:%x reg_offset:%x\n",val,reg_offset[i]);
            else {
                printf("Fail: val:%x reg_offset:%x j:%d\n",val,reg_offset[i],j);
                goto fail;
            }
        }
    }
    for(i=0; i<sizeof(reg_offset)/4; i++)
    {
        for(j=0; j<32; j++)
        {
            unsigned int val;
            REG_UPDATE32(reg_offset[i],0,1<<j);
            //verify
            val = REG_READ32(reg_offset[i]);
            if(!((1<<j)&val))
                printf("Success: val:%x reg_offset:%x\n",val,reg_offset[i]);
            else {
                printf("Fail: val:%x reg_offset:%x j:%d\n",val,reg_offset[i],j);
                goto fail;
            }
        }
    }
fail:
    return;
}
#endif

/*EPHY_DC*/
static void prep_eth_dc(void)
{
    REG_UPDATE32(0xBF004A58, 1<<19, 1<<19);
    REG_UPDATE32(0xBF004A58, 1<<24, 1<<24);
    REG_WRITE32(0xBF004C28, 0x13);
    REG_UPDATE32(0xBF004C88, 1<<11, 0x3800);
}

static void rx100_tst(void)
{
    prep_eth_dc();
    cm_mdio_wr(ETHPHY, 0x19, 0x9800);
    cm_mdio_wr(ETHPHY, 0x1b, 0x0);
    cm_mdio_wr(ETHPHY, 0x16, 0xc910);
}

static void vcm_ref_tst(void)
{
    prep_eth_dc();
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    cm_mdio_wr(ETHPHY, 0x1b, 0xc0);
    cm_mdio_wr(ETHPHY, 0x19, 0xa000);
    cm_mdio_wr(ETHPHY, 0x11, 0x538);
}

static void vcm_tx_tst(void)
{
    prep_eth_dc();
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    cm_mdio_wr(ETHPHY, 0x14, 0x4000);
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    cm_mdio_wr(ETHPHY, 0x19, 0xb000);
    cm_mdio_wr(ETHPHY, 0x11, 0x538);
}

static void vbn_tst(void)
{
    prep_eth_dc();
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    cm_mdio_wr(ETHPHY, 0x14, 0x4000);
    cm_mdio_wr(ETHPHY, 0x0, 0x2000);
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    cm_mdio_wr(ETHPHY, 0x19, 0xb800);
    cm_mdio_wr(ETHPHY, 0x10, 0xf400);
}

static void vbnc_tst(void)
{
    prep_eth_dc();
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    cm_mdio_wr(ETHPHY, 0x14, 0x4000);
    cm_mdio_wr(ETHPHY, 0x0, 0x2000);
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    cm_mdio_wr(ETHPHY, 0x19, 0xc000);
    cm_mdio_wr(ETHPHY, 0x10, 0xf400);
}

static void vbp_cmfb_tst(void)
{
    prep_eth_dc();
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    cm_mdio_wr(ETHPHY, 0x14, 0x4000);
    cm_mdio_wr(ETHPHY, 0x0, 0x2000);
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    cm_mdio_wr(ETHPHY, 0x19, 0xc800);
    cm_mdio_wr(ETHPHY, 0x10, 0xf400);
}

static void vbp_tst(void)
{
    prep_eth_dc();
    cm_mdio_wr(ETHPHY, 0x1f, 0x0);
    cm_mdio_wr(ETHPHY, 0x14, 0x4000);
    cm_mdio_wr(ETHPHY, 0x0, 0x2000);
    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    cm_mdio_wr(ETHPHY, 0x19, 0xd000);
    cm_mdio_wr(ETHPHY, 0x10, 0xf400);
}
/*END EPHY_DC*/

/*DDR_DC*/
volatile int ddr_dc_start = 0;
volatile int ddr_dc_testcase = 0;
static void ddr_dc(void)
{
    unsigned int temp,temp_read_reg,init_read_reg;
    unsigned int cycle_ibias_gen, cycle_testcase;

    temp = ANAREG_READ32(CPLL_REG);
    temp = temp&~(7<<11);
    ANAREG_WRITE32(CPLL_REG, temp|1<<11);

    temp = ANAREG_READ32(ANA_TEST_CTRL);
    temp = temp&~(0x1f<<0);
    ANAREG_WRITE32(ANA_TEST_CTRL, temp|0x11);

    //REG_WRITE32(0xbf000858, 0x0);
    REG_WRITE32(0xbf000854, 0x0); //ok

    /*Normal Temp*/
    init_read_reg = REG_READ32(0xbf000858);
    temp_read_reg = init_read_reg & ~(1<<3);
    REG_WRITE32(0xbf000858, temp_read_reg);

    cycle_testcase = ddr_dc_testcase;
    for(cycle_ibias_gen = 0; cycle_ibias_gen < 0x1; cycle_ibias_gen++)
    {
        if(cycle_testcase == 8)
            continue;
        REG_WRITE32(0xbf000850, cycle_ibias_gen<<9);
        REG_WRITE32(0xbf000854, cycle_testcase<<2|1<<0);
    }

    ddr_dc_start = 0;

    thread_exit();
}
/*END DDR_DC*/

static void pll_clk(void)
{
    unsigned short rd;

    REG_UPDATE32(0xBF004C28, 0, 1<<0);
    REG_UPDATE32(0xBF004C88, 6<<11, 7<<11);
    REG_UPDATE32(0xBF004F0C, 0, 1<<29);
    REG_UPDATE32(0xBF004F04, 1<<7, 7<<7);
    REG_UPDATE32(0xBF004C88, 4<<20, 7<<20);

    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    rd = cm_mdio_rd(ETHPHY, 0x1b);
    rd =rd&~(0x20);
    cm_mdio_wr(ETHPHY, 0x1b, rd);

}

static void pll_ephy(void)
{
    unsigned short rd;

    REG_UPDATE32(0xBF004C28, 0, 1<<0);
    REG_UPDATE32(0xBF004C88, 4<<20, 7<<20);
    REG_UPDATE32(0xBF004C88, 6<<11, 7<<11);
    REG_UPDATE32(0xBF004F0C, 0, 1<<29);
    REG_UPDATE32(0xBF004F04, 0, 7<<7);

    cm_mdio_wr(ETHPHY, 0x1f, 0x1);
    rd = cm_mdio_rd(ETHPHY, 0x1b);
    rd = rd|(7<<3);
    cm_mdio_wr(ETHPHY, 0x1b, rd);

}

#define REG1_MIN 0xBF00485C
#define REG1_MAX 0xBF004864
#define REG2_MIN 0xBF00487C
#define REG2_MAX 0xBF0048A4
#define REG3_MIN 0xBF0048EC
#define REG3_MAX 0xBF004900
#define REG4_MIN 0xBF004F04
#define REG4_MAX 0xBF004F68

#define TEST_PATTERN1 0x55555555
#define TEST_PATTERN2 0xAAAAAAAA

#define SKIP_REG1 0xBF0048F0
#define SKIP_REG2 0xBF004F34

#define REG_REGION "0xBF00_485C <= address < 0xBF00_4864\n" \
                   "0xBF00_487C <= address < 0xBF00_48A4\n" \
                   "0xBF00_48EC <= address < 0xBF00_4900\n" \
                   "0xBF00_4F04 <= address < 0xBF00_4F68\n" \
                   "Skip 0xBF0048F0 and 0xBf004F34\n"

static int reg_test(reg)
{
    unsigned int reg_val;

    //printf("0x%x\n",reg);
    REG_WRITE32(reg, TEST_PATTERN1);
    reg_val=REG_READ32(reg);
    if(reg_val != TEST_PATTERN1)
        goto fail_case1;

    REG_WRITE32(reg, TEST_PATTERN2);
    reg_val=REG_READ32(reg);
    if(reg_val != TEST_PATTERN2)
        goto fail_case2;

    return 0;

fail_case1:
    printf("Test Pattern %x Fail on reg:0x%x\n",TEST_PATTERN1, reg);
    return 1;
fail_case2:
    printf("Test Pattern %x Fail on reg:0x%x\n",TEST_PATTERN2, reg);
    return 1;
}

static void reg_test_all(void)
{
    unsigned int reg=0;
    int ret=0;

    /*Reset result*/
    REG_UPDATE32(RESULT2,0,0xF0000);

    for(reg=REG1_MIN; reg<REG1_MAX; reg+=4)
    {
        if((ret=reg_test(reg)))
        {
            REG_UPDATE32(RESULT2,0x10000,0x10000);
            break;
        }
    }

    for(reg=REG2_MIN; reg<REG2_MAX; reg+=4)
    {
        if((ret=reg_test(reg)))
        {
            REG_UPDATE32(RESULT2,0x20000,0x20000);
            break;
        }
    }

    for(reg=REG3_MIN; reg<REG3_MAX; reg+=4)
    {
        if(reg==SKIP_REG1)
            continue;
        if((ret=reg_test(reg)))
        {
            REG_UPDATE32(RESULT2,0x40000,0x40000);
            break;
        }
    }

    for(reg=REG4_MIN; reg<REG4_MAX; reg+=4)
    {
        if(reg==SKIP_REG2)
            continue;
        if((ret=reg_test(reg)))
        {
            REG_UPDATE32(RESULT2, 0x80000, 0x80000);
            break;
        }
    }

    return;
}

static void ate_scan_fail(unsigned int reg)
{
    if(reg < REG1_MIN)
        goto reg_fail;
    else if((reg >= REG1_MAX) && (reg < REG2_MIN))
        goto reg_fail;
    else if ((reg >= REG2_MAX) && (reg < REG3_MIN))
        goto reg_fail;
    else if((reg >= REG3_MAX) && (reg < REG4_MIN))
        goto reg_fail;
    else if(reg >= REG4_MAX)
        goto reg_fail;
    if((reg == SKIP_REG1) || (reg == SKIP_REG2))
        goto reg_fail;

    reg_test(reg);
    return ;
reg_fail:
    printf("Register is out of test range.\n");
    printf("%s\n",REG_REGION);
    return;
}

int ate_cmd(int argc, char *argv[])
{
    int pin;
#define CLK_ENABLE_CTRL_REG 0xBF004A58UL
#define GDMA_DISABLE_MASK   0x00000008UL
    REG_UPDATE32(CLK_ENABLE_CTRL_REG, GDMA_DISABLE_MASK, GDMA_DISABLE_MASK);

    addr_read(DDR_SIZE_INFO_ADDR,&ddr2_size);
    //printf("SRAM SIZE:%d\n",ddr2_size);
    if(argc >= 1)
    {
        if(!strcmp("prep", argv[0])) {
            pin_input_init();
            return ERR_OK;
        }
        else if(!strcmp("recover", argv[0])) {
            if(gpio_func_0_7 != 0)
                restore_pin_func();
            return ERR_OK;
        }

        /*Test Items*/
        if(!strcmp("dc_tst", argv[0]))
            dc_tst();
        else if(!strcmp("ip_tst", argv[0]))
            ip_tst();
        else if(!strcmp("wrt", argv[0]))
            wrt();
        else if(!strcmp("vol", argv[0]))
            vol();
        else if(!strcmp("voh", argv[0]))
            voh();
        else if(!strcmp("ts_md", argv[0]))
            ts_md();
        else if(!strcmp("ip_bk", argv[0]))
            ip_bk();
        else if(!strcmp("usb_ini", argv[0]))
            usb_ini();
        else if(!strcmp("usb_lo", argv[0]))
            usb_lo();
        else if(!strcmp("usb_fu", argv[0]))
            usb_fu();
        else if(!strcmp("usb_hi", argv[0]))
            usb_hi();
        else if(!strcmp("rf_tst", argv[0]))
        {
            if(argc >= 2)
            {
                if(!strcmp("tx", argv[1]))
                {
                    if(argc >= 3)
                    {
                        unsigned char chan = 1;
                        unsigned char s_chan = 0; //20
                        unsigned char tx_rate = 5;//b,g,n
                        chan = atoi(argv[2]);
                        if(argc >= 4)
                        {
                            s_chan = atoi(argv[3]);
                            if(argc == 5)
                                acfg_p->tx_rate = atoi(argv[4]);
                        }

                        printf("ch:%d s-chan:%d rate:%d\n",chan, s_chan, acfg_p->tx_rate);
                        ate_set_chan(chan, s_chan);
                    }
                    rf_tst(WIFI_TX);
                }
                else if(!strcmp("rx", argv[1]))
                {
                    if(argc >= 3)
                    {
                        unsigned char chan = 1;
                        unsigned char s_chan = 0; //20
                        chan = atoi(argv[2]);

                        if(argc == 4)
                            s_chan = atoi(argv[3]);
                        printf("ch:%d s_chan:%d\n",chan,s_chan);
                        ate_set_chan(chan, s_chan);
                    }
                    rf_tst(WIFI_RX);
                }
                else if(!strcmp("stop", argv[1]))
                    rf_tst(WIFI_STOP);
            }
        }
        else if(!strcmp("aud_tst", argv[0]))
            aud_tst();
        else if(!strcmp("mad_tst", argv[0]))
        {
            if(argc == 2)
            {
                if(!strcmp("0", argv[1]))
                    mad_tst_chani();
                else if(!strcmp("1", argv[1]))
                    mad_tst_chanq();
                else
                    printf("paremeter must be 0/1\n");
            }
        }
#if 0 //for test register write
        else if(!strcmp("reg", argv[0])) //test 10 regs
            reg_tst();
#endif
        else if(!strcmp("rx100_tst", argv[0]))
            rx100_tst();
        else if(!strcmp("vcm_ref_tst", argv[0]))
            vcm_ref_tst();
        else if(!strcmp("vcm_tx_tst", argv[0]))
            vcm_tx_tst();
        else if(!strcmp("vbn_tst", argv[0]))
            vbn_tst();
        else if(!strcmp("vbnc_tst", argv[0]))
            vbnc_tst();
        else if(!strcmp("vbp_cmfb_tst", argv[0]))
            vbp_cmfb_tst();
        else if(!strcmp("vbp_tst", argv[0]))
            vbp_tst();
        else if(!strcmp("ddr_dc", argv[0]))
        {
            if(ddr_dc_start == 0)
            {
                ddr_dc_start = 1;
                ddr_dc_testcase = atoi(argv[1]);
                if(ddr_dc_testcase < 0 || ddr_dc_testcase > 15)
                    ddr_dc_testcase = 0;
                thread_create(ddr_dc, (void *) 0, &ate_test_thread_stack[WLA_TEST_THREAD_STACK_SIZE], WLA_TEST_THREAD_STACK_SIZE);
            }
        }
        else if(!strcmp("pll", argv[0]))
        {
            if(argc == 2)
            {
                if(!strcmp("clk", argv[1]))
                    pll_clk();
                else if(!strcmp("ephy", argv[1]))
                    pll_ephy();
                else
                    printf("paremeter must be clk/ephy\n");
            }
        }
        else if(!strcmp("regt", argv[0]))
        {
            if(argc == 2)
            {
                unsigned int reg;
                sscanf(argv[1], "%x", &reg);
                ate_scan_fail(reg);
            }
            else
                reg_test_all();
        }
        else if(!strcmp("rfc", argv[0]))
        {
            dratini_start();
        }
        else if(!strcmp("otpr", argv[0]))
        {
            int ret;
            ret = otp_ate_read();
            if(ret == 0)
                REG_UPDATE32(RESULT5, 0, 0xf);
            else if(ret == 1)
                REG_UPDATE32(RESULT5, 0x1, 0xf);
            else
                REG_UPDATE32(RESULT5, 0x2, 0xf);
        }
        else if(!strcmp("otpw", argv[0]))
        {
            int ret;
            ret = otp_ate_write();
            if(ret == 0)
                REG_UPDATE32(RESULT5, 0, 0xf << 4);
            else
                REG_UPDATE32(RESULT5, 0x1 << 4, 0xf << 4);
        }
        else
            return ERR_HELP;
        return ERR_OK;
    }
    else
        return ERR_HELP;
}

cmdt cmdt_ate[] __attribute__ ((section("cmdt"))) =
{
    {
    "ate", ate_cmd, "Build date:" __DATE__" "__TIME__ ".\n"
            "ate: ATE Item\n"
            "\tate prep\n"
            "\tate dc_tst\n"
            "\tate ip_tst\n"
            "\tate wrt reg0\n"
            "\tate vol\n"
            "\tate voh\n"
            "\tate ts_md\n"
            "\tate ip_bk\n"
            "\tate usb_ini\n"
            "\tate usb_lo\n"
            "\tate usb_fu\n"
            "\tate usb_hi\n"
            "\tate rf_tst [tx/rx]\n"
            "\tate aud_tst\n"
            "\tate mad_tst\n"
            "\tate rx100_tst\n"
            "\tate vcm_ref_tst\n"
            "\tate vcm_tx_tst\n"
            "\tate vbn_tst\n"
            "\tate vbnc_tst\n"
            "\tate vbp_cmfb_tst\n"
            "\tate vbp_tst\n"
            "\tate ddr_dc\n"
            "\tate pll [clk/ephy]\n"
            "\tate regt\n"
            "\tate otpr\n"
            "\tate otpw\n"
            }
,};
#endif
