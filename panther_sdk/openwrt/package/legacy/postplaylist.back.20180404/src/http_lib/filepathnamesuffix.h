#ifndef __FILEPATHNAMESUFFIX_H__
#define __FILEPATHNAMESUFFIX_H__

//这里面放WifiAudio项目使用的各种临时文件的路径名

#define WIFIAUDIO_POSTALARMCLOCK_PATHNAME "/www/cgi-bin/AlarmClockJson%d.txt"
//存储推送闹钟的JSON文本文件的绝对路径
#define WIFIAUDIO_POSTALARMCLOCK_SEMKEY (6753)
//获取闹钟信息的JSON文本文件，访问之前的信号量key值
//这个值是随便指定的，可以改，因为文件会w+重新创建，所以没有用ftok生成，会变


#define WIFIAUDIO_POSTPLAYLIST_PATHNAME "/tmp/PlayMusicListJson.txt"
//存储推送音乐列表并播放的JSON文本文件的绝对路径


#define WIFIAUDIO_POSTPLAYLISTEX_PATHNAME "/tmp/PlayMusicListJsonEx.txt"
//存储推送音乐列表并播放扩展的JSON文本文件的绝对路径


#define WIFIAUDIO_POSTINSERTLIST_PATHNAME "/tmp/PlayInsertMusicListJson.txt"
//存储推送批量插入音乐列表的JSON文本文件的绝对路径


#define WIFIAUDIO_CURRENTPLAYLIST_PATHNAME "/usr/script/playlists/currentPlaylistInfoJson.txt"
//存储获取当前播放列表的JSON文本文件的绝对路径


#define WIFIAUDIO_TFPLAYLISTINFO_PATHNAME "/usr/script/playlists/tfPlaylistInfoJson.txt"
//存储获取T卡播放列表信息JSON文本文件的绝对路径


#define WIFIAUDIO_USBPLAYLISTINFO_PATHNAME "/usr/script/playlists/usbPlaylistInfoJson.txt"
//存储获取U盘播放列表信息JSON文本文件的绝对路径


#define WIFIAUDIO_POSTSYNCHRONOUSINFOEX_FILENAME "/tmp/SynchronousInfoJsonEx.txt"
//存储推送同步设备信息扩展的JSON文本文件的绝对路径
#define WIFIAUDIO_POSTSYNCHRONOUSINFOEX_SEMKEY (6395)
//存储推送同步设备信息扩展的JSON文本文件，访问之前的信号量key值
//这个值是随便指定的，可以改，因为文件会w+重新创建，所以没有用ftok生成，会变


#define WIFIAUDIO_FIFO_PATHNAME "/tmp/backstage_fifo"
//存储管道通讯文件的完整路径


#define WIFIAUDIO_WHETHERUPDATE_PATHNAME "/tmp/whetherupdate.txt"
//存储是否存在升级文件信息的绝对路径


#define WIFIAUDIO_SNAPCASTSERVER_PATHNAME "/etc/snapcastServer.json"
//存储同步播放配置信息的绝对路径

#define WIFIAUDIO_UPDATEBLUETOOTH_PATHNAME "/tmp/bt_upgrade.bin"
//存储升级蓝牙固件的绝对路径


//uci路径直接在uci配置库当中的头文件当中

#define WIFIAUDIO_UCICONFIG_SEMKEY (8163)
//存储全局配置文件的文件，访问之前的信号量key值
//这个值是随便指定的，可以改，因为文件可能会重新创建，所以这边



#define WIFIAUDIO_RECORDINSERVER_PATH1 "/www/"
//录音文件在web server的路径1
#define WIFIAUDIO_RECORDINSERVER_PATH2 "music/"
//录音文件在web server的路径2
#define WIFIAUDIO_RECORDINSERVER_NAME "recoder_test"
//录音文件在web server的文件名
#define WIFIAUDIO_RECORDINSERVER_SUFFIX ".wav"
//录音文件在web server的后缀


#define WIFIAUDIO_BULETOOTHDEVICELIST_PATHNAME "/tmp/BT_INFO"
//搜索周边蓝牙设备信息文件的绝对路径

#define WIFIAUDIO_BULETOOTHADDR_PATHNAME "/tmp/BT_ADDR"
//连接中的蓝牙地址文件的绝对路径

#define WIFIAUDIO_BULETOOTHSTATUS_PATHNAME "/tmp/BT_STATUS"
//蓝牙状态文件的绝对路径



#define WIFIAUDIO_LEDMATRIXSCREENDATA_PATHNAME "/tmp/LedMatrixScreen"
//存储几个LED点阵屏数据文件的绝对路径，固定屏没有数字后缀，动屏从0开始

#define WIFIAUDIO_LEDFIFO_PATHNAME "/tmp/LEDFIFO"
//关于灯的FIFO的绝对路径

#define WIFIAUDIO_LEDMATRIXSCREENDATAWRITE_SEMKEY (5831)
//给串口写数据的时候要用信号量上锁，免得写一半被kill或者别的写数据插进来，导致数据错误


#define WIFIAUDIO_CONEXANTSFS_PATHNAME "/tmp/oem_image.sfs"
//升级Conexant所需sfs文件的绝对路径

#define WIFIAUDIO_CONEXANTBIN_PATHNAME "/tmp/iflash.bin"
//升级Conexant所需bin文件的绝对路径


#define WIFIAUDIO_NORMALINSYSTEM_PATHNAME "/www/music/recoder_normal.wav"
//普通录音文件在系统上的路径
#define WIFIAUDIO_NORMALINSERVER_PATHNAME "/music/recoder_normal.wav"
//普通录音文件在web server的路径


#define WIFIAUDIO_MICINSYSTEM_PATHNAME "/www/music/recoder_mic.wav"
//麦克录音文件在系统上的路径
#define WIFIAUDIO_MICINSERVER_PATHNAME "/music/recoder_mic.wav"
//麦克录音文件在web server的路径


#define WIFIAUDIO_REFINSYSTEM_PATHNAME "/www/music/recoder_ref.wav"
//参考信号录音文件在系统上的路径
#define WIFIAUDIO_REFINSERVER_PATHNAME "/music/recoder_ref.wav"
//参考信号录音文件在web server的路径

#define WIFIAUDIO_WIFIDOWNLOADPROGRESS_PATHNAME "/tmp/wifi_up_progress.txt"
//固件升级的下载进度查询文件路径
#define WIFIAUDIO_WIFIDOWNLOADPROGRESS_SEMKEY (8246)

#define WIFIAUDIO_WIFIUPGRADEPROGRESS_PATHNAME "/tmp/progress"
//固件升级的烧写进度查询文件路径


#define WIFIAUDIO_BLUETOOTHDOWNLOADPROGRESS_PATHNAME "/tmp/bt_up_progress.txt"
//蓝牙固件升级的下载进度查询文件路径
#define WIFIAUDIO_BLUETOOTHDOWNLOADPROGRESS_SEMKEY (6724)

#define WIFIAUDIO_BLUETOOTHUPGRADEPROGRESS_PATHNAME "/tmp/progress"
//蓝牙固件升级的烧写进度查询文件路径


#define WIFIAUDIO_WIFIANDBLUETOOTHDOWNLOADPROGRESS_PATHNAME "/tmp/wifi_bt_download_progress"
//WiFi和蓝牙固件升级的下载进度查询文件路径

#define WIFIAUDIO_WIFIANDBLUETOOTHUPGRADEPROGRESS_PATHNAME "/tmp/progress"
//WiFi和蓝牙固件升级的烧写进度查询文件路径


#define WIFIAUDIO_ALARMCLOCKANDPLAYPLAN_PATHNAME "/usr/script/playlists/alarm_list.json"
//关于闹钟和定时播放信息的绝对路径

#define WIFIAUDIO_PLAYLISTASPLAN_PATHNAME "/usr/script/playlists/TimePlayListInfo%02d.json"
//定时播放列表的绝对路径
#define WIFIAUDIO_TMPPLAYLISTASPLAN_PATHNAME "/tmp/TimePlayListInfo%02d.txt"
//定时播放列表tmp下的绝对路径

#define WIFIAUDIO_PLAYMIXEDLISTASPLAN_PATHNAME "/usr/script/playlists/PlayMixedListJson%02d.json"
//定时播放混合列表的绝对路径
#define WIFIAUDIO_TMPPLAYMIXEDLISTASPLAN_PATHNAME "/tmp/PlayMixedListJson%02d.json"
//定时播放混合列表tmp下的绝对路径


#define WIFIAUDIO_SHORTCUTKEYLIST_PATHNAME "/usr/script/playlists/CollectionPlayListInfo%02d.json"
//快捷键播放列表的绝对路径
#define WIFIAUDIO_TMPSHORTCUTKEYLIST_PATHNAME "/tmp/CollectionListJson.json"
//快捷键播放列表的绝对路径

#define WIFIAUDIO_SIGNINCONFIG_PATHNAME "/etc/avs/SigninConfig.json"
//登录Amazon之后推送到板端的信息，其中redirectUri和authorizationCode写入到这个文件当中
#define WIFIAUDIO_ALEXACLIENTSDKCONFIG_PATHNAME "/etc/avs/AlexaClientSDKConfig.json"
//登录Amazon之后推送到板端的信息，其中clientid写入到这个文件当中




#endif

