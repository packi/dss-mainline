/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

var DSS = Class.create({
});	

DSS.token = undefined,
DSS.endpoint = "/json/",

	DSS.sendSyncRequest = function(_uri, _parameter) {
		var responseObj;
		var req = new Ajax.Request(this.endpoint + _uri,
		{
			method: 'get',
			parameters: _parameter,
			asynchronous: false,
			onComplete: function(transport, json) {
			responseObj = transport.responseJSON;
			}
		});
		return responseObj;
	};
	
	DSS.sendRequest = function(_uri, _parameter) {
		var request = new Ajax.Request(DSS.endpoint + _uri,
		{
			method: 'get',
			parameters: _parameter
		});
	};
//});

function hasKey(_obj, _key) {
  return (typeof(_obj[_key]) != "undefined");
}

var Group = Class.create({
  initialize: function(_name, _id) {
    this.name = _name;
    this.id = _id;
  }
}); // Group

var System = Class.create({
  sendSyncRequest: function(_functionName, _parameter) {
      return DSS.sendSyncRequest("system/" + _functionName, _parameter);
    },

    version: function() {
      var respObj = this.sendSyncRequest("version", {});
      return respObj.message;
    }
}); // System

var Apartment = Class.create({
	initialize: function() {
		this.zones = [];
	},
	
	fetch: function() {
		var self = this;
		var structure = self.sendSyncRequest("getStructure", {});
		self.zones = [];
		structure.apartment.zones.each(function(zone) {
			var zoneObj = new Zone(zone.id, zone.name);
			self.zones.push(zoneObj);
			zone.devices.each(function(device) {
			var deviceObj = new Device(device.id, device.name);
				deviceObj.fid = device.fid;
				deviceObj.busID = device.busID;
				deviceObj.circuitID = device.circuitID;
				zoneObj.devices.push(deviceObj);
				deviceObj.on = device.on;
				deviceObj.hasSwitch = device.isSwitch;
				deviceObj.isPresent = device.isPresent;
			});
		});
	},
	
	sendRequest: function(_functionName, _parameter) {
		DSS.sendRequest("apartment/" + _functionName, _parameter);
	},
	
	sendSyncRequest: function(_functionName, _parameter) {
		return DSS.sendSyncRequest("apartment/" + _functionName, _parameter);
	},
	
	getParameterForDeviceCall: function(_group) {
		var parameter = {};
		if(Object.isNumber(_group)) {
			parameter['groupID'] = _group;
		} else if(Object.isString(_group)) {
			parameter['groupName'] = _group;
		} else if(!Object.isUndefined(_group) && _group !== null) {
			if(hasKey(_group, 'name')) {
				parameter['groupName'] = _group['name'];
			} else if(hasKey(_group, 'id')) {
				parameter['groupID'] = _group['id'];
			}
		}
		return parameter;
	},
	
	turnOn: function(_group) {
		this.sendRequest("turnOn", this.getParameterForDeviceCall(_group));
	},
	
	turnOff: function(_group) {
		this.sendRequest("turnOff", this.getParameterForDeviceCall(_group));
	},
	
	callScene: function(_sceneNr, _group) {
		var parameter = this.getParameterForDeviceCall(_group);
		parameter['sceneNr'] = _sceneNr;
		this.sendRequest("callScene", parameter);
	},
	
	saveScene: function(_sceneNr, _group) {
		var parameter = this.getParameterForDeviceCall(_group);
		parameter['sceneNr'] = _sceneNr;
		this.sendRequest("saveScene", parameter);
	},
	
	undoScene: function(_sceneNr, _group) {
		var parameter = this.getParameterForDeviceCall(_group);
		parameter['sceneNr'] = _sceneNr;
		this.sendRequest("undoScene", parameter);
	},
	increaseValue: function(_group) {
		var parameter = this.getParameterForDeviceCall(_group);
		this.sendRequest("increaseValue", parameter);
	},
	decreaseValue: function(_group) {
		var parameter = this.getParameterForDeviceCall(_group);
		this.sendRequest("decreaseValue", parameter);
	},
	
	enable: function(_group) {
		var parameter = this.getParameterForDeviceCall(_group);
		this.sendRequest("enable", parameter);
	},
	
	disable: function(_group) {
		var parameter = this.getParameterForDeviceCall(_group);
		this.sendRequest("disable", parameter);
	},
	
	startDim: function(_up, _group) {
		var parameter = this.getParameterForDeviceCall(_group);
		if(Object.isUndefined(_up) || _up === true) {
			parameter.up = true;
		}
		this.sendRequest("startDim", parameter);
	},
	
	endDim: function(_group) {
		var parameter = this.getParameterForDeviceCall(_group);
		this.sendRequest("endDim", parameter);
	},
	
	getPowerConsumption: function(_group) {
		var parameter = this.getParameterForDeviceCall(_group);
		var respObj = this.sendSyncRequest("getConsumption", parameter);
		return respObj.consumption;
	},
	
	getCircuits: function() {
		var respObj = this.sendSyncRequest("getCircuits", {});
		return respObj.result.circuits;
	}
}); // Apartment


var Zone = Class.create({
  initialize: function(_id, _name) {
    this.id = _id;
    this.name = _name;
    this.devices = [];
  },

  sendRequest: function(_functionName, _parameter) {
    DSS.sendRequest("zone/" + _functionName, _parameter);
  },

  sendSyncRequest: function(_functionName, _parameter) {
    return DSS.sendSyncRequest("zone/" + _functionName, _parameter);
  },

  getParameterForDeviceCall: function(_group) {
    var parameter = {};
    parameter['id'] = this.id;
    if(Object.isNumber(_group)) {
      parameter['groupID'] = _group;
    } else if(Object.isString(_group)) {
      parameter['groupName'] = _group;
    } else if(!Object.isUndefined(_group) && _group !== null) {
      if(hasKey(_group, 'name')) {
        parameter['groupName'] = _group['name'];
      } else if(hasKey(_group, 'id')) {
        parameter['groupID'] = _group['id'];
      }
    }
    return parameter;
  },

  turnOn: function(_group) {
    this.sendRequest("turnOn", this.getParameterForDeviceCall(_group));
  },

  turnOff: function(_group) {
    this.sendRequest("turnOff", this.getParameterForDeviceCall(_group));
  },

  callScene: function(_sceneNr, _group) {
    var parameter = this.getParameterForDeviceCall(_group);
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("callScene", parameter);
  },

  saveScene: function(_sceneNr, _group) {
    var parameter = this.getParameterForDeviceCall(_group);
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("saveScene", parameter);
  },

  undoScene: function(_sceneNr, _group) {
    var parameter = this.getParameterForDeviceCall(_group);
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("undoScene", parameter);
  },

  increaseValue: function(_group) {
    var parameter = this.getParameterForDeviceCall(_group);
    this.sendRequest("increaseValue", parameter);
  },

  decreaseValue: function(_group) {
    var parameter = this.getParameterForDeviceCall(_group);
    this.sendRequest("decreaseValue", parameter);
  },

  enable: function(_group) {
    var parameter = this.getParameterForDeviceCall(_group);
    this.sendRequest("enable", parameter);
  },

  disable: function(_group) {
    var parameter = this.getParameterForDeviceCall(_group);
    this.sendRequest("disable", parameter);
  },

  startDim: function(_up, _group) {
    var parameter = this.getParameterForDeviceCall(_group);
    if(isUndefined(_up) || _up === false) {
      parameter['up'] = true;
    }
    this.sendRequest("startDim", parameter);
  },

  endDim: function(_group) {
    var parameter = this.getParameterForDeviceCall(_group);
    this.sendRequest("endDim", parameter);
  },

  getPowerConsumption: function(_group) {
    var parameter = this.getParameterForDeviceCall(_group);
    var respObj = this.sendSyncRequest("getConsumption", parameter);
    return respObj.consumption;
  },

  sendEvent: function(_name) {
    var event = new HEvent(_name, '.zone(' + this.id + ')').raise();
  },

  sendButtonPress: function(_buttonNumber, _group) {
    var parameter = this.getParameterForDeviceCall(_group);
    parameter['zoneID'] = this.id;
    parameter['buttonnr'] = _buttonNumber;
    DSS.sendRequest("sim/switch/pressed", parameter);
  }
}); // Zone

var Device = Class.create({
  initialize: function(_dsid, _name) {
    this.dsid = _dsid;
    this.name = _name;
    this.on = false;
    this.hasSwitch = false;
  },

  isSwitch: function() {
    return this.hasSwitch;
  },

  isOn: function() {
    return this.on;
  },

  sendRequest: function(_functionName, _parameter) {
    DSS.sendRequest("device/" + _functionName, _parameter);
  },

  sendSyncRequest: function(_functionName, _parameter) {
    return DSS.sendSyncRequest("device/" + _functionName, _parameter);
  },

  getParameterForDeviceCall: function() {
    var parameter = {};
    parameter['dsid'] = this.dsid;
    return parameter;
  },

  turnOn: function() {
    this.sendRequest("turnOn", this.getParameterForDeviceCall());
  },

  turnOff: function() {
    this.sendRequest("turnOff", this.getParameterForDeviceCall());
  },

  callScene: function(_sceneNr) {
    var parameter = this.getParameterForDeviceCall();
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("callScene", parameter);
  },

  saveScene: function(_sceneNr) {
    var parameter = this.getParameterForDeviceCall();
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("saveScene", parameter);
  },

  undoScene: function(_sceneNr) {
    var parameter = this.getParameterForDeviceCall();
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("undoScene", parameter);
  },

  increaseValue: function() {
    var parameter = this.getParameterForDeviceCall();
    this.sendRequest("increaseValue", parameter);
  },

  decreaseValue: function() {
    var parameter = this.getParameterForDeviceCall();
    this.sendRequest("decreaseValue", parameter);
  },

  enable: function() {
    var parameter = this.getParameterForDeviceCall();
    this.sendRequest("enable", parameter);
  },

  disable: function() {
    var parameter = this.getParameterForDeviceCall();
    this.sendRequest("disable", parameter);
  },

  startDim: function(_up) {
    var parameter = this.getParameterForDeviceCall();
    if(!isUndefined(_up) && _up !== false) {
      parameter['up'] = true;
    }
    this.sendRequest("startDim", parameter);
  },

  endDim: function() {
    var parameter = this.getParameterForDeviceCall();
    this.sendRequest("endDim", parameter);
  },

  getPowerConsumption: function() {
    var parameter = this.getParameterForDeviceCall();
    var respObj = this.sendSyncRequest("getConsumption", parameter);
    return respObj.consumption;
  },

  sendEvent: function(_name) {
    var event = new HEvent(_name, '.dsid(' + this.dsid + ')').raise();
  },

  getGroups: function() {
    var parameter = this.getParameterForDeviceCall();
    var respObj = this.sendSyncRequest("getGroups", parameter);
    return respObj.groups;
  },

  refresh: function() {
    var parameter = this.getParameterForDeviceCall();
    var respObj = this.sendSyncRequest("getState", parameter);
    if(respObj.ok) {
      this.on = respObj.result.isOn;
      return true;
    }
    return false;
  }


}); // Device

var HEvent = Class.create({
  initialize: function(_name, _location, _context) {
    this.name = _name;
    this.location = _location;
    this.context = _context;
  },

  sendRequest: function(_functionName, _parameter) {
    DSS.sendRequest("event/" + _functionName, _parameter);
  },

  sendSyncRequest: function(_functionName, _parameter) {
    return DSS.sendSyncRequest("event/" + _functionName, _parameter);
  },

  raise: function() {
    var parameter = {};
    parameter['name'] = this.name;
    if(!Object.isUndefined(this.location)) {
      parameter['location'] = this.location;
    }
    if(!Object.isUndefined(this.context)) {
      parameter['location'] = this.context;
    }
    this.sendRequest('raise', parameter);
  }
}); // HEvent

var Set = Class.create({
  initialize: function(_self) {
    this.self = _self;
  },

  sendRequest: function(_functionName, _parameter) {
    DSS.sendRequest("set/" + _functionName, _parameter);
  },

  sendSyncRequest: function(_functionName, _parameter) {
    return DSS.sendSyncRequest("set/" + _functionName, _parameter);
  },

  getParameterForSetCall: function() {
    var parameter = {};
    parameter['self'] = this.self;
    return parameter;
  },

  turnOn: function() {
    this.sendRequest("turnOn", this.getParameterForSetCall());
  },

  turnOff: function() {
    this.sendRequest("turnOff", this.getParameterForSetCall());
  },

  callScene: function(_sceneNr) {
    var parameter = this.getParameterForSetCall();
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("callScene", parameter);
  },

  saveScene: function(_sceneNr) {
    var parameter = this.getParameterForSetCall();
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("saveScene", parameter);
  },

  undoScene: function(_sceneNr) {
    var parameter = this.getParameterForSetCall();
    parameter['sceneNr'] = _sceneNr;
    this.sendRequest("undoScene", parameter);
  },

  increaseValue: function() {
    var parameter = this.getParameterForSetCall();
    this.sendRequest("increaseValue", parameter);
  },

  decreaseValue: function() {
    var parameter = this.getParameterForSetCall();
    this.sendRequest("decreaseValue", parameter);
  },

  enable: function() {
    var parameter = this.getParameterForSetCall();
    this.sendRequest("enable", parameter);
  },

  disable: function() {
    var parameter = this.getParameterForSetCall();
    this.sendRequest("disable", parameter);
  },

  startDim: function(_up) {
    var parameter = this.getParameterForSetCall();
    if(!isUndefined(_up) && _up !== false) {
      parameter['up'] = true;
    }
    this.sendRequest("startDim", parameter);
  },

  endDim: function() {
    var parameter = this.getParameterForSetCall();
    this.sendRequest("endDim", parameter);
  },

  getPowerConsumption: function() {
    var parameter = this.getParameterForSetCall();
    var respObj = this.sendSyncRequest("getConsumption", parameter);
    return respObj.consumption;
  },

  getGroups: function() {
    var parameter = this.getParameterForSetCall();
    var respObj = this.sendSyncRequest("getGroups", parameter);
    return respObj.groups;
  },

  refresh: function() {
    var parameter = this.getParameterForSetCall();
    var respObj = this.sendSyncRequest("getState", parameter);
    if(respObj.ok) {
      this.on = respObj.result.isOn;
      return true;
    }
    return false;
  },
  
  byZone: function(_zoneID) {
    var parameter = this.getParameterForSetCall();
    parameter['zoneID'] = _zoneID;
    var respObj = this.sendSyncRequest("getState", parameter);
    if(respObj.ok) {
      return new Set(respObj.result.self);
    }
    return undefined;
  },
  
  getDevices: function() {
    var respObj = this.sendSyncRequest("getDevices", this.getParameterForSetCall());
    if(respObj.ok) {
      return respObj.result.devices;
    }
    return undefined;
  }
  
}); // Set


if(hasKey(this, 'onModelLoaded') && !Object.isUndefined(onModelLoaded) && Object.isFunction(onModelLoaded)) {
  onModelLoaded();
}
