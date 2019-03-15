#!/bin/sh
curl --connect-timeout 3 --max-time 25 -v -F 'files=@./bin/panther/combined-uzImage-ubi-overlay' http://10.10.10.254/lazy.htm
curl --connect-timeout 3 --max-time 7 -i -v --user 'admin:admin' http://10.10.10.254/cli.cgi?cmd=http%20largepost
curl --connect-timeout 3 --max-time 25 -i -v --user 'admin:admin' -F 'filetype=1' -F 'files=@./bin/panther/combined-uzImage-ubi-overlay' http://10.10.10.254/upgrade.cgi
