#/bin/bash

# This script was written for MinGW + MSYS

# Compile jpegtran
cd ..
tar -xzf jpegsrc.v8a.tar.gz

cd jpeg-8a
env CFLAGS="-O3" ./configure --disable-shared
make
strip jpegtran.exe
cp -f jpegtran.exe ../windows

# Compile jhead
cd ..
tar -xzf jhead-2.90.tar.gz
cp -f windows/makefile jhead-2.90/makefile

cd jhead-2.90
env CC="gcc" make
strip jhead.exe
cp -f jhead.exe ../windows

# Clean up
cd ..
rm -rf jpeg-8a
rm -rf jhead-2.90
