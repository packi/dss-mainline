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

function registerTrigger(tPath, tEventName, tParamObj)
{
    var iIDNew = 0;
    var oOldNode = null;
    var oBaseNode = Property.getNode('/usr/triggers');
    if (oBaseNode != null) {
        var oBaseArray = oBaseNode.getChildren();
        if (oBaseArray != null) {
            for (var iIndex = 0; iIndex < oBaseArray.length; iIndex++) {
                var oEntryNode = oBaseArray[iIndex];
                var iID = parseInt(oEntryNode.name);
                if (iID >= iIDNew) {
                    iIDNew = iID + 1;
                }
                var oPath = oEntryNode.getChild('triggerPath');
                if (oPath != null) {
                    if (oPath.getValue() == tPath) {
                        oOldNode = oEntryNode;
                    }
                }
            }
        }
    }

    var triggerNode;
    if (oOldNode == null) {
        Property.setProperty('/usr/triggers/' + iIDNew + '/id', iIDNew);
        triggerNode = Property.getNode('/usr/triggers/' + iIDNew);
    } else {
        triggerNode = oOldNode;
    }
    Property.setProperty(triggerNode.getPath() + '/triggerPath', tPath)
    Property.setProperty(triggerNode.getPath() + '/relayedEventName', tEventName)
    if (tParamObj != null) {
        tParamString="";
        for (var sKey in tParamObj) {
            tParamString+=sKey+"=" + tParamObj[sKey] + "&";
        }
        if (tParamString != "") {
            tParamString=tParamString.substr(0,tParamString.length-1);
        }

        Property.setProperty(triggerNode.getPath() + '/additionalRelayingParameter', tParamString)
    }
}

function unregisterTrigger(tPath)
{
    var oOldNode = null;
    var oBaseNode = Property.getNode('/usr/triggers');
    if (oBaseNode != null) {
        var oBaseArray = oBaseNode.getChildren();
        if (oBaseArray != null) {
            for (var iIndex = 0; iIndex < oBaseArray.length; iIndex++) {
                var oEntryNode = oBaseArray[iIndex];
                var oPath = oEntryNode.getChild('triggerPath');
                if (oPath != null) {
                    if (oPath.getValue() == tPath) {
                        oOldNode = oEntryNode;
                    }
                }
            }
        }
        if (oOldNode != null) {
            oBaseNode.removeChild(oOldNode);
        } else {
            printf('unregisterTrigger: unknown trigger path: ' + tPath);
        }
    }
}
