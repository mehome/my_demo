#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <net/dst.h>
#include <net/xfrm.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <net/genetlink.h>
#include <dragonite_main.h>
#include <rf.h>
#include <bb.h>
#include <lib_test.h>
#include <resources.h>
#include <mac.h>
#include <panther_dev.h>
#include <panther_rf.h>
#include <chip.h>
#include <beacon.h>
#include <wla_cfg.h>
#include "../net/mac80211/ieee80211_i.h"
#include <asm/mach-panther/pmu.h>
#include <asm/bootinfo.h>
#include <tx.h>
#include <skb.h>
#include <buffer.h>
#include <wla_mat.h>

#include "dragonite_common.h"
#include "mac_regs.h"
#include "mac_ctrl.h"
#include "ip301.h"
#include "performance.h"

#if !defined DRAGONITE_BEACON_IN_ISR
#error spinlock should review
#endif
#define CONFIG_FPGA 1
#define WARN_QUEUE 100
#define MAX_QUEUE 200
#define DRV_NAME	"wlan"

static u32 wmediumd_portid;
u32 dragonite_debug_flag = DRAGONITE_DEBUG_SYSTEM_FLAG | DRAGONITE_DEBUG_DRIVER_WARN_FLAG | DRAGONITE_DEBUG_NETWORK_INFO_FLAG | DRAGONITE_DEBUG_ENCRPT_FLAG;
volatile u32 drg_wifi_recover = 0;
u32 drg_rf_bb_init_once = 1;
int dragonite_tx_debug_dump=0;
int dragonite_tx_ampdu_enable = 1;
int dragonite_rx_ampdu_enable = 1;
/* ACQ-VO disable ampdu setup by default */
int dragonite_tx_ampdu_vo_enable = 0;
int wifi_tx_enable=0;
u32 drg_tx_winsz = 32;
u32 drg_rx_winsz = 32;
int dragonite_dynamic_ps = 0;
int drg_wifi_poweroff_enable = 0;
int dragonite_force_disable_sniffer_function = 0;

#define DRAGONITE_RECHECK_MAGIC_MAX_TIMES 1

#if defined (DRAGONITE_ISR_CHECK)
#define DRAGONITE_ISR_TOLERANT_COUNT_PER_JIFFIES 200
static unsigned long drg_isr_prev_jiffies = 0;
static u32 drg_isr_count = 0;
static u32 drg_tx_done_isr_count = 0;
static u32 drg_tx_fail_isr_count = 0;
static u32 drg_tx_switch_isr_count = 0;
static u32 drg_tsf_isr_count = 0;
static u32 drg_rx_isr_count = 0;
static u32 drg_rx_descr_full_isr_count = 0;
static volatile u32 drg_rx_prev_count = 0;
volatile u32 drg_rx_count = 0;
static u32 drg_nobody_isr_count = 0;
static u32 drg_rx_full_count = 0;
#endif

extern int wla_debug_init(void *wla_sc);
extern int rfc_test_init(void);
extern int lrf_test_init(void);
extern int wla_procfs_init(struct mac80211_dragonite_data *wlan_data);
static int radios = 1;
module_param(radios, int, 0444);
MODULE_PARM_DESC(radios, "Number of simulated radios");
struct _drg_tx_stats drg_tx_stats;
static int channels = 1;
module_param(channels, int, 0444);
MODULE_PARM_DESC(channels, "Number of concurrent channels");

static bool paged_rx = false;
module_param(paged_rx, bool, 0644);
MODULE_PARM_DESC(paged_rx, "Use paged SKBs for RX instead of linear ones");

static bool rctbl = false;
module_param(rctbl, bool, 0444);
MODULE_PARM_DESC(rctbl, "Handle rate control table");

enum dragonite_regtest {
	DRAGONITE_REGTEST_DISABLED = 0,
	DRAGONITE_REGTEST_DRIVER_REG_FOLLOW = 1,
	DRAGONITE_REGTEST_DRIVER_REG_ALL = 2,
	DRAGONITE_REGTEST_DIFF_COUNTRY = 3,
	DRAGONITE_REGTEST_WORLD_ROAM = 4,
	DRAGONITE_REGTEST_CUSTOM_WORLD = 5,
	DRAGONITE_REGTEST_CUSTOM_WORLD_2 = 6,
	DRAGONITE_REGTEST_STRICT_FOLLOW = 7,
	DRAGONITE_REGTEST_STRICT_ALL = 8,
	DRAGONITE_REGTEST_STRICT_AND_DRIVER_REG = 9,
	DRAGONITE_REGTEST_ALL = 10,
};

/* Set to one of the DRAGONITE_REGTEST_* values above */
static int regtest = DRAGONITE_REGTEST_DISABLED;
module_param(regtest, int, 0444);
MODULE_PARM_DESC(regtest, "The type of regulatory test we want to run");

static const char *dragonite_alpha2s[] = {
	"FI",
	"AL",
	"US",
	"DE",
	"JP",
	"AL",
};

static const struct ieee80211_regdomain dragonite_world_regdom_custom_01 = {
	.n_reg_rules = 4,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(2412-10, 2462+10, 40, 0, 20, 0),
		REG_RULE(2484-10, 2484+10, 40, 0, 20, 0),
		REG_RULE(5150-10, 5240+10, 40, 0, 30, 0),
		REG_RULE(5745-10, 5825+10, 40, 0, 30, 0),
	}
};

static const struct ieee80211_regdomain dragonite_world_regdom_custom_02 = {
	.n_reg_rules = 2,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(2412-10, 2462+10, 40, 0, 20, 0),
		REG_RULE(5725-10, 5850+10, 40, 0, 30,
			NL80211_RRF_PASSIVE_SCAN | NL80211_RRF_NO_IBSS),
	}
};

#define DRAGONITE_MAGIC	0xdead6767
#define DRAGONITE_VIF_MAGIC	0x69537748
bool dragonite_vif_valid(struct dragonite_vif_priv *vp)
{
	return (vp->magic == DRAGONITE_VIF_MAGIC) ? true:false;
}
static inline void dragonite_check_magic(struct ieee80211_vif *vif)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	int times = 0;

	if(vp->magic == DRAGONITE_VIF_MAGIC) {
	}
	else if(vp->magic == DRAGONITE_MAGIC) {
	}
	else if(vp->magic == 0) {

        if(in_interrupt())
		{
			/* do nothing */
		}
		else
		{
			while((vp->magic == 0) 
				  && (times < DRAGONITE_RECHECK_MAGIC_MAX_TIMES)) {
				times++;
				schedule_timeout_interruptible(HZ/10);
			}
			if(vp->magic == 0) {
				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Magic wrong, vp->magic == 0 !!");
			}
		}
	}
	else
	{
		__WARN_printf("Invalid VIF (%p) vp (%p) magic %#x, %pM\n", 
					  vif, vp, vp->magic, vif->addr);
	}
}

static inline void dragonite_set_magic(struct ieee80211_vif *vif)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	vp->magic = DRAGONITE_VIF_MAGIC;
}

static inline void dragonite_clear_magic(struct ieee80211_vif *vif)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	vp->magic = DRAGONITE_MAGIC;
}

static inline void dragonite_set_vp_magic(struct dragonite_vif_priv *vp)
{
	vp->magic = DRAGONITE_VIF_MAGIC;
}

static inline void dragonite_clear_vp_magic(struct dragonite_vif_priv *vp)
{
	vp->magic = DRAGONITE_MAGIC;
}

#define DRAGONITE_STA_MAGIC	0x6d537749

bool dragonite_sta_valid(struct dragonite_sta_priv *sp)
{
	return (sp->magic == DRAGONITE_STA_MAGIC) ? true:false;
}
static inline void dragonite_check_sta_magic(struct ieee80211_sta *sta)
{
	struct dragonite_sta_priv *sp = (void *)sta->drv_priv;
	int times = 0;

	if(sp->magic == DRAGONITE_STA_MAGIC) {
	}
	else if(sp->magic == DRAGONITE_MAGIC) {
	}
	else if(sp->magic == 0) {

        if(in_interrupt())
		{
			/* do nothing */
		}
		else
		{
			while((sp->magic == 0) 
				  && (times < DRAGONITE_RECHECK_MAGIC_MAX_TIMES)) {
				times++;
				schedule_timeout_interruptible(HZ/10);
			}
			if(sp->magic == 0) {
				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Magic wrong, sp->magic == 0 !!");
			}
		}
	}
	else
	{
		__WARN_printf("Invalid STA (%p) sp (%p) magic %#x, %pM\n", 
					  sta, sp, sp->magic, sta->addr);
	}
}

static inline void dragonite_set_sta_magic(struct ieee80211_sta *sta)
{
	struct dragonite_sta_priv *sp = (void *)sta->drv_priv;
	sp->magic = DRAGONITE_STA_MAGIC;
}

static inline void dragonite_clear_sta_magic(struct ieee80211_sta *sta)
{
	struct dragonite_sta_priv *sp = (void *)sta->drv_priv;
	sp->magic = DRAGONITE_MAGIC;
}

static inline void dragonite_set_sp_magic(struct dragonite_sta_priv *sp)
{
	sp->magic = DRAGONITE_STA_MAGIC;
}

static inline void dragonite_clear_sp_magic(struct dragonite_sta_priv *sp)
{
	sp->magic = DRAGONITE_MAGIC;
}

struct dragonite_chanctx_priv {
	u32 magic;
};

#define DRAGONITE_CHANCTX_MAGIC 0x6d53774a

static inline void dragonite_check_chanctx_magic(struct ieee80211_chanctx_conf *c)
{
	struct dragonite_chanctx_priv *cp = (void *)c->drv_priv;
	WARN_ON(cp->magic != DRAGONITE_CHANCTX_MAGIC);
}

static inline void dragonite_set_chanctx_magic(struct ieee80211_chanctx_conf *c)
{
	struct dragonite_chanctx_priv *cp = (void *)c->drv_priv;
	cp->magic = DRAGONITE_CHANCTX_MAGIC;
}

static inline void dragonite_clear_chanctx_magic(struct ieee80211_chanctx_conf *c)
{
	struct dragonite_chanctx_priv *cp = (void *)c->drv_priv;
	cp->magic = 0;
}

static struct class *dragonite_class;

static struct net_device *dragonite_mon; /* global monitor netdev */

#define CHAN2G(_freq)  { \
	.band = IEEE80211_BAND_2GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_freq), \
	.max_power = 20, \
}

#define CHAN5G(_freq) { \
	.band = IEEE80211_BAND_5GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_freq), \
	.max_power = 20, \
}

static const struct ieee80211_channel dragonite_channels_2ghz[] = {
	CHAN2G(2412), /* Channel 1 */
	CHAN2G(2417), /* Channel 2 */
	CHAN2G(2422), /* Channel 3 */
	CHAN2G(2427), /* Channel 4 */
	CHAN2G(2432), /* Channel 5 */
	CHAN2G(2437), /* Channel 6 */
	CHAN2G(2442), /* Channel 7 */
	CHAN2G(2447), /* Channel 8 */
	CHAN2G(2452), /* Channel 9 */
	CHAN2G(2457), /* Channel 10 */
	CHAN2G(2462), /* Channel 11 */
	CHAN2G(2467), /* Channel 12 */
	CHAN2G(2472), /* Channel 13 */
	CHAN2G(2484), /* Channel 14 */
};


static const struct ieee80211_rate dragonite_rates[] = {
	{ .bitrate = 10 },
	{ .bitrate = 20, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 55, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 110, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 60 },
	{ .bitrate = 90 },
	{ .bitrate = 120 },
	{ .bitrate = 180 },
	{ .bitrate = 240 },
	{ .bitrate = 360 },
	{ .bitrate = 480 },
	{ .bitrate = 540 }
};

static spinlock_t dragonite_radio_lock;
static struct list_head dragonite_radios;
struct dragonite_radiotap_hdr {
	struct ieee80211_radiotap_header hdr;
	__le64 rt_tsft;
	u8 rt_flags;
	u8 rt_rate;
	__le16 rt_channel;
	__le16 rt_chbitmask;
} __attribute__((packed));

/* MAC80211_DRAGONITE netlinf family */
static struct genl_family dragonite_genl_family = {
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = "DRAGONITE_WIFI",
	.version = 1,
	.maxattr = DRAGONITE_ATTR_MAX,
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
enum dragonite_multicast_groups {
	DRAGONITE_MCGRP_CONFIG,
};

static const struct genl_multicast_group dragonite_mcgrps[] = {
	[DRAGONITE_MCGRP_CONFIG] = { .name = "config", },
};
#endif

/* MAC80211_DRAGONITE netlink policy */

static struct nla_policy dragonite_genl_policy[DRAGONITE_ATTR_MAX + 1] = {
	[DRAGONITE_ATTR_ADDR_RECEIVER] = { .type = NLA_UNSPEC,
				       .len = 6*sizeof(u8) },
	[DRAGONITE_ATTR_ADDR_TRANSMITTER] = { .type = NLA_UNSPEC,
					  .len = 6*sizeof(u8) },
	[DRAGONITE_ATTR_FRAME] = { .type = NLA_BINARY,
			       .len = IEEE80211_MAX_DATA_LEN },
	[DRAGONITE_ATTR_FLAGS] = { .type = NLA_U32 },
	[DRAGONITE_ATTR_RX_RATE] = { .type = NLA_U32 },
	[DRAGONITE_ATTR_SIGNAL] = { .type = NLA_U32 },
	[DRAGONITE_ATTR_TX_INFO] = { .type = NLA_UNSPEC,
				 .len = IEEE80211_TX_MAX_RATES*sizeof(
					struct dragonite_tx_rate)},
	[DRAGONITE_ATTR_COOKIE] = { .type = NLA_U64 },
};

static netdev_tx_t dragonite_mon_xmit(struct sk_buff *skb,
					struct net_device *dev)
{
	/* TODO: allow packet injection */
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static inline u64 mac80211_dragonite_get_tsf_raw(void)
{
	return ktime_to_us(ktime_get_real());
}

static __le64 __mac80211_dragonite_get_tsf(struct mac80211_dragonite_data *data)
{
	u64 now = mac80211_dragonite_get_tsf_raw();
	return cpu_to_le64(now + data->tsf_offset);
}

static u64 mac80211_dragonite_get_tsf(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_data *data = hw->priv;
	return le64_to_cpu(__mac80211_dragonite_get_tsf(data));
}

static void mac80211_dragonite_set_tsf(struct ieee80211_hw *hw,
		struct ieee80211_vif *vif, u64 tsf)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct mac80211_dragonite_data *data = hw->priv;
	u64 now = mac80211_dragonite_get_tsf(hw, vif);
	u32 bcn_int = data->beacon_int;
	u64 delta = abs(tsf - now);

	/* adjust after beaconing with new timestamp at old TBTT */
	if (tsf > now) {
		data->tsf_offset += delta;
		data->bcn_delta = do_div(delta, bcn_int);
	} else {
		data->tsf_offset -= delta;
		data->bcn_delta = -do_div(delta, bcn_int);
	}
#else
	struct mac80211_dragonite_data *data = hw->priv;
	u64 now = mac80211_dragonite_get_tsf(hw, vif);
	u32 bcn_int = data->beacon_int;
	s64 delta = tsf - now;

	data->tsf_offset += delta;
	/* adjust after beaconing with new timestamp at old TBTT */
	data->bcn_delta = do_div(delta, bcn_int);
#endif
}

static void mac80211_dragonite_monitor_rx(struct ieee80211_hw *hw,
				      struct sk_buff *tx_skb,
				      struct ieee80211_channel *chan)
{
	struct mac80211_dragonite_data *data = hw->priv;
	struct sk_buff *skb;
	struct dragonite_radiotap_hdr *hdr;
	u16 flags;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(tx_skb);
	struct ieee80211_rate *txrate = ieee80211_get_tx_rate(hw, info);

	if (!netif_running(dragonite_mon))
		return;

	skb = skb_copy_expand(tx_skb, sizeof(*hdr), 0, GFP_ATOMIC);
	if (skb == NULL)
		return;

	hdr = (struct dragonite_radiotap_hdr *) skb_push(skb, sizeof(*hdr));
	hdr->hdr.it_version = PKTHDR_RADIOTAP_VERSION;
	hdr->hdr.it_pad = 0;
	hdr->hdr.it_len = cpu_to_le16(sizeof(*hdr));
	hdr->hdr.it_present = cpu_to_le32((1 << IEEE80211_RADIOTAP_FLAGS) |
					  (1 << IEEE80211_RADIOTAP_RATE) |
					  (1 << IEEE80211_RADIOTAP_TSFT) |
					  (1 << IEEE80211_RADIOTAP_CHANNEL));
	hdr->rt_tsft = __mac80211_dragonite_get_tsf(data);
	hdr->rt_flags = 0;
	hdr->rt_rate = txrate->bitrate / 5;
	hdr->rt_channel = cpu_to_le16(chan->center_freq);
	flags = IEEE80211_CHAN_2GHZ;
	if (txrate->flags & IEEE80211_RATE_ERP_G)
		flags |= IEEE80211_CHAN_OFDM;
	else
		flags |= IEEE80211_CHAN_CCK;
	hdr->rt_chbitmask = cpu_to_le16(flags);

	skb->dev = dragonite_mon;
	skb_set_mac_header(skb, 0);
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = htons(ETH_P_802_2);
	memset(skb->cb, 0, sizeof(skb->cb));
	netif_rx(skb);
}


static void mac80211_dragonite_monitor_ack(struct ieee80211_channel *chan,
				       const u8 *addr)
{
	struct sk_buff *skb;
	struct dragonite_radiotap_hdr *hdr;
	u16 flags;
	struct ieee80211_hdr *hdr11;

	if (!netif_running(dragonite_mon))
		return;

	skb = dev_alloc_skb(100);
	if (skb == NULL)
		return;

	hdr = (struct dragonite_radiotap_hdr *) skb_put(skb, sizeof(*hdr));
	hdr->hdr.it_version = PKTHDR_RADIOTAP_VERSION;
	hdr->hdr.it_pad = 0;
	hdr->hdr.it_len = cpu_to_le16(sizeof(*hdr));
	hdr->hdr.it_present = cpu_to_le32((1 << IEEE80211_RADIOTAP_FLAGS) |
					  (1 << IEEE80211_RADIOTAP_CHANNEL));
	hdr->rt_flags = 0;
	hdr->rt_rate = 0;
	hdr->rt_channel = cpu_to_le16(chan->center_freq);
	flags = IEEE80211_CHAN_2GHZ;
	hdr->rt_chbitmask = cpu_to_le16(flags);

	hdr11 = (struct ieee80211_hdr *) skb_put(skb, 10);
	hdr11->frame_control = cpu_to_le16(IEEE80211_FTYPE_CTL |
					   IEEE80211_STYPE_ACK);
	hdr11->duration_id = cpu_to_le16(0);
	memcpy(hdr11->addr1, addr, ETH_ALEN);

	skb->dev = dragonite_mon;
	skb_set_mac_header(skb, 0);
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = htons(ETH_P_802_2);
	memset(skb->cb, 0, sizeof(skb->cb));
	netif_rx(skb);
}


static bool dragonite_ps_rx_ok(struct mac80211_dragonite_data *data,
			   struct sk_buff *skb)
{
	switch (data->ps) {
	case PS_DISABLED:
		return true;
	case PS_ENABLED:
		return false;
	case PS_AUTO_POLL:
		/* TODO: accept (some) Beacons by default and other frames only
		 * if pending PS-Poll has been sent */
		return true;
	case PS_MANUAL_POLL:
		/* Allow unicast frames to own address if there is a pending
		 * PS-Poll */
		if (data->ps_poll_pending &&
		    memcmp(data->hw->wiphy->perm_addr, skb->data + 4,
			   ETH_ALEN) == 0) {
			data->ps_poll_pending = false;
			return true;
		}
		return false;
	}

	return true;
}


struct mac80211_dragonite_addr_match_data {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	u8 addr[ETH_ALEN];
	bool ret;
#else
	bool ret;
	const u8 *addr;
#endif
};

static void mac80211_dragonite_addr_iter(void *data, u8 *mac,
				     struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_addr_match_data *md = data;
	if (memcmp(mac, md->addr, ETH_ALEN) == 0)
		md->ret = true;
}


static bool mac80211_dragonite_addr_match(struct mac80211_dragonite_data *data,
				      const u8 *addr)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct mac80211_dragonite_addr_match_data md = {
		.ret = false,
	};

	if (data->scanning && memcmp(addr, data->scan_addr, ETH_ALEN) == 0)
		return true;

	memcpy(md.addr, addr, ETH_ALEN);
#else
	struct mac80211_dragonite_addr_match_data md;

	if (memcmp(addr, data->hw->wiphy->perm_addr, ETH_ALEN) == 0)
		return true;

	md.ret = false;
	md.addr = addr;
#endif

	ieee80211_iterate_active_interfaces_atomic(data->hw,
						   IEEE80211_IFACE_ITER_NORMAL,
						   mac80211_dragonite_addr_iter,
						   &md);

	return md.ret;
}

static void mac80211_dragonite_tx_frame_nl(struct ieee80211_hw *hw,
				       struct sk_buff *my_skb,
				       int dst_portid)
{
	struct sk_buff *skb;
	struct mac80211_dragonite_data *data = hw->priv;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) my_skb->data;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(my_skb);
	void *msg_head;
	unsigned int dragonite_flags = 0;
	int i;
	struct dragonite_tx_rate tx_attempts[IEEE80211_TX_MAX_RATES];

	if (data->ps != PS_DISABLED)
		hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_PM);
	/* If the queue contains MAX_QUEUE skb's drop some */
	if (skb_queue_len(&data->pending) >= MAX_QUEUE) {
		/* Droping until WARN_QUEUE level */
		while (skb_queue_len(&data->pending) >= WARN_QUEUE)
			skb_dequeue(&data->pending);
	}

	skb = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_ATOMIC);
	if (skb == NULL)
		goto nla_put_failure;

	msg_head = genlmsg_put(skb, 0, 0, &dragonite_genl_family, 0,
			       DRAGONITE_CMD_FRAME);
	if (msg_head == NULL) {
		printk(KERN_DEBUG "mac80211_dragonite: problem with msg_head\n");
		goto nla_put_failure;
	}

	if (nla_put(skb, DRAGONITE_ATTR_ADDR_TRANSMITTER,
		    sizeof(struct mac_address), data->addresses[1].addr))
		goto nla_put_failure;

	/* We get the skb->data */
	if (nla_put(skb, DRAGONITE_ATTR_FRAME, my_skb->len, my_skb->data))
		goto nla_put_failure;

	/* We get the flags for this transmission, and we translate them to
	   wmediumd flags  */

	if (info->flags & IEEE80211_TX_CTL_REQ_TX_STATUS)
		dragonite_flags |= DRAGONITE_TX_CTL_REQ_TX_STATUS;

	if (info->flags & IEEE80211_TX_CTL_NO_ACK)
		dragonite_flags |= DRAGONITE_TX_CTL_NO_ACK;

	if (nla_put_u32(skb, DRAGONITE_ATTR_FLAGS, dragonite_flags))
		goto nla_put_failure;

	/* We get the tx control (rate and retries) info*/

	for (i = 0; i < IEEE80211_TX_MAX_RATES; i++) {
		tx_attempts[i].idx = info->status.rates[i].idx;
		tx_attempts[i].count = info->status.rates[i].count;
	}

	if (nla_put(skb, DRAGONITE_ATTR_TX_INFO,
		    sizeof(struct dragonite_tx_rate)*IEEE80211_TX_MAX_RATES,
		    tx_attempts))
		goto nla_put_failure;

	/* We create a cookie to identify this skb */
	if (nla_put_u64(skb, DRAGONITE_ATTR_COOKIE, (unsigned long) my_skb))
		goto nla_put_failure;

	genlmsg_end(skb, msg_head);
	genlmsg_unicast(&init_net, skb, dst_portid);

	/* Enqueue the packet */
	skb_queue_tail(&data->pending, my_skb);
	return;

nla_put_failure:
	printk(KERN_DEBUG "mac80211_dragonite: error occurred in %s\n", __func__);
}

static bool dragonite_chans_compat(struct ieee80211_channel *c1,
			       struct ieee80211_channel *c2)
{
	if (!c1 || !c2)
		return false;

	return c1->center_freq == c2->center_freq;
}

struct tx_iter_data {
	struct ieee80211_channel *channel;
	bool receive;
};

static void mac80211_dragonite_tx_iter(void *_data, u8 *addr,
				   struct ieee80211_vif *vif)
{
	struct tx_iter_data *data = _data;

	if (!vif->chanctx_conf)
		return;

	if (!dragonite_chans_compat(data->channel,
				rcu_dereference(vif->chanctx_conf)->def.chan))
		return;

	data->receive = true;
}

static bool mac80211_dragonite_tx_frame_no_nl(struct ieee80211_hw *hw,
					  struct sk_buff *skb,
					  struct ieee80211_channel *chan)
{
	struct mac80211_dragonite_data *data = hw->priv, *data2;
	bool ack = false;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_rx_status rx_status;
	u64 now;

	memset(&rx_status, 0, sizeof(rx_status));
	rx_status.flag |= RX_FLAG_MACTIME_START;
	rx_status.freq = chan->center_freq;
	rx_status.band = chan->band;
	if (info->control.rates[0].flags & IEEE80211_TX_RC_VHT_MCS) {
		rx_status.rate_idx =
			ieee80211_rate_get_vht_mcs(&info->control.rates[0]);
		rx_status.vht_nss =
			ieee80211_rate_get_vht_nss(&info->control.rates[0]);
		rx_status.flag |= RX_FLAG_VHT;
	} else {
		rx_status.rate_idx = info->control.rates[0].idx;
		if (info->control.rates[0].flags & IEEE80211_TX_RC_MCS)
			rx_status.flag |= RX_FLAG_HT;
	}
	if (info->control.rates[0].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
		rx_status.flag |= RX_FLAG_40MHZ;
	if (info->control.rates[0].flags & IEEE80211_TX_RC_SHORT_GI)
		rx_status.flag |= RX_FLAG_SHORT_GI;
	/* TODO: simulate real signal strength (and optional packet loss) */
	rx_status.signal = data->power_level - 50;

	if (data->ps != PS_DISABLED)
		hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_PM);

	/* release the skb's source info */
	skb_orphan(skb);
	skb_dst_drop(skb);
	skb->mark = 0;
	secpath_reset(skb);
	nf_reset(skb);

	/*
	 * Get absolute mactime here so all HWs RX at the "same time", and
	 * absolute TX time for beacon mactime so the timestamp matches.
	 * Giving beacons a different mactime than non-beacons looks messy, but
	 * it helps the Toffset be exact and a ~10us mactime discrepancy
	 * probably doesn't really matter.
	 */
	if (ieee80211_is_beacon(hdr->frame_control) ||
	    ieee80211_is_probe_resp(hdr->frame_control))
		now = data->abs_bcn_ts;
	else
		now = mac80211_dragonite_get_tsf_raw();

	/* Copy skb to all enabled radios that are on the current frequency */
	spin_lock(&dragonite_radio_lock);
	list_for_each_entry(data2, &dragonite_radios, list) {
		struct sk_buff *nskb;
		struct tx_iter_data tx_iter_data = {
			.receive = false,
			.channel = chan,
		};

		if (data == data2)
			continue;

		if (!data2->started || (data2->idle && !data2->tmp_chan) ||
		    !dragonite_ps_rx_ok(data2, skb))
			continue;

		if (!(data->group & data2->group))
			continue;

		if (!dragonite_chans_compat(chan, data2->tmp_chan) &&
		    !dragonite_chans_compat(chan, data2->channel)) {
			ieee80211_iterate_active_interfaces_atomic(
				data2->hw, IEEE80211_IFACE_ITER_NORMAL,
				mac80211_dragonite_tx_iter, &tx_iter_data);
			if (!tx_iter_data.receive)
				continue;
		}

		/*
		 * reserve some space for our vendor and the normal
		 * radiotap header, since we're copying anyway
		 */
		if (skb->len < PAGE_SIZE && paged_rx) {
			struct page *page = alloc_page(GFP_ATOMIC);

			if (!page)
				continue;

			nskb = dev_alloc_skb(128);
			if (!nskb) {
				__free_page(page);
				continue;
			}

			memcpy(page_address(page), skb->data, skb->len);
			skb_add_rx_frag(nskb, 0, page, 0, skb->len, skb->len);
		} else {
			nskb = skb_copy(skb, GFP_ATOMIC);
			if (!nskb)
				continue;
		}

		if (mac80211_dragonite_addr_match(data2, hdr->addr1))
			ack = true;

		rx_status.mactime = now + data2->tsf_offset;
#if 0
		/*
		 * Don't enable this code by default as the OUI 00:00:00
		 * is registered to Xerox so we shouldn't use it here, it
		 * might find its way into pcap files.
		 * Note that this code requires the headroom in the SKB
		 * that was allocated earlier.
		 */
		rx_status.vendor_radiotap_oui[0] = 0x00;
		rx_status.vendor_radiotap_oui[1] = 0x00;
		rx_status.vendor_radiotap_oui[2] = 0x00;
		rx_status.vendor_radiotap_subns = 127;
		/*
		 * Radiotap vendor namespaces can (and should) also be
		 * split into fields by using the standard radiotap
		 * presence bitmap mechanism. Use just BIT(0) here for
		 * the presence bitmap.
		 */
		rx_status.vendor_radiotap_bitmap = BIT(0);
		/* We have 8 bytes of (dummy) data */
		rx_status.vendor_radiotap_len = 8;
		/* For testing, also require it to be aligned */
		rx_status.vendor_radiotap_align = 8;
		/* push the data */
		memcpy(skb_push(nskb, 8), "ABCDEFGH", 8);
#endif

		memcpy(IEEE80211_SKB_RXCB(nskb), &rx_status, sizeof(rx_status));
		ieee80211_rx_irqsafe(data2->hw, nskb);
	}
	spin_unlock(&dragonite_radio_lock);

	return ack;
}
static void mac80211_dragonite_tx(struct ieee80211_hw *hw,
			      struct ieee80211_tx_control *control,
			      struct sk_buff *skb)
{
	struct mac80211_dragonite_data *data = hw->priv;
	struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
	struct ieee80211_chanctx_conf *chanctx_conf;
	struct ieee80211_channel *channel;
	struct skb_info skbinfo;
	struct dragonite_vif_priv *vp = NULL;
	struct dragonite_sta_priv *sp = NULL;
#if defined(CONFIG_MAC80211_MESH)
	struct ieee80211_vif *vif = NULL;
#endif

	if (WARN_ON(skb->len < 10)) {
		/* Should not happen; just a sanity check for addr1 use */
		dev_kfree_skb_any(skb);
		return;
	}
	if (channels == 1) {
		channel = data->channel;
	} else if (txi->hw_queue == 4) {
		channel = data->tmp_chan;
	} else {
		chanctx_conf = rcu_dereference(txi->control.vif->chanctx_conf);
		if (chanctx_conf)
			channel = chanctx_conf->def.chan;
		else
			channel = NULL;
	}

	if (WARN(!channel, "TX w/o channel - queue = %d\n", txi->hw_queue)) {
		dev_kfree_skb_any(skb);
		return;
	}

	if (data->idle && !data->tmp_chan) {
		wiphy_debug(hw->wiphy, "Trying to TX when idle - reject\n");
		dev_kfree_skb_any(skb);
		return;
	}

	if (txi->control.vif)
	{
		dragonite_check_magic(txi->control.vif);
#if defined(CONFIG_MAC80211_MESH)
		vif = txi->control.vif;
#endif
		vp = (void *) &txi->control.vif->drv_priv;
	}
	if (control->sta)
	{
		dragonite_check_sta_magic(control->sta);
		sp = (void *)control->sta->drv_priv;
	}
	skbinfo.flags = 0;
#if defined(CONFIG_MAC80211_MESH) || defined(DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
    if(0 > dragonite_parse_tx_skb(data, skb, &skbinfo, vif))
#else
    if(0 > dragonite_parse_tx_skb(data, skb, &skbinfo))
#endif
    {
        dev_kfree_skb_any(skb);
		return;
    }

	if (rctbl)
	{
		    ieee80211_get_tx_rates(txi->control.vif, control->sta, skb,
					   txi->control.rates,
					   ARRAY_SIZE(txi->control.rates));
	}
	txi->rate_driver_data[0] = channel;

	if(dragonite_tx_debug_dump) {
		dragonite_dump_tx_skb(skb);
	}

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
	dragonite_pm_lock_lock();
	if(data->poweroff) {
		dragonite_ps_wakeup(data);
		data->ps_flags |= PS_FORCE_WAKE_UP;
	}
	dragonite_pm_lock_unlock();
#endif
	if(0 > dragonite_tx_skb(data, skb, &skbinfo, vp, sp)) {
	    dev_kfree_skb_any(skb);
	}
}
struct dragonite_sta_priv *idx_to_sp(struct mac80211_dragonite_data *data, u8 idx)
{
	if(idx >= MAX_STA_NUM)
	{
		panic("sta_idx is larger then %d\n", MAX_STA_NUM);
	}
	return ((struct dragonite_sta_priv *)(data->sta_priv_addr[idx]));
}

struct dragonite_vif_priv *idx_to_vp(struct mac80211_dragonite_data *data, u8 idx)
{
	if(idx >= MAX_BSSIDS)
	{
		panic("vif_idx is larger then %d\n", MAX_BSSIDS);
	}
	return ((struct dragonite_vif_priv *)(data->vif_priv_addr[idx]));
}

struct beacon_prepare_args
{
	struct ieee80211_hw *hw;
	struct sk_buff *beacon_skb[MAC_MAX_BSSIDS];
};

/*this function are modify from __ieee80211_beacon_add_tim()*/
static void dragonite_beacon_add_tim(struct mac80211_dragonite_data *data, struct ieee80211_sub_if_data *sdata,
				       struct ps_data *ps, struct sk_buff *skb, int bss_idx)
{
	u8 *pos, *tim;
	int aid0 = 0;
	int i, have_bits = 0, n1, n2;
	struct dragonite_vif_priv *vp;
	MAC_INFO *info = data->mac_info;
	int aid, sta_idx, tid;
	bool force_enable_aid;
	struct dragonite_sta_priv *sp;

	if (ps->dtim_count == 0)
		ps->dtim_count = sdata->vif.bss_conf.dtim_period - 1;
	else
		ps->dtim_count--;

	tim = pos = (u8 *) skb_put(skb, 6);
	*pos++ = WLAN_EID_TIM;
	*pos++ = 4;
	*pos++ = ps->dtim_count;
	*pos++ = sdata->vif.bss_conf.dtim_period;

	data->dragonite_bss_info[bss_idx].dtim_count = ps->dtim_count;
	vp = idx_to_vp(data, (u8)bss_idx);
	if(vp==NULL) {
		return;
	}
	if(ps->dtim_count == 0) { /* fill aid0 */

		if(info->bss_ps_status[bss_idx] == DRG_BSS_PS_ON) /* some STAs in power save mode */
		{
			if((vp->tx_sw_ps_gc_queue.qhead != NULL) || (vp->tx_hw_ps_gc_queue.qhead != NULL)) {
				aid0 = 1;
			}
		}
	}

	/* any STAs have unicast data pkts */
 	for(sta_idx = 0; sta_idx < info->sta_idx_msb; sta_idx++)
 	{
		sp = idx_to_sp(data, (u8) sta_idx);
		if(sp==NULL) {
			continue;
		}
		force_enable_aid = false;
		if(info->sta_ps_recover & (0x1 << sta_idx)) {/* pull up PS UNKNOW STA aid */
			if(info->sta_cap_tbls[sta_idx].bssid == bss_idx) {
				force_enable_aid = true;
			}
		}
		for(tid = 0; tid < TID_MAX + 1; tid++) {
			if(((sp->tx_sw_queue[tid].qhead != NULL) && (info->sta_cap_tbls[sta_idx].bssid == bss_idx)) || force_enable_aid) {
				aid = info->sta_cap_tbls[sta_idx].aid;
                if((aid > 0) && (aid <= IEEE80211_MAX_AID))
				{
                    if(0==have_bits)
					{
                        /* lazy clear on TIM bitmap only if we start using it */
                        memset(ps->tim, 0, sizeof(ps->tim));
                        have_bits = 1;
					}
                    ps->tim[aid / 8] |= (1 << (aid % 8));
				}
				break;
			}
		}
 	}

	if (have_bits) {
		/* Find largest even number N1 so that bits numbered 1 through
		 * (N1 x 8) - 1 in the bitmap are 0 and number N2 so that bits
		 * (N2 + 1) x 8 through 2007 are 0. */
		n1 = 0;
		for (i = 0; i < IEEE80211_MAX_TIM_LEN; i++) {
			if (ps->tim[i]) {
				n1 = i & 0xfe;
				break;
			}
		}
		n2 = n1;
		for (i = IEEE80211_MAX_TIM_LEN - 1; i >= n1; i--) {
			if (ps->tim[i]) {
				n2 = i;
				break;
			}
		}

		/* Bitmap control */
		*pos++ = n1 | aid0;
		/* Part Virt Bitmap */
		skb_put(skb, n2 - n1);
		memcpy(pos, ps->tim + n1, n2 - n1 + 1);

		tim[1] = n2 - n1 + 4;
	} else {
		*pos++ = aid0; /* Bitmap control */
		*pos++ = 0; /* Part Virt Bitmap */
	}
}

struct sk_buff *dragonite_beacon_get_tim(struct ieee80211_hw *hw,
					 struct ieee80211_vif *vif,
					 u16 *tim_offset, u16 *tim_length)
{
	struct ieee80211_local *local = hw_to_local(hw);
	struct sk_buff *skb = NULL;
	struct ieee80211_tx_info *info;
	struct ieee80211_sub_if_data *sdata = NULL;
	enum ieee80211_band band;
	struct ieee80211_tx_rate_control txrc;
	struct ieee80211_chanctx_conf *chanctx_conf;
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	
	rcu_read_lock();

	sdata = vif_to_sdata(vif);
	chanctx_conf = rcu_dereference(sdata->vif.chanctx_conf);

	if (!ieee80211_sdata_running(sdata) || !chanctx_conf)
		goto out;

	if (tim_offset)
		*tim_offset = 0;
	if (tim_length)
		*tim_length = 0;

	if (sdata->vif.type == NL80211_IFTYPE_AP) {
		struct ieee80211_if_ap *ap = &sdata->u.ap;
		struct beacon_data *beacon = rcu_dereference(ap->beacon);

		if (beacon) {
			/*
			 * headroom, head length,
			 * tail length and maximum TIM length
			 */
			skb = dev_alloc_skb(local->tx_headroom +
					    beacon->head_len +
					    beacon->tail_len + 256);
			if (!skb)
				goto out;

			skb_reserve(skb, local->tx_headroom);
			memcpy(skb_put(skb, beacon->head_len), beacon->head,
			       beacon->head_len);

			dragonite_beacon_add_tim((struct mac80211_dragonite_data *)hw->priv, sdata, &ap->ps, skb, vp->bssid_idx);
			//arthur_beacon_add_tim((struct arthur_softc *)hw->priv, ap, skb, beacon, vp->bssid_idx);

			if (tim_offset)
				*tim_offset = beacon->head_len;
			if (tim_length)
				*tim_length = skb->len - beacon->head_len;

			if (beacon->tail)
				memcpy(skb_put(skb, beacon->tail_len),
				       beacon->tail, beacon->tail_len);
		} else
			goto out;

#if defined(CONFIG_MAC80211_MESH)
	} else if (ieee80211_vif_is_mesh(&sdata->vif)) {
		struct ieee80211_if_mesh *ifmsh = &sdata->u.mesh;
		struct beacon_data *beacon = NULL;

		beacon = rcu_dereference(ifmsh->beacon);
		if (!beacon)
			goto out;

#if 0
		if (beacon->csa_counter_offsets[0]) {
			if (!is_template)
				/* TODO: For mesh csa_counter is in TU, so
				 * decrementing it by one isn't correct, but
				 * for now we leave it consistent with overall
				 * mac80211's behavior.
				 */
				__ieee80211_csa_update_counter(beacon);

			ieee80211_set_csa(sdata, beacon);
		}
#endif

		if (ifmsh->sync_ops)
			ifmsh->sync_ops->adjust_tbtt(sdata, beacon);

		skb = dev_alloc_skb(local->tx_headroom +
				    beacon->head_len +
				    256 + /* TIM IE */
				    beacon->tail_len +
				    local->hw.extra_beacon_tailroom);
		if (!skb)
			goto out;
		skb_reserve(skb, local->tx_headroom);
		memcpy(skb_put(skb, beacon->head_len), beacon->head,
		       beacon->head_len);
		dragonite_beacon_add_tim((struct mac80211_dragonite_data *)hw->priv, sdata, &ifmsh->ps, skb, vp->bssid_idx);

#if 0
		if (offs) {
			offs->tim_offset = beacon->head_len;
			offs->tim_length = skb->len - beacon->head_len;
		}
#endif

		memcpy(skb_put(skb, beacon->tail_len), beacon->tail,
		       beacon->tail_len);
#endif
	} else {
		WARN_ON(1);
		goto out;
	}

	band = chanctx_conf->def.chan->band;

	info = IEEE80211_SKB_CB(skb);

	info->flags |= IEEE80211_TX_INTFL_DONT_ENCRYPT;
	info->flags |= IEEE80211_TX_CTL_NO_ACK;
	info->band = band;

	memset(&txrc, 0, sizeof(txrc));
	txrc.hw = hw;
	txrc.sband = local->hw.wiphy->bands[band];
	txrc.bss_conf = &sdata->vif.bss_conf;
	txrc.skb = skb;
	txrc.reported_rate.idx = -1;
	txrc.rate_idx_mask = sdata->rc_rateidx_mask[band];
	if (txrc.rate_idx_mask == (1 << txrc.sband->n_bitrates) - 1)
		txrc.max_rate_idx = -1;
	else
		txrc.max_rate_idx = fls(txrc.rate_idx_mask) - 1;
	txrc.bss = true;
	//rate_control_get_rate(sdata, NULL, &txrc);

	info->control.vif = vif;

	info->flags |= IEEE80211_TX_CTL_CLEAR_PS_FILT |
			IEEE80211_TX_CTL_ASSIGN_SEQ |
			IEEE80211_TX_CTL_FIRST_FRAGMENT;
 out:
	rcu_read_unlock();
	return skb;
}

static void dragonite_beacon_prepare(void *data, u8 *mac, struct ieee80211_vif *vif)
{
	struct beacon_prepare_args* args = (void *) data;
	struct ieee80211_hw *hw = args->hw;
	struct ieee80211_tx_info *info;
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	struct sk_buff *skb;
	u16 ssn;

	if(vp->bssinfo->dut_role != ROLE_AP) {
		return;
	}

	skb = dragonite_beacon_get_tim(hw, vif, 0, 0);
	if (skb == NULL) {
		return;
	}

	ssn = dragonite_tx_get_ssn(vp, NULL, NULL);
	if(!IS_INVALID_SSN(ssn))
	{
        /* replace on skb->data sequence number */
        dragonite_tx_replace_ssn(skb, ssn);
    }
	else
	{
		/* use mac80211 originally assigned sequence number */
	}
	info = IEEE80211_SKB_CB(skb);

	ASSERT( (NULL == args->beacon_skb[vp->bssid_idx]), "pending beacon skbs or duplicated bssid_idx\n");
	args->beacon_skb[vp->bssid_idx] = skb;
}

void dragonite_beacon(unsigned long arg)
{
	struct ieee80211_hw *hw = (struct ieee80211_hw *) arg;
	struct mac80211_dragonite_data *data = hw->priv;

	struct beacon_prepare_args args;
	int i;
	args.hw = hw;
	for(i=0;i<MAC_MAX_BSSIDS;i++)
		args.beacon_skb[i] = NULL;

	/* XXX: use ieee80211_iterate_active_interfaces() or ieee80211_iterate_active_interfaces_atomic() ? */
	ieee80211_iterate_active_interfaces_atomic(
		hw, 0, dragonite_beacon_prepare, &args);

	if(0 <= dragonite_tx_beacons(hw, args.beacon_skb))
	{
        dragonite_bss_ps_lock();

		for(i=0;i<MAX_BSSIDS;i++){

			dragonite_tx_bcq_handler(data, i);
		}
        dragonite_bss_ps_unlock();
	}


	for(i=0;i<MAC_MAX_BSSIDS;i++)
	{
		if(args.beacon_skb[i])
		{
			dev_kfree_skb_irq(args.beacon_skb[i]);
		}
	}
}

static irqreturn_t dragonite_isr(int irq, void *data)
{
	struct mac80211_dragonite_data *pdata = data;
#if defined(DRAGONITE_BEACON_IN_ISR)
	u32 status;
#endif
#if defined (DRAGONITE_ISR_CHECK)
	unsigned long curr_jiffies;
	u32 tx_acq_status, tx_acq_status2, ssn, esn;
    int acqid, cmdid;
    acq *q;
#endif

	pdata->isr_handle = true;

	if(!pdata->started)
	{
		DRG_NOTICE(DRAGONITE_DEBUG_SYSTEM_FLAG, "dragonite_isr done\n");
		pdata->isr_handle = false;
		goto exit;
	}

	disable_mac_interrupts();

#if defined (DRAGONITE_ISR_CHECK)
	curr_jiffies = jiffies;

	if(curr_jiffies == drg_isr_prev_jiffies)
	{
		drg_isr_count++;
		status = MACREG_READ32(MAC_INT_STATUS);
		if(status & MAC_INT_ACQ_TX_DONE) {
			drg_tx_done_isr_count++;
		}
		if(status & MAC_INT_ACQ_RETRY_FAIL) {
			drg_tx_fail_isr_count++;
		}
		if(status & MAC_INT_ACQ_CMD_SWITCH) {
			drg_tx_switch_isr_count++;
		}
		if(status & MAC_INT_TSF) {
			drg_tsf_isr_count++;
		}
		if(status & MAC_INT_SW_RX) {
			drg_rx_isr_count++;
		}
		if(status & MAC_INT_RX_DESCR_FULL) {
			drg_rx_descr_full_isr_count++;
		}
		if(!(status & (MAC_INT_ACQ_TX_DONE | MAC_INT_ACQ_RETRY_FAIL | MAC_INT_ACQ_CMD_SWITCH |
					 MAC_INT_TSF | MAC_INT_SW_RX | MAC_INT_RX_DESCR_FULL))) {
			/* indicate nobody but interrupt */
			drg_nobody_isr_count++;
		}
	}
	else
	{
		if(drg_isr_count > DRAGONITE_ISR_TOLERANT_COUNT_PER_JIFFIES)
		{
			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "ISR too much, disable interrupt !!\n");
			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "isr count %08x tx done %08x tx fail %08x tx switch %08x\n",
					 drg_isr_count, drg_tx_done_isr_count, drg_tx_fail_isr_count, drg_tx_switch_isr_count);
			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tsf %08x rx %08x rx descr full %08x nobody %08x per jiffies\n",
					 drg_tsf_isr_count, drg_rx_isr_count, drg_rx_descr_full_isr_count, drg_nobody_isr_count);
			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "rx actual prev %08x current %08x\n", drg_rx_prev_count, drg_rx_count);

			tx_acq_status = MACREG_READ32(ACQ_INTR);
			tx_acq_status2 = MACREG_READ32(ACQ_INTR2);

			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "tx_acq_status %08x tx_acq_status2 %08x\n",
					 tx_acq_status, tx_acq_status2);

			for(acqid=0;acqid<ACQ_NUM;acqid++)
			{
                for(cmdid=0;cmdid<CMD_NUM;cmdid++)
				{
                    if(tx_acq_status & ACQ_INTR_BIT(acqid,cmdid))
					{
                        q = (acq *) (pdata->mac_info->acq_hw_requested[acqid][cmdid]);
                        ssn = MACREG_READ32(ACQ_INFO(acqid,cmdid)) & 0x0fff;
                        esn = MACREG_READ32(ACQ_INFO2(acqid,cmdid)) & 0x0fff;
						DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "acqid %d cmdid %d, ssn %d, esn %d\n", acqid, cmdid, ssn, esn);
						if(q) {
                            acq_dump(q);
						}
						else
						{
							DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "q == NULL\n");
						}

					}
				}
			}
			drg_isr_count = 0;
			drg_tx_done_isr_count = 0;
			drg_tx_fail_isr_count = 0;
			drg_tx_switch_isr_count = 0;
			drg_tsf_isr_count = 0;
			drg_rx_isr_count = 0;
			drg_rx_descr_full_isr_count = 0;
			drg_rx_prev_count = drg_rx_count;
			drg_nobody_isr_count = 0;

			pdata->isr_handle = false;
			goto exit;
		}
		else
		{
			drg_isr_count = 0;
			drg_tx_done_isr_count = 0;
			drg_tx_fail_isr_count = 0;
			drg_tx_switch_isr_count = 0;
			drg_tsf_isr_count = 0;
			drg_rx_isr_count = 0;
			drg_rx_descr_full_isr_count = 0;
			drg_rx_prev_count = drg_rx_count;
			drg_nobody_isr_count = 0;
		}
	}
	drg_isr_prev_jiffies = curr_jiffies;
#endif

#if defined(DRAGONITE_BEACON_IN_ISR)
	status = MACREG_READ32(MAC_INT_STATUS);

	if(status & MAC_INT_TSF)
	{
		MACREG_WRITE32(MAC_INT_CLR, MAC_INT_TSF);
        dragonite_tsf_intr_handler(data);
	}
#endif

	tasklet_hi_schedule(&pdata->irq_tasklet);

exit:
	return IRQ_HANDLED;
}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
union {
	u64 val_64;
	u32 val_32[2];
} pre_tbtt, ts_now, ts_next, ts_o;
u64 pre_ts, pre_tbtt_remain;
ktime_t pre_ktime;
#endif
static void dragonite_irq_tasklet(struct mac80211_dragonite_data *data)
{
	u32 status;
#if defined(DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
	int idx;
	struct dragonite_vif_priv *vp;
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	bool aggr_ps = false;
#endif
#endif

	status = MACREG_READ32(MAC_INT_STATUS);

#if !defined(DRAGONITE_BEACON_IN_ISR)
	MACREG_WRITE32(MAC_INT_CLR, status);

	if(status & MAC_INT_TSF)
	{
		dragonite_tsf_intr_handler(data);
	}
#else
	MACREG_WRITE32(MAC_INT_CLR, (status & (~MAC_INT_TSF))); /* clear status without TSF */
#endif
	    /* handle SW TX return Queue */
    if (status & (MAC_INT_ACQ_TX_DONE | MAC_INT_ACQ_CMD_SWITCH | MAC_INT_ACQ_RETRY_FAIL))
	{
		dragonite_acq_intr_handler(data);
	}

	if(status & (MAC_INT_SW_RX | MAC_INT_RX_DESCR_FULL))
	{
#if defined (DRAGONITE_ISR_CHECK)
        if(status & MAC_INT_RX_DESCR_FULL)
        {
            drg_rx_full_count++;
            DRG_EMERG(DRAGONITE_DEBUG_SYSTEM_FLAG, "RX full %d\n", drg_rx_full_count);
        }
#endif
#if defined(DRAGONITE_ENABLE_OMNICFG)
        if(__omnicfg_filter_mode)
            omnicfg_process(data);
        else
            dragonite_process_rx_queue(data);
#else
        dragonite_process_rx_queue(data);
#endif
	}
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
	dragonite_pm_lock_lock();
	if(data->ps_enabled && !data->ps_flags) {
		/* wifi power off */
		if(!data->poweroff) {
			if(drg_wifi_poweroff_enable) {
				for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
				{
					vp = idx_to_vp(data, (u8) idx);
					if(vp == NULL)
						continue;
					if(vp->bssinfo->dut_role == ROLE_STA)
					{
						if(vp->bssinfo->beacon_interval != 0)
						{
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
							dragonite_calculate_pretbtt(vp->bssinfo->beacon_interval);
#endif
							if(dragonite_has_tx_pending(data)) {
								data->ps_flags |= PS_FORCE_WAKE_UP;
								break;
							}
							dragonite_ps_sleep(data);
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
							hrtimer_start(&data->tsf_timer, ktime_set(0, ((ts_next.val_64 - ts_now.val_64) - pre_tbtt_remain) * 1000), HRTIMER_MODE_REL);
							aggr_ps = true;
#endif
						}
					}
				}
			}
			else
			{
				DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "RF power-off (ignore)\n");
			}
		}
	}
	dragonite_pm_lock_unlock();
#endif

	data->isr_handle = false;

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	/* mac interrupt will be enable at power on */
	if(!aggr_ps) {
		enable_mac_interrupts();
	}
#else
	enable_mac_interrupts();
#endif
}
static void wifi_mac_reset_check(void)
{
	u32 swbl_baddr, acq_intr, acq_intr2;

	swbl_baddr = MACREG_READ32(SWBL_BADDR);

	acq_intr = MACREG_READ32(ACQ_INTR);

	acq_intr2 = MACREG_READ32(ACQ_INTR2);

	if(swbl_baddr || acq_intr || acq_intr2) {
		DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "swbl_baddr %08x, acq_intr %08x, acq_intr2 %08x\n",
				   swbl_baddr, acq_intr, acq_intr2);
		panic("DRAGONITE: wifi mac reset failed");
	}
}

static void pmu_reset_wifi_mac(void)
{
    unsigned long reset_device_ids[] = {  DEVICE_ID_WIFIMAC, 0 };

    pmu_reset_devices(reset_device_ids);

    return;
}

static void pmu_reset_bb(void)
{
    unsigned long reset_device_bb[] = {  DEVICE_ID_BBP, 0 };

    pmu_reset_devices(reset_device_bb);

    return;
}

extern unsigned char txvga_gain[14];
extern unsigned char txvga_gain_save[14];
extern unsigned char fofs;
extern unsigned char fofs_save;
extern unsigned char bg_txp_diff;
extern unsigned char ng_txp_diff;
extern unsigned char bg_txp_gap;
extern char fem_product_id[8];
extern int fem_en;
static unsigned char string_vga_converter(char c)
{
    unsigned char val;
    switch(c)
    {
        case 'a':
            val = 0xa;
            break;
        case 'b':
            val = 0xb;
            break;
        case 'c':
            val = 0xc;
            break;
        case 'd':
            val = 0xd;
            break;
        case 'e':
            val = 0xe;
            break;
        case 'f':
            val = 0xf;
            break;
        case '0':
            val = 0x0;
            break;
        case '1':
            val = 0x1;
            break;
        case '2':
            val = 0x2;
            break;
        case '3':
            val = 0x3;
            break;
        case '4':
            val = 0x4;
            break;
        case '5':
            val = 0x5;
            break;
        case '6':
            val = 0x6;
            break;
        default:
            val = 0x0;
            break;
    }
    return val;
}

static void panther_get_tx_config(void)
{
    char *str;
    char c;
    unsigned char val;
    int idx;

    if((str = strstr(arcs_cmdline, "txvga=")))
    {
        str += 6; // shift sizeof "txvga="
        for(idx = 0; idx < 14; idx++)
        {
            c = str[idx];
            val = string_vga_converter(c);
            txvga_gain[idx] = val & 0xf;
            txvga_gain_save[idx] =  txvga_gain[idx];
        }
    }
    if((str = strstr(arcs_cmdline, "fofs=")))
    {
        str += 5; // shift sizeof "fofs="
        sscanf(str, "%02x", (unsigned int *) &fofs);
        fofs_save = fofs;
    }
    if((str = strstr(arcs_cmdline, "txp_diff=")))
    {
        str += 9; // shift sizeof "txp_diff="
        if(str[0] >= '0' && str[0] <= '6')
            bg_txp_diff = str[0] - '0';
        else
            bg_txp_diff = 4;

        if(str[1] >= '0' && str[1] <= '6')
            ng_txp_diff = str[1] - '0';
        else
            ng_txp_diff = 0;

        if(str[2] == '3' && str[2] <= '6')
            bg_txp_gap = str[2] - '0';
        else
            bg_txp_gap = 6;
    }
    if((str = strstr(arcs_cmdline, "fem_product_id=")))
    {
        str += 15; // shift sizeof "fem_product_id="
        sscanf(str, "%s", fem_product_id);
    }
    if((str = strstr(arcs_cmdline, "fem_en=")))
    {
        str += 7; // shift sizeof "fem_en="
        sscanf(str, "%d", &fem_en);
    }
    else
    {
        fem_en = 1;
    }

    for(idx = 0; idx < 14; idx++)
        printk("channel %d txpwr %02x\n", idx+1, txvga_gain[idx]);
    printk("fofs %d\n", fofs);
    printk("txp_diff %d %d %d\n", bg_txp_diff, ng_txp_diff, bg_txp_gap);
    printk("fem_product_id %s\n", fem_product_id);
    printk("fem_en %d\n", fem_en);
}
#if defined (DRAGONITE_POWERSAVE_AGGRESSIVE)
static enum hrtimer_restart soft_tsf(struct hrtimer *timer)
{
	struct mac80211_dragonite_data *data = global_data;

	dragonite_pm_lock_lock();

	DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "soft preTBTT\n");

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_WAIT_FOR_BEACON;
	data->ps_flags &= ~PS_WAIT_FOR_MULTICAST;

	dragonite_correct_ts(true);

	dragonite_pm_lock_unlock();

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart dragonite_ps_sleep_full(struct hrtimer *timer)
{
	struct mac80211_dragonite_data *data = global_data;

	dragonite_pm_lock_lock();

	DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "start full sleep\n");

	if(data->poweroff)
	{
		if(dragonite_has_tx_pending(data)) {
			panic("dragonite_ps_sleep_full in dragonite_ps_sleep");
		}
		rf_power_off_aggressive();
	}

	dragonite_pm_lock_unlock();

	return HRTIMER_NORESTART;
}
#endif
#ifndef CONFIG_MONTE_CARLO
extern void panther_rfc_process(void);
#endif
static int mac80211_dragonite_start(struct ieee80211_hw *hw)
{
	struct mac80211_dragonite_data *data = hw->priv;
	int bssid;
	int ret = 0;

	wiphy_debug(hw->wiphy, "%s\n", __func__);

	DRG_NOTICE(DRAGONITE_DEBUG_SYSTEM_FLAG, "=====> mac80211_dragonite_start\n");

	mutex_lock(&data->mutex);

	if(!data->started)
	{
		if(!drg_rf_bb_init_once)
		{
			rf_init();
			bb_init();
#ifdef CONFIG_MONTE_CARLO
			bb_register_write(0, 0x5A, 0);//for Combination with panther FPGA with Monte calro
#else
			panther_rfc_process();
#endif
			schedule_timeout_interruptible(HZ/10);
			pmu_reset_wifi_mac();
			wifi_mac_reset_check();
			schedule_timeout_interruptible(HZ/10);
		}
		data->mac_info = dragonite_mac_init();

		if(NULL == data->mac_info)
		{
			DRG_ERROR(DRAGONITE_DEBUG_SYSTEM_FLAG, "Mac init failed\n");
			ret = -EIO;
			goto exit;
		}

		if(fem_en)
			panther_fem_init(fem_en);
		else
			panther_without_fem_init();

		dragonite_mac_lock_init();
		dragonite_txq_lock_init();
		dragonite_ps_lock_init();
		dragonite_bss_ps_lock_init();
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
		dragonite_pm_lock_init();
#endif

		spin_lock_init(&data->tx_per_ac_count_lock);

		memset(&drg_tx_stats, 0 , sizeof(struct _drg_tx_stats));

		drg_tx_stats.time_space = DRG_TX_STATS_DEFAULT_TIME_SPACE;

		for(bssid = 0; bssid < MAX_BSSIDS; bssid++) {
			data->mac_info->bcq_status[bssid] = DRG_BCQ_DISABLE;
			data->mac_info->bss_ps_status[bssid] = DRG_BSS_PS_OFF;
		}

		if(0 != dragonite_kthread_init(data)) {
			DRG_ERROR(DRAGONITE_DEBUG_SYSTEM_FLAG, "Kthread init failed\n");
			/* TODO: should remove data->mac_info */
			ret = -EIO;
			goto exit;
		}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
		hrtimer_init(&data->tsf_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		data->tsf_timer.function = soft_tsf;
		hrtimer_init(&data->ps_sleep_full_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		data->ps_sleep_full_timer.function = dragonite_ps_sleep_full;
#endif

#if defined (DRAGONITE_TX_HANG_CHECKER)
		dragonite_init_tx_hang_checker(data);
#endif

#if defined (DRAGONITE_TX_SKB_CHECKER)
		memset(data->tx_skb_queue, 0, sizeof(struct skb_list_head));
#endif
		memset(data->tx_sw_free_queue, 0, sizeof(struct skb_list_head));

		memset(data->tx_sw_mixed_queue, 0, sizeof(struct skb_list_head) * 5);

		memset(data->tx_hw_mixed_queue, 0, sizeof(struct skb_list_head) * 5);

		memset(data->tx_sw_ps_mixed_queue, 0, sizeof(struct skb_list_head) * 5);

		data->tx_sw_queue_total_count = 0;
		data->tx_sw_bmc_queue_total_count = 0;


		memset((void *) data->tx_per_ac_count, 0, sizeof(int) * 5);

		tasklet_init(&data->irq_tasklet, (void (*)(unsigned long))
				dragonite_irq_tasklet, (unsigned long)hw->priv);

		if(0 != request_irq(IRQ_WIFI, dragonite_isr, 0, DRV_NAME, hw->priv))
		{
			DRG_ERROR(DRAGONITE_DEBUG_SYSTEM_FLAG, "Request irq failed\n");
			/* TODO: should remove data->mac_info & kthread */
			ret = -EIO;
			goto exit;
		}

		mat_init();

		data->started = true;
        data->isr_handle = false;
		dragonite_mac_tx_rx_enable();
		dragonite_tx_start();
		dragonite_mac_start(data->mac_info);

		DRG_NOTICE(DRAGONITE_DEBUG_SYSTEM_FLAG, "Wifi start\n");
	}
	else
	{
		ret = -EIO;
	}

exit:
	mutex_unlock(&data->mutex);

	return ret;
}

static void wifi_stop_prepare(struct mac80211_dragonite_data *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    struct irq_data irq_wifi = { .irq = IRQ_WIFI };
#endif
	struct irq_desc *desc = irq_to_desc(IRQ_WIFI);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    disable_irq(irq_wifi.irq);
#else
	disable_irq(IRQ_WIFI);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	ASSERT((desc->irq_data.common->state_use_accessors & IRQD_IRQ_DISABLED), "IRQ NOT DISABLED YET\n");
#else
	ASSERT((desc->status & IRQ_DISABLED), "IRQ NOT DISABLED YET\n");
#endif

	dragonite_txq_lock();

	wifi_tx_enable = 0;

	dragonite_txq_unlock();
	data->started = false;

	while(data->isr_handle == true) {
		DRG_DBG("dragonite isr and tasklet still working\n");
		schedule_timeout_interruptible(HZ/100);
	}

	schedule_timeout_interruptible(HZ/100);

}
extern u32 drg_wifi_sniffer;
static void mac80211_dragonite_stop(struct ieee80211_hw *hw)
{
	struct mac80211_dragonite_data *data = hw->priv;
	int t_wait = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    struct irq_data irq_wifi = { .irq = IRQ_WIFI };
#endif

	wiphy_debug(hw->wiphy, "%s\n", __func__);

    DRG_NOTICE(DRAGONITE_DEBUG_SYSTEM_FLAG, "=====> mac80211_dragonite_stop\n");

    mutex_lock(&data->mutex);

	if(data->started)
	{
		while((volatile u32 ) drg_wifi_sniffer)
		{
			DRG_NOTICE(DRAGONITE_DEBUG_SYSTEM_FLAG, "disable sniffer mode, please wait\n");
			mutex_unlock(&data->mutex);

			omnicfg_stop();/* release lock before omnicfg_stop */

			if(t_wait > 5)
			{
				DRG_ERROR(DRAGONITE_DEBUG_SYSTEM_FLAG, "driver stop incompletely, ERROR !!!\n");
			}
			schedule_timeout_interruptible(HZ/5);
			t_wait++;

			mutex_lock(&data->mutex);
		}
		wifi_stop_prepare(data);

		mat_flush();

		dragonite_mac_stop(data->mac_info);
        dragonite_tx_stop();
        dragonite_mac_tx_rx_disable();

        schedule_timeout_interruptible(HZ/10);

        reset_mac_registers();

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
        enable_irq(irq_wifi.irq);
#else
        enable_irq(IRQ_WIFI);
#endif

        free_irq(IRQ_WIFI, hw->priv);
		tasklet_kill(&data->irq_tasklet);

		dragonite_kthread_cleanup(data);

		dragonite_tx_cleanup_pending_skb(data, !CHECK_OUT_HW_QUE);

		dragonite_tx_release_skb_queue(data, 0, true);

		dragonite_mac_release(data->mac_info);

		data->mac_info = NULL;

        pmu_reset_wifi_mac();

        if(0 != mac_malloc_info((char *)__func__, false))
        {
            mac_malloc_info((char *)__func__, true);
            panic("memory leak detected in dragonite driver\n");
        }
    }

	mutex_unlock(&data->mutex);
}
static void _ieee80211_sta_tear_down_tx_BA_sessions(struct sta_info *sta,
					 enum ieee80211_agg_stop_reason reason)
{
	int i;

	cancel_work_sync(&sta->ampdu_mlme.work);

	for (i = 0; i <  IEEE80211_NUM_TIDS; i++) {
		__ieee80211_stop_tx_ba_session(sta, i, reason);
	}
}
void recover_ampdu(struct mac80211_dragonite_data *data, enum dragonite_recover_ampdu_flags flags)
{
    struct ieee80211_hw *hw = data->hw;
    struct ieee80211_local *local = hw_to_local(hw);
    struct sta_info *sta;
    int n = 0;

	DRG_DBG("start to recover_ampdu\n");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    if (ieee80211_hw_check(hw, AMPDU_AGGREGATION))
#else
    if (hw->flags & IEEE80211_HW_AMPDU_AGGREGATION)
#endif
    {
        mutex_lock(&local->sta_mtx);
        list_for_each_entry(sta, &local->sta_list, list)
        {
            n++;
            switch(flags)
            {
                case SET_BLOCK_BA_FLAG:
                    set_sta_flag(sta, WLAN_STA_BLOCK_BA);
                    break;
                case TEAR_DOWN_BA_SESSIONS:
                    //ieee80211_sta_tear_down_tx_BA_sessions(sta, AGG_STOP_LOCAL_REQUEST);
                    /* only tear down tx ampdu, but not rx for Realtek compatibility */
                    _ieee80211_sta_tear_down_tx_BA_sessions(sta, AGG_STOP_LOCAL_REQUEST);
                    break;
                case CLEAR_BLOCK_BA_FLAG:
                    clear_sta_flag(sta, WLAN_STA_BLOCK_BA);
                    break;
                default:
                    DRG_DBG("recover_ampdu: Unsupported flags %d\n", flags);
            }
        }
        mutex_unlock(&local->sta_mtx);
    }

    switch(flags)
    {
        case SET_BLOCK_BA_FLAG:
            DRG_DBG("recover_ampdu: set %d sta block ba\n", n);
            break;
        case TEAR_DOWN_BA_SESSIONS:
            DRG_DBG("recover_ampdu: tear down %d sta\n", n);
            break;
        case CLEAR_BLOCK_BA_FLAG:
            DRG_DBG("recover_ampdu: clear %d sta block ba\n", n);
            break;
        default:
            DRG_DBG("recover_ampdu: Unsupported flags %d\n", flags);
    }

	DRG_DBG("finish to recover_ampdu\n");

    return;
}

void clear_all_sp_vp_magic(struct mac80211_dragonite_data *data)
{
    MAC_INFO *info = data->mac_info;
    u8 idx;
    struct dragonite_sta_priv *sp;
    struct dragonite_vif_priv *vp;

    for(idx = 0; idx < info->sta_idx_msb; idx++)
    {
        sp = idx_to_sp(data, idx);
        if(sp)
        {
            dragonite_clear_sp_magic(sp);
        }
    }
    for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(data, idx);
        if(vp)
        {
            dragonite_clear_vp_magic(vp);
        }
    }

    return;
}

void set_all_sp_vp_magic(struct mac80211_dragonite_data *data)
{
    MAC_INFO *info = data->mac_info;
    u8 idx;
    struct dragonite_sta_priv *sp;
    struct dragonite_vif_priv *vp;

    for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(data, idx);
        if(vp)
        {
            dragonite_set_vp_magic(vp);
        }
    }
    for(idx = 0; idx < info->sta_idx_msb; idx++)
    {
        sp = idx_to_sp(data, idx);
        if(sp)
        {
            dragonite_set_sp_magic(sp);
        }
    }

    return;
}

void change_sta_ps_status(struct mac80211_dragonite_data *data)
{
    MAC_INFO *info = data->mac_info;
    u8 idx;
    struct dragonite_sta_priv *sp;

    for(idx = 0; idx < info->sta_idx_msb; idx++)
    {
        if(!(info->sta_idx_bitmap & (0x1UL << idx)))
        {
			continue;
        }

        sp = idx_to_sp(data, idx);
        ASSERT((sp != NULL), "change_sta_ps_status: sp is NULL\n");
        if(!sp->sta_mode)
        {
			DRG_DBG("recover sta %d to ps on\n", sp->stacap_index);
			info->sta_ps_recover |= (0x1UL << sp->stacap_index);
			info->sta_ps_off_indicate &= ~(0x1UL << sp->stacap_index);
			info->sta_ps_on |= (0x1UL << sp->stacap_index);
			dragonite_tx_swq_to_psq(data, sp->stacap_index);
        }
		else
		{
			DRG_DBG("recover ap %d to ps on but ready for ps off\n", sp->stacap_index);
			info->sta_ps_off_indicate |= (0x1UL << sp->stacap_index);
			info->sta_ps_on |= (0x1UL << sp->stacap_index);
			dragonite_tx_swq_to_psq(data, sp->stacap_index);
		}
    }

    return;
}

void change_vif_ps_status(struct mac80211_dragonite_data *data)
{
    u8 idx;
    struct dragonite_vif_priv *vp;

    for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(data, idx);
        if(vp)
        {
			if(vp->bssinfo->dut_role == ROLE_AP) {
				data->mac_info->bss_ps_status[idx] = DRG_BSS_PS_ON;
				data->mac_info->bcq_status[idx] = DRG_BCQ_ENABLE;
				DRG_DBG("enable bss %d bcq\n", idx);
			}
        }
    }

    return;
}

#if defined (DRAGONITE_ACQ_DELAY_KICK)
void cancel_pending_acq_kick(struct mac80211_dragonite_data *data)
{
	MAC_INFO *info = data->mac_info;
	u8 idx, tid;
	acq *q;
	struct dragonite_sta_priv *sp;

	for (idx = 0; idx < info->sta_idx_msb; idx++)
	{
		sp = idx_to_sp(data, idx);
		if (sp)
		{
			for (tid = 0; tid < TID_MAX; tid++)
			{
				if ((q = SINGLE_Q(sp, tid)) != NULL)
				{
					hrtimer_cancel(&q->tx_acq_timer);
				}
			}
		}
	}
	return;
}
#endif

void cleanup_sp_sw_queue(struct mac80211_dragonite_data *data)
{
    MAC_INFO *info = data->mac_info;
    u8 idx;
    struct dragonite_sta_priv *sp;

    for(idx = 0; idx < info->sta_idx_msb; idx++)
    {
        sp = idx_to_sp(data, idx);
        if(sp)
        {
            dragonite_tx_detach_node(data, sp);
        }
    }

    return;
}

void cleanup_vp_sw_queue(struct mac80211_dragonite_data *data)
{
    u8 idx;
    struct dragonite_vif_priv *vp;

    for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(data, idx);
        if(vp)
        {
            dragonite_tx_detach_vif_node(data, vp);
        }
    }

    return;
}

void clear_hw_requested_flags(struct mac80211_dragonite_data *data)
{
    MAC_INFO *info = data->mac_info;
	int i;
    volatile acq *q;

    for(i = 0; i < ACQ_NUM; i++)
    {
       q = (volatile acq*) &info->def_acq[i];
       while(q != NULL)
       {
           q->flags &= ~(ACQ_FLAG_ACTIVE|ACQ_FLAG_CMD0_REQ|ACQ_FLAG_CMD1_REQ);
#if defined (DRAGONITE_ACQ_BAN)
           if(q->qinfo & ACQ_SINGLE) {
               q->flags |= ACQ_FLAG_BAN;
           }
#endif
           q = q->next;
       }
    }

    return;
}

void dragonite_beacon_recover(struct mac80211_dragonite_data *data)
{
    int idx;
    struct dragonite_vif_priv *vp;

    for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
    {
        vp = idx_to_vp(data, (u8) idx);
        if(vp == NULL)
        {
            continue;
        }
        if(vp->bssinfo->dut_role == ROLE_STA)
        {
			if(vp->bssinfo->beacon_interval != 0) {
				dragonite_cfg_start_tsync(vp->bssinfo->timer_index, vp->bssinfo->beacon_interval, vp->bssinfo->dtim_period, 0);
			}
        }
        else if(vp->bssinfo->dut_role == ROLE_AP)
        {
            if(vp->bssinfo->beacon_interval && vp->bssinfo->dtim_period)
            {
                dragonite_beacon_setup(vp->bssinfo->beacon_interval, vp->bssinfo->dtim_period);
            }
        }
    }
    if(data->enable_beacon)
    {
        dragonite_beacon_start(0x20);
    }
    else
    {
        dragonite_beacon_stop();
    }

    return;
}

void dragonite_pmu_mac_reset(void)
{
	pmu_reset_wifi_mac();
}
#if defined (DRAGONITE_HW_DEBUG_REGS_DUMP)
void hw_debug_regs_dump(void)
{
	int idx, size;
	unsigned int regs_val[] = { 0x0000, 0x1111, 0x2222, 0x3333,
		0x4444, 0x5555, 0x6666, 0x7777,
		0x8888, 0x9999, 0xaaaa, 0xbbbb,
		0xcccc, 0xdddd, 0xeeee, 0xffff };

	DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "dump RX debug register\n");
	size = sizeof(regs_val)/sizeof(unsigned int);
	for (idx = 0; idx < size; idx++) {
		MACREG_WRITE32(DEBUG_GROUP_SEL, regs_val[idx]);
		DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "0xbf003b44[0x%08x]-->0xbf003b6c[0x%08x]\n",
				MACREG_READ32(DEBUG_GROUP_SEL), MACREG_READ32(DEBUG_MMACRX));
	}
	DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "dump TX debug register\n");
	for (idx = 0; idx < size; idx++) {
		MACREG_WRITE32(DEBUG_PORT_SEL, regs_val[idx]);
		DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "0xbf003efc[0x%08x]-->0xbf003e28[0x%08x]\n",
				MACREG_READ32(DEBUG_PORT_SEL), MACREG_READ32(UTX_DEBUG));
	}
	DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "dump acq debug register\n");
	MACREG_WRITE32(MAC_DEBUG_SEL, 0xdddd);
	for (idx = 0; idx < 0x10; idx++) {
		MACREG_WRITE32(0xce0, idx);
		DRG_NOTICE(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "0xbf003ce0[0x%08x]-->0xbf003908[0x%08x]\n",
				MACREG_READ32(0xce0), MACREG_READ32(MAC_DEBUG_OUT0));
	}
}
#endif
#define TIME_NEXT_RECOVER_ALLOWED_MSEC  (2000)
unsigned long prev_recover_jiffies = 0;
int drg_wifi_recover_cnt = 0;
int drg_ps_force_clr_cnt = 0;
extern void enable_sniffer_mode(void);
extern void disable_sniffer_mode(void);
extern void dragonite_bb_suspend(void);
extern void dragonite_bb_resume(void);
void dragonite_wifi_recover(struct mac80211_dragonite_data *data)
{
    MAC_INFO *info = data->mac_info;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    struct irq_data irq_wifi = { .irq = IRQ_WIFI };
#endif

 	if(prev_recover_jiffies)
 	{
 		if(time_is_before_jiffies(prev_recover_jiffies + msecs_to_jiffies(TIME_NEXT_RECOVER_ALLOWED_MSEC)))
 		{
 			prev_recover_jiffies = jiffies;
 		}
 		else
 		{
 			return;
 		}
 	}
 	else
 	{
 		prev_recover_jiffies = jiffies;
 	}

    mutex_lock(&data->mutex);

	if(data->started)
	{
		drg_wifi_recover_cnt++;
#if defined (DRAGONITE_ISR_CHECK)
		DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Wifi recover %d times, force clear ps state %d times, rx full %d times\n",
			   drg_wifi_recover_cnt, drg_ps_force_clr_cnt, drg_rx_full_count);
#else
		DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Wifi recover %d times, force clear ps state %d times\n", 
			   drg_wifi_recover_cnt, drg_ps_force_clr_cnt);
#endif
		wifi_stop_prepare(data);

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
		dragonite_pm_lock_lock();
		if(data->poweroff) {
			   dragonite_ps_wakeup(data);
			   data->ps_flags |= PS_FORCE_WAKE_UP;
		}
		dragonite_pm_lock_unlock();
#endif
#if defined (DRAGONITE_ACQ_DELAY_KICK)
		cancel_pending_acq_kick(data);
#endif
#if defined (DRAGONITE_HW_DEBUG_REGS_DUMP)
		hw_debug_regs_dump();
#endif

		dragonite_mac_stop(data->mac_info);
		dragonite_tx_stop();
		dragonite_mac_tx_rx_disable();

		schedule_timeout_interruptible(HZ/100);

		dragonite_bb_suspend();

		pmu_reset_wifi_mac();

		pmu_reset_bb();

		dragonite_bb_resume();

		wifi_mac_reset_check();

		clear_all_sp_vp_magic(data);

		cleanup_sp_sw_queue(data);
		change_sta_ps_status(data);

		cleanup_vp_sw_queue(data);
		change_vif_ps_status(data);

		dragonite_tx_cleanup_pending_skb(data, !CHECK_OUT_HW_QUE);
		clear_hw_requested_flags(data);
		check_hw_queue(data);
		set_all_sp_vp_magic(data);

		dragonite_mac_recover(data);

#if defined (DRAGONITE_TX_HANG_CHECKER)
		dragonite_init_tx_hang_checker(data);
#endif
		data->started = true;
		data->isr_handle = false;

		if(drg_wifi_sniffer || __airkiss_filter_mode) {
		    enable_sniffer_mode();
		}
		else
		{
		    disable_sniffer_mode();
		}
		dragonite_mac_tx_rx_enable();
		dragonite_tx_start();
		dragonite_mac_start(data->mac_info);

		dragonite_beacon_recover(data);
		DRG_NOTICE(DRAGONITE_DEBUG_SYSTEM_FLAG, "Recover Beacon done...\n");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
        enable_irq(irq_wifi.irq);
#else
        enable_irq(IRQ_WIFI);
#endif

		mutex_unlock(&data->mutex);

		DRG_NOTICE(DRAGONITE_DEBUG_SYSTEM_FLAG, "Recover MAC done...\n");

		if(info->sta_ampdu_recover)
		{
			recover_ampdu(data, TEAR_DOWN_BA_SESSIONS);
			info->sta_ampdu_recover = false;
		}
	}
	else
	{
        DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Wifi not started yet !!\n");
		mutex_unlock(&data->mutex);
	}
    return;
}

int find_mac_bssid(struct __bss_info *bss_info, u8 *macaddr)
{
    int i, ret;

	for(i=0;i<MAC_MAX_BSSIDS;i++) {

		ret = memcmp(bss_info[i].MAC_ADDR, macaddr, ETH_ALEN);

		if(ret==0) {
			return i;
		}
	}
	return -1;
}

static void dragonite_warn_on_default_mac_address(u8 *addr)
{
    u8 default_macaddr[] = { 0x80, 0x50, 0xdf, 0x00, 0x00, 0x00 };

    if(!memcmp(default_macaddr, addr, 5))
    {
        printk(KERN_EMERG "\n\n\n\n\n!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!\n");
        printk(KERN_EMERG "!!!Using default MAC address could cause problem!!!\n\n\n\n\n");
        schedule_timeout_interruptible(2 * HZ);
        printk(KERN_EMERG "\n\n\n\n\n!!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!\n");
        printk(KERN_EMERG "!!Please change the MAC address(es) of the system!!\n\n\n\n\n");
        schedule_timeout_interruptible(2 * HZ);
    }
}

static int mac80211_dragonite_add_interface(struct ieee80211_hw *hw,
					struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_data *data = hw->priv;
	struct dragonite_vif_priv *vp = (void *) vif->drv_priv;	

	int bssid_idx;
	int timer_idx;
	int ret = 0;
    int role = ROLE_NONE;
	int i;

	wiphy_debug(hw->wiphy, "%s (type=%d mac_addr=%pM)\n",
		    __func__, ieee80211_vif_type_p2p(vif),
		    vif->addr);

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Add interface mac addr %pM\n", vif->addr);

    switch(vif->type) 
    {   
        case NL80211_IFTYPE_AP:
#if defined(CONFIG_MAC80211_MESH)
        case NL80211_IFTYPE_MESH_POINT:
#endif
        case NL80211_IFTYPE_P2P_GO:
        case NL80211_IFTYPE_MONITOR:
            role = ROLE_AP;
            break;
        case NL80211_IFTYPE_P2P_CLIENT:
        case NL80211_IFTYPE_P2P_DEVICE:
        case NL80211_IFTYPE_STATION:
            role = ROLE_STA;
            break;
        case NL80211_IFTYPE_AP_VLAN:
        case NL80211_IFTYPE_ADHOC:
            return -ENOTSUPP;
            break;
        default:
            return -ENOTSUPP;
            break;
    }

	mutex_lock(&data->mutex);

	if(!data->started)
	{
        DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Not started yet !!\n");
        ret = -EIO;
		goto exit2;
	}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	if(drg_tx_winsz <= IEEE80211_MAX_AMPDU_BUF) {
		hw->max_tx_aggregation_subframes = (drg_tx_winsz/8) * 8;
	}
	if(drg_rx_winsz <= IEEE80211_MAX_AMPDU_BUF) {
		hw->max_rx_aggregation_subframes = (drg_rx_winsz/8) * 8;
	}

	dragonite_mac_lock();

	if(data->bssid_count < MAC_MAX_BSSIDS)
    {
		if(data->bssid_count>0)
		{
			if((i = find_mac_bssid(data->dragonite_bss_info, vif->addr))>=0)
			{
				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "MAC ADDRESS DUPLICATE !!\n");
                ret = -EEXIST;
				goto exit;
			}
		}

		for(i=0; i<MAC_MAX_BSSIDS; i++) {

			if(!data->dragonite_bss_info[i].occupy) {

				bssid_idx = i;
				break;
			}
		}

		if(i>=MAC_MAX_BSSIDS) {
			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Too much interface !!\n");
            ret = -EBUSY;
			goto exit;
		}
		else{
			DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Program bssid %d !!\n", bssid_idx);
			if(role == ROLE_AP) {
				timer_idx = 0; /* AP mode timer index must be 0 */
			}
			else
			{
				//timer_idx = bssid_idx+1; /* STA mode timer index be bss index plus one */
				/* Panther A2: Only TSF3 could be used on WiFi STA wakeup feature
				   Other TSFs have time-violations on WiFi wakeup function.
				 */
				if(role == ROLE_STA)
					timer_idx = 3;
				else
					timer_idx = 1;
			}
			DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "BSS role %d, bssid %d, timer_idx %d\n", role, bssid_idx, timer_idx);
			dragonite_warn_on_default_mac_address(vif->addr);
			mac_program_bssid(vif->addr, bssid_idx, role, timer_idx);
			data->bssid_count++;
			wifi_tx_enable++;
		}
	}
	else
	{
		DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Too much interface !!\n");
		ret = -EBUSY;
		goto exit;
	}

	dragonite_set_magic(vif);

    vif->cab_queue = 0;
    vif->hw_queue[IEEE80211_AC_VO] = 0;
    vif->hw_queue[IEEE80211_AC_VI] = 1;
    vif->hw_queue[IEEE80211_AC_BE] = 2;
    vif->hw_queue[IEEE80211_AC_BK] = 3;

	data->vif_priv_addr[bssid_idx] = (u32) vp;
	vp->bssid_idx = bssid_idx;
	vp->bssinfo = &data->dragonite_bss_info[bssid_idx];

	memset(vp->bssinfo, 0, sizeof(struct __bss_info));

	memset(vp->tx_sw_gc_queue, 0, sizeof(struct skb_list_head) * (ACQ_NUM-1));
	memset(&vp->tx_sw_ps_gc_queue, 0, sizeof(struct skb_list_head));
	memset(&vp->tx_hw_ps_gc_queue, 0, sizeof(struct skb_list_head));

	memset(vp->tx_sw_queue, 0, sizeof(struct skb_list_head) * (TID_MAX+1));
	memset(vp->tx_sw_mgmt_queue, 0, sizeof(struct skb_list_head) * (TID_MAX+1));
	memset(vp->tx_hw_mgmt_queue, 0, sizeof(struct skb_list_head) * (TID_MAX+1));

	vp->bssinfo->occupy = true;

	vp->bssinfo->dut_role = role;

	vp->type = vif->type;

	memcpy(vp->bssinfo->MAC_ADDR, vif->addr, ETH_ALEN);

	vp->bssinfo->timer_index= timer_idx;

    vp->rekey_id = 0;

	vp->ssn = 0;

	spin_lock_init(&vp->vplock);

	if(role == ROLE_AP)
		mac_apply_default_wmm_ap_parameters();
	else
		mac_apply_default_wmm_station_parameters();

	dragonite_mac_unlock();

	dragonite_bss_ps_lock();

	data->mac_info->bcq_status[bssid_idx] = DRG_BCQ_DISABLE;
	data->mac_info->bss_ps_status[bssid_idx] = DRG_BSS_PS_OFF;

	dragonite_bss_ps_unlock();

	mutex_unlock(&data->mutex);

	return ret;

exit:
	dragonite_mac_unlock();

exit2:
	mutex_unlock(&data->mutex);

	return ret;
}


static int mac80211_dragonite_change_interface(struct ieee80211_hw *hw,
					   struct ieee80211_vif *vif,
					   enum nl80211_iftype newtype,
					   bool newp2p)
{
	newtype = ieee80211_iftype_p2p(newtype, newp2p);
	wiphy_debug(hw->wiphy,
		    "%s (old type=%d, new type=%d, mac_addr=%pM)\n",
		    __func__, ieee80211_vif_type_p2p(vif),
		    newtype, vif->addr);
	dragonite_check_magic(vif);

	/*
	 * interface may change from non-AP to AP in
	 * which case this needs to be set up again
	 */
	vif->cab_queue = 0;

	return 0;
}

static void mac80211_dragonite_remove_interface(
	struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_data *data = hw->priv;
	struct dragonite_vif_priv *vp = (void *) vif->drv_priv;
	int bssid_idx;
	unsigned long irqflags;

	wiphy_debug(hw->wiphy, "%s (type=%d mac_addr=%pM)\n",
		    __func__, ieee80211_vif_type_p2p(vif),
		    vif->addr);

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Remove interface mac addr %pM\n", vif->addr);

	mutex_lock(&data->mutex);

	if(!data->started)
    {
		DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Not started yet !!\n");
		goto exit;
	}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	dragonite_check_magic(vif);
	dragonite_clear_magic(vif);

    dragonite_mac_lock();

	if((bssid_idx = find_mac_bssid(data->dragonite_bss_info, vif->addr))>=0)
	{
		mac_program_bssid(vif->addr, bssid_idx, 0, bssid_idx);
		wifi_tx_enable--;
		data->bssid_count--;

		ASSERT( (data->bssid_count >= 0), "DRAGONITE: Wrong bssid count !!\n");

		/* free pending tx, include per bss mgmt packet, bmc packet, bcq backet, beacon q packet */
		dragonite_tx_detach_vif_node(data, vp);

        spin_lock_irqsave(&vp->vplock, irqflags);

		if(data->dragonite_bss_info[bssid_idx].beacon_tx_buffer_ptr) {
			mac_free((void *) data->dragonite_bss_info[bssid_idx].beacon_tx_buffer_ptr);
			data->dragonite_bss_info[bssid_idx].beacon_tx_buffer_ptr = NULL;
		}

        spin_unlock_irqrestore(&vp->vplock, irqflags);

        data->vif_priv_addr[bssid_idx] = 0;

		memset(&data->dragonite_bss_info[bssid_idx], 0, sizeof(struct __bss_info));

		dragonite_mac_unlock();

		dragonite_bss_ps_lock();

		data->mac_info->bcq_status[bssid_idx] = DRG_BCQ_DISABLE;
		data->mac_info->bss_ps_status[bssid_idx] = DRG_BSS_PS_OFF;

		dragonite_bss_ps_unlock();
#if defined(DRAGONITE_ENABLE_AIRKISS)
		if(vif->type == NL80211_IFTYPE_MONITOR)
		{
			airkiss_stop();
		}
#endif
	}
	else
	{
		dragonite_mac_unlock();

		DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Can not find such mac address !!\n");
	}

exit:
	mutex_unlock(&data->mutex);

	return;
}
static void mac80211_dragonite_tx_frame(struct ieee80211_hw *hw,
				    struct sk_buff *skb,
				    struct ieee80211_channel *chan)
{
	u32 _pid = ACCESS_ONCE(wmediumd_portid);

	if (rctbl) {
		struct ieee80211_tx_info *txi = IEEE80211_SKB_CB(skb);
		ieee80211_get_tx_rates(txi->control.vif, NULL, skb,
				       txi->control.rates,
				       ARRAY_SIZE(txi->control.rates));
	}

	mac80211_dragonite_monitor_rx(hw, skb, chan);

	if (_pid)
		return mac80211_dragonite_tx_frame_nl(hw, skb, _pid);

	mac80211_dragonite_tx_frame_no_nl(hw, skb, chan);
	dev_kfree_skb(skb);
}

unsigned char primary_ch_offset(u8 bw_type)
{
	if (bw_type == BW40MHZ_SCB)
		return CH_OFFSET_20U;
	else if(bw_type == BW40MHZ_SCA)
		return CH_OFFSET_20L;
	else
		return CH_OFFSET_20;
}
static const char * const dragonite_chanwidths[] = {
	[NL80211_CHAN_WIDTH_20_NOHT] = "noht",
	[NL80211_CHAN_WIDTH_20] = "ht20",
	[NL80211_CHAN_WIDTH_40] = "ht40",
	[NL80211_CHAN_WIDTH_80] = "vht80",
	[NL80211_CHAN_WIDTH_80P80] = "vht80p80",
	[NL80211_CHAN_WIDTH_160] = "vht160",
};

static u8 dragonite_curr_channel;
static u8 dragonite_curr_bandwidth;

unsigned char dragonite_transfer_channel_num(unsigned short freq)
{
	int i;

 	for(i = 0; i < (sizeof(dragonite_channels_2ghz))/(sizeof(dragonite_channels_2ghz[0])); i++) {
 		if(dragonite_channels_2ghz[i].center_freq == freq)
 			break;
	}
	return (i+1);
}

void dragonite_get_channel(u8 *ch, u8 *bw)
{
	if(ch)
		*ch = dragonite_curr_channel;

	if(bw)
		*bw = dragonite_curr_bandwidth;
}

void dragonite_set_channel(u8 ch, u8 bw)
{
	char pri_channel;
	int freq;

	dragonite_mac_lock();

	/* RF */
	rf_set_40mhz_channel(ch, bw);

	freq = lrf_ch2freq(ch);
	/* BB */
	if (bw == BW40MHZ_SCN)
		bb_set_20mhz_mode(freq);
	else
		bb_set_40mhz_mode(bw, freq);

	/* MAC */
	pri_channel = primary_ch_offset(bw);

	MACREG_UPDATE32(BASIC_SET,
					(pri_channel << 9)|(pri_channel << 6),
					LMAC_CH_BW_CTRL_CH_OFFSET|BASIC_CHANNEL_OFFSET);

	/* backward compatible for some FPGA-HW without second CCA circuit */
	if(bw == BW40MHZ_SCN)
		MACREG_UPDATE32(BASIC_SET,
						LMAC_DECIDE_CH_OFFSET|LMAC_DECIDE_CTRL_FR,
						LMAC_CH_BW_CTRL_AUTO_MASK);
	else
		/* David asked LMAC look both CCA0 and CCA1 to transmit, like other vendors. */
		MACREG_UPDATE32(BASIC_SET,
						LMAC_DECIDE_CH_OFFSET|LMAC_CCA1_EN|LMAC_DECIDE_CTRL_FR,
						LMAC_CH_BW_CTRL_AUTO_MASK);

	dragonite_curr_channel = ch;
	dragonite_curr_bandwidth = bw;

	if(ch >= 1 && ch <=14)
		bb_set_tx_gain(txvga_gain[ch - 1]);

	dragonite_mac_unlock();
}

static void dragonite_update_survey_stats(struct mac80211_dragonite_data *data, u16 freq)
{
	struct survey_info *survey;
	u16 channel_number;

 	channel_number = dragonite_transfer_channel_num(freq);

	if(channel_number > DRAGONITE_MAX_CHANNELS) {
		return;
	}

	survey = (struct survey_info *)&data->survey[channel_number - 1];

	dragonite_mac_lock();

	bb_rx_counter_ctrl(BB_RX_CNT_DISABLE);

	/*Record the stats*/
	survey->time += bb_rx_counter_read(BB_RX_CHANNEL_CNT_ADDR);
	survey->time_busy += bb_rx_counter_read(BB_RX_BUSY_CNT_ADDR);
	survey->noise = bb_read_noise_floor();
	dragonite_mac_unlock();

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "time %llu\n", survey->time);
	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "time busy %llu\n", survey->time_busy);
	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "noise %d dBm\n", survey->noise);

	survey->filled = SURVEY_INFO_TIME | 
					 SURVEY_INFO_TIME_BUSY |
					 SURVEY_INFO_NOISE_DBM;
}

static int mac80211_dragonite_config(struct ieee80211_hw *hw, u32 changed)
{
	struct mac80211_dragonite_data *data = hw->priv;
	struct ieee80211_conf *conf = &hw->conf;
	u16 prev_center_freq, channel_number;

	static const char *smps_modes[IEEE80211_SMPS_NUM_MODES] = {
		[IEEE80211_SMPS_AUTOMATIC] = "auto",
		[IEEE80211_SMPS_OFF] = "off",
		[IEEE80211_SMPS_STATIC] = "static",
		[IEEE80211_SMPS_DYNAMIC] = "dynamic",
	};

	if (conf->chandef.chan)
		wiphy_debug(hw->wiphy,
			    "%s (freq=%d(%d - %d)/%s idle=%d ps=%d smps=%s)\n",
			    __func__,
			    conf->chandef.chan->center_freq,
			    conf->chandef.center_freq1,
			    conf->chandef.center_freq2,
			    dragonite_chanwidths[conf->chandef.width],
			    !!(conf->flags & IEEE80211_CONF_IDLE),
			    !!(conf->flags & IEEE80211_CONF_PS),
			    smps_modes[conf->smps_mode]);
	else
		wiphy_debug(hw->wiphy,
			    "%s (freq=0 idle=%d ps=%d smps=%s)\n",
			    __func__,
			    !!(conf->flags & IEEE80211_CONF_IDLE),
			    !!(conf->flags & IEEE80211_CONF_PS),
			    smps_modes[conf->smps_mode]);

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP_UNTIL;

	dragonite_pm_lock_unlock();
#endif

	data->idle = !!(conf->flags & IEEE80211_CONF_IDLE);

	data->channel = conf->chandef.chan;


	prev_center_freq = data->curr_center_freq;

	data->curr_center_freq = data->channel->center_freq;
	data->curr_band = IEEE80211_BAND_2GHZ;

	WARN_ON(data->channel && channels > 1);

	data->power_level = conf->power_level;

	if ((changed & IEEE80211_CONF_CHANGE_CHANNEL) && (data->started)) 
	{
		memcpy(&data->chandef, &conf->chandef, sizeof(struct cfg80211_chan_def));

		channel_number = dragonite_transfer_channel_num(data->chandef.chan->center_freq);

		if(channel_number <= DRAGONITE_MAX_CHANNELS) {
			/* before update channel, update survey info first */

			dragonite_update_survey_stats(data, prev_center_freq);

			data->channel_number = channel_number;
			if(data->chandef.width==NL80211_CHAN_WIDTH_40) {

				if(data->chandef.center_freq1 > data->chandef.chan->center_freq) {
					DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "set channel %d, bandwidth 40mhz above \n", channel_number);
					data->primary_ch_offset = CH_OFFSET_20L;
					dragonite_set_channel(channel_number, BW40MHZ_SCA);
				}
				else if (data->chandef.center_freq1 < data->chandef.chan->center_freq) {
					DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "set channel %d, bandwidth 40mhz below \n", channel_number);
					data->primary_ch_offset = CH_OFFSET_20U;
					dragonite_set_channel(channel_number, BW40MHZ_SCB);
				}
				else
				{
					DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Unknown frequency\n");
				}

			}
			else
			{
				DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "set channel %d, bandwidth 20mhz \n", channel_number);
				data->primary_ch_offset = CH_OFFSET_20;
				dragonite_set_channel(channel_number, BW40MHZ_SCN);
			}

			dragonite_mac_lock();

			if(fem_en)
				panther_fem_config(fem_en);

			panther_channel_config(channel_number, fem_en);

			panther_notch_filter_config(channel_number, data->primary_ch_offset);

			/* reset bb rx counter */
			bb_rx_counter_ctrl(BB_RX_CNT_RESET);
			/* enable bb rx counter */
			bb_rx_counter_ctrl(BB_RX_CNT_ENABLE);
			/* reset noise floor to 0xff */
			bb_read_noise_floor();

			dragonite_mac_unlock();
		}
		else
		{
			DRG_ERROR(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "None support channel\n");
		}
	}
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
	if((changed & IEEE80211_CONF_CHANGE_PS)&& data->started) {

		if(conf->flags & IEEE80211_CONF_PS) {
			/* PS mode */
			dragonite_pm_lock_lock();

			data->ps_enabled = true;
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
			data->ps_flags &= ~PS_FORCE_WAKE_UP_UNTIL;
#endif

			dragonite_pm_lock_unlock();
		}
		else
		{
			/* Wakeup mode */
			dragonite_pm_lock_lock();

			if(data->poweroff) {

				dragonite_ps_wakeup(data);

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
				dragonite_correct_ts(false);
#endif
			}
			if(dragonite_dynamic_ps) {
				dragonite_ps_clear_pm_bit(data);
			}
			data->ps_enabled = false;
			data->ps_flags &= ~PS_FORCE_WAKE_UP;

			dragonite_pm_lock_unlock();
		}
	}
#endif

	return 0;
}

#if defined(DRAGONITE_ENABLE_AIRKISS)
u32 __airkiss_filter_mode = 0;
static DEFINE_SPINLOCK(airkiss_spinlock);
static unsigned long airkiss_lock_irqflags;
static inline void airkiss_lock(void)
{
	spin_lock_irqsave(&airkiss_spinlock, airkiss_lock_irqflags);
}
static inline void airkiss_unlock(void)
{
	spin_unlock_irqrestore(&airkiss_spinlock, airkiss_lock_irqflags);
}
void airkiss_start(void)
{
	drg_wifi_recover = 1;
	airkiss_lock();

	__airkiss_filter_mode = 1;

	airkiss_unlock();
}
void airkiss_stop(void)
{
	airkiss_lock();

	__airkiss_filter_mode = 0;

	airkiss_unlock();
	drg_wifi_recover = 1;
}
#endif
static void mac80211_dragonite_configure_filter(struct ieee80211_hw *hw,
					    unsigned int changed_flags,
					    unsigned int *total_flags,u64 multicast)
{
	struct mac80211_dragonite_data *data = hw->priv;
	mutex_lock(&data->mutex);                            
                                                     
	if(!data->started)                                   
	{                                                    
		DRG_DBG("DRAGONITE: Not started yet !!\n");        
		goto exit;                                         
	}                                                    

	wiphy_debug(hw->wiphy, "%s\n", __func__);

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	if(changed_flags & FIF_OTHER_BSS) {

		if(data->rx_filter & FIF_OTHER_BSS) {

			DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "disable moniter obss packets\n");

			data->rx_filter &= ~FIF_OTHER_BSS;
		}
		else
		{

			DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "enable moniter obss packets\n");

			data->rx_filter |= FIF_OTHER_BSS;
		}
	}

	if(data->rx_filter & FIF_OTHER_BSS) {

		if(!dragonite_force_disable_sniffer_function)
		{
			MACREG_UPDATE32(MAC_RX_FILTER, RXF_UC_DAT_ALL |RXF_GC_DAT_ALL, RXF_UC_DAT_TA_RA_HIT | RXF_GC_DAT_TA_HS_HIT);

			enable_sniffer_mode();
		}
	}
	else
	{
		/* restore MAC_RX_FILTER to default value */
		MACREG_WRITE32(MAC_RX_FILTER, RXF_GC_MGT_TA_HIT | RXF_GC_DAT_TA_HIT | RXF_UC_MGT_RA_HIT | RXF_UC_DAT_RA_HIT
					   | RXF_BEACON_ALL | RXF_PROBE_REQ_ALL);

		disable_sniffer_mode();
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	if (*total_flags & FIF_PROMISC_IN_BSS)
		data->rx_filter |= FIF_PROMISC_IN_BSS;
#endif
	if (*total_flags & FIF_ALLMULTI)
		data->rx_filter |= FIF_ALLMULTI;

	*total_flags = data->rx_filter;
	exit:                       
	mutex_unlock(&data->mutex);

}

static void mac80211_dragonite_bss_info_changed(struct ieee80211_hw *hw,
					    struct ieee80211_vif *vif,
					    struct ieee80211_bss_conf *info,
					    u32 changed)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	struct mac80211_dragonite_data *data = hw->priv;
	int idx;
	bool _enable_beacon = false;

    mutex_lock(&data->mutex);

	if(!data->started)
    {
		DRG_DBG("DRAGONITE: Not started yet !!\n");
		goto exit;
	}

	dragonite_check_magic(vif);

	wiphy_debug(hw->wiphy, "%s(changed=0x%x)\n", __func__, changed);

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	if (changed & BSS_CHANGED_BSSID) {
		wiphy_debug(hw->wiphy, "%s: BSSID changed: %pM\n",
			    __func__, info->bssid);
		memcpy(vp->bssid, info->bssid, ETH_ALEN);

		if(vp->bssinfo->dut_role == ROLE_STA)
		{
			memcpy(vp->bssinfo->associated_bssid, (u8 *) info->bssid, ETH_ALEN);
		}
	}

	if (changed & BSS_CHANGED_ASSOC) {
		wiphy_debug(hw->wiphy, "  ASSOC: assoc=%d aid=%d\n",
			    info->assoc, info->aid);
		vp->assoc = info->assoc;
		vp->aid = info->aid;
	}

	if (changed & BSS_CHANGED_BEACON_INT) {
		wiphy_debug(hw->wiphy, "  BCNINT: %d\n", info->beacon_int);

		if(data->started) {

			vp->bssinfo->beacon_interval = info->beacon_int;
			vp->bssinfo->dtim_period = info->dtim_period;

			if(vp->bssinfo->dut_role == ROLE_STA) {

				DRG_DBG("STA mode beacon_interval %d, dtim_period %d\n", info->beacon_int, info->dtim_period);
				dragonite_cfg_start_tsync(vp->bssinfo->timer_index, info->beacon_int, info->dtim_period, 0);
			}
			else if(vp->bssinfo->dut_role == ROLE_AP)
			{
				DRG_DBG("AP mode beacon_interval %d, dtim_period %d\n", info->beacon_int, info->dtim_period);

				dragonite_beacon_setup(info->beacon_int, info->dtim_period);
			}
		}
	}

	if (changed & BSS_CHANGED_BEACON_ENABLED) {
		wiphy_debug(hw->wiphy, "  BCN EN: %d\n", info->enable_beacon);

		if(vp->bssinfo->dut_role == ROLE_AP) {
			vp->bssinfo->enable_beacon = info->enable_beacon;
		}

		for(idx = 0; idx < MAC_MAX_BSSIDS; idx++) {

		    vp = idx_to_vp(data, (u8) idx);
		    if(vp == NULL)
		    {
		        continue;
		    }
		    if(vp->bssinfo->enable_beacon)
		    {
		        _enable_beacon = true;
		    }
		}
		vp = (void *)vif->drv_priv; /* reinit vp */

		if(data->started) {

		    if(data->enable_beacon && !_enable_beacon) {
		        data->enable_beacon = false;
		        dragonite_beacon_stop();
		    }
		    else if(!data->enable_beacon && _enable_beacon) {
		        data->enable_beacon = true;
		        dragonite_beacon_start(0x20);/* bitrate CCK 1M*/
		    }
		}
	}

	if (changed & BSS_CHANGED_ERP_CTS_PROT) {
		wiphy_debug(hw->wiphy, "  ERP_CTS_PROT: %d\n",
			    info->use_cts_prot);
	}

	if (changed & BSS_CHANGED_ERP_PREAMBLE) {
		wiphy_debug(hw->wiphy, "  ERP_PREAMBLE: %d\n",
			    info->use_short_preamble);
	}

	if (changed & BSS_CHANGED_ERP_SLOT) {
		wiphy_debug(hw->wiphy, "  ERP_SLOT: %d\n", info->use_short_slot);
	}

	if (changed & BSS_CHANGED_HT) {
		wiphy_debug(hw->wiphy, "  HT: op_mode=0x%x\n",
			    info->ht_operation_mode);
	}

	if (changed & BSS_CHANGED_BASIC_RATES) {
		wiphy_debug(hw->wiphy, "  BASIC_RATES: 0x%llx\n",
			    (unsigned long long) info->basic_rates);
	}

	if (changed & BSS_CHANGED_TXPOWER)
		wiphy_debug(hw->wiphy, "  TX Power: %d dBm\n", info->txpower);

exit:
    mutex_unlock(&data->mutex);
}

static int mac80211_dragonite_sta_add(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  struct ieee80211_sta *sta)
{
	struct mac80211_dragonite_data *data = hw->priv;
	struct dragonite_sta_priv *sp = (void *)sta->drv_priv;
	int ret;

    DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Add station mac addr %pM, sp %p\n", sta->addr, sp);

    mutex_lock(&data->mutex);

	if(!data->started)
    {
		DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Not started yet !!\n");
		ret = -EIO;
		goto exit;
	}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Start to add station\n");

	dragonite_check_magic(vif);
	dragonite_set_sta_magic(sta);

	ret = dragonite_add_station(hw->priv, sta, vif);

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Finish to add station\n");

exit:
    mutex_unlock(&data->mutex);

	return ret;
}

static int mac80211_dragonite_sta_remove(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif,
				     struct ieee80211_sta *sta)
{
	struct mac80211_dragonite_data *data = hw->priv;
	struct dragonite_sta_priv *sp = (void *)sta->drv_priv;
	int ret;

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Remove station mac addr %pM, sp %p\n", sta->addr, sp);

	mutex_lock(&data->mutex);

	if(!data->started)
    {
		DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Not started yet !!\n");
		ret = -EIO;
		goto exit;
	}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Start to remove station\n");

	dragonite_check_magic(vif);
	dragonite_clear_sta_magic(sta);;

	ret = dragonite_remove_station(hw->priv, sta, vif);

	DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Finish to remove station\n");

exit:
	mutex_unlock(&data->mutex);

	return ret;
}

static void mac80211_dragonite_sta_notify(struct ieee80211_hw *hw,
				      struct ieee80211_vif *vif,
				      enum sta_notify_cmd cmd,
				      struct ieee80211_sta *sta)
{
	dragonite_check_magic(vif);
	DRG_DBG("mac80211_dragonite_sta_notify\n");
	switch (cmd) {
	case STA_NOTIFY_SLEEP:
	case STA_NOTIFY_AWAKE:
		/* TODO: make good use of these flags */
		break;

	default:
		WARN(1, "Invalid sta notify: %d\n", cmd);
		break;
	}
}
static int mac80211_dragonite_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
				  struct ieee80211_vif *vif, struct ieee80211_sta *sta,
				  struct ieee80211_key_conf *key)
{
	struct mac80211_dragonite_data *data = hw->priv;
	int ret = 0;

	mutex_lock(&data->mutex);

    if(!data->started)
    {
		DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "Not started yet !!\n");
		ret = -EIO;
		goto exit;
	}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	switch (cmd) {
	case SET_KEY:
		ret = dragonite_key_config(data, vif, sta, key);
		break;
	case DISABLE_KEY:
		ret = dragonite_key_delete(data, vif, key);
		break;
	default:
		ret = -EINVAL;
		break;
	}

exit:
	mutex_unlock(&data->mutex);

	return ret;
}
static void mac80211_dragonite_set_default_unicast_key(struct ieee80211_hw *hw, 
						  struct ieee80211_vif *vif, int idx)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	struct mac80211_dragonite_data *data = hw->priv;
	cipher_key *gkey;

	DRG_NOTICE(DRAGONITE_DEBUG_ENCRPT_FLAG, "set default unicast key, key idx = %d\n", idx);

	if(idx >= 0 && idx < 4) {

		gkey = &data->mac_info->group_keys[vp->bssid_idx];

		gkey->wep_def_key.txkeyid = idx;
	}
}
static int mac80211_dragonite_set_tim(struct ieee80211_hw *hw,
				  struct ieee80211_sta *sta,
				  bool set)
{
	dragonite_check_sta_magic(sta);
	return 0;
}

static int mac80211_dragonite_conf_tx(
	struct ieee80211_hw *hw,
	struct ieee80211_vif *vif, u16 queue,
	const struct ieee80211_tx_queue_params *params)
{
	wiphy_debug(hw->wiphy,
		    "%s (queue=%d txop=%d cw_min=%d cw_max=%d aifs=%d)\n",
		    __func__, queue,
		    params->txop, params->cw_min,
		    params->cw_max, params->aifs);
	return 0;
}

static int mac80211_dragonite_get_survey(
	struct ieee80211_hw *hw, int idx,
	struct survey_info *survey)
{
	struct ieee80211_conf *conf = &hw->conf;
	struct mac80211_dragonite_data *data = hw->priv;

	wiphy_debug(hw->wiphy, "%s (idx=%d)\n", __func__, idx);

	if(!data->started){
		return -ENOENT;
	}

	if ((idx < 0) || (idx >= DRAGONITE_MAX_CHANNELS))
		return -ENOENT;

	data->survey[idx].channel = &data->channels_2ghz[idx];

	memcpy(survey, &data->survey[idx], sizeof(struct survey_info));

	if(conf->chandef.chan->center_freq == survey->channel->center_freq)
	{
		survey->filled = SURVEY_INFO_IN_USE;
	}

	return 0;
}

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
void dragonite_ps_set_pm_bit(struct ieee80211_vif *vif)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;

	vp->tx_pm_bit = true;
	vp->tx_pm_bit_prev_jiffies = jiffies;
}
void dragonite_ps_clear_pm_bit(struct mac80211_dragonite_data *data)
{
	int idx;
	struct dragonite_vif_priv *vp;

	for(idx = 0; idx < MAC_MAX_BSSIDS; idx++)
	{
		vp = idx_to_vp(data, (u8) idx);
		if(vp == NULL)
		{
			continue;
		}
		if(vp->bssinfo->dut_role == ROLE_STA)
		{
			vp->tx_pm_bit = false;
		}
	}
}
void dragonite_ps_wakeup(struct mac80211_dragonite_data *data)
{
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	int primary_ch_freq, channel_type;
#endif
	if(data->poweroff)
	{
		rf_power_on();
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
		hrtimer_try_to_cancel(&data->tsf_timer);

		hrtimer_try_to_cancel(&data->ps_sleep_full_timer);

		primary_ch_freq = data->channel_number;

		if(data->primary_ch_offset == CH_OFFSET_20L) {
			channel_type = BW40MHZ_SCA;
		}
		else if(data->primary_ch_offset == CH_OFFSET_20U) {
			channel_type = BW40MHZ_SCB;
		}
		else
		{
			channel_type = BW40MHZ_SCN;
		}
		/* after power down rf, rf will missing frequency */
		rf_set_40mhz_channel(primary_ch_freq, channel_type);
		enable_mac_interrupts();
#endif
		data->poweroff = false;
	}
}

void dragonite_ps_sleep(struct mac80211_dragonite_data *data)
{
	if(!data->poweroff)
	{
		if(dragonite_has_tx_pending(data)) {
			panic("dragonite_has_tx_pending in dragonite_ps_sleep");
		}
		rf_power_off();
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
		/* wait 1ms to prevent MAC access memory not done yet */
		hrtimer_start(&data->ps_sleep_full_timer, ktime_set(0, MS_DELAY_BEFORE_GATE_WIFIMAC_CLOCK * 1000), HRTIMER_MODE_REL);
#endif
		data->poweroff = true;
	}
}
#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
void dragonite_calculate_pretbtt(int beacon_interval)
{
	pre_tbtt_remain = GLOBAL_PRE_TBTT_DEFAULT_DIV(beacon_interval) * TIME_UNIT;

	ts_next.val_64 = MACREG_READ64(TS_NEXT_TBTT(3));
	ts_now.val_64 = MACREG_READ64(TS_O(3));
	pre_ktime = ktime_get();

	pre_tbtt.val_64 = ts_next.val_64 - pre_tbtt_remain;
	pre_ts = ts_now.val_64;

	DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "now %08x %08x\n", ts_now.val_32[0], ts_now.val_32[1]);
	DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "pre %08x %08x\n", pre_tbtt.val_32[0], pre_tbtt.val_32[1]);
	DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "next %08x %08x\n", ts_next.val_32[0], ts_next.val_32[1]);
}

void dragonite_correct_ts(bool pretbtt)
{
	if(pretbtt) {
		ts_o.val_64 = pre_tbtt.val_64;
	}
	else
	{
		ts_o.val_64 = pre_ts + ktime_to_us(ktime_sub(ktime_get(), pre_ktime));
	}

	DRG_NOTICE(DRAGONITE_DEBUG_PS_FLAG, "change TS_O(3) %08x %08x\n", ts_o.val_32[0], ts_o.val_32[1]);

	MACREG_WRITE64(TS_O(3), ts_o.val_64);
}
#endif
#endif

#ifdef CPTCFG_NL80211_TESTMODE
/*
 * This section contains example code for using netlink
 * attributes with the testmode command in nl80211.
 */

/* These enums need to be kept in sync with userspace */
enum dragonite_testmode_attr {
	__DRAGONITE_TM_ATTR_INVALID	= 0,
	DRAGONITE_TM_ATTR_CMD	= 1,
	DRAGONITE_TM_ATTR_PS	= 2,

	/* keep last */
	__DRAGONITE_TM_ATTR_AFTER_LAST,
	DRAGONITE_TM_ATTR_MAX	= __DRAGONITE_TM_ATTR_AFTER_LAST - 1
};

enum dragonite_testmode_cmd {
	DRAGONITE_TM_CMD_SET_PS		= 0,
	DRAGONITE_TM_CMD_GET_PS		= 1,
	DRAGONITE_TM_CMD_STOP_QUEUES	= 2,
	DRAGONITE_TM_CMD_WAKE_QUEUES	= 3,
};

static const struct nla_policy dragonite_testmode_policy[DRAGONITE_TM_ATTR_MAX + 1] = {
	[DRAGONITE_TM_ATTR_CMD] = { .type = NLA_U32 },
	[DRAGONITE_TM_ATTR_PS] = { .type = NLA_U32 },
};

static int dragonite_fops_ps_write(void *dat, u64 val);

static int mac80211_dragonite_testmode_cmd(struct ieee80211_hw *hw,
				       void *data, int len)
{
	struct mac80211_dragonite_data *dragonite = hw->priv;
	struct nlattr *tb[DRAGONITE_TM_ATTR_MAX + 1];
	struct sk_buff *skb;
	int err, ps;

	err = nla_parse(tb, DRAGONITE_TM_ATTR_MAX, data, len,
			dragonite_testmode_policy);
	if (err)
		return err;

	if (!tb[DRAGONITE_TM_ATTR_CMD])
		return -EINVAL;

	switch (nla_get_u32(tb[DRAGONITE_TM_ATTR_CMD])) {
	case DRAGONITE_TM_CMD_SET_PS:
		if (!tb[DRAGONITE_TM_ATTR_PS])
			return -EINVAL;
		ps = nla_get_u32(tb[DRAGONITE_TM_ATTR_PS]);
		return dragonite_fops_ps_write(dragonite, ps);
	case DRAGONITE_TM_CMD_GET_PS:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy,
						nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		if (nla_put_u32(skb, DRAGONITE_TM_ATTR_PS, dragonite->ps))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);
	case DRAGONITE_TM_CMD_STOP_QUEUES:
		ieee80211_stop_queues(hw);
		return 0;
	case DRAGONITE_TM_CMD_WAKE_QUEUES:
		ieee80211_wake_queues(hw);
		return 0;
	default:
		return -EOPNOTSUPP;
	}

 nla_put_failure:
	kfree_skb(skb);
	return -ENOBUFS;
}
#endif
int ampdu_factor_to_len(u8 ampdu_factor)
{
	//ampdu_factor/max_ampdu_len encoding 00: 8191, 01: 16383, 10: 32767, 11:65535
	int max_ampdu_len;
	switch(ampdu_factor) {
	case 0:
		max_ampdu_len = 8192;
		break;
	case 1:
		max_ampdu_len = 16384;
		break;
	case 2:
		max_ampdu_len = 32768;
		break;
	case 3:
		max_ampdu_len = 65536;
		break;
	default:
        panic("Unknown A-MPDU density");
        break;
	}
	return max_ampdu_len;
}

#if defined (DRAGONITE_ACQ_DELAY_KICK)
static enum hrtimer_restart tx_acq_delay_kick(struct hrtimer *timer)
{
	struct mac80211_dragonite_data *data = global_data;
	acq *q = container_of(timer, acq, tx_acq_timer);

	tx_acq_kick(data->mac_info, q, q->wptr);

	return HRTIMER_NORESTART;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
static int mac80211_dragonite_ampdu_action(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif,
				       struct ieee80211_ampdu_params *params)
#else
static int mac80211_dragonite_ampdu_action(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif,
				       enum ieee80211_ampdu_mlme_action action,
				       struct ieee80211_sta *sta, u16 tid, u16 *ssn,
				       u8 buf_size)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	enum ieee80211_ampdu_mlme_action action = params->action;
	struct ieee80211_sta *sta = params->sta;
	u16 tid = params->tid;
	u16 *ssn = &params->ssn;
	u8 buf_size = params->buf_size;
#endif
	struct mac80211_dragonite_data *data = hw->priv;
	MAC_INFO* info = data->mac_info;
	struct dragonite_sta_priv *sp = (struct dragonite_sta_priv *) sta->drv_priv;
	int addr_index;
	int max_ampdu_len;
	acq *q;
	int i, sta_index, ret = 0;
    unsigned long irqflags;
	sta_basic_info bcap;
	bool tear_down;
	u16 drg_ssn;
	
	mutex_lock(&data->mutex);

	if(!data->started)
    {
		DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "DRAGONITE: Not started yet !!\n");
		ret = -EIO;
		goto exit;
	}

#if defined(DRAGONITE_POWERSAVE_AGGRESSIVE)
	dragonite_pm_lock_lock();

	dragonite_ps_wakeup(data);
	data->ps_flags |= PS_FORCE_WAKE_UP;

	dragonite_pm_lock_unlock();
#endif

	switch (action) {
	case IEEE80211_AMPDU_TX_START:
		if(dragonite_tx_ampdu_enable==0) {

			ret = -EOPNOTSUPP;

			break;
		}
		spin_lock_irqsave(&sp->splock, irqflags);

		if(SINGLE_Q(sp, tid) == NULL)
		{
			drg_ssn = NORMAL_SSN(sp);

			drg_ssn = (drg_ssn + DRAGONITE_AMPDU_JUMP_OVER_SSN) & 0x0fff;

			*ssn = drg_ssn;

			AGGR_SSN(sp, tid) = *ssn;

			spin_unlock_irqrestore(&sp->splock, irqflags);

			ieee80211_start_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		}
		else
		{
			spin_unlock_irqrestore(&sp->splock, irqflags);

			DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "DRAGONITE: tx ampdu is already exist\n");

			ret = -EOPNOTSUPP;
		}
		break;
	case IEEE80211_AMPDU_TX_STOP_CONT:
	case IEEE80211_AMPDU_TX_STOP_FLUSH:
	case IEEE80211_AMPDU_TX_STOP_FLUSH_CONT:
		tear_down = true;

        spin_lock_irqsave(&sp->splock, irqflags);

		if((q = SINGLE_Q(sp, tid)) != NULL) {

			SINGLE_Q(sp, tid) = NULL;

			for(i = 0; i < TID_MAX; i++) {

				if(SINGLE_Q(sp, i) != NULL)
				{
					tear_down = false;

					break;
				}
			}
			if(tear_down) {
				IS_AMPDU(sp) = false;
			}
			spin_unlock_irqrestore(&sp->splock, irqflags);

			DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Remove single q tid %d, sp (%p)\n", tid, sp);

#if defined (DRAGONITE_ACQ_DELAY_KICK)
			hrtimer_cancel(&q->tx_acq_timer);
#endif
			tx_acq_detach(data, q);

			sta_index = (q->qinfo & ACQ_AIDX) >> ACQ_AIDX_SHIFT;
		
			DRG_DBG("before tx_sw_queue[%d][%d] tx_sw_queue_count %d\n", 
				   sta_index, tid, sp->tx_sw_queue[tid].qcnt);
			DRG_DBG("before tx_sw_single_queue[%d][%d] tx_sw_single_queue_count %d\n", 
				   sta_index, tid, sp->tx_sw_single_queue[tid].qcnt);
			DRG_DBG("before tx_sw_ps_single_queue[%d][%d] tx_sw_ps_single_queue_count %d\n", 
				   sta_index, tid, sp->tx_sw_ps_single_queue[tid].qcnt);
			DRG_DBG("beforetx_hw_single_queue[%d][%d] tx_hw_single_queue_count %d\n", 
				   sta_index, tid, sp->tx_hw_single_queue[tid].qcnt);

			dragonite_tx_ampdu_q_detach(data, sta_index, tid);

			DRG_DBG("after tx_sw_queue[%d][%d] tx_sw_queue_count %d\n", 
				   sta_index, tid, sp->tx_sw_queue[tid].qcnt);
			DRG_DBG("after tx_sw_single_queue[%d][%d] tx_sw_single_queue_count %d\n", 
				   sta_index, tid, sp->tx_sw_single_queue[tid].qcnt);
			DRG_DBG("after tx_sw_ps_single_queue[%d][%d] tx_sw_ps_single_queue_count %d\n", 
				   sta_index, tid, sp->tx_sw_ps_single_queue[tid].qcnt);
			DRG_DBG("after tx_hw_single_queue[%d][%d] tx_hw_single_queue_count %d\n", 
				   sta_index, tid, sp->tx_hw_single_queue[tid].qcnt);

			if(sp->tx_hw_single_queue[tid].qhead || sp->tx_sw_single_queue[tid].qhead 
			   || sp->tx_sw_ps_single_queue[tid].qhead)
            {
				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "detaching tx_sw_queue[%d][%d] tx_sw_queue_count %d\n", 
					   sta_index, tid, sp->tx_sw_queue[tid].qcnt);
				dragonite_tx_swq_dump(data, (struct list_head *) sp->tx_sw_queue[tid].qhead);
				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "detaching tx_sw_single_queue[%d][%d] tx_sw_single_queue_count %d\n", 
					   sta_index, tid, sp->tx_sw_single_queue[tid].qcnt);
				dragonite_tx_swq_dump(data, (struct list_head *) sp->tx_sw_single_queue[tid].qhead);
				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, "detaching tx_hw_single_queue[%d][%d] tx_hw_single_queue_count %d\n", 
					   sta_index, tid, sp->tx_hw_single_queue[tid].qcnt);
				dragonite_tx_swq_dump(data, (struct list_head *) sp->tx_hw_single_queue[tid].qhead);
				acq_dump(q);
				/* XXX force delete queue not so good, could be wrong*/
				DRG_WARN(DRAGONITE_DEBUG_DRIVER_WARN_FLAG, 
					"Warning !!! tx schedule force delete sta_index tid [%d][%d] sw queue and hw single queue\n", sta_index, tid);
				sp->tx_sw_queue[tid].qhead = NULL;
				sp->tx_sw_queue[tid].qcnt = 0;
				sp->tx_hw_single_queue[tid].qhead = NULL;
				sp->tx_hw_single_queue[tid].qcnt = 0;
				panic("XXX force delete queue not so good, fix it !!");
				
			}
			memset((void *)&q->acqe[0], 0, ACQE_SIZE * q->queue_size);
			memset((void *)&q->acqe[q->queue_size], 0, ACQE_SIZE * q->queue_size);

            tx_acq_free(info, q);
		}
		else
		{
			spin_unlock_irqrestore(&sp->splock, irqflags);
		}
		ieee80211_stop_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		break;
	case IEEE80211_AMPDU_TX_OPERATIONAL:
		if(dragonite_tx_ampdu_enable==0) {

			ret = -EOPNOTSUPP;

			break;
		}
        spin_lock_irqsave(&sp->splock, irqflags);

		if(SINGLE_Q(sp, tid) == NULL)
		{
			if(buf_size > drg_tx_winsz) {
				buf_size = (drg_tx_winsz*8) / 8;
				hw->max_tx_aggregation_subframes = buf_size;
			}

			DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "before ampdu operation tid %d, spacing = %d, window_size = %d, ssn = %d\n",
				   tid, sta->ht_cap.ampdu_density, buf_size, AGGR_SSN(sp, tid));

			drg_ssn = AGGR_SSN(sp, tid);

			q = tx_acq_alloc(info);
			max_ampdu_len = ampdu_factor_to_len(sta->ht_cap.ampdu_factor);
			tx_single_acq_setup(q, tid_to_qid(tid), sta->ht_cap.ampdu_density, 
								max_ampdu_len, buf_size, 64, sp->stacap_index, tid, AGGR_SSN(sp, tid));
			tx_acq_attach(info, q);
			tx_single_acq_start(q, AGGR_SSN(sp, tid));
#if defined (DRAGONITE_ACQ_DELAY_KICK)
			q->tx_count = 0;
			hrtimer_init(&q->tx_acq_timer, CLOCK_MONOTONIC,
								HRTIMER_MODE_REL);
			q->tx_acq_timer.function = tx_acq_delay_kick;

#endif
			SINGLE_Q(sp, tid) = q;
			IS_AMPDU(sp) = true;

			spin_unlock_irqrestore(&sp->splock, irqflags);

			DRG_NOTICE(DRAGONITE_DEBUG_NETWORK_INFO_FLAG, "Creat single acq tid %d, spacing = %d, max_length = %d, window_size = %d, ssn = %d\n", 
				   tid, sta->ht_cap.ampdu_density, max_ampdu_len, buf_size, AGGR_SSN(sp, tid));

			dragonite_ps_lock();

			dragonite_tx_send_ba_request(data, sp->stacap_index, tid, drg_ssn);

			dragonite_ps_unlock();
		}
		else
		{
			spin_unlock_irqrestore(&sp->splock, irqflags);
		}
		break;
	case IEEE80211_AMPDU_RX_START:
		if(dragonite_rx_ampdu_enable==0) {

			ret = -EOPNOTSUPP;

			break;
		}
		bcap.val = 0;
		DRG_DBG("before update engine sta->addr = %pM, sta_binfo %08x, addr_index %08x\n",
			    sta->addr, sp->sta_binfo.val, sp->addr_index);

		if(hw->max_rx_aggregation_subframes > drg_rx_winsz) {
			hw->max_rx_aggregation_subframes = (drg_rx_winsz/8) * 8;
		}

		mac_invalidate_ampdu_scoreboard_cache(sp->stacap_index, tid);
		while(MACREG_READ32(AMPDU_BMG_CTRL) & BMG_CLEAR)
			DRG_DBG("waiting on scoreboard flush, STA index %d, TID %d\n", sp->stacap_index, tid);

		bcap.val = (sp->sta_binfo.val) >> 8;
		bcap.field.rx_ampdu_bitmap |= (0x1 << tid);
        if(sp->sta_mode)
        {
            addr_index = wmac_addr_lookup_engine_update(info, 0, sp->addr_index, (bcap.val << 8) | sp->stacap_index, BY_ADDR_IDX | IN_DS_TBL);
        }
        else
        {
            addr_index = wmac_addr_lookup_engine_update(info, 0, sp->addr_index, (bcap.val << 8) | sp->stacap_index, BY_ADDR_IDX);
        }
		sp->sta_binfo.val = (bcap.val << 8) | sp->stacap_index;
		DRG_DBG("after update engine sta_binfo %08x, addr_index %08x\n", (bcap.val << 8) | sp->stacap_index, addr_index);
		break;
	case IEEE80211_AMPDU_RX_STOP:
		bcap.val = 0;
		DRG_DBG("before update engine sta->addr = %pM, sta_binfo %08x, addr_index %08x\n",
			    sta->addr, sp->sta_binfo.val, sp->addr_index);
		bcap.val = (sp->sta_binfo.val) >> 8;
		bcap.field.rx_ampdu_bitmap &= ~(0x1 << tid);
		if(sp->sta_mode)
		{
			addr_index = wmac_addr_lookup_engine_update(info, 0, sp->addr_index, (bcap.val << 8) | sp->stacap_index, BY_ADDR_IDX | IN_DS_TBL);
		}
		else
        {
            addr_index = wmac_addr_lookup_engine_update(info, 0, sp->addr_index, (bcap.val << 8) | sp->stacap_index, BY_ADDR_IDX);
        }
		sp->sta_binfo.val = (bcap.val << 8) | sp->stacap_index;

		mac_invalidate_ampdu_scoreboard_cache(sp->stacap_index, tid);
		while(MACREG_READ32(AMPDU_BMG_CTRL) & BMG_CLEAR)
			DRG_DBG("waiting on scoreboard flush, STA index %d, TID %d\n", sp->stacap_index, tid);

		DRG_DBG("after update engine sta_binfo %08x, addr_index %08x\n", (bcap.val << 8) | sp->stacap_index, addr_index);
		break;
	default:
		break;
	}
exit:
	mutex_unlock(&data->mutex);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
static void mac80211_dragonite_flush(struct ieee80211_hw *hw,
				 struct ieee80211_vif *vif,
				 u32 queues, bool drop)
#else
static void mac80211_dragonite_flush(struct ieee80211_hw *hw, u32 queues, bool drop)
#endif
{
	/* Not implemented, queues only on kernel side */
}

static bool mac80211_dragonite_tx_frames_pending(struct ieee80211_hw *hw)
{
	struct mac80211_dragonite_data *data = hw->priv;

	return dragonite_has_tx_pending(data);
}

static void hw_scan_work(struct work_struct *work)
{
	struct mac80211_dragonite_data *dragonite =
		container_of(work, struct mac80211_dragonite_data, hw_scan.work);
	struct cfg80211_scan_request *req = dragonite->hw_scan_request;
	int dwell, i;

	mutex_lock(&dragonite->mutex);
	if (dragonite->scan_chan_idx >= req->n_channels) {
		wiphy_debug(dragonite->hw->wiphy, "hw scan complete\n");
		ieee80211_scan_completed(dragonite->hw, false);
		dragonite->hw_scan_request = NULL;
		dragonite->hw_scan_vif = NULL;
		dragonite->tmp_chan = NULL;
		mutex_unlock(&dragonite->mutex);
		return;
	}

	wiphy_debug(dragonite->hw->wiphy, "hw scan %d MHz\n",
		    req->channels[dragonite->scan_chan_idx]->center_freq);

	dragonite->tmp_chan = req->channels[dragonite->scan_chan_idx];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	if (dragonite->tmp_chan->flags & IEEE80211_CHAN_NO_IR ||
	    !req->n_ssids) {
#else
	if (dragonite->tmp_chan->flags & IEEE80211_CHAN_PASSIVE_SCAN ||
	    !req->n_ssids) {
#endif
		dwell = 120;
	} else {
		dwell = 30;
		/* send probes */
		for (i = 0; i < req->n_ssids; i++) {
			struct sk_buff *probe;

			probe = ieee80211_probereq_get(dragonite->hw,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
						       dragonite->scan_addr,                                       
#else
						       dragonite->hw_scan_vif,
#endif
						       req->ssids[i].ssid,
						       req->ssids[i].ssid_len,
						       req->ie_len);
			if (!probe)
				continue;

			if (req->ie_len)
				memcpy(skb_put(probe, req->ie_len), req->ie,
				       req->ie_len);

			local_bh_disable();
			mac80211_dragonite_tx_frame(dragonite->hw, probe,
						dragonite->tmp_chan);
			local_bh_enable();
		}
	}
	ieee80211_queue_delayed_work(dragonite->hw, &dragonite->hw_scan,
				     msecs_to_jiffies(dwell));
	dragonite->scan_chan_idx++;
	mutex_unlock(&dragonite->mutex);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
static int mac80211_dragonite_hw_scan(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  struct ieee80211_scan_request *hw_req)
#else
static int mac80211_dragonite_hw_scan(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  struct cfg80211_scan_request *req)
#endif
{
	struct mac80211_dragonite_data *dragonite = hw->priv;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct cfg80211_scan_request *req = &hw_req->req;
#endif

	mutex_lock(&dragonite->mutex);
	if (WARN_ON(dragonite->tmp_chan || dragonite->hw_scan_request)) {
		mutex_unlock(&dragonite->mutex);
		return -EBUSY;
	}
	dragonite->hw_scan_request = req;
	dragonite->hw_scan_vif = vif;
	dragonite->scan_chan_idx = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	if (req->flags & NL80211_SCAN_FLAG_RANDOM_ADDR)
		get_random_mask_addr(dragonite->scan_addr,
				     hw_req->req.mac_addr,
				     hw_req->req.mac_addr_mask);
	else
		memcpy(dragonite->scan_addr, vif->addr, ETH_ALEN);
#endif
	mutex_unlock(&dragonite->mutex);

	wiphy_debug(hw->wiphy, "dragonite hw_scan request\n");

	ieee80211_queue_delayed_work(dragonite->hw, &dragonite->hw_scan, 0);

	return 0;
}

static void mac80211_dragonite_cancel_hw_scan(struct ieee80211_hw *hw,
					  struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_data *dragonite = hw->priv;

	wiphy_debug(hw->wiphy, "dragonite cancel_hw_scan\n");

	cancel_delayed_work_sync(&dragonite->hw_scan);

	mutex_lock(&dragonite->mutex);
	ieee80211_scan_completed(dragonite->hw, true);
	dragonite->tmp_chan = NULL;
	dragonite->hw_scan_request = NULL;
	dragonite->hw_scan_vif = NULL;
	mutex_unlock(&dragonite->mutex);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
static void mac80211_dragonite_sw_scan(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif,
				   const u8 *mac_addr)
#else
static void mac80211_dragonite_sw_scan(struct ieee80211_hw *hw)
#endif
{
	struct mac80211_dragonite_data *dragonite = hw->priv;

	mutex_lock(&dragonite->mutex);

	if (dragonite->scanning) {
		printk(KERN_DEBUG "two dragonite sw_scans detected!\n");
		goto out;
	}

	printk(KERN_DEBUG "dragonite sw_scan request, prepping stuff\n");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	memcpy(dragonite->scan_addr, mac_addr, ETH_ALEN);
#endif
	dragonite->scanning = true;

out:
	mutex_unlock(&dragonite->mutex);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
static void mac80211_dragonite_sw_scan_complete(struct ieee80211_hw *hw,
					    struct ieee80211_vif *vif)
#else
static void mac80211_dragonite_sw_scan_complete(struct ieee80211_hw *hw)
#endif
{
	struct mac80211_dragonite_data *dragonite = hw->priv;

	mutex_lock(&dragonite->mutex);

	DRG_DBG("dragonite sw_scan_complete\n");
	dragonite->scanning = false;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	eth_zero_addr(dragonite->scan_addr);
#endif

	mutex_unlock(&dragonite->mutex);
}

static void hw_roc_done(struct work_struct *work)
{
	struct mac80211_dragonite_data *dragonite =
		container_of(work, struct mac80211_dragonite_data, roc_done.work);

	mutex_lock(&dragonite->mutex);
	ieee80211_remain_on_channel_expired(dragonite->hw);
	dragonite->tmp_chan = NULL;
	mutex_unlock(&dragonite->mutex);

	wiphy_debug(dragonite->hw->wiphy, "dragonite ROC expired\n");
}

static int mac80211_dragonite_roc(struct ieee80211_hw *hw,
			      struct ieee80211_vif *vif,
			      struct ieee80211_channel *chan,
			      int duration,
			      enum ieee80211_roc_type type)
{
	struct mac80211_dragonite_data *dragonite = hw->priv;

	mutex_lock(&dragonite->mutex);
	if (WARN_ON(dragonite->tmp_chan || dragonite->hw_scan_request)) {
		mutex_unlock(&dragonite->mutex);
		return -EBUSY;
	}

	dragonite->tmp_chan = chan;
	mutex_unlock(&dragonite->mutex);

	wiphy_debug(hw->wiphy, "dragonite ROC (%d MHz, %d ms)\n",
		    chan->center_freq, duration);

	ieee80211_ready_on_channel(hw);

	ieee80211_queue_delayed_work(hw, &dragonite->roc_done,
				     msecs_to_jiffies(duration));
	return 0;
}

static int mac80211_dragonite_croc(struct ieee80211_hw *hw)
{
	struct mac80211_dragonite_data *dragonite = hw->priv;

	cancel_delayed_work_sync(&dragonite->roc_done);

	mutex_lock(&dragonite->mutex);
	dragonite->tmp_chan = NULL;
	mutex_unlock(&dragonite->mutex);

	wiphy_debug(hw->wiphy, "dragonite ROC canceled\n");

	return 0;
}

static int mac80211_dragonite_add_chanctx(struct ieee80211_hw *hw,
				      struct ieee80211_chanctx_conf *ctx)
{
	dragonite_set_chanctx_magic(ctx);
	wiphy_debug(hw->wiphy,
		    "add channel context control: %d MHz/width: %d/cfreqs:%d/%d MHz\n",
		    ctx->def.chan->center_freq, ctx->def.width,
		    ctx->def.center_freq1, ctx->def.center_freq2);
	return 0;
}

static void mac80211_dragonite_remove_chanctx(struct ieee80211_hw *hw,
					  struct ieee80211_chanctx_conf *ctx)
{
	wiphy_debug(hw->wiphy,
		    "remove channel context control: %d MHz/width: %d/cfreqs:%d/%d MHz\n",
		    ctx->def.chan->center_freq, ctx->def.width,
		    ctx->def.center_freq1, ctx->def.center_freq2);
	dragonite_check_chanctx_magic(ctx);
	dragonite_clear_chanctx_magic(ctx);
}

static void mac80211_dragonite_change_chanctx(struct ieee80211_hw *hw,
					  struct ieee80211_chanctx_conf *ctx,
					  u32 changed)
{
	dragonite_check_chanctx_magic(ctx);
	wiphy_debug(hw->wiphy,
		    "change channel context control: %d MHz/width: %d/cfreqs:%d/%d MHz\n",
		    ctx->def.chan->center_freq, ctx->def.width,
		    ctx->def.center_freq1, ctx->def.center_freq2);
}

static int mac80211_dragonite_assign_vif_chanctx(struct ieee80211_hw *hw,
					     struct ieee80211_vif *vif,
					     struct ieee80211_chanctx_conf *ctx)
{
	dragonite_check_magic(vif);
	dragonite_check_chanctx_magic(ctx);

	return 0;
}

static void mac80211_dragonite_unassign_vif_chanctx(struct ieee80211_hw *hw,
						struct ieee80211_vif *vif,
						struct ieee80211_chanctx_conf *ctx)
{
	dragonite_check_magic(vif);
	dragonite_check_chanctx_magic(ctx);
}

static struct ieee80211_ops mac80211_dragonite_ops =
{
	.tx = mac80211_dragonite_tx,
	.start = mac80211_dragonite_start,
	.stop = mac80211_dragonite_stop,
	.add_interface = mac80211_dragonite_add_interface,
	.change_interface = mac80211_dragonite_change_interface,
	.remove_interface = mac80211_dragonite_remove_interface,
	.config = mac80211_dragonite_config,
	.configure_filter = mac80211_dragonite_configure_filter,
	.bss_info_changed = mac80211_dragonite_bss_info_changed,
	.sta_add = mac80211_dragonite_sta_add,
	.sta_remove = mac80211_dragonite_sta_remove,
	.sta_notify = mac80211_dragonite_sta_notify,
	.set_tim = mac80211_dragonite_set_tim,
	.conf_tx = mac80211_dragonite_conf_tx,
	.get_survey = mac80211_dragonite_get_survey,
	CFG80211_TESTMODE_CMD(mac80211_dragonite_testmode_cmd)
	.ampdu_action = mac80211_dragonite_ampdu_action,
	.sw_scan_start = mac80211_dragonite_sw_scan,
	.sw_scan_complete = mac80211_dragonite_sw_scan_complete,
	.flush = mac80211_dragonite_flush,
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
	.tx_frames_pending = mac80211_dragonite_tx_frames_pending,
#endif
	.get_tsf = mac80211_dragonite_get_tsf,
	.set_tsf = mac80211_dragonite_set_tsf,
	.set_key = mac80211_dragonite_set_key,
    .set_default_unicast_key = mac80211_dragonite_set_default_unicast_key,
};


static void mac80211_dragonite_free(void)
{
	struct list_head tmplist, *i, *tmp;
	struct mac80211_dragonite_data *data, *tmpdata;

	INIT_LIST_HEAD(&tmplist);

	spin_lock_bh(&dragonite_radio_lock);
	list_for_each_safe(i, tmp, &dragonite_radios)
		list_move(i, &tmplist);
	spin_unlock_bh(&dragonite_radio_lock);

	list_for_each_entry_safe(data, tmpdata, &tmplist, list) {
		debugfs_remove(data->debugfs_group);
		debugfs_remove(data->debugfs_ps);
		debugfs_remove(data->debugfs);
		ieee80211_unregister_hw(data->hw);
		device_release_driver(data->dev);
		device_unregister(data->dev);
		ieee80211_free_hw(data->hw);
	}
	class_destroy(dragonite_class);
}

static struct platform_driver mac80211_dragonite_driver = {
	.driver = {
		.name = "mac80211_dragonite",
		.owner = THIS_MODULE,
	},
};

static const struct net_device_ops dragonite_netdev_ops = {
	.ndo_start_xmit 	= dragonite_mon_xmit,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static void dragonite_mon_setup(struct net_device *dev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	dev->netdev_ops = &dragonite_netdev_ops;
	dev->destructor = free_netdev;
	ether_setup(dev);
	dev->priv_flags |= IFF_NO_QUEUE;
	dev->type = ARPHRD_IEEE80211_RADIOTAP;
	eth_zero_addr(dev->dev_addr);
	dev->dev_addr[0] = 0x12;
#else
	netdev_attach_ops(dev, &dragonite_netdev_ops);
	dev->destructor = free_netdev;
	ether_setup(dev);
	dev->tx_queue_len = 0;
	dev->type = ARPHRD_IEEE80211_RADIOTAP;
	memset(dev->dev_addr, 0, ETH_ALEN);
	dev->dev_addr[0] = 0x12;
#endif
}


static void dragonite_send_ps_poll(void *dat, u8 *mac, struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_data *data = dat;
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	struct sk_buff *skb;
	struct ieee80211_pspoll *pspoll;

	if (!vp->assoc)
		return;

	wiphy_debug(data->hw->wiphy,
		    "%s: send PS-Poll to %pM for aid %d\n",
		    __func__, vp->bssid, vp->aid);

	skb = dev_alloc_skb(sizeof(*pspoll));
	if (!skb)
		return;
	pspoll = (void *) skb_put(skb, sizeof(*pspoll));
	pspoll->frame_control = cpu_to_le16(IEEE80211_FTYPE_CTL |
					    IEEE80211_STYPE_PSPOLL |
					    IEEE80211_FCTL_PM);
	pspoll->aid = cpu_to_le16(0xc000 | vp->aid);
	memcpy(pspoll->bssid, vp->bssid, ETH_ALEN);
	memcpy(pspoll->ta, mac, ETH_ALEN);

	rcu_read_lock();
	mac80211_dragonite_tx_frame(data->hw, skb,
				rcu_dereference(vif->chanctx_conf)->def.chan);
	rcu_read_unlock();
}

static void dragonite_send_nullfunc(struct mac80211_dragonite_data *data, u8 *mac,
				struct ieee80211_vif *vif, int ps)
{
	struct dragonite_vif_priv *vp = (void *)vif->drv_priv;
	struct sk_buff *skb;
	struct ieee80211_hdr *hdr;

	if (!vp->assoc)
		return;

	wiphy_debug(data->hw->wiphy,
		    "%s: send data::nullfunc to %pM ps=%d\n",
		    __func__, vp->bssid, ps);

	skb = dev_alloc_skb(sizeof(*hdr));
	if (!skb)
		return;
	hdr = (void *) skb_put(skb, sizeof(*hdr) - ETH_ALEN);
	hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					 IEEE80211_STYPE_NULLFUNC |
					 (ps ? IEEE80211_FCTL_PM : 0));
	hdr->duration_id = cpu_to_le16(0);
	memcpy(hdr->addr1, vp->bssid, ETH_ALEN);
	memcpy(hdr->addr2, mac, ETH_ALEN);
	memcpy(hdr->addr3, vp->bssid, ETH_ALEN);

	rcu_read_lock();
	mac80211_dragonite_tx_frame(data->hw, skb,
				rcu_dereference(vif->chanctx_conf)->def.chan);
	rcu_read_unlock();
}


static void dragonite_send_nullfunc_ps(void *dat, u8 *mac,
				   struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_data *data = dat;
	dragonite_send_nullfunc(data, mac, vif, 1);
}


static void dragonite_send_nullfunc_no_ps(void *dat, u8 *mac,
				      struct ieee80211_vif *vif)
{
	struct mac80211_dragonite_data *data = dat;
	dragonite_send_nullfunc(data, mac, vif, 0);
}


static int dragonite_fops_ps_read(void *dat, u64 *val)
{
	struct mac80211_dragonite_data *data = dat;
	*val = data->ps;
	return 0;
}

static int dragonite_fops_ps_write(void *dat, u64 val)
{
	struct mac80211_dragonite_data *data = dat;
	enum ps_mode old_ps;

	if (val != PS_DISABLED && val != PS_ENABLED && val != PS_AUTO_POLL &&
	    val != PS_MANUAL_POLL)
		return -EINVAL;

	old_ps = data->ps;
	data->ps = val;

	if (val == PS_MANUAL_POLL) {
		ieee80211_iterate_active_interfaces(data->hw,
						    IEEE80211_IFACE_ITER_NORMAL,
						    dragonite_send_ps_poll, data);
		data->ps_poll_pending = true;
	} else if (old_ps == PS_DISABLED && val != PS_DISABLED) {
		ieee80211_iterate_active_interfaces(data->hw,
						    IEEE80211_IFACE_ITER_NORMAL,
						    dragonite_send_nullfunc_ps,
						    data);
	} else if (old_ps != PS_DISABLED && val == PS_DISABLED) {
		ieee80211_iterate_active_interfaces(data->hw,
						    IEEE80211_IFACE_ITER_NORMAL,
						    dragonite_send_nullfunc_no_ps,
						    data);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dragonite_fops_ps, dragonite_fops_ps_read, dragonite_fops_ps_write,
			"%llu\n");


static int dragonite_fops_group_read(void *dat, u64 *val)
{
	struct mac80211_dragonite_data *data = dat;
	*val = data->group;
	return 0;
}

static int dragonite_fops_group_write(void *dat, u64 val)
{
	struct mac80211_dragonite_data *data = dat;
	data->group = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dragonite_fops_group,
			dragonite_fops_group_read, dragonite_fops_group_write,
			"%llx\n");

static struct mac80211_dragonite_data *get_dragonite_data_ref_from_addr(
			     struct mac_address *addr)
{
	struct mac80211_dragonite_data *data;
	bool _found = false;

	spin_lock_bh(&dragonite_radio_lock);
	list_for_each_entry(data, &dragonite_radios, list) {
		if (memcmp(data->addresses[1].addr, addr,
			  sizeof(struct mac_address)) == 0) {
			_found = true;
			break;
		}
	}
	spin_unlock_bh(&dragonite_radio_lock);

	if (!_found)
		return NULL;

	return data;
}

static int dragonite_tx_info_frame_received_nl(struct sk_buff *skb_2,
					   struct genl_info *info)
{

	struct ieee80211_hdr *hdr;
	struct mac80211_dragonite_data *data2;
	struct ieee80211_tx_info *txi;
	struct dragonite_tx_rate *tx_attempts;
	unsigned long ret_skb_ptr;
	struct sk_buff *skb, *tmp;
	struct mac_address *src;
	unsigned int dragonite_flags;

	int i;
	bool found = false;

	if (!info->attrs[DRAGONITE_ATTR_ADDR_TRANSMITTER] ||
	   !info->attrs[DRAGONITE_ATTR_FLAGS] ||
	   !info->attrs[DRAGONITE_ATTR_COOKIE] ||
	   !info->attrs[DRAGONITE_ATTR_TX_INFO])
		goto out;

	src = (struct mac_address *)nla_data(
				   info->attrs[DRAGONITE_ATTR_ADDR_TRANSMITTER]);
	dragonite_flags = nla_get_u32(info->attrs[DRAGONITE_ATTR_FLAGS]);

	ret_skb_ptr = nla_get_u64(info->attrs[DRAGONITE_ATTR_COOKIE]);

	data2 = get_dragonite_data_ref_from_addr(src);

	if (data2 == NULL)
		goto out;

	/* look for the skb matching the cookie passed back from user */
	skb_queue_walk_safe(&data2->pending, skb, tmp) {
		if ((unsigned long)skb == ret_skb_ptr) {
			skb_unlink(skb, &data2->pending);
			found = true;
			break;
		}
	}

	/* not found */
	if (!found)
		goto out;

	/* Tx info received because the frame was broadcasted on user space,
	 so we get all the necessary info: tx attempts and skb control buff */

	tx_attempts = (struct dragonite_tx_rate *)nla_data(
		       info->attrs[DRAGONITE_ATTR_TX_INFO]);

	/* now send back TX status */
	txi = IEEE80211_SKB_CB(skb);

	ieee80211_tx_info_clear_status(txi);

	for (i = 0; i < IEEE80211_TX_MAX_RATES; i++) {
		txi->status.rates[i].idx = tx_attempts[i].idx;
		txi->status.rates[i].count = tx_attempts[i].count;
		/*txi->status.rates[i].flags = 0;*/
	}

	txi->status.ack_signal = nla_get_u32(info->attrs[DRAGONITE_ATTR_SIGNAL]);

	if (!(dragonite_flags & DRAGONITE_TX_CTL_NO_ACK) &&
	   (dragonite_flags & DRAGONITE_TX_STAT_ACK)) {
		if (skb->len >= 16) {
			hdr = (struct ieee80211_hdr *) skb->data;
			mac80211_dragonite_monitor_ack(txi->rate_driver_data[0],
						   hdr->addr2);
		}
		txi->flags |= IEEE80211_TX_STAT_ACK;
	}
	ieee80211_tx_status_irqsafe(data2->hw, skb);
	return 0;
out:
	return -EINVAL;

}

static int dragonite_cloned_frame_received_nl(struct sk_buff *skb_2,
					  struct genl_info *info)
{

	struct mac80211_dragonite_data *data2;
	struct ieee80211_rx_status rx_status;
	struct mac_address *dst;
	int frame_data_len;
	char *frame_data;
	struct sk_buff *skb = NULL;

	if (!info->attrs[DRAGONITE_ATTR_ADDR_RECEIVER] ||
	    !info->attrs[DRAGONITE_ATTR_FRAME] ||
	    !info->attrs[DRAGONITE_ATTR_RX_RATE] ||
	    !info->attrs[DRAGONITE_ATTR_SIGNAL])
		goto out;

	dst = (struct mac_address *)nla_data(
				   info->attrs[DRAGONITE_ATTR_ADDR_RECEIVER]);

	frame_data_len = nla_len(info->attrs[DRAGONITE_ATTR_FRAME]);
	frame_data = (char *)nla_data(info->attrs[DRAGONITE_ATTR_FRAME]);

	/* Allocate new skb here */
	skb = alloc_skb(frame_data_len, GFP_KERNEL);
	if (skb == NULL)
		goto err;

	if (frame_data_len <= IEEE80211_MAX_DATA_LEN) {
		/* Copy the data */
		memcpy(skb_put(skb, frame_data_len), frame_data,
		       frame_data_len);
	} else
		goto err;

	data2 = get_dragonite_data_ref_from_addr(dst);

	if (data2 == NULL)
		goto out;

	/* check if radio is configured properly */

	if (data2->idle || !data2->started)
		goto out;

	/*A frame is received from user space*/
	memset(&rx_status, 0, sizeof(rx_status));
	rx_status.freq = data2->channel->center_freq;
	rx_status.band = data2->channel->band;
	rx_status.rate_idx = nla_get_u32(info->attrs[DRAGONITE_ATTR_RX_RATE]);
	rx_status.signal = nla_get_u32(info->attrs[DRAGONITE_ATTR_SIGNAL]);

	memcpy(IEEE80211_SKB_RXCB(skb), &rx_status, sizeof(rx_status));
	ieee80211_rx_irqsafe(data2->hw, skb);

	return 0;
err:
	printk(KERN_DEBUG "mac80211_dragonite: error occurred in %s\n", __func__);
	goto out;
out:
	dev_kfree_skb(skb);
	return -EINVAL;
}

static int dragonite_register_received_nl(struct sk_buff *skb_2,
				      struct genl_info *info)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	if (info == NULL)
		goto out;

	wmediumd_portid = genl_info_snd_portid(info);

	printk(KERN_DEBUG "mac80211_dragonite: received a REGISTER, "
	       "switching to wmediumd mode with pid %d\n", genl_info_snd_portid(info));

	return 0;
out:
#endif
	printk(KERN_DEBUG "mac80211_dragonite: error occurred in %s\n", __func__);
	return -EINVAL;
}

/* Generic Netlink operations array */
static struct genl_ops dragonite_ops[] = {
	{
		.cmd = DRAGONITE_CMD_REGISTER,
		.policy = dragonite_genl_policy,
		.doit = dragonite_register_received_nl,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = DRAGONITE_CMD_FRAME,
		.policy = dragonite_genl_policy,
		.doit = dragonite_cloned_frame_received_nl,
	},
	{
		.cmd = DRAGONITE_CMD_TX_INFO_FRAME,
		.policy = dragonite_genl_policy,
		.doit = dragonite_tx_info_frame_received_nl,
	},
};

static int mac80211_dragonite_netlink_notify(struct notifier_block *nb,
					 unsigned long state,
					 void *_notify)
{
	struct netlink_notify *notify = _notify;

	if (state != NETLINK_URELEASE)
		return NOTIFY_DONE;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	//remove_user_radios(notify->portid);

	if (notify->portid == wmediumd_portid) {
		printk(KERN_INFO "mac80211_dragonite: wmediumd released netlink"
		       " socket, switching to perfect channel medium\n");
		wmediumd_portid = 0;
	}
#else
	if (netlink_notify_portid(notify) == wmediumd_portid) {
		printk(KERN_INFO "mac80211_dragonite: wmediumd released netlink"
		       " socket, switching to perfect channel medium\n");
		wmediumd_portid = 0;
	}
#endif
	return NOTIFY_DONE;

}

static struct notifier_block dragonite_netlink_notifier = {
	.notifier_call = mac80211_dragonite_netlink_notify,
};

static int dragonite_init_netlink(void)
{
	int rc;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	/* userspace test API hasn't been adjusted for multi-channel */
	if (channels > 1)
		return 0;
#endif

	printk(KERN_INFO "mac80211_dragonite: initializing netlink\n");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	rc = genl_register_family_with_ops_groups(&dragonite_genl_family,
						  dragonite_ops,
						  dragonite_mcgrps);
	if (rc)
		goto failure;

	rc = netlink_register_notifier(&dragonite_netlink_notifier);
	if (rc) {
		genl_unregister_family(&dragonite_genl_family);
		goto failure;
	}
#else
	rc = genl_register_family_with_ops(&dragonite_genl_family,
		dragonite_ops, ARRAY_SIZE(dragonite_ops));
	if (rc)
		goto failure;

	rc = netlink_register_notifier(&dragonite_netlink_notifier);
	if (rc)
		goto failure;
#endif

	return 0;

failure:
	printk(KERN_DEBUG "mac80211_dragonite: error occurred in %s\n", __func__);
	return -EINVAL;
}

static void dragonite_exit_netlink(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	/* unregister the notifier */
	netlink_unregister_notifier(&dragonite_netlink_notifier);
	/* unregister the family */
	genl_unregister_family(&dragonite_genl_family);
#else
	int ret;

	/* userspace test API hasn't been adjusted for multi-channel */
	if (channels > 1)
		return;

	printk(KERN_INFO "mac80211_dragonite: closing netlink\n");
	/* unregister the notifier */
	netlink_unregister_notifier(&dragonite_netlink_notifier);
	/* unregister the family */
	ret = genl_unregister_family(&dragonite_genl_family);
	if (ret)
		printk(KERN_DEBUG "mac80211_dragonite: "
		       "unregister family %i\n", ret);
#endif
}

static const struct ieee80211_iface_limit dragonite_if_limits[] = {
	{ .max = 1, .types = BIT(NL80211_IFTYPE_ADHOC) },
	{ .max = 2048,  .types = BIT(NL80211_IFTYPE_STATION) |
				 BIT(NL80211_IFTYPE_P2P_CLIENT) |
#ifdef CPTCFG_MAC80211_MESH
				 BIT(NL80211_IFTYPE_MESH_POINT) |
#endif
				 BIT(NL80211_IFTYPE_AP) |
				 BIT(NL80211_IFTYPE_P2P_GO) },
	{ .max = 1, .types = BIT(NL80211_IFTYPE_P2P_DEVICE) },
};

static struct ieee80211_iface_combination dragonite_if_comb = {
	.limits = dragonite_if_limits,
	.n_limits = ARRAY_SIZE(dragonite_if_limits),
	.max_interfaces = 2048,
#if defined (CONFIG_PANTHER_WLAN)
	.num_different_channels = 2,
#else
	.num_different_channels = 1,
#endif
};

static int __init init_mac80211_dragonite(void)
{
	int i, err = 0;
	u8 addr[ETH_ALEN];
	struct mac80211_dragonite_data *data;
	struct ieee80211_hw *hw;
	enum ieee80211_band band;

	//camelot_lib_test();

#if defined(DRAGONITE_DBUS_PRIORITY_OVER_CPU)
    REG_UPDATE32(0xbf004e10, 0x80, 0xC0);
#endif

#if defined(DRAGONITE_LIMIT_CPU_PENDING_BUS_RW_REQUESTS)
    REG_UPDATE32(0xbf004e10, 0x00010100, 0x001F1F00);
#endif

    switch (pmu_get_wifi_powercfg())
    {
        case POWERCTL_DYNAMIC:
            dragonite_dynamic_ps = 1;
            break;
        case POWERCTL_STATIC_OFF:
            return -EINVAL;
            break;
        default:
            break;
    }

    if (radios < 1 || radios > 100)
		return -EINVAL;

	if (channels < 1)
		return -EINVAL;

	if (channels > 1) {
		dragonite_if_comb.num_different_channels = channels;
		mac80211_dragonite_ops.hw_scan = mac80211_dragonite_hw_scan;
		mac80211_dragonite_ops.cancel_hw_scan =
			mac80211_dragonite_cancel_hw_scan;
		mac80211_dragonite_ops.sw_scan_start = NULL;
		mac80211_dragonite_ops.sw_scan_complete = NULL;
		mac80211_dragonite_ops.remain_on_channel =
			mac80211_dragonite_roc;
		mac80211_dragonite_ops.cancel_remain_on_channel =
			mac80211_dragonite_croc;
		mac80211_dragonite_ops.add_chanctx =
			mac80211_dragonite_add_chanctx;
		mac80211_dragonite_ops.remove_chanctx =
			mac80211_dragonite_remove_chanctx;
		mac80211_dragonite_ops.change_chanctx =
			mac80211_dragonite_change_chanctx;
		mac80211_dragonite_ops.assign_vif_chanctx =
			mac80211_dragonite_assign_vif_chanctx;
		mac80211_dragonite_ops.unassign_vif_chanctx =
			mac80211_dragonite_unassign_vif_chanctx;
	}

	spin_lock_init(&dragonite_radio_lock);
	INIT_LIST_HEAD(&dragonite_radios);

	err = platform_driver_register(&mac80211_dragonite_driver);
	if (err)
		return err;

	dragonite_class = class_create(THIS_MODULE, "mac80211_dragonite");
	if (IS_ERR(dragonite_class)) {
		err = PTR_ERR(dragonite_class);
		goto failed_unregister_driver;
	}

	err = dragonite_init_netlink();
	if (err < 0)
		goto failed_unregister_driver;

	random_ether_addr((u8 *) &addr);

	for (i = 0; i < radios; i++) {
		printk(KERN_DEBUG "mac80211_dragonite: Initializing radio %d\n",
		       i);
		hw = ieee80211_alloc_hw(sizeof(*data), &mac80211_dragonite_ops);
		if (!hw) {
			printk(KERN_DEBUG "mac80211_dragonite: ieee80211_alloc_hw "
			       "failed\n");
			err = -ENOMEM;
			goto failed;
		}
		data = hw->priv;
		data->hw = hw;

		data->dev = device_create(dragonite_class, NULL, 0, hw,
					  "dragonite%d", i);
		if (IS_ERR(data->dev)) {
			printk(KERN_DEBUG
			       "mac80211_dragonite: device_create failed (%ld)\n",
			       PTR_ERR(data->dev));
			err = -ENOMEM;
			goto failed_drvdata;
		}
		data->dev->driver = &mac80211_dragonite_driver.driver;
		err = device_bind_driver(data->dev);
		if (err != 0) {
			printk(KERN_DEBUG
			       "mac80211_dragonite: device_bind_driver failed (%d)\n",
			       err);
			goto failed_hw;
		}

		skb_queue_head_init(&data->pending);

		SET_IEEE80211_DEV(hw, data->dev);
		//addr[3] = i >> 8;
		//addr[4] = i;
		memcpy(data->addresses[0].addr, addr, ETH_ALEN);
		memcpy(data->addresses[1].addr, addr, ETH_ALEN);
		data->addresses[1].addr[0] |= 0x40;
		hw->wiphy->n_addresses = 2;
		hw->wiphy->addresses = data->addresses;

		hw->wiphy->iface_combinations = &dragonite_if_comb;
		hw->wiphy->n_iface_combinations = 1;

		if (channels > 1) {
			hw->wiphy->max_scan_ssids = 255;
			hw->wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;
			hw->wiphy->max_remain_on_channel_duration = 1000;
		}

		INIT_DELAYED_WORK(&data->roc_done, hw_roc_done);
		INIT_DELAYED_WORK(&data->hw_scan, hw_scan_work);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
		hw->channel_change_time = 1;
#endif
		hw->queues = 5;
		hw->offchannel_tx_hw_queue = 4;
		hw->wiphy->interface_modes =
			BIT(NL80211_IFTYPE_STATION) |
			BIT(NL80211_IFTYPE_AP) |
			BIT(NL80211_IFTYPE_P2P_CLIENT) |
			BIT(NL80211_IFTYPE_P2P_GO) |
			BIT(NL80211_IFTYPE_ADHOC) |
			BIT(NL80211_IFTYPE_MESH_POINT) |
			BIT(NL80211_IFTYPE_P2P_DEVICE);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    	ieee80211_hw_set(hw, SUPPORT_FAST_XMIT);
    	ieee80211_hw_set(hw, CHANCTX_STA_CSA);
    	ieee80211_hw_set(hw, SUPPORTS_HT_CCK_RATES);
    	ieee80211_hw_set(hw, QUEUE_CONTROL);
    	ieee80211_hw_set(hw, WANT_MONITOR_VIF);
    	ieee80211_hw_set(hw, AMPDU_AGGREGATION);
    	ieee80211_hw_set(hw, MFP_CAPABLE);
    	ieee80211_hw_set(hw, SIGNAL_DBM);
    	//ieee80211_hw_set(hw, TDLS_WIDER_BW);
    	ieee80211_hw_set(hw, QUEUE_CONTROL);
    	ieee80211_hw_set(hw, AP_LINK_PS);
    	ieee80211_hw_set(hw, REPORTS_TX_ACK_STATUS);
#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
    	ieee80211_hw_set(hw, SUPPORTS_PS);
    	ieee80211_hw_set(hw, PS_NULLFUNC_STACK);
#endif
    	if (rctbl)
    		ieee80211_hw_set(hw, SUPPORTS_RC_TABLE);

		hw->wiphy->flags |=
			    WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
#if 0  // TODO
		hw->wiphy->flags |=
			    WIPHY_FLAG_HAS_CHANNEL_SWITCH;
#endif

		hw->wiphy->features |= NL80211_FEATURE_ACTIVE_MONITOR |
			       NL80211_FEATURE_SK_TX_STATUS |
			       /* NL80211_FEATURE_NEED_OBSS_SCAN makes wpa_supplicant do obss scan */
			       NL80211_FEATURE_AP_MODE_CHAN_WIDTH_CHANGE;

#if defined (DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
		if(dragonite_dynamic_ps)
		{
			hw->wiphy->flags |= WIPHY_FLAG_PS_ON_BY_DEFAULT;
		}
		else
		{
			hw->wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;
		}
#else
		hw->wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;
#endif
#if defined(CONFIG_MAC80211_MESH)
		hw->wiphy->flags |= WIPHY_FLAG_IBSS_RSN;
#endif
#else
		hw->flags = IEEE80211_HW_MFP_CAPABLE |
			    IEEE80211_HW_SIGNAL_DBM |
			    IEEE80211_HW_SUPPORTS_STATIC_SMPS |
			    IEEE80211_HW_SUPPORTS_DYNAMIC_SMPS |
			    IEEE80211_HW_AMPDU_AGGREGATION |
			    IEEE80211_HW_WANT_MONITOR_VIF |
			    IEEE80211_HW_QUEUE_CONTROL |
				IEEE80211_HW_AP_LINK_PS;
		if (rctbl)
			hw->flags |= IEEE80211_HW_SUPPORTS_RC_TABLE;

		hw->wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS |
				    WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
#endif

		/* ask mac80211 to reserve space for magic */
		hw->vif_data_size = sizeof(struct dragonite_vif_priv);
		hw->sta_data_size = sizeof(struct dragonite_sta_priv);
		hw->chanctx_data_size = sizeof(struct dragonite_chanctx_priv);

		memcpy(data->channels_2ghz, dragonite_channels_2ghz,
			sizeof(dragonite_channels_2ghz));
		memcpy(data->rates, dragonite_rates, sizeof(dragonite_rates));

		for (band = IEEE80211_BAND_2GHZ; band < 1; band++) {
			struct ieee80211_supported_band *sband = &data->bands[band];
			switch (band) {
			case IEEE80211_BAND_2GHZ:
				sband->channels = data->channels_2ghz;
				sband->n_channels =
					ARRAY_SIZE(dragonite_channels_2ghz);
				sband->bitrates = data->rates;
				sband->n_bitrates = ARRAY_SIZE(dragonite_rates);
				break;
			default:
				continue;
			}

			sband->ht_cap.ht_supported = true;
#if 1  /* disable 40Mhz support */
			sband->ht_cap.cap = IEEE80211_HT_CAP_GRN_FLD |
				IEEE80211_HT_CAP_SGI_20;
#else
			sband->ht_cap.cap = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
				IEEE80211_HT_CAP_GRN_FLD |
				IEEE80211_HT_CAP_SGI_20 |
				IEEE80211_HT_CAP_SGI_40 |
				IEEE80211_HT_CAP_DSSSCCK40;
#endif
			sband->ht_cap.ampdu_factor = 0x3;
			sband->ht_cap.ampdu_density = 0x6;
			memset(&sband->ht_cap.mcs, 0,
			       sizeof(sband->ht_cap.mcs));
			sband->ht_cap.mcs.rx_mask[0] = 0xff;
			sband->ht_cap.mcs.rx_mask[4] = 0x01;
			sband->ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;

			hw->wiphy->bands[band] = sband;

			sband->vht_cap.vht_supported = false;

		}
		/* By default all radios are belonging to the first group */
		data->group = 1;
		mutex_init(&data->mutex);

        /* for skb->data to be properly aligned, DRAGONITE_TXDESCR_SIZE + TX_DESCR_SIZE + CACHED_LINE = 64 + 48 + 32 <= 144*/
        hw->extra_tx_headroom = TX_DESC_HEADROOM;

		/* Enable frame retransmissions for lossy channels */
		hw->max_rates = 3;
		hw->max_rate_tries = 2;

		/* Work to be done prior to ieee80211_register_hw() */
		switch (regtest) {
		case DRAGONITE_REGTEST_DISABLED:
		case DRAGONITE_REGTEST_DRIVER_REG_FOLLOW:
		case DRAGONITE_REGTEST_DRIVER_REG_ALL:
		case DRAGONITE_REGTEST_DIFF_COUNTRY:
			/*
			 * Nothing to be done for driver regulatory domain
			 * hints prior to ieee80211_register_hw()
			 */
			break;
        case DRAGONITE_REGTEST_WORLD_ROAM:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
			if (i == 0) {
				hw->wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&dragonite_world_regdom_custom_01);
			}
#else
			if (i == 0) {
				hw->wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&dragonite_world_regdom_custom_01);
			}
#endif
			break;
        case DRAGONITE_REGTEST_CUSTOM_WORLD:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
			hw->wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
			wiphy_apply_custom_regulatory(hw->wiphy,
				&dragonite_world_regdom_custom_01);
#else
			hw->wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
			wiphy_apply_custom_regulatory(hw->wiphy,
				&dragonite_world_regdom_custom_01);
#endif
			break;
        case DRAGONITE_REGTEST_CUSTOM_WORLD_2:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
			if (i == 0) {
				hw->wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&dragonite_world_regdom_custom_01);
			}
#else
			if (i == 0) {
				hw->wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&dragonite_world_regdom_custom_01);
			}
#endif
			break;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
		case DRAGONITE_REGTEST_STRICT_ALL:
			hw->wiphy->flags |= WIPHY_FLAG_STRICT_REGULATORY;
			break;
		case DRAGONITE_REGTEST_STRICT_FOLLOW:
		case DRAGONITE_REGTEST_STRICT_AND_DRIVER_REG:
			if (i == 0)
				hw->wiphy->flags |= WIPHY_FLAG_STRICT_REGULATORY;
			break;
#endif
        case DRAGONITE_REGTEST_ALL:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
			if (i == 0) {
				hw->wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&dragonite_world_regdom_custom_01);
			}
#else
			if (i == 0) {
				hw->wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&dragonite_world_regdom_custom_01);
			}
#endif
			break;
		default:
			break;
		}

		/* give the regulatory workqueue a chance to run */
		if (regtest)
			schedule_timeout_interruptible(1);
		err = ieee80211_register_hw(hw);
		if (err < 0) {
			printk(KERN_DEBUG "mac80211_dragonite: "
			       "ieee80211_register_hw failed (%d)\n", err);
			goto failed_hw;
		}

		/* Work to be done after to ieee80211_register_hw() */
		switch (regtest) {
		case DRAGONITE_REGTEST_WORLD_ROAM:
		case DRAGONITE_REGTEST_DISABLED:
			break;
		case DRAGONITE_REGTEST_DRIVER_REG_FOLLOW:
			if (!i)
				regulatory_hint(hw->wiphy, dragonite_alpha2s[0]);
			break;
		case DRAGONITE_REGTEST_DRIVER_REG_ALL:
		case DRAGONITE_REGTEST_STRICT_ALL:
			regulatory_hint(hw->wiphy, dragonite_alpha2s[0]);
			break;
		case DRAGONITE_REGTEST_DIFF_COUNTRY:
			if (i < ARRAY_SIZE(dragonite_alpha2s))
				regulatory_hint(hw->wiphy, dragonite_alpha2s[i]);
			break;
		case DRAGONITE_REGTEST_CUSTOM_WORLD:
		case DRAGONITE_REGTEST_CUSTOM_WORLD_2:
			/*
			 * Nothing to be done for custom world regulatory
			 * domains after to ieee80211_register_hw
			 */
			break;
		case DRAGONITE_REGTEST_STRICT_FOLLOW:
			if (i == 0)
				regulatory_hint(hw->wiphy, dragonite_alpha2s[0]);
			break;
		case DRAGONITE_REGTEST_STRICT_AND_DRIVER_REG:
			if (i == 0)
				regulatory_hint(hw->wiphy, dragonite_alpha2s[0]);
			break;
		case DRAGONITE_REGTEST_ALL:
			break;
		default:
			break;
		}

		wiphy_debug(hw->wiphy, "hwaddr %pm registered\n",
			    hw->wiphy->perm_addr);
		wla_debug_init(data);
		rfc_test_init();
		lrf_test_init();
		wla_procfs_init(data);

		data->debugfs = debugfs_create_dir("dragonite",
						   hw->wiphy->debugfsdir);
		data->debugfs_ps = debugfs_create_file("ps", 0666,
						       data->debugfs, data,
						       &dragonite_fops_ps);
		data->debugfs_group = debugfs_create_file("group", 0666,
							data->debugfs, data,
							&dragonite_fops_group);

		list_add_tail(&data->list, &dragonite_radios);
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	dragonite_mon = alloc_netdev(0, "dragonite%d", NET_NAME_UNKNOWN, dragonite_mon_setup);
#else
	dragonite_mon = alloc_netdev(0, "dragonite%d", dragonite_mon_setup);
#endif
	if (dragonite_mon == NULL)
		goto failed;

	rtnl_lock();

	err = dev_alloc_name(dragonite_mon, dragonite_mon->name);
	if (err < 0)
		goto failed_mon;


	err = register_netdevice(dragonite_mon);
	if (err < 0)
		goto failed_mon;

	rtnl_unlock();

	rf_init();

	bb_init();

	panther_get_tx_config();

#ifdef CONFIG_MONTE_CARLO
	bb_register_write(0, 0x5A, 0);//for Combination with panther FPGA with Monte calro
#else
	panther_rfc_process();
#endif
	schedule_timeout_interruptible(HZ/10);

	pmu_reset_wifi_mac();

	wifi_mac_reset_check();

	return 0;

failed_mon:
	rtnl_unlock();
	free_netdev(dragonite_mon);
	mac80211_dragonite_free();
	return err;

failed_hw:
	device_unregister(data->dev);
failed_drvdata:
	ieee80211_free_hw(hw);
failed:
	mac80211_dragonite_free();
failed_unregister_driver:
	platform_driver_unregister(&mac80211_dragonite_driver);
	return err;
}
late_initcall(init_mac80211_dragonite);

static void __exit exit_mac80211_dragonite(void)
{
	printk(KERN_DEBUG "mac80211_dragonite: unregister radios\n");

	dragonite_exit_netlink();

	mac80211_dragonite_free();
	unregister_netdev(dragonite_mon);
	platform_driver_unregister(&mac80211_dragonite_driver);
}
module_exit(exit_mac80211_dragonite);
