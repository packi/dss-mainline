
function VLC() {
  var result = new MediaDSID();
  result.remotePort = 4212;
  result.remoteHost = "127.0.0.1";

  result.sendCommand = function(command) {
    print("before sending: " + command, " to ", result.remoteHost, ":", result.remotePort);
    TcpSocket.sendTo(result.remoteHost, result.remotePort, command + "\r\n", // logout\n
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
    result.sendCommand("pause");
  };

  result.onPowerOff = function() {
    result.sendCommand("pause");
  };

  result.onDeepOff = function() {
    result.sendCommand("stop");
  };

  result.onNextSong = function() {
    if(result.lastWasOff()) {
      result.powerOn();
    }
    result.sendCommand("next");
  };

  result.onPreviousSong = function() {
    if(result.lastWasOff()) {
      result.powerOn();
    }
    result.sendCommand("prev");
  };

  result.onVolumeUp = function() {
    result.sendCommand("volup");
  };

  result.onVolumeDown = function() {
    result.sendCommand("voldown");
  };

  result.onSetConfigParameter = function(name, value) {
    if(name == "port") {
      result.remotePort = parseInt(value);
    } else if(name == "host") {
      result.remoteHost = value;
    }
  };

  return result;
}

new VLC();