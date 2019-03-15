
#include "libupnpp/device.hxx"

#include "dlna/mpdcli.hxx"
#include "dlna/upmpdutils.hxx"
#include "dlna/md5.hxx"

#define FAKEURI "http://localhost/fake.mp3"
#define AVOID_TWINKLING
//#define QQ_PLAYLIST 
//#define DLNA_MPD_RECOVER

#define UPTIMER 10
#define ELAPSMS 9000

// The UPnP MPD frontend device with its 3 services
class UpMpd : public UpnpDevice {
public:
	enum Options {
		upmpdNone,
		upmpdOwnQueue, // The MPD belongs to us, we shall clear it as we like
	};
	UpMpd(const string& deviceid, const unordered_map<string, string>& xmlfiles,
		  MPDCli *mpdcli, Options opts = upmpdNone);

	// RenderingControl
	int setMute(const SoapArgs& sc, SoapData& data);
	int getMute(const SoapArgs& sc, SoapData& data);
	int setVolume(const SoapArgs& sc, SoapData& data, bool isDb);
	int getVolume(const SoapArgs& sc, SoapData& data, bool isDb);
	int listPresets(const SoapArgs& sc, SoapData& data);
	int selectPreset(const SoapArgs& sc, SoapData& data);
//	int getVolumeDBRange(const SoapArgs& sc, SoapData& data);
    virtual bool getEventDataRendering(bool all, 
									   std::vector<std::string>& names, 
									   std::vector<std::string>& values);

	// AVTransport
	int setAVTransportURI(const SoapArgs& sc, SoapData& data, bool setnext);
	int getPositionInfo(const SoapArgs& sc, SoapData& data);
	int getTransportInfo(const SoapArgs& sc, SoapData& data);
	int getXZXPlayMode(const SoapArgs& sc, SoapData& data);
	int getMediaInfo(const SoapArgs& sc, SoapData& data);
	int getDeviceCapabilities(const SoapArgs& sc, SoapData& data);
	int setPlayMode(const SoapArgs& sc, SoapData& data);
	int getTransportSettings(const SoapArgs& sc, SoapData& data);
	int getCurrentTransportActions(const SoapArgs& sc, SoapData& data);
	int playcontrol(const SoapArgs& sc, SoapData& data, int what);
	int seek(const SoapArgs& sc, SoapData& data);
	int seqcontrol(const SoapArgs& sc, SoapData& data, int what);
    virtual bool getEventDataTransport(bool all, 
									   std::vector<std::string>& names, 
									   std::vector<std::string>& values);

	// Connection Manager
	int getCurrentConnectionIDs(const SoapArgs& sc, SoapData& data);
	int getCurrentConnectionInfo(const SoapArgs& sc, SoapData& data);
	int getProtocolInfo(const SoapArgs& sc, SoapData& data);
	virtual bool getEventDataCM(bool all, std::vector<std::string>& names, 
								std::vector<std::string>& values);

	// Re-implemented from the base class and shared by all services
    virtual bool getEventData(bool all, const string& serviceid, 
							  std::vector<std::string>& names, 
							  std::vector<std::string>& values);

	// QQ player service
	int getTracksInfo(const SoapArgs& sc, SoapData& data);
	int setTracksInfo(const SoapArgs& sc, SoapData& data);
	int QPlayAuth(const SoapArgs& sc, SoapData& data);
	int setNetWork(const SoapArgs& sc, SoapData& data);
	int getMaxTracks(const SoapArgs& sc, SoapData& data);

//	int removeTrack(const SoapArgs& sc, SoapData& data);
	virtual bool getEventDataQPlay(bool all, std::vector<std::string>& names, 
								std::vector<std::string>& values);
	void ParseQPlayTracksInfo(const char *metadata, string& startidx, string& nextidx);

	bool chk_valid(unsigned int ip);
private:
	MPDCli *m_mpdcli;

	string m_curMetadata;
	string m_nextUri;
	string m_nextMetadata;

	// State variable storage
	unordered_map<string, string> m_rdstate;
	unordered_map<string, string> m_tpstate;

	// Translate MPD state to Renderer state variables.
	bool rdstateMToU(unordered_map<string, string>& state);
	// Translate MPD state to AVTransport state variables.
	bool tpstateMToU(unordered_map<string, string>& state);

	// For QQ Translate MPD state to AVTransport state variables
	bool tpstateMToU_dlnalib(unordered_map<string, string>& state);

	// My track identifiers (for cleaning up)
	set<int> m_songids;

	// Desired volume target. We may delay executing small volume
	// changes to avoid saturating with small requests.
	int m_desiredvolume;
	int m_premutevolume;
	

	int m_options;

	vector<std::string> songinfo;
#ifdef DLNA_MPD_RECOVER
	vector<std::string> songtitle;
#endif
	int m_seektrack;
	int update;
	time_t uptime;
	unsigned int elapstime;
	std::string m_queueID;
	std::string allmetadata;
};
