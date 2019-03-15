#ifdef CONFIG_FW
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtdm.h"
#include "include/mtdm_proc.h"

extern MtdmData *mtdm;

int FORWARD_DROP = 0;

int fw_macf_mode = 0;
int fw_urlf_mode = 0;
int fw_clientf_mode = 0;
int fw_enable = 0;
int fw_drop_wan_ping = 0;
int fw_drop_lan_ping = 0;
// for router
#if 0
int fw_drop_netbios = 0;
int fw_drop_tcp_null = 0;
int fw_drop_ident = 0;
int fw_drop_icmp_flood = 0;
int fw_drop_sync_flood = 0;
int fw_drop_land_attack = 0;
int fw_drop_snork = 0;
int fw_drop_udp_ploop = 0;
int fw_drop_ip_spoof = 0;
int fw_drop_smurf = 0;
int fw_drop_ping_death = 0;
int fw_drop_icmp_resp = 0;
int fw_drop_port_scan = 0;
int fw_drop_frag = 0;
#endif

char timestart[USBUF_LEN] = {0};
char timeend[USBUF_LEN] = {0};
char DAYS[USBUF_LEN] = {0};
FILE *fp;

static void do_iptables_commit()
{
    if(f_exists("/usr/sbin/iptables-restore"))
       exec_cmd("cat /tmp/filter-rule | /usr/sbin/iptables-restore");
}

static void get_time(char *time)
{
    struct tm *time_info;
    time_t tstart, tstop;

    if ((!time) || (sscanf(time, "%d-%d", (int *)&tstart, (int *)&tstop) != 2)) {
        tstart = 0;
        tstop = 0;
    }
    if (tstart < 0) {
        tstart = 0;
    }
    else if (tstart >= 86400) {
        tstart = 86399;
    }
    if (tstop < 0) {
        tstop = 0;
    }
    else if (tstop >= 86400) {
        tstop = 86399;
    }

    time_info = gmtime(&tstart);
    strftime(timestart, sizeof(timestart), "%H:%M", time_info);

    time_info = gmtime(&tstop);
    strftime(timeend, sizeof(timeend), "%H:%M", time_info);
}

static void get_day(int day)
{
    char *day_tbl[7] = {
        "Sun",
        "Mon",
        "Tue",
        "Wed",
        "Thu",
        "Fri",
        "Sat"
    };
    char *days = DAYS;
    int i;

    for (i=6; i>=0; i--) {
        if (day & (1 << i)) {
            days += sprintf(days, "%s,", day_tbl[i]);
        }
    }
    if (days != DAYS) {
        *(days - 1) = 0;
    }
    else {
        *days = 0;
    }
}

#if 0
// IPTABLES_MODE = 0 need to use this func
static void filter_clear_all(void)
{
    do_iptables_rule("-F INPUT_RULE");
    do_iptables_rule("-D INPUT -j INPUT_RULE");
    do_iptables_rule("-X INPUT_RULE");
    do_iptables_rule("-F OUTPUT_RULE");
    do_iptables_rule("-D OUTPUT -j OUTPUT_RULE");
    do_iptables_rule("-X OUTPUT_RULE");
    do_iptables_rule("-F FORWARD_RULE");
    do_iptables_rule("-D FORWARD -j FORWARD_RULE");
    do_iptables_rule("-X FORWARD_RULE");

    do_iptables_rule("-F SYSLOG");
    do_iptables_rule("-D INPUT -j SYSLOG");
    do_iptables_rule("-D FORWARD -j SYSLOG");
    do_iptables_rule("-D OUTPUT -j SYSLOG");
    do_iptables_rule("-X SYSLOG");
}
#endif

// fw_macf1=en=1&smac=00:11:22:33:44:51&desc=macfilter1
static int drop_mac_access(void)
{
    char fw_macf[MSBUF_LEN];
	char *argv[10];
    char *smac;
	int num, i, en;

    for(i=1;i<21;i++) {
        if(cdb_get_array_str("$fw_macf", i, fw_macf, sizeof(fw_macf), NULL) == NULL)
           break;
        num = str2argv(fw_macf, argv, '&');
        if(num < 3)
           break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "smac=")||!str_arg(argv, "desc="))
           continue;
        en = atoi(str_arg(argv, "en="));
        if(en == 1) {
           smac = str_arg(argv, "smac=");
           fprintf(fp, "-A FORWARD_RULE -m mac --mac-source %s -j DROP\n", smac);
        }
    }
    return 0;
}

static int accept_mac_access(void)
{
    char fw_macf[MSBUF_LEN];
	char *argv[10];
    char *smac;
	int num, i, en;

    for(i=1;i<21;i++) {
        if(cdb_get_array_str("$fw_macf", i, fw_macf, sizeof(fw_macf), NULL) == NULL)
           break;
        num = str2argv(fw_macf, argv, '&');
        if(num < 3)
           break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "smac=")||!str_arg(argv, "desc="))
           continue;
        en = atoi(str_arg(argv, "en="));
        if(en == 1) {
           smac = str_arg(argv, "smac=");
           fprintf(fp, "-A FORWARD_RULE -m mac --mac-source %s -j ACCEPT\n", smac);
        }
    }
    return 0;
}

// fw_urlf1=en=1&str=sex&sip=192.168.0.33-192.168.0.40
static int drop_urlf_access(void)
{
    char fw_urlf[MSBUF_LEN];
	char *argv[10];
    char *str, *sip, *ptr;
    char tmp[SSBUF_LEN];
	int num, i, en;

    for(i=1;i<29;i++) {
        if(cdb_get_array_str("$fw_urlf", i, fw_urlf, sizeof(fw_urlf), NULL) == NULL)
            continue;
        num = str2argv(fw_urlf, argv, '&');
        if(num < 3)
            break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "str=")||!str_arg(argv, "sip="))
            continue;
        str = str_arg(argv, "str=");
        if ((ptr = strstr(str, "http://")) != NULL) {
            ptr += 7;
        }
        else {
            ptr = str;
        }
        snprintf(tmp, sizeof(tmp), "%s", ptr);
        sip = str_arg(argv, "sip=");
        en = atoi(str_arg(argv, "en="));
        // web page doesn't have vairable "type", so we don't have type url and type host now.
        if(en == 1) {
            fprintf(fp, "-A FORWARD_RULE -p tcp -m iprange --src-range %s -m webstr --url \"%s\" -j REJECT --reject-with tcp-reset\n", sip, tmp);
        }
    }
    return 0;
}

static int accept_urlf_access(void)
{
    char fw_urlf[MSBUF_LEN];
	char *argv[10];
    char *str, *sip, *ptr;
    char tmp[SSBUF_LEN];
	int num, i, en;

    for(i=1;i<21;i++) {
        if(cdb_get_array_str("$fw_urlf", i, fw_urlf, sizeof(fw_urlf), NULL) == NULL)
           continue;
        num = str2argv(fw_urlf, argv, '&');
        if(num < 3)
           break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "str=")||!str_arg(argv, "sip="))
           continue;
        str = str_arg(argv, "str=");
        if ((ptr = strstr(str, "http://")) != NULL) {
            ptr += 7;
        }
        else {
            ptr = str;
        }
        snprintf(tmp, sizeof(tmp), "%s", ptr);
        sip = str_arg(argv, "sip=");
        en = atoi(str_arg(argv, "en="));
        // web page doesn't have vairable "type", so we don't have type url and type host now.
        if(en == 1) {
           fprintf(fp, "-A FORWARD_RULE -p tcp -m iprange --src-range %s -m webstr --url \"%s\" -j ACCEPT\n", sip, tmp);
           fprintf(fp, "-A FORWARD_RULE -p tcp -m iprange --src-range %s -m webstr ! --url \"%s\" -j REJECT --reject-with tcp-reset\n", sip, tmp);
           fprintf(fp, "-A FORWARD_RULE -p tcp --dport 80 -m iprange ! --src-range %s -j REJECT --reject-with tcp-reset\n", sip);
        }
    }
    return 0;
}

// fw_clientf1=en=1&dir=0&sip=192.168.0.7-20&sp=2210-2214&dip=203.1.1.43&dp=2208-2209&prot=tcp/udp&schen=1&time=0-72000&day=3&desc=clientf1
static int drop_client_access(void)
{
    char fw_clientf[MSBUF_LEN];
	char *argv[30];
    char sp[USBUF_LEN];
    char dp[USBUF_LEN];
    char *cmd;
    char cmd_str[SSBUF_LEN];
    char timstr[SSBUF_LEN];
	int num, i, en;

    for(i=1;i<21;i++) {
        if(cdb_get_array_str("$fw_clientf", i, fw_clientf, sizeof(fw_clientf), 	NULL) == NULL)
            break;
        num = str2argv(fw_clientf, argv, '&');
        if(num < 9)
            break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "dir=")||!str_arg(argv, "sip=")||!str_arg(argv, "dp=")||!str_arg(argv, "prot=")||!str_arg(argv, "schen=")||!str_arg(argv, "time=")||!str_arg(argv, "day=")||!str_arg(argv, "desc="))
            continue;
        en = atoi(str_arg(argv, "en="));
        if(en == 1) {
            cmd = cmd_str;
            if(str_arg(argv, "sip=") != NULL)
                sprintf(cmd, "-m iprange --src-range %s ", str_arg(argv, "sip="));
            if(str_arg(argv, "dip=") != NULL)
                cmd += sprintf(cmd, "-m iprange --dst-range %s ", str_arg(argv, "dip="));
            if(str_arg(argv, "sp=") != NULL) {
                snprintf(sp, sizeof(sp), "%s", str_arg(argv, "sp="));
                strrep(sp, '-', ':');
                cmd += sprintf(cmd, "--sport %s ", sp);
            }
            if(str_arg(argv, "dp=") != NULL) {
                snprintf(dp, sizeof(dp), "%s", str_arg(argv, "dp="));
                strrep(dp, '-', ':');
                cmd += sprintf(cmd, "--dport %s", dp);
            }

            if(!strcmp(str_arg(argv, "schen="), "1")) {
                get_time(str_arg(argv, "time="));
                get_day(atoi(str_arg(argv, "day=")));
                sprintf(timstr, "-m time --timestart %s --timestop %s --weekdays %s", timestart, timeend, DAYS);
                if(!strcmp(str_arg(argv, "prot="), "tcp/udp")) {
                    fprintf(fp, "-A FORWARD_RULE -p tcp %s %s -j DROP\n", cmd_str, timstr);
                    fprintf(fp, "-A FORWARD_RULE -p udp %s %s -j DROP\n", cmd_str, timstr);
                }
                else {
                    fprintf(fp, "-A FORWARD_RULE -p %s %s %s -j DROP\n",str_arg(argv, "prot="), cmd_str, timstr);
                }
            }
            else {
                if(!strcmp(str_arg(argv, "prot="), "tcp/udp")) {
                    fprintf(fp, "-A FORWARD_RULE -p tcp %s -j DROP\n", cmd_str);
                    fprintf(fp, "-A FORWARD_RULE -p udp %s -j DROP\n", cmd_str);
                }
                else {
                    fprintf(fp, "-A FORWARD_RULE -p %s %s -j DROP\n",str_arg(argv, "prot="), cmd_str);
                }
            }
        }
    }
    return 0;
}

static int accept_client_access(void)
{
    char fw_clientf[MSBUF_LEN];
	char *argv[30];
    char sp[USBUF_LEN];
    char dp[USBUF_LEN];
    char *cmd;
    char cmd_str[SSBUF_LEN];
    char timstr[SSBUF_LEN];
	int num, i, en;

    for(i=1;i<21;i++) {
        if(cdb_get_array_str("$fw_clientf", i, fw_clientf, sizeof(fw_clientf), 	NULL) == NULL)
            break;
        num = str2argv(fw_clientf, argv, '&');
        if(num < 9)
            break;
        if(!str_arg(argv, "en=")||!str_arg(argv, "dir=")||!str_arg(argv, "sip=")||!str_arg(argv, "dp=")||!str_arg(argv, "prot=")||!str_arg(argv, "schen=")||!str_arg(argv, "time=")||!str_arg(argv, "day=")||!str_arg(argv, "desc="))
            continue;
        en = atoi(str_arg(argv, "en="));
        if(en == 1) {
            cmd = cmd_str;
            if(str_arg(argv, "sip=") != NULL)
                sprintf(cmd, "-m iprange --src-range %s ", str_arg(argv, "sip="));
            if(str_arg(argv, "dip=") != NULL)
                cmd += sprintf(cmd, "-m iprange --dst-range %s ", str_arg(argv, "dip="));
            if(str_arg(argv, "sp=") != NULL) {
                snprintf(sp, sizeof(sp), "%s", str_arg(argv, "sp="));
                strrep(sp, '-', ':');
                cmd += sprintf(cmd, "--sport %s ", sp);
            }
            if(str_arg(argv, "dp=") != NULL) {
                snprintf(dp, sizeof(dp), "%s", str_arg(argv, "dp="));
                strrep(dp, '-', ':');
                cmd += sprintf(cmd, "--dport %s", dp);
            }

            if(!strcmp(str_arg(argv, "schen="), "1")) {
                get_time(str_arg(argv, "time="));
                get_day(atoi(str_arg(argv, "day=")));
                sprintf(timstr, "-m time --timestart %s --timestop %s --weekdays %s", timestart, timeend, DAYS);
                if(!strcmp(str_arg(argv, "prot="), "tcp/udp")) {
                    fprintf(fp, "-A FORWARD_RULE -p tcp %s %s -j ACCEPT\n", cmd_str, timstr);
                    fprintf(fp, "-A FORWARD_RULE -p udp %s %s -j ACCEPT\n", cmd_str, timstr);
                }
                else {
                    fprintf(fp, "-A FORWARD_RULE -p %s %s %s -j ACCEPT\n",str_arg(argv, "prot="), cmd_str, timstr);
                }
            }
            else {
                if(!strcmp(str_arg(argv, "prot="), "tcp/udp")) {
                    fprintf(fp, "-A FORWARD_RULE -p tcp %s -j ACCEPT\n", cmd_str);
                    fprintf(fp, "-A FORWARD_RULE -p udp %s -j ACCEPT\n", cmd_str);
                }
                else {
                    fprintf(fp, "-A FORWARD_RULE -p %s %s -j ACCEPT\n",str_arg(argv, "prot="), cmd_str);
                }
            }
        }
    }
    return 0;
}

static void filter_drop_icmp(char *interface)
{
    fprintf(fp, "-A INPUT_RULE -i %s -p icmp --icmp-type 8 -j DROP\n", interface);
}

// for router
#if 0
static void filter_drop_netbios(void)
{
    do_iptables_rule("-A INPUT_RULE -p tcp -m multiport --destination-port 135.136,137,138,139,455 -j DROP");
    do_iptables_rule("-A INPUT_RULE -p udp -m multiport --destination-port 135.136,137,138,139,455 -j DROP");
}

static void filter_drop_tcpnull(void)
{
    do_iptables_rule("-A INPUT_RULE -p tcp --tcp-flags ALL NONE -j DROP");
}

static void filter_drop_ident(void)
{
    do_iptables_rule("-A INPUT_RULE -p tcp --dport 113 -j DROP");
}

static void filter_drop_icmp_flood(void)
{
    int fw_icmpf_rate = 0;
    char *str;

    str = sprintf(str, "-A INPUT_RULE -p icmp --icmp-type echo-request -m limit --limit %s/second -j ACCEPT", cdb_get_int("$fw_icmpf_rate", 0));
    do_iptables_rule(str);
    do_iptables_rule("-A INPUT_RULE -p icmp --icmp-type echo-request -j LOG --log-prefix \\\"[DROP icmp flooding]:\\\" --log-level 1");
    do_tptables_rule("-A INPUT_RULE -p icmp --icmp_type echo-request -j DROP");
}

static void filter_drop_sync_flood(void)
{
    int fw_synf_rate = 0;
    char *str;

    str = sprintf(str, "-A INPUT_RULE -p tcp --syn -m limit --limit %s/second --limit-burst 100 -j ACCEPT", cdb_get_int("$fw_synf_rate", 0));
    do_iptables_rule(str);
    do_iptables_rule("-A INPUT_RULE -p tcp --syn -j LOG --log-prefix \\\"[DROP sync flooding];\\\" --log-level 1");
    do_iptables_rule("-A INPUT_RULE -p tcp --syn -j DROP");
}

static void filter_drop_land_attack(void)
{
    unsigned char wanif_ip[ETHER_ADDR_LEN] = { 0 };
    unsigned char lanif_ip[ETHER_ADDR_LEN] = { 0 };
    char *str;

    get_ifname_ether_ip(mtdm->rule.WAN, wanif_ip, sizeof(wanif_ip));
    get_ifname_ether_ip(mtdm->rule.LAN, lanif_ip, sizeof(lanif_ip));

    if(!strcmp(wanif_ip, NULL)) {
       str = sprintf(str, "-A INPUT_RULE -s %s -d %s -p tcp --tcp-flags SYN SYN -j DROP", wanif_ip, wanif_ip);
       do_iptables_rule(str);
    }

    if(!strcmp(lanif_ip, NULL)) {
       str = sprintf(str, "-A INPUT_RULE -s %s -d %s -p tcp --tcp-flags SYN SYN -j DROP", lanif_ip, lanif_ip);
       do_iptables_rule(str);
    }
}

static void filter_drop_snork(void)
{
    do_iptables_rule("-A INPUT_RULE -p udp --sport 7 --dport 135 -j DROP");
    do_iptables_rule("-A INPUT_RULE -p udp --sport 19 --dport 135 -j DROP");
    do_iptables_rule("-A INPUT_RULE -p udp --sport 135 --dport 135 -j DROP");
}

static void filter_drop_udp_ploop(void)
{
    do_iptables_rule("-A INPUT_RULE -p udp -m multiport --source-port 7,17,19 -m multiport --destination-port 7,17,19 -j DROP");
}

static void filter_drop_ip_spoof(void)
{
    char lan_ip[SSBUF_LEN] = {0};
    char *lannet, *str;

    lannet = exec_cmd2("echo %s | awk -F\".\" '{ printf $1\".\"$2\".\"$3\".0\" }'");
    str = sprintf(str, "-A INPUT_RULE -s %s/24 -i %s -j DROP", lannet, mtdm->rule.WAN);
    do_iptables_rule(str);
}

static void filter_drop_ping_death(void)
{
    do_iptables_rule("-A INPUT_RULE -p icmp -m length --length 65535 -j DROP");
}

static void filter_drop_icmp_err(void)
{
    do_iptables_rule("-A OUTPUT_RULE -p icmp --icmp-type 3 -j DROP");
}

static void filter_drop_port_scan(void)
{
    do_iptables_rule("-A INPUT_RULE -p tcp --tcp-flags ALL URG,PSH,FIN -j DROP");
    do_iptables_rule("-A INPUT_RULE -p tcp --tcp-flags ALL URG,ACK,RST,SYN,FIN -j DROP");
    do_iptables_rule("-A INPUT_RULE -p tcp --tcp-flags ALL NONE -j DROP");
    do_iptables_rule("-A INPUT_RULE -p tcp --tcp-flags RST,SYN RST,SYN -j DROP");
    do_iptables_rule("-A INPUT_RULE -p tcp --tcp-flags SYN,FIN SYN,FIN -j DROP");

    do_iptables_rule("-A FORWARD_RULE -p tcp --tcp-flags ALL URG,PSH,FIN -j DROP");
    do_iptables_rule("-A FORWARD_RULE -p tcp --tcp-flags ALL URG,ACK,RST,SYN,FIN -j DROP");
    do_iptables_rule("-A FORWARD_RULE -p tcp --tcp-flags ALL NONE -j DROP");
    do_iptables_rule("-A FORWARD_RULE -p tcp --tcp-flags RST,SYN RST,SYN -j DROP");
    do_iptables_rule("-A FORWARD_RULE -p tcp --tcp-flags SYN,FIN SYN,FIN -j DROP");
}
#endif

static void fw_setup(void)
{
    fprintf(fp, "-N INPUT_RULE\n");
    fprintf(fp, "-A INPUT -j INPUT_RULE\n");
    fprintf(fp, "-N FORWARD_RULE\n");
    fprintf(fp, "-A FORWARD -j FORWARD_RULE\n");
    fprintf(fp, "-N OUTPUT_RULE\n");
    fprintf(fp, "-A OUTPUT -j OUTPUT_RULE\n");
}

static void syslog_setup(void)
{
    fprintf(fp, "-N SYSLOG\n");
    fprintf(fp, "-I INPUT 1 -j SYSLOG\n");
    fprintf(fp, "-I FORWARD 1 -j SYSLOG\n");
    fprintf(fp, "-I OUTPUT 1 -j SYSLOG\n");
}

static int start(void)
{
    fp = fopen("/tmp/filter-rule", "w+");
    if(fp) {
       fprintf(fp, "*filter\n");
       fw_setup();

       /* mac_filter */
       if(cdb_get_int("$fw_macf_mode", 0) != 0) {
          if(cdb_get_int("$fw_macf_mode", 0) == 1)
             drop_mac_access();
          else {
             accept_mac_access();
             FORWARD_DROP = 1;
          }
       }

       /* url filter */
       if(cdb_get_int("$fw_urlf_mode", 0) != 0) {
          if(cdb_get_int("$fw_urlf_mode", 0) == 1)
             drop_urlf_access();
          else
             accept_urlf_access();
       }

       /* client filter */
       if(cdb_get_int("$fw_clientf_mode", 0) != 0) {
          if(cdb_get_int("$fw_clientf_mode", 0) == 1)
             drop_client_access();
          else {
             accept_client_access();
             FORWARD_DROP = 1;
          }
       }

       if(FORWARD_DROP == 1) {
          fprintf(fp, "-I FORWARD_RULE 1 -m state --state RELATED,ESTABLISHED -j ACCEPT\n");
          fprintf(fp, "-A FORWARD_RULE -j DROP\n");
       }

       /* hacker */
       if(cdb_get_int("$fw_enable", 0) == 1) {
          syslog_setup();
          if(cdb_get_int("$fw_drop_wan_ping", 0) == 1)
             filter_drop_icmp(mtdm->rule.WAN);
          if(cdb_get_int("$fw_drop_lan_ping", 0) == 1)
             filter_drop_icmp(mtdm->rule.LAN);

// for router
#if 0
       if(cdb_get_int("$fw_drop_netbios", 0) == 1)
          filter_drop_netbios();
       if(cdb_get_int("$fw_drop_tcp_null", 0) == 1)
          filter_drop_tcpnull();
       if(cdb_get_int("$fw_drop_ident", 0) == 1)
          filter_drop_ident();
       if(cdb_get_int("$fw_drop_icmp_flood", 0) == 1)
          filter_drop_icmp_flood();
       if(cdb_get_int("$fw_drop_sync_flood", 0) == 1)
          filter_drop_sync_flood();
       if(cdb_get_int("$fw_drop_land_attack", 0) == 1)
          filter_drop_land_attack();
       if(cdb_get_int("$fw_drop_snork", 0) == 1)
          filter_drop_snork();
       if(cdb_get_int("$fw_drop_udp_ploop", 0) == 1)
          filter_drop_udp_ploop();
       if(cdb_get_int("$fw_drop_ip_spoof", 0) == 1)
          filter_drop_ip_spoof();
       if(cdb_get_int("$fw_drop_smurf", 0) == 1)
          f_write_string("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", "1 ", 0, 0);
       else
          f_write_string("/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts", "0 ", 0, 0);
       if(cdb_get_int("$fw_drop_ping_death", 0) == 1)
          filter_drop_ping_death();
       if(cdb_get_int("$fw_drop_icmp_resp", 0) == 1)
          filter_drop_icmp_err();
       if(cdb_get_int("$fw_drop_port_scan", 0) == 1)
          filter_drop_port_scan();
       if(cdb_get_int("$fw_drop_frag", 0) == 1)
          f_write_string("/proc/frag_drop", "1 ", 0, 0);
       else
          f_write_string("/proc/frag_drop", "0 ", 0, 0);
#endif
       }
       fprintf(fp, "COMMIT\n");
       fclose(fp);
    }
    do_iptables_commit();
    unlink("/tmp/filter-rule");
    return 0;
}

static int stop(void)
{
    if(f_exists("/usr/sbin/iptables-restore"))
       exec_cmd("echo -e \"*filter\nCOMMIT\" | /usr/sbin/iptables-restore");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int fw_main(montage_proc_t *proc, char *cmd)
{
    MTDM_INIT_SUB_PROC(proc);
    MTDM_SUB_PROC(sub_proc, cmd);
    return 0;
}
#endif
