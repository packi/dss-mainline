print("in data/read_teperature.js");

// create a set containing all temperature sensors

var sensors = getDevices().byFunctionID(0xbeef);
print("found ", sensors.length(), " sensors");

var check = function(dev) {
  print("checking ", dev.dsid, "...");
  var temperature = dev.dSLinkSend(0x55, // value
                                   true, // lastByte
                                   false); // writeOnly
  print("temperature is ", temperature);
}

// call check on each sensor
sensors.perform(check);
print("done checking.");

// re-schedule
var evt = event("read_temperature", { 'time': '+10'} );
evt.raise();
