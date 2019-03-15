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

#include <fcntl.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "procd.h"
#include "syslog.h"
#include "plug/hotplug.h"
#include "watchdog.h"
#include "service/service.h"
#include "utils/utils.h"
#include <errno.h>

enum {
	STATE_NONE = 0,
	STATE_EARLY,
	STATE_UBUS,
	STATE_INIT,
	STATE_RUNNING,
	STATE_SHUTDOWN,
	STATE_HALT,
	__STATE_MAX,
};

static int state = STATE_NONE;
static int reboot_event;

static void set_stdio(const char* tty)
{
	if (chdir("/dev") ||
	    !freopen(tty, "r", stdin) ||
	    !freopen(tty, "w", stdout) ||
	    !freopen(tty, "w", stderr) ||
	    chdir("/"))
		ERROR("failed to set stdio\n");
	else
		fcntl(STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL) | O_NONBLOCK);
}

static void set_console(void)
{
	const char* tty;
	char* split;
	char line[ 20 ];
	const char* try[] = { "tty0", "console", NULL }; /* Try the most common outputs */
	int f, i = 0;

	tty = get_cmdline_val("console",line,sizeof(line));
	if (tty != NULL) {
		split = strchr(tty, ',');
		if ( split != NULL )
			*split = '\0';
	} else {
		// Try a default
		tty=try[i];
		i++;
	}

	if (chdir("/dev")) {
		ERROR("failed to change dir to /dev\n");
		return;
	}
	while (tty!=NULL) {
		f = open(tty, O_RDONLY);
		if (f >= 0) {
			close(f);
			break;
		}

		tty=try[i];
		i++;
	}
	if (chdir("/"))
		ERROR("failed to change dir to /\n");

	if (tty != NULL)
		set_stdio(tty);
}

#if defined(__PANTHER__)
#include <sys/stat.h>
#include <alsa/asoundlib.h>

static void snd_dev_init(char *dev_name)
{
	snd_pcm_t *handle;

	if(snd_pcm_open(&handle, dev_name, SND_PCM_STREAM_PLAYBACK, 0) == 0)
		snd_pcm_close(handle);
		else
	printf("default open failed %d\n", errno);
}
#define STARTUP_SOUND_FILE "/res/starting_up.wav"
static void play_startup_sound(void)
{
	struct stat buf;
	if(stat(STARTUP_SOUND_FILE, &buf) == 0)
	{
//		system("/usr/bin/amixer set All 70% > /dev/null");
//		system("/usr/bin/aplay " STARTUP_SOUND_FILE " &");
	}
}

static const char* get_card0_dev0_name()
{
    int err;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);
    if ((err = snd_ctl_open(&handle, "hw:0", 0)) < 0) {
        //printf("control open : %s", snd_strerror(err));
        return NULL;
    }
    if ((err = snd_ctl_card_info(handle, info)) < 0) {
        //printf("control hardware info %s", snd_strerror(err));
        snd_ctl_close(handle);
        return NULL;
    }
    snd_pcm_info_set_device(pcminfo, 0);
    snd_pcm_info_set_subdevice(pcminfo, 0);
    snd_pcm_info_set_stream(pcminfo, 1);

    err = snd_ctl_pcm_info(handle, pcminfo);
    if(err == 0)
    {
        return snd_pcm_info_get_id(pcminfo);
    }
    return NULL;
}

/*
 * open capture for pcmwb - For align time slot in TDM mode (MicArray)
 * open playback for aloop-capture - The offset of aloop-capture updating caused
 * by tx_interrupt in the kernel (see purin-dma.c)
 */
static void preopen_capture_and_playback()
{
	snd_pcm_t *capture_handle;
	snd_pcm_t *playback_handle;
	const char *capture_name = get_card0_dev0_name();
	int ret = 0;
	//printf("capture %s...\n", capture_name);
	if(capture_name != NULL && 
		( strstr(capture_name, "ES7243") != NULL || strstr(capture_name, "ES7210") != NULL ) 
		) {
		// for tdm time-slot
			ret = snd_pcm_open(&capture_handle, "micArray", SND_PCM_STREAM_CAPTURE, 0);
		}
		
		if(ret == 0) {
			// preopen for aloop
			ret = snd_pcm_open(&playback_handle, "both", SND_PCM_STREAM_PLAYBACK, 0);
			if (ret != 0) {
				printf("both open failed %d\n", errno);
			}
			// Initialize for amixer to find "All"
			snd_dev_init("default");

	}

}
#endif

static void state_enter(void)
{
	char ubus_cmd[] = "/sbin/ubusd";

	switch (state) {
	case STATE_EARLY:
		LOG("- early -\n");
		watchdog_init(0);
		hotplug("/etc/hotplug.json");
		procd_coldplug();
		break;

	case STATE_UBUS:
		// try to reopen incase the wdt was not available before coldplug
		#if defined(__PANTHER__)
		preopen_capture_and_playback();
		play_startup_sound();
		#endif
		watchdog_init(0);
		set_stdio("console");
		LOG("- ubus -\n");
		procd_connect_ubus();
		service_init();
		service_start_early("ubus", ubus_cmd);
		break;

	case STATE_INIT:
		LOG("- init -\n");
		procd_inittab();
		procd_inittab_run("respawn");
		procd_inittab_run("askconsole");
		procd_inittab_run("askfirst");
		procd_inittab_run("sysinit");

		// switch to syslog log channel
		ulog_open(ULOG_SYSLOG, LOG_DAEMON, "procd");
		break;

	case STATE_RUNNING:
		LOG("- init complete -\n");
		break;

	case STATE_SHUTDOWN:
		/* Redirect output to the console for the users' benefit */
		set_console();
		LOG("- shutdown -\n");
		procd_inittab_run("shutdown");
		sync();
		break;

	case STATE_HALT:
		// To prevent killed processes from interrupting the sleep
		signal(SIGCHLD, SIG_IGN);
		LOG("- SIGTERM processes -\n");
		kill(-1, SIGTERM);
		sync();
		sleep(1);
		LOG("- SIGKILL processes -\n");
		kill(-1, SIGKILL);
		sync();
		sleep(1);
		if (reboot_event == RB_POWER_OFF)
			LOG("- power down -\n");
		else
			LOG("- reboot -\n");

		/* Allow time for last message to reach serial console, etc */
		sleep(1);

		/* We have to fork here, since the kernel calls do_exit(EXIT_SUCCESS)
		 * in linux/kernel/sys.c, which can cause the machine to panic when
		 * the init process exits... */
		if (!vfork( )) { /* child */
			reboot(reboot_event);
			_exit(EXIT_SUCCESS);
		}

		while (1)
			sleep(1);
		break;

	default:
		ERROR("Unhandled state %d\n", state);
		return;
	};
}

void procd_state_next(void)
{
	DEBUG(4, "Change state %d -> %d\n", state, state + 1);
	state++;
	state_enter();
}

void procd_state_ubus_connect(void)
{
	if (state == STATE_UBUS)
		procd_state_next();
}

void procd_shutdown(int event)
{
	if (state >= STATE_SHUTDOWN)
		return;
	DEBUG(2, "Shutting down system with event %x\n", event);
	reboot_event = event;
	state = STATE_SHUTDOWN;
	state_enter();
}
