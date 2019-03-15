/*
 *  Copyright (c) 2017	Montage Inc.	All rights reserved.
 */
#include <linux/pm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/reg.h>
#include <asm/mach-panther/pmu.h>
#include <asm/mach-panther/pinmux.h>

#define MAX_PIN_NUM 50
static unsigned char pinmux_func[MAX_PIN_NUM];
static int pinmux_info_ready;
#if 0 // suspend/resume try to cancel popup noise
static int mclk_pin_number = 0;
#endif
void pinmux_read_info(void)
{
    int gpio_ids[MAX_PIN_NUM + 1];
    unsigned long gpio_funcs[MAX_PIN_NUM + 1];
    int i;

    if(pinmux_info_ready)
        return;

    for(i=0;i<MAX_PIN_NUM;i++)
    {
        gpio_ids[i] = i;
        gpio_funcs[i] = 10;
    }
    gpio_ids[i] = -1;

    pmu_get_gpio_function(gpio_ids, gpio_funcs);

    for(i=0;i<MAX_PIN_NUM;i++)
        pinmux_func[i] = (unsigned char) gpio_funcs[i];
}

int gspi_pinmux_selected(void)
{
#if defined(CONFIG_PANTHER_FPGA)
    return 1;
#endif

    pinmux_read_info();

    if((pinmux_func[17] == 0) && (pinmux_func[18] == 0)
        /* && (pinmux_func[19] == 0) */ && (pinmux_func[20] == 0))
        return 1;

    if((pinmux_func[5] == 5) && (pinmux_func[6] == 5)
        /* && (pinmux_func[7] == 5) */ && (pinmux_func[8] == 5))
        return 2;

    return 0;
}

int i2c_pinmux_selected(void)
{
#if defined(CONFIG_PANTHER_FPGA)
    return 1;
#endif

    pinmux_read_info();

    if((pinmux_func[0] == 1) && (pinmux_func[1] == 1))
        return 1;
    if((pinmux_func[2] == 1) && (pinmux_func[3] == 1))
        return 1;
    if((pinmux_func[10] == 1) && (pinmux_func[11] == 1))
        return 1;
    if((pinmux_func[17] == 2) && (pinmux_func[18] == 2))
        return 1;
    if((pinmux_func[23] == 2) && (pinmux_func[24] == 2))
        return 1;
    if((pinmux_func[29] == 1) && (pinmux_func[30] == 1))
        return 1;
    if((pinmux_func[40] == 1) && (pinmux_func[41] == 1))
        return 1;

    return 0;
}

int tsi_pinmux_selected(void)
{
#if defined(CONFIG_PANTHER_FPGA)
    return 1;
#endif

    pinmux_read_info();


        /*   Pinmux type 0
             GPIO0: sel 7, TS_S_CLK
             GPIO1: sel 7, TS_S_VALID
             GPIO2: sel 7, TS_S_SYNC
             GPIO3: sel 7, TS_S_ERR
             GPIO4: sel 7, TS_D0
             GPIO6: sel 7, TS_D5
             GPIO7: sel 7, TS_D6
             GPIO8: sel 7, TS_D7
             GPIO9: sel 7, TS_D1
             GPIO10: sel 7, TS_D2
             GPIO11: sel 7, TS_D3
             GPIO12: sel 7, TS_D4
         */
        /*   Pinmux type 1
             GPIO0: sel 7, TS_S_CLK
             GPIO1: sel 7, TS_S_VALID
             GPIO2: sel 7, TS_S_SYNC
             GPIO3: sel 7, TS_S_ERR
             GPIO4: sel 7, TS_D0
             GPIO9: sel 7, TS_D1
             GPIO10: sel 7, TS_D2
             GPIO11: sel 7, TS_D3
             GPIO12: sel 7, TS_D4
             GPIO13: sel 7, TS_D5
             GPIO14: sel 7, TS_D6
             GPIO15: sel 7, TS_D7
         */


    if((pinmux_func[0] == 7) && (pinmux_func[1] == 7)
        && (pinmux_func[2] == 7) && (pinmux_func[3] == 7)
        && (pinmux_func[4] == 7))
        return 1;

    return 0;
}

int uart_pinmux_selected(int uart_port_id)
{
#if defined(CONFIG_PANTHER_FPGA)
    return 1;
#endif

    pinmux_read_info();

    if(uart_port_id == 0)
    {
        if((pinmux_func[21] == 1) && (pinmux_func[22] == 1))
            return 1;
        if((pinmux_func[19] == 2) && (pinmux_func[20] == 2))
            return 1;
    }

    if(uart_port_id == 1)
    {
        if((pinmux_func[0] == 5) && (pinmux_func[1] == 5))
            return 1;
        if((pinmux_func[15] == 2) && (pinmux_func[16] == 2))
            return 1;
        if((pinmux_func[25] == 1) && (pinmux_func[26] == 1))
            return 1;
    }

    if(uart_port_id == 2)
    {
        if((pinmux_func[23] == 1) && (pinmux_func[24] == 1))
            return 1;
        if((pinmux_func[27] == 1) && (pinmux_func[28] == 1))
            return 1;
        if((pinmux_func[33] == 1) && (pinmux_func[34] == 1))
            return 1;
    }

    return 0;
}

int i2sclk_pinmux_selected(void)
{
#if defined(CONFIG_PANTHER_FPGA)
    return 1;
#endif

    pinmux_read_info();

    if((pinmux_func[2] == 5) && (pinmux_func[3] == 5) && (pinmux_func[4] == 5))
    {
#if 0 // suspend/resume try to cancel popup noise
        mclk_pin_number = 2;
#endif
        return 1;
    }

    if((pinmux_func[14] == 5) && (pinmux_func[15] == 5) && (pinmux_func[16] == 5))
    {
#if 0 // suspend/resume try to cancel popup noise
        mclk_pin_number = 14;
#endif
        return 1;
    }

    return 0;
}

#if 0 // suspend/resume try to cancel popup noise
int i2s_mclk_suspend(void)
{
    if(mclk_pin_number == 14 || mclk_pin_number == 2)
    {
        return pinmux_pin_func_gpio(mclk_pin_number);
    }
    return -EINVAL;
}

int i2s_mclk_resume(void)
{
    int gpio_ids[] = { -1, -1 };
    unsigned long gpio_funcs[] = { -1, -1 };

    if(mclk_pin_number == 14 || mclk_pin_number == 2)
    {
        gpio_ids[0] = mclk_pin_number;
        gpio_funcs[0] = 5;
        pmu_set_gpio_function(gpio_ids, gpio_funcs);
    }

    return -EINVAL;
}
#endif

int i2sdat_pinmux_selected(void)
{
#if defined(CONFIG_PANTHER_FPGA)
    return 1;
#endif

    pinmux_read_info();

    if((pinmux_func[10] == 5) && (pinmux_func[11] == 5))
        return 1;

    if((pinmux_func[21] == 5) && (pinmux_func[22] == 5))
        return 1;

    return 0;
}

int spdif_pinmux_selected(void)
{
#if defined(CONFIG_PANTHER_FPGA)
    return 1;
#endif

    pinmux_read_info();

    if(pinmux_func[1] == 3)
        return 1;

    if(pinmux_func[4] == 3)
        return 1;

    if(pinmux_func[13] == 3)
        return 1;

    if(pinmux_func[14] == 3)
        return 1;

    if(pinmux_func[22] == 3)
        return 1;

    return 0;
}

#define GPIO_DRV_12MA   (0x2)

#define GPIO5_FUNC_RMII_CLK     (8)
#define GPIO6_FUNC_RMII_TXEN    (8)
#define GPIO7_FUNC_RMII_TXD0    (8)
#define GPIO8_FUNC_RMII_TXD1    (8)
#define GPIO9_FUNC_RMII_CRS_DV  (8)
#define GPIO10_FUNC_RMII_RXD0   (8)
#define GPIO11_FUNC_RMII_RXD1   (8)
#define GPIO12_FUNC_RMII_MDC    (8)
#define GPIO13_FUNC_RMII_MDIO   (8)
void ext_phy_pinmux(void)
{
    int gpio_ids[] = { 5, 6, 7, 8, 9, 10, 11, 12, 13, -1 };
    unsigned long gpio_funcs[] = { GPIO5_FUNC_RMII_CLK,   GPIO6_FUNC_RMII_TXEN,   GPIO7_FUNC_RMII_TXD0,
                                   GPIO8_FUNC_RMII_TXD1,  GPIO9_FUNC_RMII_CRS_DV, GPIO10_FUNC_RMII_RXD0,
                                   GPIO11_FUNC_RMII_RXD1, GPIO12_FUNC_RMII_MDC,   GPIO13_FUNC_RMII_MDIO,
                                   0 };
    unsigned long gpio_vals[] = {[0 ... 8] = GPIO_DRV_12MA, 0 };

    pmu_set_gpio_function(gpio_ids, gpio_funcs);
    pmu_set_gpio_driving_strengh(gpio_ids, gpio_vals);
}

int pinmux_pin_func_gpio(int pin_number)
{
    int gpio_ids[] = { -1, -1 };
    unsigned long gpio_funcs[] = { -1, -1 };

    if((pin_number >= 0)&&(pin_number <= 16))
    {
        gpio_funcs[0] = 0;
    }
    else if((pin_number >= 17)&&(pin_number <= 20))
    {
        gpio_funcs[0] = 3;
    }
    else if((pin_number >= 21)&&(pin_number <= 22))
    {
        gpio_funcs[0] = 0;
    }

    if(gpio_funcs[0] >= 0)
    {
        gpio_ids[0] = pin_number;
        pmu_set_gpio_function(gpio_ids, gpio_funcs);
        return 0;
    }

    return -EINVAL;
}

int pinmux_pin_func_i2c(int pin_number)
{
    int gpio_ids[] = { -1, -1 };
    unsigned long gpio_funcs[] = { -1, -1 };

    if((pin_number >= 0)&&(pin_number <= 3))
    {
        gpio_funcs[0] = 1;
    }
    else if((pin_number >= 10)&&(pin_number <= 11))
    {
        gpio_funcs[0] = 1;
    }
    else if((pin_number >= 29)&&(pin_number <= 30))
    {
        gpio_funcs[0] = 1;
    }

    if(gpio_funcs[0] >= 0)
    {
        gpio_ids[0] = pin_number;
        pmu_set_gpio_function(gpio_ids, gpio_funcs);
        return 0;
    }

    return -EINVAL;
}

#if 0  // PINMUX table for easy reference
#define MSG printf
void pinmux_show_table(void)
{
    MSG("SEL DEF      0        1        2     3    4        5 6        7         8\n");
    MSG("  0   1   GPIO   I2C_SD     LED0            UART2_TX    TS_CLK           \n");
    MSG("  1   1   GPIO  I2C_SCL     LED1 SPDIF      UART2_RX    TS_VLD           \n");
    MSG("  2   0   GPIO   I2C_SD     LED2       PWM0 I2S_MCLK   TS_SYNC           \n");
    MSG("  3   0   GPIO  I2C_SCL     LED1       PWM1 I2S_BCLK    TS_ERR           \n");
    MSG("  4   0   GPIO              LED0 SPDIF PWM2  I2S_LRC     TS_D0           \n");
    MSG("  5   9   GPIO  PA_PAON     LED0 LCDDC PWM4   SPI_DI             RMII_CLK\n");
    MSG("  6   9   GPIO  PA_TXON     LED1       PWM3   SPI_DO     TS_D5  RMII_TXEN\n");
    MSG("  7   9   GPIO  PA_RXON     LED2       PWM2   SPI_CS     TS_D6  RMII_TXD0\n");
    MSG("  8   9   GPIO                         PWM1   SPI_CK     TS_D7  RMII_TXD1\n");
    MSG("  9   0   GPIO    EPAON                PWM0              TS_D1 RMII_CRSDV\n");
    MSG(" 10   0   GPIO   I2C_SD  PA_PAON            I2S_D_TX     TS_D2  RMII_RXD0\n");
    MSG(" 11   0   GPIO  I2C_SCL  PA_TXON            I2S_D_RX     TS_D3  RMII_RXD1\n");
    MSG(" 12   0   GPIO      LNA  PA_RXON                         TS_D4   RMII_MDC\n");
    MSG(" 13   0   GPIO                   SPDIF                          RMII_MDIO\n");
    MSG(" 14   0   GPIO  PA_PAON          SPDIF      I2S_MCLK     TS_D5           \n");
    MSG(" 15   0   GPIO  PA_TXON UART2_TX            I2S_BCLK     TS_D6           \n");
    MSG(" 16   0   GPIO  PA_RXON UART2_RX             I2S_LRC     TS_D7           \n");
    MSG(" 17   0 SPI_DI           I2C_SD  GPIO                                    \n");
    MSG(" 18   0 SPI_DO          I2C_SCL  GPIO                                    \n");
    MSG(" 19   0 SPI_CS         UART1_TX  GPIO                                    \n");
    MSG(" 20   0 SPI_CK         UART1_RX  GPIO                                    \n");
    MSG(" 21   1   GPIO UART1_TX                    I2S_D_TX                      \n");
    MSG(" 22   1   GPIO UART1_RX         SPDIF      I2S_D_RX                      \n");
    MSG(" 23   0 SF_SD2 UART3_TX  I2C_SD  GPIO                                    \n");
    MSG(" 24   0 SF_SD3 UART3_RX I2C_SCL  GPIO                                    \n");
    MSG(" 25   0 SD_SD0 UART2_TX                                                  \n");
    MSG(" 26   0 SD_SD1 UART2_RX                                                  \n");
    MSG(" 27   0 SD_SD2 UART3_TX                                                  \n");
    MSG(" 28   0 SD_SD3 UART3_RX                                                  \n");
    MSG(" 29   0 SD_CMD   I2C_SD                                                  \n");
    MSG(" 30   0  SD_WP  I2C_SCL                                                  \n");
    MSG(" 31   0  SD_CD     GPIO                                                  \n");
    MSG(" 32   0 SD_CLK     GPIO                                                  \n");
    MSG("SEL DEF      0        1        2     3    4        5 6        7         8\n");
}
#endif
