
#ifndef _PURIN_PCM_H
#define _PURIN_PCM_H

struct purin_dma_desc {
    u32 info;
    void * dptr;
};

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)

struct purin_dma_buffer_info {
    u8         *data_addr;          /* uncached addr */
    dma_addr_t data_addr_phy;       /* physical address */
    u32        shift_size;          /* how many bytes we shuild shift after each isr */
    u32        offset;              /* current offset in this dma buffer */
    u32        total_size;          /* total size of this dma buffer */
};

struct purin_preopen_info {
    struct purin_dma_buffer_info buffer_info;
    struct purin_dma_desc *dma_desc_array;  /* descriptor address */
    dma_addr_t dma_desc_array_phys;         /* physical descriptor address */
    int desc_ready_idx;                     /* current idx for descritor */
    spinlock_t lock;
};

#endif

struct purin_runtime_data {
    struct purin_device *device;
    int active;
    /* index for panther to prepare descriptor */
    int period;
    /* index for alsa to read/write data, should be the number of descriptor that change owner from hw -> sw*/
    int periods;

    spinlock_t lock;

    /* first index can be prepared */
    int next;
    /* init codec state flag for waiting codec CJC8988 */
    int codec_init;

    /* hw ptr */
    snd_pcm_uframes_t hw_ptr;
    /* tx/rx descriptor base physical address */
    dma_addr_t dma_desc_array_phys;
    /* tx/rx descriptor base address */
    struct purin_dma_desc *dma_desc_array;

    /* about spdif & pcmwb */
    unsigned char   *dataxfer_addr;            /* memory to deal with 16bit data in spdif & pcmwb */
    dma_addr_t      dataxfer_addr_phy;         /* physical address of dataxfer_addr */
    int dataxfer_handle_idx;
    int dataxfer_multiple;
    int dma_type;
    int is_adc;

#ifdef CONFIG_PCMGPIOCONTROL
    struct purin_pcm_event pcm_event;
#endif

};

#define PURIN_BUF_UNIT          32
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
#define PURIN_BUFSIZE           80 // equal 1280 Samples, 10 ms between interrupts
#define PURIN_DESC_NUM          25 // prepared 250 ms in queue when tx/rx enable at the same time
#else
#define PURIN_BUFSIZE           176/4
#define PURIN_DESC_NUM          2

#if defined(CONFIG_PANTHER_SUPPORT_I2S_48K) || defined(CONFIG_PANTHER_SND_PCM0_ES8388) || defined(CONFIG_PANTHER_SND_PCM1_ES8388)
#undef  PURIN_BUFSIZE
#define PURIN_BUFSIZE           120
#endif

#endif
#define MULTIPLE_FOR_ALSA       8*4

#define PURIN_DESC_BUFSIZE      (PURIN_BUFSIZE * PURIN_BUF_UNIT)    // buffer size of one decriptor
#define PURIN_DESC_SIZE         (PURIN_DESC_NUM * sizeof(struct purin_dma_desc))
#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
#define TOTAL_BUFSIZE_FOR_ALSA  PURIN_DESC_BUFSIZE * MULTIPLE_FOR_ALSA
#else
#define TOTAL_BUFSIZE_FOR_ALSA  (PURIN_DESC_BUFSIZE * PURIN_DESC_NUM) * MULTIPLE_FOR_ALSA
#endif

#if defined(CONFIG_PANTHER_SND_PCM0_PCMWB_44100_TX_ENABLE)
#define PCMWB_REAL_BYTE_SIZE_44100_INDESC   (882 * 2)
#define PCMWB_BUFFER_SIZE_44100             PCMWB_REAL_BYTE_SIZE_44100_INDESC * MULTIPLE_FOR_ALSA
#endif

#ifdef CONFIG_PCMGPIOCONTROL

struct purin_pcm_event{
    int action;                    // start:0, stop:1;
    struct delayed_work gpioctrl;
};

#endif

#if defined (CONFIG_GPIO_AUDIO_AMP_POWER) && (CONFIG_GPIO_AUDIO_AMP_POWER_NUM)

    #define CFG_IO_AUDIO_AMP_POWER CONFIG_GPIO_AUDIO_AMP_POWER_NUM
    #define AUDIO_AMP_POWER CFG_IO_AUDIO_AMP_POWER

    #if defined (CONFIG_GPIO_AUDIO_AMP_POWER_LOW_ACTIVE)
        #define AUDIO_AMP_ACTIVE_LEVEL 0
    #else
        #define AUDIO_AMP_ACTIVE_LEVEL 1
    #endif

    int purin_spk_on(int enable);

#endif

void purin_desc_specific(struct snd_pcm_substream *substream);

#if defined(CONFIG_SND_ALOOP)
void start_update_pos_timer(unsigned int period_byte_size);
#endif

#define AUDIO_RECOVERY

#endif
