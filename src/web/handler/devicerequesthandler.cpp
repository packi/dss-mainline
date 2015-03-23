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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "devicerequesthandler.h"

#include <boost/bind.hpp>
#include <limits.h>
#include <digitalSTROM/dsuid/dsuid.h>

#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/zone.h"
#include "src/model/modelconst.h"
#include "src/structuremanipulator.h"
#include "src/stringconverter.h"
#include "src/comm-channel.h"
#include "src/ds485types.h"
#include "jsonhelper.h"
#include "foreach.h"
#include "util.h"

namespace dss {

  boost::recursive_mutex DeviceRequestHandler::m_LTMODEMutex;

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
    boost::shared_ptr<std::vector<std::pair<int, int> > > out = boost::make_shared<std::vector<std::pair<int, int> > >();

    std::vector<std::string> chans = dss::splitString(_channels, ';');
    for (size_t i = 0; i < chans.size(); i++) {
      out->push_back(getOutputChannelIdAndSize(chans.at(i)));
    }

    return out;
  }

  boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > DeviceRequestHandler::parseOutputChannelsWithValues(std::string _values) {
    boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > out = boost::make_shared<std::vector<boost::tuple<int, int, int> > >();

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
    if (result == NULL) {
      result = getDeviceByName(_request);
      if (result == NULL) {
        throw DeviceNotFoundException("Need parameter name or dsuid to identify device");
      }
    }
    return result;
  } // getDeviceFromRequest

  boost::shared_ptr<Device> DeviceRequestHandler::getDeviceByDSID(const RestfulRequest& _request) {
    boost::shared_ptr<Device> result;
    std::string dsidStr = _request.getParameter("dsid");
    std::string dsuidStr = _request.getParameter("dsuid");
    if (dsuidStr.length() || dsidStr.length()) {
      dsuid_t dsuid = dsidOrDsuid2dsuid(dsidStr, dsuidStr);
      try {
        result = m_Apartment.getDeviceByDSID(dsuid);
      } catch(std::runtime_error& e) {
        throw DeviceNotFoundException("Could not find device with dsuid '" +
            dsuid2str(dsuid) + "'");
      }
    }
    return result;
  } // getDeviceByDSID

  boost::shared_ptr<Device> DeviceRequestHandler::getDeviceByName(const RestfulRequest& _request) {

    class DeviceNameFilter : public IDeviceAction {
    private:
      std::string m_name;
      std::vector<boost::shared_ptr<Device> > m_devs;
    public:
      DeviceNameFilter(std::string _name) : m_name(_name) {}
      virtual ~DeviceNameFilter() {}
      virtual bool perform(boost::shared_ptr<Device> _device) {
        if (_device->getName() == m_name) {
          m_devs.push_back(_device);
        }
        return true;
      }
      std::vector<boost::shared_ptr<Device> > getDeviceList() {
        return m_devs;
      }
    };

    boost::shared_ptr<Device> result;
    std::string deviceName = _request.getParameter("name");
    if (deviceName.empty()) {
      return result;
    }
    try {
      DeviceNameFilter mFilter(deviceName);
      Set mDevices = m_Apartment.getDevices();
      mDevices.perform(mFilter);
      if (mFilter.getDeviceList().size() > 1) {
        throw DeviceNotFoundException("Multiple devices with name '" + deviceName + "'");
      } else if (mFilter.getDeviceList().size() == 0) {
        throw DeviceNotFoundException("Could not find device named '" + deviceName + "'");
      } else {
        result = mFilter.getDeviceList().at(0);
      }
    } catch(DeviceNotFoundException&  e) {
      throw;
    } catch(std::runtime_error&  e) {
      throw DeviceNotFoundException("Error selecting device named '" + deviceName + "': " + e.what());
    }
    return result;
  } // getDeviceByName

  WebServerResponse DeviceRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    boost::shared_ptr<Device> pDevice;
    StringConverter st("UTF-8", "UTF-8");
    try {
      pDevice = getDeviceFromRequest(_request);
    } catch(DeviceNotFoundException& ex) {
      return JSONWriter::failure(ex.what());
    } catch(std::runtime_error& ex) {
      return JSONWriter::failure(ex.what());
    }
    assert(pDevice != NULL);
    if(isDeviceInterfaceCall(_request)) {
      return handleDeviceInterfaceRequest(_request, pDevice, _session);
    } else if(_request.getMethod() == "getSpec") {
      JSONWriter json;
      json.add("functionID", pDevice->getFunctionID());
      json.add("productID", pDevice->getProductID());
      json.add("revisionID", pDevice->getRevisionID());
      return json.successJSON();
    } else if(_request.getMethod() == "getGroups") {
      JSONWriter json;
      int numGroups = pDevice->getGroupsCount();

      json.startArray("groups");
      for(int iGroup = 0; iGroup < numGroups; iGroup++) {
        try {
          boost::shared_ptr<Group> group = pDevice->getGroupByIndex(iGroup);
          json.startObject();

          json.add("id", group->getID());
          if(!group->getName().empty()) {
            json.add("name", group->getName());
          }
          json.endObject();
        } catch(std::runtime_error&) {
          Logger::getInstance()->log("DeviceRequestHandler: Group only present at device level");
        }
      }
      json.endArray();
      return json.successJSON();
    } else if(_request.getMethod() == "getState") {
      JSONWriter json;
      json.add("isOn", pDevice->isOn());
      return json.successJSON();
    } else if(_request.getMethod() == "getName") {
      JSONWriter json;
      json.add("name", pDevice->getName());
      return json.successJSON();
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
          dsuid_t next;
          dsuid_get_next_dsuid(pDevice->getDSID(), &next);
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
            return JSONWriter::failure("Could not find partner device with dsid '" + dsuid2str(next) + "'");
          }
        }
        return JSONWriter::success();
      } else {
        return JSONWriter::failure("missing parameter 'newName'");
      }
    } else if(_request.getMethod() == "addTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return JSONWriter::failure("missing parameter 'tag'");
      }

      tagName = escapeHTML(tagName);

      pDevice->addTag(st.convert(tagName));
      return JSONWriter::success();
    } else if(_request.getMethod() == "removeTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return JSONWriter::failure("missing parameter 'tag'");
      }
      pDevice->removeTag(tagName);
      return JSONWriter::success();
    } else if(_request.getMethod() == "hasTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return JSONWriter::failure("missing parameter 'tag'");
      }
      JSONWriter json;
      json.add("hasTag", pDevice->hasTag(tagName));
      return json.successJSON();
    } else if(_request.getMethod() == "getTags") {
      JSONWriter json;
      json.startArray("tags");
      std::vector<std::string> tags = pDevice->getTags();
      foreach(std::string tag, tags) {
        json.add(tag);
      }
      json.endArray();
      return json.successJSON();
    } else if(_request.getMethod() == "lock") {
      if (!pDevice->isPresent()) {
        return JSONWriter::failure("Device is not present");
      }
      pDevice->lock();
      return JSONWriter::success();
    } else if(_request.getMethod() == "unlock") {
      if (!pDevice->isPresent()) {
        return JSONWriter::failure("Device is not present");
      }
      pDevice->unlock();
      return JSONWriter::success();
    } else if(_request.getMethod() == "setConfig") {
      int value = strToIntDef(_request.getParameter("value"), -1);
      if((value  < 0) || (value > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter 'value'");
      }

      int configClass = strToIntDef(_request.getParameter("class"), -1);
      if((configClass < 0) || (configClass > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter 'class'");
      }
      int configIndex = strToIntDef(_request.getParameter("index"), -1);
      if((configIndex < 0) || (configIndex > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }

      pDevice->setDeviceConfig(configClass, configIndex, value);
      return JSONWriter::success();
    } else if(_request.getMethod() == "getConfig") {
      int configClass = strToIntDef(_request.getParameter("class"), -1);
      if((configClass < 0) || (configClass > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter 'class'");
      }
      int configIndex = strToIntDef(_request.getParameter("index"), -1);
      if((configIndex < 0) || (configIndex > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }

      uint8_t value = pDevice->getDeviceConfig(configClass, configIndex);

      JSONWriter json;
      json.add("class", configClass);
      json.add("index", configIndex);
      json.add("value", value);

      return json.successJSON();
    } else if(_request.getMethod() == "getConfigWord") {
      int configClass = strToIntDef(_request.getParameter("class"), -1);
      if((configClass < 0) || (configClass > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter 'class'");
      }
      int configIndex = strToIntDef(_request.getParameter("index"), -1);
      if((configIndex < 0) || (configIndex > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }

      uint16_t value = pDevice->getDeviceConfigWord(configClass, configIndex);

      JSONWriter json;
      json.add("class", configClass);
      json.add("index", configIndex);
      json.add("value", value);

      return json.successJSON();
    } else if (_request.getMethod() == "setJokerGroup") {
      boost::recursive_mutex::scoped_lock lock(m_LTMODEMutex);
      int newGroupId = strToIntDef(_request.getParameter("groupID"), -1);
      if (!isDefaultGroup(newGroupId)) {
        return JSONWriter::failure("Invalid or missing parameter 'groupID'");
      }
      if (m_pStructureBusInterface == NULL) {
          return JSONWriter::failure("No handle to bus interface");
      }
      if (pDevice->getDeviceClass() != DEVICE_CLASS_SW) {
          return JSONWriter::failure("Device is not joker device");
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
        dsuid_t next;
        dsuid_get_next_dsuid(pDevice->getDSID(), &next);
        boost::shared_ptr<Device> pPartnerDevice;
        try {
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
        } catch(ItemNotFoundException& e) {
          return JSONWriter::failure("Could not find partner device with dsid '" +
                         dsuid2str(next) + "'");
        }

        if (manipulator.setJokerGroup(pPartnerDevice, group)) {
          modifiedDevices.push_back(pPartnerDevice);
        }
      }

      JSONWriter json;
      if (!modifiedDevices.empty()) {
        json.startArray("devices");
        foreach (const boost::shared_ptr<Device>& device, modifiedDevices) {
          const DeviceReference d(device, &m_Apartment);
          toJSON(d, json);
        }
        json.endArray();
        json.add("action", "update");
      } else {
        json.add("action", "none");
      }
      return json.successJSON();
    } else if(_request.getMethod() == "setHeatingGroup") {
      int newGroupId = strToIntDef(_request.getParameter("groupID"), -1);
      if (!isDefaultGroup(newGroupId)) {
        return JSONWriter::failure("Invalid or missing parameter 'groupID'");
      }
        if (pDevice->isValveDevice()) {
          switch (newGroupId) {
          case GroupIDHeating:
          case GroupIDCooling:
          case GroupIDVentilation:
          case GroupIDControlTemperature:
            break;
          default:
            return JSONWriter::failure("Invalid group for this device");
          }
      } else {
        return JSONWriter::failure("Cannot change group for this device");
      }

      boost::shared_ptr<Group> newGroup = m_Apartment.getZone(pDevice->getZoneID())->getGroup(newGroupId);
      StructureManipulator manipulator(*m_pStructureBusInterface,
                                       *m_pStructureQueryBusInterface,
                                       m_Apartment);
      manipulator.deviceAddToGroup(pDevice, newGroup);

      JSONWriter json;
      json.startObject("devices");
      json.startArray();
      const DeviceReference d(pDevice, &m_Apartment);
      toJSON(d, json);
      json.endArray();
      json.add("action", "update");
      json.endObject();
      return json.successJSON();

    } else if(_request.getMethod() == "setButtonID") {
      int value = strToIntDef(_request.getParameter("buttonID"), -1);
      if((value  < 0) || (value > 15)) {
        return JSONWriter::failure("Invalid or missing parameter 'buttonID'");
      }
      pDevice->setDeviceButtonID(value);

      if (pDevice->is2WayMaster()) {
        DeviceFeatures_t features = pDevice->getFeatures();
        if (!features.syncButtonID) {
          return JSONWriter::success();
        }

        dsuid_t next;
        dsuid_get_next_dsuid(pDevice->getDSID(), &next);
        try {
          boost::shared_ptr<Device> pPartnerDevice;
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
          pPartnerDevice->setDeviceButtonID(value);
        } catch(ItemNotFoundException& e) {
          return JSONWriter::failure("Could not find partner device with dsid '" + dsuid2str(next) + "'");
        }
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "setButtonInputMode") {
      boost::recursive_mutex::scoped_lock lock(m_LTMODEMutex);
      if (_request.hasParameter("modeID")) {
        return JSONWriter::failure("API has changed, parameter mode ID is no longer \
                        valid, please update your code");
      }
      std::string value = _request.getParameter("mode");
      if (value.empty()) {
        return JSONWriter::failure("Invalid or missing parameter 'mode'");
      }


      DeviceFeatures_t features = pDevice->getFeatures();
      if (features.pairing == false) {
        return JSONWriter::failure("This device does not support button pairing");
      }

      dsuid_t next;
      dsuid_get_next_dsuid(pDevice->getDSID(), &next);
      boost::shared_ptr<Device> pPartnerDevice;
      pPartnerDevice = m_Apartment.getDeviceByDSID(next);  // may throw ItemNotFoundException

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
          if (pPartnerDevice->is2WaySlave()) {
            pPartnerDevice->setDeviceButtonInputMode(
                                            DEV_PARAM_BUTTONINPUT_STANDARD);
          }
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
        return JSONWriter::failure("Invalid mode specified");
      }

      JSONWriter json;
      std::string action = "none";
      if ((wasSlave == true) && (pPartnerDevice->is2WaySlave() == false)) {
        action = "add";
      } else if ((wasSlave == false) &&
                 (pPartnerDevice->is2WaySlave() == true)) {
        action = "remove";
      }
      json.add("action", action);
      DeviceReference dr(pPartnerDevice, &m_Apartment);
      json.add("device");
      toJSON(dr, json);

      json.startObject("update");
      dsid_t dsid;
      if (dsuid_to_dsid(pDevice->getDSID(), &dsid)) {
        json.add("dsid", dsid2str(dsid));
      } else {
        json.add("dsid", "");
      }
      json.add("dSUID", dsuid2str(pDevice->getDSID()));
      json.add("buttonInputMode", pDevice->getButtonInputMode());
      json.endObject();

      if (value == BUTTONINPUT_1WAY) {
        return json.successJSON();
      }

      StructureManipulator manipulator(*m_pStructureBusInterface,
                                       *m_pStructureQueryBusInterface,
                                       m_Apartment);

      if (pDevice->getZoneID() != pPartnerDevice->getZoneID()) {
        if (m_pStructureBusInterface != NULL) {
          boost::shared_ptr<Zone> zone = m_Apartment.getZone(
                                                        pDevice->getZoneID());
          manipulator.addDeviceToZone(pPartnerDevice, zone);
        }
      }

      // #3450 - remove slave devices from clusters
      manipulator.deviceRemoveFromGroups(pPartnerDevice);

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
      return json.successJSON();
    } else if(_request.getMethod() == "setButtonActiveGroup") {
      int value = strToIntDef(_request.getParameter("groupID"), -2);
      if ((value != BUTTON_ACTIVE_GROUP_RESET) &&
          ((value < -1) || (value > 63))) {
        return JSONWriter::failure("Invalid or missing parameter 'groupID'");
      }
      pDevice->setDeviceButtonActiveGroup(value);

      if (pDevice->is2WayMaster()) {
        DeviceFeatures_t features = pDevice->getFeatures();
        if (!features.syncButtonID) {
          return JSONWriter::success();
        }

        dsuid_t next;
        dsuid_get_next_dsuid(pDevice->getDSID(), &next);
        try {
          boost::shared_ptr<Device> pPartnerDevice;
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
          pPartnerDevice->setDeviceButtonActiveGroup(value);
        } catch(ItemNotFoundException& e) {
          return JSONWriter::failure("Could not find partner device with dsid '" + dsuid2str(next) + "'");
        }
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "setOutputMode") {
      int value = strToIntDef(_request.getParameter("modeID"), -1);
      if((value  < 0) || (value > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'modeID'");
      }

      JSONWriter json;
      std::string action = "none";
      pDevice->setDeviceOutputMode(value);
      if (pDevice->getDeviceType() == DEVICE_TYPE_UMR) {
        DeviceFeatures_t features = pDevice->getFeatures();
        if (features.pairing == false) {
          json.add("action", action);
          DeviceReference dr(pDevice, &m_Apartment);
          json.add("device");
          toJSON(dr, json);
          return json.successJSON();
        }

        dsuid_t next;
        dsuid_get_next_dsuid(pDevice->getDSID(), &next);
        boost::shared_ptr<Device> pPartnerDevice;

        try {
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);  // may throw ItemNotFoundException
        } catch(ItemNotFoundException& e) {
          return JSONWriter::failure("Could not find partner device with dsid '" + dsuid2str(next) + "'");
        }

        bool wasSlave = pPartnerDevice->is2WaySlave();
        if (pDevice->multiDeviceIndex() == 2) {
          // hide 4th dsid
          if ((value == OUTPUT_MODE_TWO_STAGE_SWITCH) ||
              (value == OUTPUT_MODE_BIPOLAR_SWITCH) ||
              (value == OUTPUT_MODE_THREE_STAGE_SWITCH)) {
            if (wasSlave == false) {
              action = "remove";
            }

            pPartnerDevice->setDeviceOutputMode(value);
          } else {
            if (wasSlave == true) {
              action = "add";
              pPartnerDevice->setDeviceOutputMode(value);
              // wait to ensure the model has time to update
              sleep(1);
            }
          }

          json.add("action", action);
          DeviceReference dr(pPartnerDevice, &m_Apartment);
          json.add("device");
          toJSON(dr, json);

          StructureManipulator manipulator(*m_pStructureBusInterface,
                                           *m_pStructureQueryBusInterface,
                                            m_Apartment);

          if (pDevice->getZoneID() != pPartnerDevice->getZoneID()) {
            if (m_pStructureBusInterface != NULL) {
              boost::shared_ptr<Zone> zone = m_Apartment.getZone(
                                                        pDevice->getZoneID());
              manipulator.addDeviceToZone(pPartnerDevice, zone);
            }
          }

          // #3450 - remove slave devices from clusters
          manipulator.deviceRemoveFromGroups(pPartnerDevice);

          if ((pDevice->getJokerGroup() > 0) &&
              (pPartnerDevice->getJokerGroup() > 0) &&
              (pDevice->getJokerGroup() != pPartnerDevice->getJokerGroup())) {
            if (m_pStructureBusInterface != NULL) {
              pPartnerDevice->setDeviceJokerGroup(pDevice->getJokerGroup());
            }
          }
        } else {
          json.add("action", action);
          DeviceReference dr(pDevice, &m_Apartment);
          json.add("device");
          toJSON(dr, json);
        }
      }
      return json.successJSON();
    } else if(_request.getMethod() == "setProgMode") {
      uint8_t modeId;
      std::string pmode = _request.getParameter("mode");
      if(pmode.compare("enable") == 0) {
        modeId = 1;
      } else if(pmode.compare("disable") == 0) {
        modeId = 0;
      } else {
        return JSONWriter::failure("Invalid or missing parameter 'mode'");
      }
      pDevice->setProgMode(modeId);
      return JSONWriter::success();
    } else if(_request.getMethod() == "getTransmissionQuality") {
      std::pair<uint8_t, uint16_t> p = pDevice->getDeviceTransmissionQuality();
      uint8_t down = p.first;
      uint16_t up = p.second;
      JSONWriter json;
      json.add("upstream", up);
      json.add("downstream", down);
      return json.successJSON();

    } else if(_request.getMethod() == "getOutputValue") {
      int value;
      if (_request.hasParameter("type")) {
        std::string type = _request.getParameter("type");

        // Supported output states for the BL-KM200
        if ((pDevice->getProductID() == ProductID_KM_200) &&
            (pDevice->getDeviceClass() == DEVICE_CLASS_BL)) {
          if (type == "pwmPriorityMode") {
            value = pDevice->getDeviceConfig(CfgClassRuntime, CfgRuntime_Valve_PwmPriorityMode);
          } else if (type == "pwmValue") {
            value = pDevice->getDeviceConfig(CfgClassRuntime, CfgRuntime_Valve_PwmValue);
          } else {
            return JSONWriter::failure("Unsupported type parameter for this device");
          }
        }
        // Supported output states for the GR-KL200 und 210
        else if (((pDevice->getProductID() == ProductID_KL_200) ||
            (pDevice->getProductID() == ProductID_KL_210)) &&
            (pDevice->getDeviceClass() == DEVICE_CLASS_GR)) {
          if (type == "position") {
            value = pDevice->getDeviceConfigWord(CfgClassRuntime, CfgRuntime_Shade_Position);
          } else if (type == "positionCurrent") {
            value = pDevice->getDeviceConfigWord(CfgClassRuntime, CfgRuntime_Shade_PositionCurrent);
          } else {
            return JSONWriter::failure("Unsupported type parameter for this device");
          }
        }
        // Supported output states for the GR-KL220/GR-KL230
        else if (((pDevice->getProductID() == ProductID_KL_220) || (pDevice->getProductID() == ProductID_KL_230)) &&
            (pDevice->getDeviceClass() == DEVICE_CLASS_GR)) {
          if (type == "position") {
            value = pDevice->getDeviceConfigWord(CfgClassRuntime, CfgRuntime_Shade_Position);
          } else if (type == "angle") {
            value = pDevice->getDeviceConfig(CfgClassRuntime, CfgRuntime_Shade_PositionAngle);
          } else if (type == "positionCurrent") {
            value = pDevice->getDeviceConfigWord(CfgClassRuntime, CfgRuntime_Shade_PositionCurrent);
          } else {
            return JSONWriter::failure("Unsupported type parameter for this device");
          }
        }
        else {
          return JSONWriter::failure("Unsupported device for a type parameter");
        }

        JSONWriter json;
        json.add("value", value);
        return json.successJSON();

      } else {
        int offset = strToIntDef(_request.getParameter("offset"), -1);
        if ((offset  < 0) || (offset > 255)) {
          return JSONWriter::failure("Invalid or missing parameter 'type' or 'offset'");
        }
        int value = pDevice->getDeviceOutputValue(offset);

        JSONWriter json;
        json.add("offset", offset);
        json.add("value", value);
        return json.successJSON();
      }

    } else if(_request.getMethod() == "setOutputValue") {
      int offset = strToIntDef(_request.getParameter("offset"), -1);
      if((offset  < 0) || (offset > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'offset'");
      }
      int value = strToIntDef(_request.getParameter("value"), -1);
      if((value  < 0) || (value > 65535)) {
        return JSONWriter::failure("Invalid or missing parameter 'value'");
      }
      pDevice->setDeviceOutputValue(offset, value);
      return JSONWriter::success();
    } else if(_request.getMethod() == "setSceneValue") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 127)) {
        return JSONWriter::failure("Invalid or missing parameter 'sceneID'");
      }

      if (!_request.hasParameter("value") && !_request.hasParameter("angle")) {
        return JSONWriter::failure("Must supply at least value or angle");
      }
      int value = strToIntDef(_request.getParameter("value"), -1);
      int angle = strToIntDef(_request.getParameter("angle"), -1);
      if ((value < 0) && (angle < 0)) {
        return JSONWriter::failure("Invalid value and/or angle parameter");
      }
      if (value > 65535) {
        return JSONWriter::failure("Invalid value parameter");
      }
      if (angle > 255) {
        return JSONWriter::failure("Invalid angle parameter");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return JSONWriter::failure("Device settings are being updated for selected activity, please try again later");
      }

      if (angle != -1) {
        DeviceFeatures_t features = pDevice->getFeatures();
        if (!features.hasOutputAngle) {
          return JSONWriter::failure("Device does not support output angle configuration");
        }
        pDevice->setSceneAngle(id, angle);
      }

      if (value != -1) {
        pDevice->setSceneValue(id, value);
      }

      return JSONWriter::success();
    } else if(_request.getMethod() == "getSceneValue") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 127)) {
        return JSONWriter::failure("Invalid or missing parameter 'sceneID'");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return JSONWriter::failure("Device settings are being updated for selected activity, please try again later");
      }

      JSONWriter json;
      json.add("value", pDevice->getSceneValue(id));

      DeviceFeatures_t features = pDevice->getFeatures();
      if (features.hasOutputAngle) {
        json.add("angle", pDevice->getSceneAngle(id));
      }

      return json.successJSON();
    } else if(_request.getMethod() == "getSceneMode") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'sceneID'");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return JSONWriter::failure("Device settings are being updated for selected activity, please try again later");
      }

      DeviceSceneSpec_t config;
      pDevice->getDeviceSceneMode(id, config);
      JSONWriter json;
      json.add("sceneID", id);
      json.add("dontCare", config.dontcare);
      json.add("localPrio", config.localprio);
      json.add("specialMode", config.specialmode);
      json.add("flashMode", config.flashmode);
      json.add("ledconIndex", config.ledconIndex);
      json.add("dimtimeIndex", config.dimtimeIndex);
      return json.successJSON();
    } else if(_request.getMethod() == "setSceneMode") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'sceneID'");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return JSONWriter::failure("Device settings are being updated for selected activity, please try again later");
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

      return JSONWriter::success();

    } else if(_request.getMethod() == "getTransitionTime") {
      int id = strToIntDef(_request.getParameter("dimtimeIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return JSONWriter::failure("Invalid or missing parameter 'dimtimeIndex'");
      }
      int up, down;
      pDevice->getDeviceTransitionTime(id, up, down);
      JSONWriter json;
      json.add("dimtimeIndex", id);
      json.add("up", up);
      json.add("down", down);
      return json.successJSON();
    } else if(_request.getMethod() == "setTransitionTime") {
      int id = strToIntDef(_request.getParameter("dimtimeIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return JSONWriter::failure("Invalid or missing parameter 'dimtimeIndex'");
      }
      int up = strToIntDef(_request.getParameter("up"), -1);
      int down = strToIntDef(_request.getParameter("down"), -1);
      pDevice->setDeviceTransitionTime(id, up, down);
      return JSONWriter::success();

    } else if(_request.getMethod() == "getLedMode") {
      int id = strToIntDef(_request.getParameter("ledconIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return JSONWriter::failure("Invalid or missing parameter 'ledconIndex'");
      }
      DeviceLedSpec_t config;
      pDevice->getDeviceLedMode(id, config);
      JSONWriter json;
      json.add("ledconIndex", id);
      json.add("colorSelect", config.colorSelect);
      json.add("modeSelect", config.modeSelect);
      json.add("dimMode", config.dimMode);
      json.add("rgbMode", config.rgbMode);
      json.add("groupColorMode", config.groupColorMode);
      return json.successJSON();
    } else if(_request.getMethod() == "setLedMode") {
      int id = strToIntDef(_request.getParameter("ledconIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return JSONWriter::failure("Invalid or missing parameter 'ledconIndex'");
      }
      DeviceLedSpec_t config;
      pDevice->getDeviceLedMode(id, config);
      if (_request.hasParameter("colorSelect"))
        config.colorSelect = strToIntDef(_request.getParameter("colorSelect"), config.colorSelect);
      if (_request.hasParameter("modeSelect"))
        config.modeSelect = strToIntDef(_request.getParameter("modeSelect"), config.modeSelect);
      if (_request.hasParameter("dimMode"))
        config.dimMode = strToIntDef(_request.getParameter("dimMode"), config.dimMode);
      if (_request.hasParameter("rgbMode"))
        config.rgbMode = strToIntDef(_request.getParameter("rgbMode"), config.rgbMode);
      if (_request.hasParameter("groupColorMode"))
        config.groupColorMode = strToIntDef(_request.getParameter("groupColorMode"), config.groupColorMode);
      pDevice->setDeviceLedMode(id, config);
      return JSONWriter::success();

    } else if(_request.getMethod() == "getBinaryInputs") {
      JSONWriter json;
      json.startArray("inputs");
      std::vector<boost::shared_ptr<DeviceBinaryInput_t> > binputs = pDevice->getBinaryInputs();
      for (std::vector<boost::shared_ptr<DeviceBinaryInput_t> >::iterator it = binputs.begin();
          it != binputs.end();
          it ++) {
        json.startObject();
        json.add("inputIndex", (*it)->m_inputIndex);
        json.add("inputId", (*it)->m_inputId);
        json.add("inputType", (*it)->m_inputType);
        json.add("targetType", (*it)->m_targetGroupType);
        json.add("targetGroup", (*it)->m_targetGroupId);
        json.endObject();
      }
      json.endArray();
      return json.successJSON();
    } else if(_request.getMethod() == "setBinaryInputType") {
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }
      int type = strToIntDef(_request.getParameter("type"), -1);
      if (type < 0 || type > 254) {
        return JSONWriter::failure("Invalid or missing parameter 'type'");
      }
      pDevice->setDeviceBinaryInputType(index, type);
      return JSONWriter::success();
    } else if(_request.getMethod() == "setBinaryInputTarget") {
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }
      int gtype = strToIntDef(_request.getParameter("groupType"), -1);
      if (gtype < 0 || gtype > 4) {
        return JSONWriter::failure("Invalid or missing parameter 'groupType'");
      }
      int gid = strToIntDef(_request.getParameter("groupId"), -1);
      if (gid < 0 || gid > 63) {
        return JSONWriter::failure("Invalid or missing parameter 'groupId'");
      }
      pDevice->setDeviceBinaryInputTarget(index, gtype, gid);
      return JSONWriter::success();
    } else if(_request.getMethod() == "setBinaryInputId") {
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }
      int id = strToIntDef(_request.getParameter("inputId"), -1);
      if (id < 0 || id > 15) {
        return JSONWriter::failure("Invalid or missing parameter 'inputId'");
      }
      pDevice->setDeviceBinaryInputId(index, id);
      return JSONWriter::success();

    } else if(_request.getMethod() == "getAKMInputTimeouts") {
      JSONWriter json;
      int onDelay, offDelay;
      pDevice->getDeviceAKMInputTimeouts(onDelay, offDelay);
      json.add("ondelay", onDelay);
      json.add("offdelay", offDelay);
      return json.successJSON();
    } else if(_request.getMethod() == "setAKMInputTimeouts") {
      int onDelay = strToIntDef(_request.getParameter("ondelay"), -1);
      if (onDelay > 6552000) { // ms
        return JSONWriter::failure("Invalid parameter 'ondelay', must be < 6552000");
      }

      int offDelay = strToIntDef(_request.getParameter("offdelay"), -1);
      if (offDelay > 6552000) {
        return JSONWriter::failure("Invalid parameter 'offdelay', must be < 6552000");
      }

      if ((onDelay < 0) && (offDelay < 0)) {
        return JSONWriter::failure("No valid parameters given");
      }

      // values < 0 are ignored by this function
      pDevice->setDeviceAKMInputTimeouts(onDelay, offDelay);
      return JSONWriter::success();
    } else if (_request.getMethod() == "setAKMInputProperty") {
      boost::recursive_mutex::scoped_lock lock(m_LTMODEMutex);
      std::string mode = _request.getParameter("mode");
      if (mode.empty()) {
        return JSONWriter::failure("Invalid or missing parameter 'mode'");
      }

      if ((pDevice->getDeviceType() != DEVICE_TYPE_AKM) &&
          (pDevice->getDeviceType() != DEVICE_TYPE_UMR)) {
        return JSONWriter::failure("This device does not support AKM properties");
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
        return JSONWriter::failure("Unsupported mode: " + mode);
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "getSensorValue") {
      int id = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if((id < 0) || (id > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'sensorIndex'");
      }
      int value = pDevice->getDeviceSensorValue(id);
      JSONWriter json;
      json.add("sensorIndex", id);
      json.add("sensorValue", value);
      return json.successJSON();
    } else if(_request.getMethod() == "getSensorType") {
      int id = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if((id < 0) || (id > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'sensorIndex'");
      }
      boost::shared_ptr<DeviceSensor_t> sensor = pDevice->getSensor(id);
      int value = sensor->m_sensorType;
      JSONWriter json;
      json.add("sensorIndex", id);
      json.add("sensorType", value);
      return json.successJSON();

    } else if(_request.getMethod() == "getSensorEventTableEntry") {
      int id = strToIntDef(_request.getParameter("eventIndex"), -1);
      if((id < 0) || (id > 15)) {
        return JSONWriter::failure("Invalid or missing parameter 'eventIndex'");
      }
      DeviceSensorEventSpec_t event;
      pDevice->getSensorEventEntry(id, event);
      JSONWriter json;
      json.add("eventIndex", id);
      json.add("eventName", event.name);
      json.add("isSceneDevice", false);
      json.add("sensorIndex", event.sensorIndex);
      json.add("test", event.test);
      json.add("action", event.action);
      json.add("value", event.value);
      json.add("hysteresis", event.hysteresis);
      json.add("validity", event.validity);
      if (event.action == 2) {
        json.add("buttonNumber", event.buttonNumber);
        json.add("clickType", event.clickType);
      }
      return json.successJSON();

    } else if(_request.getMethod() == "setSensorEventTableEntry") {
      int id = strToIntDef(_request.getParameter("eventIndex"), -1);
      if((id < 0) || (id > 15)) {
        return JSONWriter::failure("Invalid or missing parameter 'eventIndex'");
      }
      DeviceSensorEventSpec_t event;
      event.name = st.convert(_request.getParameter("eventName"));
      int sensorIndex = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if ((sensorIndex < 0) || (sensorIndex > 0xF)) {
        return JSONWriter::failure("Invalid or missing parameter 'sensorIndex'");
      }
      event.sensorIndex = sensorIndex;
      int test = strToIntDef(_request.getParameter("test"), -1);
      if ((test < 0) || (test > 0x3)) {
        return JSONWriter::failure("Invalid or missing parameter 'test'");
      }
      event.test = test;
      int action = strToIntDef(_request.getParameter("action"), -1);
      if ((action < 0) || (action > 0x3)) {
        return JSONWriter::failure("Invalid or missing parameter 'action'");
      }
      event.action = action;
      int value = strToIntDef(_request.getParameter("value"), -1);
      if ((value < 0) || (value > 0xFFF)) {
        return JSONWriter::failure("Invalid or missing parameter 'value'");
      }
      event.value = value;
      int hysteresis = strToIntDef(_request.getParameter("hysteresis"), -1);
      if ((hysteresis < 0) || (hysteresis > 0xFFF)) {
        return JSONWriter::failure("Invalid or missing parameter 'hysteresis'");
      }
      event.hysteresis = hysteresis;
      int validity = strToIntDef(_request.getParameter("validity"), -1);
      if ((validity < 0) || (validity > 0x3)) {
        return JSONWriter::failure("Invalid or missing parameter 'validity'");
      }
      event.validity = validity;
      if (event.action == 2) {
        int buttonNumber = strToIntDef(_request.getParameter("buttonNumber"), -1);
        if ((buttonNumber < 0) || (buttonNumber > 0xF)) {
          return JSONWriter::failure("Invalid or missing parameter 'buttonNumber'");
        }
        event.buttonNumber = buttonNumber;
        int clickType = strToIntDef(_request.getParameter("clickType"), -1);
        if ((clickType < 0) || (clickType > 0xF)) {
          return JSONWriter::failure("Invalid or missing parameter 'clickType'");
        }
        event.clickType = clickType;
      } else {
        event.buttonNumber = 0;
        event.clickType = 0;
        event.sceneDeviceMode = 0;
        event.sceneID = 0;
      }
      pDevice->setSensorEventEntry(id, event);
      return JSONWriter::success();
    } else if(_request.getMethod() == "addToArea") {
      int areaScene = strToIntDef(_request.getParameter("areaScene"), -1);
      if (areaScene < 0) {
        return JSONWriter::failure("Missing parameter 'areaScene'");
      }
      pDevice->configureAreaMembership(areaScene, true);
      return JSONWriter::success();
    } else if(_request.getMethod() == "removeFromArea") {
      int areaScene = strToIntDef(_request.getParameter("areaScene"), -1);
      if (areaScene < 0) {
        return JSONWriter::failure("Missing parameter 'areaScene'");
      }
      pDevice->configureAreaMembership(areaScene, false);
      return JSONWriter::success();
    } else if (_request.getMethod() == "getOutputChannelValue") {
      std::string str_chan = _request.getParameter("channels");
      if (str_chan.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channels'");
      }

      boost::shared_ptr<std::vector<std::pair<int, int> > > channels =  parseOutputChannels(str_chan);

      JSONWriter json;
      json.startArray("channels");

      for (size_t i = 0; i < channels->size(); i++) {
        json.startObject();
        json.add("channel", getOutputChannelName(channels->at(i).first));
        json.add("value",
                convertFromOutputChannelValue(
                    channels->at(i).first,
                    pDevice->getDeviceOutputChannelValue(channels->at(i).first)));
        json.endObject();
        // don't flood the bus on bulk requests
        if ((channels->size() > 1) && (i < channels->size() - 1)) {
          sleep(1);
        }
      }
      json.endArray();

      return json.successJSON();
    } else if (_request.getMethod() == "setOutputChannelValue") {
      bool applyNow = strToIntDef(_request.getParameter("applyNow"), 1);
      std::string vals = _request.getParameter("channelvalues");
      if (vals.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channelvalues'");
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

      return JSONWriter::success();
    } else if (_request.getMethod() == "setOutputChannelDontCareFlag") {
      std::string str_chan = _request.getParameter("channels");
      if (str_chan.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channels'");
      }

      int flag = strToIntDef(_request.getParameter("dontCare"), -1);
      if (flag < 0) {
        return JSONWriter::failure("Missing or invalid parameter 'dontCare'");
      }

      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return JSONWriter::failure("Missing or invalid parameter 'sceneNumber'");
      }

      boost::shared_ptr<std::vector<std::pair<int, int> > > channels =  parseOutputChannels(str_chan);

      uint16_t value = pDevice->getDeviceOutputChannelDontCareFlags(scene);
      for (size_t i = 0; i < channels->size(); i++) {
        int channelIndex = pDevice->getOutputChannelIndex(channels->at(i).first);
        if (channelIndex < 0) {
          return JSONWriter::failure("Channel '" + getOutputChannelName(channels->at(i).first) + "' is unknown on this device");
        }
        if (flag) {
          value |= 1 << channelIndex;
        } else {
          value &= ~(1 << channelIndex);
        }
      }
      pDevice->setDeviceOutputChannelDontCareFlags(scene, value);
      return JSONWriter::success();
    } else if (_request.getMethod() == "getOutputChannelDontCareFlags") {
      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return JSONWriter::failure("Missing or invalid parameter 'sceneNumber'");
      }

      JSONWriter json;
      json.startObject("channels");

      uint16_t value = pDevice->getDeviceOutputChannelDontCareFlags(scene);
      for (int i = 0; i < pDevice->getOutputChannelCount(); i++) {
        json.startObject();
        int channelId = pDevice->getOutputChannel(i);
        json.add("channel", getOutputChannelName(channelId));

        json.add("dontCare", ((value & (1 << i)) > 0));
        json.endObject();
      }
      json.endObject();
      return json.successJSON();
    } else if (_request.getMethod() == "getOutputChannelSceneValue") {
      std::string str_chan = _request.getParameter("channels");
      if (str_chan.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channels'");
      }

      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return JSONWriter::failure("Missing or invalid parameter 'sceneNumber'");
      }

      boost::shared_ptr<std::vector<std::pair<int, int> > > channels =  parseOutputChannels(str_chan);

      JSONWriter json;
      json.add("sceneID", scene);
      json.startArray("channels");

      for (size_t i = 0; i < channels->size(); i++) {
        json.startObject();
        json.add("channel", getOutputChannelName(channels->at(i).first));
        json.add("value", pDevice->getDeviceOutputChannelSceneValue(channels->at(i).first, scene));
        json.endObject();
      }
      json.endArray();

      return json.successJSON();
    } else if (_request.getMethod() == "setOutputChannelSceneValue") {
      std::string vals = _request.getParameter("channelvalues");
      if (vals.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channelvalues'");
      }

      int scene = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((scene < 0) || (scene > MaxSceneNumber)) {
        return JSONWriter::failure("Missing or invalid parameter 'sceneNumber'");
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

      return JSONWriter::success();

    } else if (_request.getMethod() == "setValveTimerMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceBank3_BL conf(device);

      unsigned int protTimer;
      if (_request.getParameter("valveProtectionTimer", protTimer)) {
        if (protTimer > std::numeric_limits<uint16_t>::max()) {
          return JSONWriter::failure("valveProtectionTimer too large");
        }
        conf.setValveProtectionTimer(protTimer);
      };

      int emergencyValue;
      if (_request.getParameter("emergencyValue", emergencyValue)) {
        if (emergencyValue < -100 || emergencyValue > 100) {
          return JSONWriter::failure("emergencyValue out of [-100:100] range");
        }
        conf.setEmergencySetPoint(emergencyValue);
      };

      unsigned int emergencyTimer;
      if (_request.getParameter("emergencyTimer", emergencyTimer)) {
        if (emergencyTimer > std::numeric_limits<uint8_t>::max()) {
          return JSONWriter::failure("emergencyTimer too big");
        }
        conf.setEmergencyTimer(emergencyTimer);
      };

      return JSONWriter::success();

    } else if (_request.getMethod() == "getValveTimerMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceBank3_BL conf(device);

      JSONWriter json;
      json.add("valveProtectionTimer", conf.getValveProtectionTimer());
      json.add("emergencyValue", conf.getEmergencySetPoint());
      json.add("emergencyTimer", conf.getEmergencyTimer());
      return json.successJSON();

    } else if (_request.getMethod() == "setValvePwmMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceBank3_BL conf(device);

      unsigned pwmPeriod;
      if (_request.getParameter("pwmPeriod", pwmPeriod)) {
        if (pwmPeriod > std::numeric_limits<uint16_t>::max()) {
          return JSONWriter::failure("valveProtectionTimer too large");
        }
        conf.setPwmPeriod(pwmPeriod);
      };
      int value;
      if (_request.getParameter("pwmMinX", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMinX out of [0:100] range");
        }
        conf.setPwmMinX(value);
      };
      if (_request.getParameter("pwmMaxX", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMaxX out of [0:100] range");
        }
        conf.setPwmMaxX(value);
      };
      if (_request.getParameter("pwmMinY", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMinY out of [0:100] range");
        }
        conf.setPwmMinY(value);
      };
      if (_request.getParameter("pwmMaxY", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMaxY out of [0:100] range");
        }
        conf.setPwmMaxY(value);
      };
      int offset;
      if (_request.getParameter("pwmOffset", offset)) {
        if (offset < -100 || offset > 100) {
          return JSONWriter::failure("PWM offset out of [-100:100] range");
        }
        conf.setPwmOffset(offset);
      };
      return JSONWriter::success();

    } else if (_request.getMethod() == "getValvePwmMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      JSONWriter json;
      uint16_t value = device->getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmPeriod);
      json.add("pwmPeriod", value);
      value = device->getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmMinValue);
      json.add("pwmMinX", value & 0xff);
      json.add("pwmMaxX", (value >> 8) & 0xff);
      value = device->getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmMinY);
      json.add("pwmMinY", value & 0xff);
      json.add("pwmMaxY", (value >> 8) & 0xff);
      value = device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmOffset);
      json.add("pwmOffset", value);
      return json.successJSON();

    } else if (_request.getMethod() == "getValvePwmState") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      uint16_t value = device->getDeviceConfigWord(CfgClassRuntime, CfgRuntime_Valve_PwmValue);
      JSONWriter json;
      json.add("pwmValue", value & 0xff);
      json.add("pwmPriorityMode", (value >> 8) & 0xff);
      return json.successJSON();

    } else if (_request.getMethod() == "setValveControlMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceValveControlSpec_t config;
      device->getDeviceValveControl(config);

      _request.getParameter("ctrlClipMinZero", config.ctrlClipMinZero);
      _request.getParameter("ctrlClipMinLower", config.ctrlClipMinLower);
      _request.getParameter("ctrlClipMaxHigher", config.ctrlClipMaxHigher);
      _request.getParameter("ctrlNONC", config.ctrlNONC);

      device->setDeviceValveControl(config);
      return JSONWriter::success();

    } else if (_request.getMethod() == "getValveControlMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceValveControlSpec_t config;
      device->getDeviceValveControl(config);

      JSONWriter json;
      json.add("ctrlClipMinZero", config.ctrlClipMinZero);
      json.add("ctrlClipMinLower", config.ctrlClipMinLower);
      json.add("ctrlClipMaxHigher", config.ctrlClipMaxHigher);
      json.add("ctrlNONC", config.ctrlNONC);
      return json.successJSON();

    } else if (_request.getMethod() == "setValveType") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      if (device->isValveDevice()) {
        std::string valveType = _request.getParameter("valveType");
        if (device->setValveTypeAsString(valveType)) {
          return JSONWriter::success();
        } else {
          return JSONWriter::failure("valve type not valid");
        }
      }
      return JSONWriter::failure("device is not a valve type");

    } else if (_request.getMethod() == "getValveType") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      if (device->isValveDevice()) {
        std::string valveType = device->getValveTypeAsString();
        JSONWriter json;
        json.add("valveType", valveType);
        return json.successJSON();
      }
      return JSONWriter::failure("device is not a valve type");
    } else if (_request.getMethod() == "getUMVRelayValue") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      std::string valveType = device->getValveTypeAsString();
      JSONWriter json;
      json.add("relayValue", device->getDeviceUMVRelayValue());
      return json.successJSON();
    } else if (_request.getMethod() == "setUMVRelayValue") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      std::string value = _request.getParameter("value");
      int relayValue = strToIntDef(value, -1);
      if ((relayValue < 0) || (relayValue > 3)) { // 3/FOLLOW_L is max value
        return JSONWriter::failure("invalid relay value given");
      }

      device->setDeviceUMVRelayValue(strToInt(value));
      return JSONWriter::success();
    } else if (_request.getMethod() == "setBlinkConfig") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      int blinkCount = -1;
      double onDelay = -1;
      double offDelay = -1;

      std::string value = _request.getParameter("count");

      if (!value.empty()) {
        blinkCount = strToIntDef(value, -1);
        if ((blinkCount < 0) || (blinkCount > 255)) { //255/FCOUNT1 is max value
          return JSONWriter::failure("invalid count value given");
        }
      }

      value = _request.getParameter("ondelay");
      if (!value.empty()) {
        onDelay = strToDouble(value);
      }

      value = _request.getParameter("offdelay");
      if (!value.empty()) {
        offDelay = strToDouble(value);
      }

      if (blinkCount != -1) {
        device->setDeviceUMRBlinkRepetitions((uint8_t)blinkCount);
      }

      if (onDelay != -1) {
        device->setDeviceUMROnDelay(onDelay);
      }

      if (offDelay != -1) {
         device->setDeviceUMROffDelay(offDelay);
      }
      return JSONWriter::success();
    } else if (_request.getMethod() == "getBlinkConfig") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      JSONWriter json;
      uint8_t umr_count;
      double umr_ondelay;
      double umr_offdelay;
      device->getDeviceUMRDelaySettings(&umr_ondelay, &umr_offdelay, &umr_count);
      json.add("count", umr_count);
      json.add("ondelay", umr_ondelay);
      json.add("offdelay", umr_offdelay);
      return json.successJSON();

    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // jsonHandleRequest

} // namespace dss
