/*
 * RTSP protocol handler. This file is part of Shairport.
 * Copyright (c) James Laird 2013
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <openssl/md5.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "common.h"
#include "player.h"
#include "rtp.h"
#include "mdns.h"

#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/domain.h>
#include <avahi-common/llist.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

int shair_pid;
// only one thread is allowed to use the player at once.
// it monitors the request variable (at least when interrupted)
static pthread_mutex_t playing_mutex = PTHREAD_MUTEX_INITIALIZER;
static int please_shutdown = 0;
static pthread_t dacp_update_thread;

//DACP CTRL
#if 0
typedef struct {
	char addr[32];
	int port;
} host_t;
#endif
typedef struct {
	AvahiSimplePoll *poll;
	AvahiClient *cli;
	AvahiServiceBrowser *br;
	int ref_nr;
	struct {
		char *name_prefix;
		char *type;
	} match;
	host_t *found;
} ctx_t;

typedef struct {
	ctx_t *c;
	AvahiServiceResolver *resolver;
} srvctx_t;

static void ctx_cleanup(ctx_t * c)
{
	if (c->found)
		free(c->found);
}

static void ctx_ref(ctx_t * c)
{
	c->ref_nr++;
}

#if 0
typedef struct {
	host_t *host;
	char *dacp_id;
	char *srv_name;
	char *active_remote;
} srv_t;
#endif
srv_t dacp_srv;

static void *memdup(void *p, int size)
{
	void *r = malloc(size);
	memcpy(r, p, size);
	return r;
}

static void ctx_unref(ctx_t * c)
{
	c->ref_nr--;
	if (c->ref_nr == 0)
		avahi_simple_poll_quit(c->poll);
}

static int startswith(char *s, char *pat)
{
	int n = strlen(pat);
	return !strncmp(s, pat, n);
}

static int ctx_ismatch(ctx_t * c, const char *name)
{
	if (c->match.name_prefix)
		return startswith((char *)name, c->match.name_prefix);
	return 1;
}

static srvctx_t *srvctx_new(ctx_t * c)
{
	srvctx_t *sc = (srvctx_t *) malloc(sizeof(srvctx_t));
	sc->c = c;
	ctx_ref(c);
	return sc;
}

static srvctx_t *srvctx_unref(srvctx_t * sc)
{
	ctx_unref(sc->c);
	if (sc->resolver)
		avahi_service_resolver_free(sc->resolver);
	free(sc);
}

static const char *browser_event_to_string(AvahiBrowserEvent event)
{
	switch (event) {
	case AVAHI_BROWSER_NEW:
		return "NEW";
	case AVAHI_BROWSER_REMOVE:
		return "REMOVE";
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		return "CACHE_EXHAUSTED";
	case AVAHI_BROWSER_ALL_FOR_NOW:
		return "ALL_FOR_NOW";
	case AVAHI_BROWSER_FAILURE:
		return "FAILURE";
	}
	return "?";
}

static void service_resolver_callback(AvahiServiceResolver * r,
				      AvahiIfIndex interface,
				      AvahiProtocol protocol,
				      AvahiResolverEvent event,
				      const char *name,
				      const char *type,
				      const char *domain,
				      const char *host_name,
				      const AvahiAddress * a,
				      uint16_t port,
				      AvahiStringList * txt,
				      AVAHI_GCC_UNUSED AvahiLookupResultFlags
				      flags, void *ud)
{
	srvctx_t *sc = (srvctx_t *) ud;
	switch (event) {
	case AVAHI_RESOLVER_FOUND:{
			char address[AVAHI_ADDRESS_STR_MAX];
			avahi_address_snprint(address, sizeof(address), a);
			debug(DACP, "found: name=%s type=%s addr=%s port=%d\n",
			      name, type, address, port);
			ctx_t *c = sc->c;
			if (ctx_ismatch(c, name)) {
				if (c->found)
					free(c->found);
				c->found = (host_t *) malloc(sizeof(host_t));
				strcpy(c->found->addr, address);
				c->found->port = port;
			} else {
				debug(DACP,
				      "resolve: name not match(%s vs %s)\n",
				      name, c->match.name_prefix);
			}
			srvctx_unref(sc);
			break;
		}
	case AVAHI_RESOLVER_FAILURE:{
			srvctx_unref(sc);
			break;
		}
	}
}

static void service_browser_callback(AvahiServiceBrowser * b,
				     AvahiIfIndex interface,
				     AvahiProtocol protocol,
				     AvahiBrowserEvent event,
				     const char *name,
				     const char *type,
				     const char *domain,
				     AvahiLookupResultFlags flags, void *ud)
{
	ctx_t *c = (ctx_t *) ud;
	debug(DACP, "resolve: browser event=%s if=%d prot=%d type=%s name=%s\n",
	      browser_event_to_string(event), interface, protocol, type, name);
	switch (event) {
	case AVAHI_BROWSER_NEW:{
			srvctx_t *sc = srvctx_new(c);
			sc->resolver = avahi_service_resolver_new(c->cli,
								  interface,
								  protocol,
								  name, type,
								  domain,
								  AVAHI_PROTO_UNSPEC,
								  0,
								  service_resolver_callback,
								  sc);
			if (sc->resolver == NULL)
				srvctx_unref(sc);
			break;
		}
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		ctx_unref(c);
		break;
	}
}

static void client_callback(AvahiClient * cli, AvahiClientState state, void *ud)
{
	ctx_t *c = (ctx_t *) ud;
	switch (state) {
	case AVAHI_CLIENT_FAILURE:
		ctx_unref(c);
		break;
	case AVAHI_CLIENT_S_REGISTERING:
	case AVAHI_CLIENT_S_RUNNING:
	case AVAHI_CLIENT_S_COLLISION:{
			c->br = avahi_service_browser_new(cli,
							  AVAHI_IF_UNSPEC,
							  AVAHI_PROTO_INET,
							  c->match.type,
							  NULL,
							  0,
							  service_browser_callback,
							  c);
		}
		break;
	}
}

static void ctx_find_service(ctx_t * c)
{

	c->poll = avahi_simple_poll_new();
	if (c->poll == NULL) {
		goto cleanup;
	}

	c->ref_nr = 1;
	int err;
	c->cli =
	    avahi_client_new(avahi_simple_poll_get(c->poll), 0, client_callback,
			     c, &err);
	if (c->cli == NULL) {
		goto cleanup;
	}

	avahi_simple_poll_loop(c->poll);
cleanup:
	if (c->br)
		avahi_service_browser_free(c->br);
	if (c->cli)
		avahi_client_free(c->cli);
	if (c->poll)
		avahi_simple_poll_free(c->poll);
}

void dacp_update_thread_func(struct dacp_info *dacp_ctx)
{
	srv_t *dacp = &dacp_srv;
	char *dacpstr = (char *)malloc(strlen(dacp_ctx->dacp_id) + 128);
	char *activestr = (char *)malloc(strlen(dacp_ctx->active_remote) + 128);
	unsigned int times = 1;

	if (dacp->srv_name) {
		free(dacp->srv_name);
		dacp->srv_name = NULL;
	}

	if (dacp->active_remote) {
		free(dacp->active_remote);
		dacp->active_remote = NULL;
	}

	if (dacp->host) {
		free(dacp->host);
		dacp->host = NULL;
	}

	sprintf(dacpstr, "iTunes_Ctrl_%s", dacp_ctx->dacp_id);
	sprintf(activestr, "%s", dacp_ctx->active_remote);
	ctx_t rsv = {.match = {.name_prefix = dacpstr,.type = "_dacp._tcp"}
	};

	warn("DACP THREAD id:%d", getpid());
	do {
		ctx_find_service(&rsv);

		if (rsv.found) {
			dacp->srv_name = strdup(dacpstr);
			dacp->active_remote = strdup(activestr);
			dacp->host =
			    (host_t *) memdup(rsv.found, sizeof(host_t));
			warn("----- Found Control point -----");
		} else {
			warn("Times:%d Resolve: srv=%s dacp_ctx->dacp_id:%s failed\n\n", times, dacpstr, dacp_ctx->dacp_id);
		}
		ctx_cleanup(&rsv);
		sleep(1);
		times++;
	} while (!rsv.found && (times < 10));

	free(dacpstr);
	free(activestr);
}
