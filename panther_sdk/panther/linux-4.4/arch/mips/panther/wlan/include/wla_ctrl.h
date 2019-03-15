#ifndef __WLA_CTRL_H__
#define __WLA_CTRL_H__

extern unsigned int drg_tx_acq_delay_max_cnt;
extern unsigned int drg_tx_acq_delay_interval;
extern volatile unsigned int drg_wifi_recover;
extern unsigned int drg_rf_bb_init_once;
extern int drg_wifi_recover_cnt;
extern int dragonite_tx_ampdu_enable;
extern int dragonite_rx_ampdu_enable;
extern int dragonite_tx_ampdu_vo_enable;
extern int dragonite_tx_debug_dump;
extern int dragonite_rx_debug_dump;
extern int dragonite_tx_per_ac_max_buf;
extern int enable_swbr_shortcut;
extern int enable_swbr_debug;
extern unsigned int dragonite_debug_flag;
extern unsigned int drg_tx_winsz;
extern unsigned int drg_rx_winsz;
extern int drg_tx_protect;
extern int drg_wifi_poweroff_enable;
extern int tcp_active_session_tracking_debug;
extern int dynamic_ps_timeout_override;
extern int dynamic_ps_postpone_time;
#endif
