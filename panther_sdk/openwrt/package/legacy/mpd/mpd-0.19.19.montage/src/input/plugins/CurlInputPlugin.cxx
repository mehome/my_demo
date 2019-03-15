/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "CurlInputPlugin.hxx"
#include "../AsyncInputStream.hxx"
#include "../IcyInputStream.hxx"
#include "../InputPlugin.hxx"
#include "input/TextInputStream.hxx"
#include "config/ConfigGlobal.hxx"
#include "config/ConfigData.hxx"
#include "tag/Tag.hxx"
#include "tag/TagBuilder.hxx"
#include "event/SocketMonitor.hxx"
#include "event/TimeoutMonitor.hxx"
#include "event/Call.hxx"
#include "IOThread.hxx"
#include "Instance.hxx"
#include "Partition.hxx"
#include "DetachedSong.hxx"
#include "util/ASCII.hxx"
#include "util/StringUtil.hxx"
#include "util/NumberParser.hxx"
#include "util/CircularBuffer.hxx"
#include "util/HugeAllocator.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"
#include "util/UriUtil.hxx"
#include "Log.hxx"

#include <assert.h>
#include <string.h>

#include <iostream>
#include <utility>
#include <vector>
#include <cstdlib>

#include <curl/curl.h>

extern "C"
{
#include <cdb.h>
}

#if LIBCURL_VERSION_NUM < 0x071200
#error libcurl is too old
#endif


/**
 * Do not buffer more than this number of bytes.  It should be a
 * reasonable limit that doesn't make low-end machines suffer too
 * much, but doesn't cause stuttering on high-latency lines.
 */
static const size_t CURL_MAX_BUFFERED = 512 * 1024;

/**
 * Resume the stream at this number of bytes after it has been paused.
 */
static const size_t CURL_RESUME_AT = 384 * 1024;

struct CurlInputStream final : public AsyncInputStream {
	/* some buffers which were passed to libcurl, which we have
	   too free */
	char range[32];
	struct curl_slist *request_headers;

	/** the curl handles */
	CURL *easy;

	/** error message provided by libcurl */
	char error_buffer[CURL_ERROR_SIZE];
	
	/** montage patch save redirect flag for retry in DecoderThread.cxx*/
	bool redirect;

	/** parser for icy-metadata */
	IcyInputStream *icy;

	CurlInputStream(const char *_url, Mutex &_mutex, Cond &_cond,
			void *_buffer)
		:AsyncInputStream(_url, _mutex, _cond,
				  _buffer, (((strstr(_url, "idaddy.cn")) || (strstr(_url, "alicdn.com"))) ? (CURL_MAX_BUFFERED / 8) : CURL_MAX_BUFFERED),
				  (((strstr(_url, "idaddy.cn")) || (strstr(_url, "alicdn.com"))) ? (CURL_RESUME_AT / 8) : CURL_RESUME_AT)),
		 request_headers(nullptr),
		 icy(new IcyInputStream(this)) {}

	~CurlInputStream();

	CurlInputStream(const CurlInputStream &) = delete;
	CurlInputStream &operator=(const CurlInputStream &) = delete;

	static InputStream *Open(const char *url, Mutex &mutex, Cond &cond,
				 Error &error);

	bool InitEasy(Error &error);

	/**
	 * Frees the current "libcurl easy" handle, and everything
	 * associated with it.
	 *
	 * Runs in the I/O thread.
	 */
	void FreeEasy();

	/**
	 * Frees the current "libcurl easy" handle, and everything associated
	 * with it.
	 *
	 * The mutex must not be locked.
	 */
	void FreeEasyIndirect();

	/**
	 * Called when a new response begins.  This is used to discard
	 * headers from previous responses (for example authentication
	 * and redirects).
	 */
	void ResponseBoundary();

	void HeaderReceived(const char *name, std::string &&value);

	size_t DataReceived(const void *ptr, size_t size);

	/**
	 * A HTTP request is finished.
	 *
	 * Runs in the I/O thread.  The caller must not hold locks.
	 */
	void RequestDone(CURLcode result, long status);

	/* virtual methods from AsyncInputStream */
	virtual void DoResume() override;
	virtual void DoSeek(offset_type new_offset) override;
};

class CurlMulti;

/**
 * Monitor for one socket created by CURL.
 */
class CurlSocket final : SocketMonitor {
	CurlMulti &multi;

public:
	CurlSocket(CurlMulti &_multi, EventLoop &_loop, int _fd)
		:SocketMonitor(_fd, _loop), multi(_multi) {}

	~CurlSocket() {
		/* TODO: sometimes, CURL uses CURL_POLL_REMOVE after
		   closing the socket, and sometimes, it uses
		   CURL_POLL_REMOVE just to move the (still open)
		   connection to the pool; in the first case,
		   Abandon() would be most appropriate, but it breaks
		   the second case - is that a CURL bug?  is there a
		   better solution? */
	}

	/**
	 * Callback function for CURLMOPT_SOCKETFUNCTION.
	 */
	static int SocketFunction(CURL *easy,
				  curl_socket_t s, int action,
				  void *userp, void *socketp);

	virtual bool OnSocketReady(unsigned flags) override;

private:
	static constexpr int FlagsToCurlCSelect(unsigned flags) {
		return (flags & (READ | HANGUP) ? CURL_CSELECT_IN : 0) |
			(flags & WRITE ? CURL_CSELECT_OUT : 0) |
			(flags & ERROR ? CURL_CSELECT_ERR : 0);
	}

	gcc_const
	static unsigned CurlPollToFlags(int action) {
		switch (action) {
		case CURL_POLL_NONE:
			return 0;

		case CURL_POLL_IN:
			return READ;

		case CURL_POLL_OUT:
			return WRITE;

		case CURL_POLL_INOUT:
			return READ|WRITE;
		}

		assert(false);
		gcc_unreachable();
	}
};

/**
 * Manager for the global CURLM object.
 */
class CurlMulti final : private TimeoutMonitor {
	CURLM *const multi;

public:
	CurlMulti(EventLoop &_loop, CURLM *_multi);

	~CurlMulti() {
		curl_multi_cleanup(multi);
	}

	bool Add(CurlInputStream *c, Error &error);
	void Remove(CurlInputStream *c);

	/**
	 * Check for finished HTTP responses.
	 *
	 * Runs in the I/O thread.  The caller must not hold locks.
	 */
	void ReadInfo();

	void Assign(curl_socket_t fd, CurlSocket &cs) {
		curl_multi_assign(multi, fd, &cs);
	}

	void SocketAction(curl_socket_t fd, int ev_bitmask);

	void InvalidateSockets() {
		SocketAction(CURL_SOCKET_TIMEOUT, 0);
	}

	/**
	 * This is a kludge to allow pausing/resuming a stream with
	 * libcurl < 7.32.0.  Read the curl_easy_pause manpage for
	 * more information.
	 */
	void ResumeSockets() {
		int running_handles;
		curl_multi_socket_all(multi, &running_handles);
	}

private:
	static int TimerFunction(CURLM *multi, long timeout_ms, void *userp);

	virtual void OnTimeout() override;
};

/**
 * libcurl version number encoded in a 24 bit integer.
 */
static unsigned curl_version_num;

/** libcurl should accept "ICY 200 OK" */
static struct curl_slist *http_200_aliases;

/** HTTP proxy settings */
static const char *proxy, *proxy_user, *proxy_password;
static const char *cust_type;
static unsigned proxy_port;

static bool verify_peer, verify_host;

static CurlMulti *curl_multi;

static constexpr Domain http_domain("http");
static constexpr Domain curl_domain("curl");
static constexpr Domain curlm_domain("curlm");

CurlMulti::CurlMulti(EventLoop &_loop, CURLM *_multi)
	:TimeoutMonitor(_loop), multi(_multi)
{
	curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION,
			  CurlSocket::SocketFunction);
	curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, this);

	curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, TimerFunction);
	curl_multi_setopt(multi, CURLMOPT_TIMERDATA, this);
}

/**
 * Find a request by its CURL "easy" handle.
 *
 * Runs in the I/O thread.  No lock needed.
 */
gcc_pure
static CurlInputStream *
input_curl_find_request(CURL *easy)
{
	assert(io_thread_inside());

	void *p;
	CURLcode code = curl_easy_getinfo(easy, CURLINFO_PRIVATE, &p);
	if (code != CURLE_OK)
		return nullptr;

	return (CurlInputStream *)p;
}

void
CurlInputStream::DoResume()
{
	assert(io_thread_inside());

	mutex.unlock();

	curl_easy_pause(easy, CURLPAUSE_CONT);

	if (curl_version_num < 0x072000)
		/* libcurl older than 7.32.0 does not update
		   its sockets after curl_easy_pause(); force
		   libcurl to do it now */
		curl_multi->ResumeSockets();

	curl_multi->InvalidateSockets();

	mutex.lock();
}

int
CurlSocket::SocketFunction(gcc_unused CURL *easy,
			   curl_socket_t s, int action,
			   void *userp, void *socketp) {
	CurlMulti &multi = *(CurlMulti *)userp;
	CurlSocket *cs = (CurlSocket *)socketp;

	assert(io_thread_inside());

	if (action == CURL_POLL_REMOVE) {
		delete cs;
		return 0;
	}

	if (cs == nullptr) {
		cs = new CurlSocket(multi, io_thread_get(), s);
		multi.Assign(s, *cs);
	} else {
#ifdef USE_EPOLL
		/* when using epoll, we need to unregister the socket
		   each time this callback is invoked, because older
		   CURL versions may omit the CURL_POLL_REMOVE call
		   when the socket has been closed and recreated with
		   the same file number (bug found in CURL 7.26, CURL
		   7.33 not affected); in that case, epoll refuses the
		   EPOLL_CTL_MOD because it does not know the new
		   socket yet */
		cs->Cancel();
#endif
	}

	unsigned flags = CurlPollToFlags(action);
	if (flags != 0)
		cs->Schedule(flags);
	return 0;
}

bool
CurlSocket::OnSocketReady(unsigned flags)
{
	assert(io_thread_inside());

	multi.SocketAction(Get(), FlagsToCurlCSelect(flags));
	return true;
}

/**
 * Runs in the I/O thread.  No lock needed.
 */
inline bool
CurlMulti::Add(CurlInputStream *c, Error &error)
{
	assert(io_thread_inside());
	assert(c != nullptr);
	assert(c->easy != nullptr);

	CURLMcode mcode = curl_multi_add_handle(multi, c->easy);
	if (mcode != CURLM_OK) {
		error.Format(curlm_domain, mcode,
			     "curl_multi_add_handle() failed: %s",
			     curl_multi_strerror(mcode));
		return false;
	}

	InvalidateSockets();
	return true;
}

/**
 * Call input_curl_easy_add() in the I/O thread.  May be called from
 * any thread.  Caller must not hold a mutex.
 */
static bool
input_curl_easy_add_indirect(CurlInputStream *c, Error &error)
{
	assert(c != nullptr);
	assert(c->easy != nullptr);

	bool result;
	BlockingCall(io_thread_get(), [c, &error, &result](){
			result = curl_multi->Add(c, error);
		});
	return result;
}

inline void
CurlMulti::Remove(CurlInputStream *c)
{
	curl_multi_remove_handle(multi, c->easy);
}

void
CurlInputStream::FreeEasy()
{
	assert(io_thread_inside());

	if (easy == nullptr)
		return;

	curl_multi->Remove(this);

	curl_easy_cleanup(easy);
	easy = nullptr;

	curl_slist_free_all(request_headers);
	request_headers = nullptr;
}

void
CurlInputStream::FreeEasyIndirect()
{
	BlockingCall(io_thread_get(), [this](){
			FreeEasy();
			curl_multi->InvalidateSockets();
		});

	assert(easy == nullptr);
}

inline void
CurlInputStream::RequestDone(CURLcode result, long status)
{
	assert(io_thread_inside());
	assert(!postponed_error.IsDefined());

	FreeEasy();
	AsyncInputStream::SetClosed();

	const ScopeLock protect(mutex);

	if (result != CURLE_OK) {
		postponed_error.Format(curl_domain, result,
				       "curl failed: %s", error_buffer);
	} else if (status < 200 || status >= 300) {
		postponed_error.Format(http_domain, status,
				       "got HTTP status %ld",
				       status);
	}

	if (IsSeekPending())
		SeekDone();
	else if (!IsReady())
		SetReady();
	else
		cond.broadcast();
}

static void
input_curl_handle_done(CURL *easy_handle, CURLcode result)
{
	CurlInputStream *c = input_curl_find_request(easy_handle);
	assert(c != nullptr);

	long status = 0;
	curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &status);

	c->RequestDone(result, status);
}

void
CurlMulti::SocketAction(curl_socket_t fd, int ev_bitmask)
{
	int running_handles;
	CURLMcode mcode = curl_multi_socket_action(multi, fd, ev_bitmask,
						   &running_handles);
	if (mcode != CURLM_OK)
		FormatError(curlm_domain,
			    "curl_multi_socket_action() failed: %s",
			    curl_multi_strerror(mcode));

	ReadInfo();
}

/**
 * Check for finished HTTP responses.
 *
 * Runs in the I/O thread.  The caller must not hold locks.
 */
inline void
CurlMulti::ReadInfo()
{
	assert(io_thread_inside());

	CURLMsg *msg;
	int msgs_in_queue;

	while ((msg = curl_multi_info_read(multi,
					   &msgs_in_queue)) != nullptr) {
		if (msg->msg == CURLMSG_DONE)
			input_curl_handle_done(msg->easy_handle, msg->data.result);
	}
}

int
CurlMulti::TimerFunction(gcc_unused CURLM *_multi, long timeout_ms, void *userp)
{
	CurlMulti &multi = *(CurlMulti *)userp;
	assert(_multi == multi.multi);

	if (timeout_ms < 0) {
		multi.Cancel();
		return 0;
	}

	if (timeout_ms >= 0 && timeout_ms < 10)
		/* CURL 7.21.1 likes to report "timeout=0", which
		   means we're running in a busy loop.  Quite a bad
		   idea to waste so much CPU.  Let's use a lower limit
		   of 10ms. */
		timeout_ms = 10;

	multi.Schedule(timeout_ms);
	return 0;
}

void
CurlMulti::OnTimeout()
{
	SocketAction(CURL_SOCKET_TIMEOUT, 0);
}

/*
 * InputPlugin methods
 *
 */

static InputPlugin::InitResult
input_curl_init(const config_param &param, Error &error)
{
	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if (code != CURLE_OK) {
		error.Format(curl_domain, code,
			     "curl_global_init() failed: %s",
			     curl_easy_strerror(code));
		return InputPlugin::InitResult::UNAVAILABLE;
	}

	const auto version_info = curl_version_info(CURLVERSION_FIRST);
	if (version_info != nullptr) {
		FormatDebug(curl_domain, "version %s", version_info->version);
		if (version_info->features & CURL_VERSION_SSL)
			FormatDebug(curl_domain, "with %s",
				    version_info->ssl_version);

		curl_version_num = version_info->version_num;
	}

	http_200_aliases = curl_slist_append(http_200_aliases, "ICY 200 OK");

	proxy = param.GetBlockValue("proxy");
	proxy_port = param.GetBlockValue("proxy_port", 0u);
	proxy_user = param.GetBlockValue("proxy_user");
	proxy_password = param.GetBlockValue("proxy_password");
	cust_type = param.GetBlockValue("ext_content_type");

#if 0
	if( cust_type == nullptr)
		cust_type = config_get_string(CONF_CUST_CONTENT_TYPE, nullptr );
#endif
	if (proxy == nullptr) {
		/* deprecated proxy configuration */
		proxy = config_get_string(CONF_HTTP_PROXY_HOST, nullptr);
		proxy_port = config_get_positive(CONF_HTTP_PROXY_PORT, 0);
		proxy_user = config_get_string(CONF_HTTP_PROXY_USER, nullptr);
		proxy_password = config_get_string(CONF_HTTP_PROXY_PASSWORD, "");
	}

	verify_peer = param.GetBlockValue("verify_peer", true);
	verify_host = param.GetBlockValue("verify_host", true);

	CURLM *multi = curl_multi_init();
	if (multi == nullptr) {
		curl_slist_free_all(http_200_aliases);
		curl_global_cleanup();
		error.Set(curl_domain, 0, "curl_multi_init() failed");
		return InputPlugin::InitResult::UNAVAILABLE;
	}

	curl_multi = new CurlMulti(io_thread_get(), multi);
	return InputPlugin::InitResult::SUCCESS;
}

static void
input_curl_finish(void)
{
	BlockingCall(io_thread_get(), [](){
			delete curl_multi;
		});

	curl_slist_free_all(http_200_aliases);
	http_200_aliases = nullptr;

	curl_global_cleanup();
}

CurlInputStream::~CurlInputStream()
{
	FreeEasyIndirect();
}

inline void
CurlInputStream::ResponseBoundary()
{
	/* undo all effects of HeaderReceived() because the previous
	   response was not applicable for this stream */

	if (IsSeekPending())
		/* don't update metadata while seeking */
		return;

	seekable = true;
	size = UNKNOWN_SIZE;
	ClearMimeType();
	ClearTag();

	// TODO: reset the IcyInputStream?
}


inline void
CurlInputStream::HeaderReceived(const char *name, std::string &&value)
{
	if (IsSeekPending())
		/* don't update metadata while seeking */
		return;

	if (StringEqualsCaseASCII(name, "accept-ranges")) {
		/* a stream with icy-metadata is not seekable */
		if (!icy->IsEnabled())
			seekable = true;
	} else if (StringEqualsCaseASCII(name, "server")) {
		if (StringEqualsCaseASCII(value.c_str(), "AkamaiGHost")) {
			seekable = true;
		}
	} else if (StringEqualsCaseASCII(name, "content-length")) {
		size = offset + ParseUint64(value.c_str());
	} else if (StringEqualsCaseASCII(name, "content-type")) {
		SetMimeType(std::move(value));
	} else if (StringEqualsCaseASCII(name, "icy-name") ||
		   StringEqualsCaseASCII(name, "ice-name") ||
		   StringEqualsCaseASCII(name, "x-audiocast-name")) {
		TagBuilder tag_builder;
		tag_builder.AddItem(TAG_NAME, value.c_str());

		SetTag(tag_builder.CommitNew());
	} else if (StringEqualsCaseASCII(name, "icy-metaint")) {
		if (icy->IsEnabled())
			return;

		size_t icy_metaint = ParseUint64(value.c_str());
		FormatDebug(curl_domain, "icy-metaint=%zu", icy_metaint);

		if (icy_metaint > 0) {
			icy->Enable(icy_metaint);

			/* a stream with icy-metadata is not
			   seekable */
			seekable = false;
		}
	} else if (StringEqualsCaseASCII(name, "Location" )) {
			if (memcmp(value.c_str(), "http://",  7) != 0 &&
				memcmp(value.c_str(), "https://", 8) != 0) {
				//redirect = true;
				icy->SetRedirURI(value.c_str());
			}
	}
}

/** called by curl when new data is available */
static size_t
input_curl_headerfunction(void *ptr, size_t size, size_t nmemb, void *stream)
{
	CurlInputStream &c = *(CurlInputStream *)stream;

	size *= nmemb;

	const char *header = (const char *)ptr;
	if (size > 5 && memcmp(header, "HTTP/", 5) == 0) {
		c.ResponseBoundary();
		return size;
	}

	const char *end = header + size;

	char name[64];

	const char *value = (const char *)memchr(header, ':', size);
	if (value == nullptr || (size_t)(value - header) >= sizeof(name))
		return size;

	memcpy(name, header, value - header);
	name[value - header] = 0;

	/* skip the colon */

	++value;

	/* strip the value */

	value = StripLeft(value, end);
	end = StripRight(value, end);

	c.HeaderReceived(name, std::string(value, end));
	return size;
}

inline size_t
CurlInputStream::DataReceived(const void *ptr, size_t received_size)
{
	assert(received_size > 0);

	const ScopeLock protect(mutex);

	if (IsSeekPending())
		SeekDone();

	if (received_size > GetBufferSpace()) {
		AsyncInputStream::Pause();
		return CURL_WRITEFUNC_PAUSE;
	}

	AppendToBuffer(ptr, received_size);
	return received_size;
}


/*
 * Copy each token in wordlist delimited by enter into word 
 */     
#define foreach(word, wordlist, next) \
	for (next = &wordlist[strspn(wordlist, "\n")], \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, "\n")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, '\n'); \
	     strlen(word); \
	     next = next ? &next[strspn(next, "\n")] : (char *)"", \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, "\n")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, '\n')) \
	    
extern Instance *instance;	 


typedef std::pair<std::string, std::string> 	PLS_ENTRY_PAIR;
typedef std::vector<PLS_ENTRY_PAIR> 			PLS_ENTRY_PAIR_VEC;
typedef PLS_ENTRY_PAIR_VEC::iterator 			PLS_ENTRY_PAIR_VEC_ITER;

#define GET_PLS_ENTRY_CONTENT(evp, key, content) \
	content.clear(); \
	for(PLS_ENTRY_PAIR_VEC_ITER iter = evp.begin(); iter != evp.end(); ++iter) { \
		if(iter->first.compare(key) == 0) { \
			content = iter->second; \
			break; \
		} \
	}\


static void
pls_parser(std::string buf )
{
	std::size_t start = 0, end = 0, pos = 0;
	std::string content;
	PLS_ENTRY_PAIR_VEC pls_evp;
	DetachedSong *song;

	while ((end = buf.find("\n", start)) != std::string::npos) {
		std::string line = buf.substr(start, end - start);
		start = end + 1;

		if ((pos = line.find("=")) != std::string::npos) {
			pls_evp.push_back(std::make_pair(line.substr(0, pos), line.substr(pos+1)));
		}
	}

	GET_PLS_ENTRY_CONTENT(pls_evp,"NumberOfEntries", content)
	int num_entries = std::atoi(content.c_str());

	for (unsigned i = 1; i <= num_entries && num_entries > 0;  i++ ) {
		std::string uri;
		char key[64];
		Error error;

		sprintf(key, "File%u", i );
		GET_PLS_ENTRY_CONTENT(pls_evp, key, uri)
		
		int n = uri.find_last_of('\r');
		if( n >= 0 ) 
			uri.erase(n);
		n = uri.find_last_of('\n');
		if( n >= 0 ) 
			uri.erase(n);

		if(uri.empty()) {
			FormatError(curlm_domain, "Invalid PLS entry %s not found", key);
			return;
		}
	

		TagBuilder tag;

		sprintf(key, "Title%u", i);
		GET_PLS_ENTRY_CONTENT(pls_evp, key, content)
		if (content.size() > 0 && (content != "No Title") )
			tag.AddItem(TAG_TITLE, content.c_str());

		sprintf(key, "Length%u", i);
		GET_PLS_ENTRY_CONTENT(pls_evp, key, content)
		int length = std::atoi(content.c_str());
		if (length > 0)
			tag.SetDuration(SignedSongTime::FromS(length));

		song = new DetachedSong(uri.c_str(), tag.Commit());
		instance->partition->playlist.AppendSong(instance->partition->pc, std::move(*song), error);
		delete song;
	}
}
    
/** called by curl when new data is available */
static size_t
input_curl_writefunction(void *ptr, size_t size, size_t nmemb, void *stream)
{
	CurlInputStream &c = *(CurlInputStream *)stream;
	char line[768], *next = NULL;
	char *p = (char *)ptr;
	const char *const mime = c.GetMimeType();
	int is_m3u = 0;
		
	size *= nmemb;
	if (size == 0)
		return 0;
	
	/* added by tashin, added 'redirect' support for playlist, at present, try to support all playlists */
	memset(line, 0 ,sizeof(line));

	if ( (mime != nullptr) && 
	     (StringEqualsCaseASCII(mime, "audio/x-mpegurl", 15) ||
		/* added redirect for playlist type */
		StringEqualsCaseASCII(mime, "audio/x-scpls", 13 ) ||
		/* added redirect for asf type */
		StringEqualsCaseASCII(mime, "video/x-ms-asf", 14 ) ||
		/* added redirect for m3u8 playlist type */
		StringEqualsCaseASCII(mime, "application/vnd.apple.mpegurl", 29 )) ) {
		foreach(line, p, next)
		{
			if( !strncmp(line, "#EXTM3U", 7) ) {
					is_m3u = 1;
					continue;
			}
			else if( !strncmp(line, "[playlist]", 10) ) {
					/* pls plugin take over */
					//printf("pls: %s\n", c.GetURI() );

					c.redirect = false;
					pls_parser(p);
					break;

			} 
			else if( !strncmp(line, "#EXTINF", 7) ) {
					/* extm3u plugin take over */
					//printf("m3u: %s\n", c.GetURI() );
					c.redirect = false;
					
					break;
			} 
			else if( is_m3u && !strncmp(line, "#EXT-X-STREAM-INF", 17) ) {
					c.redirect = true;
					//printf("m3u8 redirect info\n");
					continue;
			}
			else if(!strncmp(line, "http://", 7) || !strncmp(line, "https://", 8 )) {
					int n = strcspn(line, "\n");
					if( n > 0 )
						line[n] = '\0';
					n = strcspn(line, "\r");
					if( n > 0 )
						line[n] = '\0';
					
					/* added by tashin, just redirect to the first url when it is not playlist */
					if( c.redirect ) {
						c.icy->SetRedirURI(line); 
						//printf("redirect to %s\n", line );
						break;
					}
			}
			memset(line, 0 ,sizeof(line));
		}
	}

	return c.DataReceived(ptr, size);
}

bool
CurlInputStream::InitEasy(Error &error)
{
	easy = curl_easy_init();
	if (easy == nullptr) {
		error.Set(curl_domain, "curl_easy_init() failed");
		return false;
	}

	curl_easy_setopt(easy, CURLOPT_PRIVATE, (void *)this);

#ifdef MONTAGE_MPD_PATCH
	curl_easy_setopt(easy, CURLOPT_USERAGENT,
			 "Music Player Daemon");
#else
	curl_easy_setopt(easy, CURLOPT_USERAGENT,
			 "Music Player Daemon " VERSION);
#endif

	curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION,
			 input_curl_headerfunction);
	curl_easy_setopt(easy, CURLOPT_WRITEHEADER, this);
	curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION,
			 input_curl_writefunction);
	curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(easy, CURLOPT_HTTP200ALIASES, http_200_aliases);
	curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1l);
	curl_easy_setopt(easy, CURLOPT_NETRC, 1l);
	curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 5l);
	curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1l);
	curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error_buffer);
	curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 1l);
	curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1l);
	curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, 20l);

#ifdef MONTAGE_MPD_PATCH
	 /* enable TCP keep-alive for this transfer */
	curl_easy_setopt(easy, CURLOPT_TCP_KEEPALIVE, 1l);
	/* keep-alive idle time to 120 seconds */
	curl_easy_setopt(easy, CURLOPT_TCP_KEEPIDLE, 30l);
	/* interval time between keep-alive probes: 60 seconds */
	curl_easy_setopt(easy, CURLOPT_TCP_KEEPINTVL, 10l);
#endif

	if (proxy != nullptr)
		curl_easy_setopt(easy, CURLOPT_PROXY, proxy);

	if (proxy_port > 0)
		curl_easy_setopt(easy, CURLOPT_PROXYPORT, (long)proxy_port);

	if (proxy_user != nullptr && proxy_password != nullptr) {
		char proxy_auth_str[1024];
		snprintf(proxy_auth_str, sizeof(proxy_auth_str),
			 "%s:%s",
			 proxy_user, proxy_password);
		curl_easy_setopt(easy, CURLOPT_PROXYUSERPWD, proxy_auth_str);
	}
	
	if (!strncmp(GetURI(), "https://", 8)) {
		verify_peer = false;
		verify_host = false;
	}
	
	curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, verify_peer ? 1l : 0l);
	curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, verify_host ? 2l : 0l);

	CURLcode code = curl_easy_setopt(easy, CURLOPT_URL, GetURI());
	if (code != CURLE_OK) {
		error.Format(curl_domain, code,
			     "curl_easy_setopt() failed: %s",
			     curl_easy_strerror(code));
		return false;
	}

	request_headers = nullptr;
	request_headers = curl_slist_append(request_headers,
					       "Icy-Metadata: 1");
	curl_easy_setopt(easy, CURLOPT_HTTPHEADER, request_headers);

	return true;
}

void
CurlInputStream::DoSeek(offset_type new_offset)
{
	assert(IsReady());

	/* close the old connection and open a new one */
	mutex.unlock();

	FreeEasyIndirect();

	offset = new_offset;
	if (offset == size) {
		/* seek to EOF: simulate empty result; avoid
		   triggering a "416 Requested Range Not Satisfiable"
		   response */
		mutex.lock();
		SeekDone();
		return;
	}

	Error error;
	if (!InitEasy(postponed_error)) {
		mutex.lock();
		PostponeError(std::move(error));
		return;
	}

	/* send the "Range" header */

	if (offset > 0) {
		sprintf(range, "%lld-", (long long)offset);
		curl_easy_setopt(easy, CURLOPT_RANGE, range);
	}

	if (!input_curl_easy_add_indirect(this, error)) {
		mutex.lock();
		PostponeError(std::move(error));
		return;
	}

	mutex.lock();
	offset = new_offset;
}

inline InputStream *
CurlInputStream::Open(const char *url, Mutex &mutex, Cond &cond,
		      Error &error)
{
	
	void *buffer = NULL;

	if( !strstr(url, "idaddy.cn") || !strstr(url, "alicdn.com") ) {
 		buffer = HugeAllocate(CURL_MAX_BUFFERED);
	} else {
		buffer = HugeAllocate( CURL_MAX_BUFFERED / 8);
	}

	if (buffer == nullptr) {
		error.Set(curl_domain, "Out of memory");
		return nullptr;
	}

	CurlInputStream *c = new CurlInputStream(url, mutex, cond, buffer);

	if (!c->InitEasy(error) || !input_curl_easy_add_indirect(c, error)) {
		delete c;
		return nullptr;
	}
	
	c->redirect = false;
	if ( strstr( url , ".ashx?") || strstr( url , ".m3u?") || strstr( url , ".m3u8?") || strstr( url , ".pls?") ) {
		c->redirect = true;
	}
	
	return c->icy;
}

static InputStream *
input_curl_open(const char *url, Mutex &mutex, Cond &cond,
		Error &error)
{
	if (memcmp(url, "http://",  7) != 0 &&
	    memcmp(url, "https://", 8) != 0)
		return nullptr;

	return CurlInputStream::Open(url, mutex, cond, error);
}

const struct InputPlugin input_plugin_curl = {
	"curl",
	input_curl_init,
	input_curl_finish,
	input_curl_open,
};
