#include "collect.h"

#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <json/json.h>
#include <sys/stat.h>  
#include <sys/types.h>
#include <dirent.h>

#include <fcntl.h>
#include "http.h"
#include "mpdcli.h"
#include "DeviceStatus.h"
#include "PlaylistJson.h"

static pthread_t collectPthread = 0;
static pthread_t collectWait = 0;
static sem_t collectSem;
static int collectState = COLLECT_NONE;

static pthread_mutex_t collectMtx = PTHREAD_MUTEX_INITIALIZER;

#define COLLECT_DIR		      "收藏"



int IsCollectCancled()
{
	int ret;
	pthread_mutex_lock(&collectMtx);
	ret = (collectState == COLLECT_CANCLE);
	pthread_mutex_unlock(&collectMtx);
	return ret;
}
int IsCollectRuning()
{
	int ret;
	pthread_mutex_lock(&collectMtx);
	ret = (collectState == COLLECT_ONGING);
	pthread_mutex_unlock(&collectMtx);
	return ret;
}

void SetCollectState(int state)
{
	pthread_mutex_lock(&collectMtx);
	collectState = state;
	pthread_mutex_unlock(&collectMtx);
}


int IsCollectFinshed()
{
	int ret;
	pthread_mutex_lock(&collectMtx);
	ret = (collectState == COLLECT_NONE);
	pthread_mutex_unlock(&collectMtx);
	return ret;
}
int IsCollectDone()
{
	int ret;
	pthread_mutex_lock(&collectMtx);
	ret = (collectState == COLLECT_DONE);
	pthread_mutex_unlock(&collectMtx);
	return ret;
}
void SetCollectWait(int wait)
{
	pthread_mutex_lock(&collectMtx);
	collectWait = wait;
	pthread_mutex_unlock(&collectMtx);
}
void StartCollectThread()
{
	pthread_mutex_lock(&collectMtx);
	if(collectWait == 0) {
		sem_post(&collectSem);
	}
	pthread_mutex_unlock(&collectMtx);
}

static void StartCollect()
{
	pthread_mutex_lock(&collectMtx);
	if(collectWait == 0) {
		sem_post(&collectSem);
	}
	pthread_mutex_unlock(&collectMtx);

}
/*
 * 检查tf卡剩余容量
 * 如果小于要下载的size,则删除一些文件，直至容量不小于downloadSize
 */
int CheckFreeSize(char *dir, unsigned long downloadSize)
{
	char buf[128] = {0};
	int ret = -1;
	char *file = "/usr/script/playlists/collplaylist.m3u";
	
	unsigned long freeSize = GetDeviceFreeSize(dir);
	info("freeSize:%lu, downloadSize:%lu",freeSize, downloadSize);
	if(freeSize > downloadSize) {
		info("freeSize > downloadSize");
		return 0;
	}
	//if(freeSize <= 0)
	//	return 0;
#if 0
	while(freeSize < downloadSize) {
		char *line = GetFileFistLine(file);
		if(line == NULL) {
			error("line == NULL");
			ret = -1;
			break;
		}
		info("line:%s",line);
		snprintf(buf, 1024, "/media/%s",line);	
		free(line);
		
		unlink(buf);
		SyncCollectResource();
		freeSize = GetDeviceFreeSize(dir);
		info("freeSize:%lu, downloadSize:%lu",freeSize, downloadSize);
	}
#else
	struct dirent *next = NULL;
	DIR *d; //声明一个句柄

	struct stat sb;    
	snprintf(buf, 128, "%s/收藏", dir);	
	info("buf:%s",buf);
	if(!(d = opendir(buf)))
	{
		error("error opendir %s!!!",buf);
		return -1;
	}		
	while ((next = readdir(d)) != NULL) 
	{
		char tmp[1024]= {0};
        memset(tmp, 0, 1024);
		snprintf(tmp, 1024, "%s/%s", buf, next->d_name);
		if(stat(tmp, &sb) >= 0 && S_ISREG(sb.st_mode))
        {	
			info("tmp:%s",tmp);
            unlink(tmp);
        }
		freeSize = GetDeviceFreeSize(dir);
		if(freeSize > downloadSize)  {
			info("freeSize:%lu, downloadSize:%lu",freeSize, downloadSize);
			info("freeSize > downloadSize",buf);
			ret = 0;
			break;
		}
	}
	closedir(d);

#endif
	return ret;

}
/* 下载url,保存为name文件. */
int DownloadSong(char *url, char *name, int id)
{	
	char dir[256] = {0};
	char buf[32] = {0};
	char value[16] = {0};
	int ret = 0;
	
	double downloadSize;
	
	cdb_get_str("$tf_mount_point",value,16,"");
	snprintf(dir ,256, "/media/%s", value);
	info("dir:%s", dir);
	
	downloadSize = GetDownloadFileLenth(url);//获取要下载的字节数,去http网页上去获取
	if(downloadSize <= 0) {
		return -1;
	}
	info("downloadSize:%f",downloadSize);
	// /media/mmcblk0p1/收藏/下雨天.mp3
	long fileSize = getFileSize(name);	

	info("fileSize:%ld",fileSize);
	if(fileSize == downloadSize) {
		info("fileSize == downloadSize");
		return 1;			//返回1，已经收藏，不继续往下走
	}

	ret = CheckFreeSize(dir, downloadSize);//内存空间不够了，需要释放内存
	if(ret < 0)
		return ret;
	info("after CheckFreeSize");
	if(id > 0) {
		IotCollectSong(id);		//上报信息到服务器
	}
	unlink(name);
	info("name:%s", name);
	//name:/media/mmcblk0p1/收藏/枫桥夜泊.mp3
	//传入音乐文件的名字:name和链接:url，将内容下载得到该文件中
	ret = HttpDownLoadFile(url, name);//成功返回0，即收藏成功
	return ret;
}
/* 向收藏的m3u和json文件中添加歌曲 */
int CreatCollPlaylist(AudioStatus *audioStatus, char *filename)
{
	int ret = 0;
	char cmd[512] ={0};
	char *url = NULL;
	char *title = NULL;
	char *tip = NULL;
	char *mediaId = NULL;
	if(filename) {
		url = filename;
	} else {
		url = "null";
	}
	if(audioStatus->title) {
		title = audioStatus->title;
	} else {
		title = "null";
	}
	if(audioStatus->tip) {
		tip = audioStatus->tip;
	} else {
		tip = "null";
	}
	if(audioStatus->mediaId) {
		mediaId = audioStatus->mediaId;
	} else {
		mediaId = "null";
	}
	snprintf(cmd, 512, "creatPlayList coll \"%s\" \"%s\" null \"%s\" \"%s\" null null 0 null", url, title, tip, mediaId);
	return exec_cmd(cmd);
}

/* 
 * 收藏歌曲
 * 网络歌曲，正在播放，当前歌曲在TURING_PLAYLIST_JSON中存在,才可以收藏 
 * 播放的不是网络歌曲，或者没有播放，会播放TF卡收藏夹里的歌曲
 */
static int SaveCurrentSong(char *dir)
{
	int ret = -1;
	int id = -1;
	char fileName[512]= {0};
	char url[256]= {0};
	char* songName = NULL;
	char *speech = "已经收藏过了";
	int pause; 
	AudioStatus *audioStatus =  newAudioStatus();
	MpdGetAudioStatus(audioStatus);		
	if(audioStatus->state == MPDS_PLAY)
	{
		int localOrUri = UriHasScheme(audioStatus->url);
		MusicItem *music = GetMusicItemFromJsonFile(TURING_PLAYLIST_JSON, audioStatus->url);
		if(music) {
			localOrUri = 1;
			if(music->pId) {
				audioStatus->mediaId = strdup(music->pId);
				id = atoi(audioStatus->mediaId);
			}
			if(music->pTitle)
				audioStatus->title =  strdup(music->pTitle);
			if(music->pTip)
				audioStatus->tip =  strdup(music->pTip);
			warning("music->pContentURL:%s",music->pContentURL);
			FreeMusicItem(music);
		} else {
			localOrUri = 0;
		}
		warning("localOrUri:%d",localOrUri);
		if(localOrUri == 1) {
			SetCollectState(COLLECT_ONGING);	//收藏中...
			warning("song->uri:%s",audioStatus->url);
			if(audioStatus->title) 	
			{
				songName =  audioStatus->title;
				snprintf(fileName, 512, "%s/%s.mp3", dir ,songName);
				//fileName: /media/mmcblk0p1/收藏/下雨天.mp3
				snprintf(url, 256, "%s.mp3",songName);
				
				ret = DownloadSong(audioStatus->url,  fileName, id);//下载要收藏的歌曲，id:唯一的标识id

			}
			else 		
			{
				char name[256] = {0};
				cdb_get_str("$turing_song_name",name,256,"0");//获取cdb中该变量的歌曲链接
				info("turing_song_name : %s",name);	//0
				if(strcmp(name, "0") != 0) 
				{
					snprintf(fileName, 512, "%s/%s.mp3",dir , name);
					snprintf(url, 256, "%s.mp3",songName);
					warning("collect %s",fileName);
					ret = DownloadSong(audioStatus->url,  fileName, id);
				} 
				else 
				{
				//uri:http://appfile.tuling123.com/media/audio/20180524/9830db67a23440a39a25e45de4c8a41e.mp3
				//截取audioStatus->url对应的歌曲链接:9830db67a23440a39a25e45de4c8a41e.mp3
					songName = GetUrlSongName(audioStatus->url);	
					if(songName) 
					{
						snprintf(fileName, 512, "%s/%s",dir , songName);
						snprintf(url, 256, "%s.mp3",songName);
						warning("collect %s",fileName);
						//collect /media/mmcblk0p1/收藏/9830db67a23440a39a25e45de4c8a41e.mp3
						ret = DownloadSong(audioStatus->url,  fileName, id);
						free(songName);
					}
				}
					
			}
				
			if(ret == 1) 		//已经收藏过了
			{			

				text_to_sound(speech);
				//CreatCollPlaylist(audioStatus, url);
				SyncCollectResource();

			}
			else if(ret == 0) 		//收藏成功
			{					

				text_to_sound("收藏成功");
				//CreatCollPlaylist(audioStatus, url);
				SyncCollectResource();
			}
						
		}
		else 
		{
			warning("local");
			ret = ScanCollectionDir();
		}
		
		
	}
	else 
	{
		warning("status == MPDS_STOP");
		ret = ScanCollectionDir();		//开始搜索收藏夹的内容
	}

	freeAudioStatus(&audioStatus);

	return ret;
	
}
/* 检测是否插入TF卡 */
int CheckTfExist()
{
	char *speech = "请先插入tf卡";
	int tfStatus =0;
	tfStatus = cdb_get_int("$tf_status", 0);
	warning("tf_status=%d", tfStatus);
	if(tfStatus == 0)		//没有tf卡
		text_to_sound(speech);
	return tfStatus;
}
/* 等待当前收藏完成 */
static void WaitForCollectFinshed()
{	
 
	if(!IsCollectFinshed()) {
		debug("Last Collect Not Finshed....");
		SetCollectState(5);
		debug("Set Collect Cancle...");
		while(!IsCollectFinshed()) {
		}
		debug("Collect is finished..");
	}


}
/* 启动收藏 */
int CollectSong()
{
	int ret;
	int tfStatus =0;

	char *speech = "请先插入tf卡";
	char *speech3 = "正在收藏中";
	
	tfStatus = cdb_get_int("$tf_status", 0);
	//warning("tf_status=%d", tfStatus);
	if(CheckTfExist() == 0)
		return 0;		//没插入tf卡,返回
	
	if(IsCollectRuning()) {
		warning("IsCollectRuning");
		ret = text_to_sound(speech3);
		goto exit;
	}
	WaitForCollectFinshed();
	StartCollect();		//通知收藏线程，开始收藏
exit:
	return ret;	
}
/* 收藏歌曲 */
static void CollectMusic()
{
	int ret ;
	char collDir[256] = {0};
	char buf[32] = {0};
	char value[16] = {0};
	cdb_get_str("$tf_mount_point",value,16,"");
	snprintf(collDir ,256, "/media/%s/%s", value, COLLECT_DIR);
	warning("collDir:%s",collDir);
	//collDir:mkdir /media/mmcblk0p1/收藏
	ret = create_path(collDir);
	warning("ret:%d",ret);
	if(ret < 0)
		return ret;

	return SaveCurrentSong(collDir);
	
}
/* 收藏线程 */
static void *CollectPthread(void *arg)
{ 	
	char *dst = NULL;
	while(1){
		warning("COLLECT_NONE ...");
		SetCollectState(COLLECT_NONE);
		SetCollectWait(0);
		warning("sem_wait collectSem ");

		//阻塞等待唤醒，StartCollect()该函数通知唤醒线程
		sem_wait(&collectSem);
		SetCollectWait(1);
		SetCollectState(COLLECT_STARTED);//收藏开始
		warning("collect start..");	
		CollectMusic();	
		SetCollectState(COLLECT_NONE);	//收藏完成
		warning("COLLECT_DONE ...");
	}
}

/* 启动收藏线程 */
void    CreateCollectPthread(void)
{
	int iRet;
	pthread_attr_t a_thread_attr; 
	
	pthread_attr_init(&a_thread_attr);
	pthread_attr_setstacksize(&a_thread_attr, PTHREAD_STACK_MIN*2);

	iRet = pthread_create(&collectPthread, &a_thread_attr, CollectPthread, NULL);
	
	pthread_attr_destroy(&a_thread_attr);
	if(iRet != 0)
	{
		error("pthread_create error:%s", strerror(iRet));
		exit(-1);
	}


}
/* 销毁收藏线程 */
int   DestoryCollectPthread(void)
{
	if (collectPthread != 0)
	{
		pthread_join(collectPthread, NULL);
		info("iflytekPthread  end...");
	}	
}










