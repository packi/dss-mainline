/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "devicerequesthandler.h"

#include <boost/bind.hpp>
#include <limits.h>

#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/structuremanipulator.h"
#include "src/stringconverter.h"

#include "src/web/json.h"

namespace dss {


  //=========================================== DeviceRequestHandler

  DeviceRequestHandler::DeviceRequestHandler(Apartment& _apartment,
                                             StructureModifyingBusInterface* _pStructureBusInterface,
                                             StructureQueryBusInterface* _pStructureQueryBusInterface)
  : m_Apartment(_apartment),
    m_pStructureBusInterface(_pStructureBusInterface),
    m_pStructureQueryBusInterface(_pStructureQueryBusInterface)
  { }

  class DeviceNotFoundException : public std::runtime_error {
  public:
    DeviceNotFoundException(const std::string& __arg)
    : std::runtime_error(__arg)
    { }
  };

  boost::shared_ptr<Device> DeviceRequestHandler::getDeviceFromRequest(const RestfulRequest& _request) {
    boost::shared_ptr<Device> result = getDeviceByDSID(_request);
    if(result == NULL) {
      result = getDeviceByName(_request);
    }
    if(result == NULL) {
      throw DeviceNotFoundException("Need parameter name or dsid to identify device");
    }
    return result;
  } // getDeviceFromRequest

  boost::shared_ptr<Device> DeviceRequestHandler::getDeviceByDSID(const RestfulRequest& _request) {
    boost::shared_ptr<Device> result;
    std::string deviceDSIDString = _request.getParameter("dsid");
    if(!deviceDSIDString.empty()) {
      try {
        dss_dsid_t deviceDSID = dss_dsid_t::fromString(deviceDSIDString);
        try {
          result = m_Apartment.getDeviceByDSID(deviceDSID);
        } catch(std::runtime_error& e) {
          throw DeviceNotFoundException("Could not find device with dsid '" + deviceDSIDString + "'");
        }
      } catch(std::invalid_argument& e) {
        throw DeviceNotFoundException("Could not parse dsid '" + deviceDSIDString + "'");
      }
    }
    return result;
  } // getDeviceByDSID

  boost::shared_ptr<Device> DeviceRequestHandler::getDeviceByName(const RestfulRequest& _request) {
    boost::shared_ptr<Device> result;
    std::string deviceName = _request.getParameter("name");
    if (deviceName.empty()) {
      return result;
    }
    try {
      result = m_Apartment.getDeviceByName(deviceName);
    } catch(std::runtime_error&  e) {
      throw DeviceNotFoundException("Could not find device named '" + deviceName + "'");
    }
    return result;
  } // getDeviceByName

  WebServerResponse DeviceRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    boost::shared_ptr<Device> pDevice;
    StringConverter st("UTF-8", "UTF-8");
    try {
      pDevice = getDeviceFromRequest(_request);
    } catch(DeviceNotFoundException& ex) {
      return failure(ex.what());
    }
    assert(pDevice != NULL);
    if(isDeviceInterfaceCall(_request)) {
      return handleDeviceInterfaceRequest(_request, pDevice);
    } else if(_request.getMethod() == "getSpec") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("functionID", pDevice->getFunctionID());
      resultObj->addProperty("productID", pDevice->getProductID());
      resultObj->addProperty("revisionID", pDevice->getRevisionID());
      return success(resultObj);
    } else if(_request.getMethod() == "getGroups") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      int numGroups = pDevice->getGroupsCount();

      boost::shared_ptr<JSONArrayBase> groups(new JSONArrayBase());
      resultObj->addElement("groups", groups);
      for(int iGroup = 0; iGroup < numGroups; iGroup++) {
        try {
          boost::shared_ptr<Group> group = pDevice->getGroupByIndex(iGroup);
          boost::shared_ptr<JSONObject> groupObj(new JSONObject());
          groups->addElement("", groupObj);

          groupObj->addProperty("id", group->getID());
          if(!group->getName().empty()) {
            groupObj->addProperty("name", group->getName());
          }
        } catch(std::runtime_error&) {
          Logger::getInstance()->log("DeviceRequestHandler: Group only present at device level");
        }
      }
      return success(resultObj);
    } else if(_request.getMethod() == "getState") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("isOn", pDevice->isOn());
      return success(resultObj);
    } else if(_request.getMethod() == "getName") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("name", pDevice->getName());
      return success(resultObj);
    } else if(_request.getMethod() == "setName") {
      if(_request.hasParameter("newName")) {
        std::string name = _request.getParameter("newName");
        pDevice->setName(st.convert(name));

        if (m_pStructureBusInterface != NULL) {
          StructureManipulator manipulator(*m_pStructureBusInterface,
                                           *m_pStructureQueryBusInterface,
                                           m_Apartment);
          manipulator.deviceSetName(pDevice, name);
        }
        return success();
      } else {
        return failure("missing parameter 'newName'");
      }
    } else if(_request.getMethod() == "addTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return failure("missing parameter 'tag'");
      }
      pDevice->addTag(st.convert(tagName));
      return success();
    } else if(_request.getMethod() == "removeTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return failure("missing parameter 'tag'");
      }
      pDevice->removeTag(tagName);
      return success();
    } else if(_request.getMethod() == "hasTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return failure("missing parameter 'tag'");
      }
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("hasTag", pDevice->hasTag(tagName));
      return success(resultObj);
    } else if(_request.getMethod() == "getTags") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      boost::shared_ptr<JSONArray<std::string> > tagsObj(new JSONArray<std::string>());
      resultObj->addElement("tags", tagsObj);
      std::vector<std::string> tags = pDevice->getTags();
      std::for_each(tags.begin(), tags.end(), boost::bind(&JSONArray<std::string>::add, tagsObj.get(), _1));
      return success(resultObj);
    } else if(_request.getMethod() == "lock") {
      if (!pDevice->isPresent()) {
        return failure("Device is not present");
      }
      pDevice->lock();
      return success();
    } else if(_request.getMethod() == "unlock") {
      if (!pDevice->isPresent()) {
        return failure("Device is not present");
      }
      pDevice->unlock();
      return success();
    } else if(_request.getMethod() == "setConfig") {
      int value = strToIntDef(_request.getParameter("value"), -1);
      if((value  < 0) || (value > UCHAR_MAX)) {
        return failure("Invalid or missing parameter 'value'");
      }

      int configClass = strToIntDef(_request.getParameter("class"), -1);
      if((configClass < 0) || (configClass > UCHAR_MAX)) {
        return failure("Invalid or missing parameter 'class'");
      }
      int configIndex = strToIntDef(_request.getParameter("index"), -1);
      if((configIndex < 0) || (configIndex > UCHAR_MAX)) {
        return failure("Invalid or missing parameter 'index'");
      }

      pDevice->setDeviceConfig(configClass, configIndex, value);
      return success();
    } else if(_request.getMethod() == "getConfig") {
      int configClass = strToIntDef(_request.getParameter("class"), -1);
      if((configClass < 0) || (configClass > UCHAR_MAX)) {
        return failure("Invalid or missing parameter 'class'");
      }
      int configIndex = strToIntDef(_request.getParameter("index"), -1);
      if((configIndex < 0) || (configIndex > UCHAR_MAX)) {
        return failure("Invalid or missing parameter 'index'");
      }

      uint8_t value = pDevice->getDeviceConfig(configClass, configIndex);

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("class", configClass);
      resultObj->addProperty("index", configIndex);
      resultObj->addProperty("value", value);

      return success(resultObj);
    } else if(_request.getMethod() == "getConfigWord") {
      int configClass = strToIntDef(_request.getParameter("class"), -1);
      if((configClass < 0) || (configClass > UCHAR_MAX)) {
        return failure("Invalid or missing parameter 'class'");
      }
      int configIndex = strToIntDef(_request.getParameter("index"), -1);
      if((configIndex < 0) || (configIndex > UCHAR_MAX)) {
        return failure("Invalid or missing parameter 'index'");
      }

      uint16_t value = pDevice->getDeviceConfigWord(configClass, configIndex);

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("class", configClass);
      resultObj->addProperty("index", configIndex);
      resultObj->addProperty("value", value);

      return success(resultObj);
    } else if(_request.getMethod() == "setJokerGroup") {
      int value = strToIntDef(_request.getParameter("groupID"), -1);
      if((value  < 1) || (value > 8)) {
        return failure("Invalid or missing parameter 'groupID'");
      }
      pDevice->setDeviceJokerGroup(value);
      return success();
    } else if(_request.getMethod() == "setButtonID") {
      int value = strToIntDef(_request.getParameter("buttonID"), -1);
      if((value  < 0) || (value > 15)) {
        return failure("Invalid or missing parameter 'buttonID'");
      }
      pDevice->setDeviceButtonID(value);
      return success();
    } else if(_request.getMethod() == "setButtonInputMode") {
      int value = strToIntDef(_request.getParameter("modeID"), -1);
      if(value  < 0) {
        return failure("Invalid or missing parameter 'modeID'");
      }
      pDevice->setDeviceButtonInputMode(value);
      return success();
    } else if(_request.getMethod() == "setOutputMode") {
      int value = strToIntDef(_request.getParameter("modeID"), -1);
      if((value  < 0) || (value > 255)) {
        return failure("Invalid or missing parameter 'modeID'");
      }
      pDevice->setDeviceOutputMode(value);
      return success();
    } else if(_request.getMethod() == "setProgMode") {
      uint8_t modeId;
      std::string pmode = _request.getParameter("mode");
      if(pmode.compare("enable") == 0) {
        modeId = 1;
      } else if(pmode.compare("disable") == 0) {
        modeId = 0;
      } else {
        return failure("Invalid or missing parameter 'mode'");
      }
      pDevice->setProgMode(modeId);
      return success();
    } else if(_request.getMethod() == "getTransmissionQuality") {
      std::pair<uint8_t, uint16_t> p = pDevice->getDeviceTransmissionQuality();
      uint8_t down = p.first;
      uint16_t up = p.second;
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("upstream", up);
      resultObj->addProperty("downstream", down);
      return success(resultObj);

    } else if(_request.getMethod() == "getOutputValue") {
      int offset = strToIntDef(_request.getParameter("offset"), -1);
      if((offset  < 0) || (offset > 255)) {
        return failure("Invalid or missing parameter 'offset'");
      }
      int value = pDevice->getDeviceOutputValue(offset);
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("offset", offset);
      resultObj->addProperty("value", value);
      return success(resultObj);
    } else if(_request.getMethod() == "setOutputValue") {
      int offset = strToIntDef(_request.getParameter("offset"), -1);
      if((offset  < 0) || (offset > 255)) {
        return failure("Invalid or missing parameter 'offset'");
      }
      int value = strToIntDef(_request.getParameter("value"), -1);
      if((value  < 0) || (value > 65535)) {
        return failure("Invalid or missing parameter 'value'");
      }
      pDevice->setDeviceOutputValue(offset, value);
      return success();

    } else if(_request.getMethod() == "getSceneMode") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 255)) {
        return failure("Invalid or missing parameter 'sceneID'");
      }
      DeviceSceneSpec_t config;
      pDevice->getDeviceSceneMode(id, config);
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("sceneID", id);
      resultObj->addProperty("dontCare", config.dontcare);
      resultObj->addProperty("localPrio", config.localprio);
      resultObj->addProperty("specialMode", config.specialmode);
      resultObj->addProperty("flashMode", config.flashmode);
      resultObj->addProperty("ledconIndex", config.ledconIndex);
      resultObj->addProperty("dimtimeIndex", config.dimtimeIndex);
      return success(resultObj);
    } else if(_request.getMethod() == "setSceneMode") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 255)) {
        return failure("Invalid or missing parameter 'sceneID'");
      }
      DeviceSceneSpec_t config;
      pDevice->getDeviceSceneMode(id, config);
      if(_request.hasParameter("dontCare"))
        config.dontcare = strToIntDef(_request.getParameter("dontCare"), config.dontcare);
      if(_request.hasParameter("localPrio"))
        config.localprio = strToIntDef(_request.getParameter("localPrio"), config.localprio);
      if(_request.hasParameter("specialMode"))
        config.specialmode = strToIntDef(_request.getParameter("specialMode"), config.specialmode);
      if(_request.hasParameter("flashMode"))
        config.flashmode = strToIntDef(_request.getParameter("flashMode"), config.flashmode);
      if(_request.hasParameter("ledconIndex"))
        config.ledconIndex = strToIntDef(_request.getParameter("ledconIndex"), config.ledconIndex);
      if(_request.hasParameter("dimtimeIndex"))
        config.dimtimeIndex = strToIntDef(_request.getParameter("dimtimeIndex"), config.dimtimeIndex);
      pDevice->setDeviceSceneMode(id, config);
      return success();

    } else if(_request.getMethod() == "getTransitionTime") {
      int id = strToIntDef(_request.getParameter("dimtimeIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return failure("Invalid or missing parameter 'dimtimeIndex'");
      }
      int up, down;
      pDevice->getDeviceTransitionTime(id, up, down);
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("dimtimeIndex", id);
      resultObj->addProperty("up", up);
      resultObj->addProperty("down", down);
      return success(resultObj);
    } else if(_request.getMethod() == "setTransitionTime") {
      int id = strToIntDef(_request.getParameter("dimtimeIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return failure("Invalid or missing parameter 'dimtimeIndex'");
      }
      int up = strToIntDef(_request.getParameter("up"), -1);
      int down = strToIntDef(_request.getParameter("down"), -1);
      pDevice->setDeviceTransitionTime(id, up, down);
      return success();

    } else if(_request.getMethod() == "getLedMode") {
      int id = strToIntDef(_request.getParameter("ledconIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return failure("Invalid or missing parameter 'ledconIndex'");
      }
      DeviceLedSpec_t config;
      pDevice->getDeviceLedMode(id, config);
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("ledconIndex", id);
      resultObj->addProperty("colorSelect", config.colorSelect);
      resultObj->addProperty("modeSelect", config.modeSelect);
      resultObj->addProperty("dimMode", config.dimMode);
      resultObj->addProperty("rgbMode", config.rgbMode);
      resultObj->addProperty("groupColorMode", config.groupColorMode);
      return success(resultObj);
    } else if(_request.getMethod() == "setLedMode") {
      int id = strToIntDef(_request.getParameter("ledconIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return failure("Invalid or missing parameter 'ledconIndex'");
      }
      DeviceLedSpec_t config;
      pDevice->getDeviceLedMode(id, config);
      if(_request.hasParameter(""))
        config.colorSelect = strToIntDef(_request.getParameter("colorSelect"), config.colorSelect);
      if(_request.hasParameter(""))
        config.modeSelect = strToIntDef(_request.getParameter("modeSelect"), config.modeSelect);
      if(_request.hasParameter(""))
        config.dimMode = strToIntDef(_request.getParameter("dimMode"), config.dimMode);
      if(_request.hasParameter(""))
        config.rgbMode = strToIntDef(_request.getParameter("rgbMode"), config.rgbMode);
      if(_request.hasParameter(""))
        config.groupColorMode = strToIntDef(_request.getParameter("groupColorMode"), config.groupColorMode);
      pDevice->setDeviceLedMode(id, config);
      return success();

    } else if(_request.getMethod() == "getSensorValue") {
      int id = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if((id < 0) || (id > 255)) {
        return failure("Invalid or missing parameter 'sensorIndex'");
      }
      int value = pDevice->getDeviceSensorValue(id);
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("sensorIndex", id);
      resultObj->addProperty("sensorValue", value);
      return success(resultObj);
    } else if(_request.getMethod() == "getSensorType") {
      int id = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if((id < 0) || (id > 255)) {
        return failure("Invalid or missing parameter 'sensorIndex'");
      }
      int value = pDevice->getDeviceSensorType(id);
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("sensorIndex", id);
      resultObj->addProperty("sensorType", value);
      return success(resultObj);

    } else if(_request.getMethod() == "getSensorEventTableEntry") {
      int id = strToIntDef(_request.getParameter("eventIndex"), -1);
      if((id < 0) || (id > 15)) {
        return failure("Invalid or missing parameter 'eventIndex'");
      }
      DeviceSensorEventSpec_t event;
      pDevice->getSensorEventEntry(id, event);
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("eventIndex", id);
      resultObj->addProperty("eventName", event.name);
      resultObj->addProperty("isSceneDevice", pDevice->isSceneDevice());
      resultObj->addProperty("sensorIndex", event.sensorIndex);
      resultObj->addProperty("test", event.test);
      resultObj->addProperty("action", event.action);
      resultObj->addProperty("value", event.value);
      resultObj->addProperty("hysteresis", event.hysteresis);
      resultObj->addProperty("validity", event.validity);
      if (event.action == 2) {
        if (!pDevice->isSceneDevice()) {
          resultObj->addProperty("buttonNumber", event.buttonNumber);
          resultObj->addProperty("clickType", event.clickType);
        } else {
          resultObj->addProperty("sceneDeviceMode", event.sceneDeviceMode);
          resultObj->addProperty("sceneID", event.sceneID);
        }
      }
      return success(resultObj);

    } else if(_request.getMethod() == "setSensorEventTableEntry") {
      int id = strToIntDef(_request.getParameter("eventIndex"), -1);
      if((id < 0) || (id > 15)) {
        return failure("Invalid or missing parameter 'eventIndex'");
      }
      DeviceSensorEventSpec_t event;
      event.name = st.convert(_request.getParameter("eventName"));
      event.sensorIndex = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if ((event.sensorIndex < 0) || (event.sensorIndex > 0xF)) {
        return failure("Invalid or missing parameter 'sensorIndex'");
      }
      event.test = strToIntDef(_request.getParameter("test"), -1);
      if ((event.test < 0) || (event.test > 0x3)) {
        return failure("Invalid or missing parameter 'test'");
      }
      event.action = strToIntDef(_request.getParameter("action"), -1);
      if ((event.action < 0) || (event.action > 0x3)) {
        return failure("Invalid or missing parameter 'action'");
      }
      event.value = strToIntDef(_request.getParameter("value"), -1);
      if ((event.value < 0) || (event.value > 0xFFF)) {
        return failure("Invalid or missing parameter 'value'");
      }
      event.hysteresis = strToIntDef(_request.getParameter("hysteresis"), -1);
      if ((event.hysteresis < 0) || (event.hysteresis > 0xFFF)) {
        return failure("Invalid or missing parameter 'hysteresis'");
      }
      event.validity = strToIntDef(_request.getParameter("validity"), -1);
      if ((event.validity < 0) || (event.validity > 0x3)) {
        return failure("Invalid or missing parameter 'validity'");
      }
      if (event.action == 2) {
        if (!pDevice->isSceneDevice()) {
          event.buttonNumber = strToIntDef(_request.getParameter("buttonNumber"), -1);
          if ((event.buttonNumber < 0) || (event.buttonNumber > 0xF)) {
            return failure("Invalid or missing parameter 'buttonNumber'");
          }
          event.clickType = strToIntDef(_request.getParameter("clickType"), -1);
          if ((event.clickType < 0) || (event.clickType > 0xF)) {
            return failure("Invalid or missing parameter 'clickType'");
          }
        } else {
          event.sceneDeviceMode = strToIntDef(_request.getParameter("sceneDeviceMode"), -1);
          if ((event.sceneDeviceMode < 0) || (event.sceneDeviceMode > 0x3)) {
            return failure("Invalid or missing parameter 'sceneDeviceMode'");
          }
          event.sceneID = strToIntDef(_request.getParameter("sceneID"), -1);
          if ((event.sceneID < 0) || (event.sceneID > 0x7F)) {
            return failure("Invalid or missing parameter 'sceneID'");
          }
        }
      }
      pDevice->setSensorEventEntry(id, event);
      return success();
    } else if(_request.getMethod() == "addToArea") {
      int areaScene = strToIntDef(_request.getParameter("areaScene"), -1);
      if (areaScene < 0) {
        return failure("Missing parameter 'areaScene'");
      }
      pDevice->configureAreaMembership(areaScene, true);
      return success();
    } else if(_request.getMethod() == "removeFromArea") {
      int areaScene = strToIntDef(_request.getParameter("areaScene"), -1);
      if (areaScene < 0) {
        return failure("Missing parameter 'areaScene'");
      }
      pDevice->configureAreaMembership(areaScene, false);
      return success();
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // jsonHandleRequest

} // namespace dss
