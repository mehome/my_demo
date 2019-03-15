#!/bin/sh
curl -v -F 'files=@./bin/panther/combined-uzImage-ubi' http://192.168.0.1/lazy.htm
curl -i -v --user 'admin:admin' -F 'filetype=1' -F 'files=@./bin/panther/combined-uzImage-ubi' http://192.168.0.1/upgrade.cgi
