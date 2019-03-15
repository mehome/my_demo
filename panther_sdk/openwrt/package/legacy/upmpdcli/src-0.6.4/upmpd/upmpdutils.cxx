/* Copyright (C) 2014 J.F.Dockes
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the
 *	 Free Software Foundation, Inc.,
 *	 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// 
// This file has a number of mostly uninteresting and badly
// implemented small utility functions. This is a bit ugly, but I am
// not linking to Qt or glib just to get path-concatenating
// functions...

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <math.h>
#include <pwd.h>
#include <regex.h>
#include <errno.h>
#include <string.h>
#ifndef O_STREAMING
#define O_STREAMING 0
#endif

#include <iostream>
#include <sstream>
using namespace std;

#include <dlna/mpdcli.hxx>
#include <dlna/upmpdutils.hxx>
#include <upmpdcli/log.hxx>
#include <dlna/jsonParse.h>
// Append system error string to input string
void catstrerror(string *reason, const char *what, int _errno)
{
    if (!reason)
	return;
    if (what)
	reason->append(what);

    reason->append(": errno: ");

    char nbuf[20];
    sprintf(nbuf, "%d", _errno);
    reason->append(nbuf);

    reason->append(" : ");

#ifdef sun
    // Note: sun strerror is noted mt-safe ??
    reason->append(strerror(_errno));
#else
#define ERRBUFSZ 200    
    char errbuf[ERRBUFSZ];
    // There are 2 versions of strerror_r. 
    // - The GNU one returns a pointer to the message (maybe
    //   static storage or supplied buffer).
    // - The POSIX one always stores in supplied buffer and
    //   returns 0 on success. As the possibility of error and
    //   error code are not specified, we're basically doomed
    //   cause we can't use a test on the 0 value to know if we
    //   were returned a pointer... 
    // Also couldn't find an easy way to disable the gnu version without
    // changing the cxxflags globally, so forget it. Recent gnu lib versions
    // normally default to the posix version.
    // At worse we get no message at all here.
    errbuf[0] = 0;
    (void)strerror_r(_errno, errbuf, ERRBUFSZ);
    reason->append(errbuf);
#endif
}

bool file_to_string(const string &fn, string &data, string *reason)
{
    const int RDBUFSZ = 4096;
    bool ret = false;
    int fd = -1;
    struct stat st;

    fd = open(fn.c_str(), O_RDONLY|O_STREAMING);
    if (fd < 0 || fstat(fd, &st) < 0) {
        catstrerror(reason, "open/stat", errno);
        return false;
    }

    data.reserve(st.st_size+1);

    char buf[RDBUFSZ];
    for (;;) {
	int n = read(fd, buf, RDBUFSZ);
	if (n < 0) {
	    catstrerror(reason, "read", errno);
	    goto out;
	}
	if (n == 0)
	    break;

        data.append(buf, n);
    }

    ret = true;
out:
    if (fd >= 0)
	close(fd);
    return ret;
}

void path_catslash(string &s) {
    if (s.empty() || s[s.length() - 1] != '/')
	s += '/';
}

string path_cat(const string &s1, const string &s2) {
    string res = s1;
    path_catslash(res);
    res +=  s2;
    return res;
}

string path_home()
{
    uid_t uid = getuid();

    struct passwd *entry = getpwuid(uid);
    if (entry == 0) {
	const char *cp = getenv("HOME");
	if (cp)
	    return cp;
	else 
	return "/";
    }

    string homedir = entry->pw_dir;
    path_catslash(homedir);
    return homedir;
}

string path_tildexpand(const string &s) 
{
    if (s.empty() || s[0] != '~')
	return s;
    string o = s;
    if (s.length() == 1) {
	o.replace(0, 1, path_home());
    } else if  (s[1] == '/') {
	o.replace(0, 2, path_home());
    } else {
	string::size_type pos = s.find('/');
	int l = (pos == string::npos) ? s.length() - 1 : pos - 1;
	struct passwd *entry = getpwnam(s.substr(1, l).c_str());
	if (entry)
	    o.replace(0, l+1, entry->pw_dir);
    }
    return o;
}

void trimstring(string &s, const char *ws)
{
    string::size_type pos = s.find_first_not_of(ws);
    if (pos == string::npos) {
	s.clear();
	return;
    }
    s.replace(0, pos, string());

    pos = s.find_last_not_of(ws);
    if (pos != string::npos && pos != s.length()-1)
	s.replace(pos+1, string::npos, string());
}

string xmlquote(const string& in)
{
    string out;
    for (unsigned int i = 0; i < in.size(); i++) {
        switch(in[i]) {
        case '"': out += "&quot;";break;
        case '&': out += "&amp;";break;
        case '<': out += "&lt;";break;
        case '>': out += "&gt;";break;
        case '\'': out += "&apos;";break;
        default: out += in[i];
        }
    }
    return out;
}

// Translate 0-100% MPD volume to UPnP VolumeDB: we do db upnp-encoded
// values from -10240 (0%) to 0 (100%)
int percentodbvalue(int value)
{
    int dbvalue;
    if (value == 0) {
        dbvalue = -10240;
    } else {
        float ratio = float(value)*value / 10000.0;
        float db = 10 * log10(ratio);
        dbvalue = int(256 * db);
    }
    return dbvalue;
}

#ifdef __APPLE__
#define exp10 __exp10
#endif

// Translate VolumeDB to MPD 0-100
int dbvaluetopercent(int dbvalue)
{
    float db = float(dbvalue) / 256.0;
    float vol = pow(10, db/10); //    float vol = exp10(db/10);
    int percent = floor(sqrt(vol * 10000.0));
    if (percent < 0)	percent = 0;
    if (percent > 100)	percent = 100;
    return percent;
}
// Format duration in milliseconds into UPnP duration format
string upnpduration(int ms)
{
    int hours = ms / (3600 * 1000);
    ms -= hours * 3600 * 1000;
    int minutes = ms / (60 * 1000);
    ms -= minutes * 60 * 1000;
    int secs = ms / 1000;
    ms -= secs * 1000;

    char cbuf[100];

    // This is the format from the ref doc, but it appears that the
    // decimal part in the seconds field is an issue with some control
    // points. So drop it...
    //  sprintf(cbuf, "%d:%02d:%02d.%03d", hours, minutes, secs, ms);
    sprintf(cbuf, "%02d:%02d:%02d", hours, minutes, secs);
    return cbuf;
}

// H:M:S to seconds
int upnpdurationtos(const string& dur)
{
    int hours, minutes, seconds;
    if (sscanf(dur.c_str(), "%d:%d:%d", &hours, &minutes, &seconds) != 3) {
	return 0;
    }
    return 3600 * hours + 60 * minutes + seconds;
}

// Get from ssl unordered_map, return empty string for non-existing
// key (so this only works for data where this behaviour makes sense).
const string& mapget(const unordered_map<string, string>& im, const string& k)
{
    static string ns; // null string
    unordered_map<string, string>::const_iterator it = im.find(k);
    if (it == im.end())
        return ns;
    else
        return it->second;
}

// Bogus didl fragment maker. We probably don't need a full-blown XML
// helper here
string didlmake(const MpdStatus& mpds, bool next)
{
    const unordered_map<string, string>& songmap = 
        next? mpds.nextsong : mpds.currentsong;
    ostringstream ss;
    ss << "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
        "xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\">"
       << "<item restricted=\"1\">";

    {   const string& val = mapget(songmap, "dc:title");
        ss << "<dc:title>" << xmlquote(val) << "</dc:title>";
    }
	
    // TBD Playlists etc?
    ss << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>";

    {   const string& val = mapget(songmap, "upnp:artist");
        if (!val.empty()) {
            string a = xmlquote(val);
            ss << "<dc:creator>" << a << "</dc:creator>" << 
                "<upnp:artist>" << a << "</upnp:artist>";
        }
    }

    {   const string& val = mapget(songmap, "upnp:album");
        if (!val.empty()) {
            ss << "<upnp:album>" << xmlquote(val) << "</upnp:album>";
        }
    }

    {   const string& val = mapget(songmap, "upnp:genre");
        if (!val.empty()) {
            ss << "<upnp:genre>" << xmlquote(val) << "</upnp:genre>";
        }
    }

    {const string& val = mapget(songmap, "upnp:originalTrackNumber");
        if (!val.empty()) {
            ss << "<upnp:originalTrackNumber>" << val << 
                "</upnp:originalTrackNumber>";
        }
    }

    // TBD: the res element normally has size, sampleFrequency,
    // nrAudioChannels and protocolInfo attributes, which are bogus
    // for the moment. partly because MPD does not supply them.  And
    // mostly everything is bogus if next is set...  

    ss << "<res " << "duration=\"" << upnpduration(mpds.songlenms) << "\" "
	// Bitrate keeps changing for VBRs and forces events. Keeping
	// it out for now.
	//       << "bitrate=\"" << mpds.kbrate << "\" "
       << "sampleFrequency=\"44100\" audioChannels=\"2\" "
       << "protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000\""
       << ">"
       << xmlquote(mapget(songmap, "uri")) 
       << "</res>"
       << "</item></DIDL-Lite>";
    return ss.str();
}

//string http_didlmake(const MpdStatus& mpds,WARTI_pMusic p,bool next)
string http_didlmake(const MpdStatus& mpds,WARTI_MusicPtr p)
{
   // const unordered_map<string, string>& songmap = 
    //    next? mpds.nextsong : mpds.currentsong;
        	 const unordered_map<string, string>& songmap = 
                  mpds.currentsong;
    ostringstream ss;
    ss << "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
        "xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\">"
       << "<item restricted=\"1\">";
      
    {  // const string& val = mapget(songmap, "dc:title");
	      const string& val = p->pTitle;
        ss << "<dc:title>" << xmlquote(val) << "</dc:title>";
        LOGDEB("quejw 111111 xmlquote(val) =" << xmlquote(val) << endl);
    }
	
	//LOGDEB("quejw 111111 p->pTitle =" <<  p->pTitle << endl);
	/*const string& val = "quejw 3333333333333";
	ss << "<dc:title>" << xmlquote(val) << "</dc:title>";
	LOGDEB("quejw 	ssssssss val =" << ss << endl);
	*/
    // TBD Playlists etc?
    ss << "<upnp:class>object.item.audioItem.musicTrack</upnp:class>";
    
    {  // const string& val = mapget(songmap, "upnp:artist");
    	 const string& val = p->pArtist;
    	// LOGDEB("quejw 222222 p->pArtist =" <<  p->pArtist << endl);
        if (!val.empty()) {
            string a = xmlquote(val);
            LOGDEB("quejw 222222 p->pArtist =" << xmlquote(val) << endl);
            ss << "<dc:creator>" << a << "</dc:creator>" << 
                "<upnp:artist>" << a << "</upnp:artist>";
        }
    }

    {  // const string& val = mapget(songmap, "upnp:album");
    	  const string& val = p->pAlbum;
    	//  LOGDEB("quejw 333333  p->pAlbum =" <<   p->pAlbum << endl);
        if (!val.empty()) {
            ss << "<upnp:album>" << xmlquote(val) << "</upnp:album>";
             LOGDEB("quejw 222222 p->pAlbum =" << xmlquote(val) << endl);
        }
    }

    {   const string& val = mapget(songmap, "upnp:genre");
        if (!val.empty()) {
            ss << "<upnp:genre>" << xmlquote(val) << "</upnp:genre>";
        }
    }

	 {//const string& val = mapget(songmap, "upnp:artUri");
	 //const string& val = "http://fdfs.xmcdn.com/group6/M06/9D/38/wKgDg1UGTRXwYEl4AAGaOmfRQbA928_web_large.jpg";
	  const string& val = p->pCoverURL;
	 //  LOGDEB("quejw 333333 p->pCoverURL =" <<   p->pCoverURL << endl);
	  
        if (!val.empty()) {
            ss << "<upnp:albumArtURI>" <<xmlquote(val) << 
                "</upnp:albumArtURI>";
                 LOGDEB("quejw 333333 p->pCoverURL =" <<xmlquote(val)<< endl);
        }
    }


    {const string& val = mapget(songmap, "upnp:originalTrackNumber");
        if (!val.empty()) {
            ss << "<upnp:originalTrackNumber>" << val << 
                "</upnp:originalTrackNumber>";
        }
    }

    // TBD: the res element normally has size, sampleFrequency,
    // nrAudioChannels and protocolInfo attributes, which are bogus
    // for the moment. partly because MPD does not supply them.  And
    // mostly everything is bogus if next is set...  

    ss << "<res " << "duration=\"" << upnpduration(mpds.songlenms) << "\" "
	// Bitrate keeps changing for VBRs and forces events. Keeping
	// it out for now.
	//       << "bitrate=\"" << mpds.kbrate << "\" "
       << "sampleFrequency=\"44100\" audioChannels=\"2\" "
       << "protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000\""
       << ">"
       << xmlquote(mapget(songmap, "uri")) 
       << "</res>"
       << "</item></DIDL-Lite>";
      /*
       free(p);
       p = NULL;
       */
    return ss.str();
}



// Substitute regular expression
// The c++11 regex package does not seem really ready from prime time
// (Tried on gcc + libstdc++ 4.7.2-5 on Debian, with little
// joy). So...:
string regsub1(const string& sexp, const string& input, const string& repl)
{
    regex_t expr;
    int err;
    const int ERRSIZE = 200;
    char errbuf[ERRSIZE+1];
    regmatch_t pmatch[10];

    if ((err = regcomp(&expr, sexp.c_str(), REG_EXTENDED))) {
        regerror(err, &expr, errbuf, ERRSIZE);
        LOGERR("upmpd: regsub1: regcomp() failed: " << errbuf << endl);
        return string();
    }
    
    if ((err = regexec(&expr, input.c_str(), 10, pmatch, 0))) {
        regerror(err, &expr, errbuf, ERRSIZE);
        //LOGDEB("upmpd: regsub1: regexec(" << sexp << ") failed: "
        //    <<  errbuf << endl);
        regfree(&expr);
        return string();
    }
    if (pmatch[0].rm_so == -1) {
        // No match
        regfree(&expr);
        return input;
    }
    string out = input.substr(0, pmatch[0].rm_so);
    out += repl;
    out += input.substr(pmatch[0].rm_eo);
    regfree(&expr);
    return out;
}

// We do not want to mess with the pidfile content in the destructor:
// the lock might still be in use in a child process. In fact as much
// as we'd like to reset the pid inside the file when we're done, it
// would be very difficult to do it right and it's probably best left
// alone.
Pidfile::~Pidfile()
{
    if (m_fd >= 0)
	::close(m_fd);
    m_fd = -1;
}

pid_t Pidfile::read_pid()
{
    int fd = ::open(m_path.c_str(), O_RDONLY);
    if (fd == -1)
	return (pid_t)-1;

    char buf[16];
    int i = read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (i <= 0)
	return (pid_t)-1;
    buf[i] = '\0';
    char *endptr;
    pid_t pid = strtol(buf, &endptr, 10);
    if (endptr != &buf[i])
	return (pid_t)-1;
    return pid;
}

int Pidfile::flopen()
{
    const char *path = m_path.c_str();
    if ((m_fd = ::open(path, O_RDWR|O_CREAT, 0644)) == -1) {
	m_reason = "Open failed: [" + m_path + "]: " + strerror(errno);
	return -1;
    }

#ifdef sun
    struct flock lockdata;
    lockdata.l_start = 0;
    lockdata.l_len = 0;
    lockdata.l_type = F_WRLCK;
    lockdata.l_whence = SEEK_SET;
    if (fcntl(m_fd, F_SETLK,  &lockdata) != 0) {
	int serrno = errno;
	(void)::close(m_fd);
	errno = serrno;
	m_reason = "fcntl lock failed";
	return -1;
    }
#else
    int operation = LOCK_EX | LOCK_NB;
    if (flock(m_fd, operation) == -1) {
	int serrno = errno;
	(void)::close(m_fd);
	errno = serrno;
	m_reason = "flock failed";
	return -1;
    }
#endif // ! sun

    if (ftruncate(m_fd, 0) != 0) {
	/* can't happen [tm] */
	int serrno = errno;
	(void)::close(m_fd);
	errno = serrno;
	m_reason = "ftruncate failed";
	return -1;
    }
    return 0;
}

pid_t Pidfile::open()
{
    if (flopen() < 0) {
	return read_pid();
    }
    return (pid_t)0;
}

int Pidfile::write_pid()
{
    /* truncate to allow multiple calls */
    if (ftruncate(m_fd, 0) == -1) {
	m_reason = "ftruncate failed";
	return -1;
    }
    char pidstr[20];
    sprintf(pidstr, "%u", int(getpid()));
    lseek(m_fd, 0, 0);
    if (::write(m_fd, pidstr, strlen(pidstr)) != (ssize_t)strlen(pidstr)) {
	m_reason = "write failed";
	return -1;
    }
    return 0;
}

int Pidfile::close()
{
    return ::close(m_fd);
}

int Pidfile::remove()
{
    return unlink(m_path.c_str());
}
