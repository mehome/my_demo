#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include "dlna/json.hpp"
#include "dlna/jsonParse.h"
using json = nlohmann::json;
using namespace std;


jsonParse::jsonParse()
{
	/*
		int i;
		filename_ = "/usr/script/playlists/httpPlayListInfoJson.txt";
		cout << "enter jsonParse()" << endl;
		try
	{
		ifstream ifs(filename_, std::ifstream::in);
		if (ifs.good())
		{
			json j;
			ifs >> j;
			if (j.count("num"))
			{
				json jMusicList = j["musiclist"];
				for (json::iterator it = jMusicList.begin(); it != jMusicList.end(); ++it)
				{
					WARTI_MusicPtr music = std::make_shared<_WARTI_Music>();
					music->fromJson(*it);
				//	LOGDEB("quejw http_didlmake current_url musicInfo =" << musicInfo->pContentURL << endl);
				//	cout << "music->pContentURL ==" << music->pContentURL << endl;
				cout << "jsonParse music->pContentURL ==" << music->pContentURL << endl;
                    musics.push_back(music);
					
				}
			}
		}
	}
	catch(const std::exception& e)
	{
		cout << "catch music->pContentURL == NULL" << endl;
	//	logE << "Error reading config: " << e.what() << "\n";
	}
	*/
}

WARTI_MusicPtr jsonParse::getElementMusicFile(const std::string& current_url,string filename){
		try
		  {   
					ifstream ifs(filename, std::ifstream::in);
					if (ifs.good())
					{
							json j;
							ifs >> j;
							if (j.count("num"))
							{
									json jMusicList = j["musiclist"];
									for (json::iterator it = jMusicList.begin(); it != jMusicList.end(); ++it)
									{	
											//WARTI_MusicPtr music = std::make_shared<_WARTI_Music>();
											music->fromJson(*it);
											if (music->pContentURL == current_url){
												cout << "quejw music->pContentURL ======== current_url=" << music->pContentURL << endl;
                           return music;
						          }
									}
				      }
			    }
		  }
		  catch(const std::exception& e)
		  {
			   cout << "catch music->pContentURL == NULL" << endl;
		  }
	   
	   music->pContentURL = "";
	   return music;
	
	}

WARTI_MusicPtr jsonParse::getElementMusic(const std::string& current_url)
{
		int i;
		filename_ = "/usr/script/playlists/httpPlayListInfoJson.txt";	
		tf_filename_ = "/usr/script/playlists/tfPlaylistInfoJson.txt";		                                   
		usb_filename_ = "/usr/script/playlists/usbPlaylistInfoJson.txt";		                                       
		if(last_musicUrl == current_url){
			cout << "quejw tttttttttt musicUrl == current_url =" << current_url << endl;
				return music;
		}
		else{
				last_musicUrl =current_url;
				music = getElementMusicFile(current_url,filename_);
				 if (music->pContentURL == ""){
					  music = getElementMusicFile(current_url,tf_filename_);
				}if(music->pContentURL == ""){
					  music = getElementMusicFile(current_url,usb_filename_);
				}if(music->pContentURL != ""){
					return music;
					}
			//	cout << "quejw 777777 music->pContentURL ===" << music->pContentURL << endl;
			//		return music;
				/*
			try
		  {
					ifstream ifs(filename_, std::ifstream::in);
					if (ifs.good())
					{
							json j;
							ifs >> j;
							if (j.count("num"))
							{
									json jMusicList = j["musiclist"];
									for (json::iterator it = jMusicList.begin(); it != jMusicList.end(); ++it)
									{
											//WARTI_MusicPtr music = std::make_shared<_WARTI_Music>();
											music->fromJson(*it);
											if (music->pContentURL == current_url){
                           return music;
						          }
									}
				      }
			    }
		  }
		  catch(const std::exception& e)
		  {
			   cout << "catch music->pContentURL == NULL" << endl;
		  }
*/
  
	}
	music->pContentURL = "";
	return music;
}

