#ifndef __JSONPARSE_H__
#define __JSONPARSE_H__

#include <iostream>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

template<typename T>
T jGet(json j, const std::string& what, const T& def)
{
	try
	{
		return j[what].get<T>();
	}
	catch(...)
	{
		return def;
	}
}


struct _WARTI_Music {
       std::string pArtist;
	     std::string pTitle;
	     std::string pAlbum;
	     std::string pContentURL;
	     std::string pCoverURL;
	     	
	     	void fromJson(const json& j){
	     	pArtist = jGet<std::string>(j, "artist", "");
			  pTitle = jGet<std::string>(j, "title", "");
			  pAlbum = jGet<std::string>(j, "album", "");
				pContentURL = jGet<std::string>(j, "content_url", "");
				pCoverURL = jGet<std::string>(j, "cover_url", "");
    }
    
    };
    
typedef std::shared_ptr<_WARTI_Music> WARTI_MusicPtr;
//typedef _WARTI_Music *WARTI_MusicPtr;
	
struct _WARTI_MusicList{
	     std::string pRet;				//成功标志
	     int Num;	
	     _WARTI_Music WARTI_Music;
	};
	
//typedef std::shared_ptr<_WARTI_MusicList> WARTI_MusicListPtr;
//typedef std::shared_ptr<_WARTI_MusicList> *WARTI_MusicListPtr;

class jsonParse
{
public:

	jsonParse();
static 	jsonParse& instance()
	{
	   static  jsonParse instance_;
		return instance_;
	}
    string filename_;
    string tf_filename_;
    string usb_filename_;
	string last_musicUrl;
	vector<WARTI_MusicPtr> musics;
	
	WARTI_MusicPtr music = std::make_shared<_WARTI_Music>();
	WARTI_MusicPtr getElementMusic(const std::string& current_url);	
	WARTI_MusicPtr getElementMusicFile(const std::string& current_url,string filename);

};

	
	

#endif