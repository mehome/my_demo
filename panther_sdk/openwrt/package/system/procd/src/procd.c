/*
 * Copyright (C) 2013 Felix Fietkau <nbd@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reboot.h>

#include <unistd.h>
#include <getopt.h>
#include <libgen.h>

#include "procd.h"
#include "watchdog.h"
#include "plug/hotplug.h"

unsigned int debug;

static int usage(const char *prog)
{
	ERROR("Usage: %s [options]\n"
		"Options:\n"
		"\t-s <path>\tPath to ubus socket\n"
		"\t-h <path>\trun as hotplug daemon\n"
		"\t-d <level>\tEnable debug messages\n"
		"\n", prog);
	return 1;
}

#if defined(__PANTHER__)

//#define ENABLE_COREDUMP

#if defined(ENABLE_COREDUMP)
#include <ulimit.h>
#include <sys/resource.h>
#endif

#endif

int main(int argc, char **argv)
{
	int ch;
	char *dbglvl = getenv("DBGLVL");

#if defined(__PANTHER__) && defined(ENABLE_COREDUMP)
	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;

	setrlimit(RLIMIT_CORE, &core_limits);

	system("/bin/echo \"/tmp/core.%e.%p\" >  /proc/sys/kernel/core_pattern");
#endif

	ulog_open(ULOG_KMSG, LOG_DAEMON, "procd");

	if (dbglvl) {
		debug = atoi(dbglvl);
		unsetenv("DBGLVL");
	}

	while ((ch = getopt(argc, argv, "d:s:h:")) != -1) {
		switch (ch) {
		case 'h':
			return hotplug_run(optarg);
		case 's':
			ubus_socket = optarg;
			break;
		case 'd':
			debug = atoi(optarg);
			break;
		default:
			return usage(argv[0]);
		}
	}
	setsid();
	uloop_init();
	procd_signal();
	trigger_init();


	if (getpid() != 1)
		procd_connect_ubus();
	else
		procd_state_next();
	uloop_run();
	uloop_done();

	return 0;
}
