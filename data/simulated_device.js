var dev = {
  initialize: function(dsid, shortaddress, zoneid) {
    print('initializing js device', dsid, ' ', shortaddress, ' ', zoneid);
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

  getFunctionID: function() {
    print('getFunctionID');
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
