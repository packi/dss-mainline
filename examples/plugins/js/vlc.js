
function VLC() {
  var result = new MediaDSID();
  result.remotePort = 4212;
  result.remoteHost = "localhost";

  result.sendCommand = function(command) {
    print("before sending: " + command, " to ", result.remoteHost, ":", result.remotePort);
    var socket = new TcpSocket();
    function onConnect(state) {
      if(state) {
        print("Connected");
        socket.send(command + '\r\n', onWritten);
      } else {
        print("Connection failed");
      }
    }

    function onWritten(numBytes) {
      if(numBytes > 0) {
        print('Sent request');
        readLine();
      } else {
        print('Write failed');
      }
    }

    function readLine() {
      var line = "";

      var doRead = function() {
        socket.receive(1,
          function(data) {
            if(data.length == 1) {
              if(data == "\n") {
                print('got line: ' + line);
                socket.close();
              } else {
                line += data;
                doRead();
              }
            } else {
              print('invalid data received "' + data + '"');
            }
          }
        );
      }

      doRead();
    }

    socket.connect('localhost', 4212, onConnect);
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