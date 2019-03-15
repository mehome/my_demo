#ifndef __SKB_H__
#define __SKB_H__

#include <os_compat.h>
#include <dragonite_main.h>
#include <linux/skbuff.h>

#define SKB_INFO_FLAGS_DATA_FRAME           0x00000001UL  /* frame control type DATA */
#define SKB_INFO_FLAGS_NULL_FUNCTION        0x00000002UL  /* null-data */
#define SKB_INFO_FLAGS_MULTICAST_FRAME      0x00000004UL  /* addr1 has multicast bit on */
#define SKB_INFO_FLAGS_QOS_FRAME            0x00000008UL  /* contains qos control field */
#define SKB_INFO_FLAGS_FORCE_SECURITY_OFF   0x00000010UL
#define SKB_INFO_FLAGS_USE_GLOBAL_SEQUENCE_NUMBER   0x00000020UL
#define SKB_INFO_FLAGS_PAE_FRAME            0x00000040UL  /* the data packet contains 802.1X or WAPI PAE */
#define SKB_INFO_FLAGS_BROADCAST_FRAME      0x00000080UL  /* addr1 is broadcast */

#define SKB_INFO_FLAGS_DA_IS_MULTICAST      0x00000100UL
#define SKB_INFO_FLAGS_DA_IS_BROADCAST      0x00000200UL

#define SKB_INFO_FLAGS_PROTECTED_FRAME      0x00000400UL   /* packet with protected frame bit on */
#define SKB_INFO_FLAGS_WDS_FRAME            0x00000800UL   /* contains addr4 in Wifi header */
#define SKB_INFO_FLAGS_VALID_FOR_FORWARDING 0x00001000UL   /* eligible for WDS/Ethernet forwarding */

#define SKB_INFO_FLAGS_VALID_WDS_FOR_FORWARDING 0x00002000UL  /* WDS packet only:
                                                                   this will indicate if this WDS frame is valid for forwarding
                                                               */
#define SKB_INFO_FLAGS_DA_IS_BSSID          0x00004000UL
#define SKB_INFO_FLAGS_BEACON_FRAME         0x00008000UL

#define SKB_INFO_FLAGS_INSERT_TIMESTAMP     0x00010000UL

#define SKB_INFO_FLAGS_NO_BA_SESSION                      0x00020000UL   /* do not sent within a BA session (via PSBAQ) */
#define SKB_INFO_FLAGS_FORCE_MPDU_ON_BA_SESSION           0x00040000UL   /* force the TX to use MPDU if it is within a BA session */
#define SKB_INFO_FLAGS_ACTION_FRAME                       0x00080000UL

#define SKB_INFO_FLAGS_MGMT_FRAME                         0x00100000UL   /* frame control type MGMT */
#define SKB_INFO_FLAGS_BUFFERABLE_MGMT_FRAME              0x00200000UL   /* frame control type MGMT with BUFFERABLE */

#define SKB_INFO_FLAGS_PS_POLL_RESPONSE                   0x00400000UL   /* dragonite tx ps poll response */
#define SKB_INFO_FLAGS_PS_POLL_MORE_DATA                  0x00800000UL   /* dragonite tx ps poll more data */

#define SKB_INFO_FLAGS_BAR_FRAME                          0x01000000UL   /* dragonite this is bar frame */

#define SKB_INFO_FLAGS_STA_WIFI_RX                        0x02000000UL   /* this frame is receive from sta bss */

#define SKB_INFO_FLAGS_USE_ORIGINAL_SEQUENCE_NUMBER       0x10000000UL   /* dragonite use original sequece number */
#define SKB_INFO_FLAGS_SPECIAL_DATA_FRAME                 0x20000000UL   /* dragonite this data frame can not drop */
#define SKB_INFO_FLAGS_IN_HW                              0x40000000UL   /* dragonite kick this skb in hw */
#define SKB_INFO_FLAGS_USE_SINGLE_QUEUE                   0x80000000UL   /* dragonite tx use single queue to send*/

#define SKBDATA_STATIC_ALLOCATED_MAGIC      0xA7FFFFFFUL

#define TX_SKB_DROP 2
#define TX_SKB_SUCCESS 1
#define TX_SKB_FAIL 0

#define EDCA_LEGACY_FRAME_TID					8
#define EDCA_HIGHEST_PRIORITY_TID				7

#if defined (DRAGONITE_TX_SKB_CHECKER)
#define DRG_TX_SKB_CHECKER_PERIOD 10 //sec
#define DRG_TX_SKB_TIMEOUT_TOLERANT 10 //sec
#endif

struct skb_info
{
    u32 flags;
    int tid;
    int header_length;
    int bssid_index;
    int peer_ap_index;
    int da_index;
    int sa_index;

    int ssn;
};

void drg_check_in_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb);
void drg_check_out_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, bool success);
#if defined (DRAGONITE_TX_SKB_CHECKER)
void skb_list_trace_timeout(struct mac80211_dragonite_data *data, u32 wakeup_t);
#endif
void dragonite_dump_tx_skb(struct sk_buff *skb);
void dragonite_dump_rx_skb(struct sk_buff *skb);
#if defined(CONFIG_MAC80211_MESH) || defined(DRAGONITE_SUPPORT_POWERSAVE_IN_STATION_MODE)
int dragonite_parse_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo, struct ieee80211_vif *vif);
#else
int dragonite_parse_tx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo);
#endif
int dragonite_parse_rx_skb(struct mac80211_dragonite_data *data, struct sk_buff *skb, struct skb_info *skbinfo);
void dragonite_tx_replace_ssn(struct sk_buff *skb, u16 ssn);
#endif
