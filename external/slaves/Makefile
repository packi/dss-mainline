

all:	slim_slave.so install

slim_slave.so2:	slim_slave.o
	#g++ -lPocoNet -lpthread -shared -Wl,-soname,slim_slave.so -o slim_slave.so *.o
	g++ -shared -o slim_slave.so *.o
	

slim_slave.so:	slim_slave.cpp
	g++ -g3 -O0 -Wall -fPIC -shared slim_slave.cpp ../../core/sim/plugin/pluginbase.cpp ../../core/sim/plugin/pluginmedia.cpp -o slim_slave.so

install:	slim_slave.so
	cp slim_slave.so ../../data/plugins/
