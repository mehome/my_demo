#!/bin/sh

alexaplaylistFile="/usr/script/playlists/alexaPlayList.m3u"
alexaplaylistFileName="alexaPlayList.m3u"
playlistFile="/usr/script/playlists/playlist.m3u"
playlistFileName="playlist.m3u"
usbPlaylistFile="/usr/script/playlists/usbPlaylist.m3u"
usbPlaylistFileName="usbPlaylist.m3u"
tfPlaylistFile="/usr/script/playlists/tfPlaylist.m3u"
tfPlaylistFileName="tfPlaylist.m3u"

currentplaylist="/usr/script/playlists/currentplaylist.m3u"
currentplaylistname="currentplaylist.m3u"

collPlaylistFileName="collPlaylist.m3u"
collPlaylistFileNameInfo="/usr/script/playlists/collPlaylistInfoJson.txt"

kant_m3u_path="/usr/script/playlists/kant.m3u"
tank_m3u_path="/usr/script/playlists/tank.m3u"


tank_one="/usr/script/playlists/tank_one.m3u"
tank_two="/usr/script/playlists/tank_two.m3u"
tank_three="/usr/script/playlists/tank_three.m3u"
tank_four="/usr/script/playlists/tank_four.m3u"
tank_five="/usr/script/playlists/tank_five.m3u"

coll_playlist_one="/usr/script/playlists/coll_one.m3u"
coll_playlist_two="/usr/script/playlists/coll_two.m3u"
coll_playlist_three="/usr/script/playlists/coll_three.m3u"
coll_playlist_four="/usr/script/playlists/coll_four.m3u"
coll_playlist_five="/usr/script/playlists/coll_five.m3u"


  

POWER_ON_START_PLAY()
{

	mpc clear > /dev/null
	cdb  set playmode 000 
	
	#判断文件是否为空，如果为空则直接返回，如果存在则将列表load到mpd,并将mpc的播放模式设为循环播放，mpc 1
	if [ -s "$playlistFile" ]; then
		echo "playlistFile is not empty!"
	    cp /usr/script/playlists/httpPlayListInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
		
        cp  /usr/script/playlists/playlist.m3u  /usr/script/playlists/currentplaylist.m3u
		mpc load $currentplaylistname > /dev/null
		
		mpc play 1
	
		

	else
		echo "playlistFile is a empty! exti..."
		exit 0
	fi
}

LOAD_MUSIC_PLAYLIST()
{
	playlistFilePath=$1
	echo "LOAD_MUSIC_PLAYLIST: $playlistFilePath"
	if [ ! -f "$playlistFilePath" ]; then
		echo "$playlistFilePath is a empty! exti..."
		exit 0
	fi
	
  playIndex=$2
  echo "playIndex=====: $playIndex"
	if [ ! -n "$playIndex" ]; then
		 playIndex=1;
		 echo "playIndex 111111: $playIndex"
	fi

	#获取列表的长度，linenum1：当前列表 linenum2：推送的列表
	linenum1=`wc -l $playlistFile | awk '{print $1}'`; echo "linenum1:$linenum1"; #linenum1=${linenum1:0:2}
	linenum2=`wc -l $playlistFilePath | awk '{print $1}'`; echo "linenum2:$linenum2"; #linenum2=${linenum2:0:2}

	#如果推送的列表长度超过了30条或者当前列表为空，那么直接将推送列表拷贝到当前列表
	#if [ "$linenum2" -ge "29" ] || [ ! -s "$playlistFile" ]; then
	    cp /usr/script/playlists/httpPlayListInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
		cp $playlistFilePath $playlistFile
		mpc clear > /dev/null
		mpc load $playlistFileName > /dev/null
		mpc play $playIndex  > /dev/null
		cdb  set playmode 000 
		cp  /usr/script/playlists/playlist.m3u /usr/script/playlists/currentplaylist.m3u
		
		

	#4.删除列表文件
	if [ $playlistFilePath != $playlistFile ] && [ $playlistFilePath != $playlistFileName ]; then
		rm $playlistFilePath
	fi
}

REPLACE_MUSIC_PLAYLIST()
{
	playlistFilePath=$1
	echo "LOAD_MUSIC_PLAYLIST: $playlistFilePath"
	if [ ! -f "$playlistFilePath" ]; then
		echo "$playlistFilePath is a empty! exti..."
		exit 0
	fi
  playIndex=$2
  echo "playIndex=====: $playIndex"
	if [ ! -n "$playIndex" ]; then
		 playIndex=1;
		 echo "playIndex 111111: $playIndex"
	fi

	#获取列表的长度，linenum1：当前列表 linenum2：推送的列表
#	linenum1=`wc -l $playlistFile | awk '{print $1}'`; echo "linenum1:$linenum1"; #linenum1=${linenum1:0:2}
#	linenum2=`wc -l $playlistFilePath | awk '{print $1}'`; echo "linenum2:$linenum2"; #linenum2=${linenum2:0:2}

	#如果推送的列表长度超过了30条或者当前列表为空，那么直接将推送列表拷贝到当前列表
#	if [ "$linenum2" -ge "29" ] || [ ! -s "$playlistFile" ]; then
        cp /usr/script/playlists/httpPlayListInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
		cp $playlistFilePath $playlistFile
		mpc clear > /dev/null
		mpc load $playlistFileName > /dev/null
		mpc play $playIndex  > /dev/null
		cdb  set playmode 000 
		cp  /usr/script/playlists/playlist.m3u  /usr/script/playlists/currentplaylist.m3u
		
		
		
	#4.删除列表文件
	if [ $playlistFilePath != $playlistFile ] && [ $playlistFilePath != $playlistFileName ]; then
		rm $playlistFilePath
	fi
}

USB_MUSIC_PLAYLIST()
{

    echo "in usb"
	
	mpc clear > /dev/null
	mpc update > /dev/null
	sleep 1
	
	
	mpc listall | grep sd > $usbPlaylistFile
    if [ ! -f "$usbPlaylistFile" ]; then
		echo "$playlistFilePath is a empty! exti..."
		exit 0
	fi
	
   
	cp  /usr/script/playlists/usbPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	mpc load currentplaylist.m3u > /dev/null
	
	uartdfifo.sh inusbmode> /dev/null
	cp /usr/script/playlists/usbPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt	
	mpc play 1 > /dev/null
   
	
	
	cdb  set usb_status 1
	cdb  set playmode 003
	creatPlayList createUJ  > /dev/null
   
	
	echo "out usb"
}

USB_ANOTHER_MUSIC_PLAYLIST()
{
    echo "in usb"
	
	mpc clear > /dev/null
	mpc update > /dev/null
	sleep 1
	
	
	mpc listall | grep sd > $usbPlaylistFile
    if [ ! -f "$usbPlaylistFile" ]; then
		echo "$playlistFilePath is a empty! exti..."
		exit 0
	fi
	
   
	cp  /usr/script/playlists/usbPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	mpc load currentplaylist.m3u > /dev/null
	
	uartdfifo.sh inusbmode> /dev/null
	
	mpc play 1 > /dev/null
   
	
	
	cdb  set usb_status 1
	cdb  set playmode 003
	
		
	echo "out usb"
}



USB_AUTO_PLAY()
{
	playIndex=$1
    if [ ! -n "$playIndex" ]; then
		 playIndex=1;
	fi
	
	echo "someone insert a  USB"
    count=0
    playflag=0
	
	playflag=`cdb get usb_scan_over`
	
	while true
	do
		if [ "$playflag" -eq 1 ]; then
			    if [ ! -f "$usbPlaylistFile" ]; then		
					mpc listall | grep sd > $usbPlaylistFile
				fi
				mpc load usbPlaylist.m3u > /dev/null
				break  
		else
			sleep 1
			let "count++"
			echo "Waiting for scan finished.................."
		fi
		if [ "$count" -eq 20 ];then
			return 1
		fi
		playflag=`cdb get usb_scan_over`
	done
	sleep 1
	mpc play $playIndex > /dev/null
	sleep 1
	mpc repeat on > /dev/null
	
	cdb  set usb_status 1
	cdb  set playmode 003
	
	creatPlayList createUJ > /dev/null

	cp  /usr/script/playlists/usbPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	cp /usr/script/playlists/usbPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
	echo "Exit usbautoplay............"
}

SWITCH_TO_WIFI_MODE()
{

	mpc load playlist.m3u
	
	cp  /usr/script/playlists/playlist.m3u  /usr/script/playlists/currentplaylist.m3u
	cp /usr/script/playlists/httpPlayListInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
	
	mpc play 1 > /dev/null 
	
	cdb  set playmode 000
	
	mpc repeat on > /dev/null
}


SWITCH_TO_PLAY_USB()
{ 
    mpc clear
    playIndex=$1
    if [ ! -n "$playIndex" ]; then
		 playIndex=1;
	fi
    echo "SWITCH_TO_PLAY_USB  playIndex : $playIndex"
	mpc load usbPlaylist.m3u
	
	cp  /usr/script/playlists/usbPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	cp /usr/script/playlists/usbPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
	
	mpc play $playIndex > /dev/null 
	
	cdb  set usb_status 1
	cdb  set playmode 003
	
	mpc repeat on > /dev/null
}

SWITCH_TO_PLAY_TF()
{
    mpc clear
    playIndex=$1
    if [ ! -n "$playIndex" ]; then
		 playIndex=1;		 
	fi
	echo "SWITCH_TO_PLAY_TF playIndex : $playIndex"
	mpc load tfPlaylist.m3u
	
	cp  /usr/script/playlists/tfPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	cp /usr/script/playlists/tfPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
	
	mpc play $playIndex > /dev/null 
	
	cdb  set playmode 004
	
	mpc repeat on > /dev/null
}
TF_MUSIC_PLAYLIST()
{
    echo "someone insert a  tf card"
    
	
    
	mpc clear > /dev/null
	mpc update > /dev/null
	sleep 1
	
	
    mpc listall | grep mmcblk > $tfPlaylistFile
	if [ ! -f "$tfPlaylistFile" ]; then
	     echo "$playlistFilePath is a empty! exti..."
	     exit 0
    fi
	
	
	#cat $tfPlaylistFile
	
	cp  /usr/script/playlists/tfPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	mpc load currentplaylist.m3u > /dev/null
	
	cdb  set tf_status 1
	cdb  set playmode 004
	
	uartdfifo.sh intfmode> /dev/null
	
	mpc play 1 > /dev/null
	
	creatPlayList createTJ  > /dev/null
	cp /usr/script/playlists/tfPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
	
	echo "we scan tf card over"
}

TF_AUTO_PLAY()
{
	playIndex=$1
	
	if [ ! -n "$playIndex" ]; then
		 playIndex=1;
		 echo "playIndex 111111: $playIndex"
	fi
	
	echo "someone insert a  TF card"
    count=0
    playflag=0
	
	playflag=`cdb get tf_scan_over`
	
	while true
	do
		if [ "$playflag" -eq 1 ]; then
			    if [ ! -f "$tfPlaylistFile" ]; then		
					mpc listall | grep mmc > $tfPlaylistFile
				fi
				mpc load tfPlaylist.m3u > /dev/null
				break  
		else
			sleep 1
			let "count++"
			echo "Waiting for scan finished.................."
		fi
		if [ "$count" -eq 20 ];then
			return 1
		fi
		playflag=`cdb get tf_scan_over`
	done
	sleep 1
	mpc play $playIndex > /dev/null
	sleep 1
	mpc repeat on > /dev/null
	
	cdb  set tf_status 1
	cdb  set playmode 004
	
	creatPlayList createTJ > /dev/null

	cp  /usr/script/playlists/tfPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	cp /usr/script/playlists/tfPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
	echo "Exit tfautoplay............"
}
TF_ANOTHER_MUSIC_PLAYLIST()
{
 
    echo "in tf "
	
    
	mpc clear > /dev/null
	mpc update > /dev/null
	sleep 1
	
	
    mpc listall | grep mmcblk > $tfPlaylistFile
	if [ ! -f "$tfPlaylistFile" ]; then
	     echo "$playlistFilePath is a empty! exti..."
	     exit 0
    fi
	
	
	#cat $tfPlaylistFile
	
	cp  /usr/script/playlists/tfPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	mpc load currentplaylist.m3u > /dev/null
	
	cdb  set tf_status 1
	cdb  set playmode 004
	
	uartdfifo.sh intfmode> /dev/null
	
	mpc play 1 > /dev/null
	
    echo "out tf"
}

UPDATE_TF_USB_PLAYLIST_INFO()
{
  mpc update > /dev/null
	sleep 1
  mpc listall | grep mmcblk > $tfPlaylistFile
  if [ ! -f "$tfPlaylistFile" ]; then
		echo "$playlistFilePath is a empty! exti..."
		exit 0
	fi
	
# mpc update > /dev/null
# sleep 1
 mpc listall | grep sd > $usbPlaylistFile
  if [ ! -f "$usbPlaylistFile" ]; then
		echo "$playlistFilePath is a empty! exti..."
		exit 0
	fi
	
}

PLAY_USB_MUSIC()
{
    playIndex=$1
	
	if [ ! -n "$playIndex" ]; then
		 playIndex=1;
		 echo "playIndex 111111: $playIndex"
	fi
    mpc clear > /dev/null
	
    cp  /usr/script/playlists/usbPlaylist.m3u  /usr/script/playlists/currentplaylist.m3u
	mpc load currentplaylist.m3u > /dev/null
	
	cdb  set playmode 003
		
	uartdfifo.sh inusbmode> /dev/null

      cp /usr/script/playlists/usbPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
      mpc play $playIndex > /dev/null

			
}

PLAY_TF_MUSIC()
{
    playIndex=$1
	if [ ! -n "$playIndex" ]; then
		 playIndex=1;
		 echo "playIndex 111111: $playIndex"
	fi
    mpc clear > /dev/null
    cp  /usr/script/playlists/tfPlaylist.m3u   /usr/script/playlists/currentplaylist.m3u
	mpc load currentplaylist.m3u > /dev/null
	
	uartdfifo.sh intfmode> /dev/null
	
	cdb  set playmode 004
		
    mpc play $playIndex > /dev/null
	
	cp /usr/script/playlists/tfPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
}

UPDATE_MUSIC_PLAYLIST()
{
	playlistFilePath=$1
	echo "UPDATE_MUSIC_PLAYLIST: $playlistFilePath"

	if [ ! -f "$playlistFilePath" ]; then
		echo "$playlistFilePath is a empty! exti..."
		exit 0
	fi

	#获取列表的长度，linenum1：当前列表 linenum2：推送的列表
	linenum1=`wc -l $playlistFile | awk '{print $1}'`; echo "linenum1:$linenum1"; #linenum1=${linenum1:0:2}
	linenum2=`wc -l $playlistFilePath | awk '{print $1}'`; echo "linenum2:$linenum2"; #linenum2=${linenum2:0:2}

	#如果推送的列表长度超过了30条或者当前列表为空，那么直接将推送列表拷贝到当前列表
	#if [ "$linenum2" -ge "29" ] || [ ! -s "$playlistFile" ]; then
		cp $playlistFilePath $playlistFile
		#cat $playlistFile
#		mpc clear > /dev/null
		#mpc stop > /dev/null
#		mpc load $playlistFileName > /dev/null
		#mpc repeat on > /dev/null
#		mpc play 1
	#else
	#	linenum=`expr $linenum1 + $linenum2`
	#	if [ "$linenum" -ge "29" ]; then
			# 推送列表加当前列表大于了30
			linenum=`expr 29 - $linenum2`
		
	#		echo "" >> $playlistFilePath
			# sed 是从第一行开始的计算的，其它是从第0行
			#linenum=`expr $linenum + 1`
	#		echo "`sed -n "1,${linenum}p" $playlistFile`" >> $playlistFilePath
	#		cp $playlistFilePath $playlistFile
			#cat $playlistFile
#			mpc clear > /dev/null
			#mpc stop > /dev/null
#			mpc load $playlistFileName > /dev/null
			#mpc repeat on > /dev/null
#			mpc play 1
	#	else
			# 推送列表加当前列表小于30
	#		echo "" >> $playlistFilePath
			#cat $playlistFile >> $playlistFilePath
	#		echo "`sed -n "1,${linenum1}p" $playlistFile`" >> $playlistFilePath
			#paste -d $playlistFile $playlistFilePath
	#		cp $playlistFilePath $playlistFile
	#		echo "----------------------------------------------"
			#cat $playlistFile
#			mpc clear > /dev/null
			#mpc stop > /dev/null
#			mpc load $playlistFileName > /dev/null
			#mpc repeat on > /dev/null
#			mpc play 1
	#	fi
	#fi

	#4.删除列表文件
	if [ $playlistFilePath != $playlistFile ] && [ $playlistFilePath != $playlistFileName ]; then
		rm $playlistFilePath
	fi
}


ADD_MUSIC()
{
	url=$1

	if [ ! -f "$url" ]; then
		linenum1=`wc -l $playlistFile | awk '{print $1}'`; echo "linenum1:$linenum1"; #linenum1=${linenum1:0:2}
		
		if [ ! -s "$playlistFile" ]; then
			echo "$url" >> $playlistFile
			#mpc add $url > /dev/null
			#mpc stop > /dev/null
			mpc clear > /dev/null
			cp  $playlistFileName  /usr/script/playlists/currentplaylist.m3u
		    cp  /usr/script/playlists/playlist.m3u > /dev/null
			cp /usr/script/playlists/httpPlayListInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
			
			#mpc load $playlistFileName > /dev/null
			#sleep 2
			mpc play 1
			
			exit 0
		fi

		#if [ "$linenum1" -ge "29" ]; then
			# 插入行
		#	sed -i "1i\\$url" $playlistFile
			# 删除行
		#	sed -i '$d' $playlistFile
		#	mpc clear > /dev/null
			#mpc stop > /dev/null
		#	mpc load $playlistFileName > /dev/null
			#mpc add $url > /dev/null
			#mpc repeat on > /dev/null
		#	mpc play 1
		#else
			sed -i "1i\\$url" $playlistFile
			mpc clear > /dev/null
			#mpc stop > /dev/null
			cp  /usr/script/playlists/playlist.m3u /usr/script/playlists/currentplaylist.m3u
		    mpc load $currentplaylistname > /dev/null
			cp /usr/script/playlists/httpPlayListInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
			
			#mpc load $playlistFileName > /dev/null
			#mpc add $url > /dev/null
			#mpc repeat on > /dev/null
			mpc play 1 > /dev/null
		
		#fi

	else
		LOAD_MUSIC_PLAYLIST $1
	fi
}
PLAY_ALEXA_MUSIC()
{

			mpc clear > /dev/null

			mpc add "$1"
			cp /usr/script/playlists/alexaPlayListInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
			mpc play 1 > /dev/null
			mpc playlist > /usr/script/playlists/alexaPlayList.m3u 
			cp  /usr/script/playlists/alexaPlayList.m3u  /usr/script/playlists/currentplaylist.m3u
#			uci set xzxwifiaudio.config.playmode=005 && uci commit
			
           
		
  
 
}
 PLAY_COLL_MUSIC()  #还缺少收藏播放列表
 {
           
            index=$1
			
			#echo  "$index"
			#echo  "index==$index"
		   
		  
		    mpc clear > /dev/null
			cd /usr/script/playlists/
			#########################################
			#tac  /usr/script/playlists/tank.m3u  >  /usr/script/playlists/kant.m3u
		    #系统没有tac这个功能
	
	      
	       if [ "$index"x = "one"x ]; then
		   
		                echo "in  1........"
		   
						cat  tank_one.m3u  > tank.m3u 
						answer="`tail -1 /usr/script/playlists/tank.m3u`"
						while [ -n "$answer" ]
						do
						  echo "$answer" >> /usr/script/playlists/kant.m3u
						  sed -i '$d' /usr/script/playlists/tank.m3u
						  answer="`tail -1 /usr/script/playlists/tank.m3u`"
						done 
					   ###############################################
					   if [ ! -f "$coll_playlist_one" ]; then
					      touch "$coll_playlist_one"
					    fi
						cat  coll_one.m3u  >>  kant.m3u
						cat  kant.m3u  >  coll_one.m3u
						cat /dev/null > /usr/script/playlists/kant.m3u
						cat /dev/null > /usr/script/playlists/tank.m3u
						cat /dev/null > $tank_two
						#######################################
						cp  /usr/script/playlists/coll_one.m3u   /usr/script/playlists/currentplaylist.m3u
						cp /usr/script/playlists/one_collPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
			
	       elif [ "$index"x = "two"x ]; then
		   
		   
		                 echo "in  2........"
		                cat  tank_two.m3u  > tank.m3u
		   						
						answer="`tail -1 /usr/script/playlists/tank.m3u`"
						while [ -n "$answer" ]
						do
						  echo "$answer" >> /usr/script/playlists/kant.m3u
						  sed -i '$d' /usr/script/playlists/tank.m3u
						  answer="`tail -1 /usr/script/playlists/tank.m3u`"
						done 
					   ###############################################
					    if [ ! -f "$coll_playlist_two" ]; then
					      touch "$coll_playlist_two"
					    fi
						cat  coll_two.m3u  >>  kant.m3u
						cat  kant.m3u  >  coll_two.m3u 
						cat /dev/null > /usr/script/playlists/kant.m3u
						cat /dev/null > /usr/script/playlists/tank.m3u
						cat /dev/null > $tank_two
						#######################################
						cp  /usr/script/playlists/coll_two.m3u   /usr/script/playlists/currentplaylist.m3u
						cp /usr/script/playlists/two_collPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt

		   
		   elif [ "$index"x = "three"x ]; then
		   
		    
		                echo "in  3........"
		                cat /usr/script/playlists/tank_three.m3u  > /usr/script/playlists/tank.m3u
		        						
						answer="`tail -1 /usr/script/playlists/tank.m3u`"
						while [ -n "$answer" ]
						do
						  echo "$answer" >> /usr/script/playlists/kant.m3u
						  sed -i '$d' /usr/script/playlists/tank.m3u
						  answer="`tail -1 /usr/script/playlists/tank.m3u`"
						done 
					   ###############################################
					   if [ ! -f "$coll_playlist_three" ]; then
					      touch "$coll_playlist_three"
					   fi
						cat  coll_three.m3u  >>  kant.m3u
						cat  kant.m3u  >  coll_three.m3u 
						cat /dev/null > /usr/script/playlists/kant.m3u
						cat /dev/null > /usr/script/playlists/tank.m3u
						cat /dev/null > $tank_three
						#######################################
						cp  /usr/script/playlists/coll_three.m3u  /usr/script/playlists/currentplaylist.m3u
						cp /usr/script/playlists/three_collPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt

		   
		   elif [ "$index"x = "four"x ]; then
		     
		                echo "in  4........"
		                cat /usr/script/playlists/tank_four.m3u  > /usr/script/playlists/tank.m3u
		   					
						answer="`tail -1 /usr/script/playlists/tank.m3u`"
						while [ -n "$answer" ]
						do
						  echo "$answer" >> /usr/script/playlists/kant.m3u
						  sed -i '$d' /usr/script/playlists/tank.m3u
						  answer="`tail -1 /usr/script/playlists/tank.m3u`"
						done 
					   ###############################################
					   if [ ! -f "$coll_playlist_four" ]; then
					      touch "$coll_playlist_four"
					   fi
						cat  coll_four.m3u  >>  kant.m3u
						cat  kant.m3u  >  coll_four.m3u 
						cat /dev/null > /usr/script/playlists/kant.m3u
						cat /dev/null > /usr/script/playlists/tank.m3u
						cat /dev/null > $tank_four
						#######################################
						cp  /usr/script/playlists/coll_four.m3u   /usr/script/playlists/currentplaylist.m3u
						cp /usr/script/playlists/four_collPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt
						
			elif [ "$index"x = "five"x ]; then
		     
		                echo "in  5........"
		                cat /usr/script/playlists/tank_five.m3u  > /usr/script/playlists/tank.m3u
		   					
						answer="`tail -1 /usr/script/playlists/tank.m3u`"
						while [ -n "$answer" ]
						do
						  echo "$answer" >> /usr/script/playlists/kant.m3u
						  sed -i '$d' /usr/script/playlists/tank.m3u
						  answer="`tail -1 /usr/script/playlists/tank.m3u`"
						done 
					   ###############################################
					   if [ ! -f "$coll_playlist_five" ]; then
					      touch "$coll_playlist_five"
					   fi
						cat  coll_five.m3u  >>  kant.m3u
						cat  kant.m3u  >  coll_five.m3u 
						cat /dev/null > /usr/script/playlists/kant.m3u
						cat /dev/null > /usr/script/playlists/tank.m3u
						cat /dev/null > $tank_five
						#######################################
						cp  /usr/script/playlists/coll_five.m3u   /usr/script/playlists/currentplaylist.m3u
						cp /usr/script/playlists/five_collPlaylistInfoJson.txt  /usr/script/playlists/currentPlaylistInfoJson.txt


		   
		   fi
		   
	       
		    mpc load currentplaylist.m3u > /dev/null
		    mpc play 1  > /dev/null
			
			cdb  set playmode 006
		
			sleep 1
			#mpc repeat on > /dev/null      
 }
 DEL_COLL_MUSIC()
 {          
            index=$1
			echo "$index"
			
            mpc stop  > /dev/null
            mpc clear > /dev/null
			
			if [ "$index"x = "one"x ];then
			
			echo "in clean 1........"
			rm $tank_one
			rm /usr/script/playlists/coll_one.m3u
			rm /usr/script/playlists/one_collPlaylistInfoJson.txt
			cat /dev/null > /usr/script/playlists/currentplaylist.m3u
			cat /dev/null > /usr/script/playlists/currentPlaylistInfoJson.txt 
			
			elif [ "$index"x = "two"x ]; then
			
			echo "in  clean 2........"
			rm  $tank_two
			rm /usr/script/playlists/coll_two.m3u
		    rm /usr/script/playlists/two_collPlaylistInfoJson.txt
			cat /dev/null > /usr/script/playlists/currentplaylist.m3u
			cat /dev/null > /usr/script/playlists/currentPlaylistInfoJson.txt 
			
			
			elif [ "$index"x = "three"x ]; then
			
			echo "in clean  3........"
		    rm  $tank_three
			rm  /usr/script/playlists/coll_three.m3u
			rm  /usr/script/playlists/three_collPlaylistInfoJson.txt
			cat /dev/null > /usr/script/playlists/currentplaylist.m3u
			cat /dev/null > /usr/script/playlists/currentPlaylistInfoJson.txt 
			
			elif [ "$index"x = "four"x ]; then
			
			echo "in  clean 4........"
			rm  $tank_four
			rm  /usr/script/playlists/coll_four.m3u
			rm  /usr/script/playlists/four_collPlaylistInfoJson.txt
			cat /dev/null > /usr/script/playlists/currentplaylist.m3u
			cat /dev/null > /usr/script/playlists/currentPlaylistInfoJson.txt 
			
			elif [ "$index"x = "five"x ]; then
			
			echo "in  clean 5........"
			rm  $tank_five
			rm  /usr/script/playlists/coll_five.m3u
			rm  /usr/script/playlists/five_collPlaylistInfoJson.txt
			cat /dev/null > /usr/script/playlists/currentplaylist.m3u
			cat /dev/null > /usr/script/playlists/currentPlaylistInfoJson.txt 
			
			elif [ "$index"x = "all"x ]; then 
			echo "in  clean 6........"
			rm $tank_one
			rm $tank_two
			rm $tank_three
			rm $tank_four
			rm /usr/script/playlists/coll_one.m3u
			rm /usr/script/playlists/one_collPlaylistInfoJson.txt
			rm /usr/script/playlists/coll_two.m3u
			rm /usr/script/playlists/two_collPlaylistInfoJson.txt
			rm /usr/script/playlists/coll_three.m3u
			rm /usr/script/playlists/three_collPlaylistInfoJson.txt
			rm /usr/script/playlists/coll_four.m3u
			rm /usr/script/playlists/four_collPlaylistInfoJson.txt
			cat /dev/null > /usr/script/playlists/currentplaylist.m3u
			cat /dev/null > /usr/script/playlists/currentPlaylistInfoJson.txt 
			
			
			fi
			
			
			
			cdb  set playmode 000
 }

#################
if [ ! -f "$playlistFile" ]; then
	touch "$playlistFile"
fi
case "$1" in
	"poweron")
		echo "poweron..."
		POWER_ON_START_PLAY
	;;
	"load")
		echo "load music list..."
		LOAD_MUSIC_PLAYLIST $2 $3
	;;
	"add")
		echo "add music to list..."
		ADD_MUSIC $2
	;;
	"update")
		echo "add music to list..."
		UPDATE_MUSIC_PLAYLIST $2
	;;
	"replace")
		echo "replace music list..."
		REPLACE_MUSIC_PLAYLIST $2
	;;
	"usb")
		echo "play usb music list..."
		USB_MUSIC_PLAYLIST $2
	;;
	"usbautoplay")
		echo "usbautoplay..................."
		USB_AUTO_PLAY  $2
	;;	
	"swtich_to_usb")  
		echo "swtich_to_usb..................."
		SWITCH_TO_PLAY_USB  $2
	;;	
	"swtich_to_tf")   
		echo "swtich_to_tf..................."
		SWITCH_TO_PLAY_TF   $2
	;;
	"swtich_to_wifi_mode")
		echo "swtich_to_wifimode..................."
		SWITCH_TO_WIFI_MODE
	;;	
	"tf")
		echo "play tf music list..."
		TF_MUSIC_PLAYLIST $2
	;;
	"tfautoplay")
		echo "tfautoplay...................."
		TF_AUTO_PLAY $2
	;;

	"anothertf")
		echo "play another tf music list..."
		TF_ANOTHER_MUSIC_PLAYLIST $2
	;;
	"anotherusb")
		echo "play another music list....."
		USB_ANOTHER_MUSIC_PLAYLIST $2
	;;
	"uptu")
		echo "update tf usb music list..."
		UPDATE_TF_USB_PLAYLIST_INFO $2
	;;
	"plu")
		echo "app play usb music..."
		PLAY_USB_MUSIC $2
	;;
	"plt")
		echo "app play tf music..."
		PLAY_TF_MUSIC $2
	;;
	"addalexa")
	    
    	PLAY_ALEXA_MUSIC $2
	;;
	"PlayCollplaylist")
	     echo "play coll music..."
		 PLAY_COLL_MUSIC $2
	;;
	"DelCollPlayList")
	     echo "del coll music..."
		 DEL_COLL_MUSIC $2
	;;
	*)
		echo "Unsupported parameter: $1"
	;;
esac
