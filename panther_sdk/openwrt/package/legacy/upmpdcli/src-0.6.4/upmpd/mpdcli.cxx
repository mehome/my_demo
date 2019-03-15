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
#include <memory.h>

#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include <mpd/connection.h>
#include <mpd/password.h>
#include <mpd/mixer.h>
#include <mpd/status.h>
#include <mpd/song.h>
#include <mpd/tag.h>
#include <mpd/player.h>
#include <mpd/queue.h>
#include <mpd/playlist.h>
#include <mpd/send.h>
#include <mpd/response.h>

#include <upmpdcli/log.hxx>
#include <dlna/mpdcli.hxx>

using namespace std;

#define M_CONN ((struct mpd_connection *)m_conn)

MPDCli::MPDCli(const string& host, int port, const string& pass)
    : m_conn(0), m_premutevolume(0), m_cachedvolume(50),
      m_host(host), m_port(port), m_password(pass)
{
    if (!openconn()) {
        return;
    }
    m_ok = true;
    updStatus();
}

MPDCli::~MPDCli()
{
    if (m_conn) 
        mpd_connection_free(M_CONN);
}

bool MPDCli::openconn()
{
    if (m_conn) {
        mpd_connection_free(M_CONN);
        m_conn = 0;
    }
    m_conn = mpd_connection_new(m_host.c_str(), m_port, 0);
    if (m_conn == NULL) {
        LOGERR("mpd_connection_new failed. No memory?" << endl);
        return false;
    }

    if (mpd_connection_get_error(M_CONN) != MPD_ERROR_SUCCESS) {
        showError("MPDCli::openconn");
        return false;
    }

    if(!m_password.empty()) {
        if (!mpd_run_password(M_CONN, m_password.c_str())) {
            LOGERR("Password wrong" << endl);
            return false;
        }
    }
    //mpd_run_consume(M_CONN, true);
    
	return true;
}

bool MPDCli::showError(const string& who)
{
    if (!ok()) {
        LOGERR("MPDCli::showError: bad state" << endl);
        return false;
    }

    int error = mpd_connection_get_error(M_CONN);
    if (error == MPD_ERROR_SUCCESS) {
        //LOGDEB("MPDCli::showError: " << who << " success !" << endl;);
        return false;
    }
    LOGERR(who << " failed: " <<  mpd_connection_get_error_message(M_CONN) 
           << endl);
    if (error == MPD_ERROR_SERVER) {
        LOGERR(who << " server error: " << 
               mpd_connection_get_server_error(M_CONN) << endl);
    }

    if (error == MPD_ERROR_CLOSED)
        if (openconn())
            return true;
    return false;
}

#define RETRY_CMD(CMD) {                                \
    for (int i = 0; i < 2; i++) {                       \
        if ((CMD))                                      \
            break;                                      \
        if (i == 1 || !showError(#CMD))                 \
            return false;                               \
    }                                                   \
    }

bool MPDCli::updStatus()
{
    if (!ok()) {
        LOGERR("MPDCli::updStatus: bad state" << endl);
        return false;
    }

    mpd_status *mpds = 0;
    mpds = mpd_run_status(M_CONN);
    if (mpds == 0) {
        openconn();
        mpds = mpd_run_status(M_CONN);
        if (mpds == 0) {
            LOGERR("MPDCli::updStatus: can't get status" << endl);
            showError("MPDCli::updStatus");
			m_stat.state = MpdStatus::MPDS_UNK;
			return false;
        }
    }

    if (m_stat.dlna_pause = mpd_status_get_dlna_pause(mpds))
    {
        m_stat.state = MpdStatus::MPDS_PAUSE;
        mpd_status_free(mpds);
        return true;
    }

    m_stat.volume = mpd_status_get_volume(mpds);

    if (m_stat.volume >= 0) {
        m_cachedvolume = m_stat.volume;
    } else {
        m_stat.volume = m_cachedvolume;
    }

    m_stat.rept = mpd_status_get_repeat(mpds);
    m_stat.random = mpd_status_get_random(mpds);
    m_stat.single = mpd_status_get_single(mpds);
    m_stat.consume = mpd_status_get_consume(mpds);
    m_stat.qlen = mpd_status_get_queue_length(mpds);
    m_stat.qvers = mpd_status_get_queue_version(mpds);
    m_stat.linkip = mpd_status_get_linkip(mpds);
    
	switch (mpd_status_get_state(mpds)) {
    case MPD_STATE_STOP: m_stat.state = MpdStatus::MPDS_STOP;break;
    case MPD_STATE_PLAY: m_stat.state = MpdStatus::MPDS_PLAY;break;
    case MPD_STATE_PAUSE: m_stat.state = MpdStatus::MPDS_PAUSE;break;
    case MPD_STATE_UNKNOWN: 
    default:
        m_stat.state = MpdStatus::MPDS_UNK;
        break;
    }

    m_stat.crossfade = mpd_status_get_crossfade(mpds);
    m_stat.mixrampdb = mpd_status_get_mixrampdb(mpds);
    m_stat.mixrampdelay = mpd_status_get_mixrampdelay(mpds);
    m_stat.songpos = mpd_status_get_song_pos(mpds);
    m_stat.songid = mpd_status_get_song_id(mpds);
    if (m_stat.songpos >= 0) {
        updSong(m_stat.currentsong);
        updSong(m_stat.nextsong, m_stat.songpos + 1);
    }

    m_stat.songelapsedms = mpd_status_get_elapsed_ms(mpds);
    m_stat.songlenms = mpd_status_get_total_time(mpds) * 1000;
    m_stat.kbrate = mpd_status_get_kbit_rate(mpds);

    const char *err = mpd_status_get_error(mpds);
    if (err != 0)
        m_stat.errormessage.assign(err);

	m_stat.title = mpd_status_get_title(mpds);

    mpd_status_free(mpds);
    return true;
}

bool MPDCli::updSong(unordered_map<string, string>& tsong, int pos)
{
    // LOGDEB("MPDCli::updSong" << endl);
    tsong.clear();
    if (!ok())
        return false;

    struct mpd_song *song;
    if (pos == -1) {
        RETRY_CMD(song = mpd_run_current_song(M_CONN));
    } else {
        RETRY_CMD(song = mpd_run_get_queue_song_pos(M_CONN, (unsigned int)pos));
    }
        
    if (song == 0) {
        LOGERR("mpd_run_current_song failed" << endl);
        return false;
    }

    const char *cp;
    cp = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if (cp != 0)
        tsong["upnp:artist"] = cp;

    cp = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if (cp != 0)
        tsong["upnp:album"] = cp;

    cp = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if (cp != 0) {
        tsong["dc:title"] = cp;
    }

    cp = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
    if (cp != 0)
        tsong["upnp:originalTrackNumber"] = cp;

    cp = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
    if (cp != 0)
        tsong["upnp:genre"] = cp;
    
    cp = mpd_song_get_uri(song);
    if (cp != 0)
        tsong["uri"] = cp;

    mpd_song_free(song);
    return true;
}

bool MPDCli::setVolume(int volume, bool isMute)
{
    if (!ok()) {
        return false;
    }

    // MPD does not want to set the volume if not active.
    if ((m_stat.state == MpdStatus::MPDS_UNK)) {
		dlna_log("MPDCli::setVolume: not active");
		return true;
    }

    LOGDEB("MPDCli::setVolume: vol " << volume << " isMute " << isMute << " premute " << m_premutevolume << endl);

    if (isMute) {
        if (volume) {
            // Restore premute volume
			if(m_premutevolume)
				volume = m_stat.volume = m_premutevolume;
            LOGDEB("MPDCli::setVolume: restoring premute " << m_premutevolume 
                   << endl);
            m_premutevolume = 0;
        } else {
            if (m_cachedvolume > 0) {
                m_premutevolume = m_cachedvolume;
            }
        }
    }
        
    if (volume < 0)
        volume = 0;
    else if (volume > 100)
        volume = 100;
    
    RETRY_CMD(mpd_run_set_volume(M_CONN, volume));
	m_stat.volume = volume;
    m_cachedvolume = volume;
    return true;
}

int MPDCli::getVolume()
{
    return m_stat.volume >= 0 ? m_stat.volume : m_cachedvolume;
}

static int get_airplay_state()
{
	FILE *fp=NULL;
	int ret;
	char buf[12];

	fflush(NULL);

	fp=popen("air_cli s", "r");

	if(!fp)
		return 0;

	fgets(buf, 12, fp);

	if(!strncmp(buf, "YES", 3))
		ret = 1;
	else
		ret = 0;
	
	pclose(fp);

	return ret;
}

bool MPDCli::togglePause()
{
    LOGDEB("MPDCli::togglePause" << endl);
    
	if (!ok())
        return false;

    RETRY_CMD(mpd_run_toggle_pause(M_CONN));
    return updStatus();
}

bool MPDCli::play(int pos)
{
    LOGDEB("MPDCli::play(pos=" << pos << ")" << endl);

	if (!ok())
        return false;
    if (pos >= 0) {
        RETRY_CMD(mpd_run_play_pos(M_CONN, (unsigned int)pos));
    } else {
        RETRY_CMD(mpd_run_play(M_CONN));
    }
    return updStatus();
}

bool MPDCli::stop()
{
	LOGDEB("MPDCli::stop" << endl);
	if (!ok())
		return false;
    RETRY_CMD(mpd_run_stop(M_CONN));
		return true;
}
bool MPDCli::pause(bool onoff)
{
    if (!ok())
	return false;
	RETRY_CMD(mpd_run_pause(M_CONN, onoff));
	return updStatus();
}
bool MPDCli::seek(int seconds)
{
    if (!updStatus())
        return -1;
    LOGDEB("MPDCli::seek: pos:"<<m_stat.songpos<<" seconds: "<< seconds<<endl);
	RETRY_CMD(mpd_run_seek_pos(M_CONN, m_stat.songpos, (int)seconds));
    return true;
}

bool MPDCli::next()
{
    LOGDEB("MPDCli::next" << endl);
    if (!ok())
        return false;
    RETRY_CMD(mpd_run_next(M_CONN));
    return true;
}
bool MPDCli::previous()
{
    LOGDEB("MPDCli::previous" << endl);
    if (!ok())
        return false;
    RETRY_CMD(mpd_run_previous(M_CONN));
    return true;
}
bool MPDCli::repeat(bool on)
{
    LOGDEB("MPDCli::repeat:" << on << endl);
    if (!ok())
        return false;
    RETRY_CMD(mpd_run_repeat(M_CONN, on));
    return true;
}

bool MPDCli::random(bool on)
{
    LOGDEB("MPDCli::random:" << on << endl);
    if (!ok())
        return false;
    RETRY_CMD(mpd_run_random(M_CONN, on));
    return true;
}

bool MPDCli::single(bool on)
{
    LOGDEB("MPDCli::single:" << on << endl);
    if (!ok())
        return false;
    RETRY_CMD(mpd_run_single(M_CONN, on));
    return true;
}

bool MPDCli::send_tag(const char *cid, int tag, const string& data)
{                         
    if (!mpd_send_command(M_CONN, "addtagid", cid, 
                          mpd_tag_name(mpd_tag_type(tag)),
                          data.c_str(), NULL)) {
        LOGERR("MPDCli::send_tag: mpd_send_command failed" << endl);
        return false;
    }

    if (!mpd_response_finish(M_CONN)) {
        LOGERR("MPDCli::send_tag: mpd_response_finish failed\n");
        showError("MPDCli::send_tag");
        return false;
    }
    return true;
}

bool MPDCli::send_tag_data(int id, const UpSong& meta)
{
    LOGDEB("MPDCli::send_tag_data" << endl);
/*
	if (!m_have_addtagid)
        return false;
*/
    char cid[30];
    sprintf(cid, "%d", id);
    
	if (!send_tag(cid, MPD_TAG_TITLE, meta.title))
        return false;

/*
    if (!send_tag(cid, MPD_TAG_ARTIST, meta.artist))
        return false;
    if (!send_tag(cid, MPD_TAG_ALBUM, meta.album))
        return false;
    if (!send_tag(cid, MPD_TAG_TRACK, meta.tracknum))
        return false;
	if (!send_tag(cid, MPD_TAG_COMMENT, upmpdcli_comment))
        return false;
*/		
    return true;
}

bool MPDCli::dlna_pause(bool on)
{
    LOGDEB("MPDCli::DLNA Pause:" << on << endl);
    if (!ok())
        return false;
    RETRY_CMD(mpd_run_dlna_pause(M_CONN, on));
    return true;
}

bool MPDCli::setip(unsigned int ip)
{
    if (!ok())
        return false;
    RETRY_CMD(mpd_run_linkip(M_CONN, ip));
    return true;
}

int MPDCli::insert(const string& uri, const char *name, int pos)
{
	UpSong meta;
	LOGDEB("MPDCli::insert at :" << pos << " uri " << uri << endl);
	
	if (!ok())
		return -1;
	
	if (!updStatus())
		return -1;
	
	int id;
	RETRY_CMD((id=mpd_run_add_id_to(M_CONN, uri.c_str(), (unsigned)pos))!=-1);
	
	meta.clear();
	meta.title.assign(name);
	
	//send_tag_data(m_lastinsertid, meta);
	send_tag_data(id, meta);
	
	return id;
}

bool MPDCli::upname(const char *name)
{
    LOGDEB("MPDCli::update name: " << name << endl);
    if (!ok())
        return -1;

    if (!updStatus())
        return -1;

    RETRY_CMD(mpd_run_update_name(M_CONN, name));

    return true;
}
bool MPDCli::clearQueue()
{
    LOGDEB("MPDCli::clearQueue " << endl);
    if (!ok())
        return -1;

    RETRY_CMD(mpd_run_clear(M_CONN));
    return true;
}
bool MPDCli::deleteId(int id)
{
    LOGDEB("MPDCli::deleteId " << id << endl);
    if (!ok())
        return -1;

    RETRY_CMD(mpd_run_delete_id(M_CONN, (unsigned)id));
    return true;
}

bool MPDCli::deleteRange(int start, int end)
{
    LOGDEB("MPDCli::delete from " << start << " to " << end << endl);
    if (!ok())
        return -1;

    RETRY_CMD(mpd_run_delete_range(M_CONN, (unsigned)--start, (unsigned)end));
    return true;
}

bool MPDCli::moveId(int from, int to)
{
    LOGDEB("MPDCli::move from " << from << " to " << to << endl);
    if (!ok())
        return -1;

    RETRY_CMD(mpd_run_move(M_CONN, (unsigned)--from, (unsigned)--to));
    return true;
}

bool MPDCli::statId(int id)
{
    LOGDEB("MPDCli::statId " << id << endl);
    if (!ok())
        return -1;

    mpd_song *song = mpd_run_get_queue_song_id(M_CONN, (unsigned)id);
    if (song) {
        mpd_song_free(song);
        return true;
    }
    return false;
}
int MPDCli::curpos()
{
    if (!updStatus())
        return -1;
    LOGDEB("MPDCli::curpos: pos: " << m_stat.songpos << " id " << m_stat.songid << endl);
    return m_stat.songpos;
}

bool MPDCli::curlistdelsave()
{
    if (!updStatus())
        return false;
    
	unlink(CURRENTLISTPATH);
	mpd_run_save(M_CONN, CURRENTLIST);

	return true;
}

// oid : device owner id
//   0 : DLNA default
//   1 : QPlay
bool MPDCli::device_owner(int oid)
{
    m_owner = oid;
    return true;
}

#ifdef MPDCLI_TEST

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <iostream>
using namespace std;

#include <dlna/mpdcli.hxx>

static char *thisprog;

static char usage [] =
"  \n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_s	  0x2 
#define OPT_b	  0x4 

int main(int argc, char **argv)
{
  int count = 10;
    
  thisprog = argv[0];
  argc--; argv++;

  while (argc > 0 && **argv == '-') {
    (*argv)++;
    if (!(**argv))
      /* Cas du "adb - core" */
      Usage();
    while (**argv)
      switch (*(*argv)++) {
      case 's':	op_flags |= OPT_s; break;
      case 'b':	op_flags |= OPT_b; if (argc < 2)  Usage();
	if ((sscanf(*(++argv), "%d", &count)) != 1) 
	  Usage(); 
	argc--; 
	goto b1;
      default: Usage();	break;
      }
  b1: argc--; argv++;
  }

  if (argc != 0)
    Usage();

  MPDCli cli("localhost");
  if (!cli.ok()) {
      cerr << "Cli connection failed" << endl;
      return 1;
  }
  const MpdStatus& status = cli.getStatus();
  
  if (status.state != MpdStatus::MPDS_PLAY) {
      cerr << "Not playing" << endl;
      return 1;
  }

  unsigned int seektarget = (status.songlenms - 4500)/1000;
  cerr << "songpos " << status.songpos << " songid " << status.songid <<
      " seeking to " << seektarget << " seconds" << endl;

  if (!cli.seek(seektarget)) {
      cerr << "Seek failed" << endl;
      return 1;
  }
  return 0;
}
#endif

