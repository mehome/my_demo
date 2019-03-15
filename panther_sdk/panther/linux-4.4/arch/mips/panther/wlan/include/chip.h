/*=============================================================================+
|                                                                              |
| Copyright 2014                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
/*!
*   \file chip.h
*   \brief Registers definition
*   \author Montage
*/

#ifndef CHIP_H
#define CHIP_H

#define SLP_SIGN		0x534C504D

/* interrupt number */
/*
#define IRQ_AXI			0
#define IRQ_UR      	2
#define IRQ_WD    		3
#define IRQ_TMR1    	4
#define IRQ_TMR0		5
#define IRQ_UR2			6
#define IRQ_GPIO		7
#define IRQ_PMU			8
#define IRQ_CDMA_CH0	9
#define IRQ_CDMA_CH1	10
#define IRQ_CDMA_CH2	11
#define IRQ_CDMA_CH3	12
#define IRQ_WIFI    	13
#define IRQ_UDC     	16
#define IRQ_SDIO		17
#define IRQ_MAX     	18
*/
/* register base */
// Lynx base
//#define PLL_BASE	0x000C0A00
//#define RF_BASE		0x000C0B00
//#define TM_BASE		0x000C2000
// Panther base
#define PLL_BASE	0x000C0A00
#define RF_BASE		0xBF004F00
#define TM_BASE		0xBF010000

#define REG_BASE        0xBF000000UL
#define MI_BASE         REG_BASE
#define BBP_REG_BASE    REG_BASE + 0x4000UL
#define UMAC_REG_BASE     REG_BASE + 0x3000UL
#define PMU_REG_BASE    REG_BASE + 0x4800UL
#define GPIO_BASE       REG_BASE + 0x4A00UL
#define GPREG(x)    (*(volatile u32*)(GPIO_BASE+(x)))
#define BBREG_READ32(x) (*(volatile u32*)(BBP_REG_BASE+(x)))
#define BBREG_WRITE32(x, val) (*(volatile u32*)(BBP_REG_BASE+(x)) = (u32)(val))
#define BBREG_UPDATE32(x,val,mask) do {           \
    unsigned long newval = BBREG(x);     \
    newval = (( newval & ~(mask) ) | ( (val) & (mask) ));\
    BBREG(x) = newval;      \
} while(0)
#define PMUREG_READ32(x)        (*(volatile u32*)(PMU_REG_BASE+(x)))
#define PMUREG_WRITE32(x, val)  (*(volatile u32*)(PMU_REG_BASE+(x)) = (u32)(val))
#define REG_UPDATE32(x,val,mask) do {                     \
        u32 newval;                                           \
        newval = *(volatile u32*) ((x));                      \
        newval = (( newval & ~(mask) ) | ( (val) & (mask) )); \
        *(volatile u32*)((x)) = newval;                       \
} while(0)
#define REG_READ32(x)   (*(volatile u32*)(REG_BASE+(x)))
#define PMUREG(x)       (*(volatile u32*)(PMU_REG_BASE+(x)))
#define PMUREG_UPDATE32(x,val,mask) do {           \
    unsigned long newval = PMUREG(x);     \
    newval = (( newval & ~(mask) ) | ( (val) & (mask) ));\
    PMUREG(x) = newval;      \
} while(0)
#define WMREG(x)		(*(volatile u32*)(UMAC_REG_BASE+(x)))
#define BBREG(x)        (*(volatile u32*)(BBP_REG_BASE+(x)))
/* macro for accessing related register */
#define REG(base,offset)    (((volatile unsigned int *)base)[offset >> 2])
#define PLLREG(offset)		REG(PLL_BASE,offset)
#define RFREG(offset)		REG(RF_BASE,offset)
#define TMREG(offset)  		REG(TM_BASE,offset)

// for otp
#define OTP_REG_READ32(offset)         REG(MI_BASE, offset)
#define OTP_REG_WRITE32(offset, val)   REG(MI_BASE, offset) = (unsigned int)(val)
#define OTP_REG_UPDATE32(x, val, mask) do {                   \
    unsigned int newval;                                  \
    newval = *(volatile unsigned int*) (MI_BASE + (x));   \
    newval = (( newval & ~(mask) ) | ( (val) & (mask) )); \
    *(volatile unsigned int*)(MI_BASE + (x)) = newval;    \
} while(0)

/* OTP configuration */
#define OTP_CONFIG_BASE 32   // bytes
#define OTP_CONFIG_LEN 3     // bytes
#define OTP_ENABLE_SECURE_SHIFT        0
#define OTP_DISABLE_KEY_WRITE_SHIFT    1
#define OTP_KEY_ENCRYPT_SHIFT          2
#define OTP_ENABLE_FLASH_SHIFT         3
#define OTP_FLASH_BOOT_SELECT_SHIFT    4
#define OTP_UART_SD_BOOT_SEL_SHIFT     5
#define OTP_WATCHDOG_SHIFT             6
#define OTP_JTAG_SHIFT                 7
#define OTP_READ_CMD_DUMMY_SHIFT       8
#define OTP_READ_DATA_DUMMY_SHIFT      9
#define OTP_PAGE_SIZE_SHIFT           10
#define OTP_CHECK_BAD_BLOCK_SHIFT     12
#define OTP_UART_SD_BOOT_DISABLE_SHIFT 13
#define OTP_SD_CLK_INV_SHIFT          14
#define OTP_BOOT1_IMAGE_CHECK_SHIFT   15
#define OTP_DISABLE_BAD_BLOCK_CHECK_SHIFT  16
#define OTP_SD_CLK_DIV_SHIFT               17
#define OTP_FLASH_CLK_DIV_SHIFT            20
#define OTP_ONE_SECOND_WATCHDOG_SHIFT      23


#if 1
/* ROM&SDRAM bank register */
#define SROMEN      0x08
#define  DISABLE_DMAP    (1<<3)
#define  CS2_PFLASH      (1<<2)
#define  CS1_PFLASH      (1<<1)
#endif

/* timer bank register */
#define T0CR        0x00    /* current value */
#define T0LR        0x04    /* reload value */
#define T0PR        0x08    /* prescaler */
#define T0CN        0x0c    /* control */
#define T0IS        0x10    /* interrupt status */
#define T1CR        0x14    /* current value */
#define T1LR        0x18    /* reload value */
#define T1PR        0x1c    /* prescaler */
#define T1CN        0x20    /* control */
#define T1IS        0x24    /* interrupt status */
#define T2CR        0x28    /* current value */
#define T2LR        0x2c    /* reload value */
#define T2PR        0x30    /* prescaler */
#define T2CN        0x34    /* control */
#define T2IS        0x38    /* interrupt status */
#define T0HR        0x3C    /* hardware reload value */

/* UART bank register */
#define URBR        0x0         /* tx/rx buffer register */
#define URCS        0x4         /* control/status */
#define  URCS_RXEN   (1<<31)
#define  URCS_TIE    (1<<15)
#define  URCS_RIE    (1<<14)
#define  URCS_PE     (1<<13)
#define  URCS_EVEN   (1<<12)
#define  URCS_SP2    (1<<11)
#define  URCS_LB     (1<<10)
#define  URCS_P      (1<<9)
#define  URCS_PER    (1<<8)
#define  URCS_FE     (1<<7)
/* b4~b6: rx threshold */
#define	 URCS_TF     (1<<3)      /* tx fifo full */
#define  URCS_TE     (1<<2)      /* tx empty */
#define  URCS_RF     (1<<1)      /* rx full */
#define  URCS_TB     (1<<0)      /* tx busy */

/* CPU DMA bank register */
#define DMA_CMD     0x0
#define  DMA_START   (1<<16)
#define  DMA_LEN     0xFFFF
#define DMA_SRC     0x4
#define  DMA_SADDR   0xFFFFFF
#define DMA_DST     0x8
#define  DMA_DADDR   0xFFFFF
#define DMA_CH_OFF	0x10
#define DMA_ST_IM   0x3C         /* interrupt status and mask */
#define  DMA_F_IM    (0xF<<12)   /* fail interrupt mask */
#define  DMA_D_IM    (0xF<<8)    /* done interrupt mask */
#define  DMA_F_ST    (0xF<<4)    /* fail status */
#define  DMA_D_ST    (0xF)       /* done status */
#define DMA_BOOST    0x40
#define  SRC_FLASH_DMA_SEL      (3<<7)  /* source from flash DMA channel selector */ 
#define  SRC_FROM_FLASH         (1<<6)  /* DMA CH0 source from flash */
#define  SM2_DMA_BOOST          (3<<4)
#define  SM1_DMA_BOOST          (3<<2)
#define  LM_DMA_BOOST           (3)

/* PMU bank register */
#define GPIO_FUN_EN			0x04
#define GPIO_ODS			0x08
#define GPIO_ODC			0x0C
#define GPIO_OEN			0x10
#define GPIO_ID				0x14
#define GPIO_OD				0x18
#define GPIO_COMMON_CTRL	0x1C
	#define GPIO_TMR_SEL		0x00000037
	#define SDIO_RGL_CTRL		0x00000001
#define GPIO_IE_CTRL		0x20
#define GPIO_DRV_BIT0		0x24
#define GPIO_DRV_BIT1		0x28
#define GPIO_SMTEN_CTRL		0x2C
#define GPIO_PULL_UP_CTRL	0x30
#define PKG_MODE_CTRL		0x34
	#define QFN64_EN			0x00800000
	#define TEST_REG			0x00700000
	#define UART2_EN1			0x00008000
	#define UART2_EN0			0x00004000
	#define FAST_SIM			0x00002000
	#define I2C_EN				0x00000800
	#define JTAG_EN				0x00000400
	#define FLASH_HW_LOAD_NE	0x00000100
	#define ATE_MODE			0x000000E0
	#define TEST_EN				0x00000080
	#define STATION_MODE		0x00000010
	#define HOST_IF				0x00000008
	#define PKG_ST				0x00000003
	#define NON_USB				0x00000001
#define PKG_IO				0x38
	#define PKG1_REN			0x00000008
	#define PKG1_IE				0x00000004
	#define PKG0_REN			0x00000002
	#define PKG0_IE				0x00000001
//#define PMU_CTRL			0x50
//	#define SDIO_FUN_OFF		0x00000080
//	#define USB_FUN_PWR_OFF		0x00000040
//	#define SLP_ON				0x00000020
//	#define PMU_SM				0x0000001F
#define SLP_PD_CTRL			0x54
	#define SLP_QMEM0_PD		0x00020000
	#define SLP_QMEM1_PD		0x00010000
	#define SLP_QMEM2_PD		0x00008000
	#define SLP_SDIO_IO_PD		0x00002000
	#define SLP_RF_PD			0x00001000
	#define SLP_MADC_PD			0x00000800
	#define SLP_WDAC_PD			0x00000400
	#define SLP_WADC_PD			0x00000200
	#define SLP_PLL_PD			0x00000100
	#define SLP_XTAL_PD			0x00000080
	#define SLP_AON_OSC_PD		0x00000040
	#define SLP_USB_PD			0x00000020
	#define SLP_SDIO_PD			0x00000010
	#define SLP_USB_PHY_PD		0x00000008
	//#define SLP_SW_BUCK_1P2V	0x00000004
	#define SLP_AON_LDO_PD		0x00000002
	#define SLP_BUCK_PD			0x00000001
#define SLP_CLK_CTRL		0x58
	#define SLP_SDIO_CLK			0x08000000
	#define SLP_USE_AON_OSC			0x04000000
	#define SLP_40M_CLK				0x02000000
	#define SLP_USB_CLK				0x01000000
	#define SLP_SPI_DAC_CLK			0x00800000
	#define SLP_SPI_ADC_CLK			0x00400000
	#define SLP_BBP_11N				0x00200000
	#define SLP_WMAC_CLK			0x00100000
	#define SLP_WSEC_CLK			0x00080000
	#define SLP_PLL_160M_CLK		0x00040000
	#define SLP_AXI_CLK				0x00020000
	#define SLP_CPU_CLK				0x00010000
	#define SLP_SDIO_RSTN			0x00000800
	#define SLP_USB_RSTN			0x00000400
	#define SLP_BBP11N_RSTN			0x00000200
	#define SLP_WMAC_RSTN			0x00000100
	//#define SLP_AXI_SRAM_RSTN		0x00000040
	//#define SLP_AXI_MATRIX_RSTN		0x00000020
	#define SLP_QMEM0_RSTN			0x00000010
	#define SLP_QMEM1_RSTN			0x00000008
	#define SLP_QMEM2_RSTN			0x00000004
	#define SLP_CPU_MEM_RSTN		0x00000002
	//#define SLP_CDMA_RSTN			0x00000001
#define STDBY_PD_CTRL		0x5C
	//#define STDBY_QMEM0_PD			0x00020000
	//#define STDBY_QMEM1_PD			0x00010000
	//#define STDBY_QMEM2_PD			0x00008000
	//#define STDBY_RF_RX_PD			0x00004000
	//#define STDBY_SDIO_RGL2			0x00002000
	#define STDBY_RF_PD				0x00001000
	#define STDBY_MADC_PD			0x00000800
	#define STDBY_WDAC_PD			0x00000400
	#define STDBY_WADC_PD			0x00000200
	#define STDBY_PLL_PD			0x00000100
	#define STDBY_XTAL_PD			0x00000080
	#define STDBY_AON_OSC_PD		0x00000040
	//#define STDBY_USB_PD			0x00000020
	//#define STDBY_SDIO_PD			0x00000010
	#define STDBY_USB_PHY_PD		0x00000008
	//#define STDBY_SW_BUCK_1P2V		0x00000004
	#define BOOT_SLEEPING			0x00000004
	#define STDBY_AON_LDO_PD		0x00000002
	//#define STDBY_BUCK_PD			0x00000001
#define STDBY_CLK_CTRL		0x60
	#define STDBY_SDIO_CLK			0x08000000
	#define STDBY_USE_AON_OSC		0x04000000
	#define STDBY_40M_CLK			0x02000000
	#define STDBY_USB_CLK			0x01000000
	#define STDBY_SPI_DAC_CLK		0x00800000
	#define STDBY_SPI_ADC_CLK		0x00400000
	#define STDBY_BBP_11N			0x00200000
	#define STDBY_WMAC_CLK			0x00100000
	#define STDBY_WSEC_CLK			0x00080000
	#define STDBY_PLL_160M_CLK		0x00040000
	#define STDBY_AXI_CLK			0x00020000
	#define STDBY_CPU_CLK			0x00010000
	#define STDBY_SDIO_RSTN			0x00000800
	#define STDBY_USB_RSTN			0x00000400
	#define STDBY_BBP11N_RSTN		0x00000200
	#define STDBY_WMAC_RSTN			0x00000100
	//#define STDBY_AXI_SRAM_RSTN		0x00000040
	//#define STDBY_AXI_MATRIX_RSTN	0x00000020
	#define STDBY_QMEM0_RSTN		0x00000010
	#define STDBY_QMEM1_RSTN		0x00000008
	#define STDBY_QMEM2_RSTN		0x00000004
	#define STDBY_CPU_MEM_RSTN		0x00000002
	//#define STDBY_CDMA_RSTN			0x00000001
#define PMU_INT_REG			0x64
	#define SDIO_MEM_RW_WAKE_ST		0x00004000
	#define SDIO_MEM_RW_WAKE_DIS	0x00002000
	#define SDIO_PWR_WAKE_ST		0x00001000
	#define SDIO_PWR_WAKE_DIS		0x00000800
	#define USB_WAKE_ST				0x00000400
	#define USB_WAKE_INT_DIS		0x00000200
	#define SDIO_CLK_WAKE_ST		0x00000100
	#define SDIO_CLK_WAKE_DIS		0x00000080
	#define USB_PHY_WAKE_ST			0x00000020
	#define SDIO_WAKE_ST			0x00000010
	#define SLP_TMR_WAKE_ST			0x00000008
	#define USB_PHY_WAKE_INT_DIS	0x00000004
	#define SDIO_WAKE_INT_DIS		0x00000002
	#define SLP_TMR_WAKE_INT_DIS	0x00000001
#define SLP_TMR_CTRL		0x68
	#define SLP_TMR_EN				0x01000000
	#define SLP_TMR					0x00FFFFFF
#define RTC_TMR_CLK			0x6C
	#define RTC_CLK_CLR				0x04000000
	#define RTC_CLK_HALT			0x02000000
	#define RTC_EN					0x01000000
	#define RTC_CLK					0x00FFFFFF
#define SLP_WDOG_TMR_CTRL	0x70
	#define SLP_WDOG_TMR_INT		0x02000000
	#define SLP_WDOG_TMR_EN			0x01000000
	#define SLP_WDOG_TMR			0x00FFFFFF
#define RTL_SVN_REG			0x74
#define SLP_ISO_CTRL		0x78
	#define SLP_RETAIN_SDIO_IO		0x00400000
	#define SLP_RETAIN_SDIO_CTRL	0x00200000
	#define SLP_RETAIN_USB			0x00100000
	#define SLP_RETAIN_QMEM2		0x00080000
	#define SLP_RETAIN_QMEM1		0x00040000
	#define SLP_RETAIN_QMEM0		0x00020000
	#define SLP_RETAIN_BUCK			0x00010000
	#define SLP_ISO_SDIO_IO			0x00000040
	#define SLP_ISO_SDIO_CTRL		0x00000020
	#define SLP_ISO_USB				0x00000010
	#define SLP_ISO_QMEM2			0x00000008
	#define SLP_ISO_QMEM1			0x00000004
	#define SLP_ISO_QMEM0			0x00000002
	#define SLP_ISO_BUCK			0x00000001
#define STDBY_ISO_CTRL		0x7C
	#define STDBY_RETAIN_SDIO_IO	0x00400000
	#define STDBY_RETAIN_SDIO_CTRL	0x00200000
	#define STDBY_RETAIN_USB		0x00100000
	#define STDBY_RETAIN_QMEM2		0x00080000
	#define STDBY_RETAIN_QMEM1		0x00040000
	#define STDBY_RETAIN_QMEM0		0x00020000
	#define STDBY_RETAIN_BUCK		0x00010000
	#define STDBY_ISO_SDIO_IO		0x00000040
	#define STDBY_ISO_SDIO_CTRL		0x00000020
	#define STDBY_ISO_USB			0x00000010
	#define STDBY_ISO_QMEM2			0x00000008
	#define STDBY_ISO_QMEM1			0x00000004
	#define STDBY_ISO_QMEM0			0x00000002
	#define STDBY_ISO_BUCK			0x00000001
#define GPIO_INT_RISING		0x80
#define GPIO_INT_FALLING	0x84
#define GPIO_INT_HIGH		0x88
#define GPIO_INT_LOW		0x8C
#define GPIO_INT_MASK		0x90
#define GPIO_INT_STS		0x94
#define SDIO_CTRL			0x98
	#define SDIO_1P8_CTRL			0x00100000
	#define SDIO_1P8				0x00080000
	#define SDIO_PWR_DET_EN			0x00040000
	#define SDIO_PDN				0x0003F000
	#define SDIO_CURRENT_SEL		0x00000FC0
	#define SDIO_DRV_BIT2			0x0000003F
#define DBG_CTRL			0x9C
#define AXI_MASTER_REQ_DLY	0xA0
#define PWRUP_WAIT_XTAL		0xA4
#define PWRUP_WAIT_BUCK		0xA8
#define PWRUP_WAIT_PLL		0xAC
#define WAKEUP_WAIT_XTAL	0xB0
#define WAKEUP_WAIT_BUCK	0xB4
#define WAKEUP_WAIT_PLL		0xB8
#define SLP_CLK2ISO_DLY		0xBC
#define SLP_ISO2PDN_DLY		0xC0
#define WAKEUP_PDN2ISO_DLY	0xC4
#define RF_REG_CTRL			0xC8
#define RF_WT_RCOV_TIME		0xCC
#define RF_CTRL_REG			0xD0

/* PLL register */
#define PLL_CTRL1			0x00
	#define PLL_FBDIV				0x0001F200
	#define PLL_ICP					0x000001C0
	#define PLL_PFD_DLY				0x00000020
	#define PLL_REF_DIV				0x0000001F
#define PLL_CTRL2			0x04
	#define PLL_PD					0x00004000
	#define PLL_RST_SOFT			0x00002000
	#define PLL_CAL_EN				0x00001000
	#define PLL_CAL_EXT				0x00000C00
	#define PLL_CAL_TIME			0x00000300
	#define PLL_CAL_DIV				0x000000F0
	#define PLL_CPVR_SEL			0x0000000C
	#define PLL_VCO_SEL				0x00000003
#define CLKGEN_RSVD			0x08
#define CLKGEN_CTRL			0x0C
	#define CLKGEN_ATP_EN			0x00000080
	#define CLKGEN_ATP_SEL			0x00000060
	#define CLKGEN_TEST_DIV			0x00000018
	#define CLKGEN_TEST_SEL			0x00000007
#define ADC_CTRL			0x10
	#define MADC_CLK_EN				0x00020000
	#define MADCCLK_NOGATE			0x00010000
	#define MADCCLK_UPDATE			0x00008000
	#define MADC_PREDIV				0x00007F80
	#define MADC_POSTDIV			0x00000070
	#define DAC_DIVSEL_80M			0x00000008
	#define ADC_DIVSEL_240M			0x00000004
	#define ADC_CLK_EN				0x00000002
	#define DAC_CLK_EN				0x00000001
#define CPU_CLK_CTRL		0x14
	#define CPUCLK_EN				0x00002000
	#define CPUCLK_NOGATE			0x00001000
	#define CPUCLK_UPDATE			0x00000800
	#define CPUCLK_PREDIV			0x000007F8
	#define CPUCLK_POSTDIV			0x00000003
#define AXI_CLK_CTRL		0x18
	#define AXICLK_EN				0x00002000
	#define AXICLK_NOGATE			0x00001000
	#define AXICLK_UPDATE			0x00000800
	#define AXICLK_PREDIV			0x000007F8
	#define AXICLK_POSTDIV			0x00000003
#define WIFI_CLK_CTRL		0x1C
	#define WDAC_SLP_CTRL			0x80000000
	#define WDAC_SLP				0x40000000
	#define WADC_SLP_CTRL			0x20000000
	#define WADC_SLP				0x10000000
	#define SECCLK_NOGATE			0x00004000
	#define SECCLK_UPDATE			0x00002000
	#define SECCLK_PREDIV			0x00001FE0
	#define SECCLK_POSDIV			0x0000001C
	#define WIFICLK_EN				0x00000002
	#define SECCLK_EN				0x00000001
#define WIFI_DAC_SETTING	0x20
#define WIFI_ADC_SETTING	0x24
#define ANA_TEST_CTRL		0x30
#define ANA_ADC_CTRL		0x34
#define ANA_DAC_CTRL		0x38

/* Sflash register */
#define SF_CR				0x00			/* Control register */
	#define SF_CMD					0xFF000000
	#define SF_HIGH					0x00F00000
	#define SF_LOW					0x000F0000
	#define SF_NONACT				0x00001F00
	#define SF_RD_TRIG				0x00000080
	#define SF_WR_TRIG				0x00000040
	#define SF_DUMMY_CNT			0x00000030
	#define SF_ADDR_NEED			0x00000008
	#define SF_DATA_CNT				0x00000003
#define SF_AR				0x04			/* Address register */
#define SF_WD				0x08			/* Write data register */
#define SF_RD				0x0C			/* Read data register */

/* MI bank register */
#define OTP_CTRL			0x00
	#define OTP_CLK_PULSE_WIDTH		0xFFE00000
	#define OTP_CLK_PULSE			0x00100000
	#define OTP_RDEN				0x00080000
	#define OTP_PGMEM				0x00040000
	#define OTP_A					0x0003FF00
	#define OTP_D					0x000000FF
#define OTP_CTRL1			0x04
	#define OTP_AVDD_EN				0x00000001
#define PWM1_CTRL			0x10
	#define PWM_PRESCALAR			0x03FC0000
	#define PWM_CH1_POLARITY		0x00200000
	#define PWM_CH0_POLARITY		0x00100000
	#define PWM_CH1_FREQ			0x000E0000
	#define PWM_CH0_FREQ			0x0001C000
	#define PWM_CH1_DUTY			0x00003E00
	#define PWM_CH0_DUTY			0x000001F0			
	#define PWM_CH1_AUTO			0x00000008
	#define PWM_CH0_AUTO			0x00000004
	#define PWM_CH1_EN				0x00000002
	#define PWM_CH0_EN				0x00000001
#define PWM2_CTRL			0x14

/* MADC register */
#define MADC_CTRL0			0x00
#define MADC_CTRL1			0x01
#define MADC_DATA			0x08

/* USB bank register */
#define OTGSC          0x0a4
#define  AVV              (1<<9)
#define  AVVIS            (1<<17)
#define  AVVIE            (1<<25)
#define PHY_DIG_CTRL   0x308
#define  VBUS_DETECTION   (1<<12)
#define  RECOVERY_CLK_INV (1<<4)
#define USB_REG0       0x320
#define  SQ_DELAY         (1<<31)
#define USB_PHY_PLL    0x328
#define USB_A0_REG     0x32c
#define  USB_DOWN         (1<<0)

#define END_OF_QMEM			CONFIG_DATABUF_START
#define DATA_BUFFER_MEM		CONFIG_DATABUF_START
#define DATA_BUFFER_END		0x60000
#define DATA_BUFFER_SIZE	(DATA_BUFFER_END-CONFIG_DATABUF_START)

#define ROM_START			0x68000

#endif
