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

function checkCondition(pathToDescription)
{
    var oBaseConditionNode = Property.getNode(pathToDescription + '/conditions');
    if (oBaseConditionNode != null)
    {
        var oEnabledNode = oBaseConditionNode.getChild('enabled');
        if (oEnabledNode != null) {
            if (oEnabledNode.getValue() == false) {
                return false;
            }
        }

        // all system states must match
        var oStateNode = oBaseConditionNode.getChild('states');
        var oSystemStates = Property.getNode('/usr/states');
        if (oStateNode != null && oSystemStates) {
            var oStateArrayActual = oSystemStates.getChildren();
            var oStateArrayCondition = oStateNode.getChildren();
            for (var iIndex=0; iIndex < oStateArrayCondition.length; iIndex++)        // check only for requested state, other ignored
            {
                var fFound = false;
                for (var jIndex=0; jIndex < oStateArrayActual.length; jIndex++) {
                    var testStateName = oStateArrayActual[jIndex].getChild('name');
                    var testStateValue = oStateArrayActual[jIndex].getChild('value');
                    if (oStateArrayCondition[iIndex].getName() == testStateName.getValue()) { // search for a requested state
                        fFound = true;                                                 // state found ...
                        if (oStateArrayCondition[iIndex].getValue() != testStateValue.getValue()) {
                            return false;
                        }
                        break;
                    }
                }
                if (oStateArrayCondition[iIndex].getValue() == true) {                 // state was requested as active
                    if (fFound == false) {                                             // but not found -> condition failed  
                        return false;
                    }
                }
            }
        }

        // at least one of the zone states must match
        var oZoneNode = oBaseConditionNode.getChild('zone-states');
        if (oZoneNode != null) {
            var fMatch = false;
            var oStateArrayActual = oZoneNode.getChildren();
            for (var iIndex = 0; iIndex < oStateArrayActual.length; iIndex ++) {
                var testZoneId = oStateArrayActual[iIndex].getChild('zone').getValue();
                var testGroupId = oStateArrayActual[iIndex].getChild('group').getValue();
                var testSceneId = oStateArrayActual[iIndex].getChild('scene').getValue();

                var groupNode = Property.getNode('/apartment/zones/zone' +
                                    testZoneId + '/groups/group' + testGroupId);

                if (groupNode && (groupNode.getChild('lastCalledScene').getValue() == testSceneId)) {
                    fMatch = true;
                    break;
                }
            }
            if (!fMatch) {
                return false;
            }
        }

        var oWeekdayNode = oBaseConditionNode.getChild('weekdays');
        if (oWeekdayNode != null) {
            var sWeekdayString = oWeekdayNode.getValue();
            if (sWeekdayString != null) {
                var oNow = new Date();
                var oWeekDayArray = sWeekdayString.split(",");
                var fFound = false;
                for (var iIndex = 0; iIndex < oWeekDayArray.length; iIndex++)
                    if (parseInt(oWeekDayArray[iIndex]) == oNow.getDay()) {
                        fFound = true;
                        break;
                    }
                if (fFound == false) {
                    return false;
                }
            }
        }

        var oTimeStartNode = oBaseConditionNode.getChild('time-start');
        if (oTimeStartNode != null) {
            var sTime = oTimeStartNode.getValue();
            var oTimeTemp = sTime.split(':');
            if (oTimeTemp.length == 3) {
                var secondsSinceMidnightCondition = (parseInt(oTimeTemp[0]) * 3600) + (parseInt(oTimeTemp[1]) * 60) +  parseInt(oTimeTemp[0]);
                var oNow = new Date();
                var secondsSinceMidnightNow = (oNow.getHours() * 3600) + (oNow.getMinutes() * 3600) + (oNow.getSeconds());
                if (! (secondsSinceMidnightCondition <= secondsSinceMidnightNow)) {
                    return false;
                }
            }
        }

        var oTimeEndNode = oBaseConditionNode.getChild('time-end');
        if (oTimeEndNode != null) {
            var sTime = oTimeEndNode.getValue();
            var oTimeTemp = sTime.split(':');
            if (oTimeTemp.length == 3) {
                var secondsSinceMidnightCondition = (parseInt(oTimeTemp[0]) * 3600) + (parseInt(oTimeTemp[1]) * 60) +  parseInt(oTimeTemp[0]);
                var oNow = new Date();
                var secondsSinceMidnightNow = (oNow.getHours() * 3600) + (oNow.getMinutes() * 3600) + (oNow.getSecounds());
                if (! (secondsSinceMidnightNow <= secondsSinceMidnightCondition)) {
                    return false;
                }
            }
        }
    }
    return true;
}
