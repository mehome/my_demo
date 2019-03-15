/*
 * uhttpd - Tiny single-threaded httpd - Utility functions
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

#define _XOPEN_SOURCE 500	/* crypt() */
#define _BSD_SOURCE			/* strcasecmp(), strncasecmp() */

#if defined(CAMELOT)
#include <sys/time.h>
#endif

#include <string.h>
#include "uhttpd.h"
#include "uhttpd-utils.h"
#include "uhttpd-file.h"

#ifdef HAVE_TLS
#include "uhttpd-tls.h"
#endif

static int idle_time, uh_auth_ok=0;
#ifndef CONFIG_WEB_LOGIN
static char *g_user = NULL;
static char *g_pass = NULL;
#endif
static char *g_path = NULL;

inline void uh_auth_logout_user(struct auth_realm *user);

static char *uh_index_files[] =
{
#ifdef CONFIG_WEB_LOGIN
	"index_web.htm",
	"web_login.htm",
#else
	"index.html",
	"index.htm",
	"default.html",
	"default.htm"
#endif
};

#ifdef CONFIG_WEB_LOGIN
static char *uh_noauth_files[] =
{
	"/index_web.htm",
	"/index_web_error.htm",
	"/web_login.htm",
	"/web_login_error.htm",
	"/check_login.cgi",

#ifdef CONFIG_DEFAULT_hame-ui
	"/system_status.htm",
	"/forbidden.htm",
	"/home/Login.css",
	"/js/product.js",
	"/js/wdk.js",
	"/lang/en/home.xml",
	"/lang/zhcn/home.xml",
	"/graphics/ajax-loader.gif",
	"/home/logo.gif",
	"/home/lock_t.gif",
	"/home/lock_b.gif",
	"/home/bg.gif",
	"/home/video1.png",
	"/home/adsl1.png",
	"/home/3g.png",
	"/home/wifi.png",
	"/home/loginBox.gif",
	"/home/login_Button.gif",
#endif

#ifdef CONFIG_HOTSPOT
	"/hotspotlogin.htm"
#endif
};
#endif

const char * sa_straddr(void *sa)
{
	static char str[INET6_ADDRSTRLEN];
	struct sockaddr_in *v4 = (struct sockaddr_in *)sa;
	struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)sa;

	if( v4->sin_family == AF_INET )
		return inet_ntop(AF_INET, &(v4->sin_addr), str, sizeof(str));
	else
		return inet_ntop(AF_INET6, &(v6->sin6_addr), str, sizeof(str));
}

const char * sa_strport(void *sa)
{
	static char str[6];
	snprintf(str, sizeof(str), "%i", sa_port(sa));
	return str;
}

int sa_port(void *sa)
{
	return ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
}

/* Simple strstr() like function that takes len arguments for both haystack and needle. */
char *strfind(char *haystack, int hslen, const char *needle, int ndlen)
{
	int match = 0;
	int i, j;

	for( i = 0; i < hslen; i++ )
	{
		if( haystack[i] == needle[0] )
		{
			match = ((ndlen == 1) || ((i + ndlen) <= hslen));

			for( j = 1; (j < ndlen) && ((i + j) < hslen); j++ )
			{
				if( haystack[i+j] != needle[j] )
				{
					match = 0;
					break;
				}
			}

			if( match )
				return &haystack[i];
		}
	}

	return NULL;
}

#if defined(CAMELOT)
char *mystrstr(char *s, char *str)
{
	if ( s && str )
		return strstr(s, str);
	else
		return NULL;
}
       
char *strrstr(char *s, char *str)
{
	char *p;
	int l, len;

	if ( s && str )
	{
		l = strlen(s);
		len = strlen(str);
		for (p = s + l - 1; p >= s; p--)
		{
			if ((*p == *str) && (memcmp(p, str, len) == 0))
				return p;
		}
	}
	return NULL;
}

void get_random_bytes(void *buf, int nbytes)
{
	const char alphanumeric[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                                  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                                  'U', 'V', 'W', 'X', 'Y', 'Z',
                                  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                                  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
                                  'u', 'v', 'w', 'x', 'y', 'z',
                                  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
                                };
	char *pbuf = (char *)buf;
	struct timeval  tv;
	int i;

	gettimeofday(&tv, 0);
	for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--)
		rand();
	memset(pbuf, 0, nbytes);
	for (i=0; i<nbytes; i++)
	{
		pbuf[i] = alphanumeric[(rand() % sizeof(alphanumeric))];
	}
}
#endif

/* interruptable select() */
int select_intr(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
	int rv;
	sigset_t ssn, sso;

	/* unblock SIGCHLD */
	sigemptyset(&ssn);
	sigaddset(&ssn, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &ssn, &sso);

	rv = select(n, r, w, e, t);

	/* restore signal mask */
	sigprocmask(SIG_SETMASK, &sso, NULL);

	return rv;
}

int uh_tcp_send(struct client *cl, const char *buf, int len)
{
#ifdef CONFIG_UHTTPD_BUFFER
	static char uhbuf[1024];
	static int uhlen = 0;
	static char *uhp = uhbuf;

	if (len == 0)
	{
		/* flush buffer */
		uhp = uhbuf;
		len = uhlen;
		uhlen = 0;
#ifdef HAVE_TLS
		if( cl->tls )
			return cl->server->conf->tls_send(cl, (void *)uhbuf, len);
		else
#endif
			return send(cl->socket, uhbuf, len, 0);
	}
	else
	{
		if (uhlen + len > 1024)
		{
			if (uhlen > 0)
			{
#ifdef HAVE_TLS
				if( cl->tls )
					cl->server->conf->tls_send(cl, (void *)uhbuf, uhlen);
				else
#endif
					send(cl->socket, uhbuf, uhlen, 0);
				uhp = uhbuf;
				uhlen = 0;
			}

			if (uhlen + len > 1024)
			{
#ifdef HAVE_TLS
				if( cl->tls )
					return cl->server->conf->tls_send(cl, (void *)buf, len);
				else
#endif
					return send(cl->socket, buf, len, 0);
			}
		}

		memcpy(uhp, buf, len);
		uhlen += len;
		uhp += len;
		return len;
	}
#else
	fd_set writer;
	struct timeval timeout;

	FD_ZERO(&writer);
	FD_SET(cl->socket, &writer);

	timeout.tv_sec = 0;
	timeout.tv_usec = 20000;//500000;

	if( select(cl->socket + 1, NULL, &writer, NULL, &timeout) > 0 )
	{
#ifdef HAVE_TLS
		if( cl->tls )
			return cl->server->conf->tls_send(cl, (void *)buf, len);
		else
#endif
			return send(cl->socket, buf, len, 0);
	}

	return -1;
#endif
}

int uh_tcp_peek(struct client *cl, char *buf, int len)
{
	int sz = uh_tcp_recv(cl, buf, len);

	/* store received data in peek buffer */
	if( sz > 0 )
	{
		cl->peeklen = sz;
		memcpy(cl->peekbuf, buf, sz);
	}

	return sz;
}

int uh_tcp_recv(struct client *cl, char *buf, int len)
{
	int sz = 0;
	int rsz = 0;

	/* first serve data from peek buffer */
	if( cl->peeklen > 0 )
	{
		sz = min(cl->peeklen, len);
		len -= sz; cl->peeklen -= sz;

		memcpy(buf, cl->peekbuf, sz);
		memmove(cl->peekbuf, &cl->peekbuf[sz], cl->peeklen);
	}

	/* caller wants more */
	if( len > 0 )
	{
#ifdef HAVE_TLS
		if( cl->tls )
			rsz = cl->server->conf->tls_recv(cl, (void *)&buf[sz], len);
		else
#endif
			rsz = recv(cl->socket, (void *)&buf[sz], len, 0);

		if( (sz == 0) || (rsz > 0) )
			sz += rsz;
	}

	return sz;
}

#define ensure(x) \
	do { if( x < 0 ) return -1; } while(0)

int uh_http_sendhf_2(struct client *cl, int code, const char *summary, const char *type, const char *fmt, ...)
{
	va_list ap;

	char buffer[UH_LIMIT_MSGHEAD];
	int len;

	len = snprintf(buffer, sizeof(buffer),
		"HTTP/1.1 %03i %s\r\n"
		"Connection: close\r\n"
		"Cache-control: no-cache, must-revalidate\r\n"
		"Pragma: no-cache\r\n"
		"Expires: -1\r\n"
		"Content-Type: %s\r\n"
		"Transfer-Encoding: chunked\r\n\r\n",
			code, summary, type
	);

	ensure(uh_tcp_send(cl, buffer, len));

	va_start(ap, fmt);
	len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	ensure(uh_http_sendc(cl, buffer, len));
	ensure(uh_http_sendc(cl, NULL, 0));

	return 0;
}

int uh_http_sendhf(struct client *cl, int code, const char *summary, const char *fmt, ...)
{
	return uh_http_sendhf_2(cl, code, summary, "text/plain", fmt);
}

int uh_http_sendc(struct client *cl, const char *data, int len)
{
	char chunk[8];
	int clen;

	if( len == -1 )
		len = strlen(data);

	if( len > 0 )
	{
	 	clen = snprintf(chunk, sizeof(chunk), "%X\r\n", len);
		ensure(uh_tcp_send(cl, chunk, clen));
		ensure(uh_tcp_send(cl, data, len));
		ensure(uh_tcp_send(cl, "\r\n", 2));
	}
	else
	{
		ensure(uh_tcp_send(cl, "0\r\n\r\n", 5));
	}

	return 0;
}

int uh_http_sendf(
	struct client *cl, struct http_request *req, const char *fmt, ...
) {
	va_list ap;
	char buffer[UH_LIMIT_MSGHEAD];
	int len;

	va_start(ap, fmt);
	len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	if( (req != NULL) && (req->version > 1.0) )
		ensure(uh_http_sendc(cl, buffer, len));
	else if( len > 0 )
		ensure(uh_tcp_send(cl, buffer, len));

	return 0;
}

int uh_http_send(
	struct client *cl, struct http_request *req, const char *buf, int len
) {
	if( len < 0 )
		len = strlen(buf);

	if( (req != NULL) && (req->version > 1.0) )
		ensure(uh_http_sendc(cl, buf, len));
	else if( len > 0 )
		ensure(uh_tcp_send(cl, buf, len));

	return 0;
}

int uh_urldecode(char *buf, int blen, const char *src, int slen)
{
	int i;
	int len = 0;

#define hex(x) \
	(((x) <= '9') ? ((x) - '0') : \
		(((x) <= 'F') ? ((x) - 'A' + 10) : \
			((x) - 'a' + 10)))

	for( i = 0; (i <= slen) && (i <= blen); i++ )
	{
		if( src[i] == '%' )
		{
			if( ((i+2) <= slen) && isxdigit(src[i+1]) && isxdigit(src[i+2]) )
			{
				buf[len++] = (char)(16 * hex(src[i+1]) + hex(src[i+2]));
				i += 2;
			}
			else
			{
				buf[len++] = '%';
			}
		}
		else
		{
			buf[len++] = src[i];
		}
	}

	return len;
}

int uh_urlencode(char *buf, int blen, const char *src, int slen)
{
	int i;
	int len = 0;
	const char hex[] = "0123456789abcdef";

	for( i = 0; (i <= slen) && (i <= blen); i++ )
	{
		if( isalnum(src[i]) || (src[i] == '-') || (src[i] == '_') ||
		    (src[i] == '.') || (src[i] == '~') )
		{
			buf[len++] = src[i];
		}
		else if( (len+3) <= blen )
		{
			buf[len++] = '%';
			buf[len++] = hex[(src[i] >> 4) & 15];
			buf[len++] = hex[(src[i] & 15) & 15];
		}
		else
		{
			break;
		}
	}

	return len;
}

int uh_b64decode(char *buf, int blen, const unsigned char *src, int slen)
{
	int i = 0;
	int len = 0;

	unsigned int cin  = 0;
	unsigned int cout = 0;


	for( i = 0; (i <= slen) && (src[i] != 0); i++ )
	{
		cin = src[i];

		if( (cin >= '0') && (cin <= '9') )
			cin = cin - '0' + 52;
		else if( (cin >= 'A') && (cin <= 'Z') )
			cin = cin - 'A';
		else if( (cin >= 'a') && (cin <= 'z') )
			cin = cin - 'a' + 26;
		else if( cin == '+' )
			cin = 62;
		else if( cin == '/' )
			cin = 63;
		else if( cin == '=' )
			cin = 0;
		else
			continue;

		cout = (cout << 6) | cin;

		if( (i % 4) == 3 )
		{
			if( (len + 3) < blen )
			{
				buf[len++] = (char)(cout >> 16);
				buf[len++] = (char)(cout >> 8);
				buf[len++] = (char)(cout);
			}
			else
			{
				break;
			}
		}
	}

	buf[len++] = 0;
	return len;
}

struct path_info * uh_path_lookup(struct client *cl, const char *url)
{
#if defined(CAMELOT)
	static char doc_root[PATH_MAX];
#endif
	static char path_phys[PATH_MAX];
	static char path_info[PATH_MAX];
	static struct path_info p;

	char buffer[UH_LIMIT_MSGHEAD];
	char *docroot = cl->server->conf->docroot;
	char *pathptr = NULL;

	int i = 0;
	struct stat s;

	memset(path_phys, 0, sizeof(path_phys));
	memset(path_info, 0, sizeof(path_info));
	memset(buffer, 0, sizeof(buffer));
	memset(&p, 0, sizeof(p));

	/* copy docroot */
	memcpy(buffer, docroot,
		min(strlen(docroot), sizeof(buffer) - 1));

	/* separate query string from url */
	if( (pathptr = strchr(url, '?')) != NULL )
	{
		p.query = pathptr[1] ? pathptr + 1 : NULL;

		/* urldecode component w/o query */
		if( pathptr > url )
			uh_urldecode(
				&buffer[strlen(docroot)],
				sizeof(buffer) - strlen(docroot) - 1,
				url, (int)(pathptr - url) - 1
			);
	}
	/* no query string, decode all of url */
	else
	{
		uh_urldecode(
			&buffer[strlen(docroot)],
			sizeof(buffer) - strlen(docroot) - 1,
			url, strlen(url)
		);
	}

	/* create canon path */
	for( i = strlen(buffer); i >= 0; i-- )
	{
		if( (buffer[i] == 0) || (buffer[i] == '/') )
		{
			memset(path_info, 0, sizeof(path_info));
			memcpy(path_info, buffer, min(i + 1, sizeof(path_info) - 1));

			if( realpath(path_info, path_phys) )
			{
				memset(path_info, 0, sizeof(path_info));
				memcpy(path_info, &buffer[i],
					min(strlen(buffer) - i, sizeof(path_info) - 1));

				break;
			}
		}
	}

	/* check whether found path is within docroot */
	if( strncmp(path_phys, docroot, strlen(docroot)) ||
	    ((path_phys[strlen(docroot)] != 0) &&
		 (path_phys[strlen(docroot)] != '/'))
	) {
#if defined(CAMELOT)
		if( (strncmp(path_phys, UHTTPD_STORAGE_DISK, UHTTPD_STORAGE_DISK_LEN)) )
			return NULL;

		/* support link file access */
		memset(doc_root, 0, sizeof(doc_root));
		memcpy(doc_root, path_phys, sizeof(doc_root));
		docroot = doc_root;
		for( i=1; i< sizeof(doc_root); i++ )
		{
			if(doc_root[i] == '/')
			{
				doc_root[i] = '\0';
				break;
			}
		}
#else
		return NULL;
#endif
	}

	/* test current path */
	if( ! stat(path_phys, &p.stat) )
	{
		/* is a regular file */
		if( p.stat.st_mode & S_IFREG )
		{
			p.root = docroot;
			p.phys = path_phys;
			p.name = &path_phys[strlen(docroot)];
			p.info = path_info[0] ? path_info : NULL;
		}
		/* is a directory */
		else if( (p.stat.st_mode & S_IFDIR) && !strlen(path_info) )
		{
			/* ensure trailing slash */
			if( path_phys[strlen(path_phys)-1] != '/' )
				path_phys[strlen(path_phys)] = '/';

			/* try to locate index file */
			memset(buffer, 0, sizeof(buffer));
			memcpy(buffer, path_phys, sizeof(buffer));
			pathptr = &buffer[strlen(buffer)];

			for( i = 0; i < array_size(uh_index_files); i++ )
			{
				strncat(buffer, uh_index_files[i], sizeof(buffer));

				if( !stat(buffer, &s) && (s.st_mode & S_IFREG) )
				{
					memcpy(path_phys, buffer, sizeof(path_phys));
					memcpy(&p.stat, &s, sizeof(p.stat));
					break;
				}
				*pathptr = 0;
			}

			p.root = docroot;
			p.phys = path_phys;
			p.name = &path_phys[strlen(docroot)];
		}
	}

	return p.phys ? &p : NULL;
}

static char uh_realms[UH_LIMIT_AUTHREALMS * sizeof(struct auth_realm)] = { 0 };
static int uh_realm_count = 0;

struct auth_realm * uh_auth_add(char *path, char *user, char *pass)
{
	struct auth_realm *new = NULL;
	struct passwd *pwd;
	struct spwd *spwd;

	if( uh_realm_count < UH_LIMIT_AUTHREALMS )
	{
		new = (struct auth_realm *)
			&uh_realms[uh_realm_count * sizeof(struct auth_realm)];

		memset(new, 0, sizeof(struct auth_realm));

		memcpy(new->path, path,
			min(strlen(path), sizeof(new->path) - 1));

		memcpy(new->user, user,
			min(strlen(user), sizeof(new->user) - 1));

		/* given password refers to a passwd entry */
		if( (strlen(pass) > 3) && !strncmp(pass, "$p$", 3) )
		{
			/* try to resolve shadow entry */
			if( ((spwd = getspnam(&pass[3])) != NULL) && spwd->sp_pwdp )
			{
				memcpy(new->pass, spwd->sp_pwdp,
					min(strlen(spwd->sp_pwdp), sizeof(new->pass) - 1));
			}
			/* try to resolve passwd entry */
			else if( ((pwd = getpwnam(&pass[3])) != NULL) && pwd->pw_passwd &&
				(pwd->pw_passwd[0] != '!') && (pwd->pw_passwd[0] != 0) )
			{
				memcpy(new->pass, pwd->pw_passwd,
					min(strlen(pwd->pw_passwd), sizeof(new->pass) - 1));
			}			
		}
		/* ordinary pwd */
		else
		{
			memcpy(new->pass, pass,
				min(strlen(pass), sizeof(new->pass) - 1));
		}

		uh_realm_count++;
	}

	return new;
}

#if defined(CAMELOT)
extern int MD5String (unsigned char* string, unsigned int nLen, unsigned char* digest);
int hex2str(char *pszHex, int nHexLen, char *pszString)
{
        int i;
        char ch;
        for (i = 0; i < nHexLen; i++)
        {
                ch = (pszHex[i] & 0xF0) >> 4;
                pszString[i * 2] = (ch > 9) ? (ch + 0x41 - 0x0A) : (ch + 0x30);
                ch = pszHex[i] & 0x0F;
                pszString[i * 2 + 1] = (ch > 9) ? (ch + 0x41 - 0x0A) : (ch + 0x30);
        }
       pszString[i * 2] = 0;

        return 0;
}
#endif

#ifdef CONFIG_WEB_LOGIN
void http_store_cookie(void)
{
	FILE *c;
	struct auth_realm *realm = NULL;
	int i, j;

    DBG("%s:%d To Save User and Cookies\n", __func__, __LINE__);

	if( (c = fopen("/tmp/httpd.ck", "w")) != NULL )
	{
	    for( i = 0; i < uh_realm_count; i++ )
    	{
	    	realm = (struct auth_realm *)
		    	&uh_realms[i * sizeof(struct auth_realm)];
	        if (!realm->user[0])
                continue;
	        for( j = 0; j < UH_LIMIT_CLIENT_AUTHS; j++ )
        	{
	            if (!realm->auths[j].cookie_ID[0])
                    continue;
	    	    fprintf(c, "%s %d %s %s %d\n", realm->user, realm->logout_time, realm->auths[j].cookie_ID, realm->auths[j].peeraddr_str, realm->auths[j].time);
            }
        }
		fclose(c);
	}
}

void http_restore_cookie(void)
{
	FILE *c;
	struct auth_realm *realm = NULL;
   	struct auth_realm myrealm;
	int i, j;

    DBG("%s:%d To Restore User and Cookies\n", __func__, __LINE__);

	if( (c = fopen("/tmp/httpd.ck", "r")) != NULL )
	{
        while( (fscanf(c, "%s %d %s %s %d\n", myrealm.user, &myrealm.logout_time, myrealm.auths[0].cookie_ID, myrealm.auths[0].peeraddr_str, &myrealm.auths[0].time) != EOF) )
        {
	        for( i = 0; i < uh_realm_count; i++ )
        	{
	        	realm = (struct auth_realm *)
		        	&uh_realms[i * sizeof(struct auth_realm)];
	            if (!realm->user[0])
                    continue;
	            if (strcmp(realm->user, myrealm.user))
                    continue;
                realm->logout_time = myrealm.logout_time;
	            for( j = 0; j < UH_LIMIT_CLIENT_AUTHS; j++ )
            	{
	                if (realm->auths[j].cookie_ID[0])
                        continue;
                    memcpy(&realm->auths[j], &myrealm.auths[0], sizeof(http_auths));
                    goto next;
                }
            }
next:
            DBG("%s:%d Import User:%s %d %s %s %d\n", __func__, __LINE__, myrealm.user, myrealm.logout_time, myrealm.auths[0].cookie_ID, myrealm.auths[0].peeraddr_str, myrealm.auths[0].time);
        }
		fclose(c);
	}
}

int http_process_cookie(http_cookie* hc, char *cookie_p)
{
	hc->value = cookie_p;
	return 1;
}

int http_verify_cookie(struct client *cl, http_cookie *hc, struct auth_realm *realm)
{
	const char *addr = sa_straddr(&(cl->peeraddr));
	const http_auths *auth = realm->auths;
	int i;

#ifdef HAVE_DBG
	DBG("%s:%d Verify-Cookie: UserID=%s, PeerAddr=%s, CID=%s\n", __func__, __LINE__, realm->user, addr, hc->value);
#endif
	if (!addr || !auth)
		return 0;
	for( i = 0; i < UH_LIMIT_CLIENT_AUTHS; i++ ) {
		auth = (http_auths *)&realm->auths[i];
#ifdef HAVE_DBG
		DBG("%s:%d Auths=0x%x\n", __func__, __LINE__, (unsigned int)auth);
		DBG("%s:%d PeerAddr=%s, CID=%s\n", __func__, __LINE__, auth->peeraddr_str, auth->cookie_ID);
#endif
		if(!strcmp(auth->cookie_ID, hc->value) &&
		   !strncmp(auth->peeraddr_str, addr, min(strlen(addr), INET6_ADDRSTRLEN)))
			return 1;
	}
	return 0;
}

int uh_web_auth_check(struct client *cl, struct http_request *req, struct path_info *pi)
{
	struct auth_realm *realm = NULL;
	int i, plen, rlen, ret;
	char value[32];
	char fixed_type[80];
	http_cookie hc;

	ret = cdb_get("$sys_idle", &value);
	if(ret>=0)
	{
		idle_time = atoi(value);
	}
	else
	{
		DBG("%s:%d cdb_get value failed\n", __func__, __LINE__);
		return 0;
	}

	plen = strlen(pi->name);
	g_path = pi->name;

	/* find matching realm */
	for( i = 0; i < uh_realm_count; i++ )
	{
		realm = (struct auth_realm *)
			&uh_realms[i * sizeof(struct auth_realm)];
	
		rlen = strlen(realm->path);
		if( (plen >= rlen) && !strncasecmp(pi->name, realm->path, rlen) )
		{
			if(strcmp(req->realm->cookie_ID,""))
			{
				if(http_process_cookie(&hc, req->realm->cookie_ID) && 
				   http_verify_cookie(cl, &hc, realm))
				{
					req->realm = realm;
					break;
				}
			}
		}
		realm = NULL;
	}
	/* found a realm matching the username && password */
	if( realm )
	{
        if(1) // always need to check logout_time
		{
			if ((realm->logout_flag == 1)&&(realm->logout_time == 0))
			{
			    realm->logout_flag= 0;
					goto uh_auth_err;
			}
			if(idle_time != 0)
			{
				if (realm->logout_time && realm->logout_time <= time(NULL))
				{
					//realm->logout_time = time(NULL) + idle_time;
					uh_auth_logout_user(realm);
					goto uh_auth_err;
				}
			}
		}
//	uh_auth_success:
		uh_auth_ok = 1;
		req->auth_userid = i;	
		realm->logout_time = time(NULL) + idle_time;
		realm->logout_flag = 0;

		if( !strcmp("/web_login.htm", pi->name) || !strcmp("/web_login_error.htm", pi->name) )
		{
			char *data = strrchr(pi->phys, '/');

			foreach_header(i, req->headers)
			{
				if( !strcasecmp(req->headers[i], "If-Modified-Since") ||
					!strcasecmp(req->headers[i], "If-None-Match") )
					*req->headers[i+1] = 0;
			}

			if( data )
				strcpy(data, "/index.htm");
			// do stat for new file
			if( ! stat(pi->phys, &pi->stat) )
				uh_file_request(cl, req, pi);
			else
				uh_http_sendhf(cl, 404, "Not Found", "No such file or directory");

			return 0;
		}
		return 1;
	}
uh_auth_err:
	for( i = 0; i < array_size(uh_noauth_files); i++ )
	{
		if(!strcmp(pi->name,uh_noauth_files[i])) return 1;
	}

	uh_auth_ok = 0;
	http_process_cookie(&hc,req->realm->cookie_ID);

	if(strstr(pi->name, ".cgi") || strstr(pi->name, ".bin"))
	{
		uh_http_sendhf(cl, 401, "Authorization Required",
			"The requested resource requires user authentication");
	}
	else
	{
		snprintf(fixed_type, sizeof(fixed_type), uh_file_mime_lookup("/web_login.htm"), CHARSET);
		uh_http_sendhf_2(cl, 200, "OK", fixed_type,
			"<html><head><script>location='/web_login.htm';</script></head></html>");
	}
	return 0;
}

void uh_auths_set_cookie(struct client *cl, struct auth_realm *realm, char *value)
{
	http_auths *auths = realm->auths;
	http_auths *new = NULL;
	http_auths *cur = NULL;
	const char *addr = sa_straddr(&(cl->peeraddr));
	int t = time(NULL);
	int i;

	for( i = 0; i < UH_LIMIT_CLIENT_AUTHS; i++ )
	{
		cur = (http_auths *) &auths[i];
		if( t > cur->time )
		{
			new = cur;
			t = cur->time;
		}
		if( addr && !strcmp(cur->peeraddr_str, addr) )
		{
			new = cur;
			break;
		}
	}

	if( !new )
	{
		// it should not be here !!
		new = (http_auths *)&auths[0];
	}

	new->time = time(NULL);
	if( addr )
		strncpy(new->peeraddr_str, addr, min(strlen(addr), INET6_ADDRSTRLEN));
	for(int i = 0; i < COOKIE_ID_LENGTH; i++)
	{
		new->cookie_ID[i] = value[i];
		if( new->cookie_ID[i] == '\0' )
			break;
	}
	new->cookie_ID[COOKIE_ID_LENGTH] = '\0';

#ifdef HAVE_DBG
	DBG("%s:%d Set-Cookie: Auths=0x%x(head=0x%x)\n", __func__, __LINE__, (unsigned int)new, (unsigned int)auths);
	DBG("%s:%d Set-Cookie: UserID=%s, PeerAddr=%s, CID=%s\n", __func__, __LINE__, realm->user, new->peeraddr_str, new->cookie_ID);
#endif
}
int uh_realm_set_cookie(struct client *cl, struct http_response *res, struct path_info *pi)
{
	int i, plen, rlen;
	struct auth_realm *realm = NULL;

	plen = strlen(pi->name);

	/* check whether at least one realm covers the requested url */
	for( i = 0; i < uh_realm_count; i++ )
	{
		realm = (struct auth_realm *)
			&uh_realms[i * sizeof(struct auth_realm)];

		rlen = strlen(realm->path);

		if( (plen >= rlen) && !strncasecmp(pi->name, realm->path, rlen) )
		{
			if( res->hc.userid && !strcmp(res->hc.userid, realm->user) )
				break;
		}
		realm = NULL;
	}

	/* requested resource is covered by a realm */
	if( realm && res->hc.value )
	{
		for(int j = 0; j < COOKIE_ID_LENGTH; j++)
		{
			realm->cookie_ID[j] = res->hc.value[j];
			if( realm->cookie_ID[j] == '\0' )
				break;
		}
		realm->cookie_ID[COOKIE_ID_LENGTH] = '\0';
		uh_auths_set_cookie(cl, realm, res->hc.value);
		return 1;
	}

	return 0;
}
#else
int uh_auth_check(struct client *cl, struct http_request *req, struct path_info *pi)
{
	int i, plen, rlen, ret, protected;
	char buffer[UH_LIMIT_MSGHEAD];
	char *user = NULL;
	char *pass = NULL;
	char value[32];
#if defined(CAMELOT)
	char md5_buf[32];
	static char md5_pass[32];
	static char md5_user[32];
#endif
	struct auth_realm *realm = NULL;

	ret = cdb_get("$sys_idle", &value);
	if(ret>=0)
	{
		idle_time = atoi(value);
	}
	else
	{
		DBG("%s:%d cdb_get value failed\n", __func__, __LINE__);
		return 0;
	}

	plen = strlen(pi->name);
	protected = 0;
	g_path = pi->name;

	/* check whether at least one realm covers the requested url */
	for( i = 0; i < uh_realm_count; i++ )
	{
		realm = (struct auth_realm *)
			&uh_realms[i * sizeof(struct auth_realm)];

		rlen = strlen(realm->path);

		if( (plen >= rlen) && !strncasecmp(pi->name, realm->path, rlen) )
		{
			req->realm = realm;
			protected = 1;
			break;
		}
	}

	/* requested resource is covered by a realm */
	if( protected )
	{
		/* try to get client auth info */
		foreach_header(i, req->headers)
		{
			if( !strcasecmp(req->headers[i], "Authorization") &&
				(strlen(req->headers[i+1]) > 6) &&
				!strncasecmp(req->headers[i+1], "Basic ", 6) )
			{
				memset(buffer, 0, sizeof(buffer));
				uh_b64decode(buffer, sizeof(buffer) - 1,
					(unsigned char *) &req->headers[i+1][6],
					strlen(req->headers[i+1]) - 6);

				if( (pass = strchr(buffer, ':')) != NULL )
				{
					user = buffer;
					*pass++ = 0;
					strncpy(md5_user, user, min(strlen(user), sizeof(md5_user)-1));
					md5_user[sizeof(md5_user)-1] = 0;
					g_user = md5_user;
				}
				break;
			}
		}

		/* have client auth */
		if( user && pass )
		{
			/* find matching realm */
			for( i = 0, realm = NULL; i < uh_realm_count; i++ )
			{
				realm = (struct auth_realm *)
					&uh_realms[i * sizeof(struct auth_realm)];

				rlen = strlen(realm->path);
				if( (plen >= rlen) &&
				    !strncasecmp(pi->name, realm->path, rlen) &&
				    !strcmp(user, realm->user) )
				{
					req->realm = realm;
					break;
				}
				realm = NULL;
			}

			/* found a realm matching the username */
			if( realm )
			{
#if defined(CAMELOT)
				int pass_len = strlen(pass);

				MD5String((unsigned char *)pass, pass_len, (unsigned char *)md5_buf);

				md5_buf[16] = 0;
				hex2str(md5_buf,16,md5_pass);

				DBG("%s:%d pass %s, md5 pass %s, realm->pass %s\n", __func__, __LINE__, pass, md5_pass, realm->pass);
                {
                    int i;
	    			DBG("%s:%d pass = [0x", __func__, __LINE__);
                    for (i=0;i<pass_len;i++) {
    	    			DBG(" %2.2x", pass[i]);
                    }
	    			DBG("] len = [%d]\n", pass_len);
                }
				pass = md5_pass;
#else
				/* is a crypt passwd */
				if( realm->pass[0] == '$' )
					pass = crypt(pass, realm->pass);
#endif
				g_pass = pass;
				if(uh_auth_ok)
				{
					if ((realm->logout_flag == 1)&&(realm->logout_time==0) )
					{
					    realm->logout_flag= 0;
							goto uh_auth_err;
					}
					if(idle_time != 0)
					{
						if (realm->logout_time && realm->logout_time <= time(NULL))
						{
							//realm->logout_time = time(NULL) + idle_time;
							uh_auth_logout_user(realm);
							goto uh_auth_err;
						}
					}
				}
				/* check user pass */
				if( !strcmp(pass, realm->pass) )
					goto uh_auth_success;
			}
		}
	}
uh_auth_err:
		uh_auth_ok = 0;
		/* 401 */
		uh_http_sendf(cl, NULL,
			"HTTP/%.1f 401 Authorization Required\r\n"
			"WWW-Authenticate: Basic realm=\"%s\"\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 23\r\n\r\n"
			"Authorization Required\n",
			req->version, cl->server->conf->realm);
		return 0;
uh_auth_success:
	uh_auth_ok = 1;
	realm->logout_time = time(NULL) + idle_time;
	realm->logout_flag = 0;
	return 1;
}
#endif

void uh_auth_logout(void)
{
	struct auth_realm *realm = NULL;
	int i, plen, rlen;
#ifdef CONFIG_WEB_LOGIN
	http_cookie hc;
	hc = hc;
#endif

	/* find matching realm */
#ifndef CONFIG_WEB_LOGIN
	if( g_user && g_pass )
#endif
	{
		/* find matching realm */
		for( i = 0, realm = NULL; i < uh_realm_count; i++ )
		{
			realm = (struct auth_realm *)
				&uh_realms[i * sizeof(struct auth_realm)];

			rlen = strlen(realm->path);
			plen = strlen(g_path);
#ifndef CONFIG_WEB_LOGIN
			if( (plen >= rlen) &&
			    !strncasecmp(g_path, realm->path, rlen) &&
			    !strcmp(g_user, realm->user) &&
			    !strcmp(g_pass, realm->pass))
#else
			if( (plen >= rlen) && !strncasecmp(g_path, realm->path, rlen) )
#endif
			{
//#ifndef CONFIG_WEB_LOGIN
//				req->realm = realm;
//#endif
				break;
			}
			realm = NULL;
		}
		/* found a realm matching the username && password */
		if( realm )
		{
			uh_auth_logout_user(realm);
		}
	}
}

inline void uh_auth_logout_user(struct auth_realm *user)
{
	user->addr = 0;
	user->logout_time = 0;
	user->logout_flag = 1;

#ifdef CONFIG_WEB_LOGIN
	memset(user->cookie_ID, 0, COOKIE_ID_LENGTH);
	memset(user->auths, 0, sizeof(http_auths) * UH_LIMIT_CLIENT_AUTHS);
#endif
}

static char uh_listeners[UH_LIMIT_LISTENERS * sizeof(struct listener)] = { 0 };
static char uh_clients[UH_LIMIT_CLIENTS * sizeof(struct client)] = { 0 };

static int uh_listener_count = 0;
static int uh_client_count = 0;

#if defined(CAMELOT)
static char uh_clients_info[UH_LIMIT_CLIENT_INFOS * sizeof(struct client_info)] = { 0 };

struct client_info * uh_client_info_add(struct client *cl)
{
	struct client_info *new = NULL;
	struct client_info *cur = NULL;
	const char *addr = sa_straddr(&(cl->peeraddr));
	int t = time(NULL);
	int i;

	for( i = 0; i < UH_LIMIT_CLIENT_INFOS; i++ )
	{
		cur = (struct client_info *) &uh_clients_info[i * sizeof(struct client_info)];
		if( t > cur->time )
		{
			new = cur;
			t = cur->time;
		}
		if( addr && !strcmp(cur->peeraddr_str, addr) )
		{
			new = cur;
			break;
		}
	}

	if( !new )
	{
		// it should not be here !!
		new = (struct client_info *)&uh_clients_info[0];
	}

	new->time = time(NULL);
	memcpy(&(new->peeraddr), &(cl->peeraddr), sizeof(struct sockaddr_in6));
	if( addr )
		strncpy(new->peeraddr_str, addr, min(strlen(addr), INET6_ADDRSTRLEN));
	get_random_bytes(new->storage_id, STORAGE_ID_LENGTH);
	new->storage_id[STORAGE_ID_LENGTH] = '\0';

	return new;
}

struct client_info * uh_client_info_lookup(struct client *cl)
{
	struct client_info *cur = NULL;
	const char *addr = sa_straddr(&(cl->peeraddr));
	int i;

	for( i = 0; i < UH_LIMIT_CLIENT_INFOS; i++ )
	{
		cur = (struct client_info *) &uh_clients_info[i * sizeof(struct client_info)];

		if( addr && !strcmp(cur->peeraddr_str, addr) )
			return cur;
	}

	return NULL;
}
#ifdef HAVE_DBG
void uh_client_info_dump(void)
{
	struct client_info *cur = NULL;
	int i;

	for( i = 0; i < UH_LIMIT_CLIENT_INFOS; i++ )
	{
		cur = (struct client_info *) &uh_clients_info[i * sizeof(struct client_info)];
		DBG("%s:%d %d:%s\n", __func__, __LINE__, i, cur->peeraddr_str);
	}
}
#endif
#endif

struct listener * uh_listener_add(int sock, struct config *conf)
{
	struct listener *new = NULL;
	socklen_t sl;

	if( uh_listener_count < UH_LIMIT_LISTENERS )
	{
		new = (struct listener *)
			&uh_listeners[uh_listener_count * sizeof(struct listener)];

		new->socket = sock;
		new->conf   = conf;

		/* get local endpoint addr */
		sl = sizeof(struct sockaddr_in6);
		memset(&(new->addr), 0, sl);
		getsockname(sock, (struct sockaddr *) &(new->addr), &sl);

		uh_listener_count++;
	}

	return new;
}

struct listener * uh_listener_lookup(int sock)
{
	struct listener *cur = NULL;
	int i;

	for( i = 0; i < uh_listener_count; i++ )
	{
		cur = (struct listener *) &uh_listeners[i * sizeof(struct listener)];

		if( cur->socket == sock )
			return cur;
	}

	return NULL;
}


struct client * uh_client_add(int sock, struct listener *serv)
{
	struct client *new = NULL;
	socklen_t sl;

	if( uh_client_count < UH_LIMIT_CLIENTS )
	{
		new = (struct client *)
			&uh_clients[uh_client_count * sizeof(struct client)];

		new->socket = sock;
		new->server = serv;

		/* get remote endpoint addr */
		sl = sizeof(struct sockaddr_in6);
		memset(&(new->peeraddr), 0, sl);
		getpeername(sock, (struct sockaddr *) &(new->peeraddr), &sl);

		/* get local endpoint addr */
		sl = sizeof(struct sockaddr_in6);
		memset(&(new->servaddr), 0, sl);
		getsockname(sock, (struct sockaddr *) &(new->servaddr), &sl);

		uh_client_count++;
	}

	return new;
}

struct client * uh_client_lookup(int sock)
{
	struct client *cur = NULL;
	int i;

	for( i = 0; i < uh_client_count; i++ )
	{
		cur = (struct client *) &uh_clients[i * sizeof(struct client)];

		if( cur->socket == sock )
			return cur;
	}

	return NULL;
}

void uh_client_remove(int sock)
{
	struct client *del = uh_client_lookup(sock);

	if( del )
	{
		memmove(del, del + 1,
			sizeof(uh_clients) - (int)((char *)del - uh_clients) - sizeof(struct client));

		uh_client_count--;
	}
}
