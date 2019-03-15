#!/bin/sh
cp -Rp src msign-1.0
cp -f msign-1.0/CMakeLists.txt.host msign-1.0/CMakeLists.txt
tar zcf msign-1.0.tar.gz msign-1.0
rm -rf msign-1.0
