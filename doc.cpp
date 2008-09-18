/**
\mainpage digitalStrom Server - Model API

\section overview Overview

The dSS consists of several modules as shown in the following image:

\image html block.png

At its heart is the DataModel. It provides the API that allows a developer manipulate as well as rearange devices, rooms and groups.
Built on that foundation there is a SOAP interface to make the API accessible to the outside world. In addition to the SOAP interface there is a JSON interface that interacts with JavaScript running in a webbrowser.

The DS485Proxy is responsible of controlling the physical and simulated devices. It converts abstract commands from the DataModel an converts them to one or several frames which are then sent to the DS485 bus.

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

\subsection config_apartment Apartment

The Apartment config resides in data/apartment.xml. It contains everything that won't be stored in the dSMs such as device names, event subscriptions

\verbatim
<?xml version="1.0"?>
<config version="1">
 <devices>
   <device dsid="4290772993">
     <name>Schalter 1</name>
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

All items are bound to a dsid so if a user moves a bulb from one room to another it will still be named "Bulb 1".

\subsection config_sim Simulation

The structure inside the simulated dSM is contained in data/sim.xml.

\verbatim
<?xml version="1.0"?>
<modulator busid="70" dsid="10">
  <room id="0"> <!-- id can be omitted, in fact room may be omitted if there's only one room -->
    <device dsid="1" busid="1" type="standard.switch" />
    <device dsid="2" busid="2" type="example.vlc_remote" />
    <device dsid="4" busid="4" type="standard.simple"/>
    <group id="1">
      <device busid="4" />
    </group>
    <group id="4">
      <device busid="2" />
    </group>
  </room>
  <room id="1">
    <device dsid="3" busid="3" type="standard.switch"/>
    <device dsid="5" busid="5" type="standard.simple" />
    <group id="1">
      <device busid="5" />
    </group>
  </room>  
</modulator>
\endverbatim

In the modulator tag the modulator's busid (as in DS485 bus address) as well as it's dsid get specified. The dsid as it's written there is actually adjusted in the loader to be a "simulation" address and thus or'ed with 0xFFC00000. This goes for all dsids in this config file.

\section sets Sets

The following section will show the use of the dSS Model API. One of the most important element of the API is the Set. 
 
Every DeviceContainer can deliver its Devices as a Set. A Set can be manipulated by adding and removing devices as well as being combined with other sets. Since a Set implements IDeviceInterface it can receive all commands like TurnOn() like any other Device.

\subsection working_with_sets Working with sets

\code
Apartment apt;

Device& dev1 = apt.AllocateDevice(1);
Device& dev2 = apt.AllocateDevice(2);
Device& dev3 = apt.AllocateDevice(3);
Device& dev4 = apt.AllocateDevice(4);

// Get a set containing all devices
Set allDevices = apt.GetDevices();

// Create a set containing only dev1
Set setdev1 = Set(dev1);

// Create a set of all devices excluding dev1
Set allMinusDev1 = allDevices.Remove(setdev1);

// Create a set of all device by combining the sets created above
Set allRecombined = allMinusDev1.Combine(setdev1);

// Turn on all devices
allRecombined.TurnOn();

// Turn on all lights
Set allLights = allDevices.GetByGroup("yellow");
allLights.TurnOn();

\endcode

*/
