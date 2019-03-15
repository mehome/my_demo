#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <libutils/wlutils.h>

#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_proc.h"

/*
# Working FILE
#    HOSTAPD=1       # for wlan script.      hostapd bin should use
#    WPASUP=1        # for wlan script.      wpa_supplicant bin should use
#    AP=$AP          # for wlan script.      WiFi AP interface
#    STA=$STA        # for wlan script.      WiFi STA interface
#    IFPLUGD         # for if script.        monitor interface of wan
#    MODE            # for if script.        current wan mode
#    OP              # for if script.        current work mode
#                                            0: debug mode
#                                            1: AP router(disable nat/dhcp)
#                                            2: Wifi router
#                                            3: WISP mode
#                                            4: Repeater mode(disable nat/dhcp)
#                                            5: Bridge mode(disable nat/dhcp)
#                                            6: Smart WAN
#                                            7: Mobile 3G
#                                            8: P2P
#                                            9: Pure Client mode
*/

#define PROCETH "/proc/eth"
#define PROCWLA "/proc/wla"

#define VIDW     "2"
#define VIDL     "3"
#define INF_ETH0 "eth0"
#define INF_ETHW INF_ETH0"."VIDW
#define INF_ETHL INF_ETH0"."VIDL
#define INF_AP   "wlan0"
#define INF_STA  "sta0"
#define INF_BR0  "br0"
#define INF_BR1  "br1"

#define IFPLUGD_NAME             "ifplugd"
#define IFPLUGD_PROC             "/usr/bin/"IFPLUGD_NAME
#define IFPLUGD_ETHL_PIDFILE     "/var/run/"IFPLUGD_NAME"."INF_ETHL".pid"
#define IFPLUGD_ETHW_PIDFILE     "/var/run/"IFPLUGD_NAME"."INF_ETHW".pid"

extern MtdmData *mtdm;

static int stop_br_probe = 0;

static void do_mac_init(void)
{
    unsigned char val[9];
    char wan_clone_mac[MBUF_LEN];
    int wan_clone_mac_enable = cdb_get_int("$wan_clone_mac_enable", 0);

    if ((wan_clone_mac_enable == 1) && 
        (cdb_get_str("$wan_clone_mac", wan_clone_mac, sizeof(wan_clone_mac), NULL) != NULL)) {
        my_mac2val(val, wan_clone_mac);
        my_mac2val(val+3, mtdm->boot.MAC0);
        my_val2mac(mtdm->rule.LMAC, val);
        sprintf(mtdm->rule.WMAC, "%s", wan_clone_mac);
        if (mtdm->cdb.op_work_mode == 3)
            sprintf(mtdm->rule.STAMAC, "%s", wan_clone_mac);
        else {
            my_mac2val(val+3, mtdm->boot.MAC2);
            my_val2mac(mtdm->rule.STAMAC, val);
        }
    }
    else {
        sprintf(mtdm->rule.LMAC, "%s", mtdm->boot.MAC0);
        sprintf(mtdm->rule.WMAC, "%s", mtdm->boot.MAC1);
        sprintf(mtdm->rule.STAMAC, "%s", mtdm->boot.MAC2);
    }
}

static void do_lan_mac(void)
{
        set_mac(INF_BR0, mtdm->rule.LMAC);
        set_mac(INF_ETH0, mtdm->rule.LMAC);
        set_mac(INF_AP, mtdm->rule.LMAC);
}

static void do_forward_mode(void)
{
    cdb_set_int("$wl_forward_mode", cdb_get_int("$wl_def_forward_mode", 0));
}

static void do_eth_reload(int yes)
{
    f_write_string(PROCETH, "sclk ", 0, 0);
    if (yes == 1) {
        f_write_string(PROCETH, "8021q 0003000b ", 0, 0);
        f_write_string(PROCETH, "8021q 00028100 ", 0, 0);
        //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
        f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
        f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
        f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
        f_write_string(PROCETH, "wmode 0 ", 0, 0);
        f_write_string(PROCETH, "swctl 1 ", 0, 0);
    }
    else if (yes == 0) {
        f_write_string(PROCETH, "8021q 0003000e ", 0, 0);
        f_write_string(PROCETH, "8021q 00028105 ", 0, 0);
        f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
        f_write_string(PROCETH, "epvid 00020000 ", 0, 0);
        f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
        //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
        f_write_string(PROCETH, "wmode 0 ", 0, 0);
        f_write_string(PROCETH, "swctl 1 ", 0, 0);
    }
}

static void do_br_probe(int sData)
{
    char st[BUF_LEN_16] = { 0 };
    char br[BUF_LEN_16] = { 0 };

    if ((mtdm->rule.OMODE == opMode_db) && !stop_br_probe) {
        f_read_string("/sys/kernel/debug/eth/br_state", st, sizeof(st));
        f_read_string("/sys/kernel/debug/eth/br", br, sizeof(br));
        if ((st[0] == '0') && (br[0] == '1')) {
             do_eth_reload(1);
        }
        if ((st[0] == '1') && (br[0] == '0')) {
             do_eth_reload(0);
        }
        if ((br[0] == '0') || (br[0] == '1')) {
            f_write("/sys/kernel/debug/eth/br_state", br, 1, FW_NEWLINE, 0);
        }

        TimerMod(timerMtdmBrProbe, 5);
    }
}

static void do_db_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);

    // Vlan interface
	if(!iface_exist(INF_ETHL))
	    exec_cmd2("vconfig add eth0 %s", VIDL);
	if(!iface_exist(INF_ETHW))
		exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Config mac address
    do_lan_mac();

    // Config mac address
    set_mac(INF_ETHW, mtdm->rule.WMAC);

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
    ifconfig(INF_ETHW, IFUP, NULL, NULL);
    ifconfig(INF_BR0, IFUP, NULL, NULL);

    // Generate rule
    strcpy(mtdm->rule.LAN, INF_BR0);
    strcpy(mtdm->rule.WAN, "");
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, INF_AP);
    strcpy(mtdm->rule.BRAP, INF_BR0);
    strcpy(mtdm->rule.IFPLUGD, INF_ETHW);
    mtdm->rule.HOSTAPD = 1;
    mtdm->rule.WPASUP = 0;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    do_eth_reload(0);

    f_write_string(PROCETH, "mdio 0 0 3100 ", 0, 0);
    stop_br_probe = 0;
    TimerAdd(timerMtdmBrProbe, do_br_probe, 0, 0);
}

static void do_ap_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);

    // Vlan interface
	if(!iface_exist(INF_ETHL))
		exec_cmd2("vconfig add eth0 %s", VIDL);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Config mac address
    do_lan_mac();

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
    ifconfig(INF_BR0, IFUP, NULL, NULL);

    // Generate rule
    strcpy(mtdm->rule.LAN, INF_BR0);
    strcpy(mtdm->rule.WAN, "");
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, INF_AP);
    strcpy(mtdm->rule.BRAP, INF_BR0);
    strcpy(mtdm->rule.IFPLUGD, "");
    mtdm->rule.HOSTAPD = 1;
    mtdm->rule.WPASUP = 0;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    f_write_string(PROCETH, "sclk ", 0, 0);
#if defined(CONFIG_P0_AS_LAN)
    f_write_string(PROCETH, "8021q 00030005 ", 0, 0);
    f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
#else
    f_write_string(PROCETH, "8021q 00030006 ", 0, 0);
    f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
#endif
    f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
    f_write_sprintf(PROCETH, 0, 0, "wmode %d ", mtdm->cdb.op_work_mode);
    f_write_string(PROCETH, "swctl 1 ", 0, 0);
}

static void do_wr_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);
    exec_cmd2("brctl addbr %s -NO_FLOOD_MC -FLOOD_UNKNOW_UC", INF_BR1);

    // Vlan interface
	if(!iface_exist(INF_ETHL))
		exec_cmd2("vconfig add eth0 %s", VIDL);
	if(!iface_exist(INF_ETHW))
		exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Add ether iface in br1
    exec_cmd2("brctl addif %s %s", INF_BR1, INF_ETHW);

    // Config mac address
    do_lan_mac();

    // Config mac address
    set_mac(INF_BR1, mtdm->rule.WMAC);
    set_mac(INF_ETHW, mtdm->rule.WMAC);

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
    ifconfig(INF_ETHW, IFUP, NULL, NULL);
    ifconfig(INF_BR0, IFUP, NULL, NULL);
    ifconfig(INF_BR1, IFUP, NULL, NULL);

    // Generate rule
    strcpy(mtdm->rule.LAN, INF_BR0);
    strcpy(mtdm->rule.WAN, "");
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, INF_AP);
    strcpy(mtdm->rule.BRAP, INF_BR0);
    strcpy(mtdm->rule.IFPLUGD, INF_ETHW);
    mtdm->rule.HOSTAPD = 1;
    mtdm->rule.WPASUP = 0;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    f_write_string(PROCETH, "sclk ", 0, 0);
#if defined(CONFIG_P0_AS_LAN)
    f_write_string(PROCETH, "8021q 00030005 ", 0, 0);
    f_write_string(PROCETH, "8021q 00028106 ", 0, 0);
    f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
    f_write_string(PROCETH, "epvid 00020100 ", 0, 0);
#else
    f_write_string(PROCETH, "8021q 00030006 ", 0, 0);
    f_write_string(PROCETH, "8021q 00028105 ", 0, 0);
    f_write_string(PROCETH, "epvid 00020000 ", 0, 0);
    f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
#endif
    f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
    f_write_sprintf(PROCETH, 0, 0, "wmode %d ", mtdm->cdb.op_work_mode);
    f_write_string(PROCETH, "swctl 1 ", 0, 0);
}

static void do_wi_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);
    exec_cmd2("brctl addbr %s -NO_FLOOD_MC -FLOOD_UNKNOW_UC", INF_BR1);
#if defined(__PANTHER__)
    // set forwarding delay to zero
    exec_cmd2("brctl setfd %s 0", INF_BR1);
#endif

    // Vlan interface
	if(!iface_exist(INF_ETHL))
		exec_cmd2("vconfig add eth0 %s", VIDL);
	if(!iface_exist(INF_ETHW))
		exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Add ether iface in br1
    exec_cmd2("brctl addif %s %s", INF_BR1, INF_ETHW);

#ifdef CONFIG_PACKAGE_omnicfg
    if (!is_directmode()) {

    // Config mac address
    do_lan_mac();

    }
#else
    // Config mac address
    do_lan_mac();
#endif

    // Config mac address
    set_mac(INF_BR1, mtdm->rule.STAMAC);
    set_mac(INF_ETHW, mtdm->rule.STAMAC);

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
    ifconfig(INF_ETHW, IFUP, NULL, NULL);
    ifconfig(INF_BR0, IFUP, NULL, NULL);
    ifconfig(INF_BR1, IFUP, NULL, NULL);

    // Generate rule
    strcpy(mtdm->rule.LAN, INF_BR0);
    strcpy(mtdm->rule.WAN, "");
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, INF_AP);
    strcpy(mtdm->rule.STA, INF_STA);
    strcpy(mtdm->rule.BRAP, INF_BR0);
    strcpy(mtdm->rule.BRSTA, INF_BR1);
    strcpy(mtdm->rule.IFPLUGD, INF_STA);
    mtdm->rule.HOSTAPD = 1;
    mtdm->rule.WPASUP = 1;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    f_write_string(PROCETH, "sclk ", 0, 0);
    f_write_string(PROCETH, "8021q 0002810c ", 0, 0);
#if defined(CONFIG_P0_AS_LAN)
    f_write_string(PROCETH, "8021q 0003000d ", 0, 0);
    f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
#else
    f_write_string(PROCETH, "8021q 0003000e ", 0, 0);
    f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
#endif
    f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00000100 ", 0, 0);
    f_write_sprintf(PROCETH, 0, 0, "wmode %d ", mtdm->cdb.op_work_mode);
    f_write_string(PROCETH, "swctl 1 ", 0, 0);
}

static void do_br_mode(void)
{
    // SW Forwarding mode
    cdb_set_int("$wl_forward_mode", 1);

    cdb_set_int("$wl_bss_role2", 256);

    // Bridge
    exec_cmd2("brctl addbr %s -FLOOD_UNKNOW_UC", INF_BR0);

    // Vlan interface
	if(!iface_exist(INF_ETHL))
		exec_cmd2("vconfig add eth0 %s", VIDL);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Config mac address
    do_lan_mac();

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
    ifconfig(INF_BR0, IFUP, NULL, NULL);

    // Config AP unit same as STA
    f_write_string(PROCWLA, "config.apid=1 ", 0, 0);
    f_write_string(PROCWLA, "config.staid=1 ", 0, 0);

    // Generate rule
    strcpy(mtdm->rule.LAN, INF_BR0);
    strcpy(mtdm->rule.WAN, INF_BR0);
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, INF_AP);
    strcpy(mtdm->rule.STA, INF_STA);
    strcpy(mtdm->rule.BRAP, INF_BR0);
    strcpy(mtdm->rule.BRSTA, "");
    strcpy(mtdm->rule.IFPLUGD, INF_STA);
    mtdm->rule.HOSTAPD = 1;
    mtdm->rule.WPASUP = 1;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    f_write_string(PROCETH, "sclk ", 0, 0);
#if defined(CONFIG_P0_AS_LAN)
    f_write_string(PROCETH, "8021q 0003000d ", 0, 0);
    f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
#else
    f_write_string(PROCETH, "8021q 0003000e ", 0, 0);
    f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
#endif
    f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
    f_write_sprintf(PROCETH, 0, 0, "wmode %d ", mtdm->cdb.op_work_mode);
    f_write_string(PROCETH, "swctl 1 ", 0, 0);
}

static void do_smart_mode(void)
{
    if ((mtdm->cdb.wan_mode == wanMode_dhcp) || (mtdm->cdb.wan_mode == wanMode_pppoe)) {
        do_wr_mode();
    }
    else if (mtdm->cdb.wan_mode == wanMode_3g) {
        do_ap_mode();
    }
}

static void do_mb_mode(void)
{
    do_ap_mode();
}

static void do_p2p_mode(void)
{
    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);
    exec_cmd2("brctl addbr %s -NO_FLOOD_MC -FLOOD_UNKNOW_UC", INF_BR1);

    // Vlan interface
	if(!iface_exist(INF_ETHL))
		exec_cmd2("vconfig add eth0 %s", VIDL);
	if(!iface_exist(INF_ETHW))
		exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Add ether iface in br1
    exec_cmd2("brctl addif %s %s", INF_BR1, INF_ETHW);

    // Config mac address
    do_lan_mac();

    // Config mac address
    set_mac(INF_BR1, mtdm->rule.LMAC);
    set_mac(INF_ETHW, mtdm->rule.LMAC);

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
    ifconfig(INF_ETHW, IFUP, NULL, NULL);
    ifconfig(INF_BR0, IFUP, NULL, NULL);
    ifconfig(INF_BR1, IFUP, NULL, NULL);

    // Generate rule
    strcpy(mtdm->rule.LAN, INF_BR0);
    strcpy(mtdm->rule.WAN, "");
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, "");
    strcpy(mtdm->rule.STA, INF_AP);
    strcpy(mtdm->rule.BRAP, INF_BR0);
    strcpy(mtdm->rule.BRSTA, INF_BR1);
    strcpy(mtdm->rule.IFPLUGD, INF_BR1);
    mtdm->rule.HOSTAPD = 0;
    mtdm->rule.WPASUP = 1;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    f_write_string(PROCETH, "sclk ", 0, 0);
    f_write_string(PROCETH, "8021q 0002810c ", 0, 0);
#if defined(CONFIG_P0_AS_LAN)
    f_write_string(PROCETH, "8021q 0003000d ", 0, 0);
    f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
#else
    f_write_string(PROCETH, "8021q 0003000e ", 0, 0);
    f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
#endif
    f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00020100 ", 0, 0);
    f_write_sprintf(PROCETH, 0, 0, "wmode %d ", mtdm->cdb.op_work_mode);
    f_write_string(PROCETH, "swctl 1 ", 0, 0);
}

#ifndef SPEEDUP_WLAN_CONF
static void do_client_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    exec_cmd2("brctl addbr %s", INF_BR0);
#endif
    exec_cmd2("brctl addbr %s -NO_FLOOD_MC -FLOOD_UNKNOW_UC", INF_BR1);

    // Vlan interface
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
	if(!iface_exist(INF_ETHL))
		exec_cmd2("vconfig add eth0 %s", VIDL);
#endif
	if(!iface_exist(INF_ETHW))
		exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);
#endif
    exec_cmd2("brctl addif %s %s", INF_BR1, INF_ETHW);

#ifdef CONFIG_PACKAGE_omnicfg
    if (!is_directmode()) {

    // Config mac address
    do_lan_mac();

    }
#else
    // Config mac address
    do_lan_mac();
#endif

    // Config mac address
    set_mac(INF_BR1, mtdm->rule.STAMAC);
    set_mac(INF_ETHW, mtdm->rule.STAMAC);

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
#endif
    ifconfig(INF_ETHW, IFUP, NULL, NULL);
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    ifconfig(INF_BR0, IFUP, NULL, NULL);
#endif
    ifconfig(INF_BR1, IFUP, NULL, NULL);

    // Generate rule
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    strcpy(mtdm->rule.LAN, INF_BR0);
#else
    strcpy(mtdm->rule.LAN, "");
#endif
    strcpy(mtdm->rule.WAN, "");
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, "");
    strcpy(mtdm->rule.STA, INF_STA);
    strcpy(mtdm->rule.BRAP, "");
    strcpy(mtdm->rule.BRSTA, INF_BR1);
    strcpy(mtdm->rule.IFPLUGD, INF_STA);
    mtdm->rule.HOSTAPD = 0;
    mtdm->rule.WPASUP = 1;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    f_write_string(PROCETH, "sclk ", 0, 0);
    f_write_string(PROCETH, "8021q 0002810c ", 0, 0);
#if defined(CONFIG_P0_AS_LAN)
    f_write_string(PROCETH, "8021q 0003000d ", 0, 0);
    f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
#else
    f_write_string(PROCETH, "8021q 0003000e ", 0, 0);
    f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
#endif
    f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00000100 ", 0, 0);
    f_write_sprintf(PROCETH, 0, 0, "wmode %d ", mtdm->cdb.op_work_mode);
    f_write_string(PROCETH, "swctl 1 ", 0, 0);
}
#endif

#ifndef SPEEDUP_WLAN_CONF
static void do_smartcfg_mode(void)
{
    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);

    // Vlan interface
	if(!iface_exist(INF_ETHL))
		exec_cmd2("vconfig add eth0 %s", VIDL);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Config mac address
    do_lan_mac();

    // Bring up eth interface to do calibration
    ifconfig(INF_ETH0, IFUP, NULL, NULL);
    ifconfig(INF_ETHL, IFUP, NULL, NULL);
    ifconfig(INF_BR0, IFUP, NULL, NULL);

    f_write_string(PROCWLA, "config.smart_config=1 ", 0, 0);

    // Generate rule
    strcpy(mtdm->rule.LAN, "");
    strcpy(mtdm->rule.WAN, "");
    strcpy(mtdm->rule.WANB, "");
    strcpy(mtdm->rule.AP, "");
    strcpy(mtdm->rule.STA, INF_STA);
    strcpy(mtdm->rule.BRAP, "");
    strcpy(mtdm->rule.BRSTA, "");
    strcpy(mtdm->rule.IFPLUGD, INF_STA);
    mtdm->rule.HOSTAPD = 0;
    mtdm->rule.WPASUP = 0;
    mtdm->rule.OMODE = mtdm->cdb.op_work_mode;
    mtdm->rule.WMODE = mtdm->cdb.wan_mode;

    f_write_string(PROCETH, "sclk ", 0, 0);
#if defined(CONFIG_P0_AS_LAN)
    f_write_string(PROCETH, "8021q 0003000d ", 0, 0);
    f_write_string(PROCETH, "epvid 00030000 ", 0, 0);
#else
    f_write_string(PROCETH, "8021q 0003000e ", 0, 0);
    f_write_string(PROCETH, "epvid 00030100 ", 0, 0);
#endif
    f_write_string(PROCETH, "epvid 00030200 ", 0, 0);
    //f_write_string(PROCETH, "bssid 00030000 ", 0, 0);
    f_write_sprintf(PROCETH, 0, 0, "wmode %d ", mtdm->cdb.op_work_mode);
    f_write_string(PROCETH, "swctl 1 ", 0, 0);
}
#endif

static int start(void)
{
    #ifndef SPEEDUP_WLAN_CONF
    int omod, wmod;
    char usbif_find_eth[SSBUF_LEN] = {0};
    #endif
    mtdm->cdb.op_work_mode = cdb_get_int("$op_work_mode", opMode_ap);
    mtdm->cdb.wan_mode = cdb_get_int("$wan_mode", wanMode_dhcp);

    #ifndef SPEEDUP_WLAN_CONF
    omod = mtdm->cdb.op_work_mode;
    wmod = mtdm->cdb.wan_mode;
    #endif
    do_mac_init();

    if (cdb_get_int("$smrt_enable", 0) == 1)
        mtdm->cdb.op_work_mode = opMode_smartcfg;

    MTDM_LOGGER(this, LOG_INFO, "LMAC  =%s", mtdm->rule.LMAC);
    MTDM_LOGGER(this, LOG_INFO, "WMAC  =%s", mtdm->rule.WMAC);
    MTDM_LOGGER(this, LOG_INFO, "STAMAC=%s", mtdm->rule.STAMAC);

    switch(mtdm->cdb.op_work_mode) {
        case opMode_db:
            do_db_mode();
            break;
        case opMode_ap:
            #ifdef SPEEDUP_WLAN_CONF
            do_wi_mode();
            #else
            do_ap_mode();
            #endif
            break;
        case opMode_wr:
            do_wr_mode();
            break;
        case opMode_wi:
            do_wi_mode();
            break;
        case opMode_rp:
#ifdef CONFIG_PACKAGE_omnicfg
            if (is_directmode()) {
                // due to br0 must be removed, then add with new flag again, but direct mode can't remove br0
                MTDM_LOGGER(this, LOG_INFO, "workmode(%d) can't support omnicfg in direct mode!", opMode_rp);
            }
#endif
            do_br_mode();
            break;
        case opMode_br:
            do_br_mode();
            break;
        case opMode_smart:
            do_smart_mode();
            break;
        case opMode_mb:
            do_mb_mode();
            break;
        case opMode_p2p:
            do_p2p_mode();
            break;
        case opMode_client:
            #ifdef SPEEDUP_WLAN_CONF
            do_wi_mode();
            #else
            do_client_mode();
            #endif
            break;
        case opMode_smartcfg:
            #ifndef SPEEDUP_WLAN_CONF
            do_smartcfg_mode();
            #endif
            break;
        default:
            break;
    }

    if (cdb_get_int("$smrt_enable", 0) == 1)
        mtdm->cdb.op_work_mode = opMode_smartcfg;

    #ifdef SPEEDUP_WLAN_CONF
#if defined(__PANTHER__)
    if (opMode_db == mtdm->cdb.op_work_mode) {
        strcpy(mtdm->rule.WAN, INF_ETHW);
        strcpy(mtdm->rule.WANB, INF_ETHW);
    }
    else {
        strcpy(mtdm->rule.WAN, INF_BR1);
        strcpy(mtdm->rule.WANB, INF_BR1);
    }
#else
    strcpy(mtdm->rule.WAN, "br1");
    strcpy(mtdm->rule.WANB, "br1");
#endif
    #else
    if(wmod == 1 || wmod == 2) {
       if(omod == 0 || omod == 2 || omod == 6) {
          strcpy(mtdm->rule.WAN, INF_ETHW);
          strcpy(mtdm->rule.WANB, INF_ETHW);
       }
       else if(omod == 4 || omod == 5) {
          strcpy(mtdm->rule.WAN, "br0");
          strcpy(mtdm->rule.WANB, "br0");
       }
       else if(omod == 3 || omod == 9) {
          strcpy(mtdm->rule.WAN, "br1");
          strcpy(mtdm->rule.WANB, "br1");
       }
       else if(omod == 1) {
          strcpy(mtdm->rule.WAN, "");
          strcpy(mtdm->rule.WANB, "");
       }
    }
    else if(wmod == 3 || wmod == 4 || wmod == 5 || wmod == 7 || wmod == 8) {
       if(omod == 0 || omod == 2 || omod == 6) {
          strcpy(mtdm->rule.WAN, "ppp0");
          strcpy(mtdm->rule.WANB, INF_ETHW);
       }
       else if(omod == 3) {
          strcpy(mtdm->rule.WAN, "ppp0");
          strcpy(mtdm->rule.WANB, "br1");
       }
       else {
          strcpy(mtdm->rule.WAN, "ppp0");
          strcpy(mtdm->rule.WANB, "ppp0");
       }
    }
    else if(wmod == 9) {
       if(cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL) != NULL) {
          strcpy(mtdm->rule.WAN, usbif_find_eth);
          strcpy(mtdm->rule.WANB, usbif_find_eth);
       }
       else {
          strcpy(mtdm->rule.WAN, "ppp0");
          strcpy(mtdm->rule.WANB, "ppp0");
       }
    }

    if(omod == 1) {
       if(wmod == 9) {
          if(cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL) != NULL) {
             strcpy(mtdm->rule.WAN, usbif_find_eth);
             strcpy(mtdm->rule.WANB, usbif_find_eth);
          }
          else {
             strcpy(mtdm->rule.WAN, "ppp0");
             strcpy(mtdm->rule.WANB, "ppp0");
          }
       }
       else {
          strcpy(mtdm->rule.WAN, "");
          strcpy(mtdm->rule.WANB, "");
       }
    }
    #endif

    MTDM_LOGGER(this, LOG_INFO, "workmode start: attach ifplugd op(%d)", mtdm->cdb.op_work_mode);

    return 0;
}

static int stop(void)
{
    f_write_string(PROCETH, "swctl 0 ", 0, 0);
    stop_br_probe = 1;
    if (mtdm->rule.OMODE == opMode_wi) {
        exec_cmd2("brctl delif %s %s", INF_BR1, INF_ETHW);
    ifconfig(INF_BR1, 0, NULL, NULL);
        exec_cmd2("brctl delbr %s", INF_BR1);
    }

#ifdef CONFIG_PACKAGE_omnicfg
    if (!is_directmode()) {

    brctl_delif(INF_BR0, INF_ETHL);
    ifconfig(INF_BR0, 0, NULL, NULL);
    brctl_delbr(INF_BR0);

    }
#else
    brctl_delif(INF_BR0, INF_ETHL);
    ifconfig(INF_BR0, 0, NULL, NULL);
    brctl_delbr(INF_BR0);
#endif

    return 0;
}

static int try_wifi_ap_list(void)
{
    char wl_aplist_info[MSBUF_LEN] = { 0 };
    char wl_aplist_pass[MSBUF_LEN] = { 0 };
    char *argv[10];
    char *bssid, *sec, *cipher, *last, *mode, *ssid, *tc;
    char *pass = wl_aplist_pass;
    int num;
    int omode;
    int otype;
    int ocipher;
    int ret = 0;

    MTDM_LOGGER(this, LOG_INFO, "%s: auto run aplist feature!", __func__);

    if (cdb_get_array_str("$wl_aplist_info", 1, wl_aplist_info, sizeof(wl_aplist_info), NULL) == NULL) {
        MTDM_LOGGER(this, LOG_INFO, "%s: skip, aplist is empty", __func__);
        ret = -1;
    }
    else if (strlen(wl_aplist_info) == 0) {
        MTDM_LOGGER(this, LOG_INFO, "%s: skip, aplist is empty", __func__);
        ret = -1;
    }
    else {
        cdb_get_array_str("$wl_aplist_pass", 1, wl_aplist_pass, sizeof(wl_aplist_pass), "");
        MTDM_LOGGER(this, LOG_INFO, "%s: wl_aplist_info1 is [%s]", __func__, wl_aplist_info);
        MTDM_LOGGER(this, LOG_INFO, "%s: wl_aplist_pass1 is [%s]", __func__, wl_aplist_pass);

        num = str2argv(wl_aplist_info, argv, '&');
        if((num < 7)               || !str_arg(argv, "bssid=")  || 
           !str_arg(argv, "sec=")  || !str_arg(argv, "cipher=") || 
           !str_arg(argv, "last=") || !str_arg(argv, "mode=")   || 
           !str_arg(argv, "tc=")   || !str_arg(argv, "ssid=")) {
            MTDM_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
            ret = -1;
        }
        else {
            bssid  = str_arg(argv, "bssid=");
            sec    = str_arg(argv, "sec=");
            cipher = str_arg(argv, "cipher=");
            last   = str_arg(argv, "last=");
            mode   = str_arg(argv, "mode=");
            ssid   = str_arg(argv, "ssid=");
            tc     = str_arg(argv, "tc=");
            if (sscanf(mode, "%d", &omode) != 1) {
                MTDM_LOGGER(this, LOG_INFO, "%s: skip, mode is not digtal", __func__);
                ret = -1;
            }
            if (strlen(ssid) == 0) {
                MTDM_LOGGER(this, LOG_INFO, "%s: skip, ssid is empty", __func__);
                ret = -1;
            }
            if (sscanf(tc, "%d:%d", &otype, &ocipher) != 2) {
                MTDM_LOGGER(this, LOG_INFO, "%s: skip, tc is not digtal", __func__);
                ret = -1;
            }

            if (ret == 0) {
                MTDM_LOGGER(this, LOG_INFO, "%s: bssid  is [%s]", __func__, bssid);
                MTDM_LOGGER(this, LOG_INFO, "%s: sec    is [%s]", __func__, sec);
                MTDM_LOGGER(this, LOG_INFO, "%s: cipher is [%s]", __func__, cipher);
                MTDM_LOGGER(this, LOG_INFO, "%s: last   is [%s]", __func__, last);
                MTDM_LOGGER(this, LOG_INFO, "%s: mode   is [%s]", __func__, mode);
                MTDM_LOGGER(this, LOG_INFO, "%s: tc     is [%s]", __func__, tc);
                MTDM_LOGGER(this, LOG_INFO, "%s: ssid   is [%s]", __func__, ssid);
                
                cdb_set_int("$op_work_mode", omode);

                cdb_set_int("$wl_bss_cipher2", ocipher);
                cdb_set_int("$wl_bss_sec_type2", otype);
                if (otype & 4) {
                    cdb_set("$wl_bss_wep_1key2", pass);
                    cdb_set("$wl_bss_wep_2key2", pass);
                    cdb_set("$wl_bss_wep_3key2", pass);
                    cdb_set("$wl_bss_wep_4key2", pass);
                }
                else {
                    cdb_set("$wl_bss_wpa_psk2", pass);
                }
                cdb_set("$wl_bss_ssid2", ssid);
            }
        }
    }

    return ret;
}

static int boot(void)
{
    if (cdb_get_int("$wl_aplist_en", 0) == 1) {
        if (try_wifi_ap_list() < 0) {
            cdb_set_int("$op_work_mode", opMode_ap);
        }
    }

    return start();
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  boot  },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int workmode_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

