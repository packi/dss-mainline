INSTALL_TARGET=../../../data/webplugins/


all:	iPhone_plugin.so install

iPhone_plugin.so2:	iPhone_plugin.o
	g++ -shared -o iPhone_plugin.so *.o

iPhone_plugin.so:	iPhone_plugin.cpp ../../../core/web/plugin/webserver_plugin.h
	g++ -I../../../ -g3 -O0 -Wall -fPIC -shared iPhone_plugin.cpp -o iPhone_plugin.so

install:	iPhone_plugin.so
	mkdir -p $(INSTALL_TARGET)
	cp iPhone_plugin.so $(INSTALL_TARGET)
