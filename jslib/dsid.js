function DSID() {

  this.dsid = null;
  this.zoneID = null;
  this.busID = null;
  this.callScene = function(sceneID) {
    if(this.onCallScene != undefined) {
      this.onCallScene(sceneID);
    }
  };

  this.increaseValue = function() {
    if(this.onIncreaseValue != undefined) {
      this.onIncreaseValue();
    }
  };

  this.decreaseValue = function() {
    if(this.onDecreaseValue != undefined) {
      this.onDecreaseValue();
    }
  };

  this.setConfigParameter = function(name, value) {
    if(this.onSetConfigParameter != undefined) {
      this.onSetConfigParameter(name, value);
    }
  };

  return this;
}

function MediaDSID() {
  var result = new DSID();
  result.lastSceneID = Scene.Off;
  result.onCallScene = function(sceneID) {
    print("MediaDSID.onCallScene");
    // mask out local on/off
    var realScene = sceneID & 0x00ff;
    var keepScene = (realScene != Scene.Bell);
    if(realScene == Scene.DeepOff || realScene == Scene.Panic || realScene == Scene.Alarm) {
      result.deepOff();
    } else if(realScene == Scene.Off || realScene == Scene.Min || realScene == Scene.StandBy) {
      result.powerOff();
    } else if(realScene == Scene.Max) {
      result.powerOn();
    } else if(realScene == Scene.Bell) {
      setTimeout(0,
        function() {
          if(!result.lastWasOff()) {
            result.turnOff();
            setTimeout(3000, function() {
              result.turnOn();
            });
          }
        }
      );
    } else if(realScene == Scene.User1) {
      result.powerOn();
    } else if(realScene == Scene.User2) {
      if(result.lastSceneID == Scene.User3) {
        result.previousSong();
      } else {
        result.nextSong();
      }
    } else if(realScene == Scene.User3) {
      if(result.lastSceneID == Scene.User4) {
        result.previousSong();
      } else {
        result.nextSong();
      }
    } else if(realScene == Scene.User4) {
      if(result.lastSceneID == Scene.User2) {
        result.previousSong();
      } else {
        result.nextSong();
      }
    }
    if(keepScene) {
      result.lastSceneID = realScene;
    }
  };

  result.onIncreaseValue = function() {
    result.volumeUp();
  };

  result.onDecreaseValue = function() {
    result.volumeDown();
  };

  result.lastWasOff = function() {
   return   result.lastSceneID == Scene.DeepOff || result.lastSceneID == Scene.StandBy || result.lastSceneID == Scene.Alarm
         || result.lastSceneID == Scene.Off || result.lastSceneID == Scene.Min || result.lastSceneID == Scene.Panic;
  };

  result.powerOn = function() {
    if(result.onPowerOn != undefined) {
      result.onPowerOn();
    }
  };

  result.powerOff = function() {
    if(result.onPowerOff != undefined) {
      result.onPowerOff();
    }
  };

  result.nextSong = function() {
    if(result.onNextSong != undefined) {
      result.onNextSong();
    }
  };

  result.previousSong = function() {
    if(result.onPreviousSong != undefined) {
      result.onPreviousSong();
    }
  };

  result.turnOn = function() {
    if(result.onTurnOn != undefined) {
      result.onTurnOn();
    }
  };

  result.turnOff = function() {
    if(result.onTurnOff != undefined) {
      result.onTurnOff();
    }
  };

  result.deepOff = function() {
    if(result.onDeepOff != undefined) {
      result.onDeepOff();
    }
  };

  result.volumeUp = function() {
    if(result.onVolumeUp != undefined) {
      result.onVolumeUp();
    }
  };

  result.volumeDown = function() {
    if(result.onVolumeDown != undefined) {
      result.onVolumeDown();
    }
  };

  return result;
}