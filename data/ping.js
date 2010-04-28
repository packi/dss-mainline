var LOGFILE_NAME = "ping.log"

var l = new Logger(LOGFILE_NAME);

var repeated = 0;

function pingResultHandler(f) {
  l.logln("Callback receivd, frame is: " + f);

  if (f != null) {
    l.logln("Function id:    " + f.functionID);
    l.logln("Payload length: " + f.payload.length);
    if (f.payload.length == 1) {
      if (f.payload[0] < 0) {
        l.logln("dSM returned error code: " + f.payload[0]);
        return false;
      }

      if (f.payload[0] != 2) {
        l.logln("We need to get yet another frame");
        // packi said - just return true until we get the data we need
        return true;
      }
    }
    
    l.logln("PING RESPONSE from " + f.source);
        
    var device = getDevices().byShortAddress(f.source, f.payload[1]);

    l.logln("dsid:           " + device.dsid);
    l.logln("device name:    " + device.name);
    l.logln("response index: " + f.payload[0]);
    l.logln("device address: " + f.payload[1]);
    l.logln("quality HK:     " + f.payload[2]);
    l.logln("quality RK:     " + f.payload[3]);
    l.logln();

    var evt = new event("ping_result", { "send": f.payload[2], 
                                         "receive": f.payload[3],
                                         "dsid": device.dsid,
                                         "name": device.name,
                                         "logfile": LOGFILE_NAME,
                                         "timestamp": new Date(Date.now()).toGMTString()});
    evt.raise();
  } 
 
  return false;
}

function ping(device) {
  l.logln("Sending frame\n");
  var frame = new DS485Frame();
  frame.functionID = 0x9f; // FunctionDeviceGetTransmissionQuality
  l.logln("Destination: " + device.circuitID);
  frame.destination = device.circuitID;
  frame.broadcast = false;
  frame.payload.push(device.shortAddress);
  DS485.sendFrame(frame, pingResultHandler, 5000);
}

/*
    Parameters:
        dsid    -   what device to ping
        delay   -   delay between pings
        repeat  -   how often to repeat
        count   -   how many pings per cycle
 */

function pingDelayHandler(ids) {
  l.logln("Ping delay handler, ids length: " + ids.length);
  for (i = 0; i < ids.length; i++)
  {
    l.logln("Processing dsid: " + ids[i]);
    var device = getDevices().byDSID(ids[i]);
    l.logln("Circuit id: " + device.circuitID);
    l.logln("Pinging device...");
    for (p = 0; p < raisedEvent.parameter.count; p++) {
      ping(device);
    }
  } // loop on dsid
}

function timedPing() {
  l.logln("Entering function timedPing, repeated value: " + repeated);
  var ids = raisedEvent.parameter.dsid.split(",");
  if (ids.length <= 0) {
    l.logln("Error: no dsid's to ping!");
    return;
  }

  if ((raisedEvent.parameter.repeat > 0) && (raisedEvent.parameter.delay > 0)) {
    pingDelayHandler(ids);
    if (repeated < raisedEvent.parameter.repeat) {
        setTimeout(raisedEvent.parameter.delay, timedPing);
    }
    repeated++;
  }
  else {
    pingDelayHandler(ids);
  }
}

keepContext();

timedPing(); 



