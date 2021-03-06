/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland
 *  Author: Roman Köhler <roman.koehler@aizo.com>
 *          Michael Troß <michael.tross@aizo.com>
 */

var LOGFILE_NAME = "system_action.log";
var l = new Logger(LOGFILE_NAME);
var verbose = 0;

var h = new HTTP( { 'debug' : 0 }, null);

var lActionDurationZoneScene = 500;
var lActionDurationDeviceScene = 1000;
var lActionDurationDeviceValue = 1000;
var lActionDurationDeviceBlink = 1000;
var lActionDurationZoneBlink = 500;
var lActionDurationCustomEvent = 100;
var lActionDurationUrl = 100;


function executeZoneScene(oActionNode)
{
    if (verbose >= 2) {
        l.logln('executeZoneScene ' + oActionNode.getPath() );
    }

    var oZoneNode = oActionNode.getChild("zone");
    var oGroupNode = oActionNode.getChild("group");
    var oSceneNode = oActionNode.getChild("scene");
    var oForceNode = oActionNode.getChild("force");
    var zoneId = undefined;
    var groupId = undefined;
    var sceneId = undefined;
    var forceFlag = false;

    if (oZoneNode == null) {
        l.logln('Error: missing zone parameter');
        return;
    } else {
        zoneId = parseInt(oZoneNode.getValue(), 10);
    }

    if (oGroupNode == null) {
        l.logln('Error: missing group parameter');
        return;
    } else {
        groupId = parseInt(oGroupNode.getValue(), 10);
    }

    if (oSceneNode == null) {
        l.logln('Error: missing scene parameter');
        return;
    } else {
        sceneId = parseInt(oSceneNode.getValue(), 10);
    }

    if (oForceNode != null) {
        forceFlag = oForceNode.getValue();
    }

    var z = getZoneByID(zoneId);
    if (z == null) {
        l.logln('Error: zone not found, id ' + zoneId);
        return;
    }

    z.callScene(groupId, sceneId, forceFlag);
}

function executeDeviceScene(oActionNode)
{
    if (verbose >= 2) {
        l.logln('executeDeviceScene ' + oActionNode.getPath() );
    }

    var oDeviceNode = oActionNode.getChild("dsid");
    if (oDeviceNode == null) {
        l.logln('Error: missing dsid parameter');
        return;
    }
    var target = getDevices().byDSID(oDeviceNode.getValue());
    if (target == null) {
        l.logln('Error: device not found, dsid ' + oDeviceNode.getValue());
        return;
    }
    var oSceneNode = oActionNode.getChild("scene");
    if (oSceneNode == null) {
        l.logln('Error: missing scene parameter');
        return;
    }
    var iSceneID = oSceneNode.getValue();
    var fForce = false;
    var oForceNode = oActionNode.getChild("force");
    if (oForceNode != null) {
        fForce = oForceNode.getValue();
    }
    target.callScene(iSceneID, fForce);
}

function executeDeviceValue(oActionNode)
{
    if (verbose >= 2) {
        l.logln('executeDeviceValue ' + oActionNode.getPath() );
    }

    var oDeviceNode = oActionNode.getChild("dsid");
    if (oDeviceNode == null) {
        l.logln('Error: missing dsid parameter');
        return;
    }
    var target = getDevices().byDSID(oDeviceNode.getValue());
    if (target == null) {
        l.logln('Error: device not found, dsid ' + oDeviceNode.getValue());
        return;
    }
    var oValueNode = oActionNode.getChild("value");
    if (oValueNode == null) {
        l.logln('Error: missing value parameter');
        return;
    }
    target.setValue(oValueNode.getValue());
}

function executeDeviceBlink(oActionNode)
{
    if (verbose >= 2) {
        l.logln('executeDeviceBlink ' + oActionNode.getPath() );
    }

    var oDeviceNode = oActionNode.getChild("dsid");
    if (oDeviceNode == null) {
        l.logln('error: missing dsid parameter');
        return;
    }

    var target = getDevices().byDSID(oDeviceNode.getValue());
    if (target == null) {
        l.logln('error: device not found, dsid ' + oDeviceNode.getValue());
        return;
    }
    target.blink();
}

function executeZoneBlink(oActionNode)
{
    if (verbose >= 2) {
        l.logln('executeZoneBlink ' + oActionNode.getPath() );
    }

    var oZoneNode = oActionNode.getChild("zone");
    var oGroupNode = oActionNode.getChild("group");
    var zoneId = undefined;
    var groupId = undefined;

    if (oZoneNode == null) {
        l.logln('Error: missing zone parameter');
        return;
    } else {
        zoneId = parseInt(oZoneNode.getValue(), 10);
    }

    if (oGroupNode == null) {
        groupId = 0;
    } else {
        groupId = parseInt(oGroupNode.getValue(), 10);
    }

    var z = getZoneByID(zoneId);
    if (z == null) {
        l.logln('Error: zone not found, id ' + zoneId);
        return;
    }
    z.blink(groupId);
}

function executeCustomEvent(oActionNode)
{
    if (verbose >= 2) {
        l.logln('executeCustomEvent ' + oActionNode.getPath() );
    }

    var oEventNode = oActionNode.getChild("event");
    var evt = new Event("highlevelevent", {id:oEventNode.getValue()});
    evt.raise();
}

function executeUrl(oActionNode)
{
    var oUrlNode = oActionNode.getChild("url");
    if (oUrlNode == null) {
        l.logln('Error during executeUrl: url parameter missing');
        return;
    }

    var oUrl = oUrlNode.getValue();
    var oMethod = 'GET';
    var sPost = null;

    l.logln('executeUrl: ' + oUrl);

    if (oUrl.lastIndexOf('GET ', 0) === 0) {
        oMethod = 'GET';
        oUrl = oUrl.substr(oUrl.indexOf(' ') + 1);
    }
    if (oUrl.lastIndexOf('POST ', 0) === 0) {
        oMethod = 'POST';
        oUrl = oUrl.substr(oUrl.indexOf(' ') + 1);
        if (oUrl.indexOf(' ') != -1) {
            sPost = oUrl.substr(oUrl.indexOf(' ') + 1);
            oUrl = oUrl.substr(0, oUrl.indexOf(' '));
        }
    }

    try {
        var code = 0;

        h.reset();
        if (oMethod === 'GET') {
            code = h.get(oUrl, '');
        } else if (oMethod === 'POST') {
            code = h.post(oUrl, '', sPost);
        }

        if (code.status.valueOf() === 200) {
            l.logln('OK [' + code.status + ']');
        } else {
            l.logln('FAILED! Server replied: "' + code.status + '"');
            if (verbose >= 3) {
                l.logln(code.body);
            }
        }
    } catch (e) {
        l.logln('Error during executeUrl: ' + e);
        for (var i in e) {
            l.logln(i + ': ' + e[i]);
        }
    }
}

function executeOne(oActionNode)
{
    l.logln('Execute: ' + oActionNode.getPath());

    var oTypeNode = oActionNode.getChild("type");
    if (oTypeNode != null) {
        var sActionType = oTypeNode.getValue();
        if (sActionType != null) {
            if (sActionType == 'zone-scene') {
                executeZoneScene(oActionNode);
                return lActionDurationZoneScene;
            } else if (sActionType == 'device-scene') {
                executeDeviceScene(oActionNode);
                return lActionDurationDeviceScene;
            } else if (sActionType == 'device-value') {
                executeDeviceValue(oActionNode);
                return lActionDurationDeviceValue;
            } else if (sActionType == 'device-blink') {
                executeDeviceBlink(oActionNode);
                return lActionDurationDeviceBlink;
            } else if (sActionType == 'zone-blink') {
                executeZoneBlink(oActionNode);
                return lActionDurationZoneBlink;
            } else if (sActionType == 'custom-event') {
                executeCustomEvent(oActionNode);
                return lActionDurationCustomEvent;
            } else if (sActionType == 'url') {
                setTimeout(function(){executeUrl(oActionNode)}, 1);
                return lActionDurationUrl;
            }
        } else {
            l.logln('Error: value of typenode is null');
        }
    } else {
        l.logln('Error: no typenode in ' + oActionNode.getPath());
    }

    return 0;
}

function executeStep(oActionArray)
{
    var oActionNode = oActionArray[0];
    var lWaitingTime = 100;
    if (oActionNode != null) {
        lWaitingTime = executeOne(oActionNode);
    } else {
        l.logln('Error: action node in executeStep is null ');
    }

    if (oActionArray.length > 1) {
        if (verbose >= 2) {
            l.logln('next action in ' + lWaitingTime);
        }
        oActionArray.shift();
        callNextAction = function () { 
            executeStep(oActionArray);
        };
        setTimeout(callNextAction, lWaitingTime);
    }
}

function execute(pathToDescription)
{
    if (verbose >= 1) {
        l.logln('Execute Path ' + pathToDescription);
    }

    var oBaseActionNode = Property.getNode(pathToDescription + '/actions');
    if (oBaseActionNode == null) {
        l.logln('Error: no actionNodes in path ' + pathToDescription);
        return;
    }
    var oArrayActions = oBaseActionNode.getChildren();
    if (oArrayActions == null) {
        l.logln('Error: no actionSubnodes in path ' + pathToDescription);
        return;
    }
    if (oArrayActions.length == 0) {
        l.logln('Error: actionSubnode count=0 in path ' + pathToDescription);
        return;
    }

    var oDelay = [0];
    for (var iIndex = 0; iIndex < oArrayActions.length; iIndex++)
    {
        if (oArrayActions[iIndex].getChild("delay") != null) {
            var iDelayNode = parseInt(oArrayActions[iIndex].getChild("delay").getValue(), 10);
            if (oDelay.indexOf(iDelayNode) == -1) {
                oDelay.push(iDelayNode);
                if (verbose >= 2) {
                    l.logln('Debug: found delay value: ' + iDelayNode);
                }
            }
        } else {
            if (verbose >= 2) {
                l.logln('Debug: no delay value found');
            }
        }
    }

    if (oDelay.length > 1) {
        if (verbose > 1) {
            l.logln('Debug: delay parameter present, execution will be fragmented');
        }
        for (var iIndex = 0; iIndex < oDelay.length; iIndex++) {
            if (verbose > 1) {
                l.logln('Debug: delaying event for ' + pathToDescription +
                    ' with value ' + oDelay[iIndex]);
            }
            var evt = new TimedEvent("action_execute", '+' + oDelay[iIndex],
                                {path:pathToDescription, delay:oDelay[iIndex]});
            evt.raise();
        }
    } else {
        // -> system_condition.js
        if (! checkCondition(pathToDescription)) {
            l.logln('Info: condition check failed in path ' + pathToDescription);
            return;
        }

        executeStep(oArrayActions);

        var iValue = new Date().getTime();
        Property.setProperty(pathToDescription + '/lastExecuted', '' + iValue)
    }
}

function filterActionsWithDelay(oArrayActions,delayValue) {
    var oResultArray = [];
    for (var iIndex = 0; iIndex < oArrayActions.length; iIndex++) {
        var iDelayNode = 0;
        if (oArrayActions[iIndex].getChild("delay") != null) {
            iDelayNode = parseInt(oArrayActions[iIndex].getChild("delay").getValue(),10);
        }
        if (iDelayNode == delayValue) {
            oResultArray.push(oArrayActions[iIndex]);
        }
    }
    return oResultArray;
}

function executeWithDelay(pathToDescription, delayValue) {
    if (verbose >= 1) {
        l.logln('Execute Path ' + pathToDescription + ' with delay value ' + delayValue);
    }
    var oBaseActionNode = Property.getNode(pathToDescription + '/actions');
    if (oBaseActionNode == null) {
        l.logln('Error: no actionNodes in path ' + pathToDescription);
        return;
    }
    var oArrayActions = oBaseActionNode.getChildren();
    if (oArrayActions == null) {
        l.logln('Error: no actionSubnodes in path ' + pathToDescription);
        return;
    }
    if (oArrayActions.length == 0) {
        l.logln('Error: actionSubnode count=0 in path ' + pathToDescription);
        return;
    }

    // -> system_condition.js
    if (! checkCondition(pathToDescription)) {
        l.logln('Info: condition check failed in path ' + pathToDescription);
        return;
    }

    var delayedActions = filterActionsWithDelay(oArrayActions, delayValue);
    if (delayedActions.length > 0) {
        executeStep(delayedActions);
    }
}

function getPath(name)
{
    // check for user defined actions
    var baseNode = Property.getNode('/usr/events');
    if (baseNode == null) {
        // ok if no hle's found
        return null;
    }
    var aBaseArray = baseNode.getChildren();
    if (aBaseArray == null) {
        return null;
    }
    for (var iIndex=0; iIndex < aBaseArray.length; iIndex++) {
        var oNameNode = aBaseArray[iIndex].getChild('id');
        if ((oNameNode != null) && (oNameNode.getValue() == name)) {
            return aBaseArray[iIndex].getPath();
        }
    }
    return null;
}

if (raisedEvent.name == 'action_execute')
{
    if (raisedEvent.parameter.path != null) {
        if (raisedEvent.parameter.delay != null) {
            executeWithDelay(raisedEvent.parameter.path, raisedEvent.parameter.delay);
        } else {
            execute(raisedEvent.parameter.path);
        }
    }
}

if (raisedEvent.name == 'highlevelevent') {
    if (raisedEvent.parameter.id != null) {
        var sPath = getPath(raisedEvent.parameter.id);
        if (sPath != null) {
            execute(sPath);
        }
        else {
            l.logln('Event for id \"' + raisedEvent.parameter.id + '\" not found');
        }
    }
}
