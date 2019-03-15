#!/bin/sh

play_record()
{
	  mpc pause > /dev/null 2>&1 
#	  mspotify_cli t > /dev/null 2>&1 
#	  air_cli d playpause > /dev/null 2>&1 
#	  uartdfifo.sh mute000
	  # 若不需要循环播放，可将  --loop -1  去掉
	  mpg123 -q "$*" &
	  #mpg123 -q -A "$*" &
	  #mpc play  > /dev/null 2>&1 
}

url="$*"
echo "url=$url"
play_record $url
