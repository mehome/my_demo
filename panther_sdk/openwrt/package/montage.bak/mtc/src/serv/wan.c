/*=====================================================================================+
 | The definition of wan_mode                                                          |
 +=====================================================================================*/
/* ====================================
  1: static ip
  2: dhcp
  3: pppoe
  4: pptp
  5: l2tp
  6: bigpond (we don't use this now)
  7: pppoe2
  8: pptp2
  9: 3g
===================================== */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netutils.h>
#include <libcchip/function/wav_play/wav_play.h>

#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;
static int check_subnet(void);

static int max_alias = 5;
static char dns_svr1[SSBUF_LEN] = {0};
static char dns_svr2[SSBUF_LEN] = {0};

#define RESOLV_FILE     "/etc/resolv.conf"
#define RESOLV_TMP_FILE "/etc/resolv.conf.tmp"
#define RESOLV_PPP_FILE "/etc/ppp/resolv.conf"

#define DHCPD_BR0_PIDFILE "/var/run/dhcpcd-br0.pid"
#define DHCPD_BR1_PIDFILE "/var/run/dhcpcd-br1.pid"

static void store_netstatus(void)
{
    char info[SSBUF_LEN] = {0};
    char mac_str[30];
    char *ptr;
		FILE *fp = NULL;
    int tmp = 0;

    if (get_ifname_ether_ip(mtdm->rule.WAN, info, sizeof(info)) == EXIT_SUCCESS) {
        cdb_set("$wanif_ip", info);
    }
    if (get_ifname_ether_addr(mtdm->rule.WAN, (unsigned char *)info, sizeof(info)) == EXIT_SUCCESS) {
        cdb_set("$wanif_mac", ether_etoa((unsigned char *)info, mac_str));
    }
    if (get_ifname_ether_mask(mtdm->rule.WAN, info, sizeof(info)) == EXIT_SUCCESS) {
        cdb_set("$wanif_msk", info);
    }
    exec_cmd3(info, sizeof(info), "route -n | awk '/UG/{ print $2 }'");
    cdb_set("$wanif_gw", info);
    cdb_set_int("$wanif_conntime", uptime());

    fp = fopen(RESOLV_FILE, "r");
    if (fp) {
        while ((ptr = new_fgets(info, sizeof(info), fp)) != NULL) {
            if (!strncmp(ptr, "nameserver", sizeof("nameserver")-1)) {
                ptr += sizeof("nameserver");
                if (!tmp) {
                    cdb_set("$wanif_dns1", ptr);
                    tmp = 1;
                }
                else {
                    cdb_set("$wanif_dns2", ptr);
                    break;
                }
            }
        }
        fclose(fp);
    }

    if (get_ifname_ether_ip(mtdm->rule.LAN, info, sizeof(info)) == EXIT_SUCCESS) {
        cdb_set("$lanif_ip", info);
    }
    if (get_ifname_ether_addr(mtdm->rule.LAN, (unsigned char *)info, sizeof(info)) == EXIT_SUCCESS) {
        cdb_set("$lanif_mac", ether_etoa((unsigned char *)info, mac_str));
    }
}

static void clear_netstatus(void)
{
    unsigned char wanif_mac[ETHER_ADDR_LEN] = { 0 };
    unsigned char lanif_mac[ETHER_ADDR_LEN] = { 0 };
    char wanif_mac_str[30];
    char lanif_mac_str[30];
    int wanif_conntime = cdb_get_int("$wanif_conntime", 0);
    int wanif_conntimes = cdb_get_int("$wanif_conntimes", 0);
    wanif_conntimes = uptime() - wanif_conntime + wanif_conntimes;
    get_ifname_ether_addr(mtdm->rule.WAN, wanif_mac, sizeof(wanif_mac));
    get_ifname_ether_addr(mtdm->rule.LAN, lanif_mac, sizeof(lanif_mac));

    cdb_set("$wanif_mac", ether_etoa(wanif_mac, wanif_mac_str));
    cdb_set("$wanif_ip", "0.0.0.0");
    cdb_set("$wanif_msk", "0.0.0.0");
    cdb_set("$wanif_gw", "0.0.0.0");
    cdb_set("$wanif_domain", "");
    cdb_set_int("$wanif_conntime", 0);
    cdb_set_int("$wanif_conntimes", wanif_conntimes);
    cdb_set("$wanif_dns1", "0.0.0.0");
    cdb_set("$wanif_dns2", "0.0.0.0");
    cdb_set("$lanif_mac", ether_etoa(lanif_mac, lanif_mac_str));
}

#if defined(__PANTHER__)
#define WIFI_CONNECTED_SOUND_FILE "/tmp/res/connected.wav"
static void play_wifi_connected_sound(void)
{
    struct stat buf;
    if(stat(WIFI_CONNECTED_SOUND_FILE, &buf) == 0)
    {
        system("/usr/bin/wavplayer " WIFI_CONNECTED_SOUND_FILE " &");
    }
}
#endif

/*=====================================================================================+
 | Function : wan_serv ip up/down                                                      |
 +=====================================================================================*/
int mtdm_wan_ip_up(BootState boot, montage_proc_t *ptbl, int tblcnt)
{
    montage_proc_t module_main[] = {
    //{ "dns",    0,0,RESTART, dns_proc,   0 },
#ifdef CONFIG_FW
    { "fw",     0,0,RESTART, fw_proc,    0 },
#endif
    { "nat",    0,0,RESTART, nat_proc,   0 },
    { "route",  0,0,RESTART, route_proc, 0 },
    { "ddns",   0,0,RESTART, ddns_proc,  0 },
    { "upnp",   0,0,RESTART, upnp_proc,  0 },
    { "igmp",   0,0,RESTART, igmp_proc,  0 },
    { 0,        0,0,0,       0,          0 },
    };
    struct timeval stv;
    FILE *fp = NULL;
    char info[SSBUF_LEN] = {0};
    int ra_func = cdb_get_int("$ra_func", 0);
    int i, j;

    gettimeofbegin(&stv);
    logger (LOG_INFO, "ip_up:");

    /* added check_subnet for fake station mode */
    if ((mtdm->rule.OMODE == 0) || (mtdm->rule.OMODE == 3) || (mtdm->rule.OMODE == 9)) {
        if(check_subnet()) {
            wan_main(NULL,"stop");
            wan_main(NULL,"start");
            return 0;
        }
    }

    dns_proc(NULL, STOP);
    dns_proc(NULL, START);

    /* 
     * It may be caused our program blocked due to multi-thread and fork
     * Don't use ANY fork function excepting to our start_stop_daemon()
     * For examples: eval, eval_nowait, _evalpid, _backtick, and fork
     */

    //eval("mpg123", "-q", "http://fdfs.xmcdn.com/group15/M08/77/E1/wKgDZVYFP5SgidpRAABEV7sj23Q057.mp3");
    //start_stop_daemon("/usr/bin/mpg123", SSDF_START_BACKGROUND_DAEMON, NICE_DEFAULT, 0, NULL, 
    //    "-q http://fdfs.xmcdn.com/group15/M08/77/E1/wKgDZVYFP5SgidpRAABEV7sj23Q057.mp3");

#if defined(__PANTHER__)
    int caller_status = cdb_get_int("$network_caller", 0);
    if(caller_status == 0)
    {
        wav_play_preload("/tmp/res/welcome_back.wav");
        cdb_set_int("$network_caller", 1);
        if(f_exists("/usr/bin/turingIot"))   //如果没有iot模块就不再执行相应的动作
        {
            eval_nowait("killall","-9","turingIot");
            eval_nowait("turingIot");
        }
    }
    else if(caller_status == 1)
    {
        wav_play_preload("/tmp/res/connected.wav");
        if(f_exists("/usr/bin/turingIot"))
        {
            eval_nowait("killall","-9","turingIot");
            eval_nowait("turingIot");
        }
    }
    else if(caller_status == 2)
    {
        cdb_set_int("$network_caller", 1);  
    }
    //exec_cmd2("/usr/bin/uartdfifo.sh succeed");
#endif

    if(f_exists(RESOLV_PPP_FILE)) {
        switch(mtdm->rule.WMODE) {
            case 9:
                cdb_get_str("$usbif_find_eth", info, sizeof(info), "");
                if (strcmp(info, "")) {
                    break;
                }
            case 3:
            case 4:
            case 5:
                cp(RESOLV_PPP_FILE, RESOLV_FILE);
                break;
            default:
                break;
        }
    }

    if (cdb_get_int("$dns_fix", 0) == 1) {
        //cp(RESOLV_FILE, RESOLV_TMP_FILE);
        fp = fopen(RESOLV_FILE, "w");
        if (fp) {
            fprintf(fp, "# Generated by mtdm for interface %s\n", mtdm->rule.WAN);
            cdb_get_array_str("$dns_svr", 1, info, sizeof(info), "");
            if (strcmp(info, "")) {
                MTDM_LOGGER(this, LOG_INFO, "dns_svr1=[%s]", info);
                fprintf(fp, "nameserver %s\n", info);
            }
            cdb_get_array_str("$dns_svr", 2, info, sizeof(info), "");
            if (strcmp(info, "")) {
                MTDM_LOGGER(this, LOG_INFO, "dns_svr2=[%s]", info);
                fprintf(fp, "nameserver %s\n", info);
            }
            fprintf(fp, "options timeout:1\n");
            fprintf(fp, "options attempts:10\n");
            fclose(fp);
        }
        if (f_exists(RESOLV_TMP_FILE)) {
            exec_cmd2("cat "RESOLV_TMP_FILE" >> "RESOLV_FILE);
            unlink(RESOLV_TMP_FILE);
        }
    }
    else {
        cdb_get_str("$dns_def", info, sizeof(info), "");
        if (strcmp(info, "")) {
            MTDM_LOGGER(this, LOG_INFO, "dns_def=[%s]", info);
            fp = fopen(RESOLV_FILE, "a");
            if (fp) {
                fprintf(fp, "# Generated by mtdm for interface %s\n", mtdm->rule.WAN);
                fprintf(fp, "nameserver %s\n", info);
                fprintf(fp, "options timeout:1\n");
                fprintf(fp, "options attempts:10\n");
                fclose(fp);
            }
        }
    }

    store_netstatus();

    /* these following functions have to change from script to C API  */
    for (i=0; module_main[i].exec_proc; i++) {
    		
        if (module_main[i].flags & STOP) {
            module_main[i].exec_proc(NULL, STOP);
        }
        if (module_main[i].flags & START) {
            module_main[i].exec_proc(NULL, START);
        }
    }

    if(cdb_get_int("$airkiss_state", 0) == 2) {
        exec_cmd("airkiss apply");
    }

    if(ra_func == 2) {
        ra_proc(NULL, STOP);
        ra_proc(NULL, START);
    }
    /* fw */
#ifdef CONFIG_FW
    exec_cmd("conntrack -F");
#endif
    /* lib/wdk/omnicfg_apply IOS, fixed me!!!! */
    //["$FROM_IOS" == "1" -a -n "$REMOTE_ADDR"] && route add "$REMOTE_ADDR" br0

    /* mpc (PHASE 2) */
    if(f_exists("/usr/bin/mpc") && (ra_func == raFunc_INTERNET_RADIO)) {
        exec_cmd("mpc play");
    }

    if(mtdm->rule.OMODE == 4) {
        exec_cmd("wd mat 1");
    }
    else if(mtdm->rule.OMODE == 5) {
        exec_cmd("wd mat 1");
    }

    cdb_set_int("$wanif_state", Wanif_State_CONNECTED);

    logger (LOG_INFO, "ip_up ok: %ld", gettimevaldiff(&stv));

    if ((boot == bootState_ipup) && (ptbl != NULL)) {
        if((ra_func == raFunc_DLNA_AIRPLAY)) {
            for (j=0; j<tblcnt; j++) {
                if (!strcmp((ptbl+j)->mname, "ra")) {
                    (ptbl+j)->boot = 0;
                    break;
                }
            }
        }
        for (i=0; module_main[i].exec_proc; i++) {
            for (j=0; j<tblcnt; j++) {
                if (!strcmp((ptbl+j)->mname, module_main[i].mname)) {
                    (ptbl+j)->boot = 0;
                    break;
                }
            }
        }
    }



    /* system (PHASE 2) */
    if(f_exists("/lib/wdk/system"))
       exec_cmd("/lib/wdk/system ntp");



	char val[1] = {0};
	cdb_get("$bt_update_status",val);
	if (1 == atoi(val)){
		exec_cmd("/usr/bin/dueros_ota");
	}


	if(f_exists("/usr/bin/dueros_ota"))
		exec_cmd("/usr/bin/dueros_ota &");

    return 0;
}

int mtdm_wan_ip_down(void)
{
    logger (LOG_INFO, "ip_down:");
    cdb_set_int("$wanif_state", Wanif_State_DISCONNECTED);

    return 0;
}

static int clear_arp_perm(void) // stop() will use this function
{
    char cmd[MAX_COMMAND_LEN];
    void *myhdr = NULL;
    char *myptr = NULL;
    int myfree = 0, cnt = 10;

    sprintf(cmd, "arp -na | awk '/PERM on %s/{gsub(/[()]/,\"\",$2); print $2 }'", mtdm->rule.WAN);
    while (!exec_cmd4(&myhdr, &myptr, myfree, cmd) && (--cnt > 0) ) {
        if (myptr) {
            exec_cmd2("arp -d %s > /dev/null 2>&1", myptr);
        }
    }

    return 0;
}

static int set_arp_wan_bind(void)
{
    char gw[SSBUF_LEN] = {0};

    exec_cmd3(gw, sizeof(gw), "route -n | awk '/UG/{print $2}'");
    exec_cmd2("arp -s %s `arping %s -I %s -c 1 | grep \"Unicast reply\" | awk '{$print 5}' | sed -e s/\\\\\\[// -e s/\\\\\\]//`", gw, gw, mtdm->rule.WAN);
    return 0;
}

static void save_pppoe_file(void)
{
    char poe_user[SSBUF_LEN] = {0};
    char poe_pass[SSBUF_LEN] = {0};
    char poe_sip[SSBUF_LEN] = {0};
    char poe_svc[SSBUF_LEN] = {0};
    int poe_sipe;
    int poe_mtu;
    int poe_idle;
    int poe_auto;
    char string[LSBUF_LEN] = {0};
    char *str;
    FILE *fp;

    cdb_get_str("$poe_user", poe_user, sizeof(poe_user), NULL);
    cdb_get_str("$poe_pass", poe_pass, sizeof(poe_pass), NULL);
    poe_sipe = cdb_get_int("$poe_sipe", 0);
    cdb_get_str("$poe_sip", poe_sip, sizeof(poe_sip), NULL);
    cdb_get_str("$poe_svc", poe_svc, sizeof(poe_svc), NULL);
    poe_mtu = cdb_get_int("$poe_mtu", 1500);
    poe_idle = cdb_get_int("$poe_idle", 0);
    poe_auto = cdb_get_int("$poe_auto", 0);

    if(!f_isdir("/etc/ppp/peers")) {
        mkdir("/etc/ppp", 0755);
        mkdir("/etc/ppp/peers", 0755);
    }

    unlink("/etc/ppp/peers/ppp0");
    unlink("/etc/ppp/pap-secrets");
    unlink("/etc/ppp/chap-secrets");
    unlink("/etc/ppp/resolv.conf");

    /* manual_onoff is in phase 2, ignore */

    fp = fopen("/etc/ppp/chap-secrets", "w+");
    if(fp) {
       fprintf(fp, "#USERNAME  PROVIDER  PASSWORD  IPADDRESS\n");
       fprintf(fp, "\"%s\" * \"%s\" *\n", poe_user, poe_pass);
       fclose(fp);
    }
    cp("/etc/ppp/chap-secrets", "/etc/ppp/pap-secrets");

    str = string;
    str += sprintf(str, "plugin rp-pppoe.so\n");
   
    if(strcmp(cdb_get_str("$poe_svc", poe_svc, sizeof(poe_svc), NULL), "") != 0)
       str += sprintf(str, "rp_pppoe_service \"%s\"\n", poe_svc);

    str += sprintf(str, "eth0.2\n");
    str += sprintf(str, "noipdefault\n");
    str += sprintf(str, "ipcp-accept-local\n");
    str += sprintf(str, "ipcp-accept-remote\n");
    str += sprintf(str, "defaultroute\n");
    str += sprintf(str, "replacedefaultroute\n");
    str += sprintf(str, "hide-password\n");
    str += sprintf(str, "noauth\n");
    str += sprintf(str, "refuse-eap\n");
    str += sprintf(str, "lcp-echo-interval 30\n");
    str += sprintf(str, "lcp-echo-failure 4\n");

    if(cdb_get_int("$dns_fix", 0) == 0)
       str += sprintf(str, "usepeerdns\n");
    else {
       if(strcmp(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL), "") != 0)
          str += sprintf(str, "ms-dns %s", dns_svr1);
       if(strcmp(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL), "") != 0)
          str += sprintf(str, "ms-dns %s", dns_svr2);
    }

    str += sprintf(str, "user \"%s\"\n", poe_user);

    if(poe_sipe == 1)
       str += sprintf(str, "%s:\n", poe_sip);

    str += sprintf(str, "mtu %d\n", poe_mtu);
    //str += sprintf(str, "mtu 1492\n");

    if(poe_idle != 0) {
       switch(poe_auto) {
           case 0:
               str += sprintf(str, "persist\n");
               break;
           case 1:
               str += sprintf(str, "demand\n");
               break;
       }
       if(poe_auto != 0)
          str += sprintf(str, "idle %d\n", poe_idle);
    }
    else
       str += sprintf(str, "persist\n");

    fp = fopen("/etc/ppp/peers/ppp0", "w+");
    if(fp) {
       fprintf(fp, "%s", string);
       fclose(fp);
    }
}

static void save_pptp_file(void)
{
    char pptp_user[SSBUF_LEN] = {0};
    char pptp_pass[SSBUF_LEN] = {0};
    char pptp_svr[SSBUF_LEN] = {0};
    int pptp_mtu;
    int pptp_idle;
    int pptp_auto;
    int pptp_mppe;
    char string[LSBUF_LEN] = {0};
    char *str;
    FILE *fp;

    cdb_get_str("$pptp_user", pptp_user, sizeof(pptp_user), NULL);
    cdb_get_str("$pptp_pass", pptp_pass, sizeof(pptp_pass), NULL);
    cdb_get_str("$pptp_svr", pptp_svr, sizeof(pptp_svr), NULL);
    pptp_mtu = cdb_get_int("$pptp_mtu", 1500);
    pptp_idle = cdb_get_int("$pptp_idle", 0);
    pptp_auto = cdb_get_int("$pptp_auto", 0);
    pptp_mppe = cdb_get_int("$pptp_mppe", 0);

    if(!f_isdir("/etc/ppp/peers")) {
        mkdir("/etc/ppp", 0755);
        mkdir("/etc/ppp/peers", 0755);
    }

    unlink("/etc/ppp/peers/ppp0");
    unlink("/etc/ppp/chap-secrets");
    unlink("/etc/ppp/resolv.conf");

    /* manual_onoff is in phase 2, ignore */

    fp = fopen("/etc/ppp/chap-secrets", "w+");
    if(fp) {
       fprintf(fp, "#USERNAME  PROVIDER  PASSWORD  IPADDRESS\n");
       fprintf(fp, "\"%s\" * \"%s\" *\n", pptp_user, pptp_pass);
       fclose(fp);
    }

    str = string;
    str += sprintf(str, "pty \"pptp %s --nolaunchpppd\"\n", pptp_svr);
    str += sprintf(str, "noipdefault\n");
    str += sprintf(str, "ipcp-accept-local\n");
    str += sprintf(str, "ipcp-accept-remote\n");
    str += sprintf(str, "defaultroute\n");
    str += sprintf(str, "replacedefaultroute\n");
    str += sprintf(str, "hide-password\n");
    str += sprintf(str, "noauth\n");
    str += sprintf(str, "refuse-eap\n");
    str += sprintf(str, "lcp-echo-interval 30\n");
    str += sprintf(str, "lcp-echo-failure 4\n");

    if(cdb_get_int("$dns_fix", 0) == 0)
       str += sprintf(str, "usepeerdns\n");
    else {
       if(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL) != NULL)
          str += sprintf(str, "ms-dns %s\n", dns_svr1);
       if(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL) != NULL)
          str += sprintf(str, "ms-dns %s\n", dns_svr2);
    }

    if(pptp_mppe == 1) {
       str += sprintf(str, "mppe required\n");
       str += sprintf(str, "mppe stateless\n");
    }

    str += sprintf(str, "user \"%s\"\n", pptp_user);
    str += sprintf(str, "mtu %d\n", pptp_mtu);

    if(pptp_idle != 0) {
       switch(pptp_auto) {
          case 0:
              str += sprintf(str, "persist\n");
              break;
          case 1:
              str += sprintf(str, "demand\n");
              break;
       }
       if(pptp_auto != 0)
          str += sprintf(str, "idle %d\n", pptp_idle);
    }
    else
       str += sprintf(str, "persist\n");

    fp = fopen("/etc/ppp/peers/ppp0", "w+");
    if(fp) {
       fprintf(fp, "%s", string);
       fclose(fp);
    }
}

static void save_l2tp_file(void)
{
   char l2tp_user[SSBUF_LEN] = {0};
   char l2tp_pass[SSBUF_LEN] = {0};
   char l2tp_svr[SSBUF_LEN] = {0};
   int l2tp_mtu;
   int l2tp_idle;
   int l2tp_auto;
   int l2tp_mppe;
   char *str1, *str2;
   char string1[LSBUF_LEN] = {0};
   char string2[LSBUF_LEN] = {0};
   FILE *fp;

   cdb_get_str("$l2tp_user", l2tp_user, sizeof(l2tp_user), NULL);
   cdb_get_str("$l2tp_pass", l2tp_pass, sizeof(l2tp_pass), NULL);
   cdb_get_str("$l2tp_svr", l2tp_svr, sizeof(l2tp_svr), NULL);
   l2tp_mtu = cdb_get_int("$l2tp_mtu", 1500);
   l2tp_idle = cdb_get_int("$l2tp_idle", 0);
   l2tp_auto = cdb_get_int("$l2tp_auto", 0);
   l2tp_mppe = cdb_get_int("$l2tp_mppe", 0);

    if(!f_isdir("/etc/ppp/peers")) {
        mkdir("/etc/ppp", 0755);
        mkdir("/etc/ppp/peers", 0755);
    }

    unlink("/etc/l2tp.conf");
    unlink("/etc/ppp/options");
    unlink("/etc/ppp/chap-secrets");
    unlink("/etc/ppp/resolv.conf");

    /* manual_onoff is in phase 2, ignore */

    fp = fopen("/etc/ppp/chap-secrets", "w+");
    if(fp) {
       fprintf(fp, "#USERNAME  PROVIDER  PASSWORD  IPADDRESS\n");
       fprintf(fp, "\"%s\" * \"%s\" *\n", l2tp_user, l2tp_pass);
       fclose(fp);
    }

    str1 = string1;
    str1 += sprintf(str1, "global\n");
    str1 += sprintf(str1, "load-handler \"sync-pppd.so\"\n");
    str1 += sprintf(str1, "load-handler \"cmd.so\"\n");
    str1 += sprintf(str1, "listen-port 1701\n");
    str1 += sprintf(str1, "section sync-pppd\n");
    str1 += sprintf(str1, "lac-pppd-opts \"file /etc/ppp/options\"\n");
    str1 += sprintf(str1, "section peer\n");
    str1 += sprintf(str1, "peer %s\n", l2tp_svr);
    str1 += sprintf(str1, "port 1701\n");
    str1 += sprintf(str1, "lac-handler sync-pppd\n");
    str1 += sprintf(str1, "lns-handler sync-pppd\n");
    str1 += sprintf(str1, "hide-avps yes\n");
    str1 += sprintf(str1, "section cmd\n");

    fp = fopen("/etc/l2tp.conf", "w+");
    if(fp) {
       fprintf(fp, "%s", string1);
       fclose(fp);
    }

    str2 = string2;
    str2 += sprintf(str2, "noipdefault\n");
    str2 += sprintf(str2, "ipcp-accept-local\n");
    str2 += sprintf(str2, "ipcp-accept-remote\n");
    str2 += sprintf(str2, "defaultroute\n");
    str2 += sprintf(str2, "replacedefaultroute\n");
    str2 += sprintf(str2, "hide-password\n");
    str2 += sprintf(str2, "noauth\n");
    str2 += sprintf(str2, "refuse-eap\n");
    str2 += sprintf(str2, "lcp-echo-interval 30\n");
    str2 += sprintf(str2, "lcp-echo-failure 4\n");

    if(cdb_get_int("$dns_fix", 0) == 0)
       str2 += sprintf(str2, "usepeerdns\n");
    else {
       if(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL) != NULL)
          str2 += sprintf(str2, "ms-dns %s\n", dns_svr1);
       if(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL) != NULL)
          str2 += sprintf(str2, "ms-dns %s\n", dns_svr2);
    }

    if(l2tp_mppe == 1) {
       str2 += sprintf(str2, "mppe required\n");
       str2 += sprintf(str2, "mppe stateless\n");
    }

    str2 += sprintf(str2, "user \"%s\"\n", l2tp_user);
    str2 += sprintf(str2, "mtu %d\n", l2tp_mtu);

    if(l2tp_idle != 0) {
       switch(l2tp_auto) {
          case 0:
              str2 += sprintf(str2, "persist\n");
              break;
          case 1:
              str2 += sprintf(str2, "demand\n");
              break;
       }
       if(l2tp_auto != 0)
          str2 += sprintf(str2, "idle %d\n", l2tp_idle);
    }
    else
       str2 += sprintf(str2, "persist\n");

    fp = fopen("/etc/ppp/options", "w+");
    if(fp) {
       fprintf(fp, "%s", string2);
       fclose(fp);
    }
}

static void save_3gmodem_file(void)
{
    char wan_mobile_apn[SSBUF_LEN] = {0};
    char wan_mobile_dial[SSBUF_LEN] = {0};
    int wan_mobile_auto;
    int wan_mobile_idle;
    int wan_mobile_mtu;
    int wan_mobile_autopan;
    char wan_mobile_pass[SSBUF_LEN] = {0};
    char wan_mobile_pin[SSBUF_LEN] = {0};
    char wan_mobile_user[SSBUF_LEN] = {0};
    char usbif_modemtty[SSBUF_LEN] = {0};
    char usbif_telecom[SSBUF_LEN] = {0};
    char string[LSBUF_LEN] = {0};
    FILE *fp;
    char *str;
    char mcc_mnc[SSBUF_LEN] = {0};
    char line[SSBUF_LEN] = {0};
    char pinstatus[SSBUF_LEN] = {0};

    cdb_get_str("$wan_mobile_apn", wan_mobile_apn, sizeof(wan_mobile_apn), NULL);
    wan_mobile_auto = cdb_get_int("$wan_mobile_auto", 0);
    wan_mobile_autopan = cdb_get_int("$wan_mobile_autopan", 0);
    cdb_get_str("$wan_mobile_dial", wan_mobile_dial, sizeof(wan_mobile_dial), NULL);
    wan_mobile_idle = cdb_get_int("$wan_mobile_idle", 0);
    wan_mobile_mtu = cdb_get_int("$wan_mobile_mtu", 1500);
    cdb_get_str("$wan_mobile_pass", wan_mobile_pass, sizeof(wan_mobile_pass), NULL);
    cdb_get_str("$wan_mobile_pin", wan_mobile_pin, sizeof(wan_mobile_pin), NULL);
    cdb_get_str("$wan_mobile_user", wan_mobile_user, sizeof(wan_mobile_user), NULL);
    cdb_get_str("$usbif_modemtty", usbif_modemtty, sizeof(usbif_modemtty), NULL);
    cdb_get_str("$usbif_telecom", usbif_telecom, sizeof(usbif_telecom), NULL);

    if((strcmp(cdb_get_str("$usbif_modemtty", usbif_modemtty, sizeof(usbif_modemtty), NULL), "!ERR") == 0) || (cdb_get_str("$usbif_modemtty", usbif_modemtty, sizeof(usbif_modemtty), NULL) == NULL))
       return;

    if(!f_isdir("/etc/ppp/peers")) {
        mkdir("/etc/ppp", 0755);
        mkdir("/etc/ppp/peers", 0755);
    }

    unlink("/etc/ppp/peers/ppp0");
    unlink("/etc/ppp/pap-secrets");
    unlink("/etc/ppp/chap-secrets");

    /* manual_onoff is in phase 2, ignore */

    exec_cmd3(pinstatus, sizeof(pinstatus), "`/usr/bin/comgt -d /dev/%s -s /etc/gcom/getpin.conf`", usbif_modemtty);
    if((strcmp(pinstatus, "READY") != 0) && (strcmp(pinstatus, "SIN PIN") != 0)) {
        exec_cmd2("echo \"[3G modem]!!!!!Unsupported cases; PIN's error status is %s > /dev/kmsg", pinstatus);
        exec_cmd2("echo \"[3G modem]cmd:%s > /dev/kmsg", pinstatus);

        while(strcmp(pinstatus, "SIM busy") == 0) {
           f_write_string("/dev/kmsg", "[3G modem]get pinstatus again" , 0, 0);
            sleep(1);
        }

       f_wsprintf("/dev/kmsg", "[3G modem]pinstatus is %s" , pinstatus);

       if((strcmp(pinstatus, "READY") != 0) && (strcmp(pinstatus, "SIN PIN") != 0))
            return;
        }

    if(strcmp(pinstatus, "SIM PIN") == 0) {
        cdb_set_int("$wan_mobile_ispin", 1);
        if(cdb_get_str("$wan_mobile_pin", wan_mobile_pin, sizeof(wan_mobile_pin), NULL) != NULL) {
            exec_cmd2("export \"PINCODE=%s\"", wan_mobile_pin);
            exec_cmd2("/usr/bin/comgt -d /dev/%s -s /etc/gcom/setpin.gcom", usbif_modemtty);
            if(exec_cmd2("/usr/bin/comgt -d /dev/%s -s /etc/gcom/setpin.gcom", usbif_modemtty) == 1) {
             f_write_string("/dev/kmsg", "[3G modem]set pin failed" , 0, 0);
                return;
            }
        }
       else
            cdb_set_int("$wan_mobile_ispin", 0);
        }

    exec_cmd3(pinstatus, sizeof(pinstatus), "`/usr/bin/comgt -d /dev/%s -s /etc/gcom/getpin.conf`", usbif_modemtty);
    if(strcmp(pinstatus, "READY") != 0)
        return;

    exec_cmd3(mcc_mnc, sizeof(mcc_mnc), "/usr/bin/comgt -d /dev/%s -s  /etc/gcom/getmccmnc.gcom", usbif_modemtty);
    exec_cmd3(line, sizeof(line), "sed -n \"/^%s/p\" /etc/config/apnlist.lst", mcc_mnc); // grep string "mcc_mnc" from file /etc/config/apnlist.lst

    if((cdb_get_str("$usbif_telecom", usbif_telecom, sizeof(usbif_telecom), NULL) == NULL) || (strcmp(cdb_get_str("$usbif_telecom", usbif_telecom, sizeof(usbif_telecom), NULL), "!ERR") == 0)) {
        cdb_set("$usbif_telecom", (void *)exec_cmd2("echo %s | awk -F, '{ print $6}'", line));
    }

    if(wan_mobile_autopan == 1) {
        exec_cmd3(wan_mobile_dial, sizeof(wan_mobile_dial), "echo %s | awk -F, '{ print $2}'", line);
        exec_cmd3(wan_mobile_apn, sizeof(wan_mobile_apn), "echo %s | awk -F, '{ print $3}'", line);
        exec_cmd3(wan_mobile_user, sizeof(wan_mobile_user), "echo %s | awk -F, '{ print $4}'", line);
        exec_cmd3(wan_mobile_pass, sizeof(wan_mobile_pass), "echo %s | awk -F, '{ print $5}'", line);
    }

    fp = fopen("/etc/chatscripts/3g.chat", "w");
    if(fp) {
        fprintf(fp, "TIMEOUT 10\n");
        fprintf(fp, "ABORT BUSY\n");
        fprintf(fp, "ABORT ERROR");
        fprintf(fp, "REPORT CONNECT\n");
        fprintf(fp, "ABORT NO CARRIER\n");
        fprintf(fp, "ABORT VOICE\n");
        fprintf(fp, "ABORT \"NO DIALTONE\"\n");
        fprintf(fp, "\"\" 'at+cgdcont=1,\"ip\",\"%s\"'\n", wan_mobile_apn);
        fprintf(fp, "\"\" \"atd%s\"\n", wan_mobile_dial);
        fprintf(fp, "TIMEOUT 10\n");
        fprintf(fp, "CONNECT c\n");
        fclose(fp);
    }

    str = string;
    str += sprintf(str, "/dev/%s\n", usbif_modemtty);
    str += sprintf(str, "crtscts\n");
    str += sprintf(str, "noauth\n");
    str += sprintf(str, "defaultroute\n");
    str += sprintf(str, "noipdefault\n");
    str += sprintf(str, "nopcomp\n");
    str += sprintf(str, "nocacomp\n");
    str += sprintf(str, "novj\n");
    str += sprintf(str, "nobsdcomp\n");
    str += sprintf(str, "holdoff 10\n");
    str += sprintf(str, "nodeflate\n");

    if(cdb_get_int("$dns_fix", 0) == 0)
        str += sprintf(str, "usepeerdns\n");
    else {
       if(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL) != NULL)
            str += sprintf(str, "ms-dns %s", dns_svr1);
       if(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL) != NULL)
            str += sprintf(str, "ms-dns %s", dns_svr2);
        }

    str += sprintf(str, "user \"%s\"\n", wan_mobile_user);
    str += sprintf(str, "password \"%s\"", wan_mobile_pass);
    str += sprintf(str, "mtu %d\n", wan_mobile_mtu);

    if(wan_mobile_idle != 0) {
        switch(wan_mobile_auto) {
            case 0:
                str += sprintf(str, "persist\n");
                break;
            case 1:
                str += sprintf(str, "demand\n");
                break;
        }
       if(wan_mobile_auto != 0)
            str += sprintf(str, "idle %d\n", wan_mobile_idle);
        }
    else
        str += sprintf(str, "persist\n");

    str += sprintf(str, "connect \"/usr/sbin/chat -v -r /var/log/chat.log -f /etc/chatscripts/3g.chat\"\n");

    fp = fopen("/etc/ppp/peers/ppp0", "w+");
    if(fp) {
        fprintf(fp, "%s", string);
        fclose(fp);
    }
}

static int dialup(void)
{
    if(f_exists("/tmp/cdc-wdm")) {
       int wan_mobile_auto;
       int wan_mobile_autopan;
       char wan_mobile_apn[SSBUF_LEN] = {0};
       char wan_mobile_pass[SSBUF_LEN] = {0};
       char wan_mobile_user[SSBUF_LEN] = {0};
       char mcc_mnc[SSBUF_LEN] = {0};
       char line[SSBUF_LEN] = {0};
       char APN[SSBUF_LEN] = {0};
       char PASS[SSBUF_LEN] = {0};
       char USER[SSBUF_LEN] = {0};

       wan_mobile_auto = cdb_get_int("$wan_mobile_auto", 0);
       wan_mobile_autopan = cdb_get_int("$wan_mobile_autopan", 0);

       if(wan_mobile_autopan == 1) {
          exec_cmd3(mcc_mnc, sizeof(mcc_mnc), "uqmi -sd cat /tmp/cdc-wdm --get-serving-system|awk -F, '{print $2\":\"$3}'|awk -F: '{print $2$4}'");
          exec_cmd3(line, sizeof(line), "sed -n \"/^%s/p\" /etc/config/apnlist.lst", mcc_mnc);
          exec_cmd3(APN, sizeof(APN), "echo %s | awk -F, '{ print $3}'", line);
          exec_cmd3(USER, sizeof(USER), "echo %s | awk -F, '{ print $4}'", line);
          exec_cmd3(PASS, sizeof(PASS), "echo %s | awk -F, '{ print $5}'", line);
       }
       else {
          cdb_get_str("$wan_mobile_apn", APN, sizeof(wan_mobile_apn), NULL);
          cdb_get_str("$wan_mobile_pass", PASS, sizeof(wan_mobile_pass), NULL);
          cdb_get_str("$wan_mobile_user", USER, sizeof(wan_mobile_user), NULL);
       }

       if(APN != NULL) {
          exec_cmd2("qmi start cat /tmp/cdc-wdm %s %s %s %s", APN, wan_mobile_auto, PASS, USER);
       }
       else {
          MTDM_LOGGER(this, LOG_ERR, "echo \"dialup fail: no APN\"\n");
          return 1;
       }
    }
    return 0;
}

static void start_dhcp(void)
{
    int wan_dhcp_mtu = 0;
    char wan_dhcp_reqip[SSBUF_LEN] = {0};

    //MTDM_LOGGER(this, LOG_INFO, "START DHCP");
    wan_dhcp_mtu = cdb_get_int("$wan_dhcp_mtu", 1500);
    cdb_get_str("$wan_dhcp_reqip", wan_dhcp_reqip, sizeof(wan_dhcp_reqip), NULL);

    //exec_cmd2("ifconfig %s mtu %d up", mtdm->rule.WANB, wan_dhcp_mtu);
    ifconfig2(mtdm->rule.WANB, IFUP, NULL, NULL, wan_dhcp_mtu);

    //MTDM_LOGGER(this, LOG_INFO, "wanb = %s, mtu = %d, reqip = %s", mtdm->rule.WANB, wan_dhcp_mtu, wan_dhcp_reqip);

    killall("udhcpc", SIGKILL);
    if(strlen(wan_dhcp_reqip) != 0)
       eval("udhcpc", "-T1", "-A1", "-t10", "-q", "-b", "-i", mtdm->rule.WANB, "-r", wan_dhcp_reqip, "-s", "/usr/bin/mt_udhcpcs");
    else
	 eval("udhcpc", "-T1", "-A1", "-t10", "-q", "-b", "-i", mtdm->rule.WANB, "-s", "/usr/bin/mt_udhcpcs");
      
}

static void start_ppp(void)
{
    //MTDM_LOGGER(this, LOG_INFO, "startpppp");
    start_stop_daemon("/usr/sbin/pppd", SSDF_START_DAEMON, NICE_DEFAULT, 0, NULL, 
        "call ppp0");
}

static int check_subnet(void)
{
    char lan_msk[SSBUF_LEN] = {0};
    char wanif_msk[SSBUF_LEN] = {0};
    char ip[SSBUF_LEN] = {0};
    struct in_addr msk_inp;
    struct in_addr lan_inp;
    struct in_addr wan_inp;
    struct in_addr lan_inp_rsrvd;
    int change = 0;

    cdb_get_str("$lan_msk", lan_msk, sizeof(lan_msk), "");
    cdb_get_str("$wanif_msk", wanif_msk, sizeof(wanif_msk), "");

    if (!strcmp(lan_msk, wanif_msk)) {
        cdb_get_str("$lan_ip", ip, sizeof(ip), "");
        if ((inet_aton(lan_msk, &msk_inp) != 0) && (inet_aton(ip, &lan_inp) != 0)) {
            lan_inp.s_addr &= msk_inp.s_addr;
        }
        strcpy(ip, "192.168.0.1");
        if ((inet_aton(lan_msk, &msk_inp) != 0) && (inet_aton(ip, &lan_inp_rsrvd) != 0)) {
            lan_inp_rsrvd.s_addr &= msk_inp.s_addr;
        }
        cdb_get_str("$wanif_ip", ip, sizeof(ip), "");
        if ((inet_aton(wanif_msk, &msk_inp) != 0) && (inet_aton(ip, &wan_inp) != 0)) {
            wan_inp.s_addr &= msk_inp.s_addr;
        }

        if (lan_inp.s_addr == wan_inp.s_addr) {
            cdb_set("$lan_msk","255.255.255.0");
            if (lan_inp.s_addr == lan_inp_rsrvd.s_addr) {
                strcpy(ip, "192.168.169.1");
                cdb_set("$dhcps_start", "192.168.169.2");
                cdb_set("$dhcps_end", "192.168.169.254");
            } else {
                strcpy(ip, "192.168.0.1");
                cdb_set("$dhcps_start", "192.168.0.2");
                cdb_set("$dhcps_end", "192.168.0.254");
            }

            cdb_set_int("$smrt_change", 1);
            //exec_cmd("/lib/wdk/commit");
            change = 1;
        }
        if (change)
        {
            cdb_set("$lan_ip", ip);
            cdb_save();
            ifconfig(mtdm->rule.LAN, IFDOWN, "0.0.0.0", NULL);
            ifconfig(mtdm->rule.LAN, IFUP, ip, lan_msk);
        }
    }
    return change;
}

static void static_ip_up(void)
{
    char inf[SSBUF_LEN] = {0};
    char info[SSBUF_LEN] = {0};
    char wan_msk[SSBUF_LEN] = {0};
    int wan_mtu = cdb_get_int("$wan_mtu", 0);
    int wan_alias = cdb_get_int("$wan_alias", 0);
    int inum = 1;

    //MTDM_LOGGER(this, LOG_INFO, "static ip up:");

    if(cdb_get_str("$wan_ip", info, sizeof(info), NULL) == NULL)
        MTDM_LOGGER(this, LOG_ERR, "ip is NULL, error");
    if(cdb_get_str("$wan_msk", wan_msk, sizeof(wan_msk), NULL) == NULL)
        MTDM_LOGGER(this, LOG_ERR, "netmask is NULL, error");
    ifconfig2(mtdm->rule.WANB, IFUP, info, wan_msk, wan_mtu);

    if(cdb_get_str("$wan_gw", info, sizeof(info), NULL) == NULL)
        MTDM_LOGGER(this, LOG_ERR, "gateway is NULL, error");
    exec_cmd2("route add default gw %s", info);

    if(wan_alias == 1) {
        while(inum <= max_alias) {
            cdb_get_array_str("$wan_alias_ip", inum, info, sizeof(info), "");
            if (strcmp(info, "")) {
                snprintf(inf, sizeof(inf), "%s:%d", mtdm->rule.WANB, inum);
                ifconfig(inf, IFUP, info, wan_msk);
            }
            inum++;
        }
    }
   
    //MTDM_LOGGER(this, LOG_INFO, "static ip up!!!") ;
    mtdm_wan_ip_up(bootState_done, NULL, 0);
    cdb_set_int("$wanif_state", Wanif_State_CONNECTED);
}

static void pptp_up(void)
{
    int pptp_if_mode;
    char pptp_ip[SSBUF_LEN] = {0};
    char pptp_msk[SSBUF_LEN] = {0};
    char pptp_gw[SSBUF_LEN] = {0};
    char pptp_id[SSBUF_LEN] = {0};
    
    pptp_if_mode = cdb_get_int("$pptp_if_mode", 0);
    cdb_get_str("$pptp_ip", pptp_ip, sizeof(pptp_ip), NULL);
    cdb_get_str("$pptp_msk", pptp_msk, sizeof(pptp_msk), NULL);
    cdb_get_str("$pptp_gw", pptp_gw, sizeof(pptp_gw), NULL);
    cdb_get_str("$pptp_id", pptp_id, sizeof(pptp_id), NULL);
    
    switch(pptp_if_mode) {
        case 0:
            ifconfig(mtdm->rule.WANB, IFUP, pptp_ip, pptp_msk);
            exec_cmd2("route add default gw %s", pptp_gw);
            break;

        case 1:
            start_dhcp();
            break;
    }
    save_pptp_file();
    start_ppp();
}

static void l2tp_up(void)
{
    int l2tp_if_mode;
    char l2tp_ip[SSBUF_LEN] = {0};
    char l2tp_msk[SSBUF_LEN] = {0};
    char l2tp_gw[SSBUF_LEN] = {0};
    char l2tp_id[SSBUF_LEN] = {0};
    char l2tp_svr[SSBUF_LEN] = {0};
           
    l2tp_if_mode = cdb_get_int("$l2tp_if_mode", 0);
    cdb_get_str("$l2tp_ip", l2tp_ip, sizeof(l2tp_ip), NULL);
    cdb_get_str("$l2tp_msk", l2tp_msk, sizeof(l2tp_msk), NULL);
    cdb_get_str("$l2tp_gw", l2tp_gw, sizeof(l2tp_gw), NULL);
    cdb_get_str("$l2tp_id", l2tp_id, sizeof(l2tp_id), NULL);
    cdb_get_str("$l2tp_svr", l2tp_svr, sizeof(l2tp_svr), NULL);
    
    switch(l2tp_if_mode) {
        case 0:
            ifconfig(mtdm->rule.WANB, IFUP, l2tp_ip, l2tp_msk);
            exec_cmd2("route add default gw %s", l2tp_gw);
            break;

        case 1:
            start_dhcp();
            break;
    }
    save_l2tp_file();
    exec_cmd("/usr/sbin/l2tpd");
    exec_cmd2("/usr/sbin/l2tp-control \"start-session %s\" >/dev/null 2>/dev/null",l2tp_svr);
}

static int start(void)
{
#if 0
    unsigned char mac[ETHER_ADDR_LEN];
    char ip[30];
    char out[30];

    get_ifname_ether_addr("br0", mac, sizeof(mac));
    MTDM_LOGGER(this, LOG_INFO, "br0 mac =%s", ether_etoa(mac, out));
    get_ifname_ether_ip("br0", ip, sizeof(ip));
    MTDM_LOGGER(this, LOG_INFO, "br0 ip =%s", ip);
#endif

    char usbif_find_eth[SSBUF_LEN] = {0};
    char str[SSBUF_LEN];

    cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL);

    if((mtdm->rule.IFPLUGD == NULL) && (mtdm->rule.WMODE) != 9) {
       MTDM_LOGGER(this, LOG_ERR, "wan start fail: without ifplugd interface");
       return 0;
    }

    if(mtdm->rule.WAN == NULL) {
       MTDM_LOGGER(this, LOG_ERR, "echo \"wan start fail: null wan interface(%s)\"", mtdm->rule.WAN);
       return 0;
    }

    if (((mtdm->rule.OMODE == opMode_wi) ||
         (mtdm->rule.OMODE == opMode_rp) ||
         (mtdm->rule.OMODE == opMode_br) ||
        #ifdef SPEEDUP_WLAN_CONF
         (mtdm->rule.OMODE == opMode_ap) ||
        #endif
         (mtdm->rule.OMODE == opMode_client)) && !mtdm->rule.WIFLINK) {
        return 0;
    }

    //MTDM_LOGGER(this, LOG_INFO, "wan start:1 generic opmode(%d) wmode(%d) ifs(%s)", mtdm->rule.OMODE, mtdm->rule.WMODE, mtdm->rule.WAN);
    //output_console("WMODE0 = %d, OMODE0 = %d\n", mtdm->rule.WMODE , mtdm->rule.OMODE );
    if(mtdm->rule.OMODE == 6) {
       int probe = 2;
       //char probe_str[5];
        char *probeif;
 
        if(mtdm->rule.WMODE == 9)
          probeif = "br0";
        else
          probeif = "eth0.2";

       ifconfig(probeif, IFUP, NULL, NULL);
#if 0       
       MTDM_LOGGER(this, LOG_INFO, "wan start: start probeif(%s)", probeif);
       exec_cmd3(probe_str, sizeof(probe_str), "smartprobe -I %s", probeif);
       probe = atoi(probe_str);

        if(probe == 0) {
            MTDM_LOGGER(this, LOG_INFO, "wan start: no server exists");
            if(mtdm->rule.WMODE != 9) {
                MTDM_LOGGER(this, LOG_INFO, "wan start: change to mobile 3G mode");
                cdb_set_int("$wan_mode", 9);
                cdb_set_int("$op_work_mode", 6);
             exec_cmd("/lib/wdk/commit");
                return 0;
            }
        }
       else 
#endif       	
       {
            if(mtdm->rule.WMODE == 9) {
                MTDM_LOGGER(this, LOG_INFO, "wan start: change to non-mobile 3G mode");
                cdb_set_int("$wan_mode", probe);
                cdb_set_int("$op_work_mode", 6);
             exec_cmd("/lib/wdk/commit");
                return 0;
            }

            cdb_set_int("$wan_mode", probe);
            switch(probe) {
                case 2:
                    MTDM_LOGGER(this, LOG_INFO, "wan start: start DHCP");
                 strcpy((char *)mtdm->rule.WMODE, "2");
                    strcpy(mtdm->rule.WAN, "eth0.2");
                    break;
                 
                case 3:
                    MTDM_LOGGER(this, LOG_INFO, "wan start: start PPPoE");
                 strcpy((char *)mtdm->rule.WMODE, "3");
                    strcpy(mtdm->rule.WAN, "ppp0");
                    break;
                 
                default:
                    MTDM_LOGGER(this, LOG_ERR, "wan start: error!, unknow probe option(%d)", probe);
                    return 0;
                    break;
            }
        }
#if 0
        // workmode 6(smartprobe) not use now, fix me later
        exec_cmd("sed -i -e 's;.*/etc/init.d/ethprobe$;;g' -e '/^$/d' /etc/crontabs/root");
        exec_cmd("*/1 * * * * . /etc/init.d/ethprobe > /etc/crontabs/root");
        killall("crond", SIGTERM);
        exec_cmd("/usr/sbin/crond restart -L /dev/null");
#endif
    }

    //MTDM_LOGGER(this, LOG_DEBUG, "wan start:2 generic opmode(%d) wmode(%d) ifs(%s)", mtdm->rule.OMODE, mtdm->rule.WMODE, mtdm->rule.WAN);

    if(mtdm->rule.WMODE == 9) {
        exec_cmd3(str, sizeof(str), "`ifconfig|grep ppp0|sed 's/^ppp0.*$/ppp0/g'`");
        if(str == NULL) {
            MTDM_LOGGER(this, LOG_INFO, "wan start:ppp0 doesn't exist");
        }
        else {
            MTDM_LOGGER(this, LOG_INFO, "wan start:ppp0 already exist");
            return 0;
        }
    }

    cdb_set_int("$wanif_state", Wanif_State_CONNECTING);
    if(cdb_get_int("$wan_arpbind_enable", 0) != 0)
        set_arp_wan_bind();

    exec_cmd("sed -i '//d' /etc/resolv.conf");

    //output_console("WMODE = %d, OMODE = %d\n", mtdm->rule.WMODE , mtdm->rule.OMODE );

    switch(mtdm->rule.WMODE) {
        case 1:
            static_ip_up();
            break;

        case 2:
            start_dhcp();
            break;

        case 3:
            MTDM_LOGGER(this, LOG_INFO, "pppoe UP");
            save_pppoe_file();
            ifconfig(mtdm->rule.WANB, IFUP, NULL, NULL);
            MTDM_LOGGER(this, LOG_INFO, "ppp UP, wanb up");
            start_ppp();
            break;

        case 4:
            pptp_up();
            break;

        case 5:
            l2tp_up();
            break;

        case 9:
            if(cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL) != NULL) {
                dialup();
                start_dhcp();
            }
            else {
                save_3gmodem_file();
                start_ppp();
            }
            break;
       
        default:
            MTDM_LOGGER(this, LOG_INFO, "%s unknown up", mtdm->rule.WAN);
            break;
    }
    return 0;
}

static int stop(void)
{
    char *ary[] = {"l2tpd", "udhcpc", "pppd"};
    int ary_size = (sizeof(ary))/(sizeof(ary[0]));
    int i;
    clear_netstatus();
    //output_console("STOP ==== IFPLUGD = %s, WMODE0 = %d,WAN = %s\n", mtdm->rule.IFPLUGD , mtdm->rule.WMODE, mtdm->rule.WAN);
    if((mtdm->rule.IFPLUGD == NULL) && (mtdm->rule.WMODE != 9))
        MTDM_LOGGER(this, LOG_ERR, "wan stop fail: without ifplugd interface");
    else if(mtdm->rule.WAN == NULL)
        MTDM_LOGGER(this, LOG_ERR, "wan stop fail: null wan interface(%s)", mtdm->rule.WAN);
    else {
        cdb_set_int("$wanif_state", Wanif_State_DISCONNECTING);
        clear_arp_perm();

        exec_cmd("/usr/sbin/l2tp-control \"exit\" >/dev/null 2>&1");

        for(i = 0; i < ary_size; i++) {
            killall(ary[i], SIGKILL);
        }

		if(mtdm->rule.WMODE == 2) {
			if(f_exists(DHCPD_BR0_PIDFILE))
				unlink(DHCPD_BR0_PIDFILE);
			else if(f_exists(DHCPD_BR1_PIDFILE))
				unlink(DHCPD_BR1_PIDFILE);
		}

        ifconfig(mtdm->rule.WAN, IFDOWN, "0.0.0.0", NULL);
#if defined(__PANTHER__)
        if(mtdm->rule.WPASUP)  /* workaround code, refine later, we need bridge keeps running for wpa_supplicant 4ways handshake to work, otherwise WiFi will never up after bridge down */
            ifconfig(mtdm->rule.WANB, IFUP, "0.0.0.0", NULL);
        else
            ifconfig(mtdm->rule.WANB, IFDOWN, "0.0.0.0", NULL);
#else
        ifconfig(mtdm->rule.WANB, IFDOWN, "0.0.0.0", NULL);
#endif
    }

    cdb_set_int("$wanif_state", Wanif_State_DISCONNECTED);

    MTDM_LOGGER(this, LOG_INFO, "wan stop: OMODE(%d) WMODE(%d) IFS(%s)", 
               mtdm->rule.OMODE, mtdm->rule.WMODE, mtdm->rule.WAN);

    return 0;
}

static int boot(void)
{
    /* ifplugd will call wan restart when interface is ready
     * excepting to 3g
     */
    if (mtdm->rule.WMODE == wanMode_3g) {
        start();
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  boot  },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int wan_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

