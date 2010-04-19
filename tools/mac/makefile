#--------------------------------
# jhead makefile for Mac OS X
#--------------------------------
OBJ=.
SRC=.
CFLAGS= -O3 -Wall -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc
LDFLAGS= -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc

all: jhead

objs = $(OBJ)/jhead.o $(OBJ)/jpgfile.o $(OBJ)/paths.o \
	$(OBJ)/exif.o $(OBJ)/iptc.o $(OBJ)/gpsinfo.o $(OBJ)/makernote.o

$(OBJ)/%.o:$(SRC)/%.c
	${CC} $(CFLAGS) -c $< -o $@

jhead: $(objs) jhead.h
	${CC} -o jhead $(objs) $(LDFLAGS) -lm

clean:
	rm -f $(objs) jhead

install:
	cp jhead ${DESTDIR}/usr/bin/
