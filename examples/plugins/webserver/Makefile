

all:	webserver_plugin.so install

webserver_plugin.so2:	webserver_plugin.o
		g++ -shared -o webserver_plugin.so *.o
		

webserver_plugin.so:	webserver_plugin.cpp ../../../core/web/plugin/webserver_plugin.h
			g++ -g3 -O0 -Wall -fPIC -shared webserver_plugin.cpp -o webserver_plugin.so

install:	webserver_plugin.so
		cp webserver_plugin.so ../../../data/webplugins/
