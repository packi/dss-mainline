

all:	vlc_remote.so install

vlc_remote.so:	vlc_remote.o
	g++ -shared -Wl,-soname,vlc_remote.so -o vlc_remote.so *.o

vlc_remote.o:	vlc_remote.cpp
	g++ -Wall -fPIC -c vlc_remote.cpp

install:	vlc_remote.so
	cp vlc_remote.so ../../../data/plugins/