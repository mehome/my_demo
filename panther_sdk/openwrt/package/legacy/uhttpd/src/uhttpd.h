/*
 * uhttpd - Tiny single-threaded httpd - Main header
 *
 *   Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _UHTTPD_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <netdb.h>
#include <ctype.h>

#include <dlfcn.h>


#ifdef HAVE_LUA
#include <lua.h>
#endif

#ifdef HAVE_TLS
#include <openssl/ssl.h>
#endif


#define UH_LIMIT_MSGHEAD	4096
#define UH_LIMIT_HEADERS	64

#define UH_LIMIT_LISTENERS	16
#define UH_LIMIT_CLIENTS	64
#define UH_LIMIT_AUTHREALMS	8

#define UH_HTTP_MSG_GET		0
#define UH_HTTP_MSG_HEAD	1
#define UH_HTTP_MSG_POST	2

#ifdef CONFIG_WEB_LOGIN
#define COOKIE_NAME		"CID"
#define COOKIE_VALUE	"CaltRtr"
#define COOKIE_ID_LENGTH 12
#define COOKIE_MAX_LENGTH 50
#define UH_LIMIT_CLIENT_AUTHS   UH_LIMIT_AUTHREALMS
#endif
#define CONFIG_UHTTPD_BUFFER
#define UHTTPD_STORAGE_LINK "/udisk"
#define UHTTPD_STORAGE_LINK_LEN (sizeof(UHTTPD_STORAGE_LINK)-1)
#define UHTTPD_STORAGE_DISK "/media"
#define UHTTPD_STORAGE_DISK_LEN (sizeof(UHTTPD_STORAGE_DISK)-1)
#define UH_LIMIT_CLIENT_INFOS	UH_LIMIT_AUTHREALMS
#define STORAGE_ID_LENGTH 8
#define HAVE_DBG
#ifdef HAVE_DBG
#define CGI_DEBUG_LOG           "/tmp/cgi.log"
#define CGI_DEBUG_LOG_1         "/tmp/cgi.log.1"
#define CGI_DEBUG_LOG_MAXSIZE   4096
#define DBG(args...) do {                           \
    FILE *debugfp;                                  \
    struct stat sb;                                 \
    if (stat(CGI_DEBUG_LOG, &sb) == 0) {            \
        if (sb.st_size >= CGI_DEBUG_LOG_MAXSIZE) {  \
            rename(CGI_DEBUG_LOG, CGI_DEBUG_LOG_1); \
        }                                           \
    }                                               \
    if ((debugfp = fopen(CGI_DEBUG_LOG, "aw+"))) {  \
        fprintf(debugfp, ##args);                   \
        fflush(debugfp);                            \
        fsync(fileno(debugfp));                     \
        fclose(debugfp);                            \
    }                                               \
} while(0)
//#define DBG(fmt, ...) printf("%s:%d " fmt, __func__, __LINE__, ## __VA_ARGS__)
#else
#define DBG(args...) 
#endif

struct listener;
struct client;
struct http_request;

struct config {
	char docroot[PATH_MAX];
	char *realm;
	char *file;
#ifdef HAVE_CGI
	char *cgi_prefix;
#endif
#ifdef HAVE_LUA
	char *lua_prefix;
	char *lua_handler;
	lua_State * (*lua_init) (const char *handler);
	void (*lua_close) (lua_State *L);
	void (*lua_request) (struct client *cl, struct http_request *req, lua_State *L);
#endif
#if defined(HAVE_CGI) || defined(HAVE_LUA)
	int script_timeout;
#endif
#ifdef HAVE_TLS
	char *cert;
	char *key;
	SSL_CTX *tls;
	SSL_CTX * (*tls_init) (void);
	int (*tls_cert) (SSL_CTX *c, const char *file);
	int (*tls_key) (SSL_CTX *c, const char *file);
	void (*tls_free) (struct listener *l);
	void (*tls_accept) (struct client *c);
	void (*tls_close) (struct client *c);
	int (*tls_recv) (struct client *c, void *buf, int len);
	int (*tls_send) (struct client *c, void *buf, int len);
#endif
};

struct listener {
	int socket;
	struct sockaddr_in6 addr;
	struct config *conf;
#ifdef HAVE_TLS
	SSL_CTX *tls;
#endif
};

struct client {
	int socket;
	int peeklen;
	char peekbuf[UH_LIMIT_MSGHEAD];
	struct listener *server;
	struct sockaddr_in6 servaddr;
	struct sockaddr_in6 peeraddr;
#ifdef HAVE_TLS
	SSL *tls;
#endif
};

#if defined(CAMELOT)
struct client_info {
	int time;
	struct sockaddr_in6 peeraddr;
	char peeraddr_str[INET6_ADDRSTRLEN+1];
	char storage_id[STORAGE_ID_LENGTH+1];
};
#endif

#ifdef CONFIG_WEB_LOGIN
typedef struct http_cookie
{
	char *name;
	char *value;
	char *max_age;
	char *domain;
	char *path;
	char *secure;
	char *version;
	char *userid;
}http_cookie;

typedef struct http_auths
{
	int time;
	char peeraddr_str[INET6_ADDRSTRLEN+1];
	char cookie_ID[COOKIE_ID_LENGTH+1];
}http_auths;
#endif

struct auth_realm {
	char path[PATH_MAX];
	char user[32];
	char pass[128];
#ifdef CONFIG_WEB_LOGIN
	char cookie_ID[COOKIE_ID_LENGTH+1];
	http_auths auths[UH_LIMIT_CLIENT_AUTHS];
#endif
	int addr;
	int logout_time;
	int logout_flag;
};

struct http_request {
	int	method;
	float version;
	char *url;
	char *headers[UH_LIMIT_HEADERS];
	struct auth_realm *realm;
	struct auth_realm auth_realm;
	short auth_userid;
#if defined(CAMELOT)
	int req_noauth;
#define FLAG_RANGE              0x00000004
	int reqflags;
	off_t req_RangeStart;
	off_t req_RangeEnd;
#endif
};

struct http_response {
	int statuscode;
	char *statusmsg;
	char *headers[UH_LIMIT_HEADERS];
#ifdef CONFIG_WEB_LOGIN
	http_cookie hc;
	char setcookie[COOKIE_MAX_LENGTH+1];
#endif
};

#if defined(CAMELOT)
int cdb_set(char *name, void* value);
int cdb_get(char *name, void* value);
void cdb_close(void);
#endif

#endif

