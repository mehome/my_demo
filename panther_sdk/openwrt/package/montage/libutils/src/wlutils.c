/*
 * =====================================================================================
 *
 *       Filename:  wlutils.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/16/2016 10:36:05 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/
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
#include <wdk/cdb.h>
#include <unistd.h>
#include <errno.h>

#include "nl80211.h"
#include "wlutils.h"

unsigned int channels[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472}; 

struct wifi_scan_result *scan_result;
unsigned int found;

static int cipher_suite_to_bit(const __u8 * s)
{
    __u32 suite = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];

    if (suite == RSN_CIPHER_SUITE_TKIP)
        return 1 << AUTH_CIPHER_TKIP;
    if (suite == RSN_CIPHER_SUITE_CCMP)
        return 1 << AUTH_CIPHER_CCMP;
    if (suite == WPA_CIPHER_SUITE_TKIP)
        return 1 << AUTH_CIPHER_TKIP;
    if (suite == WPA_CIPHER_SUITE_CCMP)
        return 1 << AUTH_CIPHER_CCMP;

    return 0;
}

static int wlan_parse_wpa_rsn_ie(const __u8 * start, __u32 len, __u32 * pcipher,
                                 __u32 * gcipher)
{
    __u8 *pos;
    __u32 i, count, left;

    pos = (__u8 *) start + 2;
    left = len - 2;

    if (left < SELECTOR_LEN)
        return -1;

    /* get group cipher suite */
    *gcipher |= cipher_suite_to_bit(pos);

    pos += SELECTOR_LEN;
    left -= SELECTOR_LEN;

    /* get pairwise cipher suite list */
    count = (pos[1] << 8) | pos[0];
    pos += 2;
    left -= 2;

    if (count == 0 || left < count * SELECTOR_LEN)
        return -1;

    for (i = 0; i < count; i++)
    {
        *pcipher |= cipher_suite_to_bit(pos);

        pos += SELECTOR_LEN;
        left -= SELECTOR_LEN;
    }

    return 0;
}

static void print_ies(FILE * fd, unsigned char *ie, int ielen, __u16 capa,
                      struct bss_info *bssinfo)
{
    unsigned char *sec;
    int sec_num = 0;
    unsigned int pcipher = 0;
    unsigned int gcipher = 0;
    char ssid_name[33] = { 0 };
    char band = 0;
    unsigned char ch = 0;

    while (ielen >= 2 && ielen >= ie[1])
    {
        if (fd)
        {
            if (ie[0] == 0)
            {
                int ssid_len = ie[1] > 32 ? 32 : ie[1];
                memcpy(ssid_name, (char *) ie + 2, ssid_len);
                ssid_name[ssid_len] = '\0';
            }
            else if (ie[0] == 3)
            {                   //CHANNEL
                ch = ie[2];
            }
            else if (ie[0] == 45)
            {
                band |= 4;
            }
        }

        if (ie[0] == 48)        //WPA2
        {
            sec_num |= 4;
            wlan_parse_wpa_rsn_ie(ie + 2, ielen - 2, &pcipher, &gcipher);
        }
        else if (ie[0] == 68)
        {                       //WAPI
            sec_num |= 16;
            pcipher |= 1 << AUTH_CIPHER_SMS4;
            gcipher |= 1 << AUTH_CIPHER_SMS4;
        }
        else if (ie[0] == 221)
        {                       //Vendor
            /* Microsoft OUI */
            if ((ie[2] == 0x00) && (ie[3] == 0x50) && (ie[4] == 0xf2))
            {
                sec = ie + 5;
                if (sec[0] == 0x4)
                {               // WPS
                    sec_num |= 8;
                }
                else if (sec[0] == 0x1)
                {               // WPA
                    sec_num |= 2;
                    wlan_parse_wpa_rsn_ie(ie + 6, ielen - 6, &pcipher,
                                          &gcipher);
                }
            }
        }
        ielen -= ie[1] + 2;
        ie += ie[1] + 2;
    }

    if ((capa & WLAN_CAPABILITY_PRIVACY) && ((sec_num & 6) == 0))
    {
        sec_num |= 1;
    }

    if (fd)
    {
        fprintf(fd,
                "bssid=%s&ssid=%s&ch=%d&sig=%d&band=%d&sec=%d&pcipher=%d&gcipher=%d\n",
                bssinfo->mac_addr, ssid_name, ch, bssinfo->signal, band,
                sec_num, pcipher, gcipher);
    }

    if (sec_num)
    {
        scan_result = malloc(sizeof (struct wifi_scan_result));
        if (!scan_result)
        {
            goto done;
        }

        scan_result->sec = sec_num;
        scan_result->pcipher = pcipher;
        scan_result->gcipher = gcipher;
    }

done:
    return;
}

void mac_addr_n2a(char *mac, unsigned char *arg)
{
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
            arg[0] & 0xff, arg[1] & 0xff, arg[2] & 0xff,
            arg[3] & 0xff, arg[4] & 0xff, arg[5] & 0xff);
}

static int callback_trigger(struct nl_msg *msg, void *arg)
{
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct scan_params *params = (struct scan_params *) arg;

    if (gnlh->cmd == NL80211_CMD_SCAN_ABORTED)
    {
        printf("Got NL80211_CMD_SCAN_ABORTED.\n");
        params->done = 1;
        params->aborted = 1;
    }
    else if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS)
    {
        printf("Got NL80211_CMD_NEW_SCAN_RESULTS.\n");
        params->done = 1;
        params->aborted = 0;
    }
    return NL_SKIP;
}

static int callback_dump(struct nl_msg *msg, void *arg)
{
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *bss[NL80211_BSS_MAX + 1];
    static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
        [NL80211_BSS_BSSID] = {},
        [NL80211_BSS_SIGNAL_MBM] = {.type = NLA_U32},
        [NL80211_BSS_SIGNAL_UNSPEC] = {.type = NLA_U8},
        [NL80211_BSS_CAPABILITY] = {.type = NLA_U16},
        [NL80211_BSS_INFORMATION_ELEMENTS] = {},
        [NL80211_BSS_BEACON_IES] = {},
    };
    unsigned char *ie = NULL;

    struct scan_params *params = (struct scan_params *) arg;
    FILE *fd = params->fd;

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[NL80211_ATTR_BSS])
        return NL_SKIP;

    if (nla_parse_nested(bss, NL80211_BSS_MAX,
                         tb[NL80211_ATTR_BSS], bss_policy))
        return NL_SKIP;

    if (fd)
    {
        if (!bss[NL80211_BSS_BSSID])
            return NL_SKIP;
    }

    __u16 capa = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);

    if (!fd)
    {
        if (found == SCAN_AP_FOUND)
            return NL_SKIP;
        ie = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);

        if (!ie || (strlen(params->ssid) != ie[1]))
            return NL_SKIP;

        if (!strncmp(params->ssid, (char *) (ie + 2), ie[1]))
        {

            printf("\033[41;32mFind AP [%s]\033[0m\n", params->ssid);

            print_ies(NULL, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
                      nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]), capa,
                      NULL);
            found = SCAN_AP_FOUND;
        }
    }
    else
    {
        struct bss_info bssinfo;
        memset(&bssinfo, 0, sizeof (bssinfo));

        mac_addr_n2a(bssinfo.mac_addr, nla_data(bss[NL80211_BSS_BSSID]));

        if (bss[NL80211_BSS_SIGNAL_MBM])
        {
            bssinfo.signal = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);
            bssinfo.signal /= 100;
        }

        print_ies(fd, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
                  nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]), capa,
                  &bssinfo);
    }

    return NL_SKIP;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                         void *arg)
{
    int *ret = arg;
    *ret = err->error;
    return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
    int *ret = arg;
    *ret = 0;
    return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
    int *ret = arg;
    *ret = 0;
    return NL_STOP;
}

static int family_handler(struct nl_msg *msg, void *arg)
{
    struct handler_args *grp = arg;
    struct nlattr *tb[CTRL_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *mcgrp;
    int rem_mcgrp;

    nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[CTRL_ATTR_MCAST_GROUPS])
        return NL_SKIP;

    nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp)
    {
        struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

        nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
                  nla_data(mcgrp), nla_len(mcgrp), NULL);

        if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
            !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
            continue;
        if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
                    grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
            continue;
        grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
        break;
    }

    return NL_SKIP;
}

static int nl_get_multicast_id(struct nl_sock *sock, const char *family,
                               const char *group)
{
    struct nl_msg *msg = NULL;
    struct nl_cb *cb = NULL;
    int ret, ctrlid;
    struct handler_args grp = {
        .group = group,
        .id = -ENOENT,
    };

    msg = nlmsg_alloc();
    if (!msg)
        return -ENOMEM;

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb)
    {
        ret = -ENOMEM;
        goto fail;
    }

    ctrlid = genl_ctrl_resolve(sock, "nlctrl");

    genlmsg_put(msg, 0, 0, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);

    ret = -ENOBUFS;
    nla_put(msg, CTRL_ATTR_FAMILY_NAME, strlen(family) + 1, family);

    ret = nl_send_auto_complete(sock, msg);
    if (ret < 0)
        goto fail;

    ret = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

    while (ret > 0)
        nl_recvmsgs(sock, cb);

    if (ret == 0)
        ret = grp.id;
fail:
    if (msg)
        nlmsg_free(msg);
    if (cb)
        nl_cb_put(cb);
    return ret;
}

static int nl80211_init(struct nl80211_state *state)
{
    int err;

    state->nl_sock = nl_socket_alloc();
    if (!state->nl_sock)
    {
        printf("Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    if (genl_connect(state->nl_sock))
    {
        printf("Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
    if (state->nl80211_id < 0)
    {
        printf("nl80211 not found.\n");
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

out_handle_destroy:
    nl_socket_free(state->nl_sock);
    return err;
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

static int iface_isup(const char *name)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof (struct ifreq));

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFFLAGS, &ifr);

    close(fd);

    return (ifr.ifr_flags & IFF_UP);
}

static void scan_iface_updown(bool up)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof (struct ifreq));

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, SCAN_DEVICE, IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFFLAGS, &ifr);

    if (up)
        ifr.ifr_flags |= IFF_UP;
    else
        ifr.ifr_flags &= ~IFF_UP;

    ioctl(fd, SIOCSIFFLAGS, &ifr);

    close(fd);
}

static void scan_start(char *ssid, unsigned int freq)
{
    unsigned int devidx = 0;
    struct nl_msg *msg = NULL;
    struct nl_cb *cb = NULL;
    struct nl_msg *scan_ssids = NULL;
    struct nl_msg *freqs = NULL;
    struct nl80211_state nlstate;
    int err = -1;
    struct scan_params scan_params;
    int ret;
    int mcid;
    static bool flushed = false;

    /* get devide index of "wlan0" */
    devidx = if_nametoindex(SCAN_DEVICE);
    if (devidx == 0)
    {
        printf("cannot get %s index\n", SCAN_DEVICE);
        devidx = -1;
        return;
    }

    /* init nl80211 */
    err = nl80211_init(&nlstate);
    if (err)
    {
        printf("nl80211 init fail\n");
        return;
    }

    mcid = nl_get_multicast_id(nlstate.nl_sock, "nl80211", "scan");
    nl_socket_add_membership(nlstate.nl_sock, mcid);

    /* allocate netlink message */
    msg = nlmsg_alloc();
    if (!msg)
    {
        printf("failed to allocate netlink message\n");
        goto fail;
    }

    scan_ssids = nlmsg_alloc();
    if (!scan_ssids)
    {
        fprintf(stderr, "failed to allocate netlink scan ssid\n");
        goto fail;
    }

    if (ssid)
        nla_put(scan_ssids, 1, strlen(ssid), ssid);
    else
        nla_put(scan_ssids, 1, 0, "");

    genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, 0,
                NL80211_CMD_TRIGGER_SCAN, 0);
    nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, scan_ssids);

    nla_put_u32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_STATION);

    if (freq != 0)
    {
        freqs = nlmsg_alloc();
        if (!freqs)
        {
            fprintf(stderr, "failed to allocate \
							netlink scan frequency\n");
            goto fail;
        }
        nla_put_u32(freqs, 1, freq);
        nla_put_nested(msg, NL80211_ATTR_SCAN_FREQUENCIES, freqs);
    }

    if (ssid)
        nla_put_u32(msg, NL80211_ATTR_SCAN_FLAGS, NL80211_SCAN_FLAG_FLUSH);
    else if (!flushed)
    {
        nla_put_u32(msg, NL80211_ATTR_SCAN_FLAGS, NL80211_SCAN_FLAG_FLUSH);
        flushed = true;
    }

    /* put data in netlink message */
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, devidx);

    /* allocate netlink callbacks */
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb)
    {
        printf("failed to allocate netlink callbacks\n");
        goto fail;
    }

    memset(&scan_params, 0, sizeof (scan_params));
    scan_params.ssid = ssid;
    /* set netlink callback */
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_trigger,
              (void *) &scan_params);

    /* Set up an error callback */
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);

    /* Send netlink message */
    err = 1;
    ret = nl_send_auto_complete(nlstate.nl_sock, msg);
    if (ret < 0)
    {
        printf("Send netlink message fail\n");
        goto fail;
    }

    /* Receive a set of messages from a netlink socket */
    while (err > 0)
    {
        ret = nl_recvmsgs(nlstate.nl_sock, cb);
    }

    if (ret < 0)
    {
        printf("ERROR: nl_recvmsgs() returned %d (%s).\n", ret,
               nl_geterror(-ret));
        goto fail;
    }

    while (!scan_params.done)
        nl_recvmsgs(nlstate.nl_sock, cb);
    if (scan_params.aborted)
    {
        printf("ERROR: Kernel aborted scan.\n");
        goto fail;
    }
fail:
    if (msg)
        nlmsg_free(msg);
    if (cb)
        nl_cb_put(cb);
    if (scan_ssids)
        nlmsg_free(scan_ssids);
    if (freqs)
        nlmsg_free(freqs);
    nl_socket_drop_memberships(nlstate.nl_sock, mcid);
    nl_socket_free(nlstate.nl_sock);
    return;
}

static void scan_dump(char *ssid, bool result)
{
    struct nl_msg *msg = NULL;
    struct nl_cb *cb = NULL;
    struct nl80211_state nlstate;
    unsigned int devidx = 0;
    int err = -1;
    FILE *fd = NULL;
    struct scan_params scan_params;
    int ret;

    /* get devide index of "wlan0" */
    devidx = if_nametoindex(SCAN_DEVICE);

    if (devidx == 0)
    {
        printf("cannot find wlan0\n");
        devidx = -1;
    }

    /* init nl80211 */
    err = nl80211_init(&nlstate);
    if (err)
    {
        printf("nl80211 init fail\n");
        return;
    }

    /* allocate netlink message */
    msg = nlmsg_alloc();
    if (!msg)
    {
        printf("failed to allocate netlink message\n");
        goto fail;
    }

    genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, NLM_F_DUMP,
                NL80211_CMD_GET_SCAN, 0);

    /* put data in netlink message */
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, devidx);

    /* allocate netlink callbacks */
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb)
    {
        printf("failed to allocate netlink callbacks\n");
        goto fail;
    }

    memset(&scan_params, 0, sizeof (scan_params));
    if (result)
    {
        fd = fopen(SCANRESULT, "w");
        if (!fd)
            goto fail;
        scan_params.fd = fd;
    }
    scan_params.ssid = ssid;

    /* set netlink callback */
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_dump,
              (void *) &scan_params);

    /* Set up an error callback */
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    /* Send netlink message */
    err = 1;
    ret = nl_send_auto_complete(nlstate.nl_sock, msg);
    if (ret < 0)
    {
        printf("Send netlink message fail\n");
        goto fail;
    }

    /* Receive a set of messages from a netlink socket */
    while (err > 0)
    {
        ret = nl_recvmsgs(nlstate.nl_sock, cb);
    }

fail:
    if (msg)
        nlmsg_free(msg);
    if (cb)
        nl_cb_put(cb);
    if (fd)
        fclose(fd);
    nl_socket_free(nlstate.nl_sock);
    return;
}

static void nl80211_scan_wifi_start(char *ssid, unsigned int freq)
{
    scan_start(ssid, freq);
}

static void nl80211_get_scan_result(char *ssid, bool result)
{
    scan_dump(ssid, result);
}

unsigned int wifi_scan(void)
{
    int i;
    int should_down = 0;

    if (!iface_isup(SCAN_DEVICE))
    {
        scan_iface_updown(true);
        should_down = 1;
    }

    for (i = 0; i < ARRAY_SIZE(channels); i++)
    {
        nl80211_scan_wifi_start(NULL, channels[i]);
    }
    nl80211_get_scan_result(NULL, true);

    if (should_down)
        scan_iface_updown(false);

    return 0;
}

unsigned int wifi_set_ssid_pwd(char *ssid, char *pass)
{
    int i;
    int should_down = 0;

    if (strlen(ssid) > 32)
    {
        printf("critical error, ssid two long : %s\n", ssid);
        goto done;
    }

    if (pass)
    {
        if (strlen(pass) > 64)
        {
            printf("critical error, pass two long : pass:%s\n", pass);
            goto done;
        }
    }

    if (!iface_isup(SCAN_DEVICE))
    {
        scan_iface_updown(true);
        should_down = 1;
    }

    for (i = 0; i < ARRAY_SIZE(channels); i++)
    {
        /*Assign channel to scan */
        nl80211_scan_wifi_start(ssid, channels[i]);
        nl80211_get_scan_result(ssid, false);
        if (found)
            break;
    }

    if (should_down)
        scan_iface_updown(false);

    if (!found)
        goto done;
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
        /*for speed up: use new param*/
        cdb_set("$wl_passphase2", "");
        goto done;
    }

    if (!scan_result)
    {
        goto done;
    }
    int sec = scan_result->sec & 0x07;
    int pcipher = scan_result->pcipher;

    switch (sec)
    {
        case 0:
            if (found && pass)
                found = SCAN_AP_PASS_ERROR;
            goto done;
        case 1:
            if ((strlen(pass) == 5) || (strlen(pass) == 13) ||
                (strlen(pass) == 10) || (strlen(pass) == 26))
            {
                cdb_set("$wl_bss_wep_1key2", pass);
                cdb_set("$wl_bss_wep_2key2", pass);
                cdb_set("$wl_bss_wep_3key2", pass);
                cdb_set("$wl_bss_wep_4key2", pass);
                /*for speed up: use new param*/
                cdb_set("$wl_passphase2", pass);
            }
            else
            {
                printf("WEP length error!");
                if (found)
                    found = SCAN_AP_SEC_ERROR;
                goto done;
            }

            cdb_set("$wl_bss_sec_type2", "4");
            break;
        case 2:
            cdb_set("$wl_bss_sec_type2", "8");
            break;
        case 4:
            cdb_set("$wl_bss_sec_type2", "16");
            break;
        case (2 + 4):
            cdb_set("$wl_bss_sec_type2", "24");
            break;
        default:
            printf("unknow security type!!!!\n");
            if (found)
                found = SCAN_AP_SEC_ERROR;
            goto done;
    }

    /* WPA WPA2 WPA+WPA2 */
    if (sec >= 2)
    {
        if ((strlen(pass) < 8) || (strlen(pass) > 64))
        {
            printf("WiFi password length not ok!");
            if (found)
                found = SCAN_AP_PASS_ERROR;
            goto done;
        }
        cdb_set("$wl_bss_wpa_psk2", pass);
        /*for speed up: use new param*/
        cdb_set("$wl_passphase2", pass);

        switch (pcipher)
        {
            case 8:
                cdb_set("$wl_bss_cipher2", "4");
                break;
            case 16:
                cdb_set("$wl_bss_cipher2", "8");
                break;
            case (8 + 16):
                cdb_set("$wl_bss_cipher2", "12");
                break;
            default:
                printf("unknow cipher!!!!\n");
                if (found)
                    found = SCAN_AP_SEC_ERROR;
                goto done;
        }
    }

done:
    if (scan_result)
        free(scan_result);
    return found;
}
