

all:	webserver_plugin_ds485client.so install

webserver_plugin_ds485client.so2:	webserver_plugin_ds485client.o
		g++ -shared -o webserver_plugin_ds485client.so *.o
		

webserver_plugin_ds485client.so:	webserver_plugin_ds485client.cpp ../../../core/web/plugin/webserver_plugin_ds485client.h
			g++ -I../../../ -g3 -O0 -Wall -fPIC -shared webserver_plugin_ds485client.cpp -o webserver_plugin_ds485client.so

install:	webserver_plugin_ds485client.so
		cp webserver_plugin_ds485client.so ../../../data/webplugins/
