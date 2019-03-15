/*
 * uhttpd - Tiny single-threaded httpd - Utility header
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

#ifndef _UHTTPD_UTILS_

#include <stdarg.h>
#include <fcntl.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/stat.h>

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define array_size(x) \
	(sizeof(x) / sizeof(x[0]))

#define foreach_header(i, h) \
	for( i = 0; (i + 1) < (sizeof(h) / sizeof(h[0])) && h[i]; i += 2 )

#define fd_cloexec(fd) \
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC)

struct path_info {
	char *root;
	char *phys;
	char *name;
	char *info;
	char *query;
	struct stat stat;
};


const char * sa_straddr(void *sa);
const char * sa_strport(void *sa);
int sa_port(void *sa);

char *strfind(char *haystack, int hslen, const char *needle, int ndlen);

#if defined(CAMELOT)
char *mystrstr(char *s, char *str);
char *strrstr(char *s, char *str);
void get_random_bytes(void *buf, int nbytes);
#endif

int select_intr(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t);

int uh_tcp_send(struct client *cl, const char *buf, int len);
int uh_tcp_peek(struct client *cl, char *buf, int len);
int uh_tcp_recv(struct client *cl, char *buf, int len);

int uh_http_sendhf(
	struct client *cl, int code, const char *summary,
	const char *fmt, ...
);

#define uh_http_response(cl, code, message) \
	uh_http_sendhf(cl, code, message, message)

int uh_http_sendc(struct client *cl, const char *data, int len);

int uh_http_sendf(
	struct client *cl, struct http_request *req,
	const char *fmt, ...
);

int uh_http_send(
	struct client *cl, struct http_request *req,
	const char *buf, int len
);


int uh_urldecode(char *buf, int blen, const char *src, int slen);
int uh_urlencode(char *buf, int blen, const char *src, int slen);
int uh_b64decode(char *buf, int blen, const unsigned char *src, int slen);


struct auth_realm * uh_auth_add(char *path, char *user, char *pass);

#ifdef CONFIG_WEB_LOGIN
int uh_web_auth_check(struct client *cl, struct http_request *req, struct path_info *pi);
int uh_realm_set_cookie(struct client *cl, struct http_response *res, struct path_info *pi);
int http_process_cookie(http_cookie* hc, char *cookie_p);
void http_store_cookie(void);
void http_restore_cookie(void);
#else
int uh_auth_check(struct client *cl, struct http_request *req, struct path_info *pi);
#endif

#if defined(CAMELOT)
struct client_info * uh_client_info_add(struct client *cl);
struct client_info * uh_client_info_lookup(struct client *cl);
#ifdef HAVE_DBG
void uh_client_info_dump(void);
#endif
#endif

struct path_info * uh_path_lookup(struct client *cl, const char *url);

struct listener * uh_listener_add(int sock, struct config *conf);
struct listener * uh_listener_lookup(int sock);

struct client * uh_client_add(int sock, struct listener *serv);
struct client * uh_client_lookup(int sock);
void uh_client_remove(int sock);
void uh_auth_logout(void);

#endif
