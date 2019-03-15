#ifndef __MT_PMU_H__
#define __MT_PMU_H__

//#define PMU_CTRL                 (PMU_BASE + 0x0004)
//#define PMU_SLP_PD_CTRL          (PMU_BASE + 0x0008)
//#define PMU_STDBY_CTRL           (PMU_BASE + 0x000C)
//#define PMU_INT_CTRL             (PMU_BASE + 0x0010)
//#define PMU_SLP_TMR_CTRL         (PMU_BASE + 0x0014)
//#define PMU_WIFI_TMR             (PMU_BASE + 0x0018)
//#define PMU_RTC_CLK              (PMU_BASE + 0x001C)
//#define PMU_WATCHDOG             (PMU_BASE + 0x0020)

#define PMU_SOFTWARE_GPREG       (PMU_BASE + 0x00FC)

#define PMU_GPIO_FUNC_0_7        (PMU_BASE + 0x0218)
#define PMU_GPIO_FUNC_8_15       (PMU_BASE + 0x021C)
#define PMU_GPIO_FUNC_16_23      (PMU_BASE + 0x0220)
#define PMU_GPIO_FUNC_24_31      (PMU_BASE + 0x0224)
#define PMU_GPIO_FUNC_32_39      (PMU_BASE + 0x0228)

#define PMU_GPIO_DRIVER_0_15     (PMU_BASE + 0x022C)
#define PMU_GPIO_DRIVER_16_31    (PMU_BASE + 0x0230)
#define PMU_GPIO_DRIVER_32_47    (PMU_BASE + 0x0234)

#define PMU_CLOCK_SELECT         (PMU_BASE + 0x025C)
    #define TSI_BB_SEL            0x00000020

#define PMU_RESET_REG24          (PMU_BASE + 0x0260)
#define PMU_RESET_REG25          (PMU_BASE + 0x0264)

#define PMU_CPLL_XDIV_REG_0_31   (PMU_BASE + 0x047C)
    #define PCM_CLK_8192          1
    #define PCM_CLK_8k            0
    #define PCM_CLK_SEL           0x20000000
    #define I2S_NDIV_SEL_MASK     0x000F8000
    #define I2S_NDIV_SEL_SHIFT    15
    #define SPDIF_NDIV_SEL_MASK   0x01F00000
    #define SPDIF_NDIV_SEL_SHIFT  20
#define PMU_CPLL_XDIV_REG_32_47  (PMU_BASE + 0x0480)
    #define I2S_DOMAIN_SEL        0x00000008
    #define SPDIF_DOMAIN_SEL      0x00000008
#define PMU_CPLL_XDIV_REG2_0_31  (PMU_BASE + 0x0484)
    #define I2S_BYPASS_EN         0x01000000
    #define I2S_CLK_SEL_MASK      0x00030000
    #define I2S_CLK_SEL_SHIFT     16
    #define SPDIF_BYPASS_EN       0x02000000
    #define SPDIF_CLK_SEL_MASK    0x00180000
    #define SPDIF_CLK_SEL_SHIFT   19

#define DEVICE_ID_GPIO          2431
#define DEVICE_ID_PWM           2430
#define DEVICE_ID_OTP           2429
#define DEVICE_ID_PDMA          2428
#define DEVICE_ID_UART2         2427
#define DEVICE_ID_UART1         2426
#define DEVICE_ID_UART0         2425
#define DEVICE_ID_SMI           2424
#define DEVICE_ID_TIMER         2423
#define DEVICE_ID_DISPLAY       2422
#define DEVICE_ID_GRAPHIC       2421
#define DEVICE_ID_DDR           2420
#define DEVICE_ID_USBOTG        2419
#define DEVICE_ID_USB           2418
#define DEVICE_ID_SDIO          2417
#define DEVICE_ID_AES           2416
#define DEVICE_ID_TSI           2415
#define DEVICE_ID_GSPI          2414
#define DEVICE_ID_HNAT          2413
#define DEVICE_ID_SWP2          2412
#define DEVICE_ID_SWP1          2411
#define DEVICE_ID_SWP0          2410
#define DEVICE_ID_SPDIF         2409
#define DEVICE_ID_PCM           2408
#define DEVICE_ID_RTC           2407
#define DEVICE_ID_BBP           2406
#define DEVICE_ID_GDMA          2405
#define DEVICE_ID_SRAM_CTRL     2404
#define DEVICE_ID_PBUS          2403
#define DEVICE_ID_RBUS          2402
#define DEVICE_ID_DBUS          2401
#define DEVICE_ID_CPU           2400
#define DEVICE_ID_WIFIMAC       2531
#define DEVICE_ID_SWP1_PORT     2530
#define DEVICE_ID_SWP0_PORT     2529

void pmu_reset_devices(unsigned long *device_ids);
void pmu_reset_devices_no_spinlock(unsigned long *device_ids);
void pmu_system_restart(void);
void pmu_system_halt(void);
void pmu_set_gpio_driving_strengh(int *gpio_ids, unsigned long *gpio_vals);
void pmu_set_gpio_input_enable(int *gpio_ids, unsigned long *gpio_vals);
void pmu_set_gpio_function(int *gpio_ids, unsigned long *gpio_funcs);
void pmu_update_pcm_clk(int clk_sel);
void pmu_update_i2s_clk(int domain_sel, int bypass_en, unsigned long clk_div_sel, unsigned long ndiv_sel);
void pmu_update_spdif_clk(int domain_sel, int bypass_en, unsigned long clk_div_sel, unsigned long ndiv_sel);

#endif

