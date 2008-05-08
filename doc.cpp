/**
\mainpage digitalStrom Server - Model API

The following section will show the use of the dSS Model API. One of the most important element of the API is the Set. 
 
Every DeviceContainer can deliver its Devices as a Set. A Set can be manipulated by adding and removing devices as well as being combined with other sets. Since a Set implements IDeviceInterface it can receive all commands like TurnOn() like any other Device.

\section working_with_sets Working with sets

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
