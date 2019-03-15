
#ifndef _PURIN_I2S_H
#define _PURIN_I2S_H

#include <asm/mach-panther/reg.h>

#define BIT0    0x00000001UL
#define BIT1    0x00000002UL
#define BIT2    0x00000004UL
#define BIT3    0x00000008UL
#define BIT4    0x00000010UL
#define BIT5    0x00000020UL
#define BIT6    0x00000040UL
#define BIT7    0x00000080UL
#define BIT8    0x00000100UL
#define BIT9    0x00000200UL
#define BIT10   0x00000400UL
#define BIT11   0x00000800UL
#define BIT12   0x00001000UL
#define BIT13   0x00002000UL
#define BIT14   0x00004000UL
#define BIT15   0x00008000UL
#define BIT16   0x00010000UL
#define BIT17   0x00020000UL
#define BIT18   0x00040000UL
#define BIT19   0x00080000UL
#define BIT20   0x00100000UL
#define BIT21   0x00200000UL
#define BIT22   0x00400000UL
#define BIT23   0x00800000UL
#define BIT24   0x01000000UL
#define BIT25   0x02000000UL
#define BIT26   0x04000000UL
#define BIT27   0x08000000UL
#define BIT28   0x10000000UL
#define BIT29   0x20000000UL
#define BIT30   0x40000000UL
#define BIT31   0x80000000UL

/* I2S clock */
#define PURIN_I2S_SYSCLK      0

static inline u32 AIREG_READ32(u32 x)
{
    u32 val = (*(volatile u32*)(AI_BASE+(x)));

//  printk(KERN_DEBUG "R %x %x\n", x, val);
    return val;
}

static inline void AIREG_WRITE32(u32 x, u32 val)
{
    if(x != 0x28 && x != 0x30 && x != 0x20)
    {
        printk(KERN_DEBUG "W %x %x\n", x, val);
    }
    (*(volatile u32*)(AI_BASE+(x)) = (u32)(val));
}

static inline void AIREG_UPDATE32(u32 x, u32 val, u32 mask)
{
    u32 newval;

    if(x != 0x28 && x != 0x30 && x != 0x20)
    {
        printk(KERN_DEBUG "U %x %x\n", x, val);
    }

    newval = *(volatile u32*) (AI_BASE+(x));
    newval = (( newval & ~(mask) ) | ( (val) & (mask) ));
    *(volatile u32*)(AI_BASE+(x)) = newval;
}

#define CFGREG(tmp, val, mask) \
    do { tmp = ((tmp&~mask)|val); } while(0)

#define PMUENABLEREG   (*(volatile unsigned int*)(0xBF004A58))
#define PMURESETREG    (*(volatile unsigned int*)(0xBF004A60))

#define CFG             (0x00)
#define AI_TXBASE       (0x04)
#define AI_RXBASE       (0x08)
#define ADC_RXBASE      (0x0c)
    #define DES_OWN         (1<<31)      //Owner bit 1:SW 0:HW
    #define DES_EOR         (1<<30)      //End of Ring
    #define DES_SKP         (1<<29)      //Skip compensated data
    #define DES_HWD         (1<<28)      //Hw haneld
    #define BUF_SIZE_SHIFT  16           //buffer size shift
    #define LENG_MASK       (0x07ff0000) //Mask for buffer length
    #define CNT_MASK        (0x0000ffff) //Mask for UNRUN_COUNT/OVRUN_COUNT
#define CONF_FSSYNC     (0x10)
#define CH0_CONF        (0x14)
#define CH1_CONF        (0x18)
    #define CH_BUSMODE_8    (0)
    #define CH_BUSMODE_16   (BIT12)
    #define CH_BUSMODE_WB   (BIT13)
    #define CH_BUSMODE_24   (BIT12|BIT13)
    #define CH_BUSMODE_MASK (BIT12|BIT13)
    #define CH_MSB_FIRST    (BIT10)
    #define CH_SLOT_ID_MSK  (0x000003F8)  // BIT3~9
    #define CH_RXDMA_EN     (BIT2)
    #define CH_TXDMA_EN     (BIT1)
    #define CH_EN           (BIT0)
    #define CH_DISABLE      0
    #define CH_DEFAULT      0x00000400UL
#define TX_DUMMY        (0x1c)
#define NOTIFY          (0x20)
#define STATE_MACH      (0x24)
#define INTR_STATUS     (0x28)
#define INTR_MASK       (0x2C)
#define INTR_CLR        (0x30)
#define SPDIF_CONF      (0x34)
#define SPDIF_CH_STA    (0x38)
#define SPDIF_CH_STA_1  (0x3C)
#define AUDIO_DMA_ASYNC (0x40)
#define DAC_CONF        (0x44)
#define ADC_CONF        (0x48)

#define PCMWB_SEL          (MI_BASE + 0x7014)  // bit[18:16]
    #define PCMWB_SEL_MSK  (BIT16|BIT17|BIT18)
#define PCMWB_CH4_5_SLOT   (MI_BASE + 0x4828)  // ch4 -> bit[24:18], ch5 -> bit[31:25]
    #define PCMWB_CH4_SLOT_MSK  0x000007F0
    #define PCMWB_CH5_SLOT_MSK  0x0003F800

#define NOTIFY_TX_BIT(ch) (ch * 16)     //0*16 or 1*16
#define NOTIFY_RX_BIT(ch) (ch * 16 + 1) //0*16+1 or 1*16+1

#define IF_PCM      0
#define IF_I2S      1
#define IF_PCMWB    2

#define NUM_DIR         3            // CH_TX, CH_RX
    #define CH_TX       0
    #define CH_RX       1
    #define CH_RX_ADC   2

#define INTR_STATUS_MASK           0x000F
    #define INTR_STATUS_RX         BIT0
    #define INTR_STATUS_TX         BIT1
    #define INTR_STATUS_ADC        BIT2
    #define INTR_STATUS_UNDERRUN   BIT3

/* possible sample rate in purin_i2s.c
  8000, 16000, 22050, *24000, 32000, 44100, 48000, *88200, 96000, 128000, *176400, *192000, 384000
  "*" means not supported for wm8750
  24000 & 88200 is not used but reserved
*/
#define SAMPLE_RATE_SUPPORT (13)

struct purin_device {
//    int if_mode;        // 0:PCM 1:I2S
/* common register */
    int dma_mode;
    int ctrl_mode;      // 0:Master 1:Slave
/* PCM register */
    int fs_polar;       // 0:Low 1:High Active
    int fs_long_en;     // Long Frame sync enable
    int fs_interval;    // frame sync interval
    int fs_rate;        // 0:8k  1:16k  2:32k in PCM, 1:support -1:not support in I2S/SPDIF
    int slot_mode;      // 0:256KHz 1:512KHz 2:1024KHz 3:2048kHz 4:4096kHz 5:8192kHz
    int pcm_mclk;       // 0~7 perform different clock (256kHz ~ 24.576MHz)
    int rx_bclk_latch;  // 0:Negative 1:Positive edge
/* I2S register */
    int swapch;         // 0:Left->Right 1:Right->Left
    int txd_align;      // 0:failling edge 1:rising edge
/* operation information */
    int bit_depth;      // bit depth
};

#define PCM_IS_CONFIGURED   (INTR_STATUS_MASK != AIREG_READ32(INTR_MASK))

struct purin_master {
    struct resource *res;
    struct platform_device *pdev;

    struct purin_device device;
    int initialized;
    int ch_enabled;
    spinlock_t reg_lock;    /* protects update AIREG */
};

struct purin_clk_setting {
    int domain_sel;
    int bypass_en;
    unsigned long clk_div_sel;
    unsigned long ndiv_sel;
};

extern spinlock_t i2c_lock; //spinlock for share pinmux access
#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
extern int sysctl_ai_interface;
#endif

#define PURIN_RESET_HW_PCM()    \
do {                            \
    PMURESETREG &= ~(BIT8);     \
    PMURESETREG |= (BIT8);      \
} while(0);

#define PURIN_RESET_HW_SPDIF()  \
do {                            \
    PMURESETREG &= ~(BIT9);     \
    PMURESETREG |= (BIT9);      \
} while(0);

#define ENABLE_TXDMA()                          \
do {                                            \
    if(!(AIREG_READ32(CH0_CONF) & CH_TXDMA_EN))        \
        AIREG_UPDATE32(CH0_CONF, CH_TXDMA_EN, CH_TXDMA_EN);         \
    if(!(AIREG_READ32(CH1_CONF) & CH_TXDMA_EN))        \
        AIREG_UPDATE32(CH1_CONF, CH_TXDMA_EN, CH_TXDMA_EN);         \
} while (0);

#define TXDMA_READY()                           \
do {                                            \
    AIREG_UPDATE32(NOTIFY, 1<<(NOTIFY_TX_BIT(0)), 1<<(NOTIFY_TX_BIT(0)));     \
    AIREG_UPDATE32(NOTIFY, 1<<(NOTIFY_TX_BIT(1)), 1<<(NOTIFY_TX_BIT(1)));     \
} while (0);

#define ENABLE_RXDMA()                          \
do {                                            \
    if(!(AIREG_READ32(CH0_CONF) & CH_RXDMA_EN))        \
        AIREG_UPDATE32(CH0_CONF, CH_RXDMA_EN, CH_RXDMA_EN);         \
    if(!(AIREG_READ32(CH1_CONF) & CH_RXDMA_EN))        \
        AIREG_UPDATE32(CH1_CONF, CH_RXDMA_EN, CH_RXDMA_EN);         \
} while (0);

#define RXDMA_READY()                           \
do {                                            \
    AIREG_UPDATE32(NOTIFY, 1<<(NOTIFY_RX_BIT(0)), 1<<(NOTIFY_RX_BIT(0)));     \
    AIREG_UPDATE32(NOTIFY, 1<<(NOTIFY_RX_BIT(1)), 1<<(NOTIFY_RX_BIT(1)));     \
} while (0);

#define ENABLE_DUAL_CH()                        \
do {                                            \
    AIREG_UPDATE32(CH0_CONF, CH_EN, CH_EN);                   \
    AIREG_UPDATE32(CH1_CONF, CH_EN, CH_EN);                   \
} while(0);

#define DISABLE_DUAL_CH()                       \
do {                                            \
    AIREG_UPDATE32(CH0_CONF, 0, CH_EN);                \
    AIREG_UPDATE32(CH1_CONF, 0, CH_EN);                \
} while(0);


#define DISABLE_TXDMA()                \
do {                                   \
    AIREG_UPDATE32(CH0_CONF, 0, CH_TXDMA_EN); \
    AIREG_UPDATE32(CH1_CONF, 0, CH_TXDMA_EN); \
} while (0);

#define DISABLE_RXDMA()                \
do {                                   \
    AIREG_UPDATE32(CH0_CONF, 0, CH_RXDMA_EN); \
    AIREG_UPDATE32(CH1_CONF, 0, CH_RXDMA_EN); \
} while (0);

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
// for setting pcm_open_status_flag
// if we used snd_pcm_open(capture) in userspace,
// setting pcm_open_status_flag |= PCM_OPEN_STATUS_CAPTURE
//
// if we used snd_pcm_close(capture) in userspace
// setting pcm_open_status_flag &= ~PCM_OPEN_STATUS_CAPTURE
#define PCM_OPEN_STATUS_CAPTURE  BIT0
#define PCM_OPEN_STATUS_PLAYBACK BIT1
#endif

#endif
