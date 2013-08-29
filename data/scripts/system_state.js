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

function bootstrap()
{
    print("Setup system state handling...");
    registerState("alarm", true, false);
    registerState("alarm2", true, false);
    registerState("alarm3", true, false);
    registerState("alarm4", true, false);
    registerState("panic", true, false);
    registerState("fire", true, false);
    registerState("wind", true, false);
    registerState("rain", true, false);
    registerState("hail", true, false);
}

function getOrRegisterState(stateName)
{
    var s;
    try {
        s = getState(stateName, false);
    }
    catch(err) {
        s = registerState(stateName, true, false);
    }
    return s;
}

function startup()
{
    var absent = false;
    var panic = false;
    var alarm = false;
    var sleeping = false;

    var devices = getDevices();
    devices.perform(function (device) {
        var devNode;
        var bInputNode;
        var inputIndex = 0;

        devNode = device.getPropertyNode();
        if (!devNode) {
            return;
        }
        bInputNode = devNode.getChild('binaryInputs/binaryInput' + inputIndex);
        if (!bInputNode) {
            return;
        }
        for (inputIndex = 0;
            bInputNode != null;
            inputIndex ++, bInputNode = devNode.getChild('binaryInputs/binaryInput' + inputIndex))
        {
            var inputType = bInputNode.getChild('inputType').getValue();
            var inputId = bInputNode.getChild('inputId').getValue();
            var targetType = bInputNode.getChild('targetGroupType').getValue();
            var targetId = bInputNode.getChild('targetGroupId').getValue();

            // Motion
            if (inputType === 5 || inputType === 6) {
                var stateName;
                if (targetId >= 16) {
                    stateName = 'zone.0.group.' + targetId + '.motion';
                } else {
                    stateName = 'zone.' + device.zoneID + '.motion';
                }
                getOrRegisterState(stateName);
            }

            // Presence
            if (inputType === 1 || inputType === 3) {
                var stateName;
                if (targetId >= 16) {
                    stateName = 'zone.0.group.' + targetId + '.presence';
                } else {
                    stateName = 'zone.' + device.zoneID + '.presence';
                }
                getOrRegisterState(stateName);
            }

            // Wind Monitor
            if (inputType === 8) {
                var stateName = 'wind';
                if (targetId >= 16) {
                    stateName = stateName + '.group' + targetId;
                    getOrRegisterState(stateName);
                }
            }

            // Rain Monitor
            if (inputType === 9) {
                var stateName = 'rain';
                if (targetId >= 16) {
                    stateName = stateName + '.group' + targetId;
                    getOrRegisterState(stateName);
                }
            }

            inputIndex ++;
        }
    })

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
            if (gindex <= 0) {
                continue;
            }
            if (gindex >= 16 && gindex <= 23) {
                if (g.getChild('color').getValue() == 2) {
                    registerState('wind.group' + gindex, true, false);
                }
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

    pNode = Property.getNode('/usr/states/hibernation');
    if (sleeping === true) {
        pNode.setStatusValue('sleeping');
    }
    else if (pNode.getChild('value').getValue() == '') {
        pNode.setStatusValue('awake');
    }

    var state;
    state = getState('panic', false);
    if (panic === true && state.getValue() == 'inactive') {
        state.setValue('active');
    }
    else if (panic === false) {
        state.setValue('inactive');
    }

    state = getState('alarm', false);
    if (alarm === true && state.getValue() == 'inactive') {
        state.setValue('active');
    }
    else if (panic === false) {
        state.setValue('inactive');
    }

    // clear fire alarm after 6h
    var t0 = 6 * 60 * 60;
    var sr = '{ "name":"FireAutoClear", "id": "system_state_fire_alarm_reset",\
      "triggers":[{ "type":"state-change", "name":"fire", "state":"active"}],\
      "delay":' + t0 + ',\
      "actions":[{ "type":"undo-zone-scene", "zone":0, "group":0, "scene":76, "force":"false", "delay":0 }],\
      "conditions":{ "enabled":null,"weekdays":null,"timeframe":null,"zoneState":null,\
        "systemState":[{"name":"fire","value":"1"}],\
        "addonState":null},\
      "scope":"system_state.auto_cleanup"\
      }';

    var src = escape(sr);
    var evt = new Event("system-addon-scene-responder.config", { actions: "save", value: src });
    evt.raise();
}

function callscene()
{
    var sceneID = parseInt(raisedEvent.parameter.sceneID);
    var zoneID = parseInt(raisedEvent.source.zoneID);
    var groupID = parseInt(raisedEvent.source.groupID);
    var forced = raisedEvent.parameter.forced;
    var pNode;

    var originDeviceId = 0;
    if (raisedEvent.source.isGroup) {
        originDeviceId = parseInt(raisedEvent.parameter.originDeviceID, 16);
    } else if (raisedEvent.source.dsid) {
        originDeviceId = parseInt(raisedEvent.source.dsid, 16);
    }
    if (originDeviceId == 7) {
        // ignore scene calls originated by server generated system level events,
        // e.g. scene calls issued by state changes
        return;
    }

    if (groupID === 0 && sceneID === Scene.Absent) {
        pNode = Property.getNode('/usr/states/presence');
        pNode.setStatusValue("absent");

        // #2561: auto-clear panic and fire
        state = getState('panic', false);
        if (state.getValue() == 1) {
            var z = getZoneByID(0);
            z.undoSceneSys(0, Scene.Panic, "manual");
        }
        state = getState('fire', false);
        if (state.getValue() == 1) {
            var z = getZoneByID(0);
            z.undoSceneSys(0, Scene.Fire, "manual");
        }
    }
    else if (groupID === 0 && sceneID === Scene.Present) {
        pNode = Property.getNode('/usr/states/presence');
        pNode.setStatusValue("present");
    }
    else if (groupID === 0 && sceneID === Scene.Sleeping) {
        pNode = Property.getNode('/usr/states/hibernation');
        pNode.setStatusValue("sleeping");
    }
    else if (groupID === 0 && sceneID === Scene.WakeUp) {
        pNode = Property.getNode('/usr/states/hibernation');
        pNode.setStatusValue("awake");
    }
    else if (groupID === 0 && sceneID === Scene.Panic) {
        state = getState('panic', false);
        state.setValue('active', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Fire) {
        state = getState('fire', false);
        state.setValue('active', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Alarm) {
        state = getState('alarm', false);
        state.setValue('active', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Alarm2) {
        state = getState('alarm2', false);
        state.setValue('active', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Alarm3) {
        state = getState('alarm3', false);
        state.setValue('active', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Alarm4) {
        state = getState('alarm4', false);
        state.setValue('active', 7);
    }
    else if (sceneID === Scene.WindActive) {
        if (groupID === 0) {
            state = getOrRegisterState('wind');
            state.setValue('active', 7);
        } else if (groupID >= 16 && groupID <= 23) {
            state = getOrRegisterState('wind.group' + groupID);
            state.setValue('active', 7);
        }
    }
    else if (sceneID === Scene.WindInactive) {
        if (groupID === 0) {
            state = getOrRegisterState('wind');
            state.setValue('inactive', 7);
            /* do not auto-clear wind state in user groups */
        } else if (groupID >= 16 && groupID <= 23) {
            state = getOrRegisterState('wind.group' + groupID);
            state.setValue('inactive', 7);
        }
    }
    else if (sceneID === Scene.RainActive) {
        if (groupID === 0) {
            state = getOrRegisterState('rain');
            state.setValue('active', 7);
        } else if (groupID >= 16 && groupID <= 23) {
            state = getOrRegisterState('rain.group' + groupID);
            state.setValue('active', 7);
        }
    }
    else if (sceneID === Scene.RainInactive) {
        if (groupID === 0) {
            state = getOrRegisterState('rain');
            state.setValue('inactive', 7);
            for (grp = 16; grp <= 23; grp++) {
                try {
                    state = getState('rain.group' + groupID, false);
                    state.setValue('inactive', 7);
                } catch (e) {
                }
            }
        } else if (groupID >= 16 && groupID <= 23) {
            state = getOrRegisterState('rain.group' + groupID);
            state.setValue('inactive', 7);
        }
    }
    else if (sceneID === Scene.HailActive) {
        if (groupID === 0) {
            state = getOrRegisterState('hail');
            state.setValue('active', 7);
        } else if (groupID >= 16 && groupID <= 23) {
            state = getOrRegisterState('hail.group' + groupID);
            state.setValue('active', 7);
        }
    }
    else if (sceneID === Scene.HailInactive) {
        if (groupID === 0) {
            state = getOrRegisterState('hail');
            state.setValue('inactive', 7);
            for (grp = 16; grp <= 23; grp++) {
                try {
                    state = getState('hail.group' + groupID, false);
                    state.setValue('inactive', 7);
                } catch (e) {
                }
            }
        } else if (groupID >= 16 && groupID <= 23) {
            state = getOrRegisterState('hail.group' + groupID);
            state.setValue('inactive', 7);
        }
    }
}

function undoscene()
{
    var sceneID = parseInt(raisedEvent.parameter.sceneID);
    var zoneID = parseInt(raisedEvent.source.zoneID);
    var groupID = parseInt(raisedEvent.source.groupID);
    var state;

    var originDeviceId = 0;
    if (raisedEvent.source.isGroup) {
        originDeviceId = parseInt(raisedEvent.parameter.originDeviceID, 16);
    } else if (raisedEvent.source.dsid) {
        originDeviceId = parseInt(raisedEvent.source.dsid, 16);
    }

    if (groupID === 0 && sceneID === Scene.Panic) {
        state = getState('panic', false);
        state.setValue('inactive', 7);

        // #2561: auto-reset fire if panic was reset by a button
        state = getState('fire', false);
        if ((originDeviceId >= 16) && (state.getValue() == 1)) {
            var z = getZoneByID(0);
            z.undoSceneSys(0, Scene.Fire, "manual");
        }
    }
    else if (groupID === 0 && sceneID === Scene.Fire) {
        state = getState('fire', false);
        state.setValue('inactive', 7);

        // #2561: auto-reset panic if fire was reset by a button
        state = getState('panic', false);
        if ((originDeviceId >= 16) && (state.getValue() == 1)) {
            var z = getZoneByID(0);
            z.undoSceneSys(0, Scene.Panic, "manual");
        }
    }
    else if (groupID === 0 && sceneID === Scene.Alarm) {
        state = getState('alarm', false);
        state.setValue('inactive', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Alarm2) {
        state = getState('alarm2', false);
        state.setValue('inactive', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Alarm3) {
        state = getState('alarm3', false);
        state.setValue('inactive', 7);
    }
    else if (groupID === 0 && sceneID === Scene.Alarm4) {
        state = getState('alarm4', false);
        state.setValue('inactive', 7);
    }
}

/**
1 Präsenz
2 Helligkeit (Raum)
3 Präsenz bei Dunkelheit
4 Dämmerung (Außen)
5 Bewegung
6 Bewegung bei Dunkelheit
7 Rauchmelder
8 Windwächter
9 Regenwächter
10 Sonneneinstrahlung
11 Raumthermostat
*/

function stateBinaryinput()
{
    var dev = getDevices().byDSID(raisedEvent.source.dsid);
    if (dev) {
        var devNode = dev.getPropertyNode();
        var stateNode = Property.getNode('/usr/states/' + raisedEvent.parameter.statename);
        if (stateNode == null) {
            return;
        }
        var inputIndex = stateNode.getChild('device/inputIndex').getValue();
        var devInput = devNode.getChild('binaryInputs/binaryInput' + inputIndex);
        if (devInput) {
            var inputType = devInput.getChild('inputType').getValue();
            var inputId = devInput.getChild('inputId').getValue();
            var targetType = devInput.getChild('targetGroupType').getValue();
            var targetId = devInput.getChild('targetGroupId').getValue();

            if (inputId != 15) {
                return;
            }

            // Motion
            if (inputType === 5 || inputType === 6) {
                var stateName;
                if (targetId >= 16) {
                // create state for a user group if it does not exist (new group?)
                    stateName = 'zone.0.group.' + targetId + '.motion';
                } else {
                // set motion state in the zone the dsid is logical attached to
                    stateName = 'zone.' + dev.zoneID + '.motion';
                }
                getOrRegisterState(stateName);
                stateBinaryInputGeneric(stateName, targetType, targetId, raisedEvent.parameter);
            }

            // Presence
            if (inputType === 1 || inputType === 3) {
                var stateName;
                if (targetId >= 16) {
                // create state for a user group if it does not exist (new group?)
                    stateName = 'zone.0.group.' + targetId + '.presence';
                } else {
                // set presence state in the zone the dsid is logical attached to
                    stateName = 'zone.' + dev.zoneID + '.presence';
                }
                getOrRegisterState(stateName);
                stateBinaryInputGeneric(stateName, targetType, targetId, raisedEvent.parameter);
            }

            // Smoke Detector
            if (inputType === 7) {
                var stateName = 'fire';
                var state = getState(stateName, false);
                if (raisedEvent.parameter.value == "1") {
                    state.setValue('active');
                }
            }

            // Wind Monitor
            if (inputType === 8) {
                var stateName = 'wind';

                // create state for a user group if it does not exist (new group?)
                if (targetId >= 16) {
                    stateName = stateName + '.group' + targetId;
                    getOrRegisterState(stateName);
                }
                stateBinaryInputGeneric(stateName, targetType, targetId, raisedEvent.parameter);
            }

            // Rain Monitor
            if (inputType === 9) {
                var stateName = 'rain';

                // create state for a user group if it does not exist (new group?)
                if (targetId >= 16) {
                    stateName = stateName + '.group' + targetId;
                    getOrRegisterState(stateName);
                }
                stateBinaryInputGeneric(stateName, targetType, targetId, raisedEvent.parameter);
            }

        }
    }
}

function stateBinaryInputGeneric(stateName, targetType, targetId, parameter)
{
    var state = getState(stateName, false);

    var cntNode = Property.getNode(stateName + '.' + targetType + '.' + targetId);
    if (cntNode == null) {
        cntNode = new Property(stateName + '.' + targetType + '.' + targetId);
        cntNode.setValue(0);
    }

    if (parameter.value == "1") {
        cntNode.setValue(cntNode.getValue() + 1);
        state.setValue("active");
    }
    if (parameter.value == "2") {
        if (cntNode.getValue() > 0) {
            cntNode.setValue(cntNode.getValue() - 1);
        }
        if (cntNode.getValue() == 0) {
            state.setValue("inactive");
        }
    }
}

function stateApartment()
{
    var stateName = raisedEvent.parameter.statename;
    var groupId = 0;
    var groupName = stateName.indexOf('.group');
    if (groupName > 0) {
        groupId = parseInt(stateName.substring(groupName + 6));
    }
    var z = getZoneByID(0);

    var originDeviceId = 0;
    if (raisedEvent.source.dsid) {
        originDeviceId = parseInt(raisedEvent.source.dsid, 16);
    } else {
        originDeviceId = parseInt(raisedEvent.parameter.originDeviceID, 16);
    }
    if (originDeviceId == 7) {
        // ignore scene calls originated by server generated system level events,
        // e.g. scene calls issued by state changes
        return;
    }

    if (stateName == 'fire') {
        if (raisedEvent.parameter.value == '1') {
            z.callSceneSys(0, Scene.Fire, false, "manual");
        }
    }
    if (stateName.substring(0, 4) == 'rain') {
        if (raisedEvent.parameter.value == '1') {
            z.callSceneSys(groupId, Scene.RainActive, false, "manual");
        }
        if (raisedEvent.parameter.value == '2') {
            z.callSceneSys(groupId, Scene.RainInactive, false, "manual");
        }
    }
    if (stateName.substring(0, 4) == 'wind') {
        if (raisedEvent.parameter.value == '1') {
            z.callSceneSys(groupId, Scene.WindActive, false, "manual");
        }
        if (raisedEvent.parameter.value == '2') {
            z.callSceneSys(groupId, Scene.WindInactive, false, "manual");
        }
    }
}

if (raisedEvent.name == 'running') {
    bootstrap();
}
else if (raisedEvent.name == 'model_ready') {
    startup();
}
else if (raisedEvent.name == 'callScene' && raisedEvent.source.isGroup) {
    callscene();
}
else if (raisedEvent.name == 'undoScene') {
    undoscene();
}
else if (raisedEvent.name == 'stateChange'  && raisedEvent.source.isDevice) {
    stateBinaryinput();
}
else if (raisedEvent.name == 'stateChange'  && raisedEvent.source.isService) {
    stateApartment();
}
