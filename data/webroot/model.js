function DSS() {
}

DSS.endpoint = "/json/";
DSS.sendSyncRequest = function(_uri, _parameter) {
    new Ajax.Request(DSS.endpoint + _uri, 
      {
        method: 'get', 
        parameter: _parameter,
        asynchronous: false,
        onComplete: function(transport, json) {
          self.doUpdate(transport.responseJSON.apartment.zones);
        }
      } 
    );

}

DSS.sendRequest = function(_uri, _parameter) {
    new Ajax.Request(DSS.endpoint + _uri, 
      {
        method: 'get', 
        parameter: _parameter,
      } 
    );
}

function hasKey(_obj, _key) {
  return (typeof(_obj[_key]) != "undefined");
}

function Group(_name, _id) {
  this.name = _name;
  this.id = _id;
}

function Apartment2() {
}

Apartment2.sendRequest = function(_functionName, _parameter) {
  DSS.sendRequest("apartment/" + _functionName, _parameter);
} 

Apartment2.getParameterForDeviceCall = function(_group) {
  var parameter = [];
  if(isNumber(_group)) {
    parameter['groupID'] = _group;
  } else if(isString(_group)) {
    parameter['groupName'] = _group; 
  } else if(!isUndefined(_group) && _group != null) {
    if(hasKey(_group, 'name')) {
      parameter['groupName'] = _group['name'];
    } else if(hasKey(_group, 'id')) {
      parameter['groupID'] = _group['id'];
    }
  }
  return parameter;
}

Apartment2.turnOn = function(_group) {
  this.sendRequest("turnOn", getParameterForDeviceCall(_group));
}
 
Apartment2.prototype.turnOff = function(_group) {
  this.sendRequest("turnOff", getParameterForDeviceCall(_group));
}

Apartment2.prototype.callScene = function(_sceneID, _group) {
  var parameter = getParameterForDeviceCall(_group);
  parameter['sceneID'] = _sceneID;
  this.sendRequest("callScene", parameter);
}

Apartment2.prototype.saveScene = function(_sceneID, _group) {
  var parameter = getParameterForDeviceCall(_group);
  parameter['sceneID'] = _sceneID;
  this.sendRequest("saveScene", parameter);
}

Apartment2.prototype.undoScene = function(_sceneID, _group) {
  var parameter = getParameterForDeviceCall(_group);
  parameter['sceneID'] = _sceneID;
  this.sendRequest("undoScene", parameter);
}
