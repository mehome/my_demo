/* Copyright 2015, 2017   Montage Inc. All right reserved  */
#include <linux/skbuff.h>
#include <dragonite_main.h>
#include <mac.h>
#include <mac_ctrl.h>
#include <mac_tables.h>
#include <skb.h>
#include <tx.h>

#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/ctype.h>
#include <linux/etherdevice.h>

#include <bb.h>
#include <rf.h>

#include <net/regulatory.h>
#include <net/cfg80211.h>

#include <omnicfg.h>

#include "dragonite_common.h"
#include "mac_regs.h"

extern const struct ieee80211_regdomain *cfg80211_regdomain;

extern struct mac80211_dragonite_data *global_data;

#define g_wifi_sc global_data

static void omnicfg_change_channel(struct work_struct *work);
static DECLARE_DELAYED_WORK(omnicfg_work, omnicfg_change_channel);

static void omnicfg_inform_user(struct work_struct *work);
static DECLARE_DELAYED_WORK(omnicfg_usrwork, omnicfg_inform_user);

static DEFINE_SPINLOCK(omnicfg_spinlock);

static unsigned long omnicfg_lock_irqflags;

static inline void omnicfg_lock(void)
{
    spin_lock_irqsave(&omnicfg_spinlock, omnicfg_lock_irqflags);
}

static inline void omnicfg_unlock(void)
{
    spin_unlock_irqrestore(&omnicfg_spinlock, omnicfg_lock_irqflags);
}

//#define MIMO_DEBUG

#define OC_DBG(...) printk(KERN_NOTICE __VA_ARGS__)

#define CHANNEL_HOPPING

#define CHANNEL_BLACKLIST

#define ENABLE_40MHZ_CHANNEL

//#define ENABLE_MAC_RECOVERY

#define PLAY_OMNI_VOICE

#define HANDLE_ANDROID_PROBE_REQUEST    /* for Android devices connected to 5GHz band of dualband AP */

#define NEW_LOCK_MATCH

#define TEST_MIMO
#define TEST_SISO2_AP
#define TEST_SISO2_STA

struct omnicfg __omnicfg_cfg;
struct omnicfg *cfg = &__omnicfg_cfg;

u32 __omnicfg_enable = 0;
u32 __omnicfg_dbgmask = 0;
u32 __omnicfg_filter_mode = 0;
u32 __omnicfg_voice_type = 0;
u8 *cfgdata = NULL;

u32 drg_wifi_sniffer = 0;

#define MAX_CFGDATA_SIZE 256
unsigned long cfgdata_valid[MAX_CFGDATA_SIZE / sizeof(unsigned long)];

#define OMNICFG			"/lib/wdk/omnicfg"
#define OMNI_PATH		"PATH=/usr/sbin:/usr/bin:/sbin:/bin:/lib/wdk"
#define OMNIDONE		"done"
#define OMNISTOP		"stop"
#define OMNIVOICE       "voice"

#define CHGCHANDELAY                            (HZ / 2)
#define OMNICFG_DECODE_STATUS_CHECK_INTERVAL      (HZ * 3)

#define MIMO_LOCK_ATTEMPT_TIME (HZ)

enum {
	S_SCAN_CHANNEL = 0,
	S_STAY_CHANNEL,
	S_DECODE,
    S_DONE,
	S_USRSTOP,
};

#ifdef PLAY_OMNI_VOICE
enum {
    S_VOICE_NONE = 0,
    S_VOICE_CFG,
};
#endif

#define SISO_DECODER 0
#define MIMO_DECODER 1

#define DISPATCH_SISO_SCAN      0x01
#define DISPATCH_SISO_DECODE    0x02
#define DISPATCH_MIMO_SCAN      0x04
#define DISPATCH_MIMO_DECODE    0x08

#define FILTER_MODE_DROP_ALL        0
#define FILTER_SISO_SCAN    DISPATCH_SISO_SCAN
#define FILTER_SISO_DECODE  DISPATCH_SISO_DECODE
#define FILTER_MIMO_SCAN    DISPATCH_MIMO_SCAN
#define FILTER_MIMO_DECODE  DISPATCH_MIMO_DECODE

#define SIGNATURE_NOT_FOUND         0       // no signature found
#define STAY_IN_THE_CHANNEL         1       // request to stay in the channel for futhur seeking
#define SIGNATURE_LOCKED            2       // found a candidate station

#define DECODE_FAILED               4
#define DECODE_IN_PROC              5
#define DECODE_DONE                 6

#define SIGNATURE_MAC_COUNT 2

#define SISO2_SAFE_BASE 10
#define SISO2_SAFE_RANGE 190
u8 omni_chan_multicast_addr_list[SIGNATURE_MAC_COUNT][6] = {
    {0x01, 0x00, 0x5E, 0x63, 0x68, 0x6E},
    {0x01, 0x00, 0x5E, 0x6E, 0x65, 0x6C},
};

u8 omni_cfgdata_addr_ipv4[3] = {0x01, 0x00, 0x5E};
u8 omni_cfgdata_addr_ipv6[3] = {0x33, 0x33, 0x66};
#if 1
#define OMNICFG_DEBUG_FLAG_OPEN 0x01
#define OMNICFG_DEBUG_FLAG_CLOSE 0x00
#define OMNICFG_DEBUG_FLAG_MIMO 0x02
u32 omnicfg_debug_flag = 0x00;
#endif 

u32 sniffer_rx_count;
u32 last_sniffer_rx_count;
unsigned long next_rx_count_check_time;
#define RX_COUNT_CHECK_INTERVAL     (3 * HZ)

static inline int is_cfgdata_addr_ipv4(unsigned char *mac)
{
    return ((omni_cfgdata_addr_ipv4[0]==mac[0])
            &&(omni_cfgdata_addr_ipv4[1]==mac[1])
            &&(omni_cfgdata_addr_ipv4[2]==mac[2])
            );
}
static inline int is_cfgdata_addr_ipv6(unsigned char *mac)
{
    return ((omni_cfgdata_addr_ipv6[0]==mac[0])
            &&(omni_cfgdata_addr_ipv6[1]==mac[1])
            &&(omni_cfgdata_addr_ipv6[2]==mac[2])
            );
}

int is_macaddr_all_zero(u8 *addr)
{
    return !(addr[0]|addr[1]|addr[2]|addr[3]|addr[4]|addr[5]);
}

unsigned char siso2_last_mac_addr_ipv4[6];
unsigned char siso2_last_mac_addr_ipv6[6];
unsigned char siso2_mac_addr_offset_ipv4[6];
unsigned char siso2_mac_addr_offset_ipv6[6];
unsigned int siso2_last_mac_length_ipv4;
unsigned int siso2_last_mac_length_ipv6;
unsigned int siso2_mac_length_offset_ipv4;
unsigned int siso2_mac_length_offset_ipv6;

void disable_sniffer_mode(void)
{
    dragonite_mac_lock();

    MACREG_UPDATE32(RTSCTS_SET, 0, LMAC_FILTER_ALL_PASS);
    MACREG_UPDATE32(RX_ACK_POLICY, 0, RX_SNIFFER_MODE);

    /* restore ERR_EN to default value */
    MACREG_WRITE32(ERR_EN, FASTPATH_WIFI_TO_WIFI | ERR_EN_SEC_MISMATCH_TOCPU | ERR_EN_TID_ERROR_TOCPU | ERR_EN_BSSID_CONS_ERROR_TOCPU
                   | ERR_EN_MANGMENT_TA_MISS_TOCPU | ERR_EN_DATA_TA_MISS_TOCPU);
    /* restore MAC_RX_FILTER to default value */
    MACREG_WRITE32(MAC_RX_FILTER, RXF_GC_MGT_TA_HIT | RXF_GC_DAT_TA_HIT | RXF_UC_MGT_RA_HIT | RXF_UC_DAT_RA_HIT
                   | RXF_BEACON_ALL | RXF_PROBE_REQ_ALL);

    bb_register_write(2, 0x5, 0x3);

    dragonite_mac_unlock();
}

void enable_sniffer_mode(void)
{
    dragonite_mac_lock();

    MACREG_UPDATE32(RTSCTS_SET, LMAC_FILTER_ALL_PASS, LMAC_FILTER_ALL_PASS);
    MACREG_UPDATE32(RX_ACK_POLICY, RX_SNIFFER_MODE, RX_SNIFFER_MODE);

    MACREG_UPDATE32(ERR_EN, 0, ERR_EN_FCS_ERROR_TOCPU);
    MACREG_UPDATE32(ERR_EN, ERR_EN_BA_SESSION_MISMATCH, ERR_EN_BA_SESSION_MISMATCH);

    MACREG_UPDATE32(MAC_RX_FILTER, RXF_UC_DAT_ALL |RXF_GC_DAT_ALL, RXF_UC_DAT_TA_RA_HIT | RXF_GC_DAT_TA_HS_HIT);

#if defined(HANDLE_ANDROID_PROBE_REQUEST)
    MACREG_UPDATE32(MAC_RX_FILTER, RXF_PROBE_REQ_ALL, RXF_PROBE_REQ);
#endif

    bb_register_write(2, 0x5, 0xb);

    dragonite_mac_unlock();
}

#if defined(HANDLE_ANDROID_PROBE_REQUEST)

#define MAX_OMNICFG_CANDIDATE_STA           4

#define OMNICFG_CMD52_CFGDATA_BASE64_BLOCK_SIZE     20
#define OMNICFG_CMD52_CFGDATA_BLOCK_SIZE            15
#define OMNICFG_CMD52_MAX_CFGDATA_BLOCK             8

struct omnicfg_cfgdata 
{
    unsigned long bitmap;
    unsigned int total_block_num;
    unsigned int datalen;
    u8 data[OMNICFG_CMD52_CFGDATA_BLOCK_SIZE * OMNICFG_CMD52_MAX_CFGDATA_BLOCK];
};

unsigned char omnicfg_candidate_sta[MAX_OMNICFG_CANDIDATE_STA][ETH_ALEN];
struct omnicfg_cfgdata *ocfg_data[MAX_OMNICFG_CANDIDATE_STA];

int omnicfg_add_target_sta(u8 *addr)
{
    int i;

    for (i=0;i<MAX_OMNICFG_CANDIDATE_STA;i++) 
    {
        if (is_macaddr_all_zero(omnicfg_candidate_sta[i]))
        {
            memcpy(omnicfg_candidate_sta[i], addr, ETH_ALEN);

            OC_DBG("Add OMNICFG candidate STA addr %pM, idx %d\n", addr, i);
            return i;
        }
        else if (!memcmp(omnicfg_candidate_sta[i], addr, ETH_ALEN))
        {
            /* the addr already exist */
            return i;
        }
    }

    return -1;
}

#define OMICFG_CMD52_LENGTH     28

u8 *omnicfg_parse_ssid(u8 *start, size_t len)
{
	size_t left = len;
	u8 *pos = start;

	while (left >= 2) {
		u8 id, elen;

		id = *pos++;
		elen = *pos++;
		left -= 2;

		if (elen > left)
			break;

		switch (id) 
        {
            case WLAN_EID_SSID:
                if (elen>=OMICFG_CMD52_LENGTH)
                    return pos;
    			break;
    		default:
    			break;
		}

		left -= elen;
		pos += elen;
	}

	return NULL;
}

int uh_b64decode(unsigned char *buf, int blen, unsigned char *src, int slen)
{
    int i = 0;
    int len = 0;

    unsigned int cin  = 0;
    unsigned int cout = 0;


    for (i = 0; (i <= slen) && (src[i] != 0); i++)
    {
        cin = src[i];

        if ((cin >= '0') && (cin <= '9'))
            cin = cin - '0' + 52;
        else if ((cin >= 'A') && (cin <= 'Z'))
            cin = cin - 'A';
        else if ((cin >= 'a') && (cin <= 'z'))
            cin = cin - 'a' + 26;
        else if (cin == '+')
            cin = 62;
        else if (cin == '/')
            cin = 63;
        else if (cin == '=')
            cin = 0;
        else
            continue;

        cout = (cout << 6) | cin;

        if ((i % 4) == 3)
        {
            if ((len + 3) <= blen)
            {
                buf[len++] = (char)(cout >> 16);
                buf[len++] = (char)(cout >> 8);
                buf[len++] = (char)(cout);
            }
            else
            {
                break;
            }
        }
    }

    //printk("===> len %d\n", len);
    return len;
}

uint8_t Crc8(const void *vptr, int len);
void omnicfg_cmd52_handler(u8 *ssid, u8 *sta_addr)
{
    int ocfg_sta_idx;
    unsigned int block_num;
    unsigned int total_block_num;
    unsigned int datalen;
    int i;
    struct omnicfg_cfgdata *pcfgdata;
    uint8_t data_crc;

    ocfg_sta_idx = omnicfg_add_target_sta(sta_addr);
    if (ocfg_sta_idx >= 0) 
    {
        block_num = ssid[5] - '0';
        if (block_num >= OMNICFG_CMD52_MAX_CFGDATA_BLOCK)
            goto Exit;

        if (ocfg_data[ocfg_sta_idx]==NULL)
        {
            ocfg_data[ocfg_sta_idx] = kzalloc(sizeof(struct omnicfg_cfgdata), GFP_ATOMIC);
            if (ocfg_data[ocfg_sta_idx]==NULL)
                goto Exit;
        }

        OC_DBG("OMNICFG data idx %d, block %d\n", ocfg_sta_idx, block_num);

        pcfgdata = ocfg_data[ocfg_sta_idx];
        uh_b64decode(&pcfgdata->data[OMNICFG_CMD52_CFGDATA_BLOCK_SIZE * block_num], OMNICFG_CMD52_CFGDATA_BLOCK_SIZE, &ssid[8], OMNICFG_CMD52_CFGDATA_BASE64_BLOCK_SIZE);
        pcfgdata->bitmap |= (0x01 << block_num);

        if (block_num==0) 
        {
            datalen = (((pcfgdata->data[0] + 1) + (OMNICFG_CMD52_CFGDATA_BLOCK_SIZE-1))/OMNICFG_CMD52_CFGDATA_BLOCK_SIZE) * OMNICFG_CMD52_CFGDATA_BLOCK_SIZE;
            total_block_num = datalen / OMNICFG_CMD52_CFGDATA_BLOCK_SIZE;

            OC_DBG("OMNICFG data idx %d, datalen %d, total_block_num %d\n", ocfg_sta_idx, pcfgdata->data[0], total_block_num);

            if (total_block_num > OMNICFG_CMD52_MAX_CFGDATA_BLOCK)
                goto Exit;

            pcfgdata->datalen = pcfgdata->data[0];
            pcfgdata->total_block_num = total_block_num;
        }

        if (pcfgdata->total_block_num) 
        {
            if (pcfgdata->bitmap == ((0x01 << (pcfgdata->total_block_num)) - 1))
            {
                data_crc = Crc8(pcfgdata->data, pcfgdata->datalen);
                if(pcfgdata->data[pcfgdata->datalen]==data_crc)
                {
                    printk("Pass crc check, crc = %02x\n", data_crc);

                    memcpy(cfgdata, pcfgdata->data, pcfgdata->total_block_num * OMNICFG_CMD52_CFGDATA_BLOCK_SIZE);
    
                    for(i=0;i<pcfgdata->datalen;i++)
                    {
                        if(isprint(cfgdata[i]))
                           OC_DBG("CFGDATA[%d] 0x%02x %c\n", i, cfgdata[i], cfgdata[i]);
                        else
                           OC_DBG("CFGDATA[%d] 0x%02x\n", i, cfgdata[i]);
                    }
                    goto decode_done;
                }
                else
                {
                    #if 0
                    for(i=0;i<pcfgdata->datalen + 1;i++)
                    {
                        if(isprint(pcfgdata->data[i]))
                           OC_DBG("CFGDATA[%d] 0x%02x %c\n", i, pcfgdata->data[i], pcfgdata->data[i]);
                        else
                           OC_DBG("CFGDATA[%d] 0x%02x\n", i, pcfgdata->data[i]);
                    }
                    #endif

                    printk("Wrong crc!!, correct crc = %02x, now crc = %02x\n", pcfgdata->data[pcfgdata->datalen], data_crc);
                    pcfgdata->bitmap = 0;
                    pcfgdata->datalen = 0;
                    pcfgdata->total_block_num = 0;
                }
            }
        }
    }
Exit:
    return;

decode_done:
    cfg->state = S_DONE;
    __omnicfg_filter_mode = FILTER_MODE_DROP_ALL;
    ieee80211_queue_delayed_work(g_wifi_sc->hw, &omnicfg_usrwork, 0);

    return;
}

int omnicfg_cmd52_detection(unsigned char *pkt_data, int pkt_len)
{
	struct ieee80211_mgmt *mgmt = (void *) pkt_data;
	u8 *elements;
	size_t baselen;
    u8 *ssid;

    elements = mgmt->u.probe_req.variable;
    baselen = offsetof(struct ieee80211_mgmt, u.probe_req.variable);

    ssid = omnicfg_parse_ssid(elements, pkt_len - baselen);
    if (ssid)
    {
        if(!memcmp(ssid, "OMNI0", 5) && !memcmp(&ssid[6], "52", 2))
        {
            //printk("SSID:%c%c%c%c%c%c%c%c%c%c\n",ssid[0], ssid[1], ssid[2], ssid[3], ssid[4], ssid[5], ssid[6], ssid[7], ssid[8], ssid[9]);
            omnicfg_cmd52_handler(ssid, mgmt->sa);

            return 1;
        }
    }

    return 0;
}
#endif

char *argv[5] = {NULL, NULL, NULL, NULL, NULL};
char *envp[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
static int ssid_chan = 0;
static void omnicfg_inform_user(struct work_struct *work)
{
    int data_len, mode, ssid_len, pass_len, chan;
    int err = 0;

    mutex_lock(&g_wifi_sc->mutex);

    omnicfg_lock();

    disable_sniffer_mode();

    if (envp[1])
    {
        kfree(envp[1]);
        envp[1] = NULL;
    }
    if (envp[2])
    {
        kfree(envp[2]);
        envp[2] = NULL;
    }
    if (envp[3])
    {
        kfree(envp[3]);
        envp[3] = NULL;
    }
    if (envp[4])
    {
        kfree(envp[4]);
        envp[4] = NULL;
    }

    OC_DBG("Change back to channel %d %d\n", cfg->org_channel, cfg->org_bandwidth);
    dragonite_set_channel(cfg->org_channel, cfg->org_bandwidth);

    /* inform to user-space */
    argv[0] = OMNICFG; 
    envp[0] = OMNI_PATH;

    if (cfg->state == S_DONE)
    {
        data_len = cfgdata[0];
        mode = cfgdata[1];
        ssid_len = cfgdata[2];
        if(data_len > (ssid_len + 3))
            pass_len = cfgdata[3 + ssid_len];
        else
            pass_len = 0;

        if(data_len > (ssid_len + 3 + pass_len + 1))
            chan = cfgdata[4 + ssid_len + pass_len];
        else
            chan = ssid_chan;

        if(ssid_len >= 0x7e)
            ssid_len = 0x7e;

        if(pass_len >= 0x7e)
            pass_len = 0;

        cfgdata[3 + ssid_len] = 0;
        cfgdata[4 + ssid_len + pass_len] = 0;

        argv[1] = OMNIDONE; 

        envp[1] = kmalloc((sizeof("CH=") + 2), GFP_ATOMIC);
        if(envp[1]==NULL)
            err = 1;
        else
            snprintf(envp[1],(sizeof("CH=") + 2), "CH=%d", chan);

        OC_DBG("envp[1] %s\n", envp[1]);

        envp[2] = kmalloc((sizeof("MODE=") + 2), GFP_ATOMIC);
        if(envp[2]==NULL)
            err = 1;
        else
            //sprintf(envp[2], "MODE=%d", mode);
            snprintf(envp[2],(sizeof("MODE=") + 2), "MODE=%d", mode);

        OC_DBG("envp[2] %s\n", envp[2]);

        envp[3] = kmalloc((sizeof("SSID=")+ssid_len+1), GFP_ATOMIC);
        if(envp[3]==NULL)
            err = 1;
        else
            //sprintf(envp[3], "SSID=%s", &cfgdata[3]);
            snprintf(envp[3],(sizeof("SSID=")+ssid_len+1), "SSID=%s", &cfgdata[3]);

        OC_DBG("envp[3] %s\n", envp[3]);

        if(pass_len)
        {
            envp[4] = kmalloc((sizeof("PASS=")+pass_len+1), GFP_ATOMIC);
            if(envp[4]==NULL)
                err = 1;
            else
                //sprintf(envp[4], "PASS=%s", &cfgdata[4+ssid_len]);
                snprintf(envp[4],(sizeof("PASS=")+pass_len+1), "PASS=%s", &cfgdata[4+ssid_len]);

            OC_DBG("envp[4] %s\n", envp[4]);
        }
    }
    else if (cfg->state == S_USRSTOP)
    {
        argv[1] = OMNISTOP;
        envp[1] = NULL;
    }

    if(cfgdata)
    {
        kfree(cfgdata);
        cfgdata = NULL;
    }

    if (blocks)
    {
        kfree(blocks);
        blocks=NULL;
    }

    if (lock_data)
    {
        free_lock_data(lock_data);
        kfree(lock_data);
        lock_data=NULL;
    }

    drg_wifi_sniffer = 0;

    __omnicfg_enable = 0;

    omnicfg_unlock();

    mutex_unlock(&g_wifi_sc->mutex);

    if(!err)
        call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);

    return;
}
extern u32 drg_wifi_recover;
void omnicfg_start(void)
{
#if defined(HANDLE_ANDROID_PROBE_REQUEST)
    int i;
#endif
    int t_wait = 0;

    mutex_lock(&g_wifi_sc->mutex);

    if(!global_data->started)
    {
        OC_DBG("Omnicfg: WLAN driver not started, ignored\n");
        goto exit;
    }

    if(__omnicfg_enable)
    {
        OC_DBG("Omnicfg: already started, ignored\n");
        goto exit;
    }
    else
    {
        __omnicfg_enable = 1;
    }

    drg_wifi_sniffer = 1;

    /* before dragonite wifi recover, need to unlock mutex */
    drg_wifi_recover = 1;

    mutex_unlock(&g_wifi_sc->mutex);

    while(drg_wifi_recover)
    {
        OC_DBG("Omnicfg: prepare for sniffer mode, please wait\n");
        if(t_wait > 5)
        {
            mutex_lock(&g_wifi_sc->mutex);
            goto error;
        }
        schedule_timeout_interruptible(HZ/5);
        t_wait++;
    }

    mutex_lock(&g_wifi_sc->mutex);

    omnicfg_lock();

    __omnicfg_filter_mode = FILTER_MODE_DROP_ALL;

    cfgdata = kzalloc(sizeof(u8) * (MAX_CFGDATA_SIZE + 1), GFP_ATOMIC);
    if(cfgdata==NULL)
    {
        OC_DBG("Omnicfg: cfgdata malloc failure\n");
        omnicfg_unlock();
        goto error;
    }

    OC_DBG("OmniConfig start\n");

    ssid_chan = 0;
    dragonite_get_channel(&cfg->org_channel, &cfg->org_bandwidth);
    cfg->cur_chan = cfg->org_channel;

    memset(cfgdata_valid, 0, sizeof(cfgdata_valid));

    sniffer_rx_count = 0;
    last_sniffer_rx_count = 0;

#ifdef PLAY_OMNI_VOICE
    // initial voice type with none
    __omnicfg_voice_type = S_VOICE_NONE;
#endif

    //cancel_delayed_work(&omnicfg_work);
    memset(cfg, 0, sizeof(struct omnicfg));
    memset(siso2_last_mac_addr_ipv4, 0, ETH_ALEN);
    memset(siso2_last_mac_addr_ipv6, 0, ETH_ALEN);
    memset(siso2_mac_addr_offset_ipv4, 0, ETH_ALEN);
    memset(siso2_mac_addr_offset_ipv6, 0, ETH_ALEN);

#if defined(HANDLE_ANDROID_PROBE_REQUEST)
    for (i=0; i < MAX_OMNICFG_CANDIDATE_STA; i++)
    {
        memset(omnicfg_candidate_sta[i], 0, ETH_ALEN);
        if (ocfg_data[i]) 
        {
            kfree(ocfg_data[i]);
            ocfg_data[i] = NULL;
        }
    }
#endif
    cfg->num_of_channel = 11;
    if(cfg80211_regdomain)
    {
        OC_DBG("Regulatory domain %c%c\n", cfg80211_regdomain->alpha2[0], cfg80211_regdomain->alpha2[1]);
        if((cfg80211_regdomain->alpha2[0]=='C')&&(cfg80211_regdomain->alpha2[1]=='N'))
        {
            OC_DBG("13 channels\n");
            cfg->num_of_channel = 13;
        }
    }
 
    cfg->total = jiffies;

    next_rx_count_check_time = jiffies + RX_COUNT_CHECK_INTERVAL;

    enable_sniffer_mode();

    __omnicfg_filter_mode = (FILTER_SISO_SCAN | FILTER_MIMO_SCAN);

    ieee80211_queue_delayed_work(g_wifi_sc->hw, &omnicfg_work, 0);

    omnicfg_unlock();

    goto exit;

error:
    printk(KERN_EMERG "Omnicfg: Unexpect error, escape sniffer mode !!\n");

    drg_wifi_sniffer = 0;

    __omnicfg_enable = 0;

exit:
    mutex_unlock(&g_wifi_sc->mutex);

    return;
}

static int siso2_data_decode(unsigned char *mac, struct omnicfg *cfg, unsigned int len);
static int siso2_omnicfg_decode(char *data, struct omnicfg *cfg,  unsigned int len)
{
    struct ieee80211_hdr_3addr *hdr = (struct ieee80211_hdr_3addr *)data;
    __le16 fc;
    int ret = DECODE_IN_PROC;

    fc = hdr->frame_control;
    if (ieee80211_is_data(fc))
    {   
#if defined(TEST_SISO2_STA) // smrtcfg test sta send siso2   
        if (is_multicast_ether_addr(hdr->addr3) && (!memcmp(hdr->addr2, cfg->ta, 6)))
        {
            if(ieee80211_is_data_qos(fc))
                ret = siso2_data_decode(hdr->addr3, cfg, len);
            else
                ret = siso2_data_decode(hdr->addr3, cfg, len+2);
        }
#endif

#if defined(TEST_SISO2_AP) // smrtcfg test ap forward siso2
        if (is_multicast_ether_addr(hdr->addr1) && (!memcmp(hdr->addr3, cfg->ta, 6)))
        {
            if(ieee80211_is_data_qos(fc))
                ret = siso2_data_decode(hdr->addr1, cfg, len);
            else
                ret = siso2_data_decode(hdr->addr1, cfg, len+2);
        }
#endif
    }

    return ret;
}

static int siso2_chan_chkaddr(char *mac, struct omnicfg *cfg, unsigned int len)
{
    int ret = SIGNATURE_NOT_FOUND;

    if(is_cfgdata_addr_ipv4(mac))
    {
        if((!is_macaddr_all_zero(siso2_last_mac_addr_ipv4)) &&
           ((len != siso2_last_mac_length_ipv4 && memcmp(siso2_last_mac_addr_ipv4, mac, 6))&&
            (len - siso2_last_mac_length_ipv4 == mac[3]-siso2_last_mac_addr_ipv4[3])))
        {
            if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
            {
                printk("match mac, mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
            }

            memcpy(siso2_mac_addr_offset_ipv4, mac, ETH_ALEN);
            siso2_mac_length_offset_ipv4 = len;
        }
        else
        {
            if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
            {
                printk("not match mac, read mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
            }

            memcpy(siso2_last_mac_addr_ipv4, mac, ETH_ALEN);
            siso2_last_mac_length_ipv4 = len;
            goto done;
        }
        
        OC_DBG("Signature matched\n");
        ret = SIGNATURE_LOCKED;
        goto done;   
    }
    else if(is_cfgdata_addr_ipv6(mac))
    {
        if((!is_macaddr_all_zero(siso2_last_mac_addr_ipv6)) &&
           ((len != siso2_last_mac_length_ipv6 && memcmp(siso2_last_mac_addr_ipv6, mac, 6))&&
            (len - siso2_last_mac_length_ipv6 == mac[3]-siso2_last_mac_addr_ipv6[3])))
        {
            if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
            {
                printk("match mac, mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
            }

            memcpy(siso2_mac_addr_offset_ipv6, mac, ETH_ALEN);
            siso2_mac_length_offset_ipv6 = len;
        }
        else
        {
            if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
            {
                printk("not match mac, read mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
            }

            memcpy(siso2_last_mac_addr_ipv6, mac, ETH_ALEN);
            siso2_last_mac_length_ipv6 = len;
            goto done;
        }
        
        OC_DBG("Signature matched\n");
        ret = SIGNATURE_LOCKED;
        goto done;   
    }
           
    ret = SIGNATURE_NOT_FOUND;

done:   
    return ret;
}

uint8_t Crc8(const void *vptr, int len)
{
	const uint8_t *data = vptr;
	unsigned crc = 0;
	int i, j;
	for (j = len; j; j--, data++) {
		crc ^= (*data << 8);
		for(i = 8; i; i--) {
			if (crc & 0x8000)
				crc ^= (0x1070 << 3);
			crc <<= 1;
		}
	}
	return (uint8_t)(crc >> 8);
}

static int siso2_data_decode(unsigned char *mac, struct omnicfg *cfg, unsigned int len)
{
    unsigned char idx;
    u8 byte1, byte2;
    int ret = DECODE_IN_PROC;

    if (is_cfgdata_addr_ipv4(mac) || is_cfgdata_addr_ipv6(mac))
    {
        //DO LENGTH AND MAC[3] CHECK
        if(is_cfgdata_addr_ipv4(mac))
        {
            if(is_macaddr_all_zero(siso2_mac_addr_offset_ipv4))
            {
                //COMPARE AND STORE OFFSET
                if((!is_macaddr_all_zero(siso2_last_mac_addr_ipv4)) &&
                   ((len != siso2_last_mac_length_ipv4 && memcmp(siso2_last_mac_addr_ipv4, mac, 6))&&
                    (len - siso2_last_mac_length_ipv4 == mac[3]-siso2_last_mac_addr_ipv4[3])))
                {
                    if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
                    {
                        printk("match mac, mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
                    }

                    memcpy(siso2_mac_addr_offset_ipv4, mac, ETH_ALEN);
                    siso2_mac_length_offset_ipv4 = len;
                    goto Exit;
                }
                else
                {
                    if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
                    {
                        printk("not match mac, read mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
                    }

                    memcpy(siso2_last_mac_addr_ipv4, mac, ETH_ALEN);
                    siso2_last_mac_length_ipv4 = len;
                    goto Exit;
                }               
            }
            else if(len - siso2_mac_length_offset_ipv4 != mac[3] - siso2_mac_addr_offset_ipv4[3])
            {
                goto Exit;
            }
        }
        else if(is_cfgdata_addr_ipv6(mac))
        {
            if(is_macaddr_all_zero(siso2_mac_addr_offset_ipv6))
            {
                if((!is_macaddr_all_zero(siso2_last_mac_addr_ipv6)) &&
                   ((len != siso2_last_mac_length_ipv6 && memcmp(siso2_last_mac_addr_ipv6, mac, 6))&&
                    (len - siso2_last_mac_length_ipv6 == mac[3]-siso2_last_mac_addr_ipv6[3])))
                {
                    if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
                    {
                        printk("match mac, mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
                    }

                    memcpy(siso2_mac_addr_offset_ipv6, mac, ETH_ALEN);
                    siso2_mac_length_offset_ipv6 = len;
                    goto Exit;
                }
                else
                {
                    if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
                    {
                        printk("not match mac, read mac = %02x %02x %02x %02x %02x %02x, len = %d\n", 
                               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
                    }

                    memcpy(siso2_last_mac_addr_ipv6, mac, ETH_ALEN);
                    siso2_last_mac_length_ipv6 = len;
                    goto Exit;
                }   
            }
            else if(len - siso2_mac_length_offset_ipv6 != mac[3] - siso2_mac_addr_offset_ipv6[3])
            {
                goto Exit;
            }
        }
        idx = mac[3];
        if(idx>60)
        {
            goto Exit;
        }
        byte1 = (u8) mac[4];
        byte2 = (u8) mac[5];

        cfg->valid_pkt_count++;

        if(test_bit(idx, cfgdata_valid))
        {
            if(cfgdata[idx*2]!=byte1)
            {
                OC_DBG("Inconsistent Data[%d]=%02x %02x, reset decoded data\n", idx, (u8) cfgdata[idx*2], (u8) byte1);

                memset(cfgdata_valid, 0, sizeof(cfgdata_valid));
                cfg->datacnt = 0;
                cfg->datalen = 0;
            }
            else if(cfgdata[(idx*2)+1]!=byte2)
            {
                OC_DBG("Inconsistent Data[%d]=%02x %02x, reset decoded data\n", idx, (u8) cfgdata[(idx*2)+1], (u8) byte2);

                memset(cfgdata_valid, 0, sizeof(cfgdata_valid));
                cfg->datacnt = 0;
                cfg->datalen = 0;
            }
        }
        else
        {
            set_bit(idx, cfgdata_valid);

            cfgdata[idx*2] = byte1;
            cfgdata[(idx*2)+1] = byte2;
            cfg->datacnt++;

            if(idx==0)
            {
                cfg->datalen = byte1;                
                OC_DBG("Config data length = %d\n", cfg->datalen);
            }
        }      
    }

    if (cfg->datalen && (cfg->datacnt == ((cfg->datalen+2)/2)))
    {
        int i;
        uint8_t data_crc;

        cfg->total = (long)jiffies - (long)cfg->total;

        data_crc = Crc8(cfgdata, cfg->datalen);

        if(cfgdata[cfg->datalen]==data_crc)
        {
            printk("Pass crc check, crc = %02x\n", data_crc);
        }
        else
        {
            printk("Wrong crc!!, correct crc = %02x, now crc = %02x\n", cfgdata[cfg->datalen], data_crc);
            //ReDecoding until pass crc check;
            for(i=0;i<cfg->datalen;i++)
            {
                if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
                {    
                    printk(KERN_INFO "%02d:%02x %c\n", i, cfgdata[i], cfgdata[i]);
                }
            }
            memset(cfgdata, 0, cfg->datacnt*2);
            memset(cfgdata_valid, 0, sizeof(cfgdata_valid));
            cfg->datacnt = 0;
            cfg->datalen = 0;

            goto Exit;
        }
        for(i=0;i<cfg->datalen;i++)
        {
            if(isprint(cfgdata[i]))
               OC_DBG("CFGDATA[%d] 0x%02x %c\n", i, cfgdata[i], cfgdata[i]);
            else
               OC_DBG("CFGDATA[%d] 0x%02x\n", i, cfgdata[i]);
        }

        ret = DECODE_DONE;
    }
Exit:
    return ret;
}

static int siso2_signature_search(char *data, struct omnicfg *cfg,  unsigned int len)
{
    struct ieee80211_hdr_3addr *hdr = (struct ieee80211_hdr_3addr *)data;
    __le16 fc;
    int ret = SIGNATURE_NOT_FOUND;

    fc = hdr->frame_control;

    /*We can check pkt len size*/

    if (!ieee80211_is_data(fc))
        return SIGNATURE_NOT_FOUND;

#if defined(TEST_SISO2_STA) // smrtcfg test sta send siso2
    /*should only handle multicast addr*/
    if (is_multicast_ether_addr(hdr->addr3))
    {
        if(len>=SISO2_SAFE_BASE && len<SISO2_SAFE_BASE+SISO2_SAFE_RANGE)
        {
            if(ieee80211_is_data_qos(fc))
                ret = siso2_chan_chkaddr(hdr->addr3, cfg, len);
            else
                ret = siso2_chan_chkaddr(hdr->addr3, cfg, len+2);
        }

        if(ret == SIGNATURE_LOCKED)
        {
            memcpy(cfg->ta, hdr->addr2, ETH_ALEN);
            goto locked;
        }
    }
#endif

#if defined(TEST_SISO2_AP) // smrtcfg test ap forward siso2
    if (is_multicast_ether_addr(hdr->addr1))
    {
        if(len>=SISO2_SAFE_BASE && len<SISO2_SAFE_BASE+SISO2_SAFE_RANGE)
        {
            if(ieee80211_is_data_qos(fc))
                ret = siso2_chan_chkaddr(hdr->addr1, cfg, len);
            else
                ret = siso2_chan_chkaddr(hdr->addr1, cfg, len+2);
        }

        if(ret == SIGNATURE_LOCKED)
        {
            memcpy(cfg->ta, hdr->addr3, ETH_ALEN);
            goto locked;
        }
    }
#endif

    return ret;

locked:
    cfg->chantime = (long)jiffies - (long)cfg->total;

    if(ieee80211_is_data_qos(fc))
        cfg->offset = len - OMNICFG_BASELEN;
    else
        cfg->offset = len - OMNICFG_BASELEN + 2;

    OC_DBG("Locked STA %pM, channel:%d base_len:%d offset:%d\n",cfg->ta, cfg->cur_chan, len, cfg->offset);

    return ret;
}

void omnicfg_stop(void)
{
    mutex_lock(&g_wifi_sc->mutex);

    if(!global_data->started)
    {
        OC_DBG("Omnicfg: WLAN driver not started, ignored\n");
        goto exit;
    }

    if(0==__omnicfg_enable)
    {
        OC_DBG("Omnicfg: already stopped, ignored\n");
        goto exit;
    }
		drg_wifi_recover = 1;
    cancel_delayed_work_sync(&omnicfg_work);

    omnicfg_lock();

    __omnicfg_filter_mode = FILTER_MODE_DROP_ALL;

    cfg->state = S_USRSTOP;
    
    ieee80211_queue_delayed_work(g_wifi_sc->hw, &omnicfg_usrwork, 0);

    OC_DBG("OmniConfig stopped\n");

    omnicfg_unlock();
exit:
    mutex_unlock(&g_wifi_sc->mutex);
}

#if defined(CHANNEL_HOPPING)
const u8 scan_channel_list_b[] = { 1, 6, 11, 2, 5, 8, 3, 9, 4, 7, 10 };
const u8 scan_channel_list_d[] = { 1, 6, 11, 2, 13, 5, 8, 3, 9, 4, 12, 7, 10 };
const u8 *scan_channel_list;
u8 num_of_channel;
#endif

int has_two_secondary_channels(int channel)
{
    if(num_of_channel==13)
        return ((channel >= 5) && (channel <= 9));
    else
        return ((channel >= 5) && (channel <= 7));
}
char *argv_sec[5] = {NULL, NULL, NULL, NULL, NULL};
char *envp_sec[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
static void omnicfg_change_channel(struct work_struct *work)
{
    static u8 stay_channel_state_count = 0;
    static u8 channel_below = 0;

    // Timeout concern
    if(cfg->num_of_channel==13)
    {
        num_of_channel = 13;
        scan_channel_list = scan_channel_list_d;
    }
    else
    {
        num_of_channel = 11;
        scan_channel_list = scan_channel_list_b;
    }

    if(time_after(jiffies, next_rx_count_check_time))
    {
        if(sniffer_rx_count == last_sniffer_rx_count)
        {
            if(__omnicfg_enable)
            {
                OC_DBG("Trigger MAC recovery @ %ld\n", jiffies);
                drg_wifi_recover = 1;
            }
        }

        last_sniffer_rx_count = sniffer_rx_count;
        next_rx_count_check_time = jiffies + RX_COUNT_CHECK_INTERVAL;
    }

    omnicfg_lock();

#if defined(CHANNEL_BLACKLIST)
Restart:
#endif

    if (cfg->state == S_SCAN_CHANNEL) //In this state we should change channel step by step
    {
        stay_channel_state_count = 0;
        if (!cfg->swupdwn)
        {
            if (cfg->fixchan == 1) // chan 1-4
            {
               u8 cur_chan=cfg->cur_chan+1;
               cfg->cur_chan=cfg->cur_chan>=4?1:cur_chan;
            }
            else if (cfg->fixchan == 2) // chan 5-8
            {
				u8 cur_chan=cfg->cur_chan+1;
                cfg->cur_chan=cfg->cur_chan>=8?5:cur_chan;
            }
            else if (cfg->fixchan == 3) // chan 9-11
            {
                u8 cur_chan=cfg->cur_chan+1;
                cfg->cur_chan=cfg->cur_chan>=num_of_channel?9:cur_chan;
            }
            else // all chan
            {
#if defined(CHANNEL_HOPPING)
                if(cfg->channel_index >= num_of_channel)
                    cfg->channel_index = 0;

                cfg->cur_chan = scan_channel_list[cfg->channel_index];

                cfg->channel_index++;
                if(cfg->channel_index >= num_of_channel)
                    cfg->channel_index = 0;

#else
                cfg->cur_chan=cfg->cur_chan>=num_of_channel?1:++cfg->cur_chan;
#endif

#if defined(CHANNEL_BLACKLIST)
                if(cfg->black_list_count && (cfg->black_list_channel_num==cfg->cur_chan))
                {
                    cfg->black_list_count--;
                    OC_DBG("Skip channel %d\n", cfg->cur_chan);
                    goto Restart;
                }
#endif
            }

#if defined(ENABLE_40MHZ_CHANNEL)
            if (has_two_secondary_channels(cfg->cur_chan))
                cfg->swupdwn = 2;
            else
                cfg->swupdwn = 0;
#else
            cfg->swupdwn = 0;
#endif
        }

        if ((cfg->swupdwn) && has_two_secondary_channels(cfg->cur_chan))
        {
            if(((cfg->swupdwn==2) && (cfg->channel_below_first))
                ||((cfg->swupdwn==1) && (!cfg->channel_below_first)))
            {
                dragonite_set_channel(cfg->cur_chan, BW40MHZ_SCB);
                OC_DBG("Chan:%d-\n",cfg->cur_chan);
                channel_below = 1;
            }
            else
            {
                dragonite_set_channel(cfg->cur_chan, BW40MHZ_SCA);
                OC_DBG("Chan:%d+\n",cfg->cur_chan);
                channel_below = 0;
            }
            cfg->swupdwn--;
        }
        else
        {
#if defined(ENABLE_40MHZ_CHANNEL)
            if (cfg->cur_chan < 5)
            {    
                dragonite_set_channel(cfg->cur_chan, BW40MHZ_SCA);
                OC_DBG("Chan:%d+\n",cfg->cur_chan);
            }
            else
            {    
                dragonite_set_channel(cfg->cur_chan, BW40MHZ_SCB);
                OC_DBG("Chan:%d-\n",cfg->cur_chan);
            }
#else
            dragonite_set_channel(cfg->cur_chan, BW40MHZ_SCN);
            OC_DBG("Chan:%d\n",cfg->cur_chan);
#endif
        }

        goto timer;
    }
    else if (cfg->state == S_STAY_CHANNEL) //In this state we just remain on chan 
    {
        cfg->macmatch = 0;
        stay_channel_state_count++;
        if(stay_channel_state_count>4)
        {
            OC_DBG("Lock attempt failed\n");
            cfg->swupdwn = 0;
            cfg->state = S_SCAN_CHANNEL;
            __omnicfg_filter_mode = (FILTER_SISO_SCAN | FILTER_MIMO_SCAN);

#if defined(CHANNEL_BLACKLIST)
            cfg->black_list_count = 1;
            cfg->black_list_channel_num = cfg->cur_chan;
#endif

            goto Restart;
        }        
        goto timer;
    }
    else if (cfg->state == S_DECODE)
    {
#if 1
        if(time_after(jiffies, cfg->next_decode_status_check_time))
        {
            if(cfg->signature_count==cfg->signature_count_prev)
            {
                OC_DBG("Unlock the STA and try again\n");

                cfg->swupdwn = 0;
                cfg->macmatch = 0;

                memset(siso2_mac_addr_offset_ipv4, 0, ETH_ALEN);
                memset(siso2_mac_addr_offset_ipv6, 0, ETH_ALEN);
                memset(cfgdata, 0, cfg->datacnt*2);
                memset(cfgdata_valid, 0, sizeof(cfgdata_valid));
                cfg->datacnt = 0;
                cfg->datalen = 0;

                /* swap the above/below change order */
                cfg->channel_below_first = (cfg->channel_below_first) ? 0 : 1;

                cfg->state = S_SCAN_CHANNEL;
                __omnicfg_filter_mode = (FILTER_SISO_SCAN | FILTER_MIMO_SCAN);

#if defined(CHANNEL_BLACKLIST)
                cfg->black_list_count = 1;
                cfg->black_list_channel_num = cfg->cur_chan;
#endif
            }
#if defined(ENABLE_40MHZ_CHANNEL)
            else if (has_two_secondary_channels(cfg->cur_chan))
            {
                if(cfg->valid_pkt_count==cfg->valid_pkt_count_prev)
                {
                    if(channel_below)
                    {
                        dragonite_set_channel(cfg->cur_chan, BW40MHZ_SCA);
                        OC_DBG("Chan:%d+\n",cfg->cur_chan);
                        channel_below = 0;
                    }
                    else
                    {
                        dragonite_set_channel(cfg->cur_chan, BW40MHZ_SCB);
                        OC_DBG("Chan:%d-\n",cfg->cur_chan);
                        channel_below = 1;
                    }
                }
            }
#endif

            cfg->signature_count_prev = cfg->signature_count;
            cfg->valid_pkt_count_prev = cfg->valid_pkt_count;
            cfg->next_decode_status_check_time = jiffies + OMNICFG_DECODE_STATUS_CHECK_INTERVAL;
        }
        goto timer;
#endif
    }
    omnicfg_unlock();
    return;

timer:
    //cancel_delayed_work(&omnicfg_work); /* XXX */
    ieee80211_queue_delayed_work(g_wifi_sc->hw, &omnicfg_work, CHGCHANDELAY);

    omnicfg_unlock();

#ifdef PLAY_OMNI_VOICE
    if (__omnicfg_voice_type == S_VOICE_CFG)
    {
        __omnicfg_voice_type = S_VOICE_NONE;

        /* inform to user-space */
        argv_sec[0] = OMNICFG;
        argv_sec[1] = OMNIVOICE;
        argv_sec[2] = "cfg";
        envp_sec[0] = OMNI_PATH;

        call_usermodehelper(argv_sec[0], argv_sec, envp_sec, UMH_WAIT_EXEC);
    }
#endif
}

static u32 length_data=0;
static int last_frame_tid;
#define FRAME_TYPE_DONT_CARE        0
#define FRAME_TYPE_UNSUPPORT_FORMAT 1
#define FRAME_TYPE_DATA_MPDU        2
static int last_frame_type = FRAME_TYPE_DONT_CARE;

#define TYPE_NONUF 0
#define TYPE_UF 1
#define TYPE_ACK 2
#define TYPE_BA 3

#define DECODE_DATA_LENGTH_H    (OMNICFG_CONFIG_LENGTH_BASE + (0x3f<<OMNICFG_CONFIG_SHIFT))
#define SAFE_LENGTH             100
#define MIMO_DECODE_PHASE_TID   0x5
#define MIMO_LOCK_PHASE_TID     0x1
#define AGGREGATE_PADDING       4

int omnicfg_dispatcher(u8 *ptr, unsigned int len, u8 *mac_addr, u8 *ap_addr, u32 flags);
int omnicfg_filter(u8 *ptr, unsigned int len, int frame_type)
{
    struct ieee80211_ctrl_ba *ba_head = (struct ieee80211_ctrl_ba *) ptr;
    struct ieee80211_hdr *wifi_hdr = (struct ieee80211_hdr *) ptr;
    int next_frame_type;

    next_frame_type = FRAME_TYPE_DONT_CARE;
#if defined(MIMO_DEBUG)
    printk("len: %d, frame_type: %d, ptr[0]= %x\n", len, frame_type, ptr[0]);
    printk("frame_control %x, ba_ctl %04x, start_seq_num %4x\n"
           , ba_head->frame_control, ba_head->ba_ctl, ba_head->start_seq_num);
#endif
    if ((__omnicfg_filter_mode & (FILTER_MIMO_SCAN|FILTER_MIMO_DECODE)) && (frame_type==TYPE_UF))
    {

        if ((__omnicfg_filter_mode&FILTER_MIMO_DECODE)
            &&((SC_LOCK_LENGTH_BASE > len)
               ||((DECODE_DATA_LENGTH_H + SAFE_LENGTH + header_len_diff) < len)))
        {
            return 0;
        }
        else if(__omnicfg_filter_mode&FILTER_MIMO_DECODE)
        {
            length_data=((len+4)/4)*4;
            if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
            {
                printk(KERN_INFO "UF2 %d\n", length_data);
            }
            next_frame_type = FRAME_TYPE_UNSUPPORT_FORMAT;
        }
        else if ((__omnicfg_filter_mode&FILTER_MIMO_SCAN)
            &&((SC_LOCK_LENGTH_BASE > len)
               ||((OMNICFG_CONFIG_LENGTH_BASE + SAFE_LENGTH) < len)))
        {
            return 0;
        }
        else if(__omnicfg_filter_mode&FILTER_MIMO_SCAN)
        {
            length_data=((len+4)/4)*4;
            if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
            {
                printk(KERN_INFO "UF1 %d\n", length_data);
            }
            next_frame_type = FRAME_TYPE_UNSUPPORT_FORMAT;
        }

    }
    else if ((__omnicfg_filter_mode & (FILTER_MIMO_SCAN|FILTER_MIMO_DECODE))
             &&(frame_type==TYPE_BA))
    {
        u16 cur_tid;
        cur_tid= le16_to_cpu(ba_head->ba_ctrl) >> 12;
        if ((__omnicfg_filter_mode&FILTER_MIMO_DECODE)
            &&((memcmp(ba_head->ra, lock_mac_addr, WLAN_ADDR_LEN))
               ||((MIMO_DECODE_PHASE_TID!=cur_tid)
                  &&(MIMO_LOCK_PHASE_TID!=cur_tid))))
        {
            return 0;
        }
        if((__omnicfg_filter_mode&FILTER_MIMO_DECODE) && cur_tid==5)
        {
        }
        else if ((__omnicfg_filter_mode&FILTER_MIMO_SCAN)
            &&(MIMO_LOCK_PHASE_TID!=cur_tid))
        {
            return 0;
        }
        
#if defined(MIMO_DEBUG)
        printk("state: %d, tid=%d, BA\n", __omnicfg_filter_mode, cur_tid);
#endif
        if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
        {
            printk(KERN_INFO "state: %d, tid=%d, BA\n", __omnicfg_filter_mode, cur_tid);
        }
        if (MIMO_LOCK_PHASE_TID==cur_tid)
        {
            if (last_frame_type!=FRAME_TYPE_DONT_CARE)
            {
                if ((SC_LOCK_LENGTH_BASE <= length_data)
                    &&((OMNICFG_CONFIG_LENGTH_BASE + SAFE_LENGTH) >= length_data))
                {    
    
                    if(last_frame_type==FRAME_TYPE_DATA_MPDU)
                    {                    
                        omnicfg_dispatcher(ptr,length_data+AGGREGATE_PADDING,ba_head->ra, ba_head->ta, DISPATCH_MIMO_SCAN);
                    }
                    else
                    {
                        omnicfg_dispatcher(ptr,length_data,ba_head->ra, ba_head->ta, DISPATCH_MIMO_SCAN);
                    }
                }
            }
        }
        else
        {
            if (last_frame_type!=FRAME_TYPE_DONT_CARE)
            {           
                if(last_frame_type==FRAME_TYPE_DATA_MPDU)
                {
                    omnicfg_dispatcher(ptr,length_data+AGGREGATE_PADDING,ba_head->ra, ba_head->ta, DISPATCH_MIMO_DECODE);
                }
                else
                {  
                    omnicfg_dispatcher(ptr,length_data,ba_head->ra, ba_head->ta, DISPATCH_MIMO_DECODE);
                }           
            }
        }
    }
    else if ((last_frame_type!=FRAME_TYPE_DONT_CARE)
             &&(__omnicfg_filter_mode & (FILTER_MIMO_SCAN|FILTER_MIMO_DECODE))
             &&(frame_type==TYPE_ACK))
    {

#if defined(MIMO_DEBUG)
        printk(KERN_DEBUG "ACK %d\n", length_data);
#endif

        if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
        {
            printk(KERN_INFO "ACK %d\n", length_data);
        }
        if (__omnicfg_filter_mode&FILTER_MIMO_DECODE)
        {
            if(FRAME_TYPE_DATA_MPDU==last_frame_type)
            {
                if(last_frame_tid==MIMO_LOCK_PHASE_TID)
                    omnicfg_dispatcher(ptr,(length_data+AGGREGATE_PADDING),ba_head->ra, NULL, DISPATCH_MIMO_SCAN);
                else
                    omnicfg_dispatcher(ptr,(length_data+AGGREGATE_PADDING),ba_head->ra, NULL, DISPATCH_MIMO_DECODE);
            }
            else
            {
                if ((SC_LOCK_LENGTH_BASE <= length_data)
                    &&((OMNICFG_CONFIG_LENGTH_BASE + header_len_diff - AGGREGATE_PADDING) > length_data))
                    omnicfg_dispatcher(ptr,(length_data+AGGREGATE_PADDING),ba_head->ra, NULL, DISPATCH_MIMO_SCAN);
                else 
                    omnicfg_dispatcher(ptr,(length_data+AGGREGATE_PADDING),ba_head->ra, NULL, DISPATCH_MIMO_DECODE);
            }
        }
        else if (__omnicfg_filter_mode&FILTER_MIMO_SCAN)
        {
            if(FRAME_TYPE_DATA_MPDU==last_frame_type)
            {
                if(last_frame_tid==MIMO_LOCK_PHASE_TID)
                    omnicfg_dispatcher(ptr,(length_data+AGGREGATE_PADDING),ba_head->ra, NULL, DISPATCH_MIMO_SCAN);
            }
            else
            {    
                if ((SC_LOCK_LENGTH_BASE <= length_data)
                   &&(OMNICFG_CONFIG_LENGTH_BASE + SAFE_LENGTH >= length_data))
                    omnicfg_dispatcher(ptr,(length_data+AGGREGATE_PADDING),ba_head->ra, NULL, DISPATCH_MIMO_SCAN);
            }
        }
    }
    else if ((__omnicfg_filter_mode & (FILTER_SISO_SCAN|FILTER_MIMO_SCAN|FILTER_SISO_DECODE|FILTER_MIMO_DECODE))
        && (frame_type==TYPE_NONUF))
    {
        if(ieee80211_is_data_qos(wifi_hdr->frame_control))
	    {
            u8 *qc = ieee80211_get_qos_ctl(wifi_hdr);
            int tid = *qc & IEEE80211_QOS_CTL_TID_MASK;
            if(!(*qc & IEEE80211_QOS_CTL_A_MSDU_PRESENT))
            {
                if((tid==MIMO_LOCK_PHASE_TID)||(tid==MIMO_DECODE_PHASE_TID))
                {
                    length_data=((len+4)/4)*4;
                    last_frame_tid = tid;
                    next_frame_type = FRAME_TYPE_DATA_MPDU;
                }
            }
            if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
            {
                if(length_data>1000)
                {    
                    printk(KERN_INFO "\t\t\t\t\t\t\t\t\t\t\t\tnonUF %d\n", length_data);
                }
                else
                    printk(KERN_INFO "nonUF %d\n", length_data);
            }
        }

        /* for SISO decode + scan */
        omnicfg_dispatcher(ptr, len, NULL, NULL, DISPATCH_SISO_SCAN|DISPATCH_SISO_DECODE);
    }

    last_frame_type = next_frame_type;

    return 0;
}

int mimo_omnicfg_decode(unsigned int len, struct sc_rx_block *blocks)
{
    unsigned char *ret=NULL;
    unsigned int data=0;

    if(header_len_diff > len)
        return DECODE_IN_PROC;        

    data = len - header_len_diff;

#if defined(MIMO_DEBUG)
    printk("decode data\n", data);
#endif
    ret = mimo_decoder(data, blocks);
    if (ret!=NULL)
    {
#if defined(MIMO_DEBUG)
        printk("MIMO Decode Success\n");
#endif
        memcpy(cfgdata,ret,ret[0]+1);
        return DECODE_DONE;
        //cfgdata=ret;
        //return DECODE_DONE
    }
    return DECODE_IN_PROC;
}

int mimo_signature_search(unsigned int len, char *mac_addr, struct try_lock_data *lock_data)
{
    int ret_state=0;
    header_len_diff=0;

    if(omnicfg_debug_flag & OMNICFG_DEBUG_FLAG_MIMO)
    {
        printk(KERN_INFO "MSIG %d, %02x:%02x:%02x:%02x:%02x:%02x\n", len,
                mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3],mac_addr[4],mac_addr[5]);
    }

    header_len_diff = try_lock_channel(len, mac_addr, lock_data, &ret_state);
    if (header_len_diff > 0)
    {
#if defined(MIMO_DEBUG)
    printk("length diff base=%d\n", header_len_diff);
#endif
    if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
    {
        printk(KERN_INFO "length diff base=%d\n", header_len_diff);
    }
        return SIGNATURE_LOCKED;
    }
    else if(ret_state==STAY_IN_THE_CHANNEL)
    {
        return STAY_IN_THE_CHANNEL;
    }
    return SIGNATURE_NOT_FOUND;
}

int omnicfg_dispatcher(u8 *ptr, unsigned int len, u8 *mac_addr, u8 *ap_addr, u32 flags)
{
    int ret = 0;

    if((__omnicfg_filter_mode & FILTER_SISO_SCAN) && (flags & (FILTER_SISO_SCAN|FILTER_SISO_DECODE)))
    {
        if((__omnicfg_filter_mode & FILTER_SISO_DECODE) && (len>=SISO2_SAFE_BASE && len<SISO2_SAFE_BASE+SISO2_SAFE_RANGE))
        {
            ret = siso2_omnicfg_decode(ptr, cfg, len);
            if(ret==DECODE_DONE)
                goto decode_done;
        }
        else
        {
            ret = siso2_signature_search(ptr, cfg, len);

            if(ret==STAY_IN_THE_CHANNEL)
            {
                cfg->state = S_STAY_CHANNEL;
            }
            else if(ret==SIGNATURE_LOCKED)
            {
#ifdef PLAY_OMNI_VOICE
                // Only play omni_voice in situation that never lock before
                if (!(__omnicfg_filter_mode & (FILTER_MIMO_DECODE | FILTER_SISO_DECODE)))
                {
                    __omnicfg_voice_type = S_VOICE_CFG;
                    ieee80211_queue_delayed_work(g_wifi_sc->hw, &omnicfg_work, HZ);
                }
#endif

                cfg->state = S_DECODE;
                cfg->decoder = SISO_DECODER;
                cfg->next_decode_status_check_time = jiffies + OMNICFG_DECODE_STATUS_CHECK_INTERVAL;
                __omnicfg_filter_mode |= FILTER_SISO_DECODE;
            }
        }
    }

    if((__omnicfg_filter_mode & FILTER_MIMO_SCAN) && (flags & (FILTER_MIMO_SCAN|FILTER_MIMO_DECODE)))
    {
#if !defined(TEST_MIMO) // smrtcfg test mimo
        goto done;
#endif
        if(__omnicfg_filter_mode & FILTER_MIMO_DECODE)
        {
            if(memcmp(mac_addr, lock_mac_addr, 6))
                goto done;
            if (flags & DISPATCH_MIMO_SCAN)
            {
                cfg->signature_count++;
                ret = DECODE_IN_PROC;
            }
            else if(flags & DISPATCH_MIMO_DECODE)
            {
                if (NULL!=lock_data)
                {
                    free_lock_data(lock_data);
                    kfree(lock_data);
                    lock_data=NULL;
                }

                if (NULL==blocks)
                {
                    blocks=(struct sc_rx_block *)kmalloc(sizeof(struct sc_rx_block),GFP_ATOMIC);
                    if (NULL==blocks)
                    {
                        printk("!ERR: blocks memory not allocate\n");
                        goto done;
                    }
                    memset(blocks, 0, sizeof(struct sc_rx_block));
                    blocks->current_rx_block = -1;
                }
                cfg->valid_pkt_count++;
                ret = mimo_omnicfg_decode(len, blocks);
            }

            if(ret==DECODE_DONE)
                goto decode_done;
        }
        else
        {
            if(flags & DISPATCH_MIMO_SCAN)
            {
                if (NULL!=blocks)
                {
                    kfree(blocks);
                    blocks=NULL;
                    memset(lock_mac_addr,0,WLAN_ADDR_LEN);
                }
                if (NULL==lock_data)
                {
                    lock_data=(struct try_lock_data *)kmalloc(sizeof(struct try_lock_data), GFP_ATOMIC);
                    if (NULL==lock_data)
                    {
                        printk("!ERR: lock_data not allocate\n");
                        goto done;
                    }
                    memset(lock_data,0,sizeof(struct try_lock_data));
                }
    
                ret = mimo_signature_search(len, mac_addr, lock_data);

                if(ret==STAY_IN_THE_CHANNEL)
                {
                    cfg->state = S_STAY_CHANNEL;
                }
                else if(ret==SIGNATURE_LOCKED)
                {
#ifdef PLAY_OMNI_VOICE
                    // Only play omni_voice in situation that never lock before
                    if (!(__omnicfg_filter_mode & (FILTER_MIMO_DECODE | FILTER_SISO_DECODE)))
                    {
                        __omnicfg_voice_type = S_VOICE_CFG;
                        ieee80211_queue_delayed_work(g_wifi_sc->hw, &omnicfg_work, HZ);
                    }
#endif

                    cfg->state = S_DECODE;
                    cfg->decoder = MIMO_DECODER;
                    cfg->next_decode_status_check_time = jiffies + OMNICFG_DECODE_STATUS_CHECK_INTERVAL;
                    __omnicfg_filter_mode |= FILTER_MIMO_DECODE;
                    memcpy(lock_mac_addr, mac_addr, ETH_ALEN);
                }
            }
        }
    }

done:
    return 0;

decode_done:
#ifdef PLAY_OMNI_VOICE
    __omnicfg_voice_type = S_VOICE_NONE;
#endif
    ssid_chan = cfg->cur_chan;
    cfg->state = S_DONE;
    __omnicfg_filter_mode = FILTER_MODE_DROP_ALL;
    ieee80211_queue_delayed_work(g_wifi_sc->hw, &omnicfg_usrwork, 0);
    return 0;
}

int omnicfg_process(struct mac80211_dragonite_data *data)
{
    MAC_INFO* info = data->mac_info;
    volatile rx_descriptor* rxdescr;
    unsigned long pkt_descr[(sizeof(rx_packet_descriptor)+4)/4];
    rx_packet_descriptor *pktdescr = (rx_packet_descriptor *) &pkt_descr[0];
    struct sk_buff *skb;
    int i;
    int head_index;
    int skb_offset;
    volatile buf_header* buffer_hdr;
    unsigned char *dptr;
    rx_packet_descriptor *pdescr;
    unsigned char *pktdata;
    struct ieee80211_hdr *wifi_hdr;

	i = info->rx_descr_index;
	while (info->rx_descriptors[i].own == 0)
	{
        skb = NULL;

		sniffer_rx_count++;

		rxdescr = &info->rx_descriptors[i];

		if((rxdescr->frame_header_head_index >= info->rx_buffer_hdr_count) 
				|| (rxdescr->frame_header_tail_index >= info->rx_buffer_hdr_count))
		{
			panic("dragonite_process_rx_queue(): wrong software rx buffer header index H:%d T:%d (%d), rxdescr %p\n", 
					rxdescr->frame_header_head_index, rxdescr->frame_header_tail_index, info->rx_buffer_hdr_count, rxdescr);
		}

        omnicfg_lock();

        if ((g_wifi_sc->started) && (FILTER_MODE_DROP_ALL!=__omnicfg_filter_mode))
        {
        	head_index = rxdescr->frame_header_head_index;
        	buffer_hdr = &info->buf_headers[head_index];
        
        	skb_offset = buffer_hdr->offset;

        	dptr = (unsigned char *) UNCACHED_VIRTUAL_ADDR(((unsigned long)buffer_hdr->dptr));
        	memcpy(pktdescr, dptr, sizeof(rx_packet_descriptor));
        	pdescr = pktdescr;

    		skb = (struct sk_buff *) info->swpool_bh_idx_to_skbs[head_index];
    		if((0x80000000UL > ((u32) skb)) || (( (u32) skb) == 0))
    			panic("retrieve_skb_from_rx_descr(): wrong rx skb buffer ptr %p, rxdescr %p\n", skb, rxdescr);

    		/* check for a possibly error condition, as wifi header is larger than 32bytes */
    		if(skb_offset < 8)
    		{
    			printk(KERN_EMERG "possibly wrong wifi header length, header offset %d, len %d\n", skb_offset, buffer_hdr->len);
    		}

            //printk(KERN_EMERG "packet_len = %x, skb_offset = %x", pdescr->packet_len, skb_offset);
            pktdata = (unsigned char *) &skb->data[skb_offset];

    		if(rxdescr->fsc_err_sw)
    		{
                omnicfg_filter((u8 *)UNCAC_ADDR(pktdata), (0x1fff & pdescr->packet_len), TYPE_UF);
    		}
            else if((pktdata[0]==0x88) || (pktdata[0] == 0x08))
            {
                omnicfg_filter((u8 *)UNCAC_ADDR(pktdata), (0x1fff & pdescr->packet_len), TYPE_NONUF);
            }
            else
            {
                wifi_hdr = (struct ieee80211_hdr *) UNCAC_ADDR(pktdata);
                if(ieee80211_is_ctl(wifi_hdr->frame_control))
                {
                    if(ieee80211_is_ack(wifi_hdr->frame_control))
                    {
                        omnicfg_filter((u8 *)UNCAC_ADDR(pktdata), pdescr->packet_len, TYPE_ACK);
                    }
                    else if(ieee80211_is_back(wifi_hdr->frame_control))
                    {
                        omnicfg_filter((u8 *)UNCAC_ADDR(pktdata), pdescr->packet_len, TYPE_BA);
                    }
                }

#if defined(HANDLE_ANDROID_PROBE_REQUEST)
                if(ieee80211_is_probe_req(wifi_hdr->frame_control))
                {
                    omnicfg_cmd52_detection((unsigned char *) wifi_hdr, pdescr->packet_len);
                }
#endif
            }
        }

        omnicfg_unlock();
		while (MACREG_READ32(MAC_FREE_CMD) & MAC_FREE_CMD_BUSY)
                    ;
		MACREG_WRITE32(MAC_FREE_PTR,
				((info->rx_descriptors[i].frame_header_head_index << 16)
				 | info->rx_descriptors[i].frame_header_tail_index));

		MACREG_WRITE32(MAC_FREE_CMD,  MAC_FREE_CMD_SW_BUF | MAC_FREE_CMD_KICK);

		info->rx_descriptors[i].own = 1;
		i = (i + 1) % info->rx_descr_count;
		info->rx_descr_index = i;

		MACREG_WRITE32(CPU_RD_SWDES_IND, 1);     /* kick RX queue logic as it might be pending on get free RX descriptor to write */
	}

    return 0;
}

void gen_bit_mask(unsigned int start, unsigned int end, unsigned int *mask)
{
    int block_start, block_end;
    int low, high, i;
    unsigned int reverse_mask=0;

    if(start > end)
    {
        low = end;
        high = start;
    }
    else
    {
        low = start;
        high = end;
    }
    
    for(i=0; i < SC_LOCK_PHASE_BIT_MAP_NUMS; i++)
    {
        block_start = 32*i;
        block_end = 32*(i + 1);
        mask[i] = 0;

        if(start == end)
            continue;
        else
        {
            if((low < block_end) && (high > block_start))
            {
                if(low < block_end)
                {
                    if(low <= block_start)
                        mask[i] = (unsigned int)-1;
                    else
                        mask[i] = (unsigned int)(0 - (1 << (low - block_start)));
                }
                if(high < block_end)
                {
                    reverse_mask = (1 << (high - block_start)) - 1;
                    mask[i] = mask[i] & reverse_mask;
                }
            }
        }
    }
}

struct lock_phase_ta* insert_lock_data(unsigned int data, unsigned char *addr, struct try_lock_data *lock_data)
{
    int current_pos=0;
    int i, x, y;
    struct lock_phase_ta *ta=NULL, *find=NULL, *pre=NULL;
    int observe_window = (SC_FRAME_TIME_INTERVAL + 10) * 4;
    unsigned int current_time = current_timestamp();
    unsigned int bit_mask[SC_LOCK_PHASE_BIT_MAP_NUMS];

    /* pass too old entries */
    while(lock_data->start_pos != lock_data->next_pos) // "==" : no entry
    {
#if 0
        SC_PRINT("current_time=%d, timestamp[%d]=%d, diff=%d, observe_window=%d\n",
                current_time, lock_data->start_pos, lock_data->timestamp[lock_data->start_pos],
                ((int)current_time - (int)lock_data->timestamp[lock_data->start_pos]), observe_window);
#endif
        if(((int)current_time - (int)lock_data->timestamp[lock_data->start_pos]) > observe_window)
        {
            if(++lock_data->start_pos >= SC_LOCK_PHASE_DATA_NUMS)
                lock_data->start_pos = 0;
        }
        else
            break;
    }

    /* insert data */
    lock_data->frames[lock_data->next_pos] = data;
    lock_data->timestamp[lock_data->next_pos] = current_time;
    current_pos = lock_data->next_pos;

    /* lock_data->next_pos causes the valid data num = (SC_LOCK_PHASE_DATA_NUMS - 1).
       It is acceptable.                                                                */
    if((++lock_data->next_pos) >= SC_LOCK_PHASE_DATA_NUMS)
        lock_data->next_pos = 0;
    if(lock_data->next_pos == lock_data->start_pos)
    {
        if(++lock_data->start_pos >= SC_LOCK_PHASE_DATA_NUMS)
            lock_data->start_pos = 0;
    }

#if 0
    SC_PRINT("%s(): next = %d, start=%d\n", __FUNCTION__, lock_data->next_pos, lock_data->start_pos);
#endif

    gen_bit_mask(lock_data->start_pos, lock_data->next_pos, bit_mask);
#if 0
    SC_PRINT("mask = ");
    for(i=0; i<SC_LOCK_PHASE_BIT_MAP_NUMS; i++)
        SC_PRINT("0x%08x:", bit_mask[i]);
    SC_PRINT("\n");
#endif
    ta = lock_data->ta_list;
    while(ta)
    {
        if((find == NULL) && (memcmp(ta->addr, addr, 6) == 0))
            find = ta;

        /* rebuild the bit_map */
        for(i=0; i<SC_LOCK_PHASE_BIT_MAP_NUMS; i++)
            ta->bit_map[i] &= bit_mask[i];

        pre = ta;
        ta = ta->next;
    }

    if(find == NULL)
    {
        find = (struct lock_phase_ta *)SC_MALLOC(sizeof(struct lock_phase_ta));
        if(find == NULL)
            return NULL;
        memset(find, 0, sizeof(struct lock_phase_ta));
        memcpy(find->addr, addr, 6);
        if(pre)
            pre->next = find;
        else
            lock_data->ta_list = find;
    }

    x = (current_pos)/(32);
    y = (current_pos)%(32);
    find->bit_map[x] |= (1 << y);

#if 0
    SC_PRINT("%s(): x=%d, y=%d, current_pos=%d, find->bit_map[x]=0x%08x\n",
            __FUNCTION__, x, y, current_pos, find->bit_map[x]);
#endif

    return find;
}

static int match_arr[32][3];
#ifdef NEW_LOCK_MATCH
static int match2_arr[30];
static int after_sort_match2_arr[30];
#endif
int lock_match(struct lock_phase_ta *ta, struct try_lock_data *lock_data)
{
    int ptr = lock_data->start_pos;
    int i, j;
    int x, y;
    int header_len=0;
#ifdef NEW_LOCK_MATCH
    int temp, duplicate=0, total_size=0;
#endif
    unsigned int comp_val=0;
    memset(match_arr, 0, sizeof(match_arr));
#ifdef NEW_LOCK_MATCH
    memset(match2_arr, 0 ,sizeof(match2_arr));
    memset(after_sort_match2_arr, 0, sizeof(after_sort_match2_arr));
#endif

    while(ptr != lock_data->next_pos)
    {
        x = (ptr)/32;
        y = (ptr)%32;
        
        if(ta->bit_map[x] & (1 << y))
        {
            comp_val = lock_data->frames[ptr] - SC_LOCK_INTERVAL_LEN;
#ifdef NEW_LOCK_MATCH
            for(i=1;i<30;i++)//check duplicate or not, match2_arr[0] is for insert new data
            {
                if(match2_arr[i]==lock_data->frames[ptr])
                {
                    duplicate = 1;
                    break;
                }
                else if(match2_arr[i]==0)
                {
                    total_size = i;
                    break;
                }
            }
            if(i>29)//more than 29 pkts
            {
		total_size = 29;
            }
            if(duplicate)
            {
                duplicate = 0;
            }
            else{

                match2_arr[0]=lock_data->frames[ptr];//insert data

                for(i=0;i<total_size;i++)//copy to another string
                {
                    after_sort_match2_arr[i] = match2_arr[i];
                }
                for(i=0;i<total_size-1;i++)//sort
                {
                    for(j=0;j<total_size-1-i;j++)
                    {
                        if(after_sort_match2_arr[j]>after_sort_match2_arr[j+1])
                        {
                            temp  = after_sort_match2_arr[j];
                            after_sort_match2_arr[j] = after_sort_match2_arr[j+1];
                            after_sort_match2_arr[j+1] = temp;
                        }
                    }
                }
                for(i=0;i<total_size-3;i++)
                {
                    if(((after_sort_match2_arr[i]+SC_LOCK_INTERVAL_LEN==after_sort_match2_arr[i+1])&&
                        (after_sort_match2_arr[i+1]+SC_LOCK_INTERVAL_LEN==after_sort_match2_arr[i+2])) &&
                       (after_sort_match2_arr[i+2]+SC_LOCK_INTERVAL_LEN==after_sort_match2_arr[i+3]))
                    {//match or not

                        header_len = after_sort_match2_arr[i] - SC_LOCK_SPEC_CHAR - SC_LOCK_LENGTH_BASE;
                        SC_PRINT("1-find the match value header_len=%d (0)\n", header_len);

                        return header_len;
                    }
                }
                for(i=total_size-1;i>=0;i--)//not match, so push
                {
                    match2_arr[i+1] = match2_arr[i];
                }
            }
#else
            for(i=0; i<32; i++)
            {
                if(match_arr[i][0] == 0)
                {
                    match_arr[i][0] = lock_data->frames[ptr];
                    break;
                }
                else
                {
                    for(j=2; j>=0; j--)
                    {
                        if((match_arr[i][j] != 0) && (match_arr[i][j] == comp_val))
                        {
                            if(j == 2)
                            {
                                header_len = match_arr[i][0] - SC_LOCK_SPEC_CHAR - SC_LOCK_LENGTH_BASE;
#if defined(MIMO_DEBUG)
                                SC_PRINT("find the match value header_len=%d (0)\n", header_len);
#endif
                                return header_len; 
                            }
                            else
                            {
                                match_arr[i][j+1] = lock_data->frames[ptr];
                            }
                        }
                    }
                }
            }
#endif
        }

        if(++ptr >= SC_LOCK_PHASE_DATA_NUMS)
            ptr = 0;
    }

#if 0
    for(i =0; i<32; i++)
    {
        SC_PRINT("ta=%02x:%02x:%02x:%02x:%02x:%02x:, comp_val=%x, match_arr[%d] = %04x:%04x:%04x\n", 
                ta->addr[0], ta->addr[1], ta->addr[2], 
                ta->addr[3], ta->addr[4], ta->addr[5], comp_val,
                i, match_arr[i][0], match_arr[i][1], match_arr[i][2]);
    }
#endif
    return 0;
}

int try_lock_channel(unsigned int data, unsigned char *addr, struct try_lock_data *lock_data, int *ret_state)
{
    int header_len=0;
    static unsigned long limit_time = 0;
    struct lock_phase_ta *ta;

    if(time_after(limit_time, jiffies))
    {
        *ret_state=STAY_IN_THE_CHANNEL;
    }
    ta = insert_lock_data(data, addr, lock_data);
    if(ta)
    {
        limit_time = jiffies + MIMO_LOCK_ATTEMPT_TIME;
        header_len = lock_match(ta, lock_data);
    }

    return header_len;  
}

int free_lock_data(struct try_lock_data *lock_data)
{
    struct lock_phase_ta *ta, *next;

    ta = lock_data->ta_list;

    while(ta)
    {
        next = ta->next;
        SC_FREE(ta);
        ta = next;
    }
    return 0;
}

unsigned char* mimo_decoder(unsigned int data, struct sc_rx_block *blocks)
{
    static unsigned char pre_control_data = 0x00;
    int total_length = 0;
    struct sc_block_entry *entry;
    unsigned char *ret=NULL, *ptr=NULL;
    int block_num=0;
    int i, tmp, total_group = 0;
    int check;
    uint8_t data_crc;

    if(OMNICFG_CONFIG_LENGTH_BASE > data)
        return NULL;

    data = data - OMNICFG_CONFIG_LENGTH_BASE;
    
    if(data & OMNICFG_CONFIG_LENGTH_MASK)        // not 2x2 omni config info
    {
        //SC_PRINT("%s(): not in the sc location\n", __FUNCTION__);
        return NULL;
    }

    data = data >> OMNICFG_CONFIG_SHIFT;
    check = (blocks->block_counter[0] >> 1);

#if defined(MIMO_DEBUG)
    SC_PRINT("data %02x\n",data);
#endif
    if(pre_control_data == data)
        return NULL;
    else
        pre_control_data = data;

    if((blocks->pending_rx_data) && (data & SC_CONTROL_DATA)) // pass this data info rx
    {
        //SC_PRINT("%s(): pending rx data\n", __FUNCTION__);
        return NULL;
    }
#if defined(MIMO_DEBUG)
    SC_PRINT("accept data=%x\n",data);
#endif
    if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
    {
        printk(KERN_INFO "accept data=%x\n",data);
    }
    if(data & SC_CONTROL_DATA)  // data
    {
        if(0>blocks->current_rx_block)
            return NULL;

        entry = &blocks->block[blocks->current_rx_block];
        if(blocks->current_rx_block < OMNICFG_CONFIG_BLOCK_NUMS)
        {
            if(entry->current_rx_count < OMNICFG_CONFIG_BLOCK_FRAMES)
            {    
                entry->value[entry->current_rx_count] = (data & 0xf);
                entry->current_rx_count++;
                if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
                {
                    printk(KERN_INFO "\t\t\tpass\n");
                }
            }
        }
    }
    else//control
    {
        block_num = (data - SC_CTL_SSID_BLOCK_0);
        if(block_num < OMNICFG_CONFIG_BLOCK_NUMS && block_num >= 0)
        {
            SC_PRINT("find the group %d, pending_rx_data=%d, ",block_num, blocks->pending_rx_data);
            if(blocks->current_rx_block>=0)
            {
                entry = &blocks->block[blocks->current_rx_block];
                SC_PRINT("current_rx_count=%d, ", entry->current_rx_count);

                if(blocks->pending_rx_data)
                    blocks->pending_rx_data = 0;
                else if((entry->current_rx_count == OMNICFG_CONFIG_BLOCK_FRAMES) &&
                    ((block_num == (blocks->current_rx_block + 1)) || (block_num == 0)))
                {
                    blocks->block_map |= (1 << blocks->current_rx_block);
                }
            }

            SC_PRINT("current block %d, block_map %x\n",
                     blocks->current_rx_block, blocks->block_map);

            if(blocks->block_map & 0x01)
            {
                entry = &blocks->block[0];
                total_length = (entry->value[0] << 4) | entry->value[1];
                total_length+=2;            
            }

            if(blocks->block_map & (1 << block_num))
                blocks->pending_rx_data = 1;
            else
            {
                entry = &blocks->block[block_num];
                entry->current_rx_count = 0;
            }
            
            blocks->current_rx_block = block_num;

            if(blocks->block_counter[block_num] < 255)
                blocks->block_counter[block_num]++;
            if(blocks->block_map)
            {
                for(i=(OMNICFG_CONFIG_BLOCK_NUMS-1); i>=0; i--)
                {
                    if(blocks->block_counter[i] == 0)
                        continue;
                    // we expect that the rx group id conters should be closed

                    if((blocks->block_counter[0] > 3) && (blocks->block_counter[i] < check))
                        continue;
                    tmp = (1 << (i+1)) - 1;
                    if((tmp & blocks->block_map) == tmp)
                    {
                        total_group = i+1;
                        if(total_group >= ((total_length+7)/8))
                        {
                            ret = (unsigned char *)SC_MALLOC(
                                            OMNICFG_CONFIG_BLOCK_BYTES * total_group);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            /* do other control action */
        }
    }
    if(ret) // complete all blocks
    {
        ptr = ret;

        for(i=0; i < total_group; i++)
        {
            entry = &blocks->block[i];
            for(tmp=0; tmp < OMNICFG_CONFIG_BLOCK_FRAMES; tmp += 2)
            {
                *ptr = (entry->value[tmp] << 4) | entry->value[tmp+1];
                ptr++;
            }
        }
#if defined(MIMO_DEBUG)
        if ((OMNICFG_CONFIG_BLOCK_BYTES * total_group) <= ret[0])
        {
            SC_PRINT("!!!Decode length error\n");
        }
        else
        {
            SC_PRINT("ret=%02x",ret[0]);
            for(i=0; i<ret[0]; i++)
            {
                SC_PRINT(" %02x",ret[i+1]);
            }
            SC_PRINT("\n");
        }
#endif
        data_crc = Crc8(ret, total_length - 1);
        if(data_crc == ret[total_length-1])
        {
            printk("Pass crc check, crc = %02x\n", data_crc);
        }
        else
        {
            printk("Wrong crc!!, correct crc = %02x, now crc = %02x\n", ret[total_length-1], data_crc);
            //ReDecoding until pass crc check;
            for(i=0; i < total_length; i++)
            {
                if(omnicfg_debug_flag == OMNICFG_DEBUG_FLAG_OPEN)
                {    
                    printk(KERN_INFO "%02d:%02x %c\n", i, ret[i], ret[i]);
                }
            }
            memset(blocks, 0, sizeof(struct sc_rx_block));
            blocks->current_rx_block = -1;
            SC_FREE(ret);
            ret = NULL;
        }
    }

    return ret;
}
