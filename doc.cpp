using namespace dss;

/**
\mainpage digitalStrom Server - Model API

\section overview Overview

The dSS consists of several modules as shown in the following image:

\image html block.png "System overview"
\image latex block.eps "System overview" height=10cm

The center piece of the dSS is its DataModel. It provides the API that allows a developer manipulate as well as rearrange devices, zones and groups.
Built on that foundation there is a SOAP interface to make the API accessible to the outside world. In addition to the SOAP interface there is a JSON interface that interacts with JavaScript running in a webbrowser.

The DS485Proxy is responsible of controlling the physical and simulated devices. It converts abstract commands from the DataModel an converts them to one or several DS485 frames. These frames will then be sent either to a simulated dSM or put on the wire depending on the frame's destination.

The simulated dSM may be extended by plugins. An example is a simulated dSID that controlls a VLC (mediaplayer) instance remotely using its telnet interface.

\section config Configuration

There are several configuration files which define the behaviour of the dSS and its simulation:

\subsection config_general General

The main config file is located at data/config.xml. 

\verbatim
<?xml version="1.0" encoding="utf-8"?>
<config version="1">
  <item><name>webserverport</name><value>8080</value></item>
  <item><name>webserverroot</name><value>data/webroot/</value></item>
  <item><name>apartment_config</name><value>data/apartment.xml</value></item>
</config>
\endverbatim

There are three properties (subject to change) that may be overwritten by the config file. The webserverport specifies which port the webserver should listen on. Note that in a unix environment you must be root to open a listener on a port lower than 1025.
The webserverroot specifies the webservers root directory. 
apartment_config tells the DataModel where to find and store the apartments configuration.

\subsection config_apartment Apartment

The Apartment config resides in data/apartment.xml. It contains everything that won't be stored in the dSMs such as device names, event subscriptions

\verbatim
<?xml version="1.0"?>
<config version="1">
 <devices>
   <device dsid="4290772993">
     <name>Switch 1</name>
   </device>
   <device dsid="4290772994">
     <name>VLC</name>
   </device>
   <device dsid="4290772995">
     <name>Another device</name>
     <event>2007</event>
   </device>
 </devices>
</config>
\endverbatim

All items are bound to a dsid so if a user moves a bulb from one zone to another it will still be named "Bulb 1". The event tag specifies which event should be raised if a button press is detected from the given dSID.

\subsection config_sim Simulation

The structure inside the simulated dSM is contained in data/sim.xml.

\verbatim
<?xml version="1.0"?>
<modulator busid="70" dsid="10">
  <zone id="0"> <!-- id can be omitted, in fact zone may be omitted if there's only one zone -->
    <device dsid="1" busid="1" type="standard.switch" />
    <device dsid="2" busid="2" type="example.vlc_remote" />
    <device dsid="4" busid="4" type="standard.simple"/>
    <group id="1">
      <device busid="4" />
    </group>
    <group id="4">
      <device busid="2" />
    </group>
  </zone>
  <zone id="1">
    <device dsid="3" busid="3" type="standard.switch"/>
    <device dsid="5" busid="5" type="standard.simple" />
    <group id="1">
      <device busid="5" />
    </group>
  </zone>  
</modulator>
\endverbatim

In the modulator tag the modulator's busid (as in DS485 bus address) as well as it's dsid get specified. The dsid as it's written there is actually adjusted in the loader to be a "simulation" address and thus or'ed with 0xFFC00000. This goes for all dsids in this config file.

The device-type defines what object gets instanciated by the simulated dSM. There are two built-in dSIDs "standard.simple" a device that behaves like a light bulb and "standard.switch" which simulates a 9 button switch.
If the type of a device is not specified it defaults to "standard.simple". 

It's possible to write custom dSIDs with the use of the plugin API. The standard location of the plugins is at "data/plugins". For each file there a couple of checks (such as comparing the API version) are performed to ensure the stability of the dSS. The plugin is then asked for it's name and registered in the plugin factory, ready to use.

To form groups inside a zone a group tag containing references to the devices must be inserted. At the moment it's not possible to define devices inside a group tag.

\section sets Sets

The following section will show the use of the dSS Model API. One of the most important element of the API is the Set. 
 
Every \a dss::deviceContainer can deliver its \a dss::devices as a \a dss::set. A Set can be manipulated by adding and removing devices as well as being combined with other sets. Since a Set implements \a dss::iDeviceInterface it can receive commands like TurnOn() as it would be a single Device.

\subsection working_with_sets Working with sets

\code
Apartment apt;

Device& dev1 = apt.allocateDevice(1);
Device& dev2 = apt.allocateDevice(2);
Device& dev3 = apt.allocateDevice(3);
Device& dev4 = apt.allocateDevice(4);

// Get a set containing all devices
Set allDevices = apt.getDevices();

// Create a set containing only dev1
Set setdev1 = Set(dev1);

// Create a set of all devices excluding dev1
Set allMinusDev1 = allDevices.remove(setdev1);

// Create a set of all device by combining the sets created above
Set allRecombined = allMinusDev1.combine(setdev1);

// Turn on all devices
allRecombined.turnOn();

// Turn on all lights
Set allLights = allDevices.getByGroup("yellow");
allLights.turnOn();

\endcode

\section plugins Simulation plugins

Each plugin has to meet a certain API. This API is located at "core/sim/include/dsid_plugin.h". Each function has to be present in order to function correctly.

The first function to be called is:

\code
  int dsid_getversion();
\endcode

It has to return DSID_PLUGIN_API_VERSION as defined in dsid_plugin.h. This number will change with each major API change.
One of the next calls is to:
\code
  const char* dsid_get_plugin_name();
\endcode

This call should return a pointer to the plugins name. It has to be unique on the system. The prefered naming scheme is company.product if a plugin simulates the behaviour of a specific device.

If the plugin name comes up in sim.xml an instance of the plugin is created by calling:
\code
  int dsid_create_instance();
\endcode

The result of dsid_create_instance is a handle that identifies the newly created instance. This is done to allow multiple instances of a simulated dSID.

After allocating an the dSS needs a way to talk to the instance. This is done through a structure wich contains several function pointers. This structure is returned by:
\code
  struct dsid_interface* dsid_get_interface();
\endcode

The structure looks like that:
\code
  struct dsid_interface {
    double (*get_value)(int _handle, int _parameterNumber);
    void (*set_value)(int _handle, int _parameterNumber, double _value);
    void (*increase_value)(int _handle, int _parameterNumber);
    void (*decrease_value)(int _handle, int _parameterNumber);
    void (*call_scene)(int _handle, int _sceneNr);
    void (*save_scene)(int _handle, int _sceneNr);
    void (*undo_scene)(int _handle, int _sceneNr);
    void (*enable)(int _handle);
    void (*disable)(int _handle);
    void (*get_group)(int _handle);

    void (*start_dim)(int _handle, int _directionUp, int _parameterNumber);
    void (*end_dim)(int _handle, int _parameterNumber);
    int (*get_group_id)(int _handle);
    int (*get_function_id)(int _handle);

    const char* (*get_parameter_name)(int _handle, int _parameterNumber);
  };
\endcode

The first parameter to each function is the instance identifier or handle to the instance.

The plugin has to be compiled as a shared-object. Here is an example of a makefile:

\verbatim
all:	vlc_remote.so install

vlc_remote.so:	vlc_remote.o
	g++ -lPocoNet -shared -Wl,-soname,vlc_remote.so -o vlc_remote.so *.o

vlc_remote.o:	vlc_remote.cpp
	g++ -g3 -O0 -Wall -fPIC -c vlc_remote.cpp

install:	vlc_remote.so
	cp vlc_remote.so ../../../data/plugins/
\endverbatim

Ensure that -fPIC is present, else it won't generate position independent code and is thus unsuitable for a shared-object.

\subsection plugin_example Example

The source of the dSS comes with an example implementation of a simulated dSID. This dSID simulates a mediaplayer device and controls a running VLC player. Its source can be found at "examples/plugins/vlc_remote". To try it out start a vlc instance with the following line:

\verbatim
vlc --rc-host localhost:4212
\endverbatim

*/
