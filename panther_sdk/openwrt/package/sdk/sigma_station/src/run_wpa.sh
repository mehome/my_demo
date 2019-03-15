#!/bin/sh

wpa_supplicant -Dnl80211 -iwlan0 -c./wpa.conf&
