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
#include <digitalSTROM/dsuid/dsuid.h>

#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/zone.h"
#include "src/model/modelconst.h"
#include "src/structuremanipulator.h"
#include "src/stringconverter.h"
#include "src/comm-channel.h"
#include "src/ds485types.h"
#include "src/web/json.h"
#include "jsonhelper.h"
#include "foreach.h"
#include "util.h"

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

  boost::shared_ptr<std::vector<std::pair<int, int> > > DeviceRequestHandler::parseOutputChannels(std::string _channels) {
    boost::shared_ptr<std::vector<std::pair<int, int> > > out(new std::vector<std::pair<int, int> >);

    std::vector<std::string> chans = dss::splitString(_channels, ';');
    for (size_t i = 0; i < chans.size(); i++) {
      out->push_back(getOutputChannelIdAndSize(chans.at(i)));
    }

    return out;
  }

  boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > DeviceRequestHandler::parseOutputChannelsWithValues(std::string _values) {
    boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > out(new std::vector<boost::tuple<int, int, int> >);

    std::pair<std::string, std::string> kv;
    std::vector<std::string> vals = dss::splitString(_values, ';');
    for (size_t i = 0; i < vals.size(); i++) {
      kv = splitIntoKeyValue(vals.at(i));
      double v = strToDouble(kv.second, -1);
      if (v == -1) {
        throw std::invalid_argument("invalid channel value for channel '" +
                                    kv.first + "'");
      }
      std::pair<int, int> cs = getOutputChannelIdAndSize(kv.first);
      out->push_back(boost::make_tuple(cs.first, cs.second, convertToOutputChannelValue(cs.first, v)));
    }

    return out;
  }

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
    std::string dsidStr = _request.getParameter("dsid");
    std::string dsuidStr = _request.getParameter("dsuid");
    if (dsidStr.empty() && dsuidStr.empty()) {
      throw std::runtime_error("missing parameter 'dsuid'");
    }

    dsuid_t dsuid;
    if (dsuidStr.empty()) {
      dsid_t dsid = str2dsid(dsidStr);
      dsuid = dsuid_from_dsid(&dsid);
    } else {
      dsuid = str2dsuid(dsuidStr);
    }

    try {
      result = m_Apartment.getDeviceByDSID(dsuid);
    } catch(std::runtime_error& e) {
      throw DeviceNotFoundException("Could not find device with dsuid '" +
                                    dsuid2str(dsuid) + "'");
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
    } catch(std::runtime_error& ex) {
      return failure(ex.what());
    }
    assert(pDevice != NULL);
    if(isDeviceInterfaceCall(_request)) {
      return handleDeviceInterfaceRequest(_request, pDevice, _session);
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
        name = escapeHTML(name);

        pDevice->setName(st.convert(name));

        if (m_pStructureBusInterface != NULL) {
          StructureManipulator manipulator(*m_pStructureBusInterface,
                                           *m_pStructureQueryBusInterface,
                                           m_Apartment);
          manipulator.deviceSetName(pDevice, st.convert(name));
        }

        if (pDevice->is2WayMaster()) {
          dsuid_t next = dsuid_get_next_dsuid(pDevice->getDSID());
          try {
            boost::shared_ptr<Device> pPartnerDevice;
            pPartnerDevice = m_Apartment.getDeviceByDSID(next);
            if (pPartnerDevice->getName().empty()) {
              pPartnerDevice->setName(st.convert(name));;
              if (m_pStructureBusInterface != NULL) {
                StructureManipulator manipulator(*m_pStructureBusInterface,
                                                 *m_pStructureQueryBusInterface,
                                                 m_Apartment);
                manipulator.deviceSetName(pPartnerDevice, st.convert(name));
              }
            }
          } catch(std::runtime_error& e) {
            return failure("Could not find partner device with dsid '" + dsuid2str(next) + "'");
          }
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

      tagName = escapeHTML(tagName);
     
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
    } else if (_request.getMethod() == "setJokerGroup") {
      int newGroupId = strToIntDef(_request.getParameter("groupID"), -1);
      if (!isDefaultGroup(newGroupId)) {
        return failure("Invalid or missing parameter 'groupID'");
      }

      if (m_pStructureBusInterface == NULL) {
          return failure("No handle to bus interface");
      }

      std::vector<boost::shared_ptr<Device> > modifiedDevices;
      boost::shared_ptr<Group> group = m_Apartment.getZone(0)->getGroup(newGroupId);
      StructureManipulator manipulator(*m_pStructureBusInterface,
                                       *m_pStructureQueryBusInterface,
                                       m_Apartment);

      if (manipulator.setJokerGroup(pDevice, group)) {
        modifiedDevices.push_back(pDevice);
      }

      if (pDevice->is2WayMaster()) {
        dsuid_t next = dsuid_get_next_dsuid(pDevice->getDSID());
        boost::shared_ptr<Device> pPartnerDevice;
        try {
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
        } catch(std::runtime_error& e) {
          return failure("Could not find partner device with dsid '" +
                         dsuid2str(next) + "'");
        }

        if (manipulator.setJokerGroup(pPartnerDevice, group)) {
          modifiedDevices.push_back(pPartnerDevice);
        }
      }

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      if (!modifiedDevices.empty()) {
        boost::shared_ptr<JSONArrayBase> modified(new JSONArrayBase());
        foreach (const boost::shared_ptr<Device>& device, modifiedDevices) {
          const DeviceReference d(device, &m_Apartment);
          modified->addElement("", toJSON(d));
        }
        resultObj->addProperty("action", "update");
        resultObj->addElement("devices", modified);
      } else {
        resultObj->addProperty("action", "none");
      }
      return success(resultObj);
    } else if(_request.getMethod() == "setHeatingGroup") {
      int newGroupId = strToIntDef(_request.getParameter("groupID"), -1);
      if (!isDefaultGroup(newGroupId)) {
        return failure("Invalid or missing parameter 'groupID'");
      }

      if ((pDevice->getDeviceClass() == DEVICE_CLASS_BL) &&
          (pDevice->getDeviceType() == DEVICE_TYPE_KM) &&
          (pDevice->getProductID() / 100 == 2)) {
          switch (newGroupId) {
          case GroupIDHeating:
          case GroupIDCooling:
          case GroupIDVentilation:
          case GroupIDControlTemperature:
            break;
          default:
            return failure("Invalid group for this device");
          }
      } else {
        return failure("Cannot change group for this device");
      }

      boost::shared_ptr<Group> newGroup = m_Apartment.getZone(pDevice->getZoneID())->getGroup(newGroupId);
      StructureManipulator manipulator(*m_pStructureBusInterface,
                                       *m_pStructureQueryBusInterface,
                                       m_Apartment);
      manipulator.deviceAddToGroup(pDevice, newGroup);

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      boost::shared_ptr<JSONArrayBase> modified(new JSONArrayBase());
      const DeviceReference d(pDevice, &m_Apartment);
      modified->addElement("", toJSON(d));
      resultObj->addProperty("action", "update");
      resultObj->addElement("devices", modified);
      return success(resultObj);

    } else if(_request.getMethod() == "setButtonID") {
      int value = strToIntDef(_request.getParameter("buttonID"), -1);
      if((value  < 0) || (value > 15)) {
        return failure("Invalid or missing parameter 'buttonID'");
      }
      pDevice->setDeviceButtonID(value);

      if (pDevice->is2WayMaster()) {
        DeviceFeatures_t features = pDevice->getFeatures();
        if (!features.syncButtonID) {
          return success();
        }

        dsuid_t next = dsuid_get_next_dsuid(pDevice->getDSID());
        try {
          boost::shared_ptr<Device> pPartnerDevice;
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
          pPartnerDevice->setDeviceButtonID(value);
        } catch(std::runtime_error& e) {
          return failure("Could not find partner device with dsid '" + dsuid2str(next) + "'");
        }
      }
      return success();
    } else if(_request.getMethod() == "setButtonInputMode") {
      if (_request.hasParameter("modeID")) {
        return failure("API has changed, parameter mode ID is no longer \
                        valid, please update your code");
      }
      std::string value = _request.getParameter("mode");
      if (value.empty()) {
        return failure("Invalid or missing parameter 'mode'");
      }


      DeviceFeatures_t features = pDevice->getFeatures();
      if (features.pairing == false) {
        return failure("This device does not support button pairing");
      }

      dsuid_t next = dsuid_get_next_dsuid(pDevice->getDSID());
      boost::shared_ptr<Device> pPartnerDevice;

      try {
        pPartnerDevice = m_Apartment.getDeviceByDSID(next);
      } catch(std::runtime_error& e) {
        throw DeviceNotFoundException("Could not find partner device with dsid '" + dsuid2str(next) + "'");
      }

      bool wasSlave = pPartnerDevice->is2WaySlave();

      if (value == BUTTONINPUT_2WAY_DOWN) {
        if (pDevice->getButtonInputIndex() == 0) {
          if (m_pStructureBusInterface != NULL) {
            pDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT2);
            pPartnerDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT1);
          }
          pDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT2);
          pPartnerDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT1);
        } else {
          if (m_pStructureBusInterface != NULL) {
            pDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT4);
            pPartnerDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT3);
          }
          pDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT4);
          pPartnerDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT3);
        }
      } else if (value == BUTTONINPUT_2WAY_UP) {
        if (pDevice->getButtonInputIndex() == 0) {
          if (m_pStructureBusInterface != NULL) {
            pDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT2);
            pPartnerDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT1);
          }
          pDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT2);
          pPartnerDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT1);
        } else {
          if (m_pStructureBusInterface != NULL) {
            pDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT4);
            pPartnerDevice->setDeviceButtonInputMode(
                    DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT3);
          }
          pDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT4);
          pPartnerDevice->setButtonInputMode(
                  DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT3);
        }
      } else if (value == BUTTONINPUT_1WAY) {
        if (m_pStructureBusInterface != NULL) {
          pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_STANDARD);
          pPartnerDevice->setDeviceButtonInputMode(
                                            DEV_PARAM_BUTTONINPUT_STANDARD);
        }
        pDevice->setButtonInputMode(DEV_PARAM_BUTTONINPUT_STANDARD);
        pPartnerDevice->setButtonInputMode(DEV_PARAM_BUTTONINPUT_STANDARD);
      } else if (value == BUTTONINPUT_2WAY) {
        if (m_pStructureBusInterface != NULL) {
          pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_2WAY);
          pPartnerDevice->setDeviceButtonInputMode(
                                        DEV_PARAM_BUTTONINPUT_SDS_SLAVE_M1_M2);
        }
        pDevice->setButtonInputMode(DEV_PARAM_BUTTONINPUT_2WAY);
        pPartnerDevice->setButtonInputMode(
                                        DEV_PARAM_BUTTONINPUT_SDS_SLAVE_M1_M2);
      } else if (value == BUTTONINPUT_1WAY_COMBINED) {
        if (m_pStructureBusInterface != NULL) {
          pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_1WAY);
          pPartnerDevice->setDeviceButtonInputMode(
                                        DEV_PARAM_BUTTONINPUT_SDS_SLAVE_M1_M2);
        }
        pDevice->setButtonInputMode(DEV_PARAM_BUTTONINPUT_1WAY);
        pPartnerDevice->setButtonInputMode(
                                        DEV_PARAM_BUTTONINPUT_SDS_SLAVE_M1_M2);
      } else {
        return failure("Invalid mode specified");
      }

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      std::string action = "none";
      if ((wasSlave == true) && (pPartnerDevice->is2WaySlave() == false)) {
        action = "add";
      } else if ((wasSlave == false) &&
                 (pPartnerDevice->is2WaySlave() == true)) {
        action = "remove";
      }
      resultObj->addProperty("action", action);
      DeviceReference dr(pPartnerDevice, &m_Apartment);
      resultObj->addElement("device", toJSON(dr));

      boost::shared_ptr<JSONObject> master(new JSONObject());
      try {
        master->addProperty("dsid", dsid2str(dsuid_to_dsid(pDevice->getDSID())));
      } catch (std::runtime_error &err) {
        Logger::getInstance()->log(err.what());
      }

      master->addProperty("dSUID", dsuid2str(pDevice->getDSID()));
      master->addProperty("buttonInputMode", pDevice->getButtonInputMode());
      resultObj->addElement("update", master);

      if (value == BUTTONINPUT_1WAY) {
        return success(resultObj);
      }

      if (pDevice->getZoneID() != pPartnerDevice->getZoneID()) {
        if (m_pStructureBusInterface != NULL) {
          StructureManipulator manipulator(*m_pStructureBusInterface,
                                           *m_pStructureQueryBusInterface,
                                           m_Apartment);
          boost::shared_ptr<Zone> zone = m_Apartment.getZone(
                                                        pDevice->getZoneID());
          manipulator.addDeviceToZone(pPartnerDevice, zone);
        }
      }

      if (features.syncButtonID == true) {
        if (pDevice->getButtonID() != pPartnerDevice->getButtonID()) {
          if (m_pStructureBusInterface != NULL) {
            pPartnerDevice->setDeviceButtonID(pDevice->getButtonID());
          }
          pPartnerDevice->setButtonID(pDevice->getButtonID());
        }
      }

      if ((pDevice->getJokerGroup() > 0) &&
          (pPartnerDevice->getJokerGroup() > 0) &&
          (pDevice->getJokerGroup() != pPartnerDevice->getJokerGroup())) {
        if (m_pStructureBusInterface != NULL) {
          pPartnerDevice->setDeviceJokerGroup(pDevice->getJokerGroup());
        }
      }
      return success(resultObj);
    } else if(_request.getMethod() == "setButtonActiveGroup") {
      int value = strToIntDef(_request.getParameter("groupID"), -2);
      if ((value != BUTTON_ACTIVE_GROUP_RESET) &&
          ((value < -1) || (value > 63))) {
        return failure("Invalid or missing parameter 'groupID'");
      }
      pDevice->setDeviceButtonActiveGroup(value);

      if (pDevice->is2WayMaster()) {
        DeviceFeatures_t features = pDevice->getFeatures();
        if (!features.syncButtonID) {
          return success();
        }

        dsuid_t next = dsuid_get_next_dsuid(pDevice->getDSID());
        try {
          boost::shared_ptr<Device> pPartnerDevice;
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
          pPartnerDevice->setDeviceButtonActiveGroup(value);
        } catch(std::runtime_error& e) {
          return failure("Could not find partner device with dsid '" + dsuid2str(next) + "'");
        }
      }
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
    } else if(_request.getMethod() == "setSceneValue") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 127)) {
        return failure("Invalid or missing parameter 'sceneID'");
      }

      if (!_request.hasParameter("value") && !_request.hasParameter("angle")) {
        return failure("Must supply at least value or angle");
      }
      int value = strToIntDef(_request.getParameter("value"), -1);
      int angle = strToIntDef(_request.getParameter("angle"), -1);
      if ((value < 0) && (angle < 0)) {
        return failure("Invalid value and/or angle parameter");
      }
      if (value > 65535) {
        return failure("Invalid value parameter");
      }
      if (angle > 255) {
        return failure("Invalid angle parameter");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return failure("Device settings are being updated for selected activity, please try again later");
      }

      if (angle != -1) {
        DeviceFeatures_t features = pDevice->getFeatures();
        if (!features.hasOutputAngle) {
          return failure("Device does not support output angle configuration");
        }
        pDevice->setSceneAngle(id, angle);
      }

      if (value != -1) {
        pDevice->setSceneValue(id, value);
      }

      return success();
    } else if(_request.getMethod() == "getSceneValue") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 127)) {
        return failure("Invalid or missing parameter 'sceneID'");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return failure("Device settings are being updated for selected activity, please try again later");
      }

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("value", pDevice->getSceneValue(id));

      DeviceFeatures_t features = pDevice->getFeatures();
      if (features.hasOutputAngle) {
        resultObj->addProperty("angle", pDevice->getSceneAngle(id));
      }

      return success(resultObj);
    } else if(_request.getMethod() == "getSceneMode") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 255)) {
        return failure("Invalid or missing parameter 'sceneID'");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return failure("Device settings are being updated for selected activity, please try again later");
      }

      DeviceSceneSpec_t config;
      if (pDevice->getDeviceType() == DEVICE_TYPE_UMV) {
        pDevice->getDeviceOutputChannelSceneConfig(id, config);
      } else {
        pDevice->getDeviceSceneMode(id, config);
      }
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

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return failure("Device settings are being updated for selected activity, please try again later");
      }

      DeviceSceneSpec_t config;
      if (pDevice->getDeviceType() == DEVICE_TYPE_UMV) {
        pDevice->getDeviceOutputChannelSceneConfig(id, config);
      } else {
        pDevice->getDeviceSceneMode(id, config);
      }

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
      if (pDevice->getDeviceType() == DEVICE_TYPE_UMV) {
        pDevice->setDeviceOutputChannelSceneConfig(id, config);
      } else {
        pDevice->setDeviceSceneMode(id, config);
      }

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

    } else if(_request.getMethod() == "getBinaryInputs") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      boost::shared_ptr<JSONArrayBase> inputs(new JSONArrayBase());
      resultObj->addElement("inputs", inputs);
      std::vector<boost::shared_ptr<DeviceBinaryInput_t> > binputs = pDevice->getBinaryInputs();
      for (std::vector<boost::shared_ptr<DeviceBinaryInput_t> >::iterator it = binputs.begin();
          it != binputs.end();
          it ++) {
        boost::shared_ptr<JSONObject> inputObj(new JSONObject());
        inputObj->addProperty("inputIndex", (*it)->m_inputIndex);
        inputObj->addProperty("inputId", (*it)->m_inputId);
        inputObj->addProperty("inputType", (*it)->m_inputType);
        inputObj->addProperty("targetType", (*it)->m_targetGroupType);
        inputObj->addProperty("targetGroup", (*it)->m_targetGroupId);
        inputs->addElement("", inputObj);
      }
      return success(resultObj);
    } else if(_request.getMethod() == "setBinaryInputType") {
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return failure("Invalid or missing parameter 'index'");
      }
      int type = strToIntDef(_request.getParameter("type"), -1);
      if (type < 0 || type > 254) {
        return failure("Invalid or missing parameter 'type'");
      }
      pDevice->setDeviceBinaryInputType(index, type);
      return success();
    } else if(_request.getMethod() == "setBinaryInputTarget") {
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return failure("Invalid or missing parameter 'index'");
      }
      int gtype = strToIntDef(_request.getParameter("groupType"), -1);
      if (gtype < 0 || gtype > 4) {
        return failure("Invalid or missing parameter 'groupType'");
      }
      int gid = strToIntDef(_request.getParameter("groupId"), -1);
      if (gid < 0 || gid > 63) {
        return failure("Invalid or missing parameter 'groupId'");
      }
      pDevice->setDeviceBinaryInputTarget(index, gtype, gid);
      return success();
    } else if(_request.getMethod() == "setBinaryInputId") {
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return failure("Invalid or missing parameter 'index'");
      }
      int id = strToIntDef(_request.getParameter("inputId"), -1);
      if (id < 0 || id > 15) {
        return failure("Invalid or missing parameter 'inputId'");
      }
      pDevice->setDeviceBinaryInputId(index, id);
      return success();

    } else if(_request.getMethod() == "getAKMInputTimeouts") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      int onDelay, offDelay;
      pDevice->getDeviceAKMInputTimeouts(onDelay, offDelay);
      resultObj->addProperty("ondelay", onDelay);
      resultObj->addProperty("offdelay", offDelay);
      return success(resultObj);
    } else if(_request.getMethod() == "setAKMInputTimeouts") {
      int onDelay = strToIntDef(_request.getParameter("ondelay"), -1);
      if (onDelay > 6552000) { // ms
        return failure("Invalid parameter 'ondelay', must be < 6552000");
      }

      int offDelay = strToIntDef(_request.getParameter("offdelay"), -1);
      if (offDelay > 6552000) {
        return failure("Invalid parameter 'offdelay', must be < 6552000");
      }

      if ((onDelay < 0) && (offDelay < 0)) {
        return failure("No valid parameters given");
      }

      // values < 0 are ignored by this function
      pDevice->setDeviceAKMInputTimeouts(onDelay, offDelay);
      return success();
    } else if (_request.getMethod() == "setAKMInputProperty") {
      std::string mode = _request.getParameter("mode");
      if (mode.empty()) {
        return failure("Invalid or missing parameter 'mode'");
      }

      if (pDevice->getDeviceType() != DEVICE_TYPE_AKM) {
        return failure("This device does not support AKM properties");
      }

      if (mode == BUTTONINPUT_AKM_STANDARD) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_STANDARD);
      } else if (mode == BUTTONINPUT_AKM_INVERTED) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_INVERTED);
      } else if (mode == BUTTONINPUT_AKM_ON_RISING_EDGE) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_ON_RISING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_ON_FALLING_EDGE) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_ON_FALLING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_OFF_RISING_EDGE) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_OFF_RISING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_OFF_FALLING_EDGE) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_OFF_FALLING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_RISING_EDGE) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_RISING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_FALLING_EDGE) {
        pDevice->setDeviceButtonInputMode(DEV_PARAM_BUTTONINPUT_AKM_FALLING_EDGE);
      } else {
        return failure("Unsupported mode: " + mode);
      }
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
      boost::shared_ptr<DeviceSensor_t> sensor = pDevice->getSensor(id);
      int value = sensor->m_sensorType;
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
      int sensorIndex = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if ((sensorIndex < 0) || (sensorIndex > 0xF)) {
        return failure("Invalid or missing parameter 'sensorIndex'");
      }
      event.sensorIndex = sensorIndex;
      int test = strToIntDef(_request.getParameter("test"), -1);
      if ((test < 0) || (test > 0x3)) {
        return failure("Invalid or missing parameter 'test'");
      }
      event.test = test;
      int action = strToIntDef(_request.getParameter("action"), -1);
      if ((action < 0) || (action > 0x3)) {
        return failure("Invalid or missing parameter 'action'");
      }
      event.action = action;
      int value = strToIntDef(_request.getParameter("value"), -1);
      if ((value < 0) || (value > 0xFFF)) {
        return failure("Invalid or missing parameter 'value'");
      }
      event.value = value;
      int hysteresis = strToIntDef(_request.getParameter("hysteresis"), -1);
      if ((hysteresis < 0) || (hysteresis > 0xFFF)) {
        return failure("Invalid or missing parameter 'hysteresis'");
      }
      event.hysteresis = hysteresis;
      int validity = strToIntDef(_request.getParameter("validity"), -1);
      if ((validity < 0) || (validity > 0x3)) {
        return failure("Invalid or missing parameter 'validity'");
      }
      event.validity = validity;
      if (event.action == 2) {
        if (!pDevice->isSceneDevice()) {
          int buttonNumber = strToIntDef(_request.getParameter("buttonNumber"), -1);
          if ((buttonNumber < 0) || (buttonNumber > 0xF)) {
            return failure("Invalid or missing parameter 'buttonNumber'");
          }
	  event.buttonNumber = buttonNumber;
          int clickType = strToIntDef(_request.getParameter("clickType"), -1);
          if ((clickType < 0) || (clickType > 0xF)) {
            return failure("Invalid or missing parameter 'clickType'");
          }
	  event.clickType = clickType;
        } else {
          int sceneDeviceMode = strToIntDef(_request.getParameter("sceneDeviceMode"), -1);
          if ((sceneDeviceMode < 0) || (sceneDeviceMode > 0x3)) {
            return failure("Invalid or missing parameter 'sceneDeviceMode'");
          }
	  event.sceneDeviceMode = sceneDeviceMode;
          int sceneID = strToIntDef(_request.getParameter("sceneID"), -1);
          if ((sceneID < 0) || (sceneID > 0x7F)) {
            return failure("Invalid or missing parameter 'sceneID'");
          }
	  event.sceneID = sceneID;
        }
      } else {
        event.buttonNumber = 0;
        event.clickType = 0;
        event.sceneDeviceMode = 0;
        event.sceneID = 0;
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
    } else if (_request.getMethod() == "getOutputChannelValue") {
      std::string str_chan = _request.getParameter("channels");
      if (str_chan.empty()) {
        return failure("Missing or invalid parameter 'channels'");
      }

      boost::shared_ptr<std::vector<std::pair<int, int> > > channels =  parseOutputChannels(str_chan);

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      boost::shared_ptr<JSONArrayBase> channelsObj(new JSONArrayBase());
      resultObj->addElement("channels", channelsObj);

      for (size_t i = 0; i < channels->size(); i++) {
        boost::shared_ptr<JSONObject> chanObj(new JSONObject());
        chanObj->addProperty("channel", getOutputChannelName(channels->at(i).first));  
        chanObj->addProperty("value",
                convertFromOutputChannelValue(
                    channels->at(i).first,
                    pDevice->getDeviceOutputChannelValue(channels->at(i).first)));
        channelsObj->addElement("", chanObj);
        // don't flood the bus on bulk requests
        if ((channels->size() > 1) && (i < channels->size() - 1)) {
          sleep(1);
        }
      }

      return success(resultObj);
    } else if (_request.getMethod() == "setOutputChannelValue") {
      bool applyNow = strToIntDef(_request.getParameter("applyNow"), 1);
      std::string vals = _request.getParameter("channelvalues");
      if (vals.empty()) {
        return failure("Missing or invalid parameter 'channelvalues'");
      }

      boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > channels =  parseOutputChannelsWithValues(vals);

      for (size_t i = 0; i < channels->size(); i++) {
        pDevice->setDeviceOutputChannelValue(boost::get<0>(channels->at(i)),
                                             boost::get<1>(channels->at(i)),
                                             boost::get<2>(channels->at(i)),
                                             applyNow &&
                                             (i == (channels->size() - 1)));
        // don't flood the bus on bulk requests
        if ((channels->size() > 1) && (i < channels->size() - 1)) {
          sleep(1);
        }
      }

      return success();
    } else if (_request.getMethod() == "setOutputChannelDontCareFlag") {
      std::string str_chan = _request.getParameter("channels");
      if (str_chan.empty()) {
        return failure("Missing or invalid parameter 'channels'");
      }

      int flag = strToIntDef(_request.getParameter("dontCare"), -1);
      if (flag < 0) {
        return failure("Missing or invalid parameter 'dontCare'");
      }

      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return failure("Missing or invalid parameter 'sceneNumber'");
      }

      boost::shared_ptr<std::vector<std::pair<int, int> > > channels =  parseOutputChannels(str_chan);

      uint16_t value = pDevice->getDeviceOutputChannelDontCareFlags(scene);
      for (size_t i = 0; i < channels->size(); i++) {
        int channelIndex = pDevice->getOutputChannelIndex(channels->at(i).first);
        if (channelIndex < 0) {
          return failure("Channel '" + getOutputChannelName(channels->at(i).first) + "' is unknown on this device");
        }
        if (flag) {
          value |= 1 << channelIndex;
        } else {
          value &= ~(1 << channelIndex);
        }
      }
      pDevice->setDeviceOutputChannelDontCareFlags(scene, value);
      return success();
    } else if (_request.getMethod() == "getOutputChannelDontCareFlags") {
      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return failure("Missing or invalid parameter 'sceneNumber'");
      }

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      boost::shared_ptr<JSONObject> channelsObj(new JSONObject());

      resultObj->addElement("channels", channelsObj);

      uint16_t value = pDevice->getDeviceOutputChannelDontCareFlags(scene);
      for (int i = 0; i < pDevice->getOutputChannelCount(); i++) {
        boost::shared_ptr<JSONObject> chanObj(new JSONObject());
        int channelId = pDevice->getOutputChannel(i);
        chanObj->addProperty("channel", getOutputChannelName(channelId));

        chanObj->addProperty("dontCare", ((value & (1 << i)) > 0));
        channelsObj->addElement("", chanObj);
      }
      return success(resultObj);
    } else if (_request.getMethod() == "getOutputChannelSceneValue") {
      std::string str_chan = _request.getParameter("channels");
      if (str_chan.empty()) {
        return failure("Missing or invalid parameter 'channels'");
      }

      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return failure("Missing or invalid parameter 'sceneNumber'");
      }

      boost::shared_ptr<std::vector<std::pair<int, int> > > channels =  parseOutputChannels(str_chan);

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("sceneID", scene);
      boost::shared_ptr<JSONArrayBase> channelsObj(new JSONArrayBase());
      resultObj->addElement("channels", channelsObj);

      for (size_t i = 0; i < channels->size(); i++) {
        boost::shared_ptr<JSONObject> chanObj(new JSONObject());
        chanObj->addProperty("channel", getOutputChannelName(channels->at(i).first));  
        chanObj->addProperty("value", pDevice->getDeviceOutputChannelSceneValue(channels->at(i).first, scene));
        channelsObj->addElement("", chanObj);
      }

      return success(resultObj);
    } else if (_request.getMethod() == "setOutputChannelSceneValue") {
      std::string vals = _request.getParameter("channelvalues");
      if (vals.empty()) {
        return failure("Missing or invalid parameter 'channelvalues'");
      }

      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return failure("Missing or invalid parameter 'sceneNumber'");
      }

      boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > channels =  parseOutputChannelsWithValues(vals);

      for (size_t i = 0; i < channels->size(); i++) {
        pDevice->setDeviceOutputChannelSceneValue(
                                             boost::get<0>(channels->at(i)),
                                             boost::get<1>(channels->at(i)),
                                             scene,
                                             boost::get<2>(channels->at(i)));
        // don't flood the bus on bulk requests
        if ((channels->size() > 1) && (i < channels->size() - 1)) {
          sleep(1);
        }
      }

      return success();
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // jsonHandleRequest

} // namespace dss
