var LOGFILE_NAME = "ping.log"

var l = new Logger(LOGFILE_NAME);

var repeated = 0;
var session = getProperty("/system/js/ping/session");
session++;

setProperty("/system/js/ping/session", session);

function pingResultHandler(f, shortAddr) {

  if (session != getProperty("/system/js/ping/session")) {
    return false;
  }

  if (f != null) {
    if (f.payload.length == 1) {
      if (f.payload[0] < 0) {
        l.logln("dSM returned error code: " + f.payload[0]);
        return false;
      }

      if (f.payload[0] != 2) {
        // just return true until we get the data we need
        return true;
      }
    }
    
    var deviceShortAddr = f.payload[1];
    if (deviceShortAddr != shortAddr) {
      // ignore frames that are not for us
      return true;
    }
        
    var device = getDevices().byShortAddress(f.source, f.payload[1]);

    l.logln("Ping response from " + device.dsid + " " + device.name + 
            " HK: " + f.payload[2] + " RK: " + f.payload[3]);

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
  if (session != getProperty("/system/js/ping/session")) {
    return;
  }

  var frame = new DS485Frame();
  frame.functionID = 0x9f; // FunctionDeviceGetTransmissionQuality
  frame.destination = device.circuitID;
  frame.broadcast = false;
  frame.payload.push(device.shortAddress);
  DS485.sendFrame(frame, function(frame) { 
                           return pingResultHandler(frame, device.shortAddress); 
                         } , 5000);
}

/*
    Parameters:
        dsid    -   what device to ping
        delay   -   delay between pings
        repeat  -   how often to repeat
        count   -   how many pings per cycle
 */

function pingDelayHandler(ids) {
  if (session != getProperty("/system/js/ping/session")) {
    return;
  }

  for (i = 0; i < ids.length; i++)
  {
    // split may return an empty dsid after comma
    if (ids[i].length <= 0) {
      continue;
    }
    
    var device = getDevices().byDSID(ids[i]);

    if (device === null) {
      l.logln("Device with dsid: " + ids[i] + " not found");
      continue;
    }
    
    l.logln("Pinging device with dsid: " + ids[i] + " " + device.name);

    for (p = 0; p < raisedEvent.parameter.count; p++) {
      ping(device);
    }
  } // loop on dsid
}

function timedPing() {
  if (session != getProperty("/system/js/ping/session")) {
    l.logln("Aborting old ping session.");
    return;
  }

  if (getProperty("/system/js/ping/active") === true) {
    l.logln("A ping round is currently active, rescheduling our session...");
    setTimeout(3*1000, timedPing);
    return;
  }

  setProperty("/system/js/ping/active", true);

  var ids = raisedEvent.parameter.dsid.split(",");
  if (ids.length <= 0) {
    l.logln("Error: no dsid's to ping!");
    return;
  }

  if ((raisedEvent.parameter.repeat > 0) && (raisedEvent.parameter.delay > 0)) {
    pingDelayHandler(ids);
    if ((repeated < raisedEvent.parameter.repeat) && 
        (session == getProperty("/system/js/ping/session"))) {
        setProperty("/system/js/ping/active", false); 
        setTimeout(parseInt(raisedEvent.parameter.delay) * 1000 /* msec */, timedPing);
    }
    repeated++;
  }
  else {
    pingDelayHandler(ids);
    setProperty("/system/js/ping/active", false); 
  }
}

keepContext();

timedPing(); 

