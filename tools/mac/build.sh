#/bin/bash

# Compile jpegtran
cd ..
tar -xzf jpegsrc.v8a.tar.gz

cd jpeg-8a
env CFLAGS="-O3 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc" LDFLAGS="-arch i386 -arch ppc" ./configure --disable-shared --disable-dependency-tracking
make
strip jpegtran
cp -f jpegtran ../mac

# Compile jhead
cd ..
tar -xzf jhead-2.90.tar.gz
cp -f mac/makefile jhead-2.90/makefile

cd jhead-2.90
make
strip jhead
cp -f jhead ../mac

# Clean up
cd ..
rm -rf jpeg-8a
rm -rf jhead-2.90
