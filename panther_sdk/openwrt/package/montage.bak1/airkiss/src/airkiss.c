/*
 * =====================================================================================
 *
 *       Filename:  airkiss.c
 *
 *    Description:
 *
 *        Version:  2.0
 *        Created:  04/17/2017 14:00:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *   Organization:
 *
 * =====================================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
//#include <pcap.h>
//#include <tapi/timerfd.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <wdk/cdb.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <libutils/shutils.h>
#include <libutils/wlutils.h>
#include <libutils/nl80211.h>
#include "inc/airkiss.h"
#include "inc/utils.h"

#define AK_INTERFACE "wlan0"
#define AK_MONITOR "mon"

#define ETH_ALEN 6
#define BUFSIZE  8192
#define HEADROOM  256

/* Radiotap definitions */
#define RADIOTAP_TSFT  0
#define RADIOTAP_FLAGS 1

#define RADIOTAP_F_WEP        0x04
#define RADIOTAP_F_FCS_FAILED 0x40

struct radiotap_hdr {
    uint8_t  version;
    uint8_t  pad;
    uint16_t length;
    uint32_t present;
} __attribute__((packed));

#define SPEEDUP_WLAN_CONF
#define AIRKISS_COUNTRY_CN
#define AIRKISS_SUPPORT_SECOND_CHANNEL
#define AIRKISS_DYNAMIC_CHANNEL_LIST
#define AIRKISS_SEND_DELBA

#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)

//#define DUMPBUF
#if defined(AIRKISS_COUNTRY_CN)
static int chan_seq[] = { 1, 6, 11, 2, 7, 12, 3, 8, 13, 4, 9, 5, 10 };
#else
static int chan_seq[] = { 1, 6, 11, 2, 7, 3, 8, 4, 9, 5, 10 };
#endif
#if 0
static char neiborchan[13][2] = {
    {10, 1},
    {11, 2},
    {12, 3},
    {13, 4},
    {9, 5},
    {1, 6},
    {2, 7},
    {3, 8},
    {4, 9},
    {5, 10},
    {6, 11},
    {7, 12},
    {4, 13},
};
#endif
airkiss_context_t *akcontex = NULL;

unsigned char chanidx;
bool use_below = true;
time_t recover_time;
time_t neibor_time;
time_t stop_time;

const airkiss_config_t akconf = {
    (airkiss_memset_fn) & memset,
    (airkiss_memcpy_fn) & memcpy,
    (airkiss_memcmp_fn) & memcmp,
    (airkiss_printf_fn) & printf
};

static int has_two_secondary_channels(int channel)
{
#if defined(AIRKISS_COUNTRY_CN)
        return ((channel >= 5) && (channel <= 9));
#else
        return ((channel >= 5) && (channel <= 7));
#endif
}
static int secondary_channels_is_above(int channel)
{
        return ((channel >= 1) && (channel <= 4));
}
static int set_if_flags(char *ifname, short flags)
{
    int sockfd;
    struct ifreq ifr;
    int res = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
        return -1;

    memset(&ifr, 0, sizeof ifr);

    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    res = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    if (res < 0)
    {
        cprintf("Interface '%s': Error: SIOCSIFFLAGS failed: %s\n",
                ifname, strerror(errno));
    }

    close(sockfd);

    return res;
}

static int set_if_up(char *ifname, short flags)
{
    return set_if_flags(ifname, flags | IFF_UP);
}

static int nl80211_init(struct nl80211_state *state)
{
    int err;

    state->nl_sock = nl_socket_alloc();
    if (!state->nl_sock)
    {
        cprintf("Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    if (genl_connect(state->nl_sock))
    {
        cprintf("Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
    if (state->nl80211_id < 0)
    {
        cprintf("nl80211 not found.\n");
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

out_handle_destroy:
    nl_socket_free(state->nl_sock);
    return err;
}

static void monitor_interface_add(void)
{
    struct nl_msg *msg = NULL;
    struct nl80211_state nlstate;
    int err = -1;
    unsigned int devidx = 0;

    /* get device index of "wlan0" */
    devidx = if_nametoindex(AK_INTERFACE);
    if (devidx == 0)
    {
        cprintf("cannot find wlan0\n");
        devidx = -1;
        return;
    }

    err = nl80211_init(&nlstate);
    if (err)
    {
        cprintf("nl80211 init fail\n");
        return;
    }

    msg = nlmsg_alloc();
    if (!msg)
    {
        cprintf("failed to allocate netlink message\n");
        goto nla_put_failure;
    }
    genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, 0, NL80211_CMD_NEW_INTERFACE,
                0);

    /* put data in netlink message */
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

    NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, MONITOR_DEVICE);
    NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);

    err = nl_send_auto_complete(nlstate.nl_sock, msg);
    if (err < 0)
    {
        cprintf("Send netlink message fail\n");
        goto out_free_msg;
    }

    set_if_up(MONITOR_DEVICE, 0);

out_free_msg:
    if (msg)
        nlmsg_free(msg);
nla_put_failure:
    nl_socket_free(nlstate.nl_sock);
    return;
}

static void monitor_interface_del(void)
{
    struct nl_msg *msg = NULL;
    struct nl80211_state nlstate;
    int err = -1;
    unsigned int devidx = 0;

    /* get devide index of "mon" */
    devidx = if_nametoindex(MONITOR_DEVICE);
    if (devidx == 0)
    {
        cprintf("cannot find mon\n");
        devidx = -1;
	return;
    }

    err = nl80211_init(&nlstate);
    if (err)
    {
        cprintf("nl80211 init fail\n");
        return;
    }

    msg = nlmsg_alloc();
    if (!msg)
    {
        cprintf("failed to allocate netlink message\n");
        goto nla_put_failure;
    }
    genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, 0, NL80211_CMD_DEL_INTERFACE,
                0);

    /* put data in netlink message */
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

    err = nl_send_auto_complete(nlstate.nl_sock, msg);
    if (err < 0)
    {
        cprintf("Send netlink message fail\n");
        goto out_free_msg;
    }

out_free_msg:
    if (msg)
        nlmsg_free(msg);
nla_put_failure:
    nl_socket_free(nlstate.nl_sock);
    return;
}

static void mon_srv_stop(void)
{
    exec_cmd("mtc ra stop");
#ifdef SPEEDUP_WLAN_CONF
    cdb_set_int("$op_work_mode", 1);
    exec_cmd("mtc wlhup");
    sleep(1);
#else
    exec_cmd("mtc wl stop");
    sleep(2);
#endif
}

void wifi_promiscuous_enable(bool is_open)
{
    if (is_open)
    {
        monitor_interface_add();
        exec_cmd("wd airkiss start");
    }
    else
    {
        exec_cmd("wd airkiss stop");
        monitor_interface_del();
    }
}
#if defined(AIRKISS_DYNAMIC_CHANNEL_LIST)
static void remake_channel_sequence(void)
{
    int seq_size, r_num, idx, tmp;

    seq_size = sizeof(chan_seq)/sizeof(int);
    r_num = chan_seq[0] + chan_seq[1] + chan_seq[2];

    for(idx = 0; idx < seq_size; idx++)
    {
        r_num = (chan_seq[idx] * 33 + r_num) % seq_size;

        if(idx != r_num)
        {
            tmp = chan_seq[idx];
            chan_seq[idx] = chan_seq[r_num];
            chan_seq[r_num] = tmp;
        }
    }
}
#endif

#if defined(AIRKISS_SEND_DELBA)
static unsigned char delba_target_ap_addr[ETH_ALEN];
static unsigned char delba_target_sta_addr[ETH_ALEN];
static int encap_radiotap(uint8_t *buf, size_t pos)
{
    struct radiotap_hdr *rthdr = (void*)&buf[pos - sizeof *rthdr - 1];

    rthdr->version = 0;
    rthdr->pad = 0;
    rthdr->length = htole16(sizeof *rthdr + 1);
    rthdr->present = htole32(1 << RADIOTAP_FLAGS);
    buf[pos - 1] = RADIOTAP_F_WEP;
    return pos - sizeof *rthdr - 1;
}
static int encap_action_frame(uint8_t *buf, size_t pos, bool to_ds)
{
    /* TID 0 */
    unsigned char mgmt_action[6] = {0x3, 0x2, 0x0, 0x0, 0x25, 0x0};

    buf[pos - 30] = 0xd0; /* type and subtype */
    buf[pos - 29] = 0x0;
    buf[pos - 28] = 0x0; /* duration */
    buf[pos - 27] = 0x0;

    if(to_ds)
    {
        memcpy(&buf[pos - 26], delba_target_ap_addr, ETH_ALEN); /* DA */
        memcpy(&buf[pos - 20], delba_target_sta_addr, ETH_ALEN); /* SA */
        memcpy(&buf[pos - 14], delba_target_ap_addr, ETH_ALEN); /* BSSID */
    }
    else
    {
        memcpy(&buf[pos - 26], delba_target_sta_addr, ETH_ALEN); /* DA */
        memcpy(&buf[pos - 20], delba_target_ap_addr, ETH_ALEN); /* SA */
        memcpy(&buf[pos - 14], delba_target_ap_addr, ETH_ALEN); /* BSSID */
    }

    buf[pos - 8] = 0x0; /* sequence number */
    buf[pos - 7] = 0x0;

    memcpy(&buf[pos - 6], mgmt_action, 6);

    if(to_ds)
    {
        buf[pos - 3] = 0x8; /* originator */
    }
    else
    {
        buf[pos - 3] = 0x0; /* recipient */
    }

    return pos - 30;
}

void airkiss_tx_send_delba(int sock, bool to_ds)
{
    static uint8_t g_buffer[BUFSIZE];
    uint8_t *buf = g_buffer;
    int len = HEADROOM, pos = HEADROOM;

    pos = encap_action_frame(buf, pos, to_ds);
    pos = encap_radiotap(buf, pos);

    send(sock, &buf[pos], len - pos, 0);
}
#endif
void wifi_set_channel(struct mon_airkiss *monair, bool init)
{
    unsigned int curchan, idx;

    pthread_mutex_lock(&(monair->lock));

    if (!monair->is_chan_lock)
    {
        if (!init)
        {
            if(monair->stay_channel_tick)
            {
                monair->stay_channel_tick--;
                goto exit;
            }
            if (chanidx >= (sizeof(chan_seq)/sizeof(int)) - 1)
            {
#if defined(AIRKISS_DYNAMIC_CHANNEL_LIST)
                remake_channel_sequence();
#endif
                chanidx = 0;
                use_below = !use_below;
            }
            else
                chanidx++;
        }
        else
        {
            idx = cdb_get_int("$airkiss_chidx", 0);
            chanidx = (idx == 0) ? 0 : idx;
            if (chanidx > (sizeof(chan_seq)/sizeof(int)) - 1)
                chanidx = 0;
        }

        curchan = chan_seq[chanidx];

#if defined(AIRKISS_SUPPORT_SECOND_CHANNEL)
        if(has_two_secondary_channels(curchan))
        {
            if(use_below)
            {
                cprintf("CHAN:%d-\n", curchan);
                exec_cmd2("wd chan %d 3", curchan);
                monair->current_channel_offset = 3;
            }
            else
            {
                cprintf("CHAN:%d+\n", curchan);
                exec_cmd2("wd chan %d 1", curchan);
                monair->current_channel_offset = 1;
            }
        }
        else
        {
            if(secondary_channels_is_above(curchan))
            {
                cprintf("CHAN:%d+\n", curchan);
                exec_cmd2("wd chan %d 1", curchan);
                monair->current_channel_offset = 1;
            }
            else
            {
                cprintf("CHAN:%d-\n", curchan);
                exec_cmd2("wd chan %d 3", curchan);
                monair->current_channel_offset = 3;
            }
        }
#else
        cprintf("CHAN:%d \n", curchan);
        exec_cmd2("wd chan %d 0", curchan);
#endif
        monair->current_channel = curchan;

        if(!monair->is_ap_lock)
        {
            airkiss_change_channel(akcontex);
        }
        else
        {
            if(monair->lock_ap_tick > 0)
            {
                monair->lock_ap_tick--;
            }
            else
            {
                monair->is_ap_lock = false;
            }
        }
    }
    else if(!monair->is_airkiss_complete)
    {
        if((monair->current_channel != monair->primary_channel) || (monair->current_channel_offset != monair->second_channel_offset))
        {
            cprintf("CHAN:%d OFFSET:%d\n", monair->primary_channel, monair->second_channel_offset);
            exec_cmd2("wd chan %d %d", monair->primary_channel, monair->second_channel_offset);

            monair->current_channel = monair->primary_channel;
            monair->current_channel_offset = monair->second_channel_offset;
        }

#if defined(AIRKISS_SEND_DELBA)
        airkiss_tx_send_delba(monair->sockfd, true);

        airkiss_tx_send_delba(monair->sockfd, false);
#endif
    }

#if 0
        /*IF out of RECOVERTIME then restart airkiss */
        if (!monair->is_wifi_scan || !monair->is_airkiss_complete)
        {
            if (time(NULL) > recover_time)
            {
                start_airkiss(monair);
            }
            /*Assume lock in the neighbor scan channel */
            else if (!monair->is_wifi_scan)
            {
                if (time(NULL) > neibor_time)
                {
                    static char neiboridx = -1;
                    if (neiboridx >= 1)
                        neiboridx = 0;
                    else
                        neiboridx++;

                    printf("monair->cur_channel:%d Neibor chan:%d\n",
                           monair->cur_channel,
                           neiborchan[monair->cur_channel - 1][neiboridx]);
                    exec_cmd2("wd smrtcfg chan %d",
                              neiborchan[monair->cur_channel - 1][neiboridx]);

                    neibor_time = time(NULL) + NEIBORTIME;
                }
            }
        }
#endif
exit:
    pthread_mutex_unlock(&(monair->lock));
}
#if 0
#ifdef DUMPBUF
static void dump_buf(unsigned int *p, unsigned int s)
{
    int i;
    while ((int) s > 0)
    {
        for (i = 0; i < 4; i++)
        {
            if (i < (int) s / 4)
            {
                fprintf(stdout, "%08X ", p[i]);
            }
            else
            {
                fprintf(stdout, "         ");
            }
        }
        fprintf(stdout, "\n");
        s -= 16;
        p += 4;
    }
    fprintf(stdout, "\n");
}
#endif
#endif
#if 0
void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 128 bit AES (i.e. a 128 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if (1 !=
        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

static void mon_decrypt(unsigned int len, char *pass, char *cipher)
{
    unsigned char decryptedtext[128];
    char *key = "Wechatiothardwav";
    char *iv = "Wechatiothardwav";
    int decryptedtext_len;

    memset(decryptedtext, 0, 128);
    decryptedtext_len = decrypt(pass, len, key, iv, decryptedtext);

    decryptedtext[decryptedtext_len] = '\0';
    memset(pass, 0, len);
    memcpy(pass, decryptedtext, decryptedtext_len);
}
static void airkiss_pcap_break(struct mon_airkiss *monair)
{
    if (monair->handle)
        pcap_breakloop(monair->handle);
}
#endif
#ifdef SPEEDUP_WLAN_CONF
void set_ssid_pwd(char *ssid, char *pass)
{
    if (strlen(ssid) > 32)
    {
        printf("critical error, ssid two long : %s\n", ssid);
        return;
    }

    if (pass)
    {
        if (strlen(pass) > 64)
        {
            printf("critical error, pass two long : pass:%s\n", pass);
            return;
        }
    }

    cdb_set("$wl_bss_ssid2", ssid);
    cdb_set("$wl_bss_bssid2", "");

    if (!pass)
    {
        cdb_set("$wl_bss_sec_type2", "0");
        cdb_set("$wl_bss_cipher2", "0");
        cdb_set("$wl_bss_wpa_psk2", "");
        cdb_set("$wl_bss_wep_1key2", "");
        cdb_set("$wl_bss_wep_2key2", "");
        cdb_set("$wl_bss_wep_3key2", "");
        cdb_set("$wl_bss_wep_4key2", "");
        cdb_set("$wl_passphase2", "");
        return;
    }

    cdb_set("$wl_bss_wep_1key2", pass);
    cdb_set("$wl_bss_wep_2key2", pass);
    cdb_set("$wl_bss_wep_3key2", pass);
    cdb_set("$wl_bss_wep_4key2", pass);
    cdb_set("$wl_passphase2", pass);
    cdb_set("$wl_bss_sec_type2", "4");
}
#endif

static void airkiss_finish(struct mon_airkiss *monair)
{
    int idx;

    airkiss_result_t result;

    //airkiss_pcap_break(monair);

    airkiss_get_result(akcontex, &result);

    monair->is_wifi_scan = true;

    wifi_promiscuous_enable(false);

    cprintf("ssid = \"%s\", pwd = \"%s\", ssid_length = %d, "
            "pwd_length = %d, random = 0x%02x\n",
            result.ssid, result.pwd, result.ssid_length,
            result.pwd_length, result.random);

    cdb_set_int("$airkiss_state", AIRKISS_DONE);

#ifdef SPEEDUP_WLAN_CONF
    cdb_set_int("$omi_scan_freq", (2412 + (monair->current_channel - 1) * 5));
#endif
    cdb_set_int("$airkiss_val", result.random);
    for (idx = 0; idx < sizeof (chan_seq); idx++)
    {
        if (monair->current_channel == chan_seq[idx])
        {
            cdb_set_int("$airkiss_chidx", idx);
            break;
        }
    }
#if 0
    // Montage decrypt by openssl
    if ((result.pwd_length != 0))
        mon_decrypt(result.pwd_length, result.pwd, NULL);
#endif

    monair->is_airkiss_complete = true;

#ifdef SPEEDUP_WLAN_CONF
    set_ssid_pwd(result.ssid, (result.pwd_length != 0) ? result.pwd : NULL);
    cdb_set("$op_work_mode", "3");
    monair->is_airkiss_complete = true;
    exec_cmd("mtc wlhup");
#else
    for (cnt = 0; cnt < SCAN_RETRY_CNT; cnt++)
    {
        unsigned int ap_found;
        ap_found =
            wifi_set_ssid_pwd(result.ssid,
                              (result.pwd_length != 0) ? result.pwd : NULL);
        if (ap_found == SCAN_AP_FOUND)
        {
            printf("\033[41;32mAirKiss Success:Restart process.\033[0m\n");
            cdb_set("$op_work_mode", "3");
            monair->is_airkiss_complete = true;

            exec_cmd("mtc commit");
            break;
        }
        else if (ap_found == SCAN_AP_NOT_FOUND)
            continue;
        else
            break;
    }
#endif
}

#if 0
static void dump_recv(unsigned char *args, struct pcap_pkthdr *header,
                      unsigned char *packet)
{
    int ret;
    struct mon_airkiss *monair = (struct mon_airkiss *) args;

    pthread_mutex_lock(&(monair->lock));

    if (monair->is_wifi_scan)
        goto unlock;

    if (header->len > WIFIOFFSET)
    {
        packet += WIFIOFFSET;
        header->len -= WIFIOFFSET;
#ifdef DUMPBUF
        dump_buf((unsigned int *) packet, header->len);
#endif
        ret = airkiss_recv(akcontex, (void *) packet, header->len);

        if (ret == AIRKISS_STATUS_CHANNEL_LOCKED)
        {
            cprintf("LOCK AT CHAN:%d\n", monair->cur_channel);
            monair->is_chan_lock = true;
            recover_time = time(NULL) + RECOVERTIME;
            neibor_time = time(NULL) + NEIBORTIME;
        }
        else if (ret == AIRKISS_STATUS_COMPLETE)
            airkiss_finish(monair);
    }

unlock:
    pthread_mutex_unlock(&(monair->lock));

}
#endif

int start_airkiss(struct mon_airkiss *monair)
{
    int ret;
    //const char *key = "Wechatiothardwav";

    monair->is_chan_lock = monair->is_airkiss_complete = monair->is_wifi_scan =
        false;

    if (akcontex == NULL)
        akcontex = malloc(sizeof (airkiss_context_t));
    if (akcontex == NULL)
    {
        cprintf("start_airkiss malloc mem fail!\n");
        goto air_fail;
    }

    /*akcontex might be free by airkiss_init because ackontex became NULL */
    ret = airkiss_init(akcontex, &akconf);

    if (ret < 0)
    {
        cprintf("Airkiss init failed!\n");
        goto air_fail;
    }
//#if AIRKISS_ENABLE_CRYPT
//    airkiss_set_key(&akcontex, key, strlen(key));
//#endif
    return 0;

air_fail:
    return 1;
}

static int t_len;
static void *decap_radiotap(uint8_t *buf, size_t len)
{
    struct radiotap_hdr *rthdr = (void*)&buf[0];
    size_t rtmask, rtlen;

    /* Validate the Radiotap header size */
    if (len < sizeof *rthdr) {
        return 0;
    }
    rtlen = le16toh(rthdr->length);
    rtmask = le32toh(rthdr->present);
    if (len < rtlen) {
        return 0;
    }

    /* Discard frames with FCS error */
    if (rtmask & (1 << RADIOTAP_FLAGS)) {
        size_t idx = sizeof *rthdr;
        if (rtmask & (1 << RADIOTAP_TSFT)) {
            idx += 8;
        }
        if (idx >= len || (buf[idx] & RADIOTAP_F_FCS_FAILED)) {
            return 0;
        }
    }
    t_len = len - rtlen;
    return &buf[rtlen];
}

#define DIR_SEQ_PARA_SET_ELE_ID 3
#define HT_INFO_ELE_ID 61
struct _HT_INFORMATION_ELE{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    unsigned char	ControlChl;

    unsigned char	SrvIntGranularity:3;
    unsigned char	PSMPAccessOnly:1;
    unsigned char	RIFS:1;
    unsigned char	RecommemdedTxWidth:1;
    unsigned char	ExtChlOffset:2;

    unsigned char	Revd2:8;
    unsigned char	Revd1:5;
    unsigned char	NonGFDevPresent:1;
    unsigned char	OptMode:2;

    unsigned char	DualCTSProtect:1;
    unsigned char	DualBeacon:1;
    unsigned char	Rsvd3:6;

    unsigned char	Rsvd4:4;
    unsigned char	PcoPhase:1;
    unsigned char	PcoActive:1;
    unsigned char	LSigTxopProtectFull:1;
    unsigned char	SecondaryBeacon:1;

    unsigned char	BasicMSC[16];
#else
    unsigned char	ControlChl;

    unsigned char	ExtChlOffset:2;
    unsigned char	RecommemdedTxWidth:1;
    unsigned char	RIFS:1;
    unsigned char	PSMPAccessOnly:1;
    unsigned char	SrvIntGranularity:3;

    unsigned char	OptMode:2;
    unsigned char	NonGFDevPresent:1;
    unsigned char	Revd1:5;
    unsigned char	Revd2:8;

    unsigned char	Rsvd3:6;
    unsigned char	DualBeacon:1;
    unsigned char	DualCTSProtect:1;

    unsigned char	SecondaryBeacon:1;
    unsigned char	LSigTxopProtectFull:1;
    unsigned char	PcoActive:1;
    unsigned char	PcoPhase:1;
    unsigned char	Rsvd4:4;

    unsigned char	BasicMSC[16];
#endif
};
static int airkiss_find_out_channel(unsigned char *buf_header, int buf_length, int *channel, int *channel_offset)
{
    int length, ret = -1;
    unsigned char ele_id;
    struct _HT_INFORMATION_ELE *ht_info_ele;

    /* start with first element*/
    while(buf_length > 2) /* element id and length */
    {
        ele_id = *buf_header;
        buf_header++;
        buf_length--;
        length = *buf_header;
        buf_header++;
        buf_length--;

        if(ele_id == DIR_SEQ_PARA_SET_ELE_ID)
        {
            if(buf_length >= length)
            {
                *channel = *buf_header;
                if((*channel > 0) && (*channel <= 14))
                {
                    ret = 0;
                }
            }
        }
        else if(ele_id == HT_INFO_ELE_ID)
        {
            if(buf_length >= length)
            {
                ht_info_ele = (struct _HT_INFORMATION_ELE *) buf_header;

                if(*channel != ht_info_ele->ControlChl) /* check ht info primary channel equal channel or not */
                {
                    ret = -1;
                }
                else
                {
                    *channel_offset = ht_info_ele->ExtChlOffset;
                    ret = 0;
                }
                break;
            }
        }

        if(buf_length >= length)
        {
            buf_header += length;
            buf_length -= length;
        }
    }
    return ret;
}
#define DATA_TYPE   0x08
#define TODS		0x01
#define FROMDS		0x02
static int airkiss_find_out_target_ap(unsigned char *buf_header, unsigned char *target_ap_addr, unsigned char *target_sta_addr)
{
    int ret = -1;

    if(*(buf_header + 1) & TODS)
    {
        /* ADDR1 is BSSID */
        memcpy(target_ap_addr, buf_header + 4, ETH_ALEN);
        /* ADDR2 is SA */
        memcpy(target_sta_addr, buf_header + 10, ETH_ALEN);

        ret = 0;
    }
    else if(*(buf_header + 1) & FROMDS)
    {
        /* ADDR2 is BSSID */
        memcpy(target_ap_addr, buf_header + 10, ETH_ALEN);
        /* ADDR3 is SA */
        memcpy(target_sta_addr, buf_header + 16, ETH_ALEN);

        ret = 0;
    }

    return ret;
}

static void airkiss_handler(struct mon_airkiss *monair)
{
    static uint8_t g_buffer[BUFSIZE];
    uint8_t *buf = g_buffer;
    int len, ret, ch = 0, offset = 0;
    unsigned char *buf_header;
    bool stop = false, skip;
    unsigned char target_ap_addr[ETH_ALEN] = {0};
    unsigned char target_sta_addr[ETH_ALEN] = {0};

    while(!stop) {

        /* Receive the next packet */
        len = recv(monair->sockfd, g_buffer, sizeof g_buffer, 0);

        if(len < 0) {
                continue;
        }

        buf_header = decap_radiotap(buf, len);

        /* mac header size */
        if(buf_header && (t_len > 24))
        {
            pthread_mutex_lock(&(monair->lock));

            if(monair->is_ap_lock && !monair->is_chan_lock)
            {
                /* mac header size + mgmt header size */
                if(t_len > 36)
                {
                    /* filter beacon and probe response */
                    if((buf_header[0] == 0x80) || (buf_header[0] == 0x50))
                    {
                        /* find match ap addr */
                        if(!memcmp(target_ap_addr, &buf_header[10], ETH_ALEN))
                        {
                            /* parse and find primary channel, second channel */
                            if(0 == airkiss_find_out_channel(&buf_header[36], t_len - 36, &ch, &offset))
                            {
                                cprintf("!!!!!LOCK AT CHAN:%d OFFSET:%d!!!!!\n", ch, offset);
                                monair->primary_channel = ch;
                                monair->second_channel_offset = offset;
                                monair->stay_channel_tick = 0;
                                monair->lock_ap_tick = 0;
#if defined(AIRKISS_SEND_DELBA)
                                memcpy(delba_target_ap_addr, target_ap_addr, ETH_ALEN);
                                memcpy(delba_target_sta_addr, target_sta_addr, ETH_ALEN);
#endif
                                monair->is_chan_lock = true;
                            }
                        }
                    }
                }
            }
            else
            {
                /* if target and channel was locked, filter target frame only */
                if(monair->is_chan_lock)
                {
                    skip = true;
                    if(buf_header[0] & DATA_TYPE)
                    {
                        if(buf_header[1] & TODS)
                        {
                            if(!memcmp(target_sta_addr, &buf_header[10], ETH_ALEN))
                            {
#if defined(AIRKISS_SEND_DELBA)
                                if(memcmp(delba_target_ap_addr, &buf_header[4], ETH_ALEN))
                                {
                                    memcpy(delba_target_ap_addr, &buf_header[4], ETH_ALEN);
                                    cprintf("!!!!!CHANGE DELBA ADDR TO %02x:%02x:%02x:%02x:%02x:%02x!!!!!\n",
                                            delba_target_ap_addr[0], delba_target_ap_addr[1], delba_target_ap_addr[2],
                                            delba_target_ap_addr[3], delba_target_ap_addr[4], delba_target_ap_addr[5]);
                                }
#endif
                                skip = false;
                            }
                        }
                        else if(buf_header[1] & FROMDS)
                        {
                            if(!memcmp(target_ap_addr, &buf_header[10], ETH_ALEN))
                            {
                                skip = false;
                            }
                        }
                    }
                }
                else
                {
                    skip = false;
                }
                if(!skip)
                {
                    ret = airkiss_recv(akcontex, (void *) buf_header, t_len);
                }
                else
                {
                    ret = AIRKISS_STATUS_CONTINUE;
                }
                if (ret == AIRKISS_STATUS_CHANNEL_LOCKED)
                {
                    /* parsing and find ap addr */
                    if(0 == airkiss_find_out_target_ap(buf_header, target_ap_addr, target_sta_addr))
                    {
                        cprintf("!!!!!LOCK AP ADDR %02x:%02x:%02x:%02x:%02x:%02x!!!!!\n",
                                target_ap_addr[0], target_ap_addr[1], target_ap_addr[2], target_ap_addr[3], target_ap_addr[4], target_ap_addr[5]);
                        cprintf("!!!!!LOCK STA ADDR %02x:%02x:%02x:%02x:%02x:%02x!!!!!\n",
                                target_sta_addr[0], target_sta_addr[1], target_sta_addr[2], target_sta_addr[3], target_sta_addr[4], target_sta_addr[5]);
                        monair->stay_channel_tick = 3;
                        monair->lock_ap_tick = 30;
                        monair->is_ap_lock = true;
                    }
                }
                else if(ret == AIRKISS_STATUS_COMPLETE)
                {
                    cprintf("\n!!!!!!!SUCCESS!!!!!!!\n");
                    close(monair->sockfd);
                    monair->sockfd = 0;
                    airkiss_finish(monair);
                    stop = true;
                }
            }
            pthread_mutex_unlock(&(monair->lock));
        }
    }
}

static void airkiss_rcv_thread(void *ptr)
{
#if 0
    char errbuf[PCAP_ERRBUF_SIZE];
    struct mon_airkiss *monair = (struct mon_airkiss *) ptr;
    int ret;
#endif

    airkiss_handler(ptr);

#if 0
    monair->handle = pcap_open_live(MONITOR_DEVICE, BUFSIZ, 1, 0, errbuf);
    if (monair->handle == NULL)
    {
        cprintf("Couldn't open device %s: %s\n", MONITOR_DEVICE, errbuf);
        return;
    }

    if ((ret =
         pcap_loop(monair->handle, -1, dump_recv,
                   (unsigned char *) monair)) < 0)
    {
        if (ret == -1)
        {
            cprintf("%s", pcap_geterr(monair->handle));
            return;
        }
    }
#endif

    pthread_exit(NULL);
}

static void airkiss_apply(void)
{
    int sockfd = -1;
    struct sockaddr_in broadcastAddr;
    int numbytes;
    int broadcast = 1;
    struct ifreq ifr;
    char buf[4];
    char val;
    int i;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        goto done;
    }

    memset(&ifr, 0, sizeof ifr);
    snprintf(ifr.ifr_name, sizeof (ifr.ifr_name), VERIFY_DEVICE);

    if (setsockopt
        (sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *) &ifr, sizeof (ifr)) < 0)
    {
        perror("setsockopt (SO_BINDTODEVICE)");
        goto done;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
                   sizeof broadcast) == -1)
    {
        perror("setsockopt (SO_BROADCAST)");
        goto done;
    }

    memset(&broadcastAddr, 0, sizeof broadcastAddr);
    broadcastAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "255.255.255.255", &broadcastAddr.sin_addr);
    broadcastAddr.sin_port = htons(AIRKISSPORT);

    cdb_get("$airkiss_val", &buf);
    val = atoi(buf);

    for (i = 0; i < AIRKISSTRYCOUNT; i++)
    {
        if ((numbytes = sendto(sockfd, &val, 1, 0,
                               (struct sockaddr *) &broadcastAddr,
                               sizeof (broadcastAddr))) == -1)
        {
            perror("sendto");
            goto done;
        }
    }
done:
    if (sockfd != -1)
        close(sockfd);
    cdb_set_int("$airkiss_state", AIRKISS_STANDBY);
}

static char *airkiss_state(unsigned int state)
{
    switch (state)
    {
        case AIRKISS_STANDBY:
            return "StandBy";
        case AIRKISS_ONGOING:
            return "Ongoing";
        case AIRKISS_DONE:
            return "SSID/PASS Got. Connecting...";
        case AIRKISS_VERIFIED:
            return "Connected";
        case AIRKISS_TIMEOUT:
            return "AirKiss Timeout";

    }
    return NULL;
}

static int open_monitor(const char *ifname)
{
    struct sockaddr_ll ll;
    int sock;

    memset(&ll, 0, sizeof ll);
    ll.sll_family = AF_PACKET;
    ll.sll_ifindex = if_nametoindex(ifname);

    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(sock, (struct sockaddr*)&ll, sizeof ll) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    return sock;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    struct itimerspec new_value;
    struct timespec now;
    int fd;
    uint64_t exp;
    struct mon_airkiss *monair = NULL;

    if (argc < 2)
        goto done;

    if (!strcmp("apply", argv[1]))
    {
        airkiss_apply();
        cdb_set_int("$airkiss_state", AIRKISS_VERIFIED);
        exec_cmd("cdb commit");
        goto done;
    }
    else if (!strcmp("state", argv[1]))
    {
        unsigned int state;
        state = cdb_get_int("$airkiss_state", AIRKISS_STANDBY);
        printf("AirKiss: %s\n", airkiss_state(state));
        goto done;
    }
    else if (!strcmp("start", argv[1]))
    {
        int debug = 0;
        unsigned int state;

        state = cdb_get_int("$airkiss_state", AIRKISS_STANDBY);
        if (state == AIRKISS_ONGOING)
        {
            cprintf("AirKiss Already Running\n");
            return 0;
        }
        cdb_set_int("$airkiss_state", AIRKISS_ONGOING);

        cprintf("AirKiss Version: %s\n", airkiss_version());
        cprintf("Montage AirKiss:%s\n", MONTAGE_AIRKISS_VERSION);

        if (argc == 3)
        {
            if (!strcmp("auto", argv[2]))
            {
                cdb_set("airkiss_auto", "1");
            }
        }
        if (argc > 2)
        {
            if (!strcmp("debug", argv[2]))
                debug = 1;
        }

        daemon(0, debug);

        if ((monair =
             (struct mon_airkiss *) malloc(sizeof (struct mon_airkiss))) ==
            NULL)
        {
            cprintf("Error init struct mon AirKiss.\n");
            goto done;
        }
        memset(monair, 0, sizeof (struct mon_airkiss));

        ret = start_airkiss(monair);
        if (ret)
        {
            cprintf("AirKiss Start fail\n");
            goto monfree;
        }

        if ((pthread_mutex_init(&(monair->lock), NULL) != 0))
        {
            cprintf("Init mutex lock fail\n");
            goto monfree;
        }

        mon_srv_stop();
        wifi_set_channel(monair, true);
        wifi_promiscuous_enable(true);

        monair->sockfd = open_monitor(AK_MONITOR);

        /*Recv Thread */
        if (pthread_create
            ((pthread_t *) & (monair->rcv_thread_id), NULL,
             (void *) &airkiss_rcv_thread, monair) != 0)
        {
            cprintf("Queue Thread Create Fail.\n");
            ret = 0;
            goto lockfree;
        }

        /*Timer to change channel */
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
        {
            cprintf("clock_gettime");
            goto lockfree;
        }

        new_value.it_value.tv_sec = now.tv_sec + 1;
        new_value.it_value.tv_nsec = now.tv_nsec;

        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 300000000;

        fd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (fd == -1)
        {
            cprintf("timerfd create fail");
            goto lockfree;
        }

        if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
        {
            cprintf("timerfd set fail");
            goto lockfree;
        }

        stop_time = time(NULL) + AIRKISSSTOPTIME;
        while (!monair->is_airkiss_complete)
        {
            read(fd, &exp, sizeof (uint64_t));
            wifi_set_channel(monair, false);
            if (time(NULL) > stop_time)
            {
                cdb_set_int("$airkiss_state", AIRKISS_TIMEOUT);
                //airkiss_pcap_break(monair);
                cdb_set("$op_work_mode", "1");
                exec_cmd("mtc commit");
                cprintf
                    ("\033[41;32mAirKiss Timeout, To AP MODE\033[0m\n");
                break;
            }
        }
        close(fd);
        if(!monair->is_airkiss_complete)
        {
            wifi_promiscuous_enable(false);
        }
    }

lockfree:
    if (monair)
        pthread_mutex_destroy(&(monair->lock));
monfree:
    if (monair)
        free(monair);
done:
    return 0;
}
