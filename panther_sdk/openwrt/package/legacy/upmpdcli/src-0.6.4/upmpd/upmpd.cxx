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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>

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
#include <mon.h>
using namespace std;
using namespace std::placeholders;

#include <upmpdcli/upnpplib.hxx>
#include <upmpdcli/soaphelp.hxx>
#include <upmpdcli/device.hxx>
#include <upmpdcli/log.hxx>

#include <dlna/mpdcli.hxx>
#include <dlna/upmpdutils.hxx>
#include <dlna/md5.hxx>
#include <dlna/upmpd.hxx>
#include <dlna/jsonParse.h>

static const string dfltFriendlyName("UpMpd");
static string lasturi;
static bool m_mute = false;
static int XZXPlayMode =0;
static int XZXPlayModeStatus =0;
static string XZXPlayModeSting="WIFI";
static string setMulActionString="NORMAL";
static string mulitroomStatusString="NORMAL";

static const string serviceIdRender("urn:upnp-org:serviceId:RenderingControl");
static const string serviceIdTransport("urn:upnp-org:serviceId:AVTransport");
static const string serviceIdCM("urn:upnp-org:serviceId:ConnectionManager");
static const string serviceIdQPlay("urn:tencent-com:serviceId:QPlay");

void dlna_log(const char *format, ...)
{       
	char buf[512];
	va_list args;
	
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	
	openlog("DLNA", 0, 0);
	syslog(LOG_USER | LOG_NOTICE, "%s", buf);
	closelog();
	
	return;
}

// Note: if we ever need this to work without cxx11, there is this:
// http://www.tutok.sk/fastgl/callback.html
UpMpd::UpMpd(const string& deviceid, 
			 const unordered_map<string, string>& xmlfiles,
			 MPDCli *mpdcli, Options opts)
	: UpnpDevice(deviceid, xmlfiles), m_mpdcli(mpdcli), m_desiredvolume(-1), m_options(opts) ,m_seektrack(-1) ,elapstime(0),
		update(0)
{
	addServiceType(serviceIdRender,
				   "urn:schemas-upnp-org:service:RenderingControl:1");
	addActionMapping("SetMute", bind(&UpMpd::setMute, this, _1, _2), NONE);
	addActionMapping("GetMute", bind(&UpMpd::getMute, this, _1, _2), NONE);
	addActionMapping("SetVolume", bind(&UpMpd::setVolume, this, _1, _2, false), NONE);
	addActionMapping("GetVolume", bind(&UpMpd::getVolume, this, _1, _2, false), NONE);
	addActionMapping("ListPresets", bind(&UpMpd::listPresets, this, _1, _2), NONE);
	addActionMapping("SelectPreset", bind(&UpMpd::selectPreset, this, _1, _2), NONE);
#if 0
	addActionMapping("GetVolumeDB", 
					 bind(&UpMpd::getVolume, this, _1, _2, true));
	addActionMapping("SetVolumeDB", 
					 bind(&UpMpd::setVolume, this, _1, _2, true));
	addActionMapping("GetVolumeDBRange", 
					 bind(&UpMpd::getVolumeDBRange, this, _1, _2));
#endif

	addServiceType(serviceIdTransport,
				   "urn:schemas-upnp-org:service:AVTransport:1");
	addActionMapping("SetAVTransportURI", 
					 bind(&UpMpd::setAVTransportURI, this, _1, _2, false), SETUP);
	addActionMapping("SetNextAVTransportURI", 
					 bind(&UpMpd::setAVTransportURI, this, _1, _2, true), NONE);
	addActionMapping("GetPositionInfo", 
					 bind(&UpMpd::getPositionInfo, this, _1, _2), SKIP);
	addActionMapping("GetTransportInfo", 
					 bind(&UpMpd::getTransportInfo, this, _1, _2), NONE);
	addActionMapping("GetXZXPlayMode", 
					 bind(&UpMpd::getXZXPlayMode, this, _1, _2), NONE);
	addActionMapping("GetMediaInfo", 
					 bind(&UpMpd::getMediaInfo, this, _1, _2), SKIP);
	addActionMapping("GetDeviceCapabilities", 
					 bind(&UpMpd::getDeviceCapabilities, this, _1, _2), NONE);
	addActionMapping("SetPlayMode", bind(&UpMpd::setPlayMode, this, _1, _2), NONE);
	addActionMapping("GetTransportSettings", 
					 bind(&UpMpd::getTransportSettings, this, _1, _2), NONE);
	addActionMapping("GetCurrentTransportActions", 
					 bind(&UpMpd::getCurrentTransportActions, this, _1, _2), NONE);
	addActionMapping("Stop", bind(&UpMpd::playcontrol, this, _1, _2, 0), NONE);
	addActionMapping("Play", bind(&UpMpd::playcontrol, this, _1, _2, 1), NONE);
	addActionMapping("Pause", bind(&UpMpd::playcontrol, this, _1, _2, 2), NONE);
	addActionMapping("Seek", bind(&UpMpd::seek, this, _1, _2), NONE);
	addActionMapping("Next", bind(&UpMpd::seqcontrol, this, _1, _2, 0), NONE);
	addActionMapping("Previous", bind(&UpMpd::seqcontrol, this, _1, _2, 1), NONE);

	addServiceType(serviceIdCM,
				   "urn:schemas-upnp-org:service:ConnectionManager:1");
	addActionMapping("GetCurrentConnectionIDs", 
					 bind(&UpMpd::getCurrentConnectionIDs, this, _1, _2), NONE);
	addActionMapping("GetCurrentConnectionInfo", 
					 bind(&UpMpd::getCurrentConnectionInfo, this, _1, _2), NONE);
	addActionMapping("GetProtocolInfo", 
					 bind(&UpMpd::getProtocolInfo, this, _1, _2), SKIP);

// ThirdParty service
	addServiceType(serviceIdQPlay,
				   "urn:schemas-tencent-com:service:QPlay:2");
	addActionMapping("GetTracksInfo", 
					 bind(&UpMpd::getTracksInfo, this, _1, _2), SKIP);
	addActionMapping("SetTracksInfo", 
					 bind(&UpMpd::setTracksInfo, this, _1, _2), NONE);
	addActionMapping("QPlayAuth", 
					 bind(&UpMpd::QPlayAuth, this, _1, _2), SKIP);
//	addActionMapping("RemoveTracks", 
//					 bind(&UpMpd::removeTrack, this, _1, _2), NONE);
	addActionMapping("SetNetwork", 
					 bind(&UpMpd::setNetWork, this, _1, _2), SKIP);
	addActionMapping("GetMaxTracks", 
					 bind(&UpMpd::getMaxTracks, this, _1, _2), SKIP);
}

// This is called by the polling loop at regular intervals, or when
// triggered, to retrieve changed state variables for each of the
// services (the list of services was defined in the base class by the
// "addServiceTypes()" calls during construction).
//
// We might add a method for triggering an event from the action
// methods after changing state, which would really act only if the
// interval with the previous event is long enough. But things seem to
// work ok with the systematic delay.
bool UpMpd::getEventData(bool all, const string& serviceid, 
						 std::vector<std::string>& names, 
						 std::vector<std::string>& values)
{
	if (!serviceid.compare(serviceIdRender)) {
		return getEventDataRendering(all, names, values);
	} else if (!serviceid.compare(serviceIdTransport)) {
		return getEventDataTransport(all, names, values);
	} else if (!serviceid.compare(serviceIdCM)) {
		return getEventDataCM(all, names, values);
	} else if (!serviceid.compare(serviceIdQPlay)) {
		return getEventDataQPlay(all, names, values);
	} else {
		LOGERR("UpMpd::getEventData: servid? [" << serviceid << "]" << endl);
		return UPNP_E_INVALID_PARAM;
	}
}

////////////////////////////////////////////////////
/// RenderingControl methods

// State variables for the RenderingControl. All evented through LastChange
//  PresetNameList
//  Mute
//  Volume
//  VolumeDB
// LastChange contains all the variables that were changed since the last
// event. For us that's at most Mute, Volume, VolumeDB
// <Event xmlns=”urn:schemas-upnp-org:metadata-1-0/AVT_RCS">
//   <InstanceID val=”0”>
//     <Mute channel=”Master” val=”0”/>
//     <Volume channel=”Master” val=”24”/>
//     <VolumeDB channel=”Master” val=”24”/>
//   </InstanceID>
// </Event>

bool UpMpd::rdstateMToU(unordered_map<string, string>& status)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();

	int volume = m_desiredvolume >= 0 ? m_desiredvolume : mpds.volume;
	if (volume < 0)
		volume = 0;
	char cvalue[30];

//	sprintf(cvalue, "%d", percentodbvalue(volume));
//	status["VolumeDB"] =  cvalue;

	if(m_mute) {
		sprintf(cvalue, "%d", m_premutevolume);
		status["Mute channel=\"Master\""] = "1";
		if(m_premutevolume == 0)
			status["Volume channel=\"Master\""] = "1";
		else
			status["Volume channel=\"Master\""] = cvalue;
	}
	else {
		sprintf(cvalue, "%d", volume);
		status["Mute channel=\"Master\""] = "0";
		status["Volume channel=\"Master\""] = cvalue;
	}

	return true;
}

bool UpMpd::getEventDataRendering(bool all, std::vector<std::string>& names, 
								  std::vector<std::string>& values)
{
	//LOGDEB("UpMpd::getEventDataRendering. desiredvolume " << 
	//		   m_desiredvolume << (all?" all " : "") << endl);
	if (m_desiredvolume >= 0) {
		m_mpdcli->setVolume(m_desiredvolume);
		m_desiredvolume = -1;
	}

	unordered_map<string, string> newstate;
	rdstateMToU(newstate);

	if (all)
		m_rdstate.clear();

	string 
		chgdata("<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT_RCS\">\n"
				"<InstanceID val=\"0\">\n");

	bool changefound = false;
	for (unordered_map<string, string>::const_iterator it = newstate.begin();
		 it != newstate.end(); it++) {

		const string& oldvalue = mapget(m_rdstate, it->first);
		if (!it->second.compare(oldvalue))
			continue;

		changefound = true;

		//LOGDEB("name: " << it->first << " val: " << it->second << endl);
		chgdata += "<";
		chgdata += it->first;
		chgdata += " val=\"";
		chgdata += xmlquote(it->second);
		chgdata += "\"/>\n";
	}
	chgdata += "</InstanceID>\n</Event>\n";

	if (!changefound) {
		return true;
	}

	names.push_back("LastChange");
	values.push_back(chgdata);

	m_rdstate = newstate;
//	DEBOUT << "UpMpd::getEventDataRendering: " << chgdata << endl;

	return true;
}

// Actions:
// Note: we need to return all out arguments defined by the SOAP call even if
// they don't make sense (because there is no song playing). Ref upnp arch p.51:
//
//   argumentName: Required if and only if action has out
//   arguments. Value returned from action. Repeat once for each out
//   argument. If action has an argument marked as retval, this
//   argument must be the first element. (Element name not qualified
//   by a namespace; element nesting context is sufficient.) Case
//   sensitive. Single data type as defined by UPnP service
//   description. Every “out” argument in the definition of the action
//   in the service description must be included, in the same order as
//   specified in the service description (SCPD) available from the
//   device.

#if 0
int UpMpd::getVolumeDBRange(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;

	it = sc.args.find("Channel");
	if (it == sc.args.end() || it->second.compare("Master")) {
		return UPNP_E_INVALID_PARAM;
	}
	data.addarg("MinValue", "-10240");
	data.addarg("MaxValue", "0");

	return UPNP_E_SUCCESS;
}
#endif
int UpMpd::setMute(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;

	it = sc.args.find("Channel");
	if (it == sc.args.end() || it->second.compare("Master")) {
		return UPNP_E_INVALID_PARAM;
	}
		
	it = sc.args.find("DesiredMute");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	if (it->second[0] == 'F' || it->second[0] == '0') {
		// Restore pre-mute
		m_mpdcli->setVolume(1, true);
		m_mute = false;
	} else if (it->second[0] == 'T' || it->second[0] == '1') {
		if (m_desiredvolume >= 0) {
			m_mpdcli->setVolume(m_desiredvolume);
			m_desiredvolume = -1;
		}
		m_mpdcli->setVolume(0, true);
		m_mute = true;
	} else {
		return UPNP_E_INVALID_PARAM;
	}
	loopWakeup();
	return UPNP_E_SUCCESS;
}

int UpMpd::getMute(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;

	it = sc.args.find("Channel");
	if (it == sc.args.end() || it->second.compare("Master")) {
		return UPNP_E_INVALID_PARAM;
	}
	int volume = m_mpdcli->getVolume();
	data.addarg("CurrentMute", volume == 0 ? "1" : "0");
	return UPNP_E_SUCCESS;
}

int UpMpd::setVolume(const SoapArgs& sc, SoapData& data, bool isDb)
{
	map<string, string>::const_iterator it;

	it = sc.args.find("Channel");
	if (it == sc.args.end() || it->second.compare("Master")) {
		return UPNP_E_INVALID_PARAM;
	}
		
	it = sc.args.find("DesiredVolume");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	int volume = atoi(it->second.c_str());
	if (isDb) {
		volume = dbvaluetopercent(volume);
	} 
	if (volume < 0 || volume > 100) {
		return UPNP_E_INVALID_PARAM;
	}
	
	m_mute = false;
	
	int previous_volume = m_mpdcli->getVolume();
	int delta = previous_volume - volume;
	if (delta < 0)
		delta = -delta;
	dlna_log("volume:%d delta:%d previous_vol:%d\n",volume,delta,previous_volume);

	m_premutevolume = volume;
	if (delta >= 5) {
		m_mpdcli->setVolume(volume);
		m_desiredvolume = -1;
	} else {
		m_desiredvolume = volume;
	}

	loopWakeup();
	return UPNP_E_SUCCESS;
}

int UpMpd::getVolume(const SoapArgs& sc, SoapData& data, bool isDb)
{
	// LOGDEB("UpMpd::getVolume" << endl);
	map<string, string>::const_iterator it;

	it = sc.args.find("Channel");
	if (it == sc.args.end() || it->second.compare("Master")) {
		return UPNP_E_INVALID_PARAM;
	}
		
	int volume = m_mpdcli->getVolume();
	if (isDb) {
		volume = percentodbvalue(volume);
	}
	char svolume[30];
	sprintf(svolume, "%d", volume);
	data.addarg("CurrentVolume", svolume);
	return UPNP_E_SUCCESS;
}

int UpMpd::listPresets(const SoapArgs& sc, SoapData& data)
{
	// The 2nd arg is a comma-separated list of preset names
	data.addarg("CurrentPresetNameList", "FactoryDefaults");
	return UPNP_E_SUCCESS;
}

int UpMpd::selectPreset(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;
		
	it = sc.args.find("PresetName");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	if (it->second.compare("FactoryDefaults")) {
		return UPNP_E_INVALID_PARAM;
	}

	// Well there is only the volume actually...
	int volume = 50;
	m_mpdcli->setVolume(volume);

	return UPNP_E_SUCCESS;
}

///////////////// AVTransport methods
// AVTransport eventing
// 
// Some state variables do not generate events and must be polled by
// the control point:
// RelativeTimePosition AbsoluteTimePosition
// RelativeCounterPosition AbsoluteCounterPosition.
// This leaves us with:
//    TransportState
//    TransportStatus
//    PlaybackStorageMedium
//    PossiblePlaybackStorageMedia
//    RecordStorageMedium
//    PossibleRecordStorageMedia
//    CurrentPlayMode
//    TransportPlaySpeed
//    RecordMediumWriteStatus
//    CurrentRecordQualityMode
//    PossibleRecordQualityModes
//    NumberOfTracks
//    CurrentTrack
//    CurrentTrackDuration
//    CurrentMediaDuration
//    CurrentTrackMetaData
//    CurrentTrackURI
//    AVTransportURI
//    AVTransportURIMetaData
//    NextAVTransportURI
//    NextAVTransportURIMetaData
//    RelativeTimePosition
//    AbsoluteTimePosition
//    RelativeCounterPosition
//    AbsoluteCounterPosition
//    CurrentTransportActions
//
// To be all bundled inside:    LastChange
// Translate MPD state to UPnP AVTRansport state variables
bool UpMpd::tpstateMToU(unordered_map<string, string>& status)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
//	DEBOUT << "UpMpd::tpstateMToU: curpos: " << mpds.songpos <<
//	   " qlen " << mpds.qlen << endl;
	bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
		(mpds.state == MpdStatus::MPDS_PAUSE);

#ifdef DLNA_MPD_RECOVER
	if(is_song) {
		if(!mpds.title) {
			if((int)songtitle.size() > mpds.songpos) {
				string title = songtitle[mpds.songpos];
				m_mpdcli->upname(title.c_str());
			}
		}
	}
#endif

	const string& uri = mapget(mpds.currentsong, "uri");

	/*For Encode*/
	status["TransportStatus"] = m_mpdcli->ok() ? "OK" : "ERROR_OCCURRED";
//	status["NextAVTransportURI"] = mapget(mpds.nextsong, "uri");
	if (is_song) {
		if((int)songinfo.size() > mpds.songpos + 1) {
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
			if((int)songinfo.size() > mpds.songpos) {
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
		if((mpds.dlna_pause) || ((mpds.linkip != linkip) && (mpds.linkip != 0))) {
			status["AVTransportURI"] = FAKEURI;
		}
		else {
			status["AVTransportURI"] = uri;
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

bool UpMpd::getEventDataTransport(bool all, std::vector<std::string>& names, 
								  std::vector<std::string>& values)
{
	unordered_map<string, string> newtpstate;
	bool changefound = false;
	
	if(m_queueID.empty())
		tpstateMToU(newtpstate);
	else
		tpstateMToU_dlnalib(newtpstate);
	
	if (all && update) {
	    m_tpstate.clear();
	    update--;
	}
	else
	    dlna_log("!!! all:%d update:%d\n",all,update);


	/*For encode*/
	string
		chgdata("<?xml version=\"1.0\"?>\n"
				"<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT/\">\n"
				"<InstanceID val=\"0\">\n");
	for (unordered_map<string, string>::const_iterator it = newtpstate.begin();
		 it != newtpstate.end(); it++) {

		const string& oldvalue = mapget(m_tpstate, it->first);
		if (!it->second.compare(oldvalue)) {
#ifndef AVOID_TWINKLING
			if(!it->first.compare("TransportState"))
				;
			else
#endif					
				continue;
		}

		if (it->first.compare("RelativeTimePosition") && 
			it->first.compare("AbsoluteTimePosition"))
		{
		//	DEBOUT << "Transport state update for " << it->first << 
		//	 " oldvalue [" << oldvalue << "] -> [" << it->second << endl;
			dlna_log("===== changed: %s\n",it->first.c_str());
			changefound = true;
		}
		
		/*Fix encode error*/
		chgdata += "<";
		chgdata += it->first;
		chgdata += " val=\"";
		chgdata += xmlquote(it->second);
		chgdata += "\">";
		chgdata += "<";
		chgdata += "/";
		chgdata += it->first;
		chgdata += ">\n";
	}
	chgdata += "</InstanceID>\n</Event>\n";

	if (!changefound) {
		return true;
	}
	else
		dlna_log("[Event] Send to %x\n",get_linkip());

	names.push_back("LastChange");
	values.push_back(chgdata);

	m_tpstate = newtpstate;
	DEBOUT << "UpMpd::getEventDataTransport: " << chgdata << endl;
	return true;
}

// http://192.168.4.4:8200/MediaItems/246.mp3
int UpMpd::setAVTransportURI(const SoapArgs& sc, SoapData& data, bool setnext)
{
	map<string, string>::const_iterator it;

	it = setnext? sc.args.find("NextURI") : sc.args.find("CurrentURI");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}

	m_mpdcli->dlna_pause(false);
	m_mpdcli->setip(get_linkip());
/*
#ifdef MONTAGE_CTRL_PLAYER
	inform_ctrl_msg(MONDLNA, NULL);
#endif
*/
	string uri = it->second;
	lasturi = uri;

	if(uri.compare(0,5,"qplay") == 0) {
		m_queueID = uri;
		m_mpdcli->device_owner(1);
		loopWakeup();
		return UPNP_E_SUCCESS;
	} else {
		/*Reset repeat, random, single mode*/
		m_mpdcli->repeat(false);
		m_mpdcli->random(false);
		m_mpdcli->single(false);
		m_queueID.clear();
		m_mpdcli->device_owner(0);
	}

	string metadata;
	it = setnext? sc.args.find("NextURIMetaData") : 
		sc.args.find("CurrentURIMetaData");
	if (it != sc.args.end())
		metadata = it->second;
	//cerr << "SetTransport: setnext " << setnext << " metadata[" << metadata <<
	// "]" << endl;

	if ((m_options & upmpdOwnQueue) && !setnext) {
		// If we own the queue, just clear it before setting the
		// track.  Else it's difficult to impossible to prevent it
		// from growing if upmpdcli restarts. If the option is not set, the
		// user prefers to live with the issue.
		m_mpdcli->clearQueue();
		/*Clear songinfo*/
		songinfo.clear();
#ifdef DLNA_MPD_RECOVER
		songtitle.clear();
#endif
	}

	const MpdStatus &mpds = m_mpdcli->getStatus();
	bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
		(mpds.state == MpdStatus::MPDS_PAUSE);
	int curpos = mpds.songpos;
	LOGDEB("UpMpd::set" << (setnext?"Next":"") << 
		   "AVTransportURI: curpos: " <<
		   curpos << " is_song " << is_song << " qlen " << mpds.qlen << endl);

	// curpos == -1 means that the playlist was cleared or we just started. A
	// play will use position 0, so it's actually equivalent to curpos == 0
	if (curpos == -1) {
		curpos = 0;
	}

	if (mpds.qlen == 0 && setnext) {
		LOGDEB("setNextAVTRansportURI invoked but empty queue!" << endl);
		return UPNP_E_INVALID_PARAM;
	}
	
	std::size_t start = metadata.find("<dc:title>");
	std::size_t end = metadata.rfind("</dc:title>");
	std::string title;
	title.clear();
	if ((start != std::string::npos) && (end != std::string::npos))
		title = metadata.substr(start+10,end-start-10);

	int songid;
	if ((songid = m_mpdcli->insert(uri, title.empty()?NULL:title.c_str(), setnext?curpos+1:curpos)) < 0) {
		return UPNP_E_INTERNAL_ERROR;
	}
	songinfo.push_back(metadata);
#ifdef DLNA_MPD_RECOVER
	songtitle.push_back(title);
#endif
	m_mpdcli->curlistdelsave();
#if 0
	metadata = regsub1("<\\?xml.*\\?>", metadata, "");
	if (setnext) {
		m_nextUri = uri;
		//m_nextMetadata = metadata;
	} else {
		m_curMetadata = metadata;
		m_nextUri = "";
		//m_nextMetadata = "";
	}
#endif
	if (!setnext) {
		MpdStatus::State st = mpds.state;
		// Have to tell mpd which track to play, else it will keep on
		// the previous despite of the insertion. The UPnP docs say
		// that setAVTransportURI should not change the transport
		// state (pause/stop stay pause/stop) but it seems that some clients
		// expect that the track will start playing.
		// Needs to be revisited after seeing more clients. For now try to 
		// preserve state as per standard.
		// Audionet: issues a Play
		// BubbleUpnp: issues a Play
		// MediaHouse: no setnext, Play
		m_mpdcli->play(curpos);
#if 1 || defined(upmpd_do_restore_play_state_after_add)
		switch (st) {
		case MpdStatus::MPDS_PAUSE: m_mpdcli->togglePause(); break;
		case MpdStatus::MPDS_STOP: m_mpdcli->stop(); break;
		default: break;
		}
#endif
		// Clean up old song ids
		if (!(m_options & upmpdOwnQueue)) {
			for (set<int>::iterator it = m_songids.begin();
				 it != m_songids.end(); it++) {
				// Can't just delete here. If the id does not exist, MPD 
				// gets into an apparently permanent error state, where even 
				// get_status does not work
				if (m_mpdcli->statId(*it)) {
					m_mpdcli->deleteId(*it);
				}
			}
			m_songids.clear();
		}
	}

	if (!(m_options & upmpdOwnQueue)) {
		m_songids.insert(songid);
	}

	loopWakeup();
	return UPNP_E_SUCCESS;
}

#define KUGOU_SPEC_PATCH
int UpMpd::getPositionInfo(const SoapArgs& sc, SoapData& data)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	//LOGDEB("UpMpd::getPositionInfo. State: " << mpds.state << endl);

	bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
		(mpds.state == MpdStatus::MPDS_PAUSE);

	if (is_song) {
		char nos[8];
		sprintf(nos, "%d", mpds.songpos + 1);
		data.addarg("Track", nos);
	} else
		data.addarg("Track", "0");

	const string& uri = mapget(mpds.currentsong, "uri");
#if 0
	data.addarg("TrackDuration",is_song?upnpduration(mpds.songlenms):"00:00:00");
	data.addarg("TrackURI",(is_song && !uri.empty())?xmlquote(uri):"");
	data.addarg("RelTime", is_song?upnpduration(mpds.songelapsedms):"00:00:00");
#else
	/*For Kugou*/
	//data.addarg("TrackDuration",upnpduration(mpds.songlenms));
	//data.addarg("TrackURI",(is_song && !uri.empty())?uri:lasturi);
	//data.addarg("RelTime", is_song?upnpduration(mpds.songelapsedms):upnpduration(mpds.songlenms));
#endif

	if (is_song) {
	
		const string& uri  = mapget(mpds.currentsong, "uri");
		
//		jsonParse mjsonParse; 
//		WARTI_MusicPtr musicInfo = mjsonParse.getElementMusic(uri);
               WARTI_MusicPtr musicInfo = jsonParse::instance().getElementMusic(uri);
//	WARTI_MusicPtr musicInfo upmpd_getElementMusic(uri);
			LOGDEB("quejw musicInfo current_url =" << musicInfo->pContentURL << endl);
	
			if((musicInfo->pContentURL).length() != 0){
				

			data.addarg("TrackMetaData", http_didlmake(mpds,musicInfo));
				LOGDEB("quejw http_didlmake current_url musicInfo =" << musicInfo->pContentURL << endl);
				LOGDEB("quejw http_didlmake pTitle musicInfo =" << musicInfo->pTitle << endl);
				}
	
		else if(!songinfo.empty()) {
			if((int)songinfo.size() > mpds.songpos) {
				string tmpinfo = songinfo[mpds.songpos];
				data.addarg("TrackMetaData", tmpinfo);
			}
			else
				data.addarg("TrackMetaData", "");
		}
		else
			data.addarg("TrackMetaData", "");
	} else {
		data.addarg("TrackMetaData", "");
	}
	
	/*For Media player position and Kugou*/
	unsigned int linkip = get_linkip();
	if((mpds.dlna_pause) || (((mpds.linkip != linkip) || (mpds.linkip != data.pktip)) && (mpds.linkip != 0))) {
//		dlna_log("[getPositionInfo] Send FAKEURI: to:%x mpdip:%x\n",data.pktip, mpds.linkip);
		data.addarg("TrackURI",FAKEURI);
	}
	else {
		data.addarg("TrackURI",(is_song && !uri.empty())?uri:lasturi);
//		dlna_log("[getPositionInfo] Send URI: to:%x mpdip:%x\n",linkip, mpds.linkip);
	}

	string reltime;
#ifdef KUGOU_SPEC_PATCH
	static string duration;

	if (is_song)
		duration = upnpduration(mpds.songlenms);
	data.addarg("TrackDuration", duration);

	//dlna_log("==== len:%s el:%s\n", upnpduration(mpds.songlenms).c_str(), upnpduration(mpds.songelapsedms).c_str());

	if(m_queueID.empty())
		reltime = is_song?upnpduration(mpds.songelapsedms):duration;
#else
	data.addarg("TrackDuration",upnpduration(mpds.songlenms));

	if(m_queueID.empty())
		reltime = is_song?upnpduration(mpds.songelapsedms):upnpduration(mpds.songlenms);
#endif
	else
		reltime = is_song?upnpduration(mpds.songelapsedms):"00:00:00";
	data.addarg("RelTime", reltime);
	data.addarg("AbsTime", is_song?"NOT_IMPLEMENTED":"END_OF_MEDIA");
	
	dlna_log("==== track:%d reltime:%s mpds.state:%d songinfo:%d elapstime:%d\n",
								mpds.songpos + 1, reltime.c_str(),mpds.state,songinfo.size(),elapstime);
	/*Follow DLNA Spec*/
	data.addarg("RelCount", "2147483647");
	data.addarg("AbsCount", "2147483647");
	return UPNP_E_SUCCESS;
}

int UpMpd::getTransportInfo(const SoapArgs& sc, SoapData& data)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	//LOGDEB("UpMpd::getTransportInfo. State: " << mpds.state << endl);
	
	string tstate("STOPPED");
//	unsigned int linkip = get_linkip();
	
//	if((mpds.dlna_pause) || (((mpds.linkip != linkip) || (mpds.linkip != data.pktip)) && (mpds.linkip != 0))) {
//		//tstate = "PAUSED_PLAYBACK";
//        return UPNP_E_INVALID_PARAM;
//	}
//	else {
		switch(mpds.state) {
			case MpdStatus::MPDS_PLAY: tstate = "PLAYING"; break;
			case MpdStatus::MPDS_PAUSE: tstate = "PAUSED_PLAYBACK"; break;
			default: break;
//		}
	}
	data.addarg("CurrentTransportState", tstate);
	data.addarg("CurrentTransportStatus", m_mpdcli->ok() ? "OK" : 
				"ERROR_OCCURRED");
	data.addarg("CurrentSpeed", "1");
	return UPNP_E_SUCCESS;
}

int UpMpd::getXZXPlayMode(const SoapArgs& sc, SoapData& data)
{
	LOGDEB("UpMpd::getXZXPlayMode. State: WIFI"<< XZXPlayMode << endl);
	string playmode(XZXPlayModeSting);
	string mulitroomStatus(mulitroomStatusString);
	string mulitroomAction(setMulActionString);
	switch(XZXPlayMode) {
	case 0: playmode = XZXPlayModeSting; break;
	case 1: playmode = XZXPlayModeSting; break;
	case 2: playmode = XZXPlayModeSting; break;
	//case 3: mulitroomStatus = mulitroomStatusString; break;
	//case 4: mulitroomStatus = mulitroomStatusString; break;
	//case 5: mulitroomStatus = mulitroomStatusString; break;
	case 6: mulitroomAction = setMulActionString; break;
	case 7: mulitroomAction = setMulActionString; break;
	case 8: mulitroomAction = setMulActionString; break;
	default: break;
	}
	LOGDEB("normal getXZXPlayMode===="<<setMulActionString<< endl);
	LOGDEB("normal XZXPlayModeStatus===="<<XZXPlayModeStatus<< endl);
	switch(XZXPlayModeStatus) {
	case 3: mulitroomStatus = mulitroomStatusString; break;
	case 4: mulitroomStatus = mulitroomStatusString; break;
    case 5: mulitroomStatus = mulitroomStatusString; break;
	default: break;
	}
	LOGDEB("normal mulitroomStatus===="<< mulitroomStatus << endl);

	data.addarg("CurrentAudioPlayMode", playmode);
	data.addarg("AudioMultiroomStatus", mulitroomStatus);
	data.addarg("AudioMultiroomAction", mulitroomAction);
	if(XZXPlayMode==6 || XZXPlayMode ==7){
		XZXPlayMode==8;
		setMulActionString ="NORMAL";
	}
	return UPNP_E_SUCCESS;
}


int UpMpd::getDeviceCapabilities(const SoapArgs& sc, SoapData& data)
{
	data.addarg("PlayMedia", "NETWORK,HDD");
	data.addarg("RecMedia", "NOT_IMPLEMENTED");
	data.addarg("RecQualityModes", "NOT_IMPLEMENTED");
	return UPNP_E_SUCCESS;
}

int UpMpd::getMediaInfo(const SoapArgs& sc, SoapData& data)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	//LOGDEB("UpMpd::getMediaInfo. State: " << mpds.state << endl);

	bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
		(mpds.state == MpdStatus::MPDS_PAUSE);
	const string& uri = mapget(mpds.currentsong, "uri");

	string tmpuri = m_queueID;

	if(is_song) {
		char nos[8];
		sprintf(nos, "%d", songinfo.size());
		data.addarg("NrTracks", nos);
	} else
		data.addarg("NrTracks", "0");

	if (is_song)
		data.addarg("MediaDuration", upnpduration(mpds.songlenms));
	else
		data.addarg("MediaDuration", "00:00:00");

	if(is_song) {
		unsigned int linkip = get_linkip();
		if(m_queueID.empty()) {
			const string& uri = mapget(mpds.currentsong, "uri");
			if((mpds.dlna_pause) || (((mpds.linkip != linkip) || (mpds.linkip != data.pktip)) && (mpds.linkip != 0))) {
				data.addarg("CurrentURI", FAKEURI);
				dlna_log("[getMediaInfo] Send FAKEURI to:%x\n",data.pktip);
			}
			else
				data.addarg("CurrentURI", uri);
		}
		else {
			if((mpds.dlna_pause) || ((mpds.linkip != linkip) && (mpds.linkip != 0))) {
				data.addarg("CurrentURI", FAKEURI);
				dlna_log("[getMediaInfo] Send FAKEURI to:%x\n",linkip);
			}
			else
				data.addarg("CurrentURI", tmpuri);
	}
	} else
		data.addarg("CurrentURI", "");

	if (is_song) {
	
	 LOGDEB("quejw CurrentURIMetaData ====== MPD"  << endl);
			//data.addarg("CurrentURIMetaData", didlmake(mpds));
			//CUR_WARTI_pMusic cur_musicInfo = getCurrentInfo_config();
			//data.addarg("CurrentURIMetaData", current_didlmake(mpds,cur_musicInfo));
			const string& uri  = mapget(mpds.currentsong, "uri");
            LOGDEB("quejw mapget(mpds.currentsong, uri) =" << uri << endl);
//	    	jsonParse *mjsonParse = new jsonParse(); 
//		WARTI_MusicPtr musicInfo = mjsonParse->getElementMusic(uri);
		  WARTI_MusicPtr musicInfo = jsonParse::instance().getElementMusic(uri);
	//	WARTI_MusicPtr musicInfo upmpd_getElementMusic(uri);
			LOGDEB("quejw musicInfo current_url =" << musicInfo->pContentURL << endl);
			if((musicInfo->pContentURL).length() != 0){							
			   data.addarg("CurrentURIMetaData", http_didlmake(mpds,musicInfo));
			   LOGDEB("quejw getMediaInfo current_url musicInfo =" << musicInfo->pContentURL << endl);
			   LOGDEB("quejw getMediaInfo pTitle musicInfo =" << musicInfo->pTitle << endl);
			  }
	
	
		else if(!songinfo.empty()) {
			if((int)songinfo.size() > mpds.songpos) {
				string tmpinfo = songinfo[mpds.songpos];
				data.addarg("CurrentURIMetaData", tmpinfo.c_str());
			}
			else
				data.addarg("CurrentURIMetaData", "");
		}
		else
			data.addarg("CurrentURIMetaData", "");
	}
	else
		data.addarg("CurrentURIMetaData", "");

	if (is_song) {
		data.addarg("NextURI", mapget(mpds.nextsong, "uri"));
		if((int)songinfo.size() > mpds.songpos + 1) {
			string tmpinfo = songinfo[mpds.songpos + 1];
			data.addarg("NextURIMetaData", tmpinfo.c_str());
		}
		else
			data.addarg("NextURIMetaData", "");
	}
	else {
		data.addarg("NextURI","" );
		data.addarg("NextURIMetaData", "");
	}
	
	string playmedium("NONE");
	if (is_song)
		playmedium = uri.find("http://") == 0 ?	"NONE" : "NETWORK";
	data.addarg("PlayMedium", playmedium);
	data.addarg("RecordMedium", "NOT_IMPLEMENTED");
	data.addarg("WriteStatus", "NOT_IMPLEMENTED");
	
	return UPNP_E_SUCCESS;
}

int UpMpd::playcontrol(const SoapArgs& sc, SoapData& data, int what)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	unsigned int playidx = 0;
	
	if(!(chk_valid(mpds.linkip))) {
		dlna_log("Drop Action [Stop/Pause/Play] from:%x\n",get_linkip());
		return UPNP_E_INVALID_PARAM;
	}

	LOGDEB("UpMpd::playcontrol State: " << mpds.state <<" what "<<what << " m_seektrack " << m_seektrack << endl);

	if(((mpds.state - 1) == what) && (m_seektrack == -1))
		return UPNP_E_SUCCESS;

	if ((what & ~0x3)) {
		LOGERR("UpMPd::playcontrol: bad control " << what << endl);
		return UPNP_E_INVALID_PARAM;
	}
	
	if(m_seektrack > 0) {
		playidx = m_seektrack-1;
		m_seektrack = -1; 
	}

	bool ok = true;
	switch (mpds.state) {
	case MpdStatus::MPDS_PLAY: 
		switch (what) {
		case 0:	ok = m_mpdcli->stop(); elapstime = 0; break;
		case 1:
			ok = m_mpdcli->play(playidx);
			break;
		case 2: ok = m_mpdcli->togglePause();break;
		}
		break;
	case MpdStatus::MPDS_PAUSE:
		switch (what) {
		case 0:	ok = m_mpdcli->stop(); elapstime = 0 ;break;
		case 1: ok = m_mpdcli->togglePause();break;
		case 2: break;
		}
		break;
	case MpdStatus::MPDS_STOP:
	default:
		switch (what) {
		case 0: break;
		case 1: ok = m_mpdcli->play(playidx);break;
		case 2: break;
		}
		break;
	}
#ifdef AVOID_TWINKLING
	if(!m_queueID.empty())
		update = 1;
#endif
	loopWakeup();
	return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int UpMpd::seqcontrol(const SoapArgs& sc, SoapData& data, int what)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	LOGDEB("UpMpd::seqcontrol State: " << mpds.state << " what "<<what<< endl);

	if ((what & ~0x1)) {
		LOGERR("UpMPd::seqcontrol: bad control " << what << endl);
		return UPNP_E_INVALID_PARAM;
	}

	bool ok = true;
	switch (what) {
	case 0: ok = m_mpdcli->next();break;
	case 1: ok = m_mpdcli->previous();break;
	}

	loopWakeup();
	return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}
	
int UpMpd::setPlayMode(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;
		
	it = sc.args.find("NewPlayMode");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	string playmode(it->second);
	bool ok;
	if (!playmode.compare("NORMAL")) {
		ok = m_mpdcli->repeat(false) && m_mpdcli->random(false) &&
			m_mpdcli->single(false);
	} else if (!playmode.compare("SHUFFLE")) {
		ok = m_mpdcli->repeat(false) && m_mpdcli->random(true) &&
			m_mpdcli->single(false);
	} else if (!playmode.compare("REPEAT_ONE")) {
		ok = m_mpdcli->repeat(true) && m_mpdcli->random(false) &&
			m_mpdcli->single(true);
	} else if (!playmode.compare("REPEAT_ALL")) {
		ok = m_mpdcli->repeat(true) && m_mpdcli->random(false) &&
			m_mpdcli->single(false);
	} else if (!playmode.compare("RANDOM")) {
		ok = m_mpdcli->repeat(true) && m_mpdcli->random(true) &&
			m_mpdcli->single(false);
	} else if (!playmode.compare("DIRECT_1")) {
		ok = m_mpdcli->repeat(false) && m_mpdcli->random(false) &&
			m_mpdcli->single(true);
	} else if (!playmode.compare("REPEAT_TRACK")) {
		ok = m_mpdcli->repeat(true) && m_mpdcli->random(false) &&
			m_mpdcli->single(true);
	} else {
		return UPNP_E_INVALID_PARAM;
	}
	loopWakeup();
	return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int UpMpd::getTransportSettings(const SoapArgs& sc, SoapData& data)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	string playmode = mpdsToPlaymode(mpds);
	data.addarg("PlayMode", playmode);
	data.addarg("RecQualityMode", "NOT_IMPLEMENTED");
	return UPNP_E_SUCCESS;
}

int UpMpd::getCurrentTransportActions(const SoapArgs& sc, SoapData& data)
{
	const MpdStatus &mpds = m_mpdcli->getStatus();
	string tactions("Next,Previous");
	switch(mpds.state) {
	case MpdStatus::MPDS_PLAY: 
		tactions += ",Pause,Stop,Seek";
		break;
	case MpdStatus::MPDS_PAUSE: 
		tactions += ",Play,Stop,Seek";
		break;
	default:
		tactions += ",Play";
	}
	data.addarg("CurrentTransportActions", tactions);
	return UPNP_E_SUCCESS;
}

int UpMpd::seek(const SoapArgs& sc, SoapData& data)
{
	map<string, string>::const_iterator it;

	it = sc.args.find("Unit");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	string unit(it->second);

	it = sc.args.find("Target");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	string target(it->second);

	//LOGDEB("UpMpd::seek: unit " << unit << " target " << target << 
	//	   " current posisition " << mpds.songelapsedms / 1000 << 
	//	   " seconds" << endl);

	int abs_seconds;
	// Note that ABS_TIME and REL_TIME don't mean what you'd think
	// they mean.  REL_TIME means relative to the current track,
	// ABS_TIME to the whole media (ie for a multitrack tape). So
	// take both ABS and REL as absolute position in the current song
 	if (!unit.compare("REL_TIME") || !unit.compare("ABS_TIME")) {
#ifdef AVOID_TWINKLING
		if(!m_queueID.empty())
			update = 1;
#endif
		abs_seconds = upnpdurationtos(target);
	}
	else if (!unit.compare("TRACK_NR")) {
		m_seektrack =  atoi(target.c_str());
		dlna_log("==== Seek to:%d\n",m_seektrack);
		return UPNP_E_SUCCESS;
	} else {
		return UPNP_E_INVALID_PARAM;
	}
	LOGDEB("UpMpd::seek: seeking to " << abs_seconds << " seconds (" <<
		   upnpduration(abs_seconds * 1000) << ")" << endl);

	loopWakeup();
	return m_mpdcli->seek(abs_seconds) ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

///////////////// ConnectionManager methods

// "http-get:*:audio/mpeg:DLNA.ORG_PN=MP3,"
// "http-get:*:audio/L16:DLNA.ORG_PN=LPCM,"
// "http-get:*:audio/x-flac:DLNA.ORG_PN=FLAC"
static const string 
myProtocolInfo(
	"http-get:*:audio/amr:*,"
	"http-get:*:audio/ape:*,"
	"http-get:*:audio/wav:*,"
	"http-get:*:audio/wave:*,"
	"http-get:*:audio/x-wav:*,"
	"http-get:*:audio/x-aiff:*,"
	"http-get:*:audio/mpeg:*,"
	"http-get:*:audio/x-mpeg:*,"
	"http-get:*:audio/mp1:*,"
	"http-get:*:audio/aac:*,"
	"http-get:*:audio/aac-adts:*,"
	"http-get:*:audio/flac:*,"
	"http-get:*:audio/x-flac:*,"
	"http-get:*:audio/m4a:*,"
	"http-get:*:audio/mp4:*,"
	"http-get:*:audio/x-m4a:*,"
	"http-get:*:audio/vorbis:*,"
	"http-get:*:audio/ogg:*,"
	"http-get:*:audio/x-ogg:*,"
	"http-get:*:audio/x-scpls:*,"
	"http-get:*:audio/x-ms-wma:*,"
	"http-get:*:audio/x-ape:*,"
	"http-get:*:audio/L16;rate=11025;channels=1:*,"
	"http-get:*:audio/L16;rate=22050;channels=1:*,"
	"http-get:*:audio/L16;rate=44100;channels=1:*,"
	"http-get:*:audio/L16;rate=48000;channels=1:*,"
	"http-get:*:audio/L16;rate=88200;channels=1:*,"
	"http-get:*:audio/L16;rate=96000;channels=1:*,"
	"http-get:*:audio/L16;rate=176400;channels=1:*,"
	"http-get:*:audio/L16;rate=192000;channels=1:*,"
	"http-get:*:audio/L16;rate=11025;channels=2:*,"
	"http-get:*:audio/L16;rate=22050;channels=2:*,"
	"http-get:*:audio/L16;rate=44100;channels=2:*,"
	"http-get:*:audio/L16;rate=48000;channels=2:*,"
	"http-get:*:audio/L16;rate=88200;channels=2:*,"
	"http-get:*:audio/L16;rate=96000;channels=2:*,"
	"http-get:*:audio/L16;rate=176400;channels=2:*,"
	"http-get:*:audio/L16;rate=192000;channels=2:*"
	);

bool UpMpd::getEventDataCM(bool all, std::vector<std::string>& names, 
						   std::vector<std::string>& values)
{
	//LOGDEB("UpMpd:getEventDataCM" << endl);

	// Our data never changes, so if this is not an unconditional request,
	// we return nothing.
	if (all) {
		names.push_back("SinkProtocolInfo");
		values.push_back(myProtocolInfo);
	}
	return true;
}

int UpMpd::getCurrentConnectionIDs(const SoapArgs& sc, SoapData& data)
{
	LOGDEB("UpMpd:getCurrentConnectionIDs" << endl);
	data.addarg("ConnectionIDs", "0");
	return UPNP_E_SUCCESS;
}

int UpMpd::getCurrentConnectionInfo(const SoapArgs& sc, SoapData& data)
{
	LOGDEB("UpMpd:getCurrentConnectionInfo" << endl);
	map<string, string>::const_iterator it;
	it = sc.args.find("ConnectionID");
	if (it == sc.args.end() || it->second.empty()) {
		return UPNP_E_INVALID_PARAM;
	}
	if (it->second.compare("0")) {
		return UPNP_E_INVALID_PARAM;
	}

	data.addarg("RcsID", "0");
	data.addarg("AVTransportID", "0");
	data.addarg("ProtocolInfo", "");
	data.addarg("PeerConnectionManager", "");
	data.addarg("PeerConnectionID", "-1");
	data.addarg("Direction", "Input");
	data.addarg("Status", "Unknown");

	return UPNP_E_SUCCESS;
}

int UpMpd::getProtocolInfo(const SoapArgs& sc, SoapData& data)
{
	LOGDEB("UpMpd:getProtocolInfo" << endl);
	data.addarg("Source", "");
	data.addarg("Sink", myProtocolInfo);

	return UPNP_E_SUCCESS;
}

bool UpMpd::getEventDataQPlay(bool all, std::vector<std::string>& names, 
						   std::vector<std::string>& values)
{
	return true;
}

/////////////////////////////////////////////////////////////////////
// Main program

#include "conftree.hxx"

static char *thisprog;

static int op_flags;
#define OPT_MOINS 0x1
#define OPT_h	  0x2
#define OPT_p	  0x4
#define OPT_d	  0x8
#define OPT_D     0x10
#define OPT_c     0x20
#define OPT_l     0x40
#define OPT_f     0x80
#define OPT_q     0x100
#define OPT_i     0x200
#define OPT_P     0x400

static const char usage[] = 
"-c configfile \t configuration file to use\n"
"-h host    \t specify host MPD is running on\n"
"-p port     \t specify MPD port\n"
"-d logfilename\t debug messages to\n"
"-l loglevel\t  log level (0-6)\n"
"-D          \t run as a daemon\n"
"-f friendlyname\t define device displayed name\n"
"-q 0|1      \t if set, we own the mpd queue, else avoid clearing it whenever we feel like it\n"
"-i iface    \t specify network interface name to be used for UPnP\n"
"-P upport    \t specify port number to be used for UPnP"
"  \n\n"
			;
static void
Usage(void)
{
	fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
	exit(1);
}

static string myDeviceUUID;

static string datadir(DATADIR "/");
static string configdir(CONFIGDIR "/");

// Our XML description data. !Keep description.xml first!
static const char *xmlfilenames[] = {/* keep first */ "description.xml", 
	 "RenderingControl.xml", "AVTransport.xml", "ConnectionManager.xml", "QPlaySCPD.xml"};

static const int xmlfilenamescnt = sizeof(xmlfilenames) / sizeof(char *);

// Static for cleanup in sig handler.
static UpnpDevice *dev;
Pidfile *pf;

static void onsig(int)
{
	UpnpUnRegisterRootDevice(LibUPnP::getLibUPnP()->getdvh());
  UpnpUnRegisterRootDevice(LibUPnP::getLibUPnP()->getdvh());

	pf->close();
	pf->remove();
    LOGDEB("Got sig" << endl);
	exit(0);
}

static const int catchedSigs[] = {SIGINT, SIGQUIT, SIGTERM};
static void setupsigs()
{
    struct sigaction action;
    action.sa_handler = onsig;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    for (unsigned int i = 0; i < sizeof(catchedSigs) / sizeof(int); i++)
        if (signal(catchedSigs[i], SIG_IGN) != SIG_IGN) {
            if (sigaction(catchedSigs[i], &action, 0) < 0) {
                perror("Sigaction failed");
            }
        }
}



#define BUFF_LENGTH 100
#define FIFO_FILE "/tmp/UpmpdFIFO"
#define SETPLM	"setplm"
#define SETMSTA	"setmsta"
#define SETMACT	"setmact"
void *Upmpdfifo_thread(void *arg)
{
	char byReadBuf[BUFF_LENGTH] = {0};
	char *bySendBuf;
	int fd = -1;
	int iLength = 0;

	unlink(FIFO_FILE);
	if (mkfifo(FIFO_FILE, 0666) == -1)
	{
		printf("mkfifo failed\n");
		exit(-1);
	}
	
	printf("Upmpdfifo_thread\n");
	
	while(1)
	{
		if ((fd = open(FIFO_FILE, O_RDONLY)) == -1)
		{
			exit(-1);
		}
	
	   	memset(byReadBuf,0,sizeof(byReadBuf));
		iLength = read(fd, byReadBuf, sizeof(byReadBuf));
		//printf("Received length:%d, string: %s\n", iLength, byReadBuf);
		if (iLength > 0)
		{
			if (0 == strncmp(byReadBuf, SETPLM, sizeof(SETPLM) - 1))
			{
				if (strstr(&byReadBuf[sizeof(SETPLM)-1], "wifi") != NULL)
				{
					XZXPlayMode = 0;
					XZXPlayModeSting="WIFI";
				}
				else if (strstr(&byReadBuf[sizeof(SETPLM)-1], "bt") != NULL)
				{
					XZXPlayMode = 1;
					XZXPlayModeSting="BT";
				}
				else if (strstr(&byReadBuf[sizeof(SETPLM)-1], "aux") != NULL)
				{
					XZXPlayMode = 2;
				        XZXPlayModeSting="AUX";
				}
			}
			if (0 == strncmp(byReadBuf, SETMSTA, sizeof(SETMSTA) - 1))
			{
				if (strstr(&byReadBuf[sizeof(SETMSTA)-1], "master") != NULL)
				{
					XZXPlayModeStatus = 3;
					mulitroomStatusString="MASTER";
					
				}
				else if (strstr(&byReadBuf[sizeof(SETMSTA)-1], "slaver") != NULL)
				{
					XZXPlayModeStatus = 4;
					mulitroomStatusString="SLAVER";
				}
				else if (strstr(&byReadBuf[sizeof(SETMSTA)-1], "normal") != NULL)
				{
					XZXPlayModeStatus = 5;
					mulitroomStatusString="NORMAL";				       
				}
			}
				if (0 == strncmp(byReadBuf, SETMACT, sizeof(SETMACT) - 1))
			{
			        string tmp;
				 LOGDEB("normal byReadBuf===="<< byReadBuf << endl);
				if (strstr(&byReadBuf[sizeof(SETMACT)-1], "add") != NULL)
				{
					XZXPlayMode = 6;
					tmp =&byReadBuf[sizeof(SETMACT)+3];
					setMulActionString = "ADD="+tmp;
				}
				else if (strstr(&byReadBuf[sizeof(SETMACT)-1], "rmm") != NULL)
				{
					XZXPlayMode = 7;
					tmp=&byReadBuf[sizeof(SETMACT)+3];
					setMulActionString ="RM="+tmp;
				}
				else if (strstr(&byReadBuf[sizeof(SETMACT)-1], "normal") != NULL)
				{
					XZXPlayMode = 8;
					setMulActionString="NORMAL";
                    LOGDEB("normal setMulActionString===="<<setMulActionString<< endl);
	
				}
			}
		}
		close(fd);
		fd = -1;
		usleep(1000000);
	}
}

void StartUpmpdFIFO(pthread_t *thread)
{
	int ret;
	pthread_attr_t attr;
	
	pthread_attr_init(&attr);
  	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN );
	ret = pthread_create(thread, &attr, Upmpdfifo_thread, NULL);
	if (ret != 0)
	{
		perror("StartUpnpFIFO failed.\n");
	}
	pthread_attr_destroy(&attr);
}
int main(int argc, char *argv[])
{
	string mpdhost("localhost");
	int mpdport = 6600;
	string mpdpassword;
	string logfilename;
	int loglevel(upnppdebug::Logger::LLINF);
	string configfile;
	string friendlyname(dfltFriendlyName);
	bool ownqueue = true;
	string upmpdcliuser("root");
	string pidfilename("/var/run/upmpdcli.");
	string iface;
	unsigned short upport = 0;
	string upnpip;
	pthread_t thread;

	const char *cp;
	if ((cp = getenv("UPMPD_HOST")))
		mpdhost = cp;
	if ((cp = getenv("UPMPD_PORT")))
		mpdport = atoi(cp);
	if ((cp = getenv("UPMPD_FRIENDLYNAME")))
		friendlyname = atoi(cp);
	if ((cp = getenv("UPMPD_CONFIG")))
		configfile = cp;
	if ((cp = getenv("UPMPD_UPNPIFACE")))
		iface = cp;
	if ((cp = getenv("UPMPD_UPNPPORT")))
		upport = atoi(cp);

	thisprog = argv[0];
	argc--; argv++;
	while (argc > 0 && **argv == '-') {
		(*argv)++;
		if (!(**argv))
			Usage();
		while (**argv)
			switch (*(*argv)++) {
			case 'D':	op_flags |= OPT_D; break;
			case 'c':	op_flags |= OPT_c; if (argc < 2)  Usage();
				configfile = *(++argv); argc--; goto b1;
			case 'f':	op_flags |= OPT_f; if (argc < 2)  Usage();
				friendlyname = *(++argv); argc--; goto b1;
			case 'd':	op_flags |= OPT_d; if (argc < 2)  Usage();
				logfilename = *(++argv); argc--; goto b1;
			case 'h':	op_flags |= OPT_h; if (argc < 2)  Usage();
				mpdhost = *(++argv); argc--; goto b1;
			case 'l':	op_flags |= OPT_l; if (argc < 2)  Usage();
				loglevel = atoi(*(++argv)); argc--; goto b1;
			case 'p':	op_flags |= OPT_p; if (argc < 2)  Usage();
				mpdport = atoi(*(++argv)); argc--; goto b1;
			case 'q':	op_flags |= OPT_q; if (argc < 2)  Usage();
				ownqueue = atoi(*(++argv)) != 0; argc--; goto b1;
			case 'i':	op_flags |= OPT_i; if (argc < 2)  Usage();
				iface = *(++argv); argc--; goto b1;
			case 'P':	op_flags |= OPT_P; if (argc < 2)  Usage();
				upport = atoi(*(++argv)); argc--; goto b1;
			default: Usage();	break;
			}
	b1: argc--; argv++;
	}

	if (argc != 0)
		Usage();

	if (!configfile.empty()) {
		ConfSimple config(configfile.c_str(), 1, true);
		if (!config.ok()) {
			cerr << "Could not open config: " << configfile << endl;
			return 1;
		}
		string value;
		if (!(op_flags & OPT_d))
			config.get("logfilename", logfilename);
		if (!(op_flags & OPT_f))
			config.get("friendlyname", friendlyname);
		if (!(op_flags & OPT_l) && config.get("loglevel", value))
			loglevel = atoi(value.c_str());
		if (!(op_flags & OPT_h))
			config.get("mpdhost", mpdhost);
		if (!(op_flags & OPT_p) && config.get("mpdport", value)) {
			mpdport = atoi(value.c_str());
		}
		config.get("mpdpassword", mpdpassword);
		if (!(op_flags & OPT_q) && config.get("ownqueue", value)) {
			ownqueue = atoi(value.c_str()) != 0;
		}
		if (!(op_flags & OPT_i)) {
			config.get("upnpiface", iface);
			if (iface.empty()) {
				config.get("upnpip", upnpip);
			}
		}
		if (!(op_flags & OPT_P) && config.get("upnpport", value)) {
			upport = atoi(value.c_str());
		}
	}

	if (upnppdebug::Logger::getTheLog(logfilename) == 0) {
		cerr << "Can't initialize log" << endl;
		return 1;
	}
	upnppdebug::Logger::getTheLog("")->setLogLevel(upnppdebug::Logger::LogLevel(loglevel));

	pidfilename += iface;
    Pidfile pidfile(pidfilename);

	// If started by root, do the pidfile + change uid thing
	uid_t runas(0);
	if (geteuid() == 0) {
		struct passwd *pass = getpwnam(upmpdcliuser.c_str());
		if (pass == 0) {
			LOGFAT("upmpdcli won't run as root and user " << upmpdcliuser << 
				   " does not exist " << endl);
			return 1;
		}
		runas = pass->pw_uid;

		pid_t pid;
		if ((pid = pidfile.open()) != 0) {
			LOGFAT("Can't open pidfile: " << pidfile.getreason() << 
				   ". Return (other pid?): " << pid << endl);
			return 1;
		}
		if (pidfile.write_pid() != 0) {
			LOGFAT("Can't write pidfile: " << pidfile.getreason() << endl);
			return 1;
		}
	}

	if ((op_flags & OPT_D)) {
		if (daemon(1, 0)) {
			LOGFAT("Daemon failed: errno " << errno << endl);
			return 1;
		}
	}

	if (geteuid() == 0) {
		// Need to rewrite pid, it may have changed with the daemon call
		pidfile.write_pid();
		if (!logfilename.empty() && logfilename.compare("stderr")) {
			if (chown(logfilename.c_str(), runas, -1) < 0) {
				LOGERR("chown("<<logfilename<<") : errno : " << errno << endl);
			}
		}
		if (setuid(runas) < 0) {
			LOGFAT("Can't set my uid to " << runas << " current: " << geteuid()
				   << endl);
			return 1;
		}
	}

	// Initialize libupnpp, and check health
	LibUPnP *mylib = 0;
	string hwaddr;
	int libretrysecs = 10;
    for (;;) {
		// Libupnp init fails if we're started at boot and the network
		// is not ready yet. So retry this forever
		mylib = LibUPnP::getLibUPnP(true, &hwaddr, iface, upnpip, upport);
		if (mylib) {
			break;
		}
		sleep(libretrysecs);
		libretrysecs = MIN(2*libretrysecs, 120);
	}

	if (!mylib->ok()) {
		LOGFAT("Lib init failed: " <<
			   mylib->errAsString("main", mylib->getInitError()) << endl);
		return 1;
	}

	//string upnplogfilename("/tmp/upmpdcli_libupnp.log");
	//mylib->setLogFileName(upnplogfilename, LibUPnP::LogLevelDebug);

	// Initialize MPD client module
	MPDCli mpdcli(mpdhost, mpdport, mpdpassword);
	if (!mpdcli.ok()) {
		LOGFAT("MPD connection failed" << endl);
		return 1;
	}

	//mpdcli.clearQueue();

	// Create unique ID
	string UUID = LibUPnP::makeDevUUID(friendlyname, hwaddr);

	// Read our XML data to make it available from the virtual directory
	string reason;
	unordered_map<string, string> xmlfiles;
	for (int i = 0; i < xmlfilenamescnt; i++) {
		string filename = path_cat(datadir, xmlfilenames[i]);
		string data;
		if (!file_to_string(filename, data, &reason)) {
			LOGFAT("Failed reading " << filename << " : " << reason << endl);
			return 1;
		}
		if (i == 0) {
			// Special for description: set UUID and friendlyname
			data = regsub1("@UUID@", data, UUID);
			data = regsub1("@FRIENDLYNAME@", data, friendlyname);
		}
		xmlfiles[xmlfilenames[i]] = data;
	}

	// Initialize the UPnP device object.
	UpMpd device(string("uuid:") + UUID, xmlfiles, &mpdcli,
				 ownqueue ? UpMpd::upmpdOwnQueue : UpMpd::upmpdNone);
    	dev = &device;
	pf = &pidfile;
    	setupsigs();
   	StartUpmpdFIFO(&thread);

	device.Signal_register();
	// And forever generate state change events.
	LOGDEB("Entering event loop" << endl);
	device.eventloop();

	return 0;
}

/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
