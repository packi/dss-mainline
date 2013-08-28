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
 *  Author: Michael Troß <michael.tross@aizo.com>
 */

var LOGFILE_NAME = "system-event.log";
var l = new Logger(LOGFILE_NAME);
var dumpEvent = 0;

var sceneTable0 = [
    'Off',
    'Off-Area1',
    'Off-Area2',
    'Off-Area3',
    'Off-Area4',
    'Stimmung1',
    'On-Area1',
    'On-Area2',
    'On-Area3',
    'On-Area4',
    'Dimm-Area',
    'Dec',
    'Inc',
    'Min',
    'Max',
    'Stop',
    'N/A',
    'Stimmung2',
    'Stimmung3',
    'Stimmung4',
    'Stimmung12',
    'Stimmung13',
    'Stimmung14',
    'Stimmung22',
    'Stimmung23',
    'Stimmung24',
    'Stimmung32',
    'Stimmung33',
    'Stimmung34',
    'Stimmung42',
    'Stimmung43',
    'Stimmung44',
    'Off-Stimmung10',
    'On-Stimmung11',
    'Off-Stimmung20',
    'On-Stimmung21',
    'Off-Stimmung30',
    'On-Stimmung31',
    'Off-Stimmung40',
    'On-Stimmung41',
    'Off-Automatic',
    'N/A',
    'Dec-Area1',
    'Inc-Area1',
    'Dec-Area2',
    'Inc-Area2',
    'Dec-Area3',
    'Inc-Area3',
    'Dec-Area4',
    'Inc-Area4',
    'Off-Device',
    'On-Device',
    'Stop-Area1',
    'Stop-Area2',
    'Stop-Area3',
    'Stop-Area4'
];

var sceneTable1 = [
    'N/A',
    'Panic',
    'Overload',
    'Zone-Standby',
    'Zone-Deep-Off',
    'Sleeping',
    'Wake-Up',
    'Present',
    'Absent',
    'Bell',
    'Alarm',
    'Zone-Activate',
    'Fire',
    'Smoke',
    'Water',
    'Gas',
    'Bell2',
    'Bell3',
    'Bell4',
    'Alarm2',
    'Alarm3',
    'Alarm4',
    'Wind',
    'WindN',
    'Rain',
    'RainN',
    'Hail',
    'HailN'
];

var groupTable = [
    'Broadcast',
    'Yellow',
    'Gray',
    'Blue',
    'Cyan',
    'Magenta',
    'Red',
    'Green',
    'Black',
    'White'
];

function genZoneName(zone)
{
    var zName = zone.name;
    if (zone.id == 0) {
        zName = 'Broadcast';
    }
    return zName + ';' + zone.id;
}

function genGroupName(groupId)
{
    var gName = 'Unknown';
    if (groupId < groupTable.length) {
        gName = groupTable[groupId];
    }
    return gName + ';' + groupId ;
}

function genDeviceName(originDeviceID)
{
    var devName = 'Unknown';
    var originDevice = parseInt(originDeviceID, 16);
    if (originDevice < 16) {
        if (originDevice == 0) {
            devName = '(unspecified)';
        } else if (originDevice == 1) {
            devName = 'Scripting';
        } else if (originDevice == 2) {
            devName = 'JSON';
        } else if (originDevice == 3) {
            devName = 'SOAP';
        } else if (originDevice == 4) {
            devName = 'Bus-Handler';
        } else if (originDevice == 5) {
            devName = 'Simulation';
        } else if (originDevice == 6) {
            devName = 'Test';
        } else if (originDevice == 7) {
            devName = 'System';
        } else {
            devName = '';
        }
        devName += ';' + originDevice;
    }
    else {
        var dev = getDevices().byDSID(originDeviceID);
        if (dev != undefined && dev != null) {
            devName = dev.name;
        } else {
            devName = 'Unknown';
        }
        devName += ';' + originDeviceID;
    }
    return devName;
}

function genSceneName(sceneId)
{
    var sceneName = '';
    if ((sceneId < 64) && (sceneId < sceneTable0.length)) {
        sceneName = sceneTable0[sceneId];
    }
    if ((sceneId >= 64) && ((sceneId - 64) < sceneTable1.length)) {
        sceneName = sceneTable1[sceneId-64];
    }
    sceneName += ';' + sceneId;
    return sceneName;
}

function logLastScene(zone, groupId, sceneId)
{
    var zoneName = genZoneName(zone);
    var groupName = genGroupName(groupId);
    var sceneName = genSceneName(sceneId);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';Last Scene;', sceneName, ';', zoneName, ';', groupName, ';;;');
}

function logZoneGroupScene(zone, groupId, sceneId, isForced, originDeviceId, originToken)
{
    var zoneName = genZoneName(zone);
    var groupName = genGroupName(groupId);
    var devName = genDeviceName(originDeviceId);
    var sceneName = genSceneName(sceneId);
    if (isForced == 'yes') {
        //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
        l.logln(';CallSceneForced;', sceneName, ';', zoneName, ';', groupName, ';', devName, ';', originToken);
    }
    else {
        //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
        l.logln(';CallScene;', sceneName, ';', zoneName, ';', groupName, ';', devName, ';', originToken);
    }
}

function logZoneGroupBlink(zone, groupId, originDeviceId, originToken)
{
    var zoneName = genZoneName(zone);
    var groupName = genGroupName(groupId);
    var devName = genDeviceName(originDeviceId);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';Blink;;', zoneName, ';', groupName, ';', devName, ';;', originToken);
}

function logZoneGroupUndo(zone, groupId, sceneId, originDeviceId, originToken)
{
    var zoneName = genZoneName(zone);
    var groupName = genGroupName(groupId);
    var devName = genDeviceName(originDeviceId);
    var sceneName = genSceneName(sceneId);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';UndoScene;', sceneName, ';', zoneName, ';', groupName, ';', devName, ';', originToken);
}

function logDeviceScene(device, zone, sceneId, isForced, originDeviceId, token)
{
    var zoneName = genZoneName(zone);
    var devName = device.name + ';' + device.dsid;
    var origName = genDeviceName(originDeviceId);
    var sceneName = genSceneName(sceneId);
    if (isForced == 'yes') {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;originToken');
      l.logln(';DeviceSceneForced;', sceneName, ';', zoneName, ';', devName, ';', origName, ';', token);
    }
    else {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;originToken');
      l.logln(';DeviceScene;', sceneName, ';', zoneName, ';', devName, ';', origName, ';', token);
    }
}

function logDeviceBlink(device, zone, originDeviceId, originToken)
{
    var zoneName = genZoneName(zone);
    var devName = device.name + ';' + device.dsid;
    var origName = genDeviceName(originDeviceId);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;originToken');
    l.logln(';DeviceBlink;;', zoneName, ';', devName, ';;', origName, ';', originToken);
}

function logDeviceLocalScene(sceneId, originDeviceId)
{
    var devName = genDeviceName(originDeviceId);
    var sceneName = genSceneName(sceneId);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';Device;', sceneName, ';;;;;', devName, ';');
}

function logDeviceButtonClick(device, param)
{
    var devName = device.name + ';' + device.dsid;
    var keynum = param.buttonIndex;
    var clicknum = param.clickType;
    var holdCount = param.holdCount;

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    if (holdCount == undefined) {
        l.logln(';ButtonClick', ';Type ' + clicknum, ';' + keynum, ';;;;;', devName, ';');
    }
    else {
        l.logln(';ButtonClick', ';Hold ' + holdCount, ';' + keynum, ';;;;;', devName, ';');
    }
}

function logDeviceBinaryInput(device, param)
{
    var devName = device.name + ';' + device.dsid;
    var index = param.inputIndex;
    var state = param.inputState;
    var type = param.inputType;

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';BinaryInput', ';State ' + state, ';' + index, ';;;;;', devName, ';');
}

function logDeviceSensorEvent(device, param)
{
    var devName = device.name + ';' + device.dsid;
 
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';EventTable;', param.sensorEvent, ';;;;;;', devName, ';');
}

function logStateChangeApartment(statename, state, value, originDeviceId)
{
    var origin = genDeviceName(originDeviceId);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken'');
    l.logln(';StateApartment;', statename, ';', value, ';', state ,';;;;;', origin, ';');
}

function logStateChangeScript(statename, state, value, originDeviceId)
{
    var origin = genDeviceName(originDeviceId);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';StateAddonScript;', statename, ';', value, ';', state ,';;;;;', origin, ';');
}
function logStateChangeDevice(statename, state, value, device)
{
    var devName = device.name + ';' + device.dsid;
 
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(';StateDevice;', statename, ';', value, ';', state, ';;;;', devName, ';');
}

function logStateChangeGroup(statename, state, value, groupID, zoneID)
{
    var zone = getZoneByID(zoneID);
    var groupName = genGroupName(groupID);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    l.logln(  ';StateGroup;', statename, ';', value, ';', state, ';', zone.name, ';', zoneID, ';', groupName, ';;');
}


if (dumpEvent > 0) {
    for (key in raisedEvent)
    {
       l.logln(" raisedEvent." + key + " = " + raisedEvent[key]);
       if (typeof raisedEvent[key] == "object")
       {
          for (key1 in raisedEvent[key])
          {
             l.logln("  raisedEvent." + key + "." + key1 + " = " + raisedEvent[key][key1]);
          } 
       }
    }
}

/*
 * on startup log the initial state for each zone and group
 */
if (raisedEvent.name == 'model_ready')
{
    l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');

    var zones = getZones();
    for (var z = 0; z < zones.length; z++) {
        if (!zones[z].present) {
            continue;
        }
        var groupNode = Property.getNode('/apartment/zones/zone' +
                                                       zones[z].id + '/groups');
        if (groupNode == undefined) {
            continue;
        }
        var groups = groupNode.getChildren();
        for (var i = 0; i < groups.length; i++) {
            var g = groups[i];
            var gindex = g.getChild('group').getValue();
            if (gindex <= 0 || gindex >= 16) {
                continue;
            }
            var sceneId = g.getChild('lastCalledScene').getValue();
            if (sceneId > 0) {
                logLastScene(zones[z], gindex, sceneId);
            }
        }
    }
}

/*
 * for callScene decode names and numbers, differentiate for local on/off
 */
else if (raisedEvent.name == 'callScene')
{
    var sceneId = raisedEvent.parameter.sceneID;
    var isForced = raisedEvent.parameter.forced;
    var zoneId = raisedEvent.source.zoneID;
    var token = raisedEvent.parameter.originToken;
    if (!token) {
        token = "";
    }
    if (raisedEvent.source.isGroup) {
        // ZoneGroup Action Request
        var groupId = raisedEvent.source.groupID;
        var originDeviceId = raisedEvent.parameter.originDeviceID;
        logZoneGroupScene(getZoneByID(zoneId), groupId, sceneId, isForced, originDeviceId, token);
    }
    if (raisedEvent.source.isDevice) {
        var originDeviceId = raisedEvent.parameter.originDeviceID;
        if (originDeviceId == 0) {
            // DeviceLocal Action Event
            logDeviceLocalScene(sceneId, raisedEvent.source.dsid);
        }
        else {
            // Device Action Request
            var device = getDevices().byDSID(raisedEvent.source.dsid);
            logDeviceScene(device, getZoneByID(zoneId), sceneId, isForced, originDeviceId, token);
        }
    }
}

/*
 * blink can be on device or on zone/group
 */
else if (raisedEvent.name == 'blink')
{
    var zoneId = raisedEvent.source.zoneID;
    var token = raisedEvent.parameter.originToken;
    if (!token) {
        token = "";
    }
    if (raisedEvent.source.isGroup) {
        // ZoneGroup Action Request
        var groupId = raisedEvent.source.groupID;
        var originDeviceId = raisedEvent.parameter.originDeviceID;
        logZoneGroupBlink(getZoneByID(zoneId), groupId, originDeviceId, token);
    }
    if (raisedEvent.source.isDevice) {
        // Device Action Request
        var originDeviceId = raisedEvent.parameter.originDeviceID;
        var device = getDevices().byDSID(raisedEvent.source.dsid);
        logDeviceBlink(device, getZoneByID(zoneId), originDeviceId, token);
    }
}

/*
 * undoScene is only defined on zone level
 */
else if (raisedEvent.name == 'undoScene')
{
    var sceneId = raisedEvent.parameter.sceneID;
    var zoneId = raisedEvent.source.zoneID;
    var token = raisedEvent.parameter.originToken;
    if (!token) {
        token = "";
    }
    if (raisedEvent.source.isGroup) {
        var groupId = raisedEvent.source.groupID;
        var originDeviceId = raisedEvent.parameter.originDeviceID;
        logZoneGroupUndo(getZoneByID(zoneId), groupId, sceneId, originDeviceId, token);
    }
}

/*
 * joker devices button tips
 */
else if (raisedEvent.name == 'buttonClick')
{
    var device = getDevices().byDSID(raisedEvent.source.dsid);
    logDeviceButtonClick(device, raisedEvent.parameter);
}

/*
 * joker devices binary inputs
 */
else if (raisedEvent.name == 'deviceBinaryInputEvent')
{
    var device = getDevices().byDSID(raisedEvent.source.dsid);
    logDeviceBinaryInput(device, raisedEvent.parameter);
}

/*
 * some devices are capable of generating sensor events
 */
else if (raisedEvent.name == 'deviceSensorEvent')
{
    var device = getDevices().byDSID(raisedEvent.source.dsid);
    logDeviceSensorEvent(device, raisedEvent.parameter);
}

/*
 * state changes
 */
else if (raisedEvent.name == 'stateChange')
{
    var statename = raisedEvent.parameter.statename;
    var state = raisedEvent.parameter.state;
    var value = raisedEvent.parameter.value;
    if (raisedEvent.source.isService) {
        var originId = raisedEvent.parameter.originDeviceID;
        logStateChangeApartment(statename, state, value, originId);
    }
    if (raisedEvent.source.isScript) {
        var originId = raisedEvent.parameter.originDeviceID;
        logStateChangeScript(statename, state, value, originId);
    }
    if (raisedEvent.source.isDevice) {
        var device = getDevices().byDSID(raisedEvent.source.dsid);
        logStateChangeDevice(statename, state, value, device);
    }
    if (raisedEvent.source.isGroup) {
        var groupID = raisedEvent.source.groupID;
        var zoneID = raisedEvent.source.zoneID;
        logStateChangeGroup(statename, state, value, groupID, zoneID);
    }
}

