/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include "base.h"
#include "propertysystem.h"

namespace dss {

class Device;

class ActionExecute {
public:
    ActionExecute(const Properties &properties);
    void executeWithDelay(std::string _path, std::string _delay);
    void execute(std::string _path);

private:
    Properties m_properties;

private:
    void executeZoneScene(PropertyNodePtr _actionNode);
    void executeZoneUndoScene(PropertyNodePtr _actionNode);
    void executeDeviceScene(PropertyNodePtr _actionNode);
    void executeDeviceChannelValue(PropertyNodePtr _actionNode);
    void executeDeviceValue(PropertyNodePtr _actionNode);
    void executeDeviceBlink(PropertyNodePtr _actionNode);
    void executeDeviceAction(PropertyNodePtr _actionNode);
    void executeZoneBlink(PropertyNodePtr _actionNode);
    void executeCustomEvent(PropertyNodePtr _actionNode);
    void executeURL(PropertyNodePtr _actionNode);
    void executeStateChange(PropertyNodePtr _actionNode);
    void executeAddonStateChange(PropertyNodePtr _actionNode);
    void executeHeatingMode(PropertyNodePtr _actionNode);
    unsigned int executeOne(PropertyNodePtr _actionNode);
    void executeStep(std::vector<PropertyNodePtr> _actionNodes);
    std::vector<PropertyNodePtr> filterActionsWithDelay(PropertyNodePtr _actionNode, int _delayValue);
    std::string getActionName(PropertyNodePtr _actionNode);
    boost::shared_ptr<Device> getDeviceFromNode(PropertyNodePtr _actionNode);
};

}
