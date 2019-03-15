#!/bin/sh
SPEECH_MODE()
{
	cdb set alexa_nocheck '1'
	cdb set Speech_recognition '1'
	#uci set xzxwifiaudio.config.speech_recognition=1 && uci commit
	killall -9 AlexaRequest;
	uartdfifo.sh VoiceInteraction
	setLanguage.sh CN
}

IFLYTELK_MODE()
{
	cdb set alexa_nocheck '1'
	cdb set Speech_recognition '2'
#	uci set xzxwifiaudio.config.speech_recognition=2 && uci commit
	killall -9 AlexaRequest;
	uartdfifo.sh VoiceInteraction
	setLanguage.sh CN
}

ALEXA_MODE()
{
	cdb set alexa_nocheck '0'
	cdb set Speech_recognition '3'
#	uci set xzxwifiaudio.config.speech_recognition=3 && uci commit
	uartdfifo.sh VoiceInteraction
	killall -9 AlexaRequest; AlexaRequest &
	setLanguage.sh EN
}

TURING_MODE()
{
	cdb set alexa_nocheck '1'
	cdb set Speech_recognition '4'
#	uci set xzxwifiaudio.config.speech_recognition=3 && uci commit
	killall -9 AlexaRequest
	uartdfifo.sh VoiceInteraction
	setLanguage.sh CN
}

#Speech_recognition: speech is 1,iflytek is 2,alexa is 3
#alexa_nocheck: 1.Alexa is daemon
case "$1" in
	"Alexa")
	     echo "Alexa Mode..."
		 ALEXA_MODE
	;;
	"iflytek")
	     echo "iflytek Mode..."
		 IFLYTELK_MODE
	;;
	"speech")
	     echo "speech Mode..."
		 SPEECH_MODE
	;;
	"turing")
	     echo "turing Mode..."
		 TURING_MODE
	;;
	*)
		echo "Unsupported parameter: $1"
	;;
esac
cdb commit
