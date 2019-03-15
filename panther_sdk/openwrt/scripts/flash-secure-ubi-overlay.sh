#!/bin/sh
curl --connect-timeout 3 --max-time 25 -v -F 'files=@./bin/panther/secure-combined-uzImage-ubi-overlay' http://192.168.0.1/lazy.htm
curl --connect-timeout 3 --max-time 25 -i -v --user 'admin:admin' -F 'filetype=1' -F 'files=@./bin/panther/secure-combined-uzImage-ubi-overlay' http://192.168.0.1/upgrade.cgi
