#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
/*
 * ALSA Soc Audio Layer
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/mach-panther/panther.h>
#include <asm/mach-panther/pmu.h>
#include <asm/atomic.h>
#include <asm/bootinfo.h>

#include "purin-dma.h"
#include "purin-audio.h"

//#define PURIN_DEBUG
#ifdef PURIN_DEBUG
#undef pr_debug
#define purin_fmt(fmt) "[%s: %d]: " fmt, __func__, __LINE__
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG purin_fmt(fmt), ##__VA_ARGS__)

#define dp(fmt, ...) \
    printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#endif

static int sample_rate_table[SAMPLE_RATE_SUPPORT] = {
    8000,  16000, 22050, 24000,  32000,
    44100, 48000, 88200, 96000, 128000, 176400, 192000, 384000
};

static int pcm_rate_table[SAMPLE_RATE_SUPPORT] = {
     0,  1, -1, -1,  2,
    -1,  1, -1, -1, -1, -1, -1, -1
};

#if (defined(CONFIG_PANTHER_SND_PCM0_TLV320AIC3111) || defined(CONFIG_PANTHER_SND_PCM0_ESS9018Q2C))
static int i2s_rate_table[SAMPLE_RATE_SUPPORT] = {
    1, 1, 1, 1,  1,
    1, 1, 1, 1, -1, 1, 1, 1
};
#else
static int i2s_rate_table[SAMPLE_RATE_SUPPORT] = {
    1, 1, 1, 1,  1,
    1, 1, 1, 1, -1, -1, -1, -1
};
#endif

#ifndef CONFIG_PANTHER_FPGA
static struct purin_clk_setting i2s_rate_setting_table[SAMPLE_RATE_SUPPORT] = {
    {0, 0, 2, 24}, {0, 1, 2, 24}, {1, 1, 3,  4}, {0, 1, 2, 16}, {0, 1, 2, 12},
    {1, 1, 2,  4}, {0, 1, 2,  8}, {1, 1, 1,  4}, {0, 1, 2,  4}, {0, 1, 2,  3},
    {1, 1, 0,  4}, {0, 1, 2,  2}, {0, 1, 2,  1}
};
#endif

#define PURIN_CLOSE_IF()                \
do {                                    \
    AIREG_WRITE32(CH0_CONF,0x00000400UL);  \
    AIREG_WRITE32(CH1_CONF,0x00000400UL);  \
    AIREG_WRITE32(CFG,0x002800e4UL);       \
} while(0);

/*=============================================================================+
| Functions                                                                    |
+=============================================================================*/

static int purin_rate2idx(int rat)
{
    int i;
    for (i=0; i<SAMPLE_RATE_SUPPORT; i++) {
        if (rat == sample_rate_table[i]) {
            return i;
        }
    }
    return -1;
}

static void purin_device_init(struct purin_device *dev)
{
    int tmp = AIREG_READ32(CFG);    //variable for CFGREG macro

    /* common configuration */
    CFGREG(tmp, (dev->dma_mode     << 14), BIT14);
    CFGREG(tmp, (dev->ctrl_mode    <<  1), BIT1);

    /* I2S configuration */
    if (IF_I2S == sysctl_ai_interface)
    {
        CFGREG(tmp, (dev->txd_align    << 18), BIT18);
        CFGREG(tmp, (dev->swapch       << 17), BIT17);
        if (chip_revision >= 2) {
            CFGREG(tmp, BIT16, BIT16);
        }
    }
    /* PCM configuration */
    else
    {
        CFGREG(tmp, (dev->pcm_mclk      << 19), (BIT21|BIT20|BIT19));
        CFGREG(tmp, (dev->rx_bclk_latch << 15), BIT15);
        CFGREG(tmp, (dev->slot_mode     << 11), (BIT13|BIT12|BIT11));
        CFGREG(tmp, (dev->fs_rate       <<  9), (BIT10|BIT9));
        CFGREG(tmp, (dev->fs_interval   <<  5), (BIT8|BIT7|BIT6|BIT5));
        CFGREG(tmp, (dev->fs_long_en    <<  3), BIT3);
        CFGREG(tmp, (dev->fs_polar      <<  2), BIT2);
        if (chip_revision >= 2) {
            if(IF_PCMWB != sysctl_ai_interface) {
                CFGREG(tmp, BIT0, BIT0);
            }
        }
    }

    AIREG_WRITE32(CFG, tmp);
    pr_debug("dump PCM register : CFG %x, CH0ctl %x, CH1ctl %x\n",
        AIREG_READ32(CFG), AIREG_READ32(CH0_CONF), AIREG_READ32(CH1_CONF));

    pr_debug("<Enable Audio Interface Module>");
}

#define EXT_AUDIO_IS_ENABLED    (((IF_PCM==sysctl_ai_interface||IF_PCMWB==sysctl_ai_interface)&&(AIREG_READ32(CFG)&BIT0))||((IF_I2S==sysctl_ai_interface)&&(AIREG_READ32(CFG)&BIT16)))
/*=============================================================================+
| Functions                                                                    |
+=============================================================================*/
/*
 * this funtion is called by snd_soc_dai_set_fmt
 * (from purin_hw_params in purin-external-card.c)
 */
static int purin_i2s_set_dai_fmt(struct snd_soc_dai *cpu_dai,
        unsigned int fmt)
{
    struct purin_master * master;
    struct purin_device * device;
    unsigned long irqflags;

    pr_debug("name=%s fmt=%d\n", cpu_dai->name, fmt);
    master = snd_soc_dai_get_drvdata(cpu_dai);
    device = &master->device;
    spin_lock_irqsave(&(master->reg_lock), irqflags);
    pr_debug("EXT_AUDIO_IS_ENABLED %d, %x\n", EXT_AUDIO_IS_ENABLED, (fmt & SND_SOC_DAIFMT_MASTER_MASK));
    if (EXT_AUDIO_IS_ENABLED) {
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        return 0;
    }
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
    case SND_SOC_DAIFMT_CBS_CFS:    // codec is slave, panther is master
        device->ctrl_mode = 0;
        break;
    case SND_SOC_DAIFMT_CBM_CFM:    // codec is master, panther is slave
        device->ctrl_mode = 1;
        break;
    default:
        break;
    }

    return 0;
}

void purin_channel_init(int enable_channel)
{
    int ch0_config = AIREG_READ32(CH0_CONF), ch1_config = AIREG_READ32(CH1_CONF);
    /* set bit order to MSB */
    CFGREG(ch0_config, CH_MSB_FIRST, CH_MSB_FIRST);
    CFGREG(ch1_config, CH_MSB_FIRST, CH_MSB_FIRST);
    /* set default as 16bit */
    CFGREG(ch0_config, CH_BUSMODE_16, CH_BUSMODE_MASK);
    CFGREG(ch1_config, CH_BUSMODE_16, CH_BUSMODE_MASK);
    switch (sysctl_ai_interface) {
    case IF_PCMWB:
        {
            CFGREG(ch0_config, CH_BUSMODE_WB, CH_BUSMODE_MASK);
            CFGREG(ch1_config, CH_BUSMODE_WB, CH_BUSMODE_MASK);
            if (chip_revision >= 2) {
                REG_UPDATE32(PCMWB_SEL, (1 << 16), PCMWB_SEL_MSK);
                REG_UPDATE32(PCMWB_CH4_5_SLOT, (8 << 4), PCMWB_CH4_SLOT_MSK);
                REG_UPDATE32(PCMWB_CH4_5_SLOT, (10 << 11), PCMWB_CH5_SLOT_MSK);
#if defined(CONFIG_PANTHER_SUPPORT_I2S_48K) || defined(CONFIG_PANTHER_SND_PCM0_ES8388) || defined(CONFIG_PANTHER_SND_PCM1_ES8388)
                AIREG_UPDATE32(SPDIF_CH_STA_1, BIT16, BIT16);
#endif
            }
        }
    case IF_PCM:
        /* set ch1 PCM slot id to 2(divide 2 while 16bit, 1 slot = 8bit),
           ch0 slot id should always be 0 */
        CFGREG(ch0_config, 0<<3, CH_SLOT_ID_MSK);
        CFGREG(ch1_config, 2<<3, CH_SLOT_ID_MSK);
        break;
    default:
        break;
    }

    if (enable_channel) {
        CFGREG(ch0_config, CH_EN, CH_EN);
        CFGREG(ch1_config, CH_EN, CH_EN);
    }
    AIREG_UPDATE32(CH0_CONF, ch0_config, 0x0000FFFF);
    AIREG_UPDATE32(CH1_CONF, ch1_config, 0x0000FFFF);
    pr_debug("dual channel 0x<%x>, 0x<%x>", AIREG_READ32(CH0_CONF), AIREG_READ32(CH1_CONF));
}

static int purin_channel_update(int bit_depth)
{
    int ch0_config = AIREG_READ32(CH0_CONF), ch1_config = AIREG_READ32(CH1_CONF);
    int ret = 0;
    switch (bit_depth)
    {
    case 8:
        if (IF_PCM != sysctl_ai_interface) {
            ret = -EINVAL;
            goto result;
        }
        CFGREG(ch0_config, CH_BUSMODE_8, CH_BUSMODE_MASK);
        CFGREG(ch1_config, CH_BUSMODE_8, CH_BUSMODE_MASK);
        break;
    case 16:
        CFGREG(ch0_config, CH_BUSMODE_16, CH_BUSMODE_MASK);
        CFGREG(ch1_config, CH_BUSMODE_16, CH_BUSMODE_MASK);
        break;
    case 24:
        if (IF_I2S != sysctl_ai_interface) {
            ret = -EINVAL;
            goto result;
        }
        CFGREG(ch0_config, CH_BUSMODE_24, CH_BUSMODE_MASK);
        CFGREG(ch1_config, CH_BUSMODE_24, CH_BUSMODE_MASK);
        break;
    default:
        ret = -EINVAL;
        goto result;
    }
    AIREG_WRITE32(CH0_CONF,ch0_config);
    AIREG_WRITE32(CH1_CONF,ch1_config);
    pr_debug("dual channel 0x<%x>, 0x<%x>\n", AIREG_READ32(CH0_CONF), AIREG_READ32(CH1_CONF));
result:
    return ret;
}

static int purin_i2s_startup_with_reset(struct snd_pcm_substream *substream, 
                                        struct snd_soc_dai *cpu_dai)
{
    pr_debug("EXT_AUDIO_IS_ENABLED %d\n", EXT_AUDIO_IS_ENABLED);
    return 0;
}

static int purin_i2s_startup_without_reset(struct snd_pcm_substream *substream,
                                           struct snd_soc_dai *cpu_dai)
{
    struct purin_master * master;
    master = snd_soc_dai_get_drvdata(cpu_dai);
    pr_debug("EXT_AUDIO_IS_ENABLED %d\n", EXT_AUDIO_IS_ENABLED);
    /* Do not reset audio every time we play.
       Just reset at first time */
    if (0 == master->initialized)
    {
        pr_debug("RESET_HW_PCM\n");
        PURIN_RESET_HW_PCM();
        purin_channel_init(1);
        master->initialized = 1;
    }
    return 0;
}

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static int purin_i2s_prepare_with_reset(struct snd_pcm_substream *substream,
                                        struct snd_soc_dai *dai)
{
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long reset_device_ids[] = {  DEVICE_ID_PCM, 0 };
    unsigned long irqflags;
    spin_lock_irqsave(&(master->reg_lock), irqflags);
    if (!PCM_IS_CONFIGURED) {
        //PURIN_RESET_HW_PCM();
        REG_UPDATE32(0xbf004a44, 0, 0x1);
        pmu_reset_devices(reset_device_ids);
        udelay(5000);
        // descriptor specified tx and rx
        purin_desc_specific(substream);
        purin_device_init(&master->device);
        // reset flow, CH_ENABLE should keep original state
        purin_channel_init(0);
//      AIREG_UPDATE32(TX_DUMMY, 0xFFFFFF, 0x00FFFFFF);
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_TX);
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_RX);
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_UNDERRUN);
    }
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
    return 0;
}
#else
static int purin_i2s_prepare_with_reset(struct snd_pcm_substream *substream,
                                        struct snd_soc_dai *dai)
{
    int ret = 0;
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;
    pr_debug("\n");
    spin_lock_irqsave(&(master->reg_lock), irqflags);
    pr_debug("PCM_IS_CONFIGURED %d, master %p, sub %p\n", PCM_IS_CONFIGURED, master, substream);
    if (!PCM_IS_CONFIGURED) {
        pr_debug("PURIN_RESET_HW_PCM\n");
        PURIN_RESET_HW_PCM();
    }

    purin_desc_specific(substream);
    purin_device_init(&master->device);
    // reset flow, CH_ENABLE should keep original state
    purin_channel_init(0);
    if (IF_PCMWB != sysctl_ai_interface) {
        ret = purin_channel_update(master->device.bit_depth);
        if (ret < 0) {
            printk(KERN_ERR "[%s:%d]FORMAT not Support\n", __func__, __LINE__);
            goto result;
        }
    }

    /* unmask interrupt */
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_TX);
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_UNDERRUN);
    }
    else
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_RX);

result:
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);

    return ret;
}
#endif

static int purin_i2s_prepare_without_reset(struct snd_pcm_substream *substream,
                                           struct snd_soc_dai *dai)
{
    int ret = 0;
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;
    pr_debug("\n");
    spin_lock_irqsave(&(master->reg_lock), irqflags);

    purin_desc_specific(substream);
    purin_device_init(&master->device);
    ret = purin_channel_update(master->device.bit_depth);
    if (ret < 0) {
        printk(KERN_ERR "[%s:%d]FORMAT not Support\n", __func__, __LINE__);
        goto result;
    }

    /* unmask interrupt */
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_TX);
    else
        AIREG_UPDATE32(INTR_MASK, 0, INTR_STATUS_RX);

result:
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);

    return ret;
}

static int purin_i2s_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *params,
                struct snd_soc_dai *cpu_dai)
{
    struct purin_master *master;
    struct purin_device *device;

    int idx = 0;
    unsigned long irqflags;

    master = snd_soc_dai_get_drvdata(cpu_dai);
    if (!master) {
        pr_debug("%s:%d no master!!\n", __func__, __LINE__);
        return -1;
    }
    spin_lock_irqsave(&(master->reg_lock), irqflags);
    pr_debug("EXT_AUDIO_IS_ENABLED %d", EXT_AUDIO_IS_ENABLED);
    if (EXT_AUDIO_IS_ENABLED && PCM_IS_CONFIGURED) {
        spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        return 0;
    }
    spin_unlock_irqrestore(&(master->reg_lock), irqflags);

    device = &master->device;

    device->bit_depth = snd_pcm_format_width(params_format(params));
    pr_debug("bit_depth=%d\n", device->bit_depth);

    idx = purin_rate2idx(params_rate(params));
    pr_debug("rate:%d, idx:%d\n", params_rate(params), idx);
    if (idx < 0) {
        printk(KERN_ERR "Invalid sample rate %d\n", params_rate(params));
        return -1;
    }

    device->fs_rate = (IF_I2S == sysctl_ai_interface) ? i2s_rate_table[idx] : pcm_rate_table[idx];
    if (device->fs_rate < 0) {
        printk(KERN_ERR "Invalid sample rate %d in %d[0:PCM,1:I2S,2:PCMWB]\n", params_rate(params), sysctl_ai_interface);
        return -1;
    }

    if (IF_I2S != sysctl_ai_interface)
    {
#if defined(CONFIG_PANTHER_SND_PCM0_ES7243) || defined(CONFIG_PANTHER_SND_PCM0_ES8388) || defined(CONFIG_PANTHER_SND_PCM1_ES8388)
        device->pcm_mclk  = 4;	// it's fixed. represent 4.096MHz
        device->slot_mode = 3;
#else
        device->pcm_mclk  = 5;  // it's fixed. represent 8.192MHz
        device->slot_mode = 3;
#endif
#ifndef CONFIG_PANTHER_FPGA
        pmu_update_pcm_clk(PCM_CLK_8192);
#endif
    }
    else
    {
#ifndef CONFIG_PANTHER_FPGA
        pmu_update_i2s_clk(i2s_rate_setting_table[idx].domain_sel,
                           i2s_rate_setting_table[idx].bypass_en,
                           i2s_rate_setting_table[idx].clk_div_sel,
                           i2s_rate_setting_table[idx].ndiv_sel);
#endif
    }

    /* set rx latch as positive edge. (I2S/PCM) */
    device->rx_bclk_latch = 1;
    /* TDX align at falling edge (I2S) */
    device->txd_align = 0;
    /* No swapch (I2S) */
    device->swapch = 0;
    return 0;
}

#define ENABLE_INTERFACE()                              \
do {                                                    \
    if (IF_I2S == sysctl_ai_interface)                  \
        AIREG_UPDATE32(CFG, BIT16, BIT16);              \
    else                                                \
        AIREG_UPDATE32(CFG, BIT0, BIT0);                \
} while (0);

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static int purin_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
                  struct snd_soc_dai *cpu_dai)
{
    int ret = 0;
    struct purin_master *master = snd_soc_dai_get_drvdata(cpu_dai);
    unsigned long irqflags;
    //pr_debug("%s:%d name=%s cmd=%d id=%d\n", __func__, __LINE__, cpu_dai->name, cmd, cpu_dai->id);

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        {
            spin_lock_irqsave(&(master->reg_lock), irqflags);
            if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
                TXDMA_READY();
            }
            else
            {
                RXDMA_READY();
            }
            if (!EXT_AUDIO_IS_ENABLED)
            {
                ENABLE_TXDMA();
                ENABLE_RXDMA();
                ENABLE_DUAL_CH();
                udelay(100);
                ENABLE_INTERFACE();
            }
            spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        }
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        {
            pr_debug("stream %s stop\n", (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)? "PB":"REC");
            if(substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
                unsigned long reset_device_ids[] = {  DEVICE_ID_PCM, 0 };
                pmu_reset_devices(reset_device_ids);
            }
        }
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}
#else
static int purin_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
                  struct snd_soc_dai *cpu_dai)
{
    int ret = 0;
    struct purin_master *master = snd_soc_dai_get_drvdata(cpu_dai);
    unsigned long irqflags;
    //pr_debug("%s:%d name=%s cmd=%d id=%d\n", __func__, __LINE__, cpu_dai->name, cmd, cpu_dai->id);

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        {
            spin_lock_irqsave(&(master->reg_lock), irqflags);
            if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
                ENABLE_TXDMA();
                TXDMA_READY();
            }
            else
            {
                ENABLE_RXDMA();
                RXDMA_READY();
            }
            if (!EXT_AUDIO_IS_ENABLED)
            {
                pr_debug("ENABLE_DUAL_CH\n");
                ENABLE_DUAL_CH();
                ENABLE_INTERFACE();
            }
            spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        }
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        {
            spin_lock_irqsave(&(master->reg_lock), irqflags);
            pr_debug("stream %s stop\n", (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)? "PB":"REC");
            if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
                DISABLE_TXDMA();
            }
            else
            {
                DISABLE_RXDMA();
            }
            spin_unlock_irqrestore(&(master->reg_lock), irqflags);
        }
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}
#endif

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static void purin_i2s_shutdown_with_reset(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{

}
#else
static void purin_i2s_shutdown_with_reset(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;
    pr_debug("name=%s\n", dai->name);
    spin_lock_irqsave(&(master->reg_lock), irqflags);

    pr_debug("shutdown %d\n", substream->stream);
    /* mask interrupt */
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        AIREG_UPDATE32(INTR_MASK, INTR_STATUS_TX, INTR_STATUS_TX);
    else
        AIREG_UPDATE32(INTR_MASK, INTR_STATUS_RX, INTR_STATUS_RX);

    pr_debug("PCM_IS_CONFIGURED %d", PCM_IS_CONFIGURED);
    if (!PCM_IS_CONFIGURED) {
        PURIN_CLOSE_IF();
        PURIN_RESET_HW_PCM();
    }

    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
}
#endif

static void purin_i2s_shutdown_without_reset(struct snd_pcm_substream *substream,
                struct snd_soc_dai *dai)
{
    struct purin_master *master = snd_soc_dai_get_drvdata(dai);
    unsigned long irqflags;
    pr_debug("name=%s\n", dai->name);
    spin_lock_irqsave(&(master->reg_lock), irqflags);

    pr_debug("shutdown %d\n", substream->stream);
    /* mask interrupt */
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        AIREG_UPDATE32(INTR_MASK, INTR_STATUS_TX, INTR_STATUS_TX);
    else
        AIREG_UPDATE32(INTR_MASK, INTR_STATUS_RX, INTR_STATUS_RX);

    spin_unlock_irqrestore(&(master->reg_lock), irqflags);
}

#ifdef CONFIG_PM
static int purin_i2s_suspend(struct snd_soc_dai *dai)
{
    return 0;
}

static int purin_i2s_resume(struct snd_soc_dai *dai)
{
    return 0;
}

#else
#define purin_i2s_suspend NULL
#define purin_i2s_resume  NULL
#endif

#define PURIN_I2S_RATES (SNDRV_PCM_RATE_8000 | \
        SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_32000| SNDRV_PCM_RATE_22050 | \
        SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_384000)

static const struct snd_soc_dai_ops purin_ops_with_reset = {
    .startup    = purin_i2s_startup_with_reset,
    .shutdown   = purin_i2s_shutdown_with_reset,
    .prepare    = purin_i2s_prepare_with_reset,
    .trigger    = purin_i2s_trigger,
    .hw_params  = purin_i2s_hw_params,
    .set_fmt    = purin_i2s_set_dai_fmt,
};

static const struct snd_soc_dai_ops purin_ops_without_reset = {
    .startup    = purin_i2s_startup_without_reset,
    .shutdown   = purin_i2s_shutdown_without_reset,
    .prepare    = purin_i2s_prepare_without_reset,
    .trigger    = purin_i2s_trigger,
    .hw_params  = purin_i2s_hw_params,
    .set_fmt    = purin_i2s_set_dai_fmt,
};

static const struct snd_soc_component_driver purin_i2s_component = {
    .name       = "purin-i2s",
};


#ifdef CONFIG_CPU_LITTLE_ENDIAN
#define PURIN_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
                               SNDRV_PCM_FMTBIT_S24_LE)
#else
#define PURIN_SUPPORT_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_BE | \
                               SNDRV_PCM_FMTBIT_S24_BE)
#endif

static struct snd_soc_dai_driver purin_i2s_dai = {
    .suspend = purin_i2s_suspend,
    .resume = purin_i2s_resume,
#if defined(CONFIG_PANTHER_SND_PCM0_ES7243) || defined(CONFIG_PANTHER_SND_PCM0_ES7210)
    .capture = {
        .channels_min = 1,
#if defined(CONFIG_PANTHER_SUPPORT_I2S_48K)
        .channels_max = 6,
#else
        .channels_max = 8,
#endif
        .rates = PURIN_I2S_RATES,
        .formats = PURIN_SUPPORT_FORMATS,},
#if defined(CONFIG_PANTHER_SND_PCM1_ES8388)
    .playback = {
        .channels_min = 1,       //set channels_min 1 is due to mono wav and stereo wav need different machanism
        .channels_max = 2,
        .rates = PURIN_I2S_RATES,
        .formats = PURIN_SUPPORT_FORMATS,},
#endif
#else
    .playback = {
        .channels_min = 1,       //set channels_min 1 is due to mono wav and stereo wav need different machanism
        .channels_max = 2,
        .rates = PURIN_I2S_RATES,
        .formats = PURIN_SUPPORT_FORMATS,},
    .capture = {
        .channels_min = 1,
        .channels_max = 2,
        .rates = PURIN_I2S_RATES,
        .formats = PURIN_SUPPORT_FORMATS,},
#endif
    .ops = &purin_ops_with_reset,
};

EXPORT_SYMBOL_GPL(purin_i2s_dai);

static int purin_i2s_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int ret;
    struct resource *res;

    struct purin_master *master;
#if 0 // For FPGA
    int gpio_ids[] = { 10, 11, 14, 15, 16, -1 };
    unsigned long gpio_funcs[] = { 5, 5, 5, 5, 5};

    printk(KERN_EMERG "pmu_set_gpio_function\n");
    pmu_set_gpio_function(gpio_ids, gpio_funcs);
#endif

    master = kzalloc(sizeof(struct purin_master), GFP_KERNEL);

    if (unlikely(!master)) {
        dev_err(dev, "Could not allocate master\n");
        ret = -ENOMEM;
        goto err_out;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (unlikely(res < 0)) {
        dev_err(dev, "no memory specified\n");
        ret = -ENOENT;
        goto err_out;
    }

    ret = devm_snd_soc_register_component(dev, &purin_i2s_component, &purin_i2s_dai, 1);

    if (unlikely(ret != 0)) {
        dev_err(dev, "failed to register dai\n");
        goto err_out;
    }

    master->res = res;
    master->pdev = pdev;
    spin_lock_init(&(master->reg_lock));

    dev_set_drvdata(dev, (void *)master);
    if (chip_revision >= 2) {
        master->initialized = 0;
        if(IF_PCMWB != sysctl_ai_interface) {
            purin_i2s_dai.ops = &purin_ops_without_reset;
        }
    }
    return ret;

err_out:
#if !defined(AUDIO_RECOVERY)
    if (master) {
        kfree(master);
        master = NULL;
    }
#endif
    return ret;
}

static int purin_i2s_remove(struct platform_device *pdev)
{
    struct device *dev;
    struct purin_master* master;
    dev=&pdev->dev;
    master = dev_get_drvdata(dev);
    if (likely(master)) {
        kfree(master);
        master = NULL;
    }
    snd_soc_unregister_component(&pdev->dev);
    return 0;
}

static struct platform_driver purin_i2s_driver = {
    .probe = purin_i2s_probe,
    .remove = purin_i2s_remove,

    .driver = {
        .name = "purin-audio",
        .owner = THIS_MODULE,
    },
};

static int __init purin_i2s_init(void)
{
    /* ai */
    if (strstr(arcs_cmdline, "ai=pcmwb"))
    {
        sysctl_ai_interface = IF_PCMWB;
    }
    else if (strstr(arcs_cmdline, "ai=pcm"))
    {
        sysctl_ai_interface = IF_PCM;
    }
    else
    {
        sysctl_ai_interface = IF_I2S;
    }

    return platform_driver_register(&purin_i2s_driver);
}

static void __exit purin_i2s_exit(void)
{
    platform_driver_unregister(&purin_i2s_driver);
}

module_init(purin_i2s_init);
module_exit(purin_i2s_exit);

/* Module information */
MODULE_AUTHOR("Nady Chen");
MODULE_DESCRIPTION("I2S SoC Interface");
MODULE_LICENSE("GPL");
#endif //!defined(CONFIG_PANTHER_SND_PCM0_NONE)
