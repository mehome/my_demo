/*
 * ALSA PCM interface for the Cheetah chip
 *
 */

#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <asm/delay.h>

#include <sound/core.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/pcm.h>

#include <asm/mach-panther/panther.h>
#include <asm/mach-generic/dma-coherence.h>
#include <asm/io.h>

#ifdef DBG_PCMWB_UNDERRUN
#include <linux/jiffies.h>
#include <linux/ktime.h>
#endif

#include "purin-audio.h"
#include "purin-dma.h"

#define DETECT_TX_ALL_ZERO 0
#define ENABLE_NETLINK 0

#if DETECT_TX_ALL_ZERO
#include <net/genetlink.h>
#endif

#if ENABLE_NETLINK
#define NETLINK_USER 31
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#endif

#ifdef CFG_IO_AUDIO_AMP_POWER
#include <asm/mach-panther/gpio.h>
#endif

#include <linux/workqueue.h>
#include <asm/mach-panther/pmu.h>
#include <asm/mach-panther/pinmux.h>

#define purin_fmt(fmt) "[%s: %d]: " fmt, __func__, __LINE__
//#define PURIN_DEBUG
#ifdef PURIN_DEBUG
#undef pr_debug
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG purin_fmt(fmt), ##__VA_ARGS__)
#endif

#if 1
#define CACHED_ADDR(x)     ( ((u32) (x) & 0x1fffffffUL) | 0x80000000UL )
#define UNCACHED_ADDR(x)   ( ((u32) (x) & 0x1fffffffUL) | 0xA0000000UL )
#define PHYSICAL_ADDR(x)   ((u32) (x) & 0x1fffffffUL)
#define VIRTUAL_ADDR(x)    ((u32) (x) | 0x80000000L)
#define UNCACHED_VIRTUAL_ADDR(x)    ((u32) (x) | 0xA0000000L)
#endif

#undef pr_err
#define pr_err(fmt, ...)    printk(KERN_EMERG purin_fmt(fmt), ##__VA_ARGS__)
#define ASSERT(cond, message)   \
do {                            \
    if(!(cond)) {               \
        panic(message);         \
    }                           \
} while(0)

enum {
    DMATYPE_DEFAULT = 0,
    DMATYPE_SPDIF16,
    DMATYPE_PCMWB,
};

#define PCMWB_SLOT_NUM 8

#ifdef CONFIG_PCMGPIOCONTROL
#define hotplug_path uevent_helper
static void purin_pcm_event_hook(struct work_struct *work);
#endif

#if defined (CFG_IO_AUDIO_AMP_POWER)
static int gpio_status = -1;
#endif

int purin_spk_on(int enable)
{
#if defined (CFG_IO_AUDIO_AMP_POWER)
    if (!gpio_status) {
        if (enable)
            gpio_set_value(AUDIO_AMP_POWER, AUDIO_AMP_ACTIVE_LEVEL);
        else
            gpio_set_value(AUDIO_AMP_POWER, !AUDIO_AMP_ACTIVE_LEVEL);
    }
#endif
    return 0;
}

#ifdef CONFIG_CPU_LITTLE_ENDIAN
    #define DMA_SUPPORTED_FORMATS (SNDRV_PCM_FMTBIT_S8 |        \
                                   SNDRV_PCM_FMTBIT_U8 |        \
                                   SNDRV_PCM_FMTBIT_S16_LE |    \
                                   SNDRV_PCM_FMTBIT_S20_3LE |   \
                                   SNDRV_PCM_FMTBIT_S24_LE)
#else
    #define DMA_SUPPORTED_FORMATS (SNDRV_PCM_FMTBIT_S8 |        \
                                   SNDRV_PCM_FMTBIT_U8 |        \
                                   SNDRV_PCM_FMTBIT_S16_BE |    \
                                   SNDRV_PCM_FMTBIT_S20_3BE |   \
                                   SNDRV_PCM_FMTBIT_S24_BE)
#endif

/**
 *  audio data is SNDRV_PCM_INFO_INTERLEAVED in panther
 */
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static const struct snd_pcm_hardware purin_pcm_tx_hardware = {
    .info           = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID,
    .formats        = DMA_SUPPORTED_FORMATS,
    .rate_min       = 8000,
    .rate_max       = 384000,
    .channels_min   = 1,
    .channels_max   = 2,
    .fifo_size      = 32,
    .period_bytes_min   = PCMWB_REAL_BYTE_SIZE_44100_INDESC,
    .period_bytes_max   = PCMWB_REAL_BYTE_SIZE_44100_INDESC,
    .periods_min        = MULTIPLE_FOR_ALSA,
    .periods_max        = MULTIPLE_FOR_ALSA,
    .buffer_bytes_max   = PCMWB_BUFFER_SIZE_44100,
};

static const struct snd_pcm_hardware purin_pcm_hardware = {
    .info           = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID,
    .formats        = DMA_SUPPORTED_FORMATS,
    .rate_min       = 8000,
    .rate_max       = 384000,
    .channels_min   = 1,
#if defined(CONFIG_PANTHER_SND_PCM0_ES7243) || defined(CONFIG_PANTHER_SND_PCM0_ES7210)
#if defined(CONFIG_PANTHER_SUPPORT_I2S_48K)
    .channels_max   = 6,
#else
    .channels_max   = 8,
#endif
#else
    .channels_max   = 2,
#endif
    .fifo_size      = 32,
    .period_bytes_min   = PURIN_DESC_BUFSIZE,
    .period_bytes_max   = PURIN_DESC_BUFSIZE,
    .periods_min        = MULTIPLE_FOR_ALSA,
    .periods_max        = MULTIPLE_FOR_ALSA,
    .buffer_bytes_max   = TOTAL_BUFSIZE_FOR_ALSA,
};

#else
static const struct snd_pcm_hardware purin_pcm_hardware = {
    .info           = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID,
    .formats        = DMA_SUPPORTED_FORMATS,
    .rate_min       = 8000,
    .rate_max       = 384000,
    .channels_min   = 1,
#if defined(CONFIG_PANTHER_SND_PCM0_ES7243)
    .channels_max   = 8,
#else
    .channels_max   = 2,
#endif
    .fifo_size      = 32,
    .period_bytes_min   = PURIN_DESC_BUFSIZE,
    .period_bytes_max   = PURIN_DESC_BUFSIZE,
    .periods_min        = PURIN_DESC_NUM * MULTIPLE_FOR_ALSA,
    .periods_max        = PURIN_DESC_NUM * MULTIPLE_FOR_ALSA,
    .buffer_bytes_max   = TOTAL_BUFSIZE_FOR_ALSA,
};
#endif

struct purin_pcm_stream
{
    struct snd_pcm_substream *tx_substream;
    struct snd_pcm_substream *rx_substream;
    struct snd_pcm_substream *adc_substream;
    spinlock_t pcm_lock;
};
static struct purin_pcm_stream ai_pcm;

#if defined(AUDIO_RECOVERY)
#define AUTO_DETECT_RX_OVERRUN
void purin_desc_specific_recovery(struct snd_pcm_substream *substream);
void purin_desc_init_recovery(struct snd_pcm_substream *substream);
static int dma_tx_new_period(struct snd_pcm_substream *substream);
static int dma_rx_new_period(struct snd_pcm_substream *substream);
extern void purin_channel_init(int enable_channel);
extern int sysctl_ai_recovery;
enum {
    RECOVERY_NONE     = 0,
    RECOVERY_SHUTDOWN,
    RECOVERY_START,
    RECOVERY_DONE
};

enum {
    RECOVERY_IF_PCM = BIT0,
    RECOVERY_IF_I2S = BIT1,
    RECOVERY_IF_DAC = BIT2
};

enum {
    BACKUP_CFG = 0,
    BACKUP_INTR_MASK,
    BACKUP_CH0_CONF,
    BACKUP_CH1_CONF
};

struct recovery_info {
    u32 state;
    u32 enabled_interface;
    u32 reg_backup[4];  // CONF, INTR_MASK, CH0_CONF, CH1_CONF
};

static struct recovery_info recovery;

static void disable_interface(void)
{
    if(recovery.enabled_interface & RECOVERY_IF_PCM)
    {
        AIREG_UPDATE32(CFG, 0, BIT0);
    }
    if(recovery.enabled_interface & RECOVERY_IF_I2S)
    {
        AIREG_UPDATE32(CFG, 0, BIT16);
    }

    if(recovery.enabled_interface & RECOVERY_IF_DAC)
    {
        AIREG_UPDATE32(DAC_CONF, 0, BIT0);
    }
    printk(KERN_DEBUG "[%s]%x\n", __func__, recovery.enabled_interface);
}

static void enable_interface(void)
{
    if(recovery.enabled_interface & RECOVERY_IF_PCM)
    {
        AIREG_UPDATE32(CFG, BIT0, BIT0);
    }
    if(recovery.enabled_interface & RECOVERY_IF_I2S)
    {
        AIREG_UPDATE32(CFG, BIT16, BIT16);
    }

    if(recovery.enabled_interface & RECOVERY_IF_DAC)
    {
        AIREG_UPDATE32(DAC_CONF, BIT0, BIT0);
    }
}

static struct hrtimer recovery_timer;

void start_recovery_timer(void)
{
    recovery.reg_backup[BACKUP_INTR_MASK] = AIREG_READ32(INTR_MASK);
    AIREG_UPDATE32(INTR_MASK, 0x0F, 0x0F);  // it will UNMASK after RECOVERY_START
    recovery.state = RECOVERY_SHUTDOWN;
    hrtimer_start(&recovery_timer, ktime_set(0, (300 * 1000)), HRTIMER_MODE_REL);  // 300 micro sec
}

void do_backup(void)
{
    recovery.enabled_interface = 0;
    if(AIREG_READ32(CFG) & BIT0)
    {
        recovery.enabled_interface |= RECOVERY_IF_PCM;
    }
    else if(AIREG_READ32(CFG) & BIT16)
    {
        recovery.enabled_interface |= RECOVERY_IF_I2S;
    }

    if(AIREG_READ32(DAC_CONF) & BIT0)
    {
        recovery.enabled_interface |= RECOVERY_IF_DAC;
    }
    recovery.reg_backup[BACKUP_CFG] = AIREG_READ32(CFG);
    recovery.reg_backup[BACKUP_CH0_CONF]  = AIREG_READ32(CH0_CONF);
    recovery.reg_backup[BACKUP_CH1_CONF]  = AIREG_READ32(CH1_CONF);
}

void do_suspend(void)
{
    unsigned long reset_device_ids[] = { DEVICE_ID_PCM, 0 };
    do_backup();
#if 0 // switch mclk pinmux to gpio, try to cancel popup noise
    i2s_mclk_suspend();
#endif
    AIREG_UPDATE32(CH0_CONF, 0, 0x07);  // DISABLE TXDMA, RXDMA, CH0
    AIREG_UPDATE32(CH1_CONF, 0, 0x07);  // DISABLE TXDMA, RXDMA, CH1
    disable_interface();
    pmu_reset_devices(reset_device_ids);
}

void do_resume(void)
{
    u32 prepare_idx;
    if(ai_pcm.tx_substream) {
        printk(KERN_DEBUG "[%s]tx_substream\n", __func__);
        purin_desc_init_recovery(ai_pcm.tx_substream);
        purin_desc_specific_recovery(ai_pcm.tx_substream);
        for(prepare_idx = 0; prepare_idx < PURIN_DESC_NUM; ++prepare_idx) {
            dma_tx_new_period(ai_pcm.tx_substream);
        }
    }
    if(ai_pcm.rx_substream) {
        printk(KERN_DEBUG "[%s]rx_substream\n", __func__);
        purin_desc_init_recovery(ai_pcm.rx_substream);
        purin_desc_specific_recovery(ai_pcm.rx_substream);
        for(prepare_idx = 0; prepare_idx < PURIN_DESC_NUM; ++prepare_idx) {
            dma_rx_new_period(ai_pcm.rx_substream);
        }
    }
    /* setting about channel info about pcm, pcmwb, i2s, bit width*/
    purin_channel_init(0);
    /* restore CFG, but we do not enable i2s/pcm here */
    AIREG_UPDATE32(CFG, recovery.reg_backup[BACKUP_CFG], 0xFFFEFFFE);
    AIREG_WRITE32(INTR_MASK, recovery.reg_backup[BACKUP_INTR_MASK]);
    /* just update txdma, rxdma, channel enabled, we set others at purin_channel_init(0) */
    AIREG_UPDATE32(CH0_CONF, recovery.reg_backup[BACKUP_CH0_CONF], 0x00000007);
    AIREG_UPDATE32(CH1_CONF, recovery.reg_backup[BACKUP_CH1_CONF], 0x00000007);
    recovery.state = RECOVERY_DONE;
    sysctl_ai_recovery = 0;
    udelay(100);
    enable_interface();
#if 0 // return pinmux from gpio to mclk, try to cancel popup noise
    i2s_mclk_resume();
#endif
}

static enum hrtimer_restart recovery_timer_func(struct hrtimer *timer)
{
    switch(recovery.state) {
    case RECOVERY_SHUTDOWN:
        printk(KERN_DEBUG "[%s]%d\n", __func__, recovery.state);
        do_suspend();
        recovery.state = RECOVERY_START;
        hrtimer_start(&recovery_timer, ktime_set(0, (10 * 1000)), HRTIMER_MODE_REL);
        break;
    case RECOVERY_START:
        printk(KERN_DEBUG "[%s]%d\n", __func__, recovery.state);
        do_resume();
        hrtimer_start(&recovery_timer, ktime_set(1, 0), HRTIMER_MODE_REL); // 1 s
        break;
    case RECOVERY_DONE:
        recovery.state = RECOVERY_NONE;
        break;
    default:
        printk(KERN_DEBUG "[%s]%d\n", __func__, recovery.state);
        break;
    }
    return HRTIMER_NORESTART;
}
#if defined(CONFIG_PM)
void audio_suspend(void)
{
    recovery.reg_backup[BACKUP_INTR_MASK] = AIREG_READ32(INTR_MASK);
    AIREG_UPDATE32(INTR_MASK, 0x0F, 0x0F);
    do_suspend();
}

void audio_resume(void)
{
    do_resume();
    recovery.state = RECOVERY_NONE;
    //hrtimer_start(&recovery_timer, ktime_set(0, (500)), HRTIMER_MODE_REL);  // 500ns
}
#endif
#endif

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static int ready_before_open = 0;

#ifdef DBG_PCMWB_UNDERRUN
static unsigned long enter_lock_isr_jiffies = 0;
static unsigned long pre_isr_jiffies_tx = 0;
static unsigned long pre_isr_jiffies_rx = 0;
static struct hrtimer tsf_timer;
static ktime_t cur_ktime;
static unsigned long txdesc_jiffies[PURIN_DESC_NUM];

static enum hrtimer_restart check_irq_timer(struct hrtimer *timer)
{
    printk(KERN_DEBUG "[%s] INTR_MASK %x, INTR_STATUS %x, CH0_CONF %x, CH1_CONF %x, CFG %x, STATE_MACHINE %x\n", __func__,
           AIREG_READ32(INTR_MASK),
           AIREG_READ32(INTR_STATUS),
           AIREG_READ32(CH0_CONF),
           AIREG_READ32(CH1_CONF),
           AIREG_READ32(CFG),
           AIREG_READ32(STATE_MACH));
    printk(KERN_DEBUG "jiffies %lu, ktime_sub %lld\n", jiffies, ktime_to_us(ktime_sub(ktime_get(), cur_ktime)));
    return HRTIMER_NORESTART;
}
#endif

static int desc_ready = 0;
static int prepared_trigger = 0;
volatile int pcm_open_status_flag;
static struct purin_dma_buffer_info pcm_44100_dmabuffer;
static struct purin_preopen_info tx_dma_before_open;
static struct purin_preopen_info rx_dma_before_open;
#endif

static inline void purin_desc_inc(unsigned int * no)
{
    (*no)++;
    if (PURIN_DESC_NUM == (*no))
        (*no) = 0;
}

dma_addr_t spdif_16bit_transfter(struct purin_runtime_data *purin_rtd, unsigned char* src_data, unsigned int dma_size)
{
    u16* src_ptr;
    u32* dst_ptr;
    dma_addr_t ret = 0;
    int i;

    src_ptr = (u16*)src_data;
    dst_ptr = (u32*)(purin_rtd->dataxfer_addr + ((PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple) * purin_rtd->dataxfer_handle_idx));
    ret = purin_rtd->dataxfer_addr_phy + ((PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple) * purin_rtd->dataxfer_handle_idx);

    for (i = 0; i < dma_size/2; i++)
    {
        dst_ptr[i] = ((src_ptr[i] << 8) & 0x00ffff00UL);
    }

    purin_rtd->dataxfer_handle_idx++;
    purin_rtd->dataxfer_handle_idx = purin_rtd->dataxfer_handle_idx % 2;
    return ret;
}

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
dma_addr_t pcmwb_16bit_transfter(struct purin_runtime_data *purin_rtd, unsigned char* src_data, unsigned int dma_size)
{
    s16* src_ptr;
    s16* dst_ptr;
    dma_addr_t ret = 0;
    int offset = ((PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple) * purin_rtd->dataxfer_handle_idx);
    int src_idx, dst_idx = 0;
    int sum_left_right = 0;
    s16 avg_16 = 0;
    int filled_idx = 0;

    src_ptr = (s16*)src_data;
    dst_ptr = (s16*)(purin_rtd->dataxfer_addr + offset);
    ret = purin_rtd->dataxfer_addr_phy + offset;
    //for(src_idx = 0, dst_idx = 0; src_idx < 0;)
    for(src_idx = 0, dst_idx = 0; src_idx < dma_size/2; )
    {
        for(filled_idx = 0; filled_idx < 8; ++filled_idx)
        {
            if(filled_idx >=2 && filled_idx <= 5)
            {
                dst_ptr[dst_idx++] = 0xaaaa;
                continue;
            }

            if(src_idx >= dma_size/2)
                break;

            sum_left_right = (int)src_ptr[src_idx++];
            sum_left_right+= (int)src_ptr[src_idx++];
            avg_16 = (s16)(sum_left_right / 2);
            if(avg_16 == -1)
            {
                avg_16 = -2;
            }
            dst_ptr[dst_idx++] = avg_16;
        }
    }

    purin_rtd->dataxfer_handle_idx++;
    purin_rtd->dataxfer_handle_idx = purin_rtd->dataxfer_handle_idx % PURIN_DESC_NUM;
    //printk(KERN_EMERG "#offset %d\n", offset);
    return ret;
}
#else
dma_addr_t pcmwb_16bit_transfter(struct purin_runtime_data *purin_rtd, unsigned char* src_data, unsigned int dma_size)
{
    u16* src_ptr;
    u16* dst_ptr;
    u16* dst_end;
    dma_addr_t ret = 0;
    int i;
    int j = 0;
    int mod_i = 0;
    int offset = ((PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple) * purin_rtd->dataxfer_handle_idx);

    src_ptr = (u16*)src_data;
    dst_ptr = (u16*)(purin_rtd->dataxfer_addr + offset);
    dst_end = dst_ptr + (PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple)/2; // 16bit = 2 byte, so we div 2
    ret = purin_rtd->dataxfer_addr_phy + offset;

    for (i = 0; dst_ptr+i < dst_end; i++) {
        mod_i = i % PCMWB_SLOT_NUM;
        switch(mod_i) {
        case 0: case 2: case 4: case 6:
            dst_ptr[i] = src_ptr[j];
            break;
        case 1: case 3: case 5: case 7:
            dst_ptr[i] = src_ptr[j+1];
            break;
        }
        if(PCMWB_SLOT_NUM-1 == mod_i) {
            j += 2;
        }
    }

    purin_rtd->dataxfer_handle_idx++;
    purin_rtd->dataxfer_handle_idx = purin_rtd->dataxfer_handle_idx % 2;
    //printk(KERN_EMERG "#offset %d\n", offset);
    return ret;
}
#endif

#if ENABLE_NETLINK
#define NL_AUDIO_MSG_MUTE    1
#define NL_AUDIO_MSG_UNMUTE  0

static struct sock *purin_nl_sk = NULL;
static int purin_nl_pid = -1;
static int registered_netlink = 0;
static int is_in_mute = 0;

static void notify_user_mute(int mute)
{
    struct nlmsghdr *nlh;
    int msg_size;
    char *msg = NULL;
    struct sk_buff *skb_out;
    int res;

    if(purin_nl_pid < 0) {
        return;
    }

    if(mute) {
        msg = "mute";
    } else {
        msg = "unmute";
    }

    printk(KERN_DEBUG "Entering: %s\n", __FUNCTION__);
    msg_size = strlen(msg) + 1;

    skb_out = nlmsg_new(msg_size, 0);
    if(!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    if(nlh == NULL)
    {
        nlmsg_free(skb_out);
        printk(KERN_INFO "Error while put msg to skb_out\n");
        return;
    }
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(purin_nl_sk, skb_out, purin_nl_pid);

    if(res < 0) {
        printk(KERN_DEBUG "Error while sending bak to user\n");
    }
}

static void purin_nl_recv_register(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    char *msg = "Success";
    int msg_size;
    int res;

    printk(KERN_DEBUG "Entering: %s\n", __FUNCTION__);

    msg_size = strlen(msg);

    nlh = (struct nlmsghdr*)skb->data;
    printk(KERN_DEBUG "Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
    purin_nl_pid = nlh->nlmsg_pid; /*pid of sending process */

    skb_out = nlmsg_new(msg_size, 0);

    if(!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    } 
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    if(nlh == NULL)
    {
        nlmsg_free(skb_out);
        printk(KERN_INFO "Error while put msg to skb_out\n");
        return;
    }
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(purin_nl_sk, skb_out, purin_nl_pid);

    if(res < 0)
        printk(KERN_INFO "Error while sending bak to user\n");
}


static struct netlink_kernel_cfg purin_nl_cfg = {
    .input = purin_nl_recv_register,
};

static void init_netlink(void)
{
    purin_nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &purin_nl_cfg);
    if(!purin_nl_sk)
    {
        printk(KERN_DEBUG "Error creating socket.\n");
        return;
    }
    registered_netlink = 1;
}

static void finalize_netlink(void)
{
    if (registered_netlink == 0)
        return;

    netlink_kernel_release(purin_nl_sk);
}

#endif


#if DETECT_TX_ALL_ZERO
enum {
	PURIN_ATTR_UNSPEC,
	PURIN_ATTR_NOTIFY_USER,
	__PURIN_ATTR_MAX,
};

#define PURIN_ATTR_MAX (__PURIN_ATTR_MAX - 1)

static struct genl_family purin_genl_family = {
    .id = GENL_ID_GENERATE,
    .hdrsize = 0,
    .name = "AUDIO_CTRL",
    .version = 1,
    .maxattr = PURIN_ATTR_MAX,
};

static struct nla_policy purin_genl_policy[PURIN_ATTR_MAX + 1] = {
    [PURIN_ATTR_NOTIFY_USER] = { .type = NLA_U32 },
};

enum {
	PURIN_CMD_UNSPEC,
	PURIN_CMD_NOTIFY_USER,
	__PURIN_CMD_MAX,
};
#define PURIN_CMD_MAX (__PURIN_CMD_MAX - 1)

static int purin_doit(struct sk_buff *skb_2, struct genl_info *info)
{
        return 0;
}

static struct genl_ops purin_genl_ops[] = {
    {
        .cmd = PURIN_CMD_NOTIFY_USER,
        .policy = purin_genl_policy,
        .doit = purin_doit,
    },
};

enum purin_multicast_groups {
	PURIN_MCGRP_NOTIFY_USER,
};

static const struct genl_multicast_group purin_mcgrps[] = {
	[PURIN_MCGRP_NOTIFY_USER] = { .name = "audio_group", },
};

#define NL_AUDIO_MSG_MUTE    1
#define NL_AUDIO_MSG_UNMUTE  0
#define NL_AUDIO_MSG_SIZE   16

static int registered_netlink = 0;
static int is_in_mute = 0;

static void notify_user_mute(int mute)
{
    struct sk_buff *skb;
    void *msg_hdr;
    int err = 0;
    if(registered_netlink == 0) {
        printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
        return;
    }

    printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
    skb = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!skb)
        return;

    printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
    msg_hdr = genlmsg_put(skb, 0, 0, &purin_genl_family, 0, PURIN_CMD_NOTIFY_USER);
    if (msg_hdr == NULL) {
        nlmsg_free(skb);
        printk(KERN_DEBUG "genlmsg_put failed\n");
        return;
    }

    printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
    if (nla_put_u32(skb, PURIN_ATTR_NOTIFY_USER, mute)) {
        genlmsg_cancel(skb, msg_hdr);
        nlmsg_free(skb);
        printk(KERN_DEBUG "genlmsg_put failed\n");
        return;
    }
    printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
    genlmsg_end(skb, msg_hdr);
    printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
    err = genlmsg_multicast(&purin_genl_family, skb, 0, 0, GFP_KERNEL);
    printk(KERN_DEBUG "[%s][%d][%d]\n", __func__, __LINE__, err);
}

static int purin_genl_rcv_nl_event(struct notifier_block *this,
				 unsigned long event, void *ptr)
{
    return NOTIFY_DONE;
}

static struct notifier_block purin_nl_notifier = {
	.notifier_call  = purin_genl_rcv_nl_event,
};

static void init_netlink(void)
{
    int rc;
    rc = genl_register_family_with_ops_groups(&purin_genl_family,
						  purin_genl_ops,
						  purin_mcgrps);
    printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
	if (rc){
        printk(KERN_DEBUG "genl_register_family_with_ops_groups failed %d\n", rc);
		return;
    }

    printk(KERN_DEBUG "[%s][%d]\n", __func__, __LINE__);
	netlink_register_notifier(&purin_nl_notifier);

    registered_netlink = 1;
}

static void finalize_netlink(void)
{
    if (registered_netlink == 0)
        return;

    netlink_unregister_notifier(&purin_nl_notifier);
	genl_unregister_family(&purin_genl_family);
}

#endif

u32 dma_desc_tdata;
/**
 * fill data to tx descriptor for playing music
 */
static int dma_tx_new_period(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    volatile struct purin_dma_desc *des;

    unsigned int dma_size;
    unsigned int offset;
    dma_addr_t mem_addr;
    int ret = 0;
    if (purin_rtd->active) {
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + tx_dma_before_open.desc_ready_idx;
#else
        des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + purin_rtd->next;
#endif
        if(!(des->info & DES_OWN)) {
            printk(KERN_EMERG "tx desc is in use, continue ...\n");
            return ret;
        }

        dma_size = frames_to_bytes(runtime, runtime->period_size);
        offset = dma_size * purin_rtd->period;
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        purin_rtd->hw_ptr += bytes_to_frames(runtime, PCMWB_REAL_BYTE_SIZE_44100_INDESC);
#else
        purin_rtd->hw_ptr += bytes_to_frames(runtime, PURIN_DESC_BUFSIZE);
#endif
//#if DETECT_TX_ALL_ZERO
#if ENABLE_NETLINK
{
        u32 compare_idx = 0;
        u32 compare_ret = 0;
        u16* src_ptr;
        pr_debug("period (%d) out of (%d)\n", purin_rtd->period, runtime->periods);
        pr_debug("period_size %d frames\n offset %d bytes\n", (u32)runtime->period_size, offset);
        pr_debug("dma_size %d bytes\n", dma_size);
        src_ptr = (u16*)(runtime->dma_area + offset);
        for (compare_idx = 0; compare_idx < (unsigned int)runtime->period_size; ++compare_idx)
        {
            if (src_ptr[compare_idx] != 0)
            {
                compare_ret = 1;
                break;
            }
        }
        if (compare_ret == 1 && is_in_mute == 1)
        {
            pmu_internal_dac_enable(1);
            printk(KERN_EMERG "enable internal DAC\n");
            notify_user_mute(NL_AUDIO_MSG_UNMUTE);
            is_in_mute = 0;
            printk(KERN_DEBUG "!!!filled All ZERO [%d frames][%d bytes]\n\n", (u32)runtime->period_size, dma_size);
        }
        else if (compare_ret == 0 && is_in_mute == 0)
        {
            pmu_internal_dac_enable(0);
            printk(KERN_EMERG "disable internal DAC\n");
            notify_user_mute(NL_AUDIO_MSG_MUTE);
            is_in_mute = 1;
            printk(KERN_DEBUG "filled All ZERO [%d frames][%d bytes]\n\n", (u32)runtime->period_size, dma_size);
        }
}
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        snd_BUG_ON(dma_size > purin_pcm_tx_hardware.period_bytes_max);
#else
        snd_BUG_ON(dma_size > purin_pcm_hardware.period_bytes_max);
#endif
        /* runtime->dma_addr is physical address */
        mem_addr = (dma_addr_t)(runtime->dma_addr + offset);
        switch (purin_rtd->dma_type) {
        case DMATYPE_SPDIF16:
            des->dptr = (void *)spdif_16bit_transfter(purin_rtd, (runtime->dma_area + offset), dma_size);
            break;
#if !defined(CONFIG_PANTHER_SND_PCM0_ES8388) && !defined(CONFIG_PANTHER_SND_PCM1_ES8388)
        case DMATYPE_PCMWB:
            des->dptr = (void *)pcmwb_16bit_transfter(purin_rtd, (runtime->dma_area + offset), dma_size);
            break;
#endif
        default:
            des->dptr = (void *)mem_addr;
            break;
        }
        /*--switch own bit to hw--*/
        des->info &= ~DES_OWN;

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
#ifdef DBG_PCMWB_UNDERRUN
        txdesc_jiffies[tx_dma_before_open.desc_ready_idx] = jiffies;
#endif
        purin_desc_inc(&tx_dma_before_open.desc_ready_idx);
#else
        purin_desc_inc(&purin_rtd->next);
#endif

        dma_desc_tdata = *((volatile u32 *)des);
        purin_rtd->period += 1;
        purin_rtd->period %= runtime->periods;
        //printk(KERN_EMERG "..purin_rtd->period=%d, offset=%d", purin_rtd->period, offset);

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        if(pcm_open_status_flag & PCM_OPEN_STATUS_PLAYBACK)
#endif
        /*--- this section aims for underrun recovery ---*/
        {
            struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

#if defined(AUDIO_RECOVERY)
            if(recovery.state == RECOVERY_START) {
                return ret;
            }
#endif
            if (likely(cpu_dai->driver->ops->trigger)) {
                ret = cpu_dai->driver->ops->trigger(substream, SNDRV_PCM_TRIGGER_START, cpu_dai);
                if (unlikely(ret < 0)) {
                    printk(KERN_ERR "re trigger failed\n");
                }
            }
        }
    }
    return ret;
}

static void audio_stop_dma(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;

    purin_rtd->active = 0;
    purin_rtd->period = 0;
    purin_rtd->periods = 0;
}

/**
 * prepare rx descriptor to capture
 */
static int dma_rx_new_period(struct snd_pcm_substream *substream){
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    volatile struct purin_dma_desc *des;

    unsigned int dma_size;
    unsigned int offset;
    dma_addr_t mem_addr;
    int ret = 0;

    if (purin_rtd->active) {
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + rx_dma_before_open.desc_ready_idx;
#else
        des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + purin_rtd->next;
#endif

        if(unlikely(!(des->info & DES_OWN))) {
            printk(KERN_ERR "desc is in use, continue ...\n");
            return ret;
        }

        dma_size = frames_to_bytes(runtime, runtime->period_size);
        offset = dma_size * purin_rtd->period;
        snd_BUG_ON(dma_size > purin_pcm_hardware.period_bytes_max);

        mem_addr = (dma_addr_t)(runtime->dma_addr + offset);

        des->dptr = (void *)mem_addr;
        des->info &= ~DES_OWN;
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        purin_desc_inc(&rx_dma_before_open.desc_ready_idx);
#else
        purin_desc_inc(&purin_rtd->next);
#endif
#if 0
        pr_debug("dma_addr = 0x%x, dma_area = 0x%x\n", (unsigned int) runtime->dma_addr, (unsigned int) runtime->dma_area);
        pr_debug("mem_addr = %x, dma_size = %d\n", (unsigned int) mem_addr, dma_size);
        print_hex_dump(KERN_EMERG, "hexdump:", DUMP_PREFIX_OFFSET,
            16, 1, runtime->dma_area + offset, dma_size, false);
#endif

        purin_rtd->period += 1;
        purin_rtd->period %= runtime->periods;

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        if(pcm_open_status_flag & PCM_OPEN_STATUS_CAPTURE)
#endif
        /* this section aims for overrun recovery */
        {
            struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

#if defined(AUDIO_RECOVERY)
            if(recovery.state == RECOVERY_START) {
                return ret;
            }
#endif
            if (cpu_dai->driver->ops->trigger) {
                ret = cpu_dai->driver->ops->trigger(substream, SNDRV_PCM_TRIGGER_START, cpu_dai);
                if (ret < 0) {
                    printk(KERN_ERR "re trigger failed\n");
                }
            }
        }
    }
    return ret;
}

static irqreturn_t tx_interrupt(int irq, struct snd_pcm_substream *substream) {
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    struct purin_dma_desc *des;
#if defined(CONFIG_SND_ALOOP)
    unsigned int total_offset = 0;
#endif
    unsigned long irqflags;

    if (!(purin_rtd->active))
    {
        return IRQ_HANDLED;
    }

#ifdef DBG_PCMWB_UNDERRUN
    enter_lock_isr_jiffies = jiffies;
#endif
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + tx_dma_before_open.desc_ready_idx;
#else
    des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + purin_rtd->next;
#endif
    while(des->info & DES_OWN)
    {
        purin_rtd->periods += 1;
        purin_rtd->periods %= runtime->periods;
        snd_pcm_period_elapsed(substream);
#if defined(CONFIG_SND_ALOOP)
        total_offset += frames_to_bytes(runtime, runtime->period_size);
#endif

        if (!purin_rtd->active)
            break;

        // in free-wheeling mode (dmix)
        // we don't need any Condition
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        spin_lock_irqsave(&tx_dma_before_open.lock, irqflags);
#else
        spin_lock_irqsave(&purin_rtd->lock, irqflags);
#endif
        dma_tx_new_period(substream);
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        spin_unlock_irqrestore(&tx_dma_before_open.lock, irqflags);
#else
        spin_unlock_irqrestore(&purin_rtd->lock, irqflags);
#endif

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + tx_dma_before_open.desc_ready_idx;
#else
        des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + purin_rtd->next;
#endif
    }

#if defined(CONFIG_SND_ALOOP)
    if(total_offset != 0) {
        start_update_pos_timer(total_offset);
    }
#endif

#ifdef DBG_PCMWB_UNDERRUN
    if(jiffies - enter_lock_isr_jiffies > 2) {
        printk(KERN_DEBUG "\n*********\njiffies %lu, enter_lock_isr_jiffies %lu\n********\n",
               jiffies, enter_lock_isr_jiffies);
    }
#endif
    return IRQ_HANDLED;
}

int micarray_channels = 0;
#if defined(AUTO_DETECT_RX_OVERRUN)
int autorecovey_rx_overrun(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    unsigned int offset = 0;
    unsigned int last_periods = 0;
    int i;
    u16* rx_data;

    if(micarray_channels >= 8 || micarray_channels == 0) {
        return 0; // we can not detect overrun if the number of channels >= 8
    }
    last_periods = ((purin_rtd->periods - 1) + runtime->periods) % runtime->periods;
    offset = frames_to_bytes(runtime, runtime->period_size) * last_periods;
    rx_data = (u16*)(runtime->dma_area + offset);
    if(rx_data[0] == 0xFFFF && rx_data[1] == 0xFFFF &&
       rx_data[2] == 0xFFFF && rx_data[3] == 0xFFFF &&
       rx_data[4] == 0xFFFF && rx_data[5] == 0xFFFF &&
       rx_data[6] == 0xFFFF && rx_data[7] == 0xFFFF
       )
    {
        // sometimes all 0xFFFF at startup
        return 0;
    }

    for(i = micarray_channels; i < 8; i++)
    {
        if(rx_data[i] != 0) {
            printk(KERN_DEBUG "recovery micarray %d\n", micarray_channels);
            print_hex_dump(KERN_DEBUG, "hexdump:", DUMP_PREFIX_OFFSET,
                            8, 2, runtime->dma_area + offset, 16, false);
            sysctl_ai_recovery = 1;
            start_recovery_timer();
            return 1;
        }
    }
    return 0;
}
#endif

static irqreturn_t rx_interrupt(int irq, struct snd_pcm_substream *substream) {
    volatile struct purin_dma_desc *des;
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;

    unsigned long irqflags;

    if (!(purin_rtd->active))
    {
        return IRQ_HANDLED;
    }

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + rx_dma_before_open.desc_ready_idx;
#else
    des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + purin_rtd->next;
#endif
    while(des->info & DES_OWN) {
        purin_rtd->periods += 1;
        purin_rtd->periods %= runtime->periods;
        snd_pcm_period_elapsed(substream);
        if (purin_rtd->active)
        {
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
            spin_lock_irqsave(&rx_dma_before_open.lock, irqflags);
#else
            spin_lock_irqsave(&purin_rtd->lock, irqflags);
#endif
            dma_rx_new_period(substream);
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
            spin_unlock_irqrestore(&rx_dma_before_open.lock, irqflags);
#else
            spin_unlock_irqrestore(&purin_rtd->lock, irqflags);
#endif
        }
        else
        {
            break;
        }
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + rx_dma_before_open.desc_ready_idx;
#else
    des = ((struct purin_dma_desc *)purin_rtd->dma_desc_array) + purin_rtd->next;
#endif
    }
#if defined(AUTO_DETECT_RX_OVERRUN)
    autorecovey_rx_overrun(substream);
#endif
    return IRQ_HANDLED;
}

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static void dma_tx_rx_prepare_for_trigger(struct snd_pcm_substream *substream)
{
    volatile struct purin_dma_desc *des;
    u32 des_idx = 0;
    struct purin_preopen_info * dma_before_open;
    if(prepared_trigger == 1)
    {
        return;
    }

    if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
    {
        des = (struct purin_dma_desc *)rx_dma_before_open.dma_desc_array;
        dma_before_open = &rx_dma_before_open;
    }
    else
    {
        des = (struct purin_dma_desc *)tx_dma_before_open.dma_desc_array;
        dma_before_open = &tx_dma_before_open;
    }
    for(des_idx = 0; des_idx < PURIN_DESC_NUM; des_idx++)
    {
        if(!(des->info & DES_OWN)) {
            continue;
        }
        des->dptr = (void*)(dma_before_open->buffer_info.data_addr_phy);
        des->info &= ~DES_OWN;
        des++;
#ifdef DBG_PCMWB_UNDERRUN
        txdesc_jiffies[des_idx] = jiffies;
#endif
    }
    prepared_trigger = 1;
    if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
    {
        //printk(KERN_EMERG "AI_PCM: not support open PLYABACK_first !!");
        panic("AI_PCM: not support open PLYABACK_first !!");
        for(des_idx = 0; des_idx < PURIN_DESC_NUM; des_idx++)
            dma_tx_new_period(substream);
    }
    else
    {
        for(des_idx = 0; des_idx < PURIN_DESC_NUM; des_idx++)
            dma_rx_new_period(substream);
    }
    tx_dma_before_open.desc_ready_idx = 0;
    rx_dma_before_open.desc_ready_idx = 0;
#ifdef DBG_PCMWB_UNDERRUN
    pre_isr_jiffies_tx = jiffies;
#endif
}
#endif

void purin_desc_init_recovery(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    volatile struct purin_dma_desc *des = (struct purin_dma_desc *)purin_rtd->dma_desc_array;
    int no;
    u32 des_bufsize = (LENG_MASK & (PURIN_BUFSIZE << BUF_SIZE_SHIFT));

    for (no = PURIN_DESC_NUM; no > 0; no--, des++)
    {
        /* des own bit setting */
        des->info = DES_OWN;
        des->info |= des_bufsize;
        /* des buf setting */
        des->dptr= 0;
    }
    /* set DES_EOR to tell hw this descriptor is the end of ring buffers */
    des--;
    des->info |= DES_EOR;
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        tx_dma_before_open.desc_ready_idx = 0;
    else
        rx_dma_before_open.desc_ready_idx = 0;
#else
    purin_rtd->next = 0;
#endif
    purin_rtd->period = purin_rtd->periods + 1; // start with the last buffer (tx/rx done) + 1
    purin_rtd->period %= runtime->periods;
}

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static void purin_desc_init(struct purin_dma_desc * desc_array)
{
    volatile struct purin_dma_desc *des = desc_array;
    int no;
    u32 des_bufsize = (LENG_MASK & (PURIN_BUFSIZE << BUF_SIZE_SHIFT));

    for (no = PURIN_DESC_NUM; no > 0; no--, des++)
    {
        /* des own bit setting */
        des->info = DES_OWN;
        des->info |= des_bufsize;
        /* des buf setting */
        des->dptr= 0;
    }
    /* set DES_EOR to tell hw this descriptor is the end of ring buffers */
    des--;
    des->info |= DES_EOR;
}
#else
static void purin_desc_init(struct snd_pcm_substream *substream, int buf_length)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    volatile struct purin_dma_desc *des = (struct purin_dma_desc *)purin_rtd->dma_desc_array;
    int no;
    u32 des_bufsize = (LENG_MASK & (buf_length << BUF_SIZE_SHIFT));

    for (no = PURIN_DESC_NUM; no > 0; no--, des++)
    {
        /* des own bit setting */
        des->info = DES_OWN;
        des->info |= des_bufsize;
        /* des buf setting */
        des->dptr= 0;
    }
    /* set DES_EOR to tell hw this descriptor is the end of ring buffers */
    des--;
    des->info |= DES_EOR;

    pr_debug("dma_desc_array=0x%08x, dma_desc_array_phys=0x%08x\n",
        (unsigned int)purin_rtd->dma_desc_array, (unsigned int)purin_rtd->dma_desc_array_phys);
}
#endif

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static irqreturn_t pcm_isr(int irq, void *dev)
{
    unsigned int status;
    unsigned int check_flag;
    unsigned long irqflags;
    volatile struct purin_dma_desc *des;
    status = AIREG_READ32(INTR_STATUS);
    AIREG_WRITE32(INTR_CLR, status);   // clear status
    status &= (~(AIREG_READ32(INTR_MASK)) & INTR_STATUS_MASK);
#ifdef DBG_PCMWB_UNDERRUN
    if(AIREG_READ32(INTR_MASK) != 0x04)
    {
        panic("INTR_MASK != 0x04\n");
    }
#endif
    if(status & INTR_STATUS_UNDERRUN)
    {
#ifdef DBG_PCMWB_UNDERRUN
        int dbg_idx = 0;
        unsigned long current_jiffies = jiffies;
        printk(KERN_DEBUG "[%s] INTR_MASK %x, INTR_STATUS %x, CH0_CONF %x, CH1_CONF %x, CFG %x, STATE_MACHINE %x\n", __func__,
           AIREG_READ32(INTR_MASK),
           AIREG_READ32(INTR_STATUS),
           AIREG_READ32(CH0_CONF),
           AIREG_READ32(CH1_CONF),
           AIREG_READ32(CFG),
           AIREG_READ32(STATE_MACH));

        printk(KERN_DEBUG "next_des_index_for_prepare %d\n", tx_dma_before_open.desc_ready_idx);
        for(dbg_idx = 0; dbg_idx < PURIN_DESC_NUM; dbg_idx++)
        {
            des = ((struct purin_dma_desc *)tx_dma_before_open.dma_desc_array) + dbg_idx;
            printk(KERN_DEBUG "des[%d] %p, des->dptr %p, des->info %x, tx_jiffies %lu, current %lu\n",
                   dbg_idx, des, des->dptr, des->info, txdesc_jiffies[dbg_idx], current_jiffies);
        }
#endif
#if !defined(AUDIO_RECOVERY)
        printk(KERN_EMERG "************** INTR_STATUS_UNDERRUN status %x**************\n", status);
        pmu_system_restart();
//      panic("INTR_STATUS_UNDERRUN\n");
#else
        printk(KERN_DEBUG "IRQ UNDERRUN %x\n", status);
        sysctl_ai_recovery = 1;
#endif
    }

#if defined(AUDIO_RECOVERY)
    if((sysctl_ai_recovery != 0) && (recovery.state == RECOVERY_NONE)) {
        start_recovery_timer();
        return IRQ_HANDLED;
    }
#endif

    if (status & INTR_STATUS_TX)
    {
#ifdef DBG_PCMWB_UNDERRUN
        hrtimer_try_to_cancel(&tsf_timer);
        if(pre_isr_jiffies_tx != 0) {
            if(jiffies - pre_isr_jiffies_tx >= 3) {
                printk(KERN_DEBUG "\n==========\njiffies %lu, pre_isr_jiffies_tx %lu, diff %lu\n==========\n",
                       jiffies, pre_isr_jiffies_tx, (jiffies - pre_isr_jiffies_tx));
            }
        }
        pre_isr_jiffies_tx = jiffies;
#endif
        spin_lock_irqsave(&tx_dma_before_open.lock, irqflags);
        check_flag = (pcm_open_status_flag & PCM_OPEN_STATUS_PLAYBACK);
        spin_unlock_irqrestore(&tx_dma_before_open.lock, irqflags);
        if (check_flag && (ai_pcm.tx_substream != NULL))
        {
            tx_interrupt(irq, ai_pcm.tx_substream);
        }
        else
        {
#ifdef DBG_PCMWB_UNDERRUN
            // prepare fake desc
            enter_lock_isr_jiffies = jiffies;
#endif
            des = ((struct purin_dma_desc *)tx_dma_before_open.dma_desc_array) + tx_dma_before_open.desc_ready_idx;
            while(des->info & DES_OWN)
            {
                spin_lock_irqsave(&tx_dma_before_open.lock, irqflags);
                des->dptr = (void*)(tx_dma_before_open.buffer_info.data_addr_phy);
                des->info &= ~DES_OWN;
#ifdef DBG_PCMWB_UNDERRUN
                txdesc_jiffies[tx_dma_before_open.desc_ready_idx] = jiffies;
#endif
                purin_desc_inc(&tx_dma_before_open.desc_ready_idx);
                TXDMA_READY();
                spin_unlock_irqrestore(&tx_dma_before_open.lock, irqflags);
                des = ((struct purin_dma_desc *)tx_dma_before_open.dma_desc_array) + tx_dma_before_open.desc_ready_idx;
            }
#ifdef DBG_PCMWB_UNDERRUN
            if(jiffies - enter_lock_isr_jiffies > 2) {
                printk(KERN_DEBUG "\n==========\njiffies %lu, enter_lock_isr_jiffies %lu\n==========\n",
                       jiffies, enter_lock_isr_jiffies);
            }
#endif
        }
#ifdef DBG_PCMWB_UNDERRUN
        cur_ktime = ktime_get();
        hrtimer_start(&tsf_timer, ktime_set(0, (30 * 1000 * 1000)), HRTIMER_MODE_REL);
#endif
    }

    if (status & INTR_STATUS_RX)
    {
#ifdef DBG_PCMWB_UNDERRUN
        if(pre_isr_jiffies_rx != 0) {
            if(jiffies - pre_isr_jiffies_rx >= 3) {
                printk(KERN_DEBUG "\n==========\njiffies %lu, pre_isr_jiffies_rx %lu, diff %lu\n==========\n",
                       jiffies, pre_isr_jiffies_rx, (jiffies - pre_isr_jiffies_rx));
            }
        }
        pre_isr_jiffies_rx = jiffies;
#endif
        spin_lock_irqsave(&tx_dma_before_open.lock, irqflags);
        check_flag = (pcm_open_status_flag & PCM_OPEN_STATUS_CAPTURE);
        spin_unlock_irqrestore(&tx_dma_before_open.lock, irqflags);
        if (check_flag && (ai_pcm.rx_substream != NULL))
        {
            rx_interrupt(irq, ai_pcm.rx_substream);
        }
        else
        {
            // prepare fake desc
            spin_lock_irqsave(&rx_dma_before_open.lock, irqflags);
            des = ((struct purin_dma_desc *)rx_dma_before_open.dma_desc_array) + rx_dma_before_open.desc_ready_idx;
            if(des->info & DES_OWN)
            {
                des->dptr = (void*)(rx_dma_before_open.buffer_info.data_addr_phy);
                des->info &= ~DES_OWN;
                purin_desc_inc(&rx_dma_before_open.desc_ready_idx);
            }
            spin_unlock_irqrestore(&rx_dma_before_open.lock, irqflags);
        }
    }
    return IRQ_HANDLED;
}
#else
static irqreturn_t pcm_isr(int irq, void *dev)
{
    unsigned int status;
    irqreturn_t irq_ret = IRQ_NONE;

    status = AIREG_READ32(INTR_STATUS);
    pr_debug("status %x, mask %x\n,", status, AIREG_READ32(INTR_MASK));
    AIREG_WRITE32(INTR_CLR, status);   // clear status
    status &= (~(AIREG_READ32(INTR_MASK)) & INTR_STATUS_MASK);
    pr_debug("status %x\n", status);

    if(status & INTR_STATUS_UNDERRUN)
    {
#if defined(AUDIO_RECOVERY)
        printk(KERN_DEBUG "IRQ UNDERRUN %x\n", status);
        sysctl_ai_recovery = 1;
#endif
    }

#if defined(AUDIO_RECOVERY)
    if((sysctl_ai_recovery != 0) && (recovery.state == RECOVERY_NONE)) {
        start_recovery_timer();
        return IRQ_HANDLED;
    }
#endif
    if (status & INTR_STATUS_TX)
    {
        if (ai_pcm.tx_substream != NULL)
        {
            irq_ret |= tx_interrupt(irq, ai_pcm.tx_substream);
        }
        else
        {
            pr_err("tx_substream is NULL in pcm_isr\n");
        }
    }

    if (status & INTR_STATUS_RX)
    {
        if (ai_pcm.rx_substream != NULL)
        {
            irq_ret |= rx_interrupt(irq, ai_pcm.rx_substream);
        }
        else
        {
            pr_err("rx_substream is NULL in pcm_isr\n");
        }
    }

    if (status & INTR_STATUS_ADC)
    {
        if (ai_pcm.adc_substream != NULL)
        {
            irq_ret |= rx_interrupt(irq, ai_pcm.adc_substream);
        }
        else
        {
            pr_err("adc_substream is NULL in pcm_isr\n");
        }
    }

    return irq_ret;
}
#endif

static int purin_dma_setup_handlers(struct snd_pcm_substream *substream)
{
    int ret = -ENODEV;
    int stream_type = substream->stream;
    int is_tx = (stream_type == SNDRV_PCM_STREAM_PLAYBACK) ? 1 : 0;
//  const char * irq_name = (is_tx ? "AI_TX" : "AI_RX");
    unsigned long irqflags;
    struct purin_runtime_data *purin_rtd = substream->runtime->private_data;

    if (stream_type > SNDRV_PCM_STREAM_LAST) {
        printk(KERN_CRIT "Can't register IRQ %d for unsupported stream_type %d\n", IRQ_PCM, stream_type);
        return ret;
    }

    spin_lock_irqsave(&(ai_pcm.pcm_lock), irqflags);
    if ((NULL == ai_pcm.tx_substream) && (NULL == ai_pcm.rx_substream) && (NULL == ai_pcm.adc_substream))
    {
        if (is_tx)
        {
            ai_pcm.tx_substream = substream;
        }
        else if(1 == purin_rtd->is_adc)
        {
            ai_pcm.adc_substream = substream;
        }
        else
        {
            ai_pcm.rx_substream = substream;
        }
        ret = request_irq(IRQ_PCM, pcm_isr, IRQF_SHARED, "AI_PCM", (void *)&ai_pcm);
        if (unlikely(ret)) {
            printk(KERN_CRIT "Can't register IRQ %d for DMA stream_type %d\n", IRQ_PCM, stream_type);
            if (is_tx)
            {
                ai_pcm.tx_substream = NULL;
            }
            else if(1 == purin_rtd->is_adc)
            {
                ai_pcm.adc_substream = NULL;
            }
            else
            {
                ai_pcm.rx_substream = NULL;
            }
        }
    }
    else
    {
        if (is_tx)
        {
            if (ai_pcm.tx_substream == NULL)
            {
                ai_pcm.tx_substream = substream;
                ret = 0;
            }
            else
            {
                pr_err("tx_substream duplicate irq request\n");
            }
        }
        else if(1 == purin_rtd->is_adc)
        {
            if (ai_pcm.adc_substream == NULL)
            {
                pr_debug("adc_substream\n");
                ai_pcm.adc_substream = substream;
                ret = 0;
            }
            else
            {
                pr_err("adc_substream duplicate irq request\n");
            }
        }
        else
        {
            if (ai_pcm.rx_substream == NULL)
            {
                ai_pcm.rx_substream = substream;
                ret = 0;
            }
            else
            {
                pr_err("rx_substream duplicate irq request\n");
            }
        }
        //printk(KERN_NOTICE "IRQ %d is registered, ignore DMA stream_type %s\n", IRQ_PCM, irq_name);
    }
    spin_unlock_irqrestore(&(ai_pcm.pcm_lock), irqflags);

    return ret;
}

/**
 * when start playing music, (aplay or others)
 */
static int purin_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd;
    int ret;
    /*
     * Set it to make sure that the DMA buffer can be evenly divided into periods.
     * Reject any DMA buffer whose size is not a multiple of the period size.
     */
    ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
    if (unlikely(ret < 0))
        goto out;

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        ret = snd_soc_set_runtime_hwparams(substream, &purin_pcm_tx_hardware);
    else
        ret = snd_soc_set_runtime_hwparams(substream, &purin_pcm_hardware);
#else
    ret = snd_soc_set_runtime_hwparams(substream, &purin_pcm_hardware);
#endif
    if (unlikely(ret < 0))
        goto out;

    ret = -ENOMEM;
    purin_rtd = kzalloc(sizeof(struct purin_runtime_data), GFP_KERNEL);
    if (unlikely(!purin_rtd))
        goto out;

    spin_lock_init(&purin_rtd->lock);
    /* init codec state flag for waiting codec CJC8988 (cheetah code) */
    purin_rtd->codec_init = 0;
    /* set default not playing 16 bit-depth data in spdif */
    purin_rtd->dma_type = DMATYPE_DEFAULT;
    purin_rtd->dataxfer_addr = NULL;
    purin_rtd->dataxfer_addr_phy = 0;
    purin_rtd->dataxfer_multiple = 1;
    runtime->private_data = purin_rtd;

#ifdef CONFIG_PCMGPIOCONTROL
    INIT_DELAYED_WORK(&purin_rtd->pcm_event.gpioctrl, purin_pcm_event_hook);
#endif
    return 0;

 out:
    return ret;
}

#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
static int purin_pcm_external_open(struct snd_pcm_substream *substream)
{
    int ret = purin_pcm_open(substream);
    struct purin_runtime_data *purin_rtd;
    if (unlikely(ret < 0))
        goto out;

    purin_rtd = substream->runtime->private_data;
    /* setup handlers for interrupt*/
    ret = purin_dma_setup_handlers(substream);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "ERR: Error %d setting interrupt function\n", ret);
        goto err1;
    }

    if (IF_PCMWB == sysctl_ai_interface)
        purin_rtd->dma_type = DMATYPE_PCMWB;

    return 0;

err1:
    kfree(purin_rtd);
out:
    return ret;
}
#endif

static int purin_pcm_internal_open(struct snd_pcm_substream *substream)
{
    int ret = purin_pcm_open(substream);
    struct purin_runtime_data *purin_rtd;
    if (unlikely(ret < 0))
        goto out;

    purin_rtd = substream->runtime->private_data;
    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
        purin_rtd->is_adc = 1;
    }

    /* setup handlers for interrupt*/
    ret = purin_dma_setup_handlers(substream);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "ERR: Error %d setting interrupt function\n", ret);
        goto err1;
    }
    return 0;

err1:
    kfree(purin_rtd);
out:
    return ret;
}

/**
 * when stop playing music, (aplay or others)
 */
static int purin_pcm_close(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    unsigned long irqflags;
    int ret = -EINVAL;

    spin_lock_irqsave(&(ai_pcm.pcm_lock), irqflags);
    pr_debug("start %d\n", substream->stream);
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
    {
        if (ai_pcm.tx_substream == substream)
        {
            ai_pcm.tx_substream = NULL;
            ret = 0;
        }
        else
        {
            pr_err("tx_substream not equal\n");
        }
    }
    else if(1 == purin_rtd->is_adc)
    {
        if (ai_pcm.adc_substream == substream)
        {
            ai_pcm.adc_substream = NULL;
            ret = 0;
        }
        else
        {
            pr_err("adc_substream not equal\n");
        }
    }
    else
    {
        if (ai_pcm.rx_substream == substream)
        {
            ai_pcm.rx_substream = NULL;
            ret = 0;
        }
        else
        {
            pr_err("rx_substream not equal\n");
        }
    }

    if ((NULL == ai_pcm.tx_substream) && (NULL == ai_pcm.rx_substream) && (NULL == ai_pcm.adc_substream))
    {
        pr_debug("free irq %d\n", IRQ_PCM);
        free_irq(IRQ_PCM, (void *)&ai_pcm);
    }
    spin_unlock_irqrestore(&(ai_pcm.pcm_lock), irqflags);

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    desc_ready = 0;
    prepared_trigger = 0;
#endif
    kfree(purin_rtd);
    return ret;
}

static int purin_pcm_hw_params_common_part(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;

    /* alloc descriptor size for tx/rx */
    pr_debug("alloc total size of descriptor: %d\n", PURIN_DESC_SIZE);
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
    {
        purin_rtd->dma_desc_array = tx_dma_before_open.dma_desc_array;
        purin_rtd->dma_desc_array_phys = tx_dma_before_open.dma_desc_array_phys;
    }
    else
    {
        purin_rtd->dma_desc_array = rx_dma_before_open.dma_desc_array;
        purin_rtd->dma_desc_array_phys = rx_dma_before_open.dma_desc_array_phys;
    }
#else
    purin_rtd->dma_desc_array = dma_alloc_coherent(substream->pcm->card->dev,
                                                   PURIN_DESC_SIZE,
                                                   &purin_rtd->dma_desc_array_phys,
                                                   GFP_KERNEL);
    if (unlikely(!purin_rtd->dma_desc_array))
        return -ENOMEM;

#endif
    pr_debug("dma_desc_array=0x%08x, dma_desc_array_phys=0x%08x\n",
             (unsigned int)purin_rtd->dma_desc_array,
             (unsigned int)purin_rtd->dma_desc_array_phys);

    /* copy buffer information to runtime->dma_buffer */
    snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
    runtime->dma_bytes = params_buffer_bytes(params);

    pr_debug("%s: snd_purin_audio_hw_params runtime->dma_addr 0x(%x)\n",
        __func__, (unsigned int)runtime->dma_addr);
    pr_debug("%s: snd_purin_audio_hw_params runtime->dma_area 0x(%x)\n",
        __func__, (unsigned int)runtime->dma_area);
    pr_debug("%s: snd_purin_audio_hw_params runtime->dma_bytes %d\n",
        __func__, (unsigned int)runtime->dma_bytes);

    return 0;
}

static int purin_pcm_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
    struct purin_runtime_data *purin_rtd = substream->runtime->private_data;
#endif
    int ret = purin_pcm_hw_params_common_part(substream, params);
    int des_bufsize = PURIN_BUFSIZE;
    if (ret != 0) {
        return ret;
    }

#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
    if (DMATYPE_PCMWB == purin_rtd->dma_type && SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        purin_rtd->dataxfer_multiple = 1;
        des_bufsize = PURIN_BUFSIZE * purin_rtd->dataxfer_multiple;
        /*
            FOR TX, we create a buffer to filled data with the following format in a descriptor
            [|44100 MHzdata (882 sample)| (1280-882)sample 0xffff] * 2 piece
        */
        purin_rtd->dataxfer_addr = pcm_44100_dmabuffer.data_addr;
        purin_rtd->dataxfer_addr_phy = pcm_44100_dmabuffer.data_addr_phy;
        purin_rtd->dataxfer_handle_idx = 0;
#else
#if !defined(CONFIG_PANTHER_SND_PCM0_ES8388) && !defined(CONFIG_PANTHER_SND_PCM1_ES8388)
        purin_rtd->dataxfer_multiple = 4;
        des_bufsize = PURIN_BUFSIZE * purin_rtd->dataxfer_multiple;
        /*
            FOR TX
            |0xFFFF|0xFFFF|  -> |0xFFFF|0xFFFF|0x0000|0x0000|0x0000|0x0000|0x0000|0x0000|
            we must prepare 4 times buffer size to filled in.

            TODO: FOR RX
        */
        purin_rtd->dataxfer_addr = dma_alloc_coherent(substream->pcm->card->dev,
                                                   PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple*2,    //(PURIN_BUFSIZE * PURIN_BUF_UNIT) * 4 * 2;
                                                   &purin_rtd->dataxfer_addr_phy,
                                                   GFP_KERNEL);

        ASSERT((purin_rtd->dataxfer_addr != NULL), "Failed to alloc PCM 16bit-wideband dataxfer\n");

        purin_rtd->dataxfer_handle_idx = 0;
#endif
#endif
#if !defined(CONFIG_PANTHER_SND_PCM0_ES8388) && !defined(CONFIG_PANTHER_SND_PCM1_ES8388)
        pr_debug("purin_rtd->dataxfer_addr=0x%08x, purin_rtd->dataxfer_addr_phy=0x%08x\n, "
                 "alloc %d, des_bufsize %d\n",
                 (unsigned int)purin_rtd->dataxfer_addr, (unsigned int)purin_rtd->dataxfer_addr_phy,
                 PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple*2, des_bufsize);
#endif
    }
    pr_debug("DMA_TYPE %d\n", purin_rtd->dma_type);
#endif
    /* initialize descriptor */
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    purin_desc_init(substream, des_bufsize);
#endif

    return 0;
}

static int purin_pcm_hw_free(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    pr_debug("%s:%d\n", __func__, __LINE__);

#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    dma_free_coherent(substream->pcm->card->dev,
                      PURIN_DESC_SIZE,
                      purin_rtd->dma_desc_array,
                      purin_rtd->dma_desc_array_phys);
#endif
    if (purin_rtd->dataxfer_addr) {
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        pr_debug("free purin_rtd->spdif_addr=0x%08x, purin_rtd->spdif_addr_phy=0x%08x\n",
                 (unsigned int)purin_rtd->dataxfer_addr, (unsigned int)purin_rtd->dataxfer_addr_phy);
        dma_free_coherent(substream->pcm->card->dev,
                          PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple*2, //(PURIN_BUFSIZE * PURIN_BUF_UNIT) * 4 * 2;
                          purin_rtd->dataxfer_addr,
                          purin_rtd->dataxfer_addr_phy);
#endif
        purin_rtd->dataxfer_addr = NULL;
        purin_rtd->dataxfer_addr_phy = 0;
    }
    snd_pcm_set_runtime_buffer(substream, NULL);

    return 0;
}

static int purin_pcm_spdif_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    unsigned int bit_depth =
            snd_pcm_format_width(params_format(params));
    int des_bufsize = PURIN_BUFSIZE;

    int ret = purin_pcm_hw_params_common_part(substream, params);
    if (ret != 0) {
        return ret;
    }
    pr_debug("spdif bit_depth = %d\n", bit_depth);
    if (bit_depth == 16 && substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        purin_rtd->dma_type = DMATYPE_SPDIF16;
        /*
            spdif 16bit will be filled in 32bit in buffer as following
            |0xFFFF|  -> |0x00FFFF00|
            we must prepare double size buffer to filled in.
        */
        purin_rtd->dataxfer_multiple = 2;
        des_bufsize = PURIN_BUFSIZE * purin_rtd->dataxfer_multiple;
        purin_rtd->dataxfer_addr = dma_alloc_coherent(substream->pcm->card->dev,
                                                   PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple*2,    //(PURIN_BUFSIZE * 2) * PURIN_BUF_UNIT * 2;
                                                   &purin_rtd->dataxfer_addr_phy,
                                                   GFP_KERNEL);

        ASSERT((purin_rtd->dataxfer_addr != NULL), "Failed to alloc SPDIF 16bit dataxfer\n");

        purin_rtd->dataxfer_handle_idx = 0;
        pr_debug("purin_rtd->spdif_addr=0x%08x, purin_rtd->spdif_addr_phy=0x%08x\n",
                 (unsigned int)purin_rtd->dataxfer_addr, (unsigned int)purin_rtd->dataxfer_addr_phy);
    }
    /* initialize descriptor */
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    purin_desc_init(substream, des_bufsize);
#endif

    return 0;
}

static int purin_pcm_spdif_hw_free(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    pr_debug("%s:%d\n", __func__, __LINE__);

    dma_free_coherent(substream->pcm->card->dev,
                      PURIN_DESC_SIZE,
                      purin_rtd->dma_desc_array,
                      purin_rtd->dma_desc_array_phys);

    if (purin_rtd->dataxfer_addr) {
        pr_debug("free purin_rtd->spdif_addr=0x%08x, purin_rtd->spdif_addr_phy=0x%08x\n",
                 (unsigned int)purin_rtd->dataxfer_addr, (unsigned int)purin_rtd->dataxfer_addr_phy);
        dma_free_coherent(substream->pcm->card->dev,
                          PURIN_DESC_BUFSIZE*purin_rtd->dataxfer_multiple*2, //(PURIN_BUFSIZE * 2) * PURIN_BUF_UNIT * 2;
                          purin_rtd->dataxfer_addr,
                          purin_rtd->dataxfer_addr_phy);
        purin_rtd->dataxfer_addr = NULL;
        purin_rtd->dataxfer_addr_phy = 0;
    }
    snd_pcm_set_runtime_buffer(substream, NULL);

    return 0;
}

static int purin_pcm_prepare(struct snd_pcm_substream *substream)
{
    struct purin_runtime_data *purin_rtd = substream->runtime->private_data;

#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    purin_rtd->period = 0;
    purin_rtd->periods = 0;
    purin_rtd->next = 0;
#else
    purin_rtd->period = 0;
    purin_rtd->periods = -1;
#endif

    return 0;
}

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
void purin_desc_specific_recovery(struct snd_pcm_substream *substream)
{
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        AIREG_WRITE32(AI_TXBASE, tx_dma_before_open.dma_desc_array_phys);
    if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
        AIREG_WRITE32(AI_RXBASE, rx_dma_before_open.dma_desc_array_phys);
}
void purin_desc_specific(struct snd_pcm_substream *substream)
{
    if(desc_ready == 0)
    {
        purin_desc_init(tx_dma_before_open.dma_desc_array);
        purin_desc_init(rx_dma_before_open.dma_desc_array);
        AIREG_WRITE32(AI_TXBASE, tx_dma_before_open.dma_desc_array_phys);
        AIREG_WRITE32(AI_RXBASE, rx_dma_before_open.dma_desc_array_phys);
        desc_ready = 1;
    }
}
#else
void purin_desc_specific(struct snd_pcm_substream *substream)
{
    struct purin_runtime_data *purin_rtd = substream->runtime->private_data;

    if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
        AIREG_WRITE32(AI_TXBASE, purin_rtd->dma_desc_array_phys);
    else {
        if (purin_rtd->is_adc) {
            AIREG_WRITE32(ADC_RXBASE, purin_rtd->dma_desc_array_phys);
        } else {
            AIREG_WRITE32(AI_RXBASE, purin_rtd->dma_desc_array_phys);
        }
    }
}

void purin_desc_specific_recovery(struct snd_pcm_substream *substream)
{
    purin_desc_specific(substream);
}
#endif

#ifdef CONFIG_PCMGPIOCONTROL
static void purin_pcm_event_hook(struct work_struct *work)
{
    struct purin_pcm_event *pcm_event = (struct purin_pcm_event *)container_of(work, struct purin_pcm_event, gpioctrl.work);
    struct kobj_uevent_env *env = kzalloc(sizeof(struct kobj_uevent_env), GFP_KERNEL);
    char *argv[3] = {
        hotplug_path,
        "pcm",
        NULL
    };

    if (!env) {
        pr_debug("[%s:%d]pcm ENOMEM\n",__func__,__LINE__);
        return;
    }

    if (add_uevent_var(env, "ACTION=%s", (pcm_event->action) ? "stop" : "start")) {
        pr_debug("[%s:%d]pcm ENOMEM\n",__func__,__LINE__);
        kfree(env);
        return;
    }

    call_usermodehelper(argv[0], argv, env->envp, UMH_WAIT_EXEC);
    kfree(env);
}
#endif

#ifdef PURIN_DEBUG
static void purin_print_trigger_cmd(int cmd)
{
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
        pr_debug("SNDRV_PCM_TRIGGER_START\n");
        break;
    case SNDRV_PCM_TRIGGER_STOP:
        pr_debug("SNDRV_PCM_TRIGGER_STOP\n");
        break;
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        pr_debug("SNDRV_PCM_TRIGGER_PAUSE_PUSH\n");
        break;
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        pr_debug("SNDRV_PCM_TRIGGER_PAUSE_RELEASE\n");
        break;
    case SNDRV_PCM_TRIGGER_SUSPEND:
        pr_debug("SNDRV_PCM_TRIGGER_SUSPEND\n");
        break;
    case SNDRV_PCM_TRIGGER_RESUME:
        pr_debug("SNDRV_PCM_TRIGGER_RESUME\n");
        break;
    default:
        pr_debug("NO SUCH COMMAND\n");
    }
}
#endif

static int purin_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    int ret = 0;
    unsigned long irqflags;

    #ifdef PURIN_DEBUG
    purin_print_trigger_cmd(cmd);
    #endif

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
        purin_rtd->active = 1;
        purin_rtd->hw_ptr = runtime->status->hw_ptr;
        dma_tx_rx_prepare_for_trigger(substream);

        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            spin_lock_irqsave(&tx_dma_before_open.lock, irqflags);
        else
            spin_lock_irqsave(&rx_dma_before_open.lock, irqflags);

        pcm_open_status_flag |= (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
                                    PCM_OPEN_STATUS_PLAYBACK : PCM_OPEN_STATUS_CAPTURE;

        if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            spin_unlock_irqrestore(&tx_dma_before_open.lock, irqflags);
        else
            spin_unlock_irqrestore(&rx_dma_before_open.lock, irqflags);
#else
        spin_lock_irqsave(&purin_rtd->lock, irqflags);
        purin_rtd->active = 1;
        if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
            /* gpio control */
            #ifdef CONFIG_PCMGPIOCONTROL
                purin_rtd->pcm_event.action = 0;
                schedule_delayed_work(&purin_rtd->pcm_event.gpioctrl, 1);
                pr_debug("%s:%d requested stream startup \n", __func__, __LINE__);
            #endif

            #if defined (CFG_IO_AUDIO_AMP_POWER)
                if (!gpio_status) {
                    pr_debug("%s:%d GPIO%d Audio AMP Power On\n", __func__, __LINE__, AUDIO_AMP_POWER);
                    gpio_set_value(AUDIO_AMP_POWER, AUDIO_AMP_ACTIVE_LEVEL);
                }
            #endif

            purin_rtd->hw_ptr = runtime->status->hw_ptr;
            dma_tx_new_period(substream);
            dma_tx_new_period(substream);
        } else{
            dma_rx_new_period(substream);
            dma_rx_new_period(substream);
        }
        spin_unlock_irqrestore(&purin_rtd->lock, irqflags);
#endif
        break;
    case SNDRV_PCM_TRIGGER_STOP:
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
#ifdef DBG_PCMWB_UNDERRUN
        printk(KERN_DEBUG "SNDRV_PCM_TRIGGER_STOP stream %d\n\n\n", substream->stream);
#endif
        if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            spin_lock_irqsave(&tx_dma_before_open.lock, irqflags);
        else
            spin_lock_irqsave(&rx_dma_before_open.lock, irqflags);

        pcm_open_status_flag &= (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
                                    (~PCM_OPEN_STATUS_PLAYBACK) : (~PCM_OPEN_STATUS_CAPTURE);
        audio_stop_dma(substream);

        if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            spin_unlock_irqrestore(&tx_dma_before_open.lock, irqflags);
        else
            spin_unlock_irqrestore(&rx_dma_before_open.lock, irqflags);
#else
        {
            spin_lock_irqsave(&purin_rtd->lock, irqflags);
            /* requested stream shutdown */
            audio_stop_dma(substream);
            spin_unlock_irqrestore(&purin_rtd->lock, irqflags);

            /* gpio control */
            #ifdef CONFIG_PCMGPIOCONTROL
                pr_debug("%s:%d requested stream stutdown \n", __func__, __LINE__);
                purin_rtd->pcm_event.action = 1;
                schedule_delayed_work(&purin_rtd->pcm_event.gpioctrl, 1);
            #endif

            #if defined (CFG_IO_AUDIO_AMP_POWER)
                if (!gpio_status) {
                    pr_debug("%s:%d GPIO%d Audio AMP Power Off\n", __func__, __LINE__, AUDIO_AMP_POWER);
                    gpio_set_value(AUDIO_AMP_POWER, !AUDIO_AMP_ACTIVE_LEVEL);
                }
            #endif
        }
#endif
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}


/**
 * determine the current position of the DMA transfer
 * called by snd_pcm_period_elapsed()
 */
static snd_pcm_uframes_t purin_pcm_pointer(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct purin_runtime_data *purin_rtd = runtime->private_data;
    unsigned int offset = 0;

    offset = (runtime->period_size * (purin_rtd->periods));
    if (offset >= runtime->buffer_size) {
        pr_debug("runtime:periods=%d \nperiod_size=%d \nbuffer_size=%d \n offset=%d\n",
                 runtime->periods, (int)runtime->period_size, (int)runtime->buffer_size,
                 offset);
        offset = 0;
    }

    #if 0
    /* large noise print */
    pr_debug("runtime:periods=%d, period_size=%d, buffer_size=%d \n",
             runtime->periods, (int)runtime->period_size, (int)runtime->buffer_size);
    pr_debug("purin_rtd:periods=%d, period=%d \n", purin_rtd->periods, purin_rtd->period);
    pr_debug("pointer offset %d\n", offset);
    #endif

    return offset;
}

/**
 *  enable MMAP chagnes the behavior which alsa lib wirtes dma
 *  buffer. use MMAP could improve this driver's performance.
 */
#if 0
static int purin_pcm_mmap(struct snd_pcm_substream *substream,
    struct vm_area_struct *vma)
{
    return remap_pfn_range(vma, vma->vm_start,
               substream->dma_buffer.addr >> PAGE_SHIFT,
               vma->vm_end - vma->vm_start, vma->vm_page_prot);
/*
#if 1
    return remap_pfn_range(vma, vma->vm_start,
               substream->dma_buffer.addr >> PAGE_SHIFT,
               vma->vm_end - vma->vm_start, vma->vm_page_prot);
#elif 1
    struct snd_pcm_runtime *runtime = substream->runtime;
    unsigned long off = vma->vm_pgoff;

#define __phys_to_pfn(paddr)    ((paddr) >> PAGE_SHIFT)
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    return remap_pfn_range(vma, vma->vm_start,
        __phys_to_pfn(runtime->dma_addr) + off,
        vma->vm_end - vma->vm_start, vma->vm_page_prot);
#else
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct page *pg;
    unsigned long addr;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    addr = plat_dma_addr_to_phys(substream->pcm->card->dev, runtime->dma_addr);
    runtime->dma_area = phys_to_virt(addr);
    pg = virt_to_page(runtime->dma_area);

    return remap_pfn_range(vma, vma->vm_start, page_to_pfn(pg) + vma->vm_pgoff,
                    runtime->dma_bytes, vma->vm_page_prot);
#endif
*/
}
#endif

#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
static struct snd_pcm_ops purin_external_ops = {
    .open       = purin_pcm_external_open,
    .close      = purin_pcm_close,
    .ioctl      = snd_pcm_lib_ioctl,
    .hw_params  = purin_pcm_hw_params,
    .hw_free    = purin_pcm_hw_free,
    .prepare    = purin_pcm_prepare,
    .trigger    = purin_pcm_trigger,
    .pointer    = purin_pcm_pointer,
    .mmap       = snd_pcm_lib_mmap_iomem,
};
#endif

#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static struct snd_pcm_ops purin_internal_ops = {
    .open       = purin_pcm_internal_open,
    .close      = purin_pcm_close,
    .ioctl      = snd_pcm_lib_ioctl,
    .hw_params  = purin_pcm_hw_params,
    .hw_free    = purin_pcm_hw_free,
    .prepare    = purin_pcm_prepare,
    .trigger    = purin_pcm_trigger,
    .pointer    = purin_pcm_pointer,
    .mmap       = snd_pcm_lib_mmap_iomem,
};
#endif

static struct snd_pcm_ops purin_spdif_ops = {
    .open       = purin_pcm_internal_open,
    .close      = purin_pcm_close,
    .ioctl      = snd_pcm_lib_ioctl,
    .hw_params  = purin_pcm_spdif_hw_params,
    .hw_free    = purin_pcm_spdif_hw_free,
    .prepare    = purin_pcm_prepare,
    .trigger    = purin_pcm_trigger,
    .pointer    = purin_pcm_pointer,
    .mmap       = snd_pcm_lib_mmap_iomem,
};

static int purin_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
    struct snd_pcm_substream *substream = pcm->streams[stream].substream;
    struct snd_dma_buffer *buf = &substream->dma_buffer;
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    size_t size;

    if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        size = purin_pcm_tx_hardware.buffer_bytes_max;
    else
        size = purin_pcm_hardware.buffer_bytes_max;
#else
    size_t size = purin_pcm_hardware.buffer_bytes_max;
#endif

    buf->dev.type = SNDRV_DMA_TYPE_DEV;
    buf->dev.dev  = pcm->card->dev;

    buf->area = dma_alloc_coherent(pcm->card->dev, size,
                                   &buf->addr, GFP_KERNEL);
    buf->private_data = NULL;

    if (unlikely(!buf->area))
        return -ENOMEM;

    buf->bytes = size;

    pr_debug("buf->area=0x%08x, buf->addr=0x%08x, size %d\n",
             (unsigned int)buf->area,
             (unsigned int)buf->addr,
             size);

    return 0;
}

static u64 purin_dma_dmamask = DMA_BIT_MASK(32);
static int purin_soc_dma_new(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_card *card = rtd->card->snd_card;
    struct snd_pcm *pcm = rtd->pcm;
    int ret = 0;
    int stream;

    /* tell the driver that we only support 32bit addr_phy */
    if (!card->dev->dma_mask)
        card->dev->dma_mask = &purin_dma_dmamask;
    if (!card->dev->coherent_dma_mask)
        card->dev->coherent_dma_mask = DMA_BIT_MASK(32);


    /* prealloc dma buffer for playback and record(capture) */
    for (stream = 0; stream <= SNDRV_PCM_STREAM_LAST; stream++) {
        if (!pcm->streams[stream].substream)
            continue;

        ret = purin_pcm_preallocate_dma_buffer(pcm, stream);
        if (unlikely(ret))
            goto out;
    }

#if defined (CFG_IO_AUDIO_AMP_POWER)
    if (!(gpio_status = gpio_request(AUDIO_AMP_POWER, "purin_amp_en")))
        gpio_direction_output(AUDIO_AMP_POWER, !AUDIO_AMP_ACTIVE_LEVEL);
    else
        printk(KERN_ERR "ERR: request gpio[%d] for Audio AMP Power\n", AUDIO_AMP_POWER);
#endif

    spin_lock_init(&(ai_pcm.pcm_lock));

out:
    return ret;
}

static void purin_soc_dma_free(struct snd_pcm *pcm)
{
    int stream;
    struct snd_pcm_substream *substream;
    struct snd_dma_buffer *buf;

#if defined (CFG_IO_AUDIO_AMP_POWER)
    if (!gpio_status) {
        gpio_set_value(AUDIO_AMP_POWER, !AUDIO_AMP_ACTIVE_LEVEL);
        gpio_free(AUDIO_AMP_POWER);
        gpio_status = -1;
    }
#endif

    /* release dma buffer for playback and record(capture) */
    for (stream = 0; stream <= SNDRV_PCM_STREAM_LAST; stream++) {
        substream = pcm->streams[stream].substream;
        buf = &(substream->dma_buffer);
        if (buf->area == NULL) {
            continue;
        }
        dma_free_coherent(pcm->card->dev, buf->bytes,
                          buf->area, buf->addr);

        buf->area = NULL;
        buf->bytes = 0;
    }
}

#if !defined(CONFIG_PANTHER_SND_PCM0_NONE)
/*
 * about pcm/i2s for external codec
 */
static struct snd_soc_platform_driver panther_external_platform = {
    .ops        = &purin_external_ops,
    .pcm_new    = purin_soc_dma_new,
    .pcm_free   = purin_soc_dma_free,
};

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static void initialize_dma_before_open(struct device *dev)
{
    int i = 0;
    u16 *data_ptr;
    spin_lock_init(&tx_dma_before_open.lock);
    tx_dma_before_open.desc_ready_idx = 0;
    tx_dma_before_open.buffer_info.offset = 0;
    tx_dma_before_open.buffer_info.shift_size = PURIN_DESC_BUFSIZE;
    tx_dma_before_open.buffer_info.total_size = PURIN_DESC_BUFSIZE;
    tx_dma_before_open.buffer_info.data_addr = dma_alloc_coherent(dev,
                                                                  tx_dma_before_open.buffer_info.total_size,
                                                                  &tx_dma_before_open.buffer_info.data_addr_phy,
                                                                  GFP_KERNEL);
    memset(tx_dma_before_open.buffer_info.data_addr, 0xff, tx_dma_before_open.buffer_info.total_size);
    pr_debug("[tx]data_addr=0x%08x, data_addr_phy=0x%08x\n",
             (unsigned int)tx_dma_before_open.buffer_info.data_addr,
             (unsigned int)tx_dma_before_open.buffer_info.data_addr_phy);
#ifdef DBG_PCMWB_UNDERRUN
    printk(KERN_DEBUG "[tx]fake_data_addr=0x%08x, fake_data_addr_phy=0x%08x\n",
             (unsigned int)tx_dma_before_open.buffer_info.data_addr,
             (unsigned int)tx_dma_before_open.buffer_info.data_addr_phy);
#endif
    tx_dma_before_open.dma_desc_array = dma_alloc_coherent(dev,
                                                           PURIN_DESC_SIZE,
                                                           &tx_dma_before_open.dma_desc_array_phys,
                                                           GFP_KERNEL);
#ifdef DBG_PCMWB_UNDERRUN
    printk(KERN_DEBUG "[tx]dma_desc_array=0x%08x, dma_desc_array_phys=0x%08x\n",
             (unsigned int)tx_dma_before_open.dma_desc_array,
             (unsigned int)tx_dma_before_open.dma_desc_array_phys);
#endif
    pr_debug("[tx]dma_desc_array=0x%08x, dma_desc_array_phys=0x%08x\n",
             (unsigned int)tx_dma_before_open.dma_desc_array,
             (unsigned int)tx_dma_before_open.dma_desc_array_phys);

#if 0   // for testing time slot
    data_ptr = (u16*)tx_dma_before_open.buffer_info.data_addr;
    for(i = 0; i <= (tx_dma_before_open.buffer_info.total_size/2 - 8);)
    {
        data_ptr[i] = 0;
        data_ptr[i+1] = 0;
        data_ptr[i+2] = 0xffff;
        data_ptr[i+3] = 0xffff;
        data_ptr[i+4] = 0xffff;
        data_ptr[i+5] = 0xffff;
        data_ptr[i+6] = 0;
        data_ptr[i+7] = 0;
        i += 8;
    }
#endif

    pcm_44100_dmabuffer.offset = 0;
    pcm_44100_dmabuffer.total_size = PURIN_DESC_BUFSIZE * PURIN_DESC_NUM;
    pcm_44100_dmabuffer.shift_size = PURIN_DESC_BUFSIZE;
    pcm_44100_dmabuffer.data_addr = dma_alloc_coherent(dev,
                                                       pcm_44100_dmabuffer.total_size,
                                                       &pcm_44100_dmabuffer.data_addr_phy,
                                                       GFP_KERNEL);
#ifdef DBG_PCMWB_UNDERRUN
    for(i = 0; i < PURIN_DESC_NUM; ++i)
    {
        printk(KERN_DEBUG "[tx]real_data_addr=0x%08x, real_data_addr_phy=0x%08x\n",
                 (unsigned int)(pcm_44100_dmabuffer.data_addr + PURIN_DESC_BUFSIZE * i),
                 (unsigned int)(pcm_44100_dmabuffer.data_addr_phy + PURIN_DESC_BUFSIZE * i));
    }
    printk(KERN_DEBUG "[tx]real_data_addr_end=0x%08x, real_data_addr_end_phy=0x%08x\n",
                 (unsigned int)(pcm_44100_dmabuffer.data_addr + pcm_44100_dmabuffer.total_size),
                 (unsigned int)(pcm_44100_dmabuffer.data_addr_phy + pcm_44100_dmabuffer.total_size));
#endif
    //memset(pcm_44100_dmabuffer.data_addr, 0xff, PURIN_DESC_BUFSIZE * PURIN_DESC_NUM);
    data_ptr = (u16*)pcm_44100_dmabuffer.data_addr;
    for(i = 0; i <= (pcm_44100_dmabuffer.total_size/2 - 8);)
    {
#if 0   // for testing time slot
        data_ptr[i] = 0x0;
        data_ptr[i+1] = 0x0;
        data_ptr[i+2] = 0xffff;
        data_ptr[i+3] = 0xffff;
        data_ptr[i+4] = 0xffff;
        data_ptr[i+5] = 0xffff;
        data_ptr[i+6] = 0x0;
        data_ptr[i+7] = 0x0;
#else
        data_ptr[i] = 0xffff;
        data_ptr[i+1] = 0xffff;
        data_ptr[i+2] = 0xaaaa;
        data_ptr[i+3] = 0xaaaa;
        data_ptr[i+4] = 0xaaaa;
        data_ptr[i+5] = 0xaaaa;
        data_ptr[i+6] = 0xffff;
        data_ptr[i+7] = 0xffff;
#endif
        i += 8;
    }

    spin_lock_init(&rx_dma_before_open.lock);
    rx_dma_before_open.desc_ready_idx = 0;
    rx_dma_before_open.buffer_info.offset = 0;
    rx_dma_before_open.buffer_info.shift_size = PURIN_DESC_BUFSIZE;
    rx_dma_before_open.buffer_info.total_size = PURIN_DESC_BUFSIZE;
    rx_dma_before_open.buffer_info.data_addr = dma_alloc_coherent(dev,
                                                                  rx_dma_before_open.buffer_info.total_size,
                                                                  &rx_dma_before_open.buffer_info.data_addr_phy,
                                                                  GFP_KERNEL);
    pr_debug("[rx]data_addr=0x%08x, data_addr_phy=0x%08x\n",
             (unsigned int)rx_dma_before_open.buffer_info.data_addr,
             (unsigned int)rx_dma_before_open.buffer_info.data_addr_phy);
    rx_dma_before_open.dma_desc_array = dma_alloc_coherent(dev,
                                                           PURIN_DESC_SIZE,
                                                           &rx_dma_before_open.dma_desc_array_phys,
                                                           GFP_KERNEL);
    pr_debug("[rx]dma_desc_array=0x%08x, dma_desc_array_phys=0x%08x\n",
             (unsigned int)rx_dma_before_open.dma_desc_array,
             (unsigned int)rx_dma_before_open.dma_desc_array_phys);
#ifdef DBG_PCMWB_UNDERRUN
    printk(KERN_DEBUG "[rx]dma_desc_array=0x%08x, dma_desc_array_phys=0x%08x\n",
             (unsigned int)rx_dma_before_open.dma_desc_array,
             (unsigned int)rx_dma_before_open.dma_desc_array_phys);
    hrtimer_init(&tsf_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    tsf_timer.function = check_irq_timer;
#endif
}
#endif

static int panther_external_platform_probe(struct platform_device *pdev)
{
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    int ret = snd_soc_register_platform(&pdev->dev, &panther_external_platform);
    if(ready_before_open == 0 && ret == 0)
    {
        initialize_dma_before_open(&pdev->dev);
        ready_before_open = 1;
        pcm_open_status_flag = 0;
        hrtimer_init(&recovery_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        recovery_timer.function = recovery_timer_func;
    }
    return ret;
#else
    return snd_soc_register_platform(&pdev->dev, &panther_external_platform);
#endif
}

static int panther_external_platform_remove(struct platform_device *pdev)
{
    snd_soc_unregister_platform(&pdev->dev);
    return 0;
}

static struct platform_driver panther_external_dma_platform = {
    .driver = {
            .name = "purin-external-dma",
    },
    .probe = panther_external_platform_probe,
    .remove = panther_external_platform_remove,
};

module_platform_driver(panther_external_dma_platform);
#endif

/*
 *   about spdif
 */
static struct snd_soc_platform_driver panther_spdif_platform = {
    .ops        = &purin_spdif_ops,
    .pcm_new    = purin_soc_dma_new,
    .pcm_free   = purin_soc_dma_free,
};

static int panther_spdif_platform_probe(struct platform_device *pdev)
{
    return snd_soc_register_platform(&pdev->dev, &panther_spdif_platform);
}

static int panther_spdif_platform_remove(struct platform_device *pdev)
{
    snd_soc_unregister_platform(&pdev->dev);
    return 0;
}

static struct platform_driver panther_spdif_dma_platform = {
    .driver = {
            .name = "purin-spdif-dma",
    },
    .probe = panther_spdif_platform_probe,
    .remove = panther_spdif_platform_remove,
};
module_platform_driver(panther_spdif_dma_platform);

/*
 * about pcm/i2s for internal adc/dac
 */
#if !defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
static struct snd_soc_platform_driver panther_internal_platform = {
    .ops        = &purin_internal_ops,
    .pcm_new    = purin_soc_dma_new,
    .pcm_free   = purin_soc_dma_free,
};

static int panther_internal_platform_probe(struct platform_device *pdev)
{
    #if (DETECT_TX_ALL_ZERO | ENABLE_NETLINK)
    init_netlink();
    #endif
    #if defined(AUDIO_RECOVERY)
    hrtimer_init(&recovery_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    recovery_timer.function = recovery_timer_func;
    #endif
    return snd_soc_register_platform(&pdev->dev, &panther_internal_platform);
}

static int panther_internal_platform_remove(struct platform_device *pdev)
{
    #if (DETECT_TX_ALL_ZERO | ENABLE_NETLINK)
    finalize_netlink();
    #endif
    snd_soc_unregister_platform(&pdev->dev);
    return 0;
}
#endif

static struct platform_driver panther_internal_dma_platform = {
    .driver = {
            .name = "purin-internal-dma",
    },
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
    .probe = panther_external_platform_probe,
    .remove = panther_external_platform_probe,
#else
    .probe = panther_internal_platform_probe,
    .remove = panther_internal_platform_remove,
#endif
};
module_platform_driver(panther_internal_dma_platform);

/*
 *
 */
MODULE_AUTHOR("Edden Tsai");
MODULE_DESCRIPTION("Panther AUDIO DMA module");
MODULE_LICENSE("GPL");
