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
 *  Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland
 *  Author: Michael Troß <michael.tross@aizo.com>
 */
/*
 * Hail backend service script
 *
 * Provides API event handler:
 * system.hail.config
 * system.hail.state
 *
 * ***
 * Configuration: system.hail.config
 * Parameters:
 *   ifabsent [ array ]
 *     type
 *       zone-scene
 *       device-scene
 *     target
 *       numeric 0 - 31  group number
 *       dsid            device dsid
 *   ifpresent [ array ]
 *     see above
 *
 * Example:
 * /json/event/raise?
 *   name=system.hail.config&
 *   parameter=command={"ifpresent":{"type":"zone-scene","target":"18"},"ifabsent":{"type":"zone-scene","target":"0"}}
 *
 * ***
 * Hail state set/unset: system.hail.state
 * Parameters:
 *   value
 *     1  active
 *     2  inactive
 *
 * Example:
 * /json/event/raise?name=system.hail.state&parameter=value=active
 * /json/event/raise?name=system.hail.state&parameter=value=inactive
 *
 */

var LOGFILE_NAME = "system_hail.log";
var lWarn = {logln:function (o){}};
var lInfo = {logln:function (o){}};
lWarn = new Logger(LOGFILE_NAME);
//lInfo = new Logger(LOGFILE_NAME);

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

function storeSubParameter(path, value) {
    Property.setProperty('settings/' + path, value);
    Property.setFlag('settings/' + path, 'ARCHIVE', true);
}

function storeHailSettings(path, target)
{
    storeSubParameter(path + 'type', 'zone-scene');
    storeSubParameter(path + 'zone', 0);
    storeSubParameter(path + 'group', target);
    storeSubParameter(path + 'scene', Scene.HailActive);
    storeSubParameter(path + 'category', 'algorithm');
}

function storeNohailSettings(path, target)
{
    storeSubParameter(path + 'type', 'zone-scene');
    storeSubParameter(path + 'zone', 0);
    storeSubParameter(path + 'group', target);
    storeSubParameter(path + 'scene', Scene.HailInactive);
    storeSubParameter(path + 'category', 'algorithm');
}

function storeParameter(par)
{
    // cleanup old settings
    var baseNode = Property.getNode('settings');
    if (baseNode != null) {
        lWarn.logln('Config: purge old configuration');
        baseNode.removeChild('present');
        baseNode.removeChild('present-nohail');
        baseNode.removeChild('absent');
        baseNode.removeChild('absent-nohail');
    }

    lWarn.logln('Config: command = ', par.command);
    var value = {};
    try {
        value = JSON.parse(unescape(par.command));
    }
    catch (e) {
        lWarn.logln('Error parsing parameter: ', e);
    }

    // hail configuration for 'present'
    if (value.ifpresent) {
        var ifType = value.ifpresent.type;
        if (ifType == "zone-scene") {
            var ifTarget = parseInt(value.ifpresent.target, 10);
            storeHailSettings('present/actions/0/', ifTarget);
            storeNohailSettings('present-nohail/actions/0/', ifTarget);
        } else {
            lWarn.logln("Config: invalid type parameter: " + ifType);
        }
    }

    // hail configuration for 'absent'
    if (value.ifabsent) {
        var ifType = value.ifabsent.type;
        if (ifType == "zone-scene") {
            var ifTarget = parseInt(value.ifabsent.target, 10);
            storeHailSettings('absent/actions/0/', ifTarget);
            storeNohailSettings('absent-nohail/actions/0/', ifTarget);
        } else {
            lWarn.logln("Config: invalid type parameter: " + ifType);
        }
    }

    Property.store();
}

if (raisedEvent.name === "running")
{
    lInfo.logln("Running, load configuration");

    registerState("hail", true, false);
    Property.load();
}

else if (raisedEvent.name === "system.hail.config")
{
    lInfo.logln("Configure");
    storeParameter(raisedEvent.parameter);
}

else if (raisedEvent.name === "system.hail.state")
{
    lWarn.logln("Set hail state = ", raisedEvent.parameter.value);
    try {
        var state = getState("hail", false);

        var pNode = Property.getNode('/usr/states/presence');
        var present = pNode.getChild('value').getValue() == 'present';

        var actionpath = '/scripts/system_hail/settings/';
        if (present) {
            actionpath += 'present';
        } else {
            actionpath += 'absent';
        }

        if (raisedEvent.parameter.value === "active") {
            state.setValue('active', 7);
            (new Event('action_execute', {path : actionpath} )).raise();
            Property.setProperty('undopath', actionpath + '-nohail');

        } else if (raisedEvent.parameter.value === "inactive") {
            state.setValue('inactive', 7);
            (new Event('action_execute', {path : Property.getProperty('undopath')} )).raise();
            Property.setProperty('undopath', '');
        }
        Property.store();
    }
    catch (e) {
        lWarn.logln('Error setting state: ', e);
    }
}

else if (raisedEvent.name === "callScene")
{
    lInfo.logln('Call Scene Hail/NoHail');

    var originDeviceId = 0;
    if (raisedEvent.source.isGroup) {
        originDeviceId = parseInt(raisedEvent.parameter.originDeviceID, 16);
    } else if (raisedEvent.source.dsid) {
        originDeviceId = parseInt(raisedEvent.source.dsid, 16);
    }
    if (originDeviceId == 1 || originDeviceId == 7) {
        // ignore scene calls originated by server generated system level events,
        // e.g. scene calls issued by state changes
        //return;
    } else if (raisedEvent.parameter.forced) {
        // ignore non-forced calls
    } else if (raisedEvent.source.isGroup) {
        var sceneID = parseInt(raisedEvent.parameter.sceneID, 10);
        var groupID = parseInt(raisedEvent.parameter.groupID, 10);

        if (sceneID === Scene.HailActive) {
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
                for (var grp = 16; grp <= 23; grp++) {
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
}
