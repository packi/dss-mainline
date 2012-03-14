
if(raisedEvent.name == 'model_ready') {
  var s = subscription("temperature_interrupt", "javascript", { 'filename': 'data/handle_temperature_sensor_events.js' } );
  s.subscribe();
  var sensors = getDevices().byFunctionID(0xbeef);
  print("found ", sensors.length(), " sensors");

  var setup = function(dev) {
    print("setting-up ", dev.dsid, "...");
    var temperature = dev.dSLinkSend(0x55, // value
                                     true, // lastByte
                                     false); // writeOnly
    print("temperature is ", temperature);
    var propBasePath = '/apartment/zones/zone0/' + dev.dsid;
    setProperty(propBasePath + '/interrupt/mode', 'raise_event');
    setProperty(propBasePath + '/interrupt/event/name', 'temperature_interrupt');    
  }

  // call check on each sensor
  sensors.perform(setup);
} else if(raisedEvent.name == 'temperature_interrupt') {
  var readRegister = function(dev, number) {
    dev.dSLinkSend(number, false, false);
    return dev.dSLinkSend(number, true, false);
  };

  var dsid = raisedEvent.parameter.device;
  var dev = getDevices().byDSID(dsid);
  var propBasePath = '/apartment/zones/zone' + dev.zoneID + '/' + dsid;
  var newTemp = readRegister(dev, 0);
  log("newTemp: ", newTemp);

  setProperty(propBasePath + 'temperature', newTemp);
}
