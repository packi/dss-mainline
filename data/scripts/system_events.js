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
    'N/A',
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
    'Gas'
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
    return 'Zone ' + zName + ';' + zone.id;
}

function genGroupName(groupId)
{
    var gName = 'Unknown';
    if (groupId < groupTable.length) {
        gName = groupTable[groupId];
    }
    return 'Group ' + gName + ';' + groupId ;
}

function genDeviceName(originDeviceID)
{
    var devName = 'Device Unknown';
    var originDevice = parseInt(originDeviceID, 16);
    if (originDevice < 16) {
        if (originDevice == 0) {
            devName = 'Device (unspecified)';
        } else if (originDevice == 1) {
            devName = 'Server via Scripting';
        } else if (originDevice == 2) {
            devName = 'Server via JSON';
        } else if (originDevice == 3) {
            devName = 'Server via SOAP';
        } else if (originDevice == 4) {
            devName = 'Server via Bus-Handler';
        } else if (originDevice == 5) {
            devName = 'Server via Simulation';
        } else if (originDevice == 6) {
            devName = 'Server Test';
        } else {
            devName = 'Server via ' + originDevice;
        }
    }
    else {
        var dev = getDevices().byDSID(originDeviceID);
        if (dev != undefined && dev != null) {
            devName = 'Device ' + dev.name;
        } else {
            devName = 'Device Unknown';
        }
        devName += ';' + originDeviceID;
    }
    return devName;
}

function genSceneName(sceneId)
{
    var sceneName = 'Scene ';
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
    //l.logln('Time;Event;Action;Value;Zone;Zone-ID;Group;Group-ID;Device;DSID');
    l.logln(';Last Scene;', sceneName, ';', zoneName, ';', groupName, ';;');
}

function logZoneGroupScene(zone, groupId, sceneId, originDeviceId)
{
    var zoneName = genZoneName(zone);
    var groupName = genGroupName(groupId);
    var devName = genDeviceName(originDeviceId);
    var sceneName = genSceneName(sceneId);
    //l.logln('Time;Event;Action;Value;Zone;Zone-ID;Group;Group-ID;Device;DSID');
    l.logln(';CallScene;', sceneName, ';', zoneName, ';', groupName, ';', devName);
}

function logZoneGroupUndo(zone, groupId, sceneId, originDeviceId)
{
    var zoneName = genZoneName(zone);
    var groupName = genGroupName(groupId);
    var devName = genDeviceName(originDeviceId);
    var sceneName = genSceneName(sceneId);
    //l.logln('Time;Event;Action;Value;Zone;Zone-ID;Group;Group-ID;Device;DSID');
    l.logln(';UndoScene;', sceneName, ';', zoneName, ';', groupName, ';', devName);
}

function logDeviceLocalScene(sceneId, originDeviceId)
{
    var devName = genDeviceName(originDeviceId);
    var sceneName = genSceneName(sceneId);
    //l.logln('Time;Event;Action;Value;Zone;Zone-ID;Group;Group-ID;Device;DSID');
    l.logln(';Device;', sceneName, ';;;;;', devName);
}

function logDeviceButtonClick(device, param)
{
    var devName = 'Device ' + device.name + ';' + device.dsid;
    var keynum = param.buttonIndex;
    var clicknum = param.clickType;
    var holdCount = param.holdCount;

    //l.logln('Time;Event;Action;Value;Zone;Zone-ID;Group;Group-ID;Device;DSID');
    if (holdCount == undefined) {
        l.logln(';ButtonClick', ';Type ' + clicknum, ';Index ' + keynum, ';;;;;', devName);
    }
    else {
        l.logln(';ButtonClick', ';Hold ' + holdCount, ';Index ' + keynum, ';;;;;', devName);
    }
}

function logDeviceSensorEvent(device, param)
{
    var devName = 'Device ' + device.name + ';' + device.dsid;
    
    //l.logln('Time;Event;Action;Value;Zone;Zone-ID;Group;Group-ID;Device;DSID');
    l.logln(';EventTable;', param.sensorEvent, ';;;;;;', devName);
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
    l.logln('Time;Event;Action;Value;Zone;Zone-ID;Group;Group-ID;Device;DSID');

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
    var zoneId = raisedEvent.source.zoneID;
    if (raisedEvent.source.isGroup) {
        var groupId = raisedEvent.source.groupID;
        var originDeviceId = raisedEvent.parameter.originDeviceID;
        logZoneGroupScene(getZoneByID(zoneId), groupId, sceneId, originDeviceId);
    }
    if (raisedEvent.source.isDevice) {
        logDeviceLocalScene(sceneId, raisedEvent.source.dsid);
    }
}

/*
 * undoScene is only defined on zone level
 */
else if (raisedEvent.name == 'undoScene')
{
    var sceneId = raisedEvent.parameter.sceneID;
    var zoneId = raisedEvent.source.zoneID;
    if (raisedEvent.source.isGroup) {
        var groupId = raisedEvent.source.groupID;
        var originDeviceId = raisedEvent.parameter.originDeviceID;
        logZoneGroupUndo(getZoneByID(zoneId), groupId, sceneId, originDeviceId);
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
 * some devices are capable of generating sensor events
 */
else if (raisedEvent.name == 'deviceSensorEvent')
{
    var device = getDevices().byDSID(raisedEvent.source.dsid);
    logDeviceSensorEvent(device, raisedEvent.parameter);
}
