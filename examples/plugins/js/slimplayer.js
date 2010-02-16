
function SlimPlayer() {
  var result = new MediaDSID();
  result.remotePort = 9090;
  result.remoteHost = "127.0.0.1";
  result.playerMACHeader = "00:00:00:00:00:00";
  result.defaultVolume = -1;

  result.sendCommand = function(command) {
    print("before sending: " + command, " to ", result.remoteHost, ":", result.remotePort);
    TcpSocket.sendTo(result.remoteHost, result.remotePort, result.playerMACHeader + " " + command + "\n",
      function(success) {
        if(success) {
          print("sending ", command, " success");
        } else {
          print("*** FAILED: sending ", command);
        }
      }
    );
    print("done sending");
  };

  result.onPowerOn = function() {
    result.sendCommand("power 1");
    if(result.defaultVolume != -1) {
      result.sendCommand("mixer volume " + result.defaultVolume);
    }
    result.sendCommand("play");
  };

  result.onPowerOff = function() {
    result.sendCommand("power 0");
  };

  result.onDeepOff = function() {
    result.sendCommand("power 0");
  };

  result.onNextSong = function() {
    if(result.lastWasOff()) {
      result.powerOn();
    }
    result.sendCommand("playlist index +1");
  };

  result.onPreviousSong = function() {
    if(result.lastWasOff()) {
      result.powerOn();
    }
    result.sendCommand("playlist index -1");
  };

  result.onVolumeUp = function() {
    result.sendCommand("mixer volume +5");
  };

  result.onVolumeDown = function() {
    result.sendCommand("mixer volume -6\r\nmixer volume +1");
  };

  result.onSetConfigParameter = function(name, value) {
    if(name == "port") {
      result.remotePort = parseInt(value);
    } else if(name == "host") {
      result.remoteHost = value;
    } else if(name == "playermac") {
      print("config-param: ", value);
      result.playerMACHeader = value.replace(/:/g, "%3A");
      print("after: ", result.playerMACHeader);
    } else if(name == "volume") {
      result.defaultVolume = value;
    }
  };

  return result;
}

new SlimPlayer();
