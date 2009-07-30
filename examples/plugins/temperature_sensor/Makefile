

all:	temperature_sensor.so install

temperature_sensor.so2:	temperature_sensor.o
	#g++ -lPocoNet -lpthread -shared -Wl,-soname,temperature_sensor.so -o temperature_sensor.so *.o
	g++ -shared -o temperature_sensor.so *.o
	

temperature_sensor.so:	temperature_sensor.cpp ../../../core/sim/plugin/pluginbase.cpp ../../../core/sim/plugin/pluginmedia.cpp ../../../core/sim/include/dsid_plugin.h
	g++ -g3 -O0 -Wall -fPIC -shared temperature_sensor.cpp ../../../core/sim/plugin/pluginbase.cpp ../../../core/sim/plugin/pluginmedia.cpp -o temperature_sensor.so

install:	temperature_sensor.so
	cp temperature_sensor.so ../../../data/plugins/
