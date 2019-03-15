#!/bin/sh
#################
case "$1" in
        "de-DE")
        uci set xzxwifiaudio.config.amazon_language="de-DE" && uci commit
        ;;
        "en-US")
        uci set xzxwifiaudio.config.amazon_language="en-US" && uci commit
        ;;
		"en-GB")
        uci set xzxwifiaudio.config.amazon_language="en-GB" && uci commit
        ;;
        *)
        echo "not support this language"
        ;;
esac
