#ifndef __DRAGONITE_MAIN_H__
#define __DRAGONITE_MAIN_H__
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#include <linux/version.h>
#include <net/mac80211.h>
#include <mac.h>
#include <wla_cfg.h>

#if defined (DRAGONITE_ACQ_DELAY_KICK_SUPPORT)
#define DRAGONITE_ACQ_DELAY_KICK
#endif

/* config BUS arbitrator to have DBUS(for AXI masters) priority higher than CPU/OCP */
//#define DRAGONITE_DBUS_PRIORITY_OVER_CPU

/* limit CPU pending bus read/write requests */
//#define DRAGONITE_LIMIT_CPU_PENDING_BUS_RW_REQUESTS

/* trigger recovery on consecutive beacon loss */
#define DRAGONITE_TRIGGER_RECOVERY_ON_BEACON_LOSS

/* output mlme debug message and beacon loss debug infomation */
#define DRAGONITE_MLME_DEBUG
    #define BEACON_RX_HISTORY_DEPTH   10

#define DRAGONITE_MACREG_ACCESS_TRACE
#define DRAGONITE_POWERSAVE_AGGRESSIVE
    #define MS_DELAY_BEFORE_GATE_WIFIMAC_CLOCK	10000
#define DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE
#define DRAGONITE_THROUGHPUT_IMPROVE
#define DRAGONITE_HW_DEBUG_REGS_DUMP
#define DRAGONITE_BEACON_CHECKER
#define DRAGONITE_BEACON_IN_ISR
#define DRAGONITE_ACQ_BAN
#define DRAGONITE_ISR_CHECK
#define DRAGONITE_TX_SKB_CHECKER
#define DRAGONITE_TX_BHDR_CHECKER
#define DRAGONITE_TX_HANG_CHECKER
#define DRAGONITE_TX_BCQ_TAIL_DROP
#define DRAGONITE_TX_BCQ_QSIZE_LIMIT 30
#define DRAGONITE_ENABLE_OMNICFG
#define DRAGONITE_ENABLE_AIRKISS
#define DRAGONITE_BB_DUMP
/*
   if defined DRAGONITE_MESH_TX_GC_HW_ENCRYPT
     802.11s group-cast TX uses hardware encrypt, groupcast RX uses software decrypt
   if not defined DRAGONITE_MESH_TX_GC_HW_ENCRYPT
     802.11s group-cast TX/RX all use software encrypt/decrypt
*/
#define DRAGONITE_MESH_TX_GC_HW_ENCRYPT

#if defined(DRAGONITE_MLME_DEBUG)
#if !defined(CONFIG_MAC80211_MLME_DEBUG) || !defined(CONFIG_MAC80211_STA_DEBUG)
#error please enable CONFIG_MAC80211_MLME_DEBUG and CONFIG_MAC80211_STA_DEBUG
#endif
#endif

#define MAX_2GHZ_CHANNELS	16
#define MAX_5GHZ_CHANNELS	32
#define DRAGONITE_RATE_MAX	32
#define DRAGONITE_MAX_CHANNELS	14

#define TID_MAX             8

#define SKB_LIST_NAME_LEN_MAX   25

#define DRAGONITE_AMPDU_JUMP_OVER_SSN 2000

#define COUNT_TX_PER_AC true

struct skb_list_head {
	volatile struct list_head *qhead;
	volatile int qcnt;
};

enum {
	QUEUE_FROM_DATA,
	QUEUE_FROM_VP,
	QUEUE_FROM_SP,
};
struct queue_param {
	char queue_name[SKB_LIST_NAME_LEN_MAX];
	int queue_located;
	bool use_tid;
	bool use_acqid;
};

#define DRG_TX_STATS_DEFAULT_TIME_SPACE 20 /* jiffies */
struct _drg_tx_stats {
	int stats_enable;
	bool stats_start;
	unsigned long time_space;
	unsigned long jiffies_pre;
	u32 tx_count_pre;
	u32 tx_count;
	u32 tx_done_count_pre;
	u32 tx_done_count;
};
extern struct _drg_tx_stats drg_tx_stats;

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
/* POWERSAVE FLAGS */
#define PS_WAIT_FOR_BEACON       BIT(0)
#define PS_WAIT_FOR_MULTICAST    BIT(1)
#define PS_FORCE_WAKE_UP         BIT(2)
#define PS_FORCE_WAKE_UP_UNTIL   BIT(3)
#endif
struct mac80211_dragonite_data {
	struct list_head list;
	struct ieee80211_hw *hw;
	struct device *dev;
	struct ieee80211_supported_band bands[IEEE80211_NUM_BANDS];
	enum ieee80211_band curr_band;
	u16 curr_center_freq;
	struct ieee80211_channel channels_2ghz[MAX_2GHZ_CHANNELS];
	struct survey_info survey[DRAGONITE_MAX_CHANNELS];
	struct ieee80211_rate rates[DRAGONITE_RATE_MAX];

	struct mac_address addresses[2];

	struct ieee80211_channel *tmp_chan;
	struct delayed_work roc_done;
	struct delayed_work hw_scan;
	struct cfg80211_scan_request *hw_scan_request;
	struct ieee80211_vif *hw_scan_vif;
	int scan_chan_idx;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	u8 scan_addr[ETH_ALEN];
#endif

	struct ieee80211_channel *channel;
	struct cfg80211_chan_def chandef;
	u16 channel_number;
	u32 primary_ch_offset;               /* channel offset setting for sending on 20Mhz-primary channel */
	u64 beacon_int	/* beacon interval in us */;
	u8 dtim_period;
	bool enable_beacon;
	unsigned int rx_filter;
	volatile bool started, isr_handle;
	bool idle, scanning;
	struct mutex mutex;
	struct tasklet_hrtimer beacon_timer;
	enum ps_mode {
		PS_DISABLED, PS_ENABLED, PS_AUTO_POLL, PS_MANUAL_POLL
	} ps;
	bool ps_poll_pending;
	struct dentry *debugfs;
	struct dentry *debugfs_ps;

	struct sk_buff_head pending;	/* packets pending */
	/*
	 * Only radios in the same group can communicate together (the
	 * channel has to match too). Each bit represents a group. A
	 * radio can be in more then one group.
	 */
	u64 group;
	struct dentry *debugfs_group;

	int power_level;

	/* difference between this hw's clock and the real clock, in usecs */
	s64 tsf_offset;
	s64 bcn_delta;
	/* absolute beacon transmission time. Used to cover up "tx" delay. */
	u64 abs_bcn_ts;

	MAC_INFO* mac_info;
        struct task_struct *dragonited_task;
	struct tasklet_struct irq_tasklet;
	u8 beacon_pending_tx_bitmap;

	struct __bss_info dragonite_bss_info[MAC_MAX_BSSIDS];
	int bssid_count;

	volatile u32 vif_priv_addr[MAX_BSSIDS];
	volatile u32 sta_priv_addr[MAX_STA_NUM];

#if defined (DRAGONITE_TX_SKB_CHECKER)
	/*skb queue*/
	struct skb_list_head tx_skb_queue[1];
#endif
    struct skb_list_head tx_sw_free_queue[1];//skb wait for free queue/

    /*sw group queue*/
    struct skb_list_head tx_sw_mixed_queue[ACQ_NUM-1]; //per qid mixed queue

    /*hw group queue*/
    struct skb_list_head tx_hw_mixed_queue[ACQ_NUM-1]; //per qid mixed queue

    /*sw ps queue*/
    struct skb_list_head tx_sw_ps_mixed_queue[ACQ_NUM-1]; //per qid mixed queue

    volatile int tx_sw_queue_total_count;
    volatile int tx_sw_bmc_queue_total_count;
    volatile int tx_per_ac_count[ACQ_NUM-1];
    spinlock_t tx_per_ac_count_lock;

    spinlock_t drg_txq_lock;
    unsigned long drg_txq_lock_irqflags;

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
    bool ps_enabled;
    bool poweroff;
    u8 ps_flags;
    spinlock_t pm_lock;
    unsigned long pm_lock_irqflags;
#if defined (DRAGONITE_POWERSAVE_AGGRESSIVE)
    struct hrtimer tsf_timer;
    struct hrtimer ps_sleep_full_timer;
#endif
#endif
};

#define NORMAL_SSN(_priv) (_priv->ssn)

struct dragonite_vif_priv {
        volatile u32 magic;
        u8 bssid[ETH_ALEN];
        int bssid_idx;
        bool assoc;
        u16 aid;
        struct __bss_info *bssinfo;
        enum nl80211_iftype type;

	bool ts_err_mute;
	unsigned long ts_err_jiffies;
	int ts_err_count;

        u32 tsc[4];
        int rekey_id;

        spinlock_t vplock;
        u16 ssn;

	/*sw private queue*/
	struct skb_list_head tx_sw_gc_queue[ACQ_NUM-1]; //per bss per groupcast queue
	/*sw ps queue*/
	struct skb_list_head tx_sw_ps_gc_queue; //per bss per bcq
	/*hw ps queue*/
	struct skb_list_head tx_hw_ps_gc_queue; //per bss per bcq

	struct skb_list_head tx_sw_queue[TID_MAX+1]; //per bss tid mgmt station
	struct skb_list_head tx_sw_mgmt_queue[TID_MAX+1]; //per bss tid mgmt station
	struct skb_list_head tx_hw_mgmt_queue[TID_MAX+1]; //per bss tid mgmt station
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
	bool tx_pm_bit;
	unsigned long tx_pm_bit_prev_jiffies;
#endif
};

#define IS_AMPDU(sp) (sp->drg_tx_aggr.is_aggr)
#define SINGLE_Q(sp, tid) (sp->drg_tx_aggr.sgl_q[tid])
#define AGGR_SSN(sp, tid) (sp->drg_tx_aggr.ssn[tid])
#define PREV_AGGR_SSN(sp, tid) (sp->drg_tx_aggr.prev_ssn[tid])

struct drg_aggr_mgmt {
	bool is_aggr;
    	acq *sgl_q[TID_MAX+1];
	u16 ssn[TID_MAX+1];
	u16 prev_ssn[TID_MAX+1];
};
struct dragonite_sta_priv {
	volatile u32 magic;

        u8 mac_addr[ETH_ALEN];
	sta_basic_info sta_binfo;
	int stacap_index;
	u32 addr_index;
	u32 tsc[4];
	int rekey_id;
	bool sta_mode;
        spinlock_t splock;

	struct drg_aggr_mgmt drg_tx_aggr;

	u16 ssn;

	struct skb_list_head tx_sw_queue[TID_MAX+1]; //per ra tid queue
	struct skb_list_head tx_sw_single_queue[TID_MAX+1]; //per ra tid single queue
	struct skb_list_head tx_hw_single_queue[TID_MAX+1]; //per ra tid single queue
	struct skb_list_head tx_sw_ps_single_queue[TID_MAX+1]; //per ra tid single queue

	int tx_hw_pending_skb_count;
    int tx_ps_buffer_skb_count;
};

enum dragonite_tx_control_flags {
	DRAGONITE_TX_CTL_REQ_TX_STATUS		= BIT(0),
	DRAGONITE_TX_CTL_NO_ACK			= BIT(1),
	DRAGONITE_TX_STAT_ACK			= BIT(2),
};

enum {
	DRAGONITE_CMD_UNSPEC,
	DRAGONITE_CMD_REGISTER,
	DRAGONITE_CMD_FRAME,
	DRAGONITE_CMD_TX_INFO_FRAME,
	__DRAGONITE_CMD_MAX,
};
#define DRAGONITE_CMD_MAX (_DRAGONITE_CMD_MAX - 1)

enum {
	DRAGONITE_ATTR_UNSPEC,
	DRAGONITE_ATTR_ADDR_RECEIVER,
	DRAGONITE_ATTR_ADDR_TRANSMITTER,
	DRAGONITE_ATTR_FRAME,
	DRAGONITE_ATTR_FLAGS,
	DRAGONITE_ATTR_RX_RATE,
	DRAGONITE_ATTR_SIGNAL,
	DRAGONITE_ATTR_TX_INFO,
	DRAGONITE_ATTR_COOKIE,
	__DRAGONITE_ATTR_MAX,
};
#define DRAGONITE_ATTR_MAX (__DRAGONITE_ATTR_MAX - 1)

enum {
	SW_MIXED=0,
	HW_MIXED,
	SW_PS_MIXED,

	VP_SW_GC,
	VP_SW_PS_GC,
	VP_HW_PS_GC,
	VP_SW,
	VP_SW_MGMT,
	VP_HW_MGMT,

	SP_SW,
	SP_SW_SINGLE,
	SP_HW_SINGLE,
	SP_SW_PS_SINGLE,

	QUEUE_INDEX_MAX,
};

enum dragonite_recover_ampdu_flags
{
	SET_BLOCK_BA_FLAG,
	TEAR_DOWN_BA_SESSIONS,
	CLEAR_BLOCK_BA_FLAG,
};

struct dragonite_tx_rate {
	s8 idx;
	u8 count;
}__attribute__((__packed__));

extern struct mac80211_dragonite_data *global_data;

static inline void dragonite_mac_lock_init(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_init(&data->mac_info->drg_mac_lock);
}

static inline void dragonite_mac_lock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_irqsave(&data->mac_info->drg_mac_lock, data->mac_info->drg_mac_lock_irqflags);
}

static inline void dragonite_mac_unlock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_unlock_irqrestore(&data->mac_info->drg_mac_lock, data->mac_info->drg_mac_lock_irqflags);
}

static inline void dragonite_txq_lock_init(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_init(&data->drg_txq_lock);
}

static inline void dragonite_txq_lock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_irqsave(&data->drg_txq_lock, data->drg_txq_lock_irqflags);
}

static inline void dragonite_txq_unlock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_unlock_irqrestore(&data->drg_txq_lock, data->drg_txq_lock_irqflags);
}

static inline void dragonite_ps_lock_init(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_init(&data->mac_info->drg_ps_lock);
}

static inline void dragonite_ps_lock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_irqsave(&data->mac_info->drg_ps_lock, data->mac_info->drg_ps_lock_irqflags);
}

static inline void dragonite_ps_unlock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_unlock_irqrestore(&data->mac_info->drg_ps_lock, data->mac_info->drg_ps_lock_irqflags);
}

static inline void dragonite_bss_ps_lock_init(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_init(&data->mac_info->drg_bss_ps_lock);
}

static inline void dragonite_bss_ps_lock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_irqsave(&data->mac_info->drg_bss_ps_lock, data->mac_info->drg_bss_ps_lock_irqflags);
}

static inline void dragonite_bss_ps_unlock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_unlock_irqrestore(&data->mac_info->drg_bss_ps_lock, data->mac_info->drg_bss_ps_lock_irqflags);
}
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
static inline void dragonite_pm_lock_init(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_init(&data->pm_lock);
}

static inline void dragonite_pm_lock_lock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_lock_irqsave(&data->pm_lock, data->pm_lock_irqflags);
}

static inline void dragonite_pm_lock_unlock(void)
{
    struct mac80211_dragonite_data *data = global_data;

    spin_unlock_irqrestore(&data->pm_lock, data->pm_lock_irqflags);
}
#endif

static inline bool dragonite_vif_is_sta_mode(struct ieee80211_vif *vif)
{
    if((vif->type == NL80211_IFTYPE_STATION)
        || (vif->type == NL80211_IFTYPE_P2P_CLIENT)
        || (vif->type == NL80211_IFTYPE_MESH_POINT))
        return true;

    return false;
}

struct dragonite_sta_priv *idx_to_sp(struct mac80211_dragonite_data *data, u8 idx);
struct dragonite_vif_priv *idx_to_vp(struct mac80211_dragonite_data *data, u8 idx);

int dragonite_process_rx_queue(struct mac80211_dragonite_data *data);

void dragonite_handle_power_saving(struct mac80211_dragonite_data *data);
void dragonite_handle_power_saving_off(struct mac80211_dragonite_data *data);

int dragonite_add_station(struct mac80211_dragonite_data *data, struct ieee80211_sta *sta, struct ieee80211_vif *vif);
int dragonite_remove_station(struct mac80211_dragonite_data *data, struct ieee80211_sta *sta, struct ieee80211_vif *vif);
int dragonite_change_station(struct mac80211_dragonite_data *data, struct ieee80211_sta *sta, struct ieee80211_vif *vif);
void dragonite_recover_station(struct mac80211_dragonite_data *data, u8 sta_idx);

int dragonite_key_config(struct mac80211_dragonite_data *data, struct ieee80211_vif *vif,
		struct ieee80211_sta *sta, struct ieee80211_key_conf *key);
int dragonite_key_delete(struct mac80211_dragonite_data *data, struct ieee80211_vif *vif, 
		struct ieee80211_key_conf *key);

void dragonite_beacon(unsigned long arg);

int dragonite_kthread_init(struct mac80211_dragonite_data *data);
void dragonite_kthread_cleanup(struct mac80211_dragonite_data *data);

extern u32 __omnicfg_filter_mode;
void omnicfg_start(void);
void omnicfg_stop(void);
int omnicfg_process(struct mac80211_dragonite_data *data);

#if defined(DRAGONITE_ENABLE_AIRKISS)
extern u32 __airkiss_filter_mode;
void airkiss_start(void);
void airkiss_stop(void);
#endif

void recover_ampdu(struct mac80211_dragonite_data *data, enum dragonite_recover_ampdu_flags flags);
void dragonite_mac_recover(struct mac80211_dragonite_data *data);
void dragonite_wifi_recover(struct mac80211_dragonite_data *data);
void dragonite_get_channel(u8 *ch, u8 *bw);
void dragonite_set_channel(u8 ch, u8 bw);

bool dragonite_vif_valid(struct dragonite_vif_priv *vp);
bool dragonite_sta_valid(struct dragonite_sta_priv *sp);

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
void dragonite_ps_set_pm_bit(struct ieee80211_vif *vif);
void dragonite_ps_clear_pm_bit(struct mac80211_dragonite_data *data);
void dragonite_ps_wakeup(struct mac80211_dragonite_data *data);
void dragonite_ps_sleep(struct mac80211_dragonite_data *data);
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
void dragonite_calculate_pretbtt(int beacon_interval);
void dragonite_correct_ts(bool pretbtt);
#endif
#endif

#endif /* __DRAGONITE_MAIN_H__ */
