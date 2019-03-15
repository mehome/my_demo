#ifndef __MT_PMU_H__
#define __MT_PMU_H__

#define PMU_CTRL                 (PMU_BASE + 0x0004)
#define PMU_SLP_PD_CTRL          (PMU_BASE + 0x0008)
#define PMU_STDBY_CTRL           (PMU_BASE + 0x000C)
        #define STDBY_PD_AUDIO_CODEC 0x40000000UL
        #define STDBY_PD_DDR_PHY     0x20000000UL
        #define STDBY_PD_USB_PHY     0x10000000UL
        #define STDBY_PD_EPHY        0x08000000UL
        #define STDBY_PD_MADC        0x04000000UL
        #define STDBY_PD_WIFI_ADC    0x02000000UL
        #define STDBY_PD_WIFI_DAC    0x01000000UL
        #define STDBY_PD_RF          0x00800000UL
        #define STDBY_PD_SDIO_IO     0x00080000UL
        #define STDBY_PD_BUCK_RF     0x00040000UL
        #define STDBY_PD_BUCK_SOC    0x00020000UL
#define PMU_INT_CTRL             (PMU_BASE + 0x0010)
#define PMU_SLP_TMR_CTRL         (PMU_BASE + 0x0014)
#define PMU_WIFI_TMR             (PMU_BASE + 0x0018)
#define PMU_RTC_CLK              (PMU_BASE + 0x001C)
#define PMU_WATCHDOG             (PMU_BASE + 0x0020)

#define PMU_RF_REG_CTRL          (PMU_BASE + 0x00F8)

#define PMU_SOFTWARE_GPREG       (PMU_BASE + 0x00FC)

#define PMU_GPIO_FUNC_0_7        (PMU_BASE + 0x0218)
#define PMU_GPIO_FUNC_8_15       (PMU_BASE + 0x021C)
#define PMU_GPIO_FUNC_16_23      (PMU_BASE + 0x0220)
#define PMU_GPIO_FUNC_24_31      (PMU_BASE + 0x0224)
#define PMU_GPIO_FUNC_32_39      (PMU_BASE + 0x0228)

#define PMU_GPIO_DRIVER_0_15     (PMU_BASE + 0x022C)
#define PMU_GPIO_DRIVER_16_31    (PMU_BASE + 0x0230)
#define PMU_GPIO_DRIVER_32_47    (PMU_BASE + 0x0234)

#define PMU_CLOCK_ENABLE         (PMU_BASE + 0x0258)
    #define PMU_CLOCK_PWM         0x20000000
    #define PMU_CLOCK_PDMA        0x10000000
    #define PMU_CLOCK_UART2       0x08000000
    #define PMU_CLOCK_UART1       0x04000000
    #define PMU_CLOCK_UART0       0x02000000
    #define PMU_CLOCK_SMI         0x01000000
    #define PMU_CLOCK_TIMER       0x00800000
    #define PMU_CLOCK_EJTAG       0x00400000
    #define PMU_CLOCK_GRAPHIC     0x00200000
    #define PMU_CLOCK_DDR         0x00100000
    #define PMU_CLOCK_EPHY        0x00080000
    #define PMU_CLOCK_USBOTG      0x00040000
    #define PMU_CLOCK_USB         0x00020000
    #define PMU_CLOCK_SDIO        0x00010000
    #define PMU_CLOCK_AES         0x00008000
    #define PMU_CLOCK_TSI         0x00004000
    #define PMU_CLOCK_GSPI        0x00002000
    #define PMU_CLOCK_HNAT        0x00001000
    #define PMU_CLOCK_SWP2        0x00000800
    #define PMU_CLOCK_SWP1        0x00000400
    #define PMU_CLOCK_SWP0        0x00000200
    #define PMU_CLOCK_SPDIF       0x00000100
    #define PMU_CLOCK_PCM         0x00000080
    #define PMU_CLOCK_RTC         0x00000040
    #define PMU_CLOCK_BB          0x00000020
    #define PMU_CLOCK_WMAC        0x00000010
    #define PMU_CLOCK_GDMA        0x00000008
    #define PMU_CLOCK_PBUS        0x00000004
    #define PMU_CLOCK_DBUS        0x00000002
    #define PMU_CLOCK_CPU         0x00000001

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
    #define DAC_FS_MASK           0x00003000
    #define DAC_FS_SHIFT          12
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

#define PMU_ADC_DAC_REG_CTRL      (PMU_BASE + 0x025C)
    #define INTERNAL_ADC_EN       0x00000080
    #define INTERNAL_DAC_EN       0x00000100

#define PMU_ADC_REG0_1            (PMU_BASE + 0x048C)
    #define AADC_RESET_MASK       0x00030000
#define PMU_ADC_REG2              (PMU_BASE + 0x0490)
    #define AADC_FS_MASK          0x0000000c
    #define AADC_FS_16k           1

#define PMU_AUDIO_DAC_REG0_REG1   (PMU_BASE + 0x0494)
    #define DAC_POWER_LVL_MASK    0x00000030
    #define DAC_POWER_1_2_V       0x00000000
    #define DAC_POWER_1_3_V       0x00000010
    #define DAC_POWER_1_4_V       0x00000020
    #define DAC_POWER_1_5_V       0x00000030
#define PMU_AUDIO_DAC_REG2        (PMU_BASE + 0x0498)
#define PMU_AUDIO_DAC_REG3        (PMU_BASE + 0x049C)
#define PMU_AUDIO_DAC_REG4        (PMU_BASE + 0x04A0)

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
void pmu_get_gpio_function(int *gpio_ids, unsigned long *gpio_funcs);
void pmu_update_pcm_clk(int clk_sel);
void pmu_update_i2s_clk(int domain_sel, int bypass_en, unsigned long clk_div_sel, unsigned long ndiv_sel);
void pmu_update_spdif_clk(int domain_sel, int bypass_en, unsigned long clk_div_sel, unsigned long ndiv_sel);
void pmu_initial_dac(void);
void pmu_internal_audio_enable(int en);
void pmu_internal_dac_enable(int en);
void pmu_internal_adc_enable(int en);
void pmu_update_dac_clk(int dac_fc);
void pmu_update_adc_clk(int mode_type);
void pmu_internal_adc_clock_en(int en);

void load_powercfg_from_boot_cmdline(void);
void powercfg_apply(void);

#define POWERCTL_STATIC_OFF         1
#define POWERCTL_STATIC_ON          2
#define POWERCTL_DYNAMIC            3
int pmu_get_usb_powercfg(void);
int pmu_get_sdio_powercfg(void);
int pmu_get_ethernet_powercfg(void);
int pmu_get_audio_codec_powercfg(void);
int pmu_get_tsi_powercfg(void);
int pmu_get_wifi_powercfg(void);

#define POWERCTL_FLAG_USB_STATIC         0x01
#define POWERCTL_FLAG_SDIO_STATIC        0x02
#define POWERCTL_FLAG_ETHERNET_STATIC    0x04
#define POWERCTL_FLAG_AUDIO_CODEC_STATIC 0x08
#define POWERCTL_FLAG_TSI_STATIC         0x10
#define POWERCTL_FLAG_WIFI_STATIC        0x20

#define POWERCTL_FLAG_USB_DYNAMIC         0x010000
#define POWERCTL_FLAG_SDIO_DYNAMIC        0x020000
#define POWERCTL_FLAG_ETHERNET_DYNAMIC    0x040000
#define POWERCTL_FLAG_AUDIO_CODEC_DYNAMIC 0x080000
#define POWERCTL_FLAG_TSI_DYNAMIC         0x100000
#define POWERCTL_FLAG_WIFI_DYNAMIC        0x200000

#endif

