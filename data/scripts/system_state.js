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
    registerState("alarm", true);
    registerState("alarm2", true);
    registerState("alarm3", true);
    registerState("alarm4", true);
    registerState("panic", true);
    registerState("fire", true);
    registerState("wind", true);
    registerState("rain", true);
    registerState("hail", true);
}

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
            if (gindex <= 0) {
                continue;
            }
            if (gindex >= 16 && gindex <= 23) {
                if (g.getChild('color').getValue() == 2) {
                    registerState('wind.group' + gindex, true);
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
    state = getState('panic');
    if (panic === true && state.getValue() == 'inactive') {
        state.setValue('active');
    }
    else if (panic === false) {
        state.setValue('inactive');
    }

    state = getState('alarm');
    if (alarm === true && state.getValue() == 'inactive') {
        state.setValue('active');
    }
    else if (panic === false) {
        state.setValue('inactive');
    }

    /* fire is always reset on system restart */
    state = getState('fire');
    state.setValue('inactive');
}

function callscene()
{
    var sceneID = parseInt(raisedEvent.parameter.sceneID);
    var zoneID = parseInt(raisedEvent.source.zoneID);
    var groupID = parseInt(raisedEvent.source.groupID);
    var forced = raisedEvent.parameter.forced;
    var pNode;

    if (groupID === 0 && sceneID === Scene.Absent) {
        pNode = Property.getNode('/usr/states/presence');
        pNode.setStatusValue("absent");
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
        state = getState('panic');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.Fire) {
        state = getState('fire');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm) {
        state = getState('alarm');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm2) {
        state = getState('alarm2');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm3) {
        state = getState('alarm3');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm4) {
        state = getState('alarm4');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.WindActive) {
        state = getState('wind');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.WindInactive) {
        state = getState('wind');
        state.setValue('inactive');
    }
    else if (groupID === 0 && sceneID === Scene.RainActive) {
        state = getState('rain');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.RainInactive) {
        state = getState('rain');
        state.setValue('inactive');
    }
    else if (groupID === 0 && sceneID === Scene.HailActive) {
        state = getState('hail');
        state.setValue('active');
    }
    else if (groupID === 0 && sceneID === Scene.HailInactive) {
        state = getState('hail');
        state.setValue('inactive');
    }

    if (raisedEvent.source.isGroup && (sceneID < 64)) {
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
    var state;

    if (groupID === 0 && sceneID === Scene.Panic) {
        state = getState('panic');
        state.setValue('inactive');
    }
    else if (groupID === 0 && sceneID === Scene.Fire) {
        state = getState('fire');
        state.setValue('inactive');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm) {
        state = getState('alarm');
        state.setValue('inactive');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm2) {
        state = getState('alarm2');
        state.setValue('inactive');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm3) {
        state = getState('alarm3');
        state.setValue('inactive');
    }
    else if (groupID === 0 && sceneID === Scene.Alarm4) {
        state = getState('alarm4');
        state.setValue('inactive');
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
        var devStateName = 'dev.' + raisedEvent.source.dsid + '.' + raisedEvent.parameter.inputIndex;
        var stateNode = Property.getNode('/usr/states/' + devStateName);
        var inputIndex = stateNode.getChild('device/inputIndex').getValue();
        var devInput = devNode.getChild('binaryInputs/binaryInput' + inputIndex);

        if (devInput) {
            var inputType = devInput.getChild('inputType').getValue();
            var inputId = devInput.getChild('inputId').getValue();
            var targetType = devInput.getChild('targetGroupType').getValue();
            var targetId = devInput.getChild('targetGroupId').getValue();

            if (inputId != 15) {
                print('stateBinaryinput: ignoring event, device is not in app mode');
                return;
            }

            if (inputType === 7) {
                var stateName = 'fire';
                var state = getState(stateName);
                if (raisedEvent.parameter.value == "1") {
                    state.setValue('active');
                }
            }

            if (inputType === 8) {
                var stateName = 'wind';

                // create state for a user group if it does not exist (new group?)
                if (targetId >= 16) {
                    stateName = stateName + '.group' + targetId;
                    if (! getState(stateName)) {
                        registerState(stateName, true);
                    }
                }
                stateBinaryInputGeneric(stateName, targetType, targetId, raisedEvent.parameter);
            }

            if (inputType === 9) {
                var stateName = 'rain';

                // create state for a user group if it does not exist (new group?)
                if (targetId >= 16) {
                    stateName = stateName + '.group' + targetId;
                    if (! getState(stateName)) {
                        registerState(stateName, true);
                    }
                }
                stateBinaryInputGeneric(stateName, targetType, targetId, raisedEvent.parameter);
            }

        }
    }
}

function stateBinaryInputGeneric(stateName, targetType, targetId, parameter)
{
    var state = getState(stateName);
    var stype = state.type;
    var svalue = state.value;
    var newvalue = parseInt(parameter.inputState);
    var groupId = 0;
    var groupName = stateName.indexOf('.group');
    if (groupName > 0) {
        groupId = parseInt(stateName.substring(groupName + 6));
    }

    // only change "Apartment" states
    if (stype != 0) {
        return;
    }
    if (newvalue == 0) {
        newvalue = 2;
    }

    var cntNode = Property.getNode(stateName + '.' + targetType + '.' + targetId);
    if (cntNode == null) {
        cntNode = new Property(stateName + '.' + targetType + '.' + targetId);
        cntNode.setValue(0);
    }

    if (newvalue == 1) {
        cntNode.setValue(cntNode.getValue() + 1);
        state.setValue("active");
    }
    if (newvalue == 2) {
        if (cntNode.getValue() > 0) {
            cntNode.setValue(cntNode.getValue() - 1);
        }
        if (cntNode.getValue() == 0) {
            state.setValue("inactive");
        }
    }

    if (newvalue != svalue) {

        if (stateName == 'fire') {
            if (newvalue == 1) {
                var z = getZoneByID(0);
                z.callScene(0, Scene.Fire, false);
            }
        }

        if (stateName.substring(0, 4) == 'rain') {
            if (newvalue == 1) {
                var z = getZoneByID(0);
                z.callScene(groupId, Scene.RainActive, false);
            }
            if (newvalue == 2) {
                var z = getZoneByID(0);
                z.callScene(groupId, Scene.RainInactive, false);
            }
        }

        if (stateName.substring(0, 4) == 'wind') {
            if (newvalue == 1) {
                var z = getZoneByID(0);
                z.callScene(groupId, Scene.WindActive, false);
            }
            if (newvalue == 2) {
                var z = getZoneByID(0);
                z.callScene(groupId, Scene.WindInactive, false);
            }
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
else if (raisedEvent.name == 'deviceBinaryInputEvent') {
    stateBinaryinput();
}
