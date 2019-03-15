#include <stdio.h>
#include <wdk/cdb.h>
#include <libutils/shutils.h>
#include <libutils/netutils.h>

#define DEFAULT_IF      "mon0"
#define WLAN0_IF        "wlan0"
#define STA0_IF         "sta0"
#define TX_REPEAT_MAGIC 88888888

void print_help(void)
{
	printf("help\n");
}
int main(int argc, char *argv[])
{
	int cnt, len, burst;
	pid_t pid;
	int ret;

	if(argc < 2)
	{
		goto help;
	}
	if(!strcmp("init", argv[1]))
	{
		ifconfig(WLAN0_IF, 0, NULL, NULL);
		ifconfig(STA0_IF, 0, NULL, NULL);

		if(0 != ifconfig(DEFAULT_IF, IFUP, NULL, NULL))
		{
			cdb_set_int("$wt_start", 0);
			exec_cmd2("wd sniffer_off 1");
			exec_cmd2("iw phy phy0 interface add mon0 type monitor");
			ifconfig(DEFAULT_IF, IFUP, NULL, NULL);
		}
	}
	else if(!strcmp("deinit", argv[1]))
	{
		exec_cmd2("wd sniffer_off 0");
		ifconfig(DEFAULT_IF, 0, NULL, NULL);
	}
	else if(!strcmp("start", argv[1]))
	{
		if(cdb_get_int("$wt_start", 0))
		{
			printf("wt already started\n");
			return 0;
		}
		cdb_set_int("$wt_start", 1);
		/* wt tx mode */
		if(!cdb_get_int("$wt_rxmode", 0))
		{
			cnt = cdb_get_int("$wt_txcnt", 0);
			len = cdb_get_int("$wt_txlen", 0);
			burst = cdb_get_int("$wt_txburst", 0);
			pid = fork();
			if(!pid)
				send_pkt(cnt, len, burst);
		}
		else /* wt rx mode */
		{
		}
	}
	else if(!strcmp("stop", argv[1]))
	{
		if(cdb_get_int("$wt_start", 0))
		{
			cdb_set_int("$wt_start", 0);
			exec_cmd2("killall wt");
		}
	}
	else if(!strcmp("tx", argv[1]))
	{
		if(argc > 2)
		{
			/* use magic number to instead of repeat mode cause cdb can not store negative number */
			if(!strcmp("repeat", argv[2]))
				cnt = TX_REPEAT_MAGIC;
			else
				cnt = atoi(argv[2]);
			cdb_set_int("$wt_txcnt", cnt);
		}
		else
			goto help;

		if(argc > 3)
		{
			len = atoi(argv[3]);
			cdb_set_int("$wt_txlen", len);
		}
		if(argc > 4)
		{
			burst = atoi(argv[4]);
			cdb_set_int("$wt_txburst", burst);
		}
		cdb_set_int("$wt_rxmode", 0);
	}
	else if(!strcmp("rx", argv[1]))
	{
		cdb_set_int("$wt_rxmode", 1);
	}
	else if(!strcmp("chan", argv[1]))
	{
		if(argc == 3)
			exec_cmd2("iw dev mon0 set channel %d", atoi(argv[2]));
		else if(argc == 4)
		{
			if(1 == atoi(argv[3]))
				exec_cmd2("iw dev mon0 set channel %d HT40+", atoi(argv[2]));
			else if(3 == atoi(argv[3]))
				exec_cmd2("iw dev mon0 set channel %d HT40-", atoi(argv[2]));
			else
				goto help;
		}
		else
			goto help;
	}
	else if(!strcmp("txrate", argv[1]))
	{
		if(argc == 3)
			exec_cmd2("wd txrate %d", atoi(argv[2]));
	}
	return 0;
help:
	print_help();

	return -1;
}
