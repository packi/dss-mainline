var dev = {
  initialize: function(dsid, shortaddress, zoneid) {
    print('initializing js device (temperature_sensor.js) ', dsid, ' ', shortaddress, ' ', zoneid);
    var self = this;
    self.dsid = dsid;
    self.temperature = 20;
    self.linkState = 'none';
    var tempPropName = '/sim/'+dsid+'/temperature';
    setProperty(tempPropName, self.temperature);
    setListener(tempPropName, function() {
                        var oldTemp= self.temperature;
                        self.temperature = getProperty(tempPropName);
                        if(oldTemp != self.temperature) {
                            dSLinkInterrupt(self.dsid);
                          }
                        }
    );
  },

  log: function(str) {
    print('temp_sensor: ', str);
  },

  dSLinkSend: function(value, flags) {
    this.log('in dSLinkSend with value ' + value + ' flags ' + flags);
    if(this.linkState == 'none') {
      if((value & 0x40) == 0x40) {
        var eventID = (value & 0x3F);
//        this.processEvent(eventID);
        return 0;
      } else {
        this.registerAddress = (value & 0x7F);
        if((value & 0x80) == 0x80) {
          this.linkState = 'write';
          this.log('new state: write');
          return 0;
        } else {
          this.linkState = 'read';
          this.log('new state: read');
          return 0;
        }
      }
    } else if(this.linkState == 'write') {
      var result = 0;
//      this.writeRegister(this.registerAddress, value);
      this.linkState = 'none';
      this.log('new state: none');
      return result;
    } else if(this.linkState == 'read') {
//      var result = this.readRegister(this.registerAddress);
      var result = this.temperature;
      this.linkState = 'none';
      this.log('new state: none');
      return result;
    }
  },

  getFunctionID: function() {
    return 0xbeef;
  },

  callScene: function(nr) {
    print('called scene ', nr);
  },

  saveScene: function(nr) {
    print('save scene ', nr);
  },

  undoScene: function(nr) {
    print('undo scene');
  },

  increaseValue: function() {
    print('increase value');
  },

  decreaseValue: function() {
    print('decrease value');
  },

  enable: function() {
    print('enable');
  },

  disable: function() {
    print('disable');
  },

  getConsumption: function() {
    print('getConsumption');
    return 0;
  },

  startDim: function(direction) {
    print('startDim ', direction);
  },

  endDim: function() {
    print('endDim');
  },

  setValue: function(value) {
    print('setValue ', value);
  },

  getValue: function() {
    print('getValue');
    return 0;
  },

  setConfigParameter: function(name, value) {
    print('setConfigParameter ', name, '=', value);
  },

  getConfigParameter: function(name) {
    print('getConfigParameter ', name);
  }

  
};
dev;
