var LOGFILE_NAME = "ping.log";
var l = new Logger(LOGFILE_NAME);

if (raisedEvent.name == "running") {
  setProperty("/system/js/settings/extendedPing/session", -1);
  setProperty("/system/js/settings/extendedPing/active", false);
  setProperty("/system/js/settings/extendedPing/logfile", LOGFILE_NAME);
  setProperty("/system/js/features/extendedPing", true);
  l.logln("Extended ping script initialized");
}

var repeated = 0;
var last_request = false;
var ids = [];
var session = getProperty("/system/js/settings/extendedPing/session");
session++;

setProperty("/system/js/settings/extendedPing/session", session);

function stats(session) {
  this.sentTotal = 0;
  this.callbacksTotal = 0;
  this.receivedTotal = 0;
  this.dsid = "";
  this.sendErr = [0,0,0,0,0,0,0];
  this.recvMax = undefined;
  this.recvMin = undefined;
  this.recvSum = 0;
  this.missedResponsesPercentage = 0;
  this.device = null;
  this.session = session;
  this.summary = function() {
    if (this.device === null) {
      log("Ping summary is missing device information, ignoring");
      return;
    }

    log("");
    log("Summary for device " + this.device.dsid + " " + this.device.name);
    log("---------------------------------------------------------");
    log("          Total requests sent: " + this.sentTotal);
    log("     Total responses received: " + this.receivedTotal);
    log("   Missed response percentage: " + 
            Math.round((100-(this.receivedTotal / this.sentTotal)*100)) + "%");
    log("");
    for (var i = 0; i < this.sendErr.length; i++) {
      log("                  HK Errors " + i +": " + this.sendErr[i]);
    }
    log("");
    log("    Worst received value (RK): " + ((this.recvMin === undefined) ? "none" : this.recvMin));
    log("     Best received value (RK): " + ((this.recvMax === undefined) ? "none" : this.recvMax));
    log(" Average received values (RK): " + (((this.receivedTotal == 0) || (this.recvSum == 0)) ? "none" : (this.recvSum / this.receivedTotal)));
    log("---------------------------------------------------------");
    log("");
  }
}

function devSt() {
  this.devices = [];
  this.getStats = function(dsid) {
    var found = -1;
    for (var i = 0; i < this.devices.length; i++) {
      if ((this.devices[i].device != null) && 
          (this.devices[i].device.dsid == dsid)) {
        found = i;
      }
    }

    if (found > -1) {
      return this.devices[found];
    } else {
      return null; 
    }
  }

  this.update = function(stats) {
    var found = -1;
    for (var i = 0; i < this.devices.length; i++) {
      if ((this.devices[i].device != null) && 
          (this.devices[i].device.dsid == stats.device.dsid)) {
        found = i;
      }
    }

    if (found > -1) {
      this.devices[found] = stats;
    } else {
      this.devices.push(stats);
    }
  }

  this.printAll = function() {
    for (var i = 0; i < this.devices.length; i++) {
      this.devices[i].summary();
    }
  }
}

var deviceStats = new devSt();

function log(logstring) {
  l.logln("SESSION " + session + " | " + logstring);
}

function pingResultHandler(f, shortAddr, dsid, name, index) {
  if (session != getProperty("/system/js/settings/extendedPing/session")) {
    log("This ping session became obsolete, aborting.");
    deviceStats.printAll();
    return false;
  }
    
  var s = deviceStats.getStats(dsid);
    
  if (f != null) {
    if (f.payload.length == 1) {
      if (f.payload[0] < 0) {
        log("dSM returned error code: " + f.payload[0]);
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

    s = deviceStats.getStats(device.dsid);
    if (s != null) {
      s.receivedTotal++;
      s.callbacksTotal++;

      if ((s.recvMax === undefined) || (f.payload[3] > s.recvMax)) {
        s.recvMax = f.payload[3];
      }

      if ((s.recvMin === undefined) || (f.payload[3] < s.recvMin)) {
        s.recvMin = f.payload[3];
      }

      s.recvSum = s.recvSum + f.payload[3];

      if (f.payload[2] < s.sendErr.length) {
        s.sendErr[f.payload[2]]++;
      }

      deviceStats.update(s);
    } else {
      log("Error: summary object not available!");
    }

    log(index + "Ping response from  dsid: " + device.dsid + " " 
        + device.name + " HK: " + f.payload[2] + " RK: " + f.payload[3]);

    var evt = new event("ping_result", { "send": f.payload[2], 
                                         "receive": f.payload[3],
                                         "dsid": device.dsid,
                                         "name": device.name,
                                         "logfile": LOGFILE_NAME,
                                         "reqindex" : index,
                                         "timestamp": new Date(Date.now())});
    evt.raise();
  } else {
    if (s === null) {
      log("Error: summary object not available!");
    } else {
      s.callbacksTotal++;
      deviceStats.update(s);
    }
    log("");
    log(index + "** NO RESPONSE from dsid: " + dsid + " ****** " + name); 
  }

  if ((last_request === true) && (s.callbacksTotal >= s.sentTotal)) {
    s.summary(dsid);
  }

  return false;
}

function ping(device, index) {
  if (session != getProperty("/system/js/settings/extendedPing/session")) {
    log("This ping session became obsolete, aborting.");
    deviceStats.printAll();
    return;
  }

  var s = deviceStats.getStats(device.dsid);
  if (s === null) {
    s = new stats(session);
    s.device = device;
    s.dsid = device.dsid;
  }

  s.sentTotal++;
  deviceStats.update(s);

  var frame = new DS485Frame();
  frame.functionID = 0x9f; // FunctionDeviceGetTransmissionQuality
  frame.destination = device.circuitID;
  frame.broadcast = false;
  frame.payload.push(device.shortAddress);
  DS485.sendFrame(frame, function(frame) { 
                           return pingResultHandler(frame, 
                                                    device.shortAddress, 
                                                    device.dsid,
                                                    device.name,
                                                    index); 
                         } , 5000);
}

/*
    Paurameters:
        dsid    -   what device to ping
        delay   -   delay between pings
        repeat  -   how often to repeat
*/

function pingDelayHandler() {
  var index = "#" + (repeated + 1) + " ";
 
  for (var i = 0; i < ids.length; i++)
  {
    if (session != getProperty("/system/js/settings/extendedPing/session")) {
        log("This ping session became obsolete, aborting.");
        deviceStats.printAll();
        return;
    }

    // split may return an empty dsid after comma
    if (ids[i].length <= 0) {
      continue;
    }
    
    var device = getDevices().byDSID(ids[i]);

    if (device === null) {
      log("Device with dsid: " + ids[i] + " not found");
      continue;
    }

    log(index + "Pinging device with dsid: " + ids[i] + " " + device.name);
    ping(device, index);
  } // loop on dsid
}

function timedPing() {
  if (session != getProperty("/system/js/settings/extendedPing/session")) {
    log("This ping session became obsolete, aborting.");
    deviceStats.printAll();
    return;
  }


  if (getProperty("/system/js/settings/extendedPing/active") === true) {
    log("Another ping round is currently active, rescheduling our session...");
    setTimeout(3*1000, timedPing);
    return;
  }

  setProperty("/system/js/settings/extendedPing/active", true);

  if ((raisedEvent.parameter.repeat > 0) && (raisedEvent.parameter.delay > 0)) {
    var one_more = false;
    if ((repeated < raisedEvent.parameter.repeat) && 
        (session == getProperty("/system/js/settings/extendedPing/session"))) {
      one_more = true;
    } else {
      last_request = true;
    }
    pingDelayHandler();
    if (one_more === true) {
        setTimeout(parseInt(raisedEvent.parameter.delay) * 1000 /* msec */, timedPing);
    }
    repeated++;
  } else {
    last_request = true;
    pingDelayHandler();
  }
    
  setProperty("/system/js/settings/extendedPing/active", false); 
}

if (raisedEvent.name == "ping") {
  keepContext();
  l.logln("");
  l.logln("Starting extended ping session " + session + 
          " ------------------------------------------------");
  log("Will ping following dsids:  " + raisedEvent.parameter.dsid);
  log("Number of pings per device: " + raisedEvent.parameter.repeat);
  raisedEvent.parameter.repeat--;
  log("Delay between repetitions:  " + raisedEvent.parameter.delay + 
      " sec.");
  last_request = false;

  ids = raisedEvent.parameter.dsid.split(",");
  if (ids.length <= 0) {
    log("Error: no dsid's to ping!");
  } else {
    timedPing(); 
  }
}
