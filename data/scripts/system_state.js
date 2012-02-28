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

/*
 * Manage system level states
 */

function startup()
{
    var absent = false;
    var panic = false;
    var alarm = false;
    var sleeping = false;

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
            if (g.getChild('lastCalledScene').getValue() == Scene.Absent) {
                absent = true;
            }
            if (g.getChild('lastCalledScene').getValue() == Scene.Panic) {
                panic = true;
            }
            if (g.getChild('lastCalledScene').getValue() == Scene.Alarm) {
                alarm = true;
            }
            if (g.getChild('lastCalledScene').getValue() == Scene.Sleeping) {
                sleeping = true;
            }
        }
    }

    var pNode;
    pNode = Property.getNode('/usr/states/presence');
    if (absent === true && pNode.getChild('value').getValue() == 'present') {
        pNode.setStatusValue("absent");
    }
    else if (absent === false) {
        pNode.setStatusValue("present");
    }

    pNode = Property.getNode('/usr/states/panic');
    if (panic === true && pNode.getChild('value').getValue() == 'inactive') {
        pNode.setStatusValue("active");
    }
    else if (panic === false) {
        pNode.setStatusValue("inactive");
    }

    pNode = Property.getNode('/usr/states/alarm');
    if (alarm === true && pNode.getChild('value').getValue() == 'inactive') {
        pNode.setStatusValue("active");
    }
    else if (alarm === false) {
        pNode.setStatusValue("inactive");
    }

    pNode = Property.getNode('/usr/states/hibernation');
    if (sleeping === true) {
        pNode.setStatusValue('sleeping');
    }
    else if (pNode.getChild('value').getValue() == '') {
        pNode.setStatusValue('awake');
    }
}

function callscene()
{
    var sceneID = parseInt(raisedEvent.parameter.sceneID);
    var zoneID = parseInt(raisedEvent.source.zoneID);
    var groupID = parseInt(raisedEvent.source.groupID);
    var pNode;

    if (groupID === 0 && sceneID === Scene.Absent) {
        pNode = Property.getNode('/usr/states/presence');
        pNode.setStatusValue("absent");
    }
    else if (groupID === 0 && sceneID === Scene.Present) {
        pNode = Property.getNode('/usr/states/presence');
        pNode.setStatusValue("present");
    }
    else if (groupID === 0 && sceneID === Scene.Panic) {
        pNode = Property.getNode('/usr/states/panic');
        pNode.setStatusValue("active");
    }
    else if (groupID === 0 && sceneID === Scene.Alarm) {
        pNode = Property.getNode('/usr/states/alarm');
        pNode.setStatusValue("active");
    }
    else if (groupID === 0 && sceneID === Scene.Sleeping) {
        pNode = Property.getNode('/usr/states/hibernation');
        pNode.setStatusValue("sleeping");
    }
    else if (groupID === 0 && sceneID === Scene.WakeUp) {
        pNode = Property.getNode('/usr/states/hibernation');
        pNode.setStatusValue("awake");
    }
    else if (raisedEvent.source.isGroup && (sceneID < 64)) {
        var originDevice = parseInt(raisedEvent.parameter.originDeviceID, 16);
        // reserved origin addresses
        if (originDevice >= 16) {
            pNode = Property.getNode('/usr/states/presence');
            if (pNode.getChild('value').getValue() == 'absent') {
                pNode.setStatusValue("present");
            }
        }
    }
}

function undoscene()
{
    var sceneID = parseInt(raisedEvent.parameter.sceneID);
    var zoneID = parseInt(raisedEvent.source.zoneID);
    var groupID = parseInt(raisedEvent.source.groupID);

    if (groupID === 0 && sceneID === Scene.Panic) {
        var pNode = Property.getNode('/usr/states/panic');
        pNode.setStatusValue("inactive");
    }
    if (groupID === 0 && sceneID === Scene.Alarm) {
        var pNode = Property.getNode('/usr/states/alarm');
        pNode.setStatusValue("inactive");
    }
}

if (raisedEvent.name == 'model_ready') {
    startup();
}
else if (raisedEvent.name == 'callScene') {
    callscene();
}
else if (raisedEvent.name == 'undoScene') {
    undoscene();
}
