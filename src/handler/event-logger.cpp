/*
 *  Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#include "event-logger.h"

#include "event/event_fields.h"
#include "model/apartment.h"
#include "model/cluster.h"
#include "model/data_types.h"
#include "model/device.h"
#include "model/group.h"
#include "model/modelconst.h"
#include "model/modulator.h"
#include "model/set.h"
#include "model/state.h"
#include "model/zone.h"
#include "security/security.h"

namespace dss {
  const std::string ProtectionLog = "system-protection.log";
  const std::string EventLog = "system-event.log";
  const std::string SensorLog = "system-sensor.log";

  SystemEventLog::SystemEventLog() : SystemEvent(), m_evtRaiseLocation(erlApartment) {
  }

  SystemEventLog::~SystemEventLog() {
  }

  std::string SystemEventLog::getZoneName(boost::shared_ptr<Zone> _zone) {
    std::string zName = _zone->getName();
    if (_zone->getID() == 0) {
      zName = "Broadcast";
    }
    return zName + ";" + intToString(_zone->getID());
  }

  std::string SystemEventLog::getGroupName(boost::shared_ptr<Group> _group) {
    std::string gName = _group->getName();
    if (gName.empty()) {
      gName = "Unknown";
    }

    return gName + ";" + intToString(_group->getID());
  }

  std::string SystemEventLog::getSceneName(int _sceneID, int _groupID) {
    return SceneHelper::getSceneName(_sceneID, _groupID) + ";" + intToString(_sceneID);
  }

  std::string SystemEventLog::getDeviceName(std::string _origin_dsuid) {
    std::string devName;

    if (!_origin_dsuid.empty()) {
      try {
        boost::shared_ptr<Device> device =
            DSS::getInstance()->getApartment().getDeviceByDSID(
                                str2dsuid(_origin_dsuid));
        if (device && (!device->getName().empty())) {
            devName = device->getName();
        }
      } catch (ItemNotFoundException &ex) {
        devName = "Unknown";
      }
    }

    devName += ";" + _origin_dsuid;
    return devName;
  }

  std::string SystemEventLog::getCallOrigin(callOrigin_t _call_origin) {
    switch (_call_origin) {
      case coJSScripting:
        return "Scripting";
      case coJSON:
        return "JSON";
      case coSubscription:
        return "Bus-Handler";
      case coTest:
        return "Test";
      case coSystem:
        return "System";
      case coSystemBinaryInput:
        return "System-Binary-Input";
      case coDsmApi:
        return "dSM-API";
      case coUnknown:
        return "(unspecified)";
      default:
        return intToString(_call_origin);
    }
  }

  void SystemEventLog::logLastScene(boost::shared_ptr<ScriptLogger> _logger,
                                    boost::shared_ptr<Zone> _zone,
                                    boost::shared_ptr<Group> _group,
                                    int _scene_id) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_group);
    std::string sceneName = getSceneName(_scene_id, _group->getID());

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";Last Scene;" + sceneName + ';' + zoneName + ';' +
                  groupName + ";;;");
  }

  void SystemEventLog::logZoneGroupScene(boost::shared_ptr<ScriptLogger> _logger,
                                         boost::shared_ptr<Zone> _zone,
                                         int _group_id, int _scene_id,
                                         bool _is_forced,
                                         std::string _origin_dsuid,
                                         callOrigin_t _call_origin,
                                         std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string sceneName = getSceneName(_scene_id, _group_id);
    std::string devName = getDeviceName(_origin_dsuid);
    if (_is_forced) {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";CallSceneForced;" + sceneName + ";" + zoneName + ";" +
                     groupName + ";" + devName + ";" +
                     getCallOrigin(_call_origin) + ";" + _origin_token);
    } else {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";CallScene;" + sceneName + ";" + zoneName + ";" +
                     groupName + ";" + devName +";" +
                     getCallOrigin(_call_origin) + ";" + _origin_token);
    }
  }

  void SystemEventLog::logDeviceLocalScene(
                                        boost::shared_ptr<ScriptLogger> _logger,
                                        int _scene_id,
                                        std::string _origin_dsuid,
                                        callOrigin_t _call_origin) {
    std::string devName = getDeviceName(_origin_dsuid);
    std::string sceneName = getSceneName(_scene_id, -1);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";Device;" + sceneName + ";;;;;" + devName + ";" +
                   getCallOrigin(_call_origin) + ";");
  }

  void SystemEventLog::logDeviceScene(boost::shared_ptr<ScriptLogger> _logger,
                                      boost::shared_ptr<const Device> _device,
                                      boost::shared_ptr<Zone> _zone,
                                      int _scene_id, bool _is_forced,
                                      std::string _origin_dsuid,
                                      callOrigin_t _call_origin,
                                      std::string _token) {
    std::string zoneName = getZoneName(_zone);
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string origName = getDeviceName(_origin_dsuid);
    std::string sceneName = getSceneName(_scene_id, -1);

    if (_is_forced) {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";DeviceSceneForced;" + sceneName + ";" + zoneName + ";" +
                     devName + ";" + origName + ";" +
                     getCallOrigin(_call_origin) + ";" + _token);
    } else {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";DeviceScene;" + sceneName + ";" + zoneName + ";" +
                     devName + ";" + origName + ";" +
                     getCallOrigin(_call_origin) + ";" + _token);
    }
  }

  void SystemEventLog::logZoneGroupBlink(
                                        boost::shared_ptr<ScriptLogger> _logger,
                                        boost::shared_ptr<Zone> _zone,
                                        int _group_id,
                                        std::string _origin_dsuid,
                                        callOrigin_t _call_origin,
                                        std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string devName = getDeviceName(_origin_dsuid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";Blink;;" + zoneName + ";" + groupName + ";" + devName +
                   ";;" + getCallOrigin(_call_origin) + ";" +_origin_token);
  }

  void SystemEventLog::logDeviceBlink(boost::shared_ptr<ScriptLogger> _logger,
                                      boost::shared_ptr<const Device> _device,
                                      boost::shared_ptr<Zone> _zone,
                                      std::string _origin_dsuid,
                                      callOrigin_t _call_origin,
                                      std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string origName = getDeviceName(_origin_dsuid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";DeviceBlink;;" + zoneName + ";" + devName + ";;" +
                   origName + ";" + getCallOrigin(_call_origin) + ";" + _origin_token);
  }

  void SystemEventLog::logZoneGroupUndo(boost::shared_ptr<ScriptLogger> _logger,
                                        boost::shared_ptr<Zone> _zone,
                                        int _group_id, int _scene_id,
                                        std::string _origin_dsuid,
                                        callOrigin_t _call_origin,
                                        std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string devName = getDeviceName(_origin_dsuid);
    std::string sceneName = getSceneName(_scene_id, _group_id);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";UndoScene;" + sceneName + ";" + zoneName + ";" +
                   groupName + ";" + devName + ";" +
                   getCallOrigin(_call_origin) + ";" + _origin_token);
  }

  void SystemEventLog::logDeviceButtonClick(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string keynum;
    if (m_properties.has("buttonIndex")) {
      keynum = m_properties.get("buttonIndex");
    }

    std::string clicknum;
    if (m_properties.has("clickType")) {
      clicknum = m_properties.get("clickType");
    }

    std::string holdcount;
    if (m_properties.has("holdCount")) {
      holdcount = m_properties.get("holdCount");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    if (holdcount.empty()) {
      _logger->logln(";ButtonClick;Type " + clicknum + ";" + keynum +
                     ";;;;;" + devName + ";");
    } else {
      _logger->logln(";ButtonClick;Hold " + holdcount + ';' + keynum +
                     ";;;;;" + devName + ";");
    }
  }

  void SystemEventLog::logDirectDeviceAction(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string action;
    if (m_properties.has("actionID")) {
      action = m_properties.get("actionID");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";DirectDeviceAction;Action " + action + ";" + action +
                   ";;;;;" + devName + ";");
  }

  void SystemEventLog::logDeviceNamedAction(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" + dsuid2str(_device->getDSID());
    std::string actionName;
    if (m_properties.has("actionId")) {
      actionName = m_properties.get("actionId");
    }
    _logger->logln(";DeviceNamedAction;" + actionName + ";" + actionName + ";;;;;" + devName + ";");
  }

  void SystemEventLog::logDeviceNamedEvent(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" + dsuid2str(_device->getDSID());
    std::string evtName;
    if (m_properties.has("eventId")) {
      evtName = m_properties.get("eventId");
    }
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    _logger->logln(";DeviceNamedEvent;" + evtName + ";" + value + ";;;;;" + devName + ";");
  }

  void SystemEventLog::logDeviceNamedState(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" + dsuid2str(_device->getDSID());
    std::string stateName;
    if (m_properties.has("stateId")) {
      stateName = m_properties.get("stateId");
    }
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    _logger->logln(";DeviceNamedState;" + stateName + ";" + value + ";;;;;" + devName + ";");
  }

  void SystemEventLog::logDeviceBinaryInput(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());

    std::string index;
    if (m_properties.has("inputIndex")) {
      index = m_properties.get("inputIndex");
    }

    std::string state;
    if (m_properties.has("inputState")) {
      state = m_properties.get("inputState");
    }

    std::string type;
    if (m_properties.has("inputType")) {
      type = m_properties.get("inputType");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";BinaryInput;State " + state + ";" + index + ";;;;;" +
                   devName + ";");
  }

  void SystemEventLog::logDeviceSensorEvent(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());

    std::string sensorEvent;
    if (m_properties.has("sensorEvent")) {
      sensorEvent = m_properties.get("sensorEvent");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";EventTable;" + sensorEvent + ";;;;;;" + devName + ";");
  }

  void SystemEventLog::logDeviceSensorValue(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    boost::shared_ptr<const Device> _device,
                                    boost::shared_ptr<Zone> _zone) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string zoneName = getZoneName(_zone);

    std::string sensorIndex;
    if (m_properties.has("sensorIndex")) {
      sensorIndex = m_properties.get("sensorIndex");
    }

    auto sensorType = SensorType::UnknownType;
    if (m_properties.has("sensorType")) {
      sensorType = static_cast<SensorType>(strToInt(m_properties.get("sensorType")));
    } else {
      try {
        boost::shared_ptr<DeviceSensor_t> pSensor = _device->getSensor(strToInt(sensorIndex));
        sensorType = pSensor->m_sensorType;
      } catch (ItemNotFoundException& ex) {}
    }

    std::string sensorValue;
    if (m_properties.has("sensorValue")) {
      sensorValue = m_properties.get("sensorValue");
    }

    std::string sensorValueFloat;
    if (m_properties.has("sensorValueFloat")) {
      sensorValueFloat = m_properties.get("sensorValueFloat");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";SensorValue;" + sensorTypeName(sensorType) +
        " [" + intToString(static_cast<int>(sensorType)) + '/' + sensorIndex + "];" +
        sensorValueFloat + " [" + sensorValue + "];" +
        zoneName + ";;;" + devName + ";");
  }

  void SystemEventLog::logZoneSensorValue(
      boost::shared_ptr<ScriptLogger> _logger,
      boost::shared_ptr<Zone> _zone,
      int _groupId) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_groupId));
    uint8_t sensorType = strToInt(m_properties.get("sensorType"));
    std::string sensorValue = m_properties.get("sensorValue");
    std::string sensorValueFloat = m_properties.get("sensorValueFloat");

    std::string typeName = sensorTypeName(static_cast<SensorType>(sensorType));
    std::string origName = getDeviceName(m_properties.get("originDSID"));

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";ZoneSensorValue;" +
        typeName + " [" + intToString(sensorType) + "];" +
        sensorValueFloat + " [" + sensorValue + "];" +
        zoneName + ";" + groupName + ";" + origName + ";");
  }

  void SystemEventLog::logStateChange(boost::shared_ptr<ScriptLogger> _logger,
      boost::shared_ptr<const State> _st,
      const std::string& _statename, const std::string& _state, const std::string& _value,
      const std::string& _origin_device_id, const callOrigin_t _callOrigin) {

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken'');

    if (m_raisedAtState->getType() == StateType_Service) {
      std::string devName = getDeviceName(_origin_device_id);
      _logger->logln(";StateApartment;" + _statename + ";" + _value + ";" + _state  + ";;;;" +
          devName + ";" + getCallOrigin(_callOrigin) + ";" );

    } else if (m_raisedAtState->getType() == StateType_Device) {
      boost::shared_ptr<Device> device = m_raisedAtState->getProviderDevice();
      std::string devName = device->getName() + ";" + dsuid2str(device->getDSID());
      _logger->logln(";StateDevice;" + _statename + ";" + _value + ";" + _state + ";;;;" + devName + ";");

    } else if (m_raisedAtState->getType() == StateType_Circuit) {
      boost::shared_ptr<DSMeter> meter = m_raisedAtState->getProviderDsm();
      std::string devName = meter->getName() + ";" + dsuid2str(meter->getDSID());
      _logger->logln(";StateDevice;" + _statename + ";" + _value + ";" + _state + ";;;;" + devName + ";");

    } else if (m_raisedAtState->getType() == StateType_Group) {
      boost::shared_ptr<Group> group = m_raisedAtState->getProviderGroup();
      int groupID = group->getID();
      int zoneID = group->getZoneID();
      boost::shared_ptr<Zone> zone = DSS::getInstance()->getApartment().getZone(zoneID);
      std::string groupName = getGroupName(zone->getGroup(groupID));
      std::string zoneName = getZoneName(zone);
      _logger->logln(";StateGroup;" + _statename + ";" + _value + ";" + _state + ";" +
          zoneName + ";" + groupName + ";;");

    } else if (m_raisedAtState->getType() == StateType_SensorDevice) {
      boost::shared_ptr<Device> device = m_raisedAtState->getProviderDevice();
      std::string devName = device->getName() + ";" + dsuid2str(device->getDSID());
      _logger->logln(";StateSensorDevice;" + _statename + ";" + _value + ";" + _state + ";;;;" + devName + ";");

    } else if (m_raisedAtState->getType() == StateType_SensorZone) {
      boost::shared_ptr<Group> group = m_raisedAtState->getProviderGroup();
      int groupID = group->getID();
      int zoneID = group->getZoneID();
      boost::shared_ptr<Zone> zone = DSS::getInstance()->getApartment().getZone(zoneID);
      std::string groupName = getGroupName(zone->getGroup(groupID));
      std::string zoneName = getZoneName(zone);
      _logger->logln(";StateSensorGroup;" + _statename + ";" + _value + ";" + _state + ";" +
          zoneName + ";" + groupName + ";;");

    } else {
      std::string devName = getDeviceName(_origin_device_id);
      _logger->logln(";StateScript;" + _statename + ";" + _value + ";" + _state + ";;;;" +
          devName + ";" + getCallOrigin(_callOrigin) + ";" );
    }
  }

  void SystemEventLog::logSunshine(boost::shared_ptr<ScriptLogger> _logger,
                                   std::string _value,
                                   std::string _direction,
                                   std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";Sunshine;" + _value + ";;" + _direction + ";;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logFrostProtection(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _value,
                                          std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";FrostProtection;" + _value + ";;;;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logHeatingModeSwitch(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _value,
                                          std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";HeatingModeSwitch;" + _value + ";;;;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logBuildingService(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _value,
                                          std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";BuildingService;" + _value + ";;;;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logExecutionDenied(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _action, std::string _reason) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";ExecutionDenied;" + _action + ";" + _reason + ";;;;;;");
  }

  void SystemEventLog::logOperationLock(boost::shared_ptr<ScriptLogger> _logger,
                                        boost::shared_ptr<Zone> _zone,
                                        int _groupId,
                                        int _lock,
                                        callOrigin_t _call_origin) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_groupId));
    std::string origName = getCallOrigin(_call_origin);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";OperationLock;" +
        intToString(_lock) + ";;" +
        zoneName + ";" + groupName + ";" + origName + ";");
  }

  void SystemEventLog::logDevicesFirstSeen(boost::shared_ptr<ScriptLogger> _logger,
                                            std::string& _dateTime,
                                            std::string& _token,
                                            callOrigin_t _call_origin) {
    std::string origName = getCallOrigin(_call_origin);
    _logger->logln(";SetDevicesFirstSeen;" + _dateTime + ";;" + _token + ";;;;" + origName + ";");
  }

  void SystemEventLog::logHighLevelEvent(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string& _id,
                                          std::string& _name) {
    _logger->logln(";UserDefinedAction;" + _id + ";" + _name + ";;;;;;");
  }

  void SystemEventLog::model_ready() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));

    logger->logln("Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken");

    std::vector<boost::shared_ptr<Zone> > zones = DSS::getInstance()->getApartment().getZones();
    for (size_t z = 0; z < zones.size(); z++) {
      if (!zones.at(z)->isPresent()) {
        continue;
      }

      std::vector<boost::shared_ptr<Group> > groups = zones.at(z)->getGroups();
      for (size_t g = 0; g < groups.size(); g++) {
        int group_id = groups.at(g)->getID();
        if (((group_id > 0) && (group_id < GroupIDStandardMax)) ||
            (group_id == GroupIDControlTemperature)) {
          int scene_id = groups.at(g)->getLastCalledScene();
          if (scene_id >= 0) {
            logLastScene(logger, zones.at(z), groups.at(g), scene_id);
          }
        }
      }
    }
  }



  void SystemEventLog::callScene() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    int sceneId = -1;
    if (m_properties.has("sceneID")) {
        sceneId = strToIntDef(m_properties.get("sceneID"), -1);
    }

    bool isForced = false;
    if (m_properties.has("forced")) {
        if (m_properties.get("forced") == "true") {
          isForced = true;
        }
    }
    int zoneId = -1;
    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = m_properties.get(ef_originDSUID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      // ZoneGroup Action Request
      zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);

        logZoneGroupScene(logger, zone, groupId, sceneId, isForced,
                          originDSUID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    } else if ((m_evtRaiseLocation == erlDevice) &&
               (m_raisedAtDevice != NULL)) {
      zoneId = m_raisedAtDevice->getDevice()->getZoneID();

      if (callOrigin == coUnknown) {
        // DeviceLocal Action Event
        logDeviceLocalScene(logger, sceneId,
                        dsuid2str(m_raisedAtDevice->getDevice()->getDSID()),
                        callOrigin);
      } else {
        // Device Action Request
        try {
          boost::shared_ptr<Zone> zone =
              DSS::getInstance()->getApartment().getZone(zoneId);
          logDeviceScene(logger, m_raisedAtDevice->getDevice(), zone, sceneId,
                         isForced, originDSUID, callOrigin, token);
        } catch (ItemNotFoundException &ex) {}
      }
    }
  }

  void SystemEventLog::blink() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    int zoneId = -1;
    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = m_properties.get(ef_originDSUID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneGroupBlink(logger, zone, groupId, originDSUID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    } else if ((m_evtRaiseLocation == erlDevice) &&
               (m_raisedAtDevice != NULL)) {
      zoneId = m_raisedAtDevice->getDevice()->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logDeviceBlink(logger, m_raisedAtDevice->getDevice(), zone,
                       originDSUID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::undoScene() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    int sceneId = -1;
    if (m_properties.has("sceneID")) {
        sceneId = strToIntDef(m_properties.get("sceneID"), -1);
    }

    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = m_properties.get(ef_originDSUID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      int groupId = m_raisedAtGroup->getID();
      int zoneId = m_raisedAtGroup->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
                            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneGroupUndo(logger, zone, groupId, sceneId, originDSUID,
                         callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::buttonClick() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceButtonClick(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::directDeviceAction() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    if (m_raisedAtDevice != NULL) {
      logDirectDeviceAction(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceNamedAction() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceNamedAction(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceNamedEvent() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceNamedEvent(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceNamedState() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceNamedState(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceBinaryInputEvent() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceBinaryInput(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceSensorEvent() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceSensorEvent(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceSensorValue() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        SensorLog, NULL));
    if (m_raisedAtDevice != NULL) {
      int zoneId = m_raisedAtDevice->getDevice()->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logDeviceSensorValue(logger, m_raisedAtDevice->getDevice(), zone);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::zoneSensorValue() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        SensorLog, NULL));
    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      // Zone Sensor Value
      int zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneSensorValue(logger, zone, groupId);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::stateChange() {
    std::string statename;
    if (m_properties.has("statename")) {
      statename = m_properties.get("statename");
    }
    std::string state;
    if (m_properties.has("state")) {
      state = m_properties.get("state");
    }
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = m_properties.get(ef_originDSUID);
    }
    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }
    if ((m_evtRaiseLocation == erlState) && (m_raisedAtState != NULL)) {
      boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
          EventLog, NULL));
      try {
        logStateChange(logger, m_raisedAtState, statename, state, value, originDSUID, callOrigin);
      } catch (std::exception &ex) {}

      std::string name = statename.substr(0, statename.find("."));
      std::string location = statename.substr(statename.find(".") + 1);
      if (name == "wind" ||
          name == "rain" ||
          name == "hail" ||
          name == "panic" ||
          name == "fire" ||
          name == "alarm" ||
          name == "alarm2" ||
          name == "alarm3" ||
          name == "alarm4" ||
          name == "heating_water_system") {
        logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
            ProtectionLog, NULL));
        try {
          name[0] = toupper(name[0]);
          if (!location.empty()) {
            std::string groupName("group");
            size_t groupPos = location.find(groupName);
            if (std::string::npos != groupPos) {
              groupPos += groupName.length();
              int clusterID = strToInt(location.substr(groupPos));
              boost::shared_ptr<Cluster> cluster = DSS::getInstance()->getApartment().getCluster(clusterID);
              std::string clusterName = cluster->getName();
              if (clusterName.empty()) {
                clusterName = "Cluster #" + intToString(clusterID);
              }
              logger->logln(name + " is " + state + " in " + clusterName);
            } else {
              logger->logln(name + " is " + state);
            }
          } else {
            logger->logln(name + " is " + state);
          }
        } catch (std::exception &ex) {}
      }
    }
  }

  void SystemEventLog::sunshine() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string direction;
    if (m_properties.has("direction")) {
      direction = m_properties.get("direction");
    }
    std::string originDeviceID;
    if (m_properties.has(ef_callOrigin)) {
      originDeviceID = m_properties.get(ef_callOrigin);
    }
    try {
      logSunshine(logger, value, direction, originDeviceID);
    } catch (std::exception &ex) {}

    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        ProtectionLog, NULL));
    try {
      int v = strToInt(value);
      std::string valueString;
      switch(v) {
        case 1: valueString = "active"; break;
        case 2: valueString = "inactive"; break;
        default: valueString = "unknown"; break;
      }
      CardinalDirection_t dir;
      parseCardinalDirection(direction, &dir);
      logger->logln("Sun protection from " + toUIString(dir) + " is " + valueString);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::frostProtection() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string originDeviceID;
    if (m_properties.has(ef_callOrigin)) {
      originDeviceID = m_properties.get(ef_callOrigin);
    }
    try {
      logFrostProtection(logger, value, originDeviceID);
    } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}

    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        ProtectionLog, NULL));
    try {
      int v = strToInt(value);
      std::string valueString;
      switch(v) {
        case 1: valueString = "active"; break;
        case 2: valueString = "inactive"; break;
        default: valueString = "unknown"; break;
      }
      logger->logln("Frost protection is " + valueString);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::heatingModeSwitch() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string originDeviceID;
    if (m_properties.has(ef_callOrigin)) {
      originDeviceID = m_properties.get(ef_callOrigin);
    }
    try {
      logHeatingModeSwitch(logger, value, originDeviceID);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::buildingService() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string originDeviceID;
    if (m_properties.has(ef_callOrigin)) {
      originDeviceID = m_properties.get(ef_callOrigin);
    }
    try {
      logBuildingService(logger, value, originDeviceID);
    } catch (std::exception &ex) {}

    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        ProtectionLog, NULL));
    try {
      int v = strToInt(value);
      std::string valueString;
      switch(v) {
        case 1: valueString = "active"; break;
        case 2: valueString = "inactive"; break;
        default: valueString = "unknown"; break;
      }
      logger->logln("Service protection is " + valueString);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::executionDenied() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));
    std::string action;
    std::string reason;
    if (m_properties.has("source-name")) {
      action = m_properties.get("source-name");
    }
    if (action.empty()) {
      if (m_properties.has("action-name")) {
        action = m_properties.get("action-name");
      }
    }
    if (m_properties.has("reason")) {
      reason = m_properties.get("reason");
    }

    try {
      logExecutionDenied(logger, action, reason);
    } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}

    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        ProtectionLog, NULL));
    try {
      logger->logln("Timer or automatic activity \"" + action + "\": " + reason);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::operationLock(const std::string& _evtName) {
    int lockStatus = -1;
    if (m_properties.has("lock")) {
      lockStatus = strToInt(m_properties.get("lock"));
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
          EventLog, NULL));
      int groupId, zoneId;
      try {
        groupId = m_raisedAtGroup->getID();
        zoneId = m_raisedAtGroup->getZoneID();
        boost::shared_ptr<Zone> zone = DSS::getInstance()->getApartment().getZone(zoneId);
        logOperationLock(logger, zone, groupId, lockStatus, callOrigin);
      } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}

      logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
          ProtectionLog, NULL));
      try {
        boost::shared_ptr<Group> group = DSS::getInstance()->getApartment().getZone(zoneId)->getGroup(groupId);
        std::string lockString;
        switch (lockStatus) {
          case 0: lockString = "inactive"; break;
          case 1: lockString = "active"; break;
          default: lockString = "unknown"; break;
        }
        std::string gName = group->getName();
        if (gName.empty()) {
          gName = std::string("Cluster #") + intToString(groupId);
        }
        if (_evtName == EventName::OperationLock) {
          logger->logln("Operation lock " + lockString + " in " + gName);
        } else if (_evtName == EventName::ClusterConfigLock) {
          logger->logln("Configuration lock " + lockString + " in " + gName + " (from " + getCallOrigin(callOrigin) + ")");
        }
      } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}
    }
  }

  void SystemEventLog::devicesFirstSeen() {

    std::string dateTime;
    if (m_properties.has("dateTime")) {
      dateTime = m_properties.get("dateTime");
    }

    std::string token;
    if (m_properties.has("X-DS-TrackingID")) {
      token = m_properties.get("X-DS-TrackingID");
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));

    try {
      logDevicesFirstSeen(logger, dateTime, token, callOrigin);
    } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}
  }

  void SystemEventLog::highlevelevent() {
    std::string evtId;
    if (m_properties.has("id")) {
      evtId = m_properties.get("id");
    }

    std::string evtName;
    PropertySystem &propSystem(DSS::getInstance()->getPropertySystem());
    PropertyNodePtr triggerProperty = propSystem.getProperty("/scripts/system-addon-user-defined-actions/" + evtId + "/name");
    if (triggerProperty) {
      evtName = triggerProperty->getAsString();
    }

    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        EventLog, NULL));

    try {
      logHighLevelEvent(logger, evtId, evtName);
    } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}
  }

  void SystemEventLog::run() {
    if (DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser(
        "SystemEventLog needs system rights");
    } else {
      return;
    }

    if (m_evtName == EventName::ModelReady) {
      model_ready();
    } else if (m_evtName == EventName::CallScene) {
      callScene();
    } else if (m_evtName == EventName::IdentifyBlink) {
      blink();
    } else if (m_evtName == EventName::UndoScene) {
      undoScene();
    } else if (m_evtName == EventName::DeviceButtonClick) {
      buttonClick();
    } else if (m_evtName == EventName::ButtonDeviceAction) {
      directDeviceAction();
    } else if (m_evtName == EventName::DeviceActionEvent) {
      deviceNamedAction();
    } else if (m_evtName == EventName::DeviceEventEvent) {
      deviceNamedEvent();
    } else if (m_evtName == EventName::DeviceStateEvent) {
      deviceNamedState();
    } else if (m_evtName == EventName::DeviceBinaryInputEvent) {
      deviceBinaryInputEvent();
    } else if (m_evtName == EventName::DeviceSensorEvent) {
      deviceSensorEvent();
    } else if (m_evtName == EventName::StateChange) {
      stateChange();
    } else if (m_evtName == EventName::DeviceSensorValue) {
      deviceSensorValue();
    } else if (m_evtName == EventName::ZoneSensorValue) {
      zoneSensorValue();
    } else if (m_evtName == EventName::Sunshine) {
      sunshine();
    } else if (m_evtName == EventName::FrostProtection) {
      frostProtection();
    } else if (m_evtName == EventName::HeatingModeSwitch) {
      heatingModeSwitch();
    } else if (m_evtName == EventName::BuildingService) {
      buildingService();
    } else if (m_evtName == EventName::ExecutionDenied) {
      executionDenied();
    } else if (m_evtName == EventName::OperationLock || m_evtName == EventName::ClusterConfigLock) {
      operationLock(m_evtName);
    } else if (m_evtName == EventName::DevicesFirstSeen) {
      devicesFirstSeen();
    } else if (m_evtName == EventName::HighLevelEvent) {
      highlevelevent();
    }
  }

  bool SystemEventLog::setup(Event& _event) {
    m_evtName = _event.getName();
    m_evtRaiseLocation = _event.getRaiseLocation();
    m_raisedAtGroup = _event.getRaisedAtGroup(DSS::getInstance()->getApartment());
    m_raisedAtDevice = _event.getRaisedAtDevice();
    m_raisedAtState = _event.getRaisedAtState();
    return SystemEvent::setup(_event);
  }
}; // namespace
