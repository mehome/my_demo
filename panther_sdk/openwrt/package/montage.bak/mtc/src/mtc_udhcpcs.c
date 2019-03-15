/*
 * udhcpc scripts
 *
 * Copyright (C) 2014, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: udhcpc.c 291523 2011-10-24 06:12:27Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "netutils.h"
#include "shutils.h"
#include "wdk/cdb.h"

/* format dns_str=dns1 dns2  */
int
add_dns(char *dns_str)
{
    FILE *fp;
    char word[100], *next;
    char line[100];

    /* Open resolv.conf to read */
    if (!(fp = fopen("/etc/resolv.conf", "r+"))) {
        perror("/etc/resolv.conf");
        return errno;
    }
    /* Append only those not in the original list */
    foreach(word, dns_str , next) {
        fseek(fp, 0, SEEK_SET);
        while (fgets(line, sizeof(line), fp)) {
            char *token = strtok(line, " \t\n");

            if (!token || strcmp(token, "nameserver") != 0)
                continue;
            if (!(token = strtok(NULL, " \t\n")))
                continue;

            if (!strcmp(token, word))
                break;
        }
        if (feof(fp))
            fprintf(fp, "nameserver %s\n", word);
    }
    fclose(fp);

    /* notify dnsmasq */
    killall("dnsmasq", 9);

    return 0;
}

static int expires(unsigned int in)
{
	struct sysinfo info;
	FILE *fp;

	sysinfo(&info);

	/*
	 * Save uptime ranther than system time, because the system time may
	 * change 
	 */
	if (!(fp = fopen("/tmp/udhcpc.expires", "w"))) {
		perror("/tmp/udhcpd.expires");
		return errno;
	}
	fprintf(fp, "%d", (unsigned int)info.uptime + in);
	fclose(fp);
	return 0;
}


/*
 * deconfig: This argument is used when udhcpc starts, and when a
 * leases is lost. The script should put the interface in an up, but
 * deconfigured state.
 */
static int deconfig(void)
{
	char *wan_ifname = safe_getenv("interface");

	ifconfig( wan_ifname, IFUP,  NULL, NULL);
	expires(0);

	cdb_set("$wanif_ip", "0.0.0.0");
	cdb_set("$wanif_msk", "0.0.0.0");
	cdb_set("$wanif_gw", "0.0.0.0");
	cdb_set("$wanif_dns1", "");

	unlink("/tmp/get_lease_time");
	unlink("/tmp/lease_time");
	
	eval("mtc", "wanipdown");
	eval("route", "del", "default");

	return 0;
}


/*
 * bound: This argument is used when udhcpc moves from an unbound, to
 * a bound state. All of the paramaters are set in enviromental
 * variables, The script should configure the interface, and set any
 * other relavent parameters (default gateway, dns server, etc).
*/
static int
bound(void)
{
	char *wan_ifname = safe_getenv("interface");
	char *value = NULL;
	char ip[18], mask[18], gw[18];
    char *dns = NULL;

	cdb_set("$wanif_state", "2");

	if ((value = getenv("ip"))) {
		cdb_set("$wanif_ip", value);
		strncpy(ip, value, sizeof(ip));
		//output_console("IP = %s\n" , ip);
	}
	if ((value = getenv("subnet"))) {
		cdb_set("$wanif_msk", value);
		strncpy(mask, value, sizeof(mask));
		//output_console("MASK = %s\n" , mask);
	}
	if ((value = getenv("router"))) {	
		cdb_set("$wanif_gw", value);
		//output_console("GW = %s\n" , value);
		strncpy(gw, value, sizeof(gw));
	}
	if ((value = getenv("namesrv"))) {
		//cdb_set("$wanif_dns1", value);
		//output_console("NAMESRV = %s\n" , value);
	}
	if ((value = getenv("dns"))) {
        char *ptr = value;
		//output_console("DNS = %s\n" , value);
        dns = strdup(value);
        while (*ptr++) {
            if (*ptr == ' ') {
                *ptr = 0;
                break;
            }
        }
		cdb_set("$wanif_dns1", value);
	}
	if ((value = getenv("domain"))) {
		cdb_set("$wanif_domain", value);
		//output_console("DOMAIN = %s\n" , value);
	}
	if ((value = getenv("ntpsrv"))) {
		cdb_set("$sys_ntp_svr1", value);
		//output_console("NTPSRV = %s\n" , value);
	}
	
	if ((value = getenv("lease"))) {
		//output_console("LEASE = %s\n" , value);
		expires(atoi(value));
	}

	ifconfig(wan_ifname, IFUP, ip , mask );

	eval("route", "del", "default");
	route_add(NULL, 0, "0.0.0.0", gw, "0.0.0.0");
	route_add(wan_ifname, 0, ip, gw, mask);

	/* Add dns servers to resolv.conf */
    if (dns) {
    	add_dns(dns);
        free(dns);
    }
    
	cdb_set("$omi_result", "2");
	cdb_set("$omi_ap_scan_status", "1");
	cdb_set("$scan_result", "1");  
	eval("mtc", "ocfg", "stop");
	eval("mtc", "ocfgarg");
	eval("mtc", "wanipup");

	return 0;
}

/*
 * renew: This argument is used when a DHCP lease is renewed. All of
 * the paramaters are set in enviromental variables. This argument is
 * used when the interface is already configured, so the IP address,
 * will not change, however, the other DHCP paramaters, such as the
 * default gateway, subnet mask, and dns server may change.
 */
static int
renew(void)
{
	bound();

	return 0;
}

int main(int argc, char **argv)
{
	//output_console("mtc_udhcpcs = %s , 1 = %s\n", argv[0] , argv[1] );
	if (!argv[1])
		return EINVAL;
		
	else if (strstr(argv[1], "deconfig"))
		return deconfig();
	else if (strstr(argv[1], "bound"))
		return bound();
	else if (strstr(argv[1], "renew"))
		return renew();
	else
		return EINVAL;
}

