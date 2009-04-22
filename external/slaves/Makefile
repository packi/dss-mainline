

all:	vlc_remote.so install

vlc_remote.so2:	vlc_remote.o
	#g++ -lPocoNet -lpthread -shared -Wl,-soname,vlc_remote.so -o vlc_remote.so *.o
	g++ -shared -o vlc_remote.so *.o
	

vlc_remote.so:	vlc_remote.cpp
	g++ -g3 -O0 -Wall -fPIC -shared vlc_remote.cpp -o vlc_remote.so

install:	vlc_remote.so
	cp vlc_remote.so ../../../data/plugins/
