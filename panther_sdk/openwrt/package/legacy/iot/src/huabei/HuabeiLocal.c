#include "HuabeiLocal.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#ifdef USE_MPDCLI
#include  "mpdcli.h"
#endif
#include <fcntl.h>

#define HUABEI_PLAYLIST   			"huabei"
#define HUABEI_MC_PLAYLIST   			"mchuabei"
int HuabeiSetLocalUrl(char *columnDir)
{
	int  ret, length; 
	char columnRootDir[256] = {0};
	char updateDir[256] = {0};
	char value[16] = {0};

	char playlistFileDir[128] = {0};

	cdb_get_str("$tf_mount_point",value,16,"");
	
	snprintf(columnRootDir ,256, "%s/%s", value, columnDir);

	SetHuabeiDataUrl(columnRootDir);
	SetHuabeiDataDirName(columnDir);
	return 0;
}
	
int HuabeiCreatPlaylist(char *columnDir)
{
	int  ret, length; 
	char columnRootDir[256] = {0};
	char updateDir[256] = {0};
	char value[16] = {0};
	char cmd[1024] = {0};
	
	char playlistFileDir[128] = {0};
	char mcPlaylistFileDir[128] = {0};
	char playlistFile[64] = {0};
	char linkFile[512] = {0};

	cdb_get_str("$tf_mount_point",value,16,"");
	
	snprintf(columnRootDir ,256, "/media/%s/%s", value, columnDir);
	snprintf(updateDir ,256, "%s/%s", value, columnDir);
	warning("columnRootDir:%s" ,columnRootDir);
	ret = access(columnRootDir, F_OK);
	if(ret != 0)
		return ret;

	snprintf(playlistFile, 64, "%splaylist" ,HUABEI_PLAYLIST);
	snprintf(playlistFileDir, 128, "/usr/script/playlists/%splaylist.m3u" ,HUABEI_PLAYLIST);
	snprintf(mcPlaylistFileDir, 128, "/usr/script/playlists/%splaylist.m3u" ,HUABEI_MC_PLAYLIST);	

	//if( access(playlistFileDir, F_OK) ==0  &&  access(mcPlaylistFileDir, F_OK) ==0 )
	//{
	
	//	error(" current playlist link to %s", playlistFileDir);
	//	goto link;
	//}
	
	warning("columnDir:%s", columnDir);
	unlink(playlistFileDir);
	unlink(mcPlaylistFileDir);

#ifdef USE_MPDCLI
	MpdUpdate(updateDir);

#else
	eval("mpc", "update", updateDir);
#endif
	memset(cmd, 0, 1024);
	error(" create	%s", playlistFileDir);
	snprintf(cmd, 1024, "mpc listall | grep \"%s\" | grep -v _mc > /tmp/tmp.txt && cp /tmp/tmp.txt %s",columnDir, playlistFileDir);
	exec_cmd(cmd);
	
	memset(cmd, 0, 1024);
	snprintf(cmd, 1024, "mpc listall | grep \"%s\" | grep _mc > /tmp/tmp.txt && cp /tmp/tmp.txt %s",columnDir, mcPlaylistFileDir);
	exec_cmd(cmd);
	if(access(playlistFileDir,F_OK) != 0){
		return -1;
	}
	if(access(mcPlaylistFileDir,F_OK) != 0){
		return -1;
	}

link:
	unlink(CURRENT_PLAYLIST_M3U);
	unlink(MC_CURRENT_PLAYLIST);

	symlink(playlistFileDir, CURRENT_PLAYLIST_M3U);
	symlink(mcPlaylistFileDir, MC_CURRENT_PLAYLIST_M3U);
	return 0;
}


int HuabeiPlaylistPlayMc()
{
	char cmd[1024] = {0};
#if 0
	MpdVolume(200);
	MpdClear();
	MpdRepeat(0);
	MpdSingle(1);
	MpdLoad(MC_CURRENT_PLAYLIST);
#else
	SetHuabeiSendStop(0);
	cdb_set_int("$turing_mpd_play", 0);
	exec_cmd("mpc volume 200");
	exec_cmd("mpc clear");
	exec_cmd("mpc repeat off");
	exec_cmd("mpc single on");
	bzero(cmd, 1024);
	snprintf(cmd, 1024,"mpc load %s", MC_CURRENT_PLAYLIST);
	exec_cmd(cmd);
#endif
	exec_cmd("mpc play");
	return 0;
}


int HuabeiPlaylistToIndex(char *url)
{
	FILE *dp =NULL;
	int ret = -1;
	size_t len = 0;
	int num  = 0;
	ssize_t read;  
	char line[1024];
	if(url)
	{

		dp= fopen(CURRENT_PLAYLIST_M3U, "r");
		if(dp == NULL)
			return -1;
		//while ((read = getline(&line, &len, dp)) != -1)  
		while(!feof(dp))
	    {  
	   	    memset(line, 0 , 1024);
	      
			fgets(line, sizeof(line), dp);
			int len =  strlen(line) -1;
			char ch = line[len];
			if(ch  == '\n' || ch  == '\r')
				line[strlen(line)-1] = 0;
		    line[strlen(line)] = 0;
			num++;
			//if(strcmp(line, "mmcblk0p1/Xiaohuajia_Qingwa_01/Xiaohuajia_Qingwa_01_01.mp3")==0)
			if(strstr(line, url) != NULL)
			{
				break;
			}

		
	    }  
		ret  = num;
		/*if(line) {
			free(line);
			line = NULL;
		}*/
	}	
	if(dp) {
		fclose(dp);
	}
	return ret;
}
void PlaylistPlay(char *url)
{
	
	int index;
	error("url:%s",url);	
#ifdef USE_MPDCLI
	MpdVolume(200);
	MpdClear();
	MpdRepeat(1);
	MpdLoad(CURRENT_PLAYLIST);
#else
	exec_cmd("mpc voloume 200");
	exec_cmd("mpc clear");
	exec_cmd("mpc repeat on");
	eval("mpc", "load", CURRENT_PLAYLIST);
#endif

	index = HuabeiPlaylistToIndex(url);
	error("index:%d",index);

#if 0
	index--;
	MpdRunSeek(index, 0);
#else
	char buf[32]={0};
	snprintf(buf, 128,"mpc play %d", index);
	exec_cmd(buf);
#endif

}






