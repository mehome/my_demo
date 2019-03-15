#ifndef __BEACON_H__
#define __BEACON_H__

void dragonite_beacon_setup(unsigned short beacon_interval, unsigned char dtime_period);
void dragonite_beacon_start(unsigned long beacon_bitrate);
void dragonite_beacon_stop(void);
int dragonite_tx_beacons(struct ieee80211_hw *hw, struct sk_buff *skb_list[]);
void dragonite_cfg_start_tsync(u32 tsf_idx, u32 beacon_interval, u32 dtim_interval, u32 timeout);
void dragonite_tsf_intr_handler(struct mac80211_dragonite_data *data);
#define TEST_TSF_START  0x100000ULL
#define TIME_UNIT    1024
#if defined (DRAGONITE_BEACON_CHECKER)
#define TBTT_INTERVAL(beacon_interval) (TIME_UNIT * (beacon_interval))
#define TBTT_INTERRUPT_REACTIVE_TOLERANT (TBTT_INTERVAL((100)) * 1/3 * 1/2) /* 1024 TU x PRE_TBTT interval x reactive tolerant */
#define BCQ_PROCESS_TIME (TBTT_INTERRUPT_REACTIVE_TOLERANT/2)
#endif
#define TS_ERROR_LIMIT 10
#define TS_ERROR_MUTE_TIME (60 * 30 * HZ) /* 30 mins */
#define GLOBAL_PRE_TBTT_DEFAULT_DIV_S(beacon_interval) (beacon_interval / 3)
#define GLOBAL_PRE_TBTT_DEFAULT_DIV(beacon_interval) (beacon_interval / 30)
#endif //__BEACON_H__
