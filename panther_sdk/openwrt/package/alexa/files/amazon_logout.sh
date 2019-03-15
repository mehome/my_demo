#!/bin/sh

# clear value
cdb set amazon_authorizationCode '0'
cdb set amazon_redirectUri '0'
cdb set amazon_clientId '0'
cdb set amazon_ProductId '0'
cdb set amazon_Dsn '0'
cdb set amazon_CodeChallengeMethod '0'
cdb set amazon_CodeChallenge '0'
cdb set amazon_code_verifier '0'
cdb set amazon_AccessToken '0'
cdb set amazon_RefreshToken '0'
cdb set amazon_loginStatus '0'
cdb commit

uartdfifo.sh AlexaAlerEnd
rm /etc/conf/alarms.json

uci set xzxwifiaudio.config.amazon_authorizationCode=0
uci set xzxwifiaudio.config.amazon_redirectUri=0
uci set xzxwifiaudio.config.amazon_clientId=0
uci set xzxwifiaudio.config.amazon_ProductId=0
uci set xzxwifiaudio.config.amazon_Dsn=0
uci set xzxwifiaudio.config.amazon_CodeChallengeMethod=0
uci set xzxwifiaudio.config.amazon_CodeChallenge=0
uci set xzxwifiaudio.config.amazon_code_verifier=0
uci set xzxwifiaudio.config.amazon_AccessToken=0
uci set xzxwifiaudio.config.amazon_RefreshToken=0
uci set xzxwifiaudio.config.amazon_loginStatus=0
uci set xzxwifiaudio.config.amazon_endpoint='https://avs-alexa-na.amazon.com'
uci commit
playmode=`uci get xzxwifiaudio.config.playmode`
if [ "$playmode" = "005" ];then
echo "The current song is alexa song.."
uci set xzxwifiaudio.config.playmode=0 && uci commit
mpc clear
fi

#amazon_client=`ps | grep "AlexaClient" | grep -v grep`
#[ -n "$amazon_client" ] && killall -9 AlexaClient

amazon_client=`ps | grep "AlexaRequest" | grep -v grep`
[ -n "$amazon_client" ] && killall -9 AlexaRequest
/usr/bin/AlexaRequest &
