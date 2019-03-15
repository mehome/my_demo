/*=============================================================================+
|                                                                              |
| Copyright 2016                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <json-c/json.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>
#include <set>
#include <cerrno>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
using namespace std;
using namespace std::placeholders;

#include "libupnpp/upnpplib.hxx"
#include "libupnpp/soaphelp.hxx"
#include "libupnpp/log.hxx"
#include "dlna/upmpd.hxx"

#define Q_CLEAR 0x1
#define Q_STOP 0x2

bool UpMpd::chk_valid(unsigned int ip)                        
{                                                             
    /*Note:Currently we do not check zero status.*/           
    unsigned int linkip = get_linkip();                       
                                                              
    return ip == linkip;                                      
}                                             

int UpMpd::getTracksInfo(const SoapArgs& sc, SoapData& data)
{
	if(!allmetadata.empty()) {
		dlna_log("SEND ALLMETADATA\n");
		data.addarg("TracksMetaData", allmetadata.c_str());
#ifdef AVOID_TWINKLING	
		update = 2;
#endif
		loopWakeup();
	}
	else
		data.addarg("TracksMetaData", "");
	return UPNP_E_SUCCESS;
}

void UpMpd::ParseQPlayTracksInfo(const char *metadata, string& startidx, string& nextidx)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	json_object *jobj=NULL, *jmeta=NULL, *jtrack=NULL, *juri=NULL, *jhttp=NULL, *jtitle=NULL;
	int arraylen,urilen,i,j;
	int action = 0;
	int idx;
	int curpos = mpds.songpos;
	int skip=-1;
	FILE *fd;

	idx = atoi(nextidx.c_str());

	if(startidx.compare("-1")==0)
		action |= Q_CLEAR;
	if(nextidx.compare("-1")==0)
		action |= Q_STOP;

	dlna_log("==== start:%s, next:%s\n",startidx.c_str(),nextidx.c_str());

	if(action == (Q_CLEAR|Q_STOP)) {
		songinfo.clear();
#ifdef DLNA_MPD_RECOVER
		songtitle.clear();
#endif
		m_mpdcli->clearQueue();
	}
	else if(action == Q_STOP)
		m_mpdcli->stop();
	else if (action == Q_CLEAR) {
		skip = idx - 2;
#ifdef QQ_PLAYLIST
		if(skip == 0) {
			dlna_log("==== Need to Append\n");	
		}
		else 
#endif
		{
			dlna_log("==== Need to Clear\n");
			if(curpos!=0)
				m_mpdcli->moveId(curpos+1, 1);

			if(mpds.qlen > 1)
				m_mpdcli->deleteRange(2, mpds.qlen);

			songinfo.clear();
#ifdef DLNA_MPD_RECOVER
			songtitle.clear();
#endif
		}
	}

	jobj = json_tokener_parse((char *)metadata);
	
	jmeta = json_object_object_get(jobj, "TracksMetaData");

	arraylen = json_object_array_length(jmeta);
	
	if((arraylen < atoi(nextidx.c_str())) && ((action & Q_STOP) == 0)) {
		action |= Q_STOP;
		m_mpdcli->stop();
	}

#ifdef QQ_PLAYLIST
	if(skip!=0)
#endif
		fd=fopen(CURRENTLISTPATH, "w");
#ifdef QQ_PLAYLIST
	else
		fd=fopen(CURRENTLISTPATH, "a");
#endif

	for (i = 0; i < arraylen; i++) { 
		// get the i-th object in meta array  
	    jtrack = json_object_array_get_idx(jmeta, i); 
		jtitle = json_object_object_get(jtrack, "title");
		
		juri = json_object_object_get(jtrack,"trackURIs"); 
	    urilen = json_object_array_length(juri); 
	    for(j=0; j<urilen;j++) {
	        jhttp = json_object_array_get_idx(juri, j);
	    }

		if(fd) {	
			string http	= json_object_get_string(jhttp);
#ifdef QQ_PLAYLIST
			if((skip==0) && (i==0)) {
				;
			}
			else
#endif
				fprintf(fd, "%s\n", http.c_str());
		}

		if(action&Q_CLEAR) {
			if(jtrack)
#ifdef QQ_PLAYLIST
				if(skip == 0) {
					if(i)
						songinfo.push_back(json_object_get_string(jtrack));
				}
				else
#endif
					songinfo.push_back(json_object_get_string(jtrack));

#ifdef DLNA_MPD_RECOVER
			if(jtitle)
#ifdef QQ_PLAYLIST
				if(skip == 0) {
					if(i)
						songtitle.push_back(json_object_get_string(jtitle));
				}
				else
#endif
					songtitle.push_back(json_object_get_string(jtitle));
#endif

			if(action == Q_CLEAR) {
#ifdef QQ_PLAYLIST
				if(skip == 0) {
					if(i)
						m_mpdcli->insert(json_object_get_string(jhttp),json_object_get_string(jtitle) ,i + 199);
				}
				else 
#endif
				{
					if(arraylen == 1)
						goto done;
					if(i != skip) {
						if(skip != -1) {
							m_mpdcli->insert(json_object_get_string(jhttp),json_object_get_string(jtitle) ,i);
							//LOGERR("Insert Title " << json_object_get_string(jtitle)  << endl);
						}
						else if(i != arraylen - 1) {
							m_mpdcli->insert(json_object_get_string(jhttp),json_object_get_string(jtitle) ,i);
							//LOGERR("Insert Title " << json_object_get_string(jtitle)  << endl);
						}
					}
				}
			}
			else { 
				//LOGERR("Insert Title " << json_object_get_string(jtitle) << " idx " << i << endl);
				m_mpdcli->insert(json_object_get_string(jhttp),json_object_get_string(jtitle) ,i);
			}
		}
	}
done:
	if(fd)
		fclose(fd);
	if(arraylen) {
		json_object_put(jtrack);
		json_object_put(jtitle);
		json_object_put(juri);
		json_object_put(jhttp);
	}
	json_object_put(jobj);
	json_object_put(jmeta);

	dlna_log("==== Parse Done\n");
//	LOGERR("attr " << metadata << endl);
	return;
}

int UpMpd::setTracksInfo(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;

	// check NextIndex valid
	it = sc.args.find("NextIndex");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	string nextIdx = it->second;

	it = sc.args.find("StartingIndex");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	string startIdx = it->second;

	// TracksMetaData
	it = sc.args.find("TracksMetaData");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}

	const char *metadata = it->second.c_str();
	allmetadata.clear();
	allmetadata = it->second;
	ParseQPlayTracksInfo(metadata,startIdx, nextIdx);

	const MpdStatus &mpds = m_mpdcli->getStatus();
	int curpos = mpds.songpos;

	// curpos == -1 means that the playlist was cleared or we just started. A
	// play will use position 0, so it's actually equivalent to curpos == 0
	if (curpos == -1) {
		curpos = 0;
	}

	char nos[8];
	sprintf(nos, "%d", songinfo.size());
	data.addarg("NumberOfSuccess", nos);

	loopWakeup();
	return UPNP_E_SUCCESS;
}

int UpMpd::QPlayAuth(const SoapArgs& sc, SoapData& data)
{
	LOGERR("Hello QPlayAuth" << endl);

	map<string, string>::const_iterator it;
		
	it = sc.args.find("Seed");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}

	string seed = it->second;
	string input = seed;
	string hash;
	const unsigned char PSK[16] ={ 0xe6, 0xbe, 0x9c, 0xe8, 0xb5, 0xb7, 0xe7, 0xa7,
						  0x91, 0xe6, 0x8a, 0x80, 0x3a, 0x57, 0x69, 0x2d};
	for (int i=0; i<16; i++)
		input.append(1, PSK[i]);
		
	MD5String(input, hash);

	char hashstring[100];
	sprintf(hashstring, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			hash[0]&0xff, hash[1]&0xff, hash[2]&0xff, hash[3]&0xff,
			hash[4]&0xff, hash[5]&0xff, hash[6]&0xff, hash[7]&0xff,
			hash[8]&0xff, hash[9]&0xff, hash[10]&0xff, hash[11]&0xff,
			hash[12]&0xff, hash[13]&0xff, hash[14]&0xff, hash[15]&0xff
			);

	data.addarg("Code", hashstring);
	data.addarg("MID", "62900065");
	data.addarg("DID", "TonlyDMR");
	//loopWakeup();
	return UPNP_E_SUCCESS;
}

int UpMpd::setNetWork(const SoapArgs& sc, SoapData& data)
{
	const char *ssid = NULL ;
	const char *pass = NULL ;
	char cmd[512];
	
	map<string, string>::const_iterator it;
	
	it = sc.args.find("SSID");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	ssid = it->second.c_str();
	LOGDEB("ssid: " << ssid << endl);

	it = sc.args.find("Key");
	
	if (!it->second.empty()) {
		pass = it->second.c_str();
		LOGDEB("pass: " << pass << endl);
	}
	
	if(pass)
		sprintf(cmd, "/lib/wdk/omnicfg_apply 3 %s %s",ssid,pass);
	else
		sprintf(cmd, "/lib/wdk/omnicfg_apply 3 %s",ssid);
	
	system(cmd);
	
	return UPNP_E_SUCCESS;
}

int UpMpd::getMaxTracks(const SoapArgs& sc, SoapData& data)
{
	data.addarg("MaxTracks", "200");
	
	return UPNP_E_SUCCESS;
}

#if 0
int UpMpd::removeTrack(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;
	unsigned int startnum, delnum;

	it = sc.args.find("QueueID");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	std::string curqid = m_queueID.substr(8);

	if(curqid.compare(it->second))
		return UPNP_E_INVALID_PARAM;
	
	it = sc.args.find("StartingIndex");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	string startIdx = it->second;
	
	if(!startIdx.compare("-1"))
		startnum = 1;
	else
		startnum = atoi(it->second.c_str());

	it = sc.args.find("NumberOfTracks");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	delnum = atoi(it->second.c_str());

	if(delnum == 200)
		delnum = songinfo.size();
	
	m_mpdcli->deleteRange(startnum, startnum + delnum - 1);
	songinfo.erase(songinfo.begin()+startnum-1,songinfo.begin()+delnum);
	
	data.addarg("NumberOfSuccess", it->second);
	return UPNP_E_SUCCESS;
}
#endif

// Translate MPD mode flags to UPnP Play mode
std::string mpdsToPlaymode(const MpdStatus& mpds)
{
	string playmode = "NORMAL";
    if (!mpds.rept && mpds.random && !mpds.single)
		playmode = "SHUFFLE";
	else if (mpds.rept && !mpds.random && mpds.single)
		playmode = "REPEAT_ONE";
	else if (mpds.rept && !mpds.random && !mpds.single)
		playmode = "REPEAT_ALL";
	else if (mpds.rept && mpds.random && !mpds.single)
		playmode = "RANDOM";
	else if (!mpds.rept && !mpds.random && mpds.single)
		playmode = "DIRECT_1";
	return playmode;
}

bool UpMpd::tpstateMToU_dlnalib(unordered_map<string, string>& status)
{
#ifdef AVOID_TWINKLING
	static int oldplayms = 0;
#endif
	const MpdStatus &mpds = m_mpdcli->getStatus();
	bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
		(mpds.state == MpdStatus::MPDS_PAUSE);

#ifdef DLNA_MPD_RECOVER
	if(is_song) {
		if(!mpds.title) {
			if(songtitle.size() > mpds.songpos) {
				string title = songtitle[mpds.songpos];
				m_mpdcli->upname(title.c_str());
			}
		}
	}
#endif

	const string& uri = mapget(mpds.currentsong, "uri");
#ifdef AVOID_TWINKLING
	if(!m_queueID.empty()) {
		time_t curtime;
		if(update)
			goto updatenow;

		if(is_song) {
			const string oldurl = mapget(m_tpstate, "CurrentTrackURI");
			const string cururl = mapget(mpds.currentsong, "uri");

			/*Playing song changed*/
			if (cururl.compare(oldurl)) {
				elapstime = 0;
				dlna_log("==== Change Song\n");
				update = 1;
			}

			// every 10 sec sync with APP, but only reduce the twinking frequency
			if((curtime=time(NULL)) > uptime && !(update)) {
				if(mpds.state == MpdStatus::MPDS_PLAY) {
					unsigned int chktime;
					if(mpds.songelapsedms < elapstime) {
						dlna_log("!!!!!!!!!!! ELAPSTIME MS:%d CUR:%d MUSIC DELAY !!!!!!!!!\n",elapstime, mpds.songelapsedms);
						update = 1;
					}
					chktime = mpds.songelapsedms + ELAPSMS;
					if(!elapstime)
						dlna_log("!!!!!!!!!!! FIRST set time:%d !!!!!!!!!\n",chktime);
					elapstime =(chktime>mpds.songlenms)?mpds.songlenms:(chktime);
					if(!update)
						uptime = curtime + UPTIMER;
				}
			}
		}
updatenow:
		if(update) {
			m_tpstate["TransportState"]="modify";
			m_tpstate["CurrentTrackMetaData"]="modify";
			m_tpstate["CurrentTrackURI"]="modify";
			m_tpstate["AVTransportURI"]="modify";
			dlna_log("==== update enable\n");
			uptime = time(NULL) + UPTIMER;
		}
	}
#endif
	/*For Encode*/
	status["TransportStatus"] = m_mpdcli->ok() ? "OK" : "ERROR_OCCURRED";
//	status["NextAVTransportURI"] = mapget(mpds.nextsong, "uri");
	if (is_song) {
		if(songinfo.size() > mpds.songpos + 1) {
			string tmpinfo = songinfo[mpds.songpos + 1];
//			status["NextAVTransportURIMetaData"] = tmpinfo.c_str();
		}
//		else
//			status["NextAVTransportURIMetaData"] = "";
	} 
//	else {
//		status["NextAVTransportURIMetaData"] = "";
//	}

	if (is_song) {
		if(!songinfo.empty()) {
			if(songinfo.size() > mpds.songpos) {
				string tmpinfo = songinfo[mpds.songpos];
				status["CurrentTrackMetaData"] = tmpinfo.c_str();
				status["AVTransportURIMetaData"] = tmpinfo.c_str();
			}
			else {
				status["CurrentTrackMetaData"] = "";
				status["AVTransportURIMetaData"] = "";
			}
		}
		else {
			status["CurrentTrackMetaData"] = "";
			status["AVTransportURIMetaData"] = "";
		}
	} else {
		status["CurrentTrackMetaData"] = "";
		status["AVTransportURIMetaData"] = "";
	}

#if 0
	/*
	 According to AVTransport v1 p.17
	 The following variables are thererefore not evented via LastChange
	*/
	status["RelativeCounterPosition"] = "2147483647";
	status["AbsoluteCounterPosition"] = "2147483647";
	status["RelativeTimePosition"] = is_song?upnpduration(mpds.songelapsedms):"00:00:00";
	status["AbsoluteTimePosition"] = is_song?"NOT_IMPLEMENTED":"END_OF_MEDIA";
#endif

	string playmedium("NONE");
	if (is_song)
		playmedium = uri.find("http://") == 0 ?	"NONE" : "NETWORK";
	status["PlaybackStorageMedium"] = playmedium;
	status["PossibleRecordStorageMedia"] = "NOT_IMPLEMENTED";
	status["CurrentPlayMode"] = mpdsToPlaymode(mpds);
	status["TransportPlaySpeed"] = "1";
	status["PossiblePlaybackStorageMedia"] = "HDD,NETWORK";

	char curTrack[8];
	sprintf(curTrack, "%d", mpds.songpos + 1);
	status["CurrentTrack"] = curTrack;

	status["CurrentTrackURI"] = (is_song&&(!uri.empty()))?uri:"";
	
	string tactions("Next,Previous");
	string tstate("STOPPED");
	switch(mpds.state) {
		case MpdStatus::MPDS_PLAY: 
			tstate = "PLAYING"; 
			tactions += ",Pause,Stop,Seek";
			break;
		case MpdStatus::MPDS_PAUSE: 
			tstate = "PAUSED_PLAYBACK"; 
			tactions += ",Play,Stop,Seek";
			break;
		default:
			tactions += ",Play";
	}
	status["CurrentTransportActions"] = tactions;

	char nos[8];
	sprintf(nos, "%d", songinfo.size());
	status["NumberOfTracks"] = nos;

	if(is_song) {
		unsigned int linkip = get_linkip();
		if(m_queueID.empty()) {
			if((mpds.dlna_pause) || ((mpds.linkip != linkip) && (mpds.linkip != 0))) {
				status["AVTransportURI"] = FAKEURI;
			}
			else {
				status["AVTransportURI"] = uri;
			}
		}
		else {
			if((mpds.dlna_pause) || ((mpds.linkip != linkip) && (mpds.linkip != 0))) {
				status["AVTransportURI"] = FAKEURI;
			}
			else {
				status["AVTransportURI"] = m_queueID;
			}
		}
	}
	else
		status["AVTransportURI"] = "";
	
	status["CurrentRecordQualityMode"] = "NOT_IMPLEMENTED";

	status["RecordStorageMedium"] = "NOT_IMPLEMENTED";
	status["RecordMediumWriteStatus"] = "NOT_IMPLEMENTED";

	status["CurrentTrackDuration"] = is_song?upnpduration(mpds.songlenms):"00:00:00";
	status["CurrentMediaDuration"] = is_song?upnpduration(mpds.songlenms):"00:00:00";
	
	status["TransportState"] = tstate;
	status["PossibleRecordQualityModes"] = "NOT_IMPLEMENTED";
	return true;
}

/*Another Encode*/
#if 0
bool UpMpd::tpstateMToU_dlnalib(unordered_map<string, string>& status)
{
#ifdef AVOID_TWINKLING
	static int oldplayms = 0;
#endif
	const MpdStatus &mpds = m_mpdcli->getStatus();
//	DEBOUT << "UpMpd::tpstateMToU: curpos: " << mpds.songpos <<
//	   " qlen " << mpds.qlen << endl;
	bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
		(mpds.state == MpdStatus::MPDS_PAUSE);

	if(is_song) {
		if(!mpds.title) {
			if(songtitle.size() > mpds.songpos) {
				string title = songtitle[mpds.songpos];
				m_mpdcli->upname(title.c_str());
			}
		}
	}

	const string& uri = mapget(mpds.currentsong, "uri");
#if 0	
#ifdef AVOID_TWINKLING
	if(!m_queueID.empty()) {
		time_t curtime;
		if(update)
			goto updatenow;

		if(is_song) {
			const string oldurl = mapget(m_tpstate, "CurrentTrackURI");
			const string cururl = mapget(mpds.currentsong, "uri");
			if (cururl.compare(oldurl)) {
				//songelapsedms = mpds.songelapsedms;
				update = 1;
			}
			else {	
				/*For QQ single repeat*/
				if(songinfo.size() == 1) {
					if(mpds.songelapsedms < oldplayms)
						update = 1;
					oldplayms = mpds.songelapsedms;
				}
				else if(songinfo.size())
					oldplayms = 0;
			}
			// every 10 sec sync with APP, but only reduce the twinking frequency
			if((curtime=time(NULL)) > uptime) {
				uptime = curtime + UPTIMER;
				dlna_log("!!!!!!!!!!! UPDATE NOW UPTIME:%d!!!!!!!!!\n",uptime);
				update = 1;
			}
#if 0
			if ((mpds.songelapsedms - songelapsedms) > 10000)
			{
				songelapsedms = mpds.songelapsedms;
				update = 1;
			}
#endif
		}
updatenow:
		if(update) {
			m_tpstate["TransportState"]="";
			m_tpstate["CurrentTrackMetaData"]="";
			m_tpstate["CurrentTrackURI"]="";
			m_tpstate["AVTransportURI"]="";

			update = 0;
		}
	}
#endif
#endif
	/*For Encode*/
	string tstate("STOPPED");
	string tactions;
	switch(mpds.state) {
		case MpdStatus::MPDS_PLAY: 
			tstate = "PLAYING"; 
			tactions += "Pause,Stop,Seek";
			break;
		case MpdStatus::MPDS_PAUSE: 
			tstate = "PAUSED_PLAYBACK"; 
			tactions += "Play,Stop,Seek";
			break;
		default:
			tactions += "Play";
	}
	status["TransportState"] = tstate;
	status["TransportStatus"] = m_mpdcli->ok() ? "OK" : "ERROR_OCCURRED";
	status["PlaybackStorageMedium"] = "UNKNOWN";
	status["PossiblePlaybackStorageMedia"] = "NETWORK,UNKNOWN";
	status["RecordStorageMedium"] = "NOT_IMPLEMENTED";
	status["PossibleRecordStorageMedia"] = "NOT_IMPLEMENTED";
	status["CurrentPlayMode"] = mpdsToPlaymode(mpds);
	status["TransportPlaySpeed"] = "1";
	status["RecordMediumWriteStatus"] = "NOT_IMPLEMENTED";
	status["CurrentRecordQualityMode"] = "NOT_IMPLEMENTED";
	status["PossibleRecordQualityModes"] = "NOT_IMPLEMENTED";
	char nos[8];
	sprintf(nos, "%d", songinfo.size());
	status["NumberOfTracks"] = nos;
	char curTrack[8];
	sprintf(curTrack, "%d", mpds.songpos + 1);
	status["CurrentTrack"] = curTrack;
	status["CurrentTrackDuration"] = is_song?upnpduration(mpds.songlenms):"00:00:00";
	status["CurrentMediaDuration"] = is_song?upnpduration(mpds.songlenms):"00:00:00";
	if (is_song) {
		if(!songinfo.empty()) {
			if((int)songinfo.size() > mpds.songpos) {
				string tmpinfo = songinfo[mpds.songpos];
				status["CurrentTrackMetaData"] = tmpinfo.c_str();
			}
			else {
				status["CurrentTrackMetaData"] = "";
			}
		}
		else {
			status["CurrentTrackMetaData"] = "";
		}
	} else {
		status["CurrentTrackMetaData"] = "";
	}
	status["CurrentTrackURI"] = (is_song&&(!uri.empty()))?uri:"";
	if(is_song) {
		unsigned int linkip = get_linkip();
		if(m_queueID.empty()) {
			if((mpds.dlna_pause) || ((mpds.linkip != linkip) && (mpds.linkip != 0))) {
				status["AVTransportURI"] = FAKEURI;
			}
			else {
				status["AVTransportURI"] = uri;
			}
		}
		else {
			if((mpds.dlna_pause) || ((mpds.linkip != linkip) && (mpds.linkip != 0))) {
				status["AVTransportURI"] = FAKEURI;
			}
			else {
				status["AVTransportURI"] = m_queueID;
			}
		}
	}
	else
		status["AVTransportURI"] = "";
	status["AVTransportURIMetaData"] = "";
	status["NextAVTransportURI"] = "";
	status["NextAVTransportURIMetaData"] = "";
	status["RelativeTimePosition"] = is_song?upnpduration(mpds.songelapsedms):"00:00:00";
	status["AbsoluteTimePosition"] = "NOT_IMPLEMENTED";
	status["RelativeCounterPosition"] = "2147483647";
	status["AbsoluteCounterPosition"] = "2147483647";
	status["CurrentTransportActions"] = tactions;
	
	return true;
}

#endif
