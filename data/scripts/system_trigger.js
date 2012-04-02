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

var LOGFILE_NAME = "system_trigger.log";
var l = new Logger(LOGFILE_NAME);

function relayTrigger(oRelay)
{
    if ((oRelay.getChild('triggerPath') == null) ||
        (oRelay.getChild('relayedEventName') == null)) {
        l.logln("Missing trigger properties path or event name in: " + oRelay.getPath());
        return;
    }
    var sTriggerPath = oRelay.getChild('triggerPath').getValue();
    var sRelayedEventName = oRelay.getChild('relayedEventName').getValue();
    var additionalParameter = null;
    if (oRelay.getChild('additionalRelayingParameter') != null) {
        additionalParameter = oRelay.getChild('additionalRelayingParameter').getValue();
    }

    // copy all parameters from original event
    var oObjectParameter = {};
    for (var sKey in raisedEvent.parameter) {
        oObjectParameter[sKey] = raisedEvent.parameter[sKey];
    }
    oObjectParameter.path = sTriggerPath;
    if (additionalParameter != null) {
        try
        {
            var paramsObject = JSON.parse(additionalParameter);
            for (var sKey in paramsObject)
            {
                oObjectParameter[sKey]=paramsObject[sKey];
            }
        } catch (e) {
            l.logln("parseError on additionalParameter " + e);
        }
    }

    // raise event with including the original parameter values,
    // the original 'source' object is not provided
    var evt = new Event(sRelayedEventName, oObjectParameter);
    evt.raise();
}

function checkDeviceScene(triggerProp)
{
    if (raisedEvent.name != 'callScene') {
        return false;
    }
    if (! raisedEvent.source.isDevice) {
        return false;
    }

    var dsid = raisedEvent.source.dsid;
    var scene = raisedEvent.parameter.sceneID;

    if ((scene == null) || (dsid == null)) {
        return false;
    }

    var triggerDSID = triggerProp.getChild('dsid');
    if (triggerDSID == null) {
        return false;
    }
    var triggerSCENE = triggerProp.getChild('scene');
    if (triggerSCENE == null) {
        return false;
    }
    var sDSID = triggerDSID.getValue();
    var iSCENE = triggerSCENE.getValue();
    if ((sDSID == "-1") || (sDSID == dsid)) {
        if ((iSCENE == scene) || (iSCENE == -1)) {
            l.logln("*** Match: DeviceScene dSID:" + sDSID + ", Scene: " + iSCENE);
            return true;
        }
    }
    return false;
}

function checkSceneZone(triggerProp)
{
    if (raisedEvent.name != 'callScene') {
        return false;
    }
    if (!raisedEvent.source.isGroup) {
        return false;
    }

    var zone = raisedEvent.source.zoneID;
    var group = raisedEvent.source.groupID;
    var scene = raisedEvent.parameter.sceneID;
    var originDevice = raisedEvent.parameter.originDeviceID;

    if ((zone == null) || (group == null) || (scene == null)) {
        return false;
    }

    var triggerZone = triggerProp.getChild('zone');
    if (triggerZone == null) {
        return false;
    }
    var iZone = triggerZone.getValue();
    if ((iZone >= 0) && (iZone != zone)) {
        return false;
    }

    var triggerGroup = triggerProp.getChild('group');
    if (triggerGroup == null) {
        return true;
    }
    var iGroup = triggerGroup.getValue();
    if ((iGroup >= 0) && (iGroup != group)) {
        return false;
    }

    var triggerScene = triggerProp.getChild('scene');
    if (triggerScene == null) {
        return true;
    }
    var iScene = triggerScene.getValue();
    if ((iScene >= 0) && (iScene != scene)) {
        return false;
    }

    var triggerDSID = triggerProp.getChild('dsid');
    var iDevice = undefined;
    if (triggerDSID != null) {
        iDevice = triggerDSID.getValue();
        if ((iDevice != '') && (iDevice != '-1') && (iDevice != originDevice)) {
            return false;
        }
    }

    l.logln("*** Match: CallScene Zone:" + iZone + ", Group:" + iGroup + ", Scene:" + iScene + ", Origin:" + iDevice);
    return true;
}

function checkDeviceSensor(triggerProp)
{
    if (raisedEvent.name != 'deviceSensorEvent') {
        return false;
    }

    var dsid = raisedEvent.source.dsid;
    var eventName = raisedEvent.parameter.sensorEvent;
    var eventIndex = raisedEvent.parameter.sensorIndex;

    var triggerDSID = triggerProp.getChild('dsid');
    if (triggerDSID == null) {
        return false;
    }
    var sDSID = triggerDSID.getValue();
    if ( ! ((sDSID == "-1") || (sDSID == dsid))) {
        return false;
    }

    var triggerEventId = triggerProp.getChild('eventid');
    var triggerName = triggerProp.getChild('evt');

    if (triggerEventId == null) {
        if (triggerName == null) {
            return false;
        }
        var iName = triggerName.getValue();
        if ((iName == eventName) || (iName == -1)) {
            l.logln("*** Match: SensorEvent dSID: " + sDSID + " EventName: " + iName);
            return true;
        }
        return false;
    }

    var iEventId = triggerEventId.getValue();
    if ((iEventId == eventIndex) || (iEventId == -1)) {
        l.logln("*** Match: SensorEvent dSID: " + sDSID + " EventId: " + iEventId);
        return true;
    }

    return false;
}

function checkDevice(triggerProp)
{
    if (raisedEvent.name != 'buttonClick') {
        return false;
    }

    var dsid = raisedEvent.source.dsid;
    var clickType = raisedEvent.parameter.clickType;

    if (dsid == null) {
        return false;
    }

    if (clickType == null) {
        return false;
    }

    var triggerDSID = triggerProp.getChild('dsid');
    if (triggerDSID == null) {
        return false;
    }
    var triggerMSG = triggerProp.getChild('msg');
    if (triggerMSG == null) {
        return false;
    }

    var sDSID = triggerDSID.getValue();
    var iMSG = triggerMSG.getValue();
    if ((sDSID == "-1") || (sDSID == dsid)) {
        if ((iMSG == clickType) || (iMSG == -1)) {
            l.logln("*** Match: ButtonClick dSID:" + sDSID + ", Klick: " + iMSG);
            return true;
        }
    }
    return false;
}

function checkHighlevel(triggerProp)
{
    if (raisedEvent.name != 'highlevelevent') {
        return false;
    }

    var id = raisedEvent.parameter.id;

    if (id == null) {
         return false;
    }

    var triggerEvent = triggerProp.getChild('event');
    if (triggerEvent ==null) {
        return false;
    }
    var sEvent = triggerEvent.getValue();
    if (id == sEvent) {
        l.logln("*** Match: HighLevelEcent EventID:" + id);
        return true;
    }

    return false;
}

function checkTrigger(sTriggerPath) {
    var appProperty = Property.getNode(sTriggerPath);
    if (appProperty == null) {
        return false;
    }
    var appTrigger = appProperty.getChild('triggers');
    if (appTrigger == null) {
        return false;
    }
    var triggerArray = appTrigger.getChildren();
    if (triggerArray == null) {
        return false;
    }

    for (var iIndex = 0; iIndex < triggerArray.length; iIndex++) {
        var triggerProp = triggerArray[iIndex];
        if (triggerProp.getChild('type') != null) {
            try
            {
                switch (triggerProp.getChild('type').getValue()) {
                    case 'zone-scene':
                        if (checkSceneZone(triggerProp)) {
                            return true;
                        }
                        break;
                    case 'device-scene':
                        if (checkDeviceScene(triggerProp)) {
                            return true;
                        }
                        break;
                    case 'device-sensor':
                        if (checkDeviceSensor(triggerProp)) {
                            return true;
                        }
                        break;
                    case 'device-msg':
                        if (checkDevice(triggerProp)) {
                            return true;
                        }
                        break;
                    case 'custom-event':
                        if (checkHighlevel(triggerProp)) {
                            return true;
                        }
                        break;
                }
            }
            catch (e)
            {
                l.logln('Error during trigger evaluation: ' + e);
                for (var i in e) {
                    l.logln(i + ': ' + e[i]);
                }
            }

        }
    }
    return false;
}

function main() {
    var triggerProperty = Property.getNode('/usr/triggers');
    if (triggerProperty == null) {
        return;
    }
    var triggers = triggerProperty.getChildren();
    if (triggers == null) {
        return;
    }
    for (var iIndex = 0; iIndex < triggers.length; iIndex++) {
        var triggerNode = triggers[iIndex];
        if (triggerNode.getChild('triggerPath') != null) {
            var sTriggerPath = triggerNode.getChild('triggerPath').getValue();
            var conditionsOk = checkCondition(sTriggerPath);
            var triggersOk = checkTrigger(sTriggerPath);
            if (conditionsOk && triggersOk) {
                relayTrigger(triggerNode);
            }
        }
    }
}

main();
