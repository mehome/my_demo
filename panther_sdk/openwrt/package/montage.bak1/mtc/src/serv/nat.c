#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_misc.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

FILE *fp1, *fp2;

static void do_iptables_commit()
{
    if(f_exists("/usr/sbin/iptables-restore")) {
       exec_cmd("cat /tmp/nat-rule | /usr/sbin/iptables-restore");
#ifdef CONFIG_NAT
       exec_cmd("cat /tmp/mangle-rule | /usr/sbin/iptables-restore");
#endif
    }
}

static void do_iptables_delete(char *str)
{
    if(f_exists("/usr/sbin/iptables-restore"))
       exec_cmd2("echo -e \"*%s\nCOMMIT\" | /usr/sbin/iptables-restore", str);
}

static void nat_clear_all_new(void)
{
#ifdef CONFIG_NAT
    do_iptables_delete("mangle");
#endif
    do_iptables_delete("nat");
#ifdef CONFIG_NAT
    exec_cmd("conntrack -F");
    exec_cmd("conntrack -D");
#endif
}

#ifdef CONFIG_NAT
//nat_port_map1=en=1&gp=2&gprot=tcp&lip=192.168.0.20&schen=1&time=0-7200&day=127&desc=test1
static int nat_add_port_mapping(void)
{
    int i, err, en, num;
    char *argv[30];
    char nat_port_map[MSBUF_LEN];
    char wanip[SSBUF_LEN];
    char gp[USBUF_LEN];

    err = 0;
    get_ifname_ether_ip(mtdm->rule.WAN, wanip, sizeof(wanip));

    for(i=1;i<21;i++) {
        if(cdb_get_array_str("$nat_port_map", i, nat_port_map, sizeof(nat_port_map), NULL) == NULL)
            break;
        num = str2argv(nat_port_map, argv, '&');
        if(num < 5)
            break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "gp=")||!str_arg(argv, "gprot=")||!str_arg(argv, "lip=")||!str_arg(argv, "desc="))
            continue;
        en = atoi(str_arg(argv, "en="));
        if(en == 1) {
            snprintf(gp, sizeof(gp), "%s", str_arg(argv, "gp="));
            strrep(gp, '-', ':');
            if(!strcmp(str_arg(argv, "gprot="), "tcp/udp")) {
                if(strlen(wanip) != 0) {
                    fprintf(fp1, "-A PREROUTE_RULE -p tcp -d %s -m multiport --dport %s -j DNAT --to %s\n", 
                        wanip, gp, str_arg(argv, "lip="));
                }
                else {
                    err = 1;
                }

                if(strlen(wanip) != 0) {
                    fprintf(fp1, "-A PREROUTE_RULE -p udp -d %s -m multiport --dport %s -j DNAT --to %s\n", 
                        wanip, gp, str_arg(argv, "lip="));
                }
                else {
                    err = 1;
                }
            }
            else {
                if(strlen(wanip) != 0) {
                    fprintf(fp1, "-A PREROUTE_RULE -p %s -d %s -m multiport --dport %s -j DNAT --to %s\n", 
                        str_arg(argv, "gprot="), wanip, gp, str_arg(argv, "lip="));
                }
                else {
                    err = 1;
                }
            }
        }
    }
    return err;
}

//nat_vserver1=en=1&desc=vs1&lp=8080&gp=80&gprot=tcp/udp&lip=192.168.0.10
static int nat_add_virtual_server(void)
{
    int i, err, en, num;
    char *argv[30];
    char nat_vserver[MSBUF_LEN];
    char wanip[SSBUF_LEN];
    char lannet[SSBUF_LEN];

    err = 0;
    cdb_get_str("$lan_ip", lannet, sizeof(lannet), NULL);
    if ((argv[0] = strrchr(lannet, '.')) != NULL) {
        *(argv[0] + 1) = '0';
        *(argv[0] + 2) = 0;
    }
    get_ifname_ether_ip(mtdm->rule.WAN, wanip, sizeof(wanip));

    for(i=1;i<21;i++) {
        if(cdb_get_array_str("$nat_vserver", i, nat_vserver, sizeof(nat_vserver), NULL) == NULL)
            break;
        num = str2argv(nat_vserver, argv, '&');
        if(num < 6)
            break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "gp=")||!str_arg(argv, "gprot=")||!str_arg(argv, "lp=")||!str_arg(argv, "lip=")||!str_arg(argv, "desc="))
            continue;
        en = atoi(str_arg(argv, "en="));
        if(en == 1) {
            if(!strcmp(str_arg(argv, "gprot="), "tcp/udp")) {
                if(strlen(wanip) != 0) {
                    fprintf(fp1, "-A PREROUTE_RULE -p tcp -d %s --dport %s -j DNAT --to %s:%s\n", 
                        wanip, str_arg(argv, "gp="), str_arg(argv, "lip="), str_arg(argv, "lp="));
                }
                else {
                    err = 1;
                }

                if(strlen(wanip) != 0) {
                    fprintf(fp1, "-A PREROUTE_RULE -p udp -d %s --dport %s -j DNAT --to %s:%s\n", 
                        wanip, str_arg(argv, "gp="), str_arg(argv, "lip="), str_arg(argv, "lp="));
                }
                else {
                    err = 1;
                }

                // for NAT-LoopBack
                fprintf(fp1, "-A POSTROUTE_RULE -p tcp -d %s -s %s/24 --dport %s -j SNAT --to %s\n", 
                    str_arg(argv, "lip="), str_arg(argv, "lip="), str_arg(argv, "lp="), str_arg(argv, "lip="));
                fprintf(fp1, "-A POSTROUTE_RULE -p udp -d %s -s %s/24 --dport %s -j SNAT --to %s\n", 
                    str_arg(argv, "lip="), str_arg(argv, "lip="), str_arg(argv, "lp="), str_arg(argv, "lip="));

                // for hairpin
                fprintf(fp1, "-A POSTROUTE_RULE -s %s/24 -p tcp --dport %s -d %s -j MASQUERADE\n", 
                    lannet, str_arg(argv, "gp="), str_arg(argv, "lip="));
                fprintf(fp1, "-A POSTROUTE_RULE -s %s/24 -p udp --dport %s -d %s -j MASQUERADE\n", 
                    lannet, str_arg(argv, "gp="), str_arg(argv, "lip="));
            }
            else {
                fprintf(fp1, "-A PREROUTE_RULE -p %s -d %s --dport %s -j DNAT --to %s:%s\n", 
                    str_arg(argv, "gprot="), wanip, str_arg(argv, "gp="), str_arg(argv, "lip="), str_arg(argv, "lp="));
                fprintf(fp1, "-A POSTROUTE_RULE -s %s/24 -p %s --dport %s -d %s -j MASQUERADE\n", 
                    lannet, str_arg(argv, "gprot="), str_arg(argv, "gp="), str_arg(argv, "lip="));
            }
        }
    }
    return err;
}

//nat_port_trig1=en=1&rp=2210-2215&rprot=udp&gp=2208-2209&gprot=tcp&desc=trig1
static int nat_add_triggerport(void)
{
    int i, err, en, num;
    char *argv[30];
    char gp[USBUF_LEN];
    char rp[USBUF_LEN];
    char nat_port_trig[MSBUF_LEN];
    char trigstr[MSBUF_LEN];

    err = 0;

    for(i=1;i<11;i++) {
        if(cdb_get_array_str("$nat_port_trig", i, nat_port_trig, sizeof(nat_port_trig), NULL) == NULL)
            break;
        num = str2argv(nat_port_trig, argv, '&');
        if(num < 6)
            break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "rp=")||!str_arg(argv, "rprot=")||!str_arg(argv, "gp=")||!str_arg(argv, "gprot=")||!str_arg(argv, "desc="))
            continue;
        en = atoi(str_arg(argv, "en="));
        if(en == 1) {
            snprintf(gp, sizeof(gp), "%s", str_arg(argv, "gp="));
            strrep(gp, '-', ':');
            snprintf(rp, sizeof(rp), "%s", str_arg(argv, "rp="));
            strrep(rp, '-', ':');
            sprintf(trigstr, "--type out --trprot %s --trport %s --reprot %s --report %s", 
                str_arg(argv, "rprot="), rp, str_arg(argv, "gprot="), gp);
            if(!strcmp(str_arg(argv, "rprot="), "tcp/udp")) {
                fprintf(fp1, "-I POSTROUTE_RULE -o %s -p tcp --dport %s -j TRIGGER %s\n", 
                    mtdm->rule.WAN, rp, trigstr);
                fprintf(fp1, "-I POSTROUTE_RULE -o %s -p udp --dport %s -j TRIGGER %s\n", 
                    mtdm->rule.WAN, rp, trigstr);
            }
            else {
                fprintf(fp1, "-I POSTROUTE_RULE -o %s -p %s --dport %s -j TRIGGER %s\n", 
                    mtdm->rule.WAN, str_arg(argv, "rprot="), rp, trigstr);
            }

            if(!strcmp(str_arg(argv, "gprot="), "tcp/udp")) {
                fprintf(fp1, "-I PREROUTE_RULE -i %s -p tcp --dport %s -j TRIGGER --type dnat\n", 
                    mtdm->rule.WAN, gp);
                fprintf(fp1, "-I PREROUTE_RULE -i %s -p udp --dport %s -j TRIGGER --type dnat\n", 
                    mtdm->rule.WAN, gp);
            }
            else {
                fprintf(fp1, "-I PREROUTE_RULE -i %s -p %s --dport %s -j TRIGGER --type dnat\n", 
                    mtdm->rule.WAN, str_arg(argv, "gprot="), gp);
            }
        }
    }
    return err;
}

//nat_dmz_host1=en=1&lip=192.168.0.9&gip=1.1.1.1
static int nat_dmz(void)
{
    int i, err, en, num;
    char *argv[30];
    char nat_dmz_host[MSBUF_LEN];
    char wanip[SSBUF_LEN];

    err = 0;
    get_ifname_ether_ip(mtdm->rule.WAN, wanip, sizeof(wanip));

    for(i=1;i<11;i++) {
       if(cdb_get_array_str("$nat_dmz_host", i, nat_dmz_host, sizeof(nat_dmz_host), NULL) == NULL)
          break;
       num = str2argv(nat_dmz_host, argv, '&');
       if(num < 3)
          break;
       if(!str_arg(argv, "en=")||!str_arg(argv, "lip=")||!str_arg(argv, "gip="))
          continue;
       en = atoi(str_arg(argv, "en="));
       if(en == 1) {
          if(strcmp(str_arg(argv, "gip="), "\0") == 0) {
             if(strlen(wanip) != 0)
                fprintf(fp1, "-A PREROUTE_RULE -i %s -p tcp -d %s -j DNAT --to %s\n", mtdm->rule.WAN, wanip, str_arg(argv, "lip="));
             else
                err = 1;

             if(strlen(wanip) != 0)
                fprintf(fp1, "-A PREROUTE_RULE -i %s -p udp -d %s -j DNAT --to %s\n", mtdm->rule.WAN, wanip, str_arg(argv, "lip="));
             else
                err = 1;
          }
          else {
             fprintf(fp1, "-A PREROUTE_RULE -i %s -p tcp -d %s -j DNAT --to %s\n", mtdm->rule.WAN, str_arg(argv, "gip="), str_arg(argv, "lip="));
             fprintf(fp1, "-A PREROUTE_RULE -i %s -p tcp -d %s -j DNAT --to %s\n", mtdm->rule.WAN, str_arg(argv, "gip="), str_arg(argv, "lip="));
          }
       }
    }
    return err;
}
#endif

#if 0
static void remote_setup(void)
{
    ipt_mangle_str += sprintf(ipt_mangle_str, "-N REMOTEMGT");
    ipt_mangle_str += sprintf(ipt_mangle_str, "-I PREROUTING -j REMOTEMGT");
}

static void nat_remote_redirect(void)
{
    char lan_ip[SSBUF_LEN];
    char wanip[SSBUF_LEN];

    remote_setup();

    get_ifname_ether_ip(mtdm->rule.WAN, wanip, sizeof(wanip));
    if(strlen(wanip) != 0)
       ipt_nat_str += sprintf(ipt_nat_str, "-A PREROUTE_RULE -p tcp -d %s --dport 80 -j DNAT --to %s:80\n", wanip, cdb_get_str("$lan_ip", lan_ip, sizeof(lan_ip), NULL));
    if(strlen(wanip) != 0)
       ipt_nat_str += sprintf(ipt_nat_str, "-A PREROUTE_RULE -p tcp -d %s --dport 8443 -j DNAT --to %s:8443\n", wanip, cdb_get_str("$lan_ip", lan_ip, sizeof(lan_ip), NULL));
}
#endif

static void nat_masquerade(void)
{
    if(iface_exist(mtdm->rule.WAN))
        fprintf(fp1, "-A POSTROUTE_RULE -o %s -j MASQUERADE\n", mtdm->rule.WAN);
}

#ifdef CONFIG_NAT
static void non_standard_ftp(void)
{
    f_write_sprintf("/proc/non_standard_ftp", 0, 0, "%d ", cdb_get_int("$nat_alg_ftp_port", 0));
}

static void nat_pptp_alg(void)
{
    if(cdb_get_int("$nat_alg_pptp", 0) == 1)
        f_write_string("/proc/pptp_alg", "1 ", 0, 0);
    if(cdb_get_int("$nat_alg_pptp", 0) == 0)
        f_write_string("/proc/pptp_alg", "0 ", 0, 0);
}

static void nat_rtsp_alg(void)
{
    if(cdb_get_int("$nat_alg_rtsp", 0) == 1)
        f_write_string("/proc/rtsp_alg", "1 ", 0, 0);
    if(cdb_get_int("$nat_alg_rtsp", 0) == 0)
        f_write_string("/proc/rtsp_alg", "0 ", 0, 0);
}

static void nat_h323_alg(void)
{
    if(cdb_get_int("$nat_alg_netmeeting", 0) == 1)
        f_write_string("/proc/h323_alg", "1 ", 0, 0);
    if(cdb_get_int("$nat_alg_netmeeting", 0) == 0)
        f_write_string("/proc/h323_alg", "0 ", 0, 0);
}

static void nat_sip_alg(void)
{
    if(cdb_get_int("$nat_alg_sip", 0) == 1)
        f_write_string("/proc/sip_alg", "1 ", 0, 0);
    if(cdb_get_int("$nat_alg_sip", 0) == 0)
        f_write_string("/proc/sip_alg", "0 ", 0, 0);
}

static void nat_msn_alg(void)
{
    if(cdb_get_int("$nat_alg_msn", 0) == 1)
        f_write_string("/proc/msn_alg", "1 ", 0, 0);
    if(cdb_get_int("$nat_alg_msn", 0) == 0)
        f_write_string("/proc/msn_alg", "0 ", 0, 0);
}

static void nat_ipsec_alg(void)
{
    if(cdb_get_int("$nat_alg_ipsec", 0) == 1) {
       fprintf(fp1, "-A PREROUTE_RULE -i %s -p 50 -m esp --type out\n", mtdm->rule.LAN);
       fprintf(fp1, "-A PREROUTE_RULE -i %s -p 50 -m esp --type dnat\n", mtdm->rule.WAN);
    }
}

static void nat_tcp_mss_clamp(void)
{
    fprintf(fp2, "-N CLAMP\n");
    fprintf(fp2, "-A FORWARD -j CLAMP\n");
    fprintf(fp2, "-A CLAMP -p tcp -m tcp --tcp-flags SYN,RST SYN -m tcpmss --mss 1400:1536 -j TCPMSS --clamp-mss-to-pmtu\n");
}

static void mangle_pre_setup(void)
{
    fprintf(fp2, "-N PRE_RULE\n");
    fprintf(fp2, "-A PREROUTING -j PRE_RULE\n");
}
#endif

static void nat_setup(void)
{
    fprintf(fp1, "-N PREROUTE_RULE\n");
    fprintf(fp1, "-A PREROUTING -j PREROUTE_RULE\n");
    fprintf(fp1, "-N POSTROUTE_RULE\n");
    fprintf(fp1, "-A POSTROUTING -j POSTROUTE_RULE\n");
}

static void nat_hnat(void)
{
    char lan_ip[SSBUF_LEN];
    char wanip[SSBUF_LEN];

    get_ifname_ether_ip(mtdm->rule.WAN, wanip, sizeof(wanip));
    if(strlen(wanip) != 0) {
        f_write_sprintf("/proc/hnat", 0, 0, "wan_mode=%d ", mtdm->rule.WMODE);
        f_write_sprintf("/proc/hnat", 0, 0, "wan_ip=%s ", wanip);
        f_write_sprintf("/proc/hnat", 0, 0, "lan_ip=%s ", cdb_get_str("$lan_ip", lan_ip, sizeof(lan_ip), NULL));
        if((cdb_get_int("$nat_acmode", 0) == 0) || (cdb_get_int("$nat_acmode", 0) == 1))
            f_write_sprintf("/proc/hnat", 0, 0, "hnat_mode=%d ", cdb_get_int("$nat_acmode", 0));
    }
}

static int start(void)
{
    /*We need NAT Table always*/
    fp1 = fopen("/tmp/nat-rule", "w+");
#ifdef CONFIG_NAT
    fp2 = fopen("/tmp/mangle-rule", "w+");
#endif
    if(cdb_get_int("$sys_funcmode", 0) == 1)
       goto done;

    if(fp1) {
       fprintf(fp1, "*nat\n");
       nat_setup();
       
       if(cdb_get_int("$nat_enable", 0) == 1) {
          /*We Need MASQUERADE Always*/;
          nat_masquerade();
#ifdef CONFIG_NAT
          nat_add_port_mapping();
          nat_add_virtual_server();
          nat_add_triggerport();
          nat_pptp_alg();
          nat_rtsp_alg();
          nat_h323_alg();
          nat_sip_alg();
          nat_msn_alg();

          if(mtdm->rule.OMODE != 9)
             nat_ipsec_alg();
          
          nat_tcp_mss_clamp();
          non_standard_ftp();
#endif
          nat_hnat();

#ifdef CONFIG_NAT
          if(cdb_get_int("$nat_dmz_enable", 0) == 1)
             nat_dmz();
#endif
       }
       fprintf(fp1, "COMMIT\n");
    }

#ifdef CONFIG_NAT
    if(fp2) {
       fprintf(fp2, "*mangle\n");
       mangle_pre_setup();
       fprintf(fp2, "COMMIT\n");
    }
#endif
done:
    if(fp1)
        fclose(fp1);
#ifdef CONFIG_NAT
    if(fp2)
        fclose(fp2);
#endif
    do_iptables_commit();
    unlink("/tmp/nat-rule");
#ifdef CONFIG_NAT
    unlink("/tmp/mangle-rule");
#endif
    return 0;
}

static int stop(void)
{
    nat_clear_all_new();
    f_write_string("/proc/hnat", "reset ", 0, 0);
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int nat_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}

