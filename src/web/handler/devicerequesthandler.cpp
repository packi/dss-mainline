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

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <limits.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include "foreach.h"

#include <digitalSTROM/dsuid.h>

#include "src/businterface.h"
#include "src/model/apartment.h"
#include "src/model/data_types.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/zone.h"
#include "src/model/modelconst.h"
#include "src/vdc-db.h"
#include "src/stringconverter.h"
#include "src/comm-channel.h"
#include "src/ds485types.h"
#include "jsonhelper.h"
#include "util.h"
#include "propertyquery.h"
#include "src/messages/vdc-messages.pb.h"
#include "src/protobufjson.h"
#include "src/vdc-element-reader.h"
#include "src/vdc-connection.h"
#include "vdc-info.h"

namespace dss {

  boost::recursive_mutex DeviceRequestHandler::m_setConfigMutex;

  //=========================================== DeviceRequestHandler

  DeviceRequestHandler::DeviceRequestHandler(Apartment& _apartment,
                                             StructureModifyingBusInterface &modify,
                                             StructureQueryBusInterface &query)
  : m_Apartment(_apartment),
    m_manipulator(modify, query, m_Apartment)
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

  WebServerResponse DeviceRequestHandler::processRequestDeviceInfo(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
    StringConverter st("UTF-8", "UTF-8");
    std::string gtin("");
    _request.getParameter("gtin", gtin);
    std::string langCode("");
    _request.getParameter("lang", langCode);

    boost::shared_ptr<Device> pDevice;
    JSONWriter json;

    if (_request.getMethod() == "getInfoStatic") {
      VdcDb db(*DSS::getInstance());
      if (!gtin.empty()) {
        vdcInfo::addStateDescriptions(db, gtin, langCode, json);
        vdcInfo::addEventDescriptions(db, gtin, langCode, json);
        vdcInfo::addPropertyDescriptions(db, gtin, langCode, json);
        vdcInfo::addSensorDescriptions(db, gtin, langCode, json);
        vdcInfo::addActionDescriptions(db, gtin, langCode, json);
        vdcInfo::addStandardActions(db, gtin, langCode, json);
      } else {
        pDevice = getDeviceFromRequest(_request);
        vdcInfo::addSpec(db, *pDevice, langCode, json);
        vdcInfo::addStateDescriptionsDev(db, *pDevice, langCode, json);
        vdcInfo::addEventDescriptionsDev(db, *pDevice, langCode, json);
        vdcInfo::addPropertyDescriptionsDev(db, *pDevice, langCode, json);
        vdcInfo::addSensorDescriptionsDev(db, *pDevice, langCode, json);
        vdcInfo::addActionDescriptionsDev(db, *pDevice, langCode, json);
        vdcInfo::addStandardActionsDev(db, *pDevice, langCode, json);
      }
      return json.successJSON();
    } else if (_request.getMethod() == "getInfoCustom") {
      pDevice = getDeviceFromRequest(_request);
      vdcInfo::addCustomActions(*pDevice, json);
      return json.successJSON();
    } else if (_request.getMethod() == "getInfo") {
      // exceptions will cause the call to abort with a JSON failure.
      // imho it should not silently ignore and still return a success JSON
      // if anything goes wrong, as it was done in the previous version

      std::string filterParam;
      _request.getParameter("filter", filterParam);
      std::string langCode("");
      _request.getParameter("lang", langCode);

      VdcDb db(*DSS::getInstance());
      auto filter = vdcInfo::parseFilter(filterParam);
      if (!gtin.empty()) {
        vdcInfo::addByFilter(db, gtin, filter, langCode, json);
      } else {
        pDevice = getDeviceFromRequest(_request);
        vdcInfo::addByFilter(db, *pDevice, filter, langCode, json);
      }
      return json.successJSON();
    } else if (_request.getMethod() == "getInfoOperational") {
      pDevice = getDeviceFromRequest(_request);
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      VdcDb db(*DSS::getInstance());
      std::string langCode("");
      _request.getParameter("lang", langCode);
      vdcInfo::addOperationalValues(db, *pDevice, langCode, json);
      return json.successJSON();
    } else if (_request.getMethod() == "setProperty") {
      pDevice = getDeviceFromRequest(_request);
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      // /json/device/setProperty?dsuid=5601E3DDC1845C14C02503E66296CB5700&id=waterhardness&value=5.55
      // /json/device/setProperty?dsuid=5601E3DDC1845C14C02503E66296CB5700&id=name&value="new name"
      std::string id;
      if (!_request.getParameter("id", id)) {
        return json.failure("missing parameter: id");
      }
      std::string value;
      if (!_request.getParameter("value", value) ) {
        return json.failure("missing parameter: value");
      }
      vdcapi::PropertyElement element = ProtobufToJSon::jsonToElement(value);
      element.set_name(id);
      pDevice->setProperty(element);
      return json.successJSON();
    } else if (_request.getMethod() == "setCustomAction") {
      pDevice = getDeviceFromRequest(_request);
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      // /json/device/setCustomAction?dsuid=5601E3DDC1845C14C02503E66296CB5700&id=custom77&title=Action%201&action=heat&params={"duration":17}
      std::string id;
      if (!_request.getParameter("id", id)) {
        return json.failure("missing parameter: id");
      }
      std::string action;
      _request.getParameter("action", action);
      std::string title;
      vdcapi::PropertyElement parsedParamsElement;
      const vdcapi::PropertyElement* paramsElement = &vdcapi::PropertyElement::default_instance();
      if (!action.empty()) {
        if (!_request.getParameter("title", title)) {
          return json.failure("missing parameter: title");
        }
        std::string params;
        if (!_request.getParameter("params", params) ) {
          return json.failure("missing parameter: params");
        }
        parsedParamsElement = ProtobufToJSon::jsonToElement(params);
        paramsElement = &parsedParamsElement;
      }
      pDevice->setCustomAction(id, title, action, *paramsElement);
      return json.successJSON();
    } else if (_request.getMethod() == "callAction") {
      pDevice = getDeviceFromRequest(_request);
      std::string id;
      if (!_request.getParameter("id", id)) {
        return json.failure("missing parameter: id");
      }
      std::string params;
      vdcapi::PropertyElement parsedParamsElement;
      const vdcapi::PropertyElement* paramsElement = &vdcapi::PropertyElement::default_instance();
      if (_request.getParameter("params", params) && !params.empty()) {
        parsedParamsElement = ProtobufToJSon::jsonToElement(params);
        paramsElement = &parsedParamsElement;
      }
      pDevice->callAction(id, *paramsElement);
      return json.successJSON();
    }
    return json.failure("Error in processRequestDeviceInfo service handler");
  }

  WebServerResponse DeviceRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {

    if ((_request.getMethod() == "getInfo") ||
        (_request.getMethod() == "getInfoStatic") ||
        (_request.getMethod() == "getInfoCustom") ||
        (_request.getMethod() == "getInfoOperational") ||
        (_request.getMethod() == "setProperty") ||
        (_request.getMethod() == "setCustomAction") ||
        (_request.getMethod() == "callAction")) {
      return processRequestDeviceInfo(_request, _session, _connection);
    }

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
    } else if (_request.getMethod() == "getSpec") {
      JSONWriter json;
      json.add("functionID", pDevice->getFunctionID());
      json.add("productID", pDevice->getProductID());
      json.add("revisionID", pDevice->getRevisionID());
      return json.successJSON();
    } else if (_request.getMethod() == "getGroups") {
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
    } else if (_request.getMethod() == "getState") {
      JSONWriter json;
      json.add("isOn", pDevice->isOn());
      return json.successJSON();
    } else if (_request.getMethod() == "getName") {
      JSONWriter json;
      json.add("name", pDevice->getName());
      return json.successJSON();
    } else if (_request.getMethod() == "setName") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      if(_request.hasParameter("newName")) {
        std::string name = _request.getParameter("newName");
        name = escapeHTML(name);

        pDevice->setName(st.convert(name));

        m_manipulator.deviceSetName(pDevice, st.convert(name));

        if (pDevice->is2WayMaster()) {
          auto pPartnerDevice = pDevice->getPartnerDevice();
          if (pPartnerDevice->getName().empty()) {
            pPartnerDevice->setName(st.convert(name));
            m_manipulator.deviceSetName(pPartnerDevice, st.convert(name));
          }
        }
        return JSONWriter::success();
      } else {
        return JSONWriter::failure("missing parameter 'newName'");
      }
    } else if (_request.getMethod() == "addTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return JSONWriter::failure("missing parameter 'tag'");
      }

      tagName = escapeHTML(tagName);

      pDevice->addTag(st.convert(tagName));
      return JSONWriter::success();
    } else if (_request.getMethod() == "removeTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return JSONWriter::failure("missing parameter 'tag'");
      }
      pDevice->removeTag(tagName);
      return JSONWriter::success();
    } else if (_request.getMethod() == "hasTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return JSONWriter::failure("missing parameter 'tag'");
      }
      JSONWriter json;
      json.add("hasTag", pDevice->hasTag(tagName));
      return json.successJSON();
    } else if (_request.getMethod() == "getTags") {
      JSONWriter json;
      json.startArray("tags");
      std::vector<std::string> tags = pDevice->getTags();
      foreach(std::string tag, tags) {
        json.add(tag);
      }
      json.endArray();
      return json.successJSON();
    } else if (_request.getMethod() == "lock") {
      if (!pDevice->isPresent()) {
        return JSONWriter::failure("Device is not present");
      }
      pDevice->lock();
      return JSONWriter::success();
    } else if (_request.getMethod() == "unlock") {
      if (!pDevice->isPresent()) {
        return JSONWriter::failure("Device is not present");
      }
      pDevice->unlock();

      if (pDevice->isMainDevice() && (pDevice->getPairedDevices() > 0)) {
        dsuid_t next;
        dsuid_t current = pDevice->getDSID();
        for (int p = 0; p < pDevice->getPairedDevices(); p++) {
          dsuid_get_next_dsuid(current, &next);
          current = next;
          try {
            boost::shared_ptr<Device> pPartnerDev;
            pPartnerDev = m_Apartment.getDeviceByDSID(next);
            usleep(500 * 1000); // 500ms
            pPartnerDev->unlock();
          } catch(std::runtime_error& e) {
            Logger::getInstance()->log(std::string("Could not find partner device with dsuid '") + dsuid2str(next) + "'");
          }
        }
      }
      return JSONWriter::success();
    } else if (_request.getMethod() == "setConfig") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
    } else if (_request.getMethod() == "getConfig") {
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
    } else if (_request.getMethod() == "getConfigWord") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int newGroupId = strToIntDef(_request.getParameter("groupID"), -1);
      if (!isDefaultGroup(newGroupId) && !isGlobalAppDsGroup(newGroupId)) {
        return JSONWriter::failure("Invalid or missing parameter 'groupID'");
      }

      std::vector<boost::shared_ptr<Device> > modifiedDevices;

      if (m_manipulator.setJokerGroup(pDevice, newGroupId)) {
        modifiedDevices.push_back(pDevice);
      }

      if (pDevice->is2WayMaster()) {
        auto pPartnerDevice = pDevice->getPartnerDevice();
        m_manipulator.setJokerGroup(pPartnerDevice, newGroupId);
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
    } else if (_request.getMethod() == "setHeatingGroup") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      m_manipulator.deviceAddToGroup(pDevice, newGroup);

      JSONWriter json;
      json.startArray("devices");
      const DeviceReference d(pDevice, &m_Apartment);
      toJSON(d, json);
      json.endArray();
      json.add("action", "update");
      return json.successJSON();

    } else if (_request.getMethod() == "setButtonID") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int value = strToIntDef(_request.getParameter("buttonID"), -1);
      if((value  < 0) || (value > 15)) {
        return JSONWriter::failure("Invalid or missing parameter 'buttonID'");
      }
      pDevice->setDeviceButtonID(value);

      if (pDevice->is2WayMaster()) {
        DeviceFeatures_t features = pDevice->getDeviceFeatures();
        if (!features.syncButtonID) {
          return JSONWriter::success();
        }

        auto pPartnerDevice = pDevice->getPartnerDevice();
        pPartnerDevice->setDeviceButtonID(value);
      }
      return JSONWriter::success();
    } else if (_request.getMethod() == "setButtonInputMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      if (_request.hasParameter("modeID")) {
        return JSONWriter::failure("API has changed, parameter mode ID is no longer \
                        valid, please update your code");
      }
      std::string value = _request.getParameter("mode");
      if (value.empty()) {
        return JSONWriter::failure("Invalid or missing parameter 'mode'");
      }


      DeviceFeatures_t features = pDevice->getDeviceFeatures();
      if (features.pairing == false) {
        return JSONWriter::failure("This device does not support button pairing");
      }

      if ((pDevice->getDeviceClass() == DEVICE_CLASS_SW) &&
          (pDevice->getActiveGroup() == GroupIDBlack) &&
          (value != BUTTONINPUT_1WAY)) {
        return JSONWriter::failure("Joker devices must be set to a specific group for button pairing");
      }

      auto pPartnerDevice = pDevice->getPartnerDevice();
      bool wasSlave = pPartnerDevice->is2WaySlave();

      if (value == BUTTONINPUT_2WAY_DOWN) {
        if (pDevice->getButtonInputIndex() == 0) {
          pDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_DW_WITH_INPUT2);
          pPartnerDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_UP_WITH_INPUT1);
        } else {
          pDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_DW_WITH_INPUT4);
          pPartnerDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_UP_WITH_INPUT3);
        }
      } else if (value == BUTTONINPUT_2WAY_UP) {
        if (pDevice->getButtonInputIndex() == 0) {
          pDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_UP_WITH_INPUT2);
          pPartnerDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_DW_WITH_INPUT1);
        } else {
          pDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_UP_WITH_INPUT4);
          pPartnerDevice->setDeviceButtonInputMode(
                  ButtonInputMode::TWO_WAY_DW_WITH_INPUT3);
        }
      } else if (value == BUTTONINPUT_1WAY) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::STANDARD);
        if (pPartnerDevice->is2WaySlave()) {
          pPartnerDevice->setDeviceButtonInputMode(
                                          ButtonInputMode::STANDARD);
        }
      } else if (value == BUTTONINPUT_2WAY) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::TWO_WAY);
        pPartnerDevice->setDeviceButtonInputMode(
                                      ButtonInputMode::SDS_SLAVE_M1_M2);
      } else if (value == BUTTONINPUT_1WAY_COMBINED) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::ONE_WAY);
        pPartnerDevice->setDeviceButtonInputMode(
                                      ButtonInputMode::SDS_SLAVE_M1_M2);
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
      json.add("dSUID", pDevice->getDSID());
      json.add("buttonInputMode", pDevice->getButtonInputMode());
      json.endObject();

      if (value == BUTTONINPUT_1WAY) {
        return json.successJSON();
      }

      if (pDevice->getZoneID() != pPartnerDevice->getZoneID()) {
        boost::shared_ptr<Zone> zone = m_Apartment.getZone(
                                                      pDevice->getZoneID());
        m_manipulator.addDeviceToZone(pPartnerDevice, zone);
      }

      // #3450 - remove slave devices from clusters
      m_manipulator.deviceRemoveFromGroups(pPartnerDevice);

      if (features.syncButtonID == true) {
        if (pDevice->getButtonID() != pPartnerDevice->getButtonID()) {
          pPartnerDevice->setDeviceButtonID(pDevice->getButtonID());
        }
      }

      if ((pDevice->getActiveGroup() != GroupIDNotApplicable) &&
          (pDevice->getActiveGroup() != pPartnerDevice->getActiveGroup())) {
        m_manipulator.setJokerGroup(pPartnerDevice, pDevice->getActiveGroup());
      }
      return json.successJSON();
    } else if (_request.getMethod() == "setButtonActiveGroup") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int value = strToIntDef(_request.getParameter("groupID"), -2);
      if ((value != BUTTON_ACTIVE_GROUP_RESET) &&
          ((value < -1) || (value > GroupIDGlobalAppMax))) {
        return JSONWriter::failure("Invalid or missing parameter 'groupID'");
      }
      pDevice->setDeviceButtonActiveGroup(value);

      if (pDevice->is2WayMaster()) {
        DeviceFeatures_t features = pDevice->getDeviceFeatures();
        if (!features.syncButtonID) {
          return JSONWriter::success();
        }

        auto pPartnerDevice = pDevice->getPartnerDevice();
        pPartnerDevice->setDeviceButtonActiveGroup(value);
      }
      return JSONWriter::success();
    } else if (_request.getMethod() == "setOutputMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int value = strToIntDef(_request.getParameter("modeID"), -1);
      if((value  < 0) || (value > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'modeID'");
      }

      JSONWriter json;
      std::string action = "none";
      pDevice->setDeviceOutputMode(value);
      if (pDevice->getDeviceType() == DEVICE_TYPE_UMR) {
        DeviceFeatures_t features = pDevice->getDeviceFeatures();
        if (features.pairing == false) {
          json.add("action", action);
          DeviceReference dr(pDevice, &m_Apartment);
          json.add("device");
          toJSON(dr, json);
          return json.successJSON();
        }

        auto pPartnerDevice = pDevice->getPartnerDevice();
        bool wasSlave = pPartnerDevice->is2WaySlave();
        if (pDevice->multiDeviceIndex() == 2) {
          // hide 4th dsid
          if ((value == OUTPUT_MODE_TWO_STAGE_SWITCH) ||
              (value == OUTPUT_MODE_BIPOLAR_SWITCH) ||
              (value == OUTPUT_MODE_THREE_STAGE_SWITCH) ||
              (value == OUTPUT_MODE_TEMPCONTROL_2OUT_2STEPS) ||
              (value == OUTPUT_MODE_TEMPCONTROL_2OUT_3STEPS) ||
              (value == OUTPUT_MODE_TEMPCONTROL_2OUT_PARALLEL)) {
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

          // if the devices are connected move second device to proper zone and group
          if (pPartnerDevice->is2WaySlave()) {
            if (pDevice->getZoneID() != pPartnerDevice->getZoneID()) {
              boost::shared_ptr<Zone> zone = m_Apartment.getZone(
                                                        pDevice->getZoneID());
              m_manipulator.addDeviceToZone(pPartnerDevice, zone);
            }

            // #3450 - remove slave devices from clusters
            m_manipulator.deviceRemoveFromGroups(pPartnerDevice);

            if ((pDevice->getActiveGroup() != GroupIDNotApplicable) &&
                (pDevice->getActiveGroup() != pPartnerDevice->getActiveGroup())) {
              m_manipulator.setJokerGroup(pPartnerDevice, pDevice->getActiveGroup());
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
    } else if (_request.getMethod() == "setProgMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
    } else if (_request.getMethod() == "getTransmissionQuality") {
      std::pair<uint8_t, uint16_t> p = pDevice->getDeviceTransmissionQuality();
      uint8_t down = p.first;
      uint16_t up = p.second;
      JSONWriter json;
      json.add("upstream", up);
      json.add("downstream", down);
      return json.successJSON();

    } else if (_request.getMethod() == "getOutputValue") {
      int value;
      if (_request.hasParameter("type")) {
        std::string type = _request.getParameter("type");

        // Supported output states for the BL-KM200
        if (((pDevice->getProductID() == ProductID_KM_200) || (pDevice->getProductID() == ProductID_SDS_200)) &&
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

    } else if (_request.getMethod() == "setOutputValue") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
    } else if (_request.getMethod() == "setSceneValue") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 127)) {
        return JSONWriter::failure("Invalid or missing parameter 'sceneID'");
      }
      if (_request.hasParameter("command")) {
        std::string action = _request.getParameter("command");
        if (!pDevice->isVdcDevice() || !pDevice->getHasActions()) {
          return JSONWriter::failure("Device does not support action configuration");
        }
        google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
        vdcapi::PropertyElement* e1 = query.Add();
        e1->set_name("scenes");
        vdcapi::PropertyElement* e2 = e1->add_elements();
        e2->set_name(intToString(id));
        vdcapi::PropertyElement* e3 = e2->add_elements();
        e3->set_name("command");
        e3->mutable_value()->set_v_string(action);
        vdcapi::PropertyElement* e4 = e2->add_elements();
        e4->set_name("dontCare");
        e4->mutable_value()->set_v_bool(action.empty());
        pDevice->setVdcProperty(query);
      }
      if (_request.hasParameter("value") || _request.hasParameter("angle")) {
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
          DeviceFeatures_t features = pDevice->getDeviceFeatures();
          if (!features.hasOutputAngle) {
            return JSONWriter::failure("Device does not support output angle configuration");
          }
          pDevice->setSceneAngle(id, angle);
        }
        if (value != -1) {
          pDevice->setSceneValue(id, value);
        }
      }
      return JSONWriter::success();
    } else if (_request.getMethod() == "getSceneValue") {
      int id = strToIntDef(_request.getParameter("sceneID"), -1);
      if((id  < 0) || (id > 127)) {
        return JSONWriter::failure("Invalid or missing parameter 'sceneID'");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)id)) {
        return JSONWriter::failure("Device settings are being updated for selected activity, please try again later");
      }

      JSONWriter json;
      if (pDevice->isVdcDevice()) {
        google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
        vdcapi::PropertyElement* e1 = query.Add();
        e1->set_name("scenes");
        vdcapi::PropertyElement* e2 = e1->add_elements();
        e2->set_name(intToString(id));
        vdcapi::Message message = pDevice->getVdcProperty(query);
        VdcElementReader reader(message.vdc_response_get_property().properties());
        json.add("scenes");
        ProtobufToJSon::processElementsPretty(reader["scenes"].childElements(), json);
        return json.successJSON();
      }

      json.add("value", pDevice->getSceneValue(id));
      DeviceFeatures_t features = pDevice->getDeviceFeatures();
      if (features.hasOutputAngle) {
        json.add("angle", pDevice->getSceneAngle(id));
      }
      return json.successJSON();

    } else if (_request.getMethod() == "getApartmentScenes") {
      if (!pDevice->isVdcDevice() || !pDevice->getHasActions()) {
        return JSONWriter::failure("Device does not support action configuration");
      }

      google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
      vdcapi::PropertyElement* e1 = query.Add();
      e1->set_name("scenes");
      for (int id = 64; id < 128; id++) {
        vdcapi::PropertyElement* e2 = e1->add_elements();
        e2->set_name(intToString(id));
      }
      vdcapi::Message message = pDevice->getVdcProperty(query);
      VdcElementReader reader(message.vdc_response_get_property().properties());

      JSONWriter json;
      json.add("scenes");
      ProtobufToJSon::processElementsPretty(reader["scenes"].childElements(), json);
      return json.successJSON();

    } else if (_request.getMethod() == "getSceneMode") {
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
    } else if (_request.getMethod() == "setSceneMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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

    } else if (_request.getMethod() == "getTransitionTime") {
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
    } else if (_request.getMethod() == "setTransitionTime") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int id = strToIntDef(_request.getParameter("dimtimeIndex"), -1);
      if((id  < 0) || (id > 2)) {
        return JSONWriter::failure("Invalid or missing parameter 'dimtimeIndex'");
      }
      int up = strToIntDef(_request.getParameter("up"), -1);
      int down = strToIntDef(_request.getParameter("down"), -1);
      pDevice->setDeviceTransitionTime(id, up, down);
      return JSONWriter::success();

    } else if (_request.getMethod() == "getLedMode") {
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
    } else if (_request.getMethod() == "setLedMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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

    } else if (_request.getMethod() == "getBinaryInputs") {
      JSONWriter json;
      json.startArray("inputs");
      auto&& binputs = pDevice->getBinaryInputs();
      for (auto&& it = binputs.begin();
          it != binputs.end();
          it ++) {
        json.startObject();
        json.add("inputIndex", (*it)->m_inputIndex);
        json.add("inputId", (*it)->m_inputId);
        json.add("inputType", (*it)->m_inputType);
        json.add("targetGroup", (*it)->m_targetGroupId);
        json.endObject();
      }
      json.endArray();
      return json.successJSON();
    } else if (_request.getMethod() == "setBinaryInputType") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }
      int type = strToIntDef(_request.getParameter("type"), -1);
      if (type < 0 || type > 254) {
        return JSONWriter::failure("Invalid or missing parameter 'type'");
      }
      pDevice->setDeviceBinaryInputType(index, static_cast<BinaryInputType>(type));
      return JSONWriter::success();
    } else if (_request.getMethod() == "setBinaryInputTarget") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }
      int gid = strToIntDef(_request.getParameter("groupId"), -1);
      if (gid < 0 || gid > GroupIDGlobalAppMax) {
        return JSONWriter::failure("Invalid or missing parameter 'groupId'");
      }
      pDevice->setDeviceBinaryInputTargetId(index, gid);
      return JSONWriter::success();
    } else if (_request.getMethod() == "setBinaryInputId") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int index = strToIntDef(_request.getParameter("index"), -1);
      if (index < 0) {
        return JSONWriter::failure("Invalid or missing parameter 'index'");
      }
      int id = strToIntDef(_request.getParameter("inputId"), -1);
      if (id < 0 || id > 15) {
        return JSONWriter::failure("Invalid or missing parameter 'inputId'");
      }
      pDevice->setDeviceBinaryInputId(index, static_cast<BinaryInputId>(id));
      return JSONWriter::success();

    } else if (_request.getMethod() == "getAKMInputTimeouts") {
      JSONWriter json;
      int onDelay, offDelay;
      pDevice->getDeviceAKMInputTimeouts(onDelay, offDelay);
      json.add("ondelay", onDelay);
      json.add("offdelay", offDelay);
      return json.successJSON();
    } else if (_request.getMethod() == "setAKMInputTimeouts") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      std::string mode = _request.getParameter("mode");
      if (mode.empty()) {
        return JSONWriter::failure("Invalid or missing parameter 'mode'");
      }

      if ((pDevice->getDeviceType() != DEVICE_TYPE_AKM) &&
          (pDevice->getDeviceType() != DEVICE_TYPE_UMR)) {
        return JSONWriter::failure("This device does not support AKM properties");
      }

      JSONWriter json;
      // Partner device cannot be / stay in slave button mode when master
      // device is configured to sensor.
      // It would be hardware misconfiguration.
      if (pDevice->is2WayMaster()) {
        auto&& pPartnerDevice = pDevice->getPartnerDevice();
        if (pPartnerDevice->is2WaySlave()) {
          pPartnerDevice->setDeviceButtonInputMode(ButtonInputMode::STANDARD);

          json.add("action", "add");
          json.add("device");
          toJSON(DeviceReference(pPartnerDevice, &m_Apartment), json);
        }
      }

      if (mode == BUTTONINPUT_AKM_STANDARD) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_STANDARD);
      } else if (mode == BUTTONINPUT_AKM_INVERTED) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_INVERTED);
      } else if (mode == BUTTONINPUT_AKM_ON_RISING_EDGE) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_ON_RISING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_ON_FALLING_EDGE) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_ON_FALLING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_OFF_RISING_EDGE) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_OFF_RISING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_OFF_FALLING_EDGE) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_OFF_FALLING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_RISING_EDGE) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_RISING_EDGE);
      } else if (mode == BUTTONINPUT_AKM_FALLING_EDGE) {
        pDevice->setDeviceButtonInputMode(ButtonInputMode::AKM_FALLING_EDGE);
      } else {
        return JSONWriter::failure("Unsupported mode: " + mode);
      }

      return json.successJSON();
    } else if (_request.getMethod() == "getSensorValue") {
      int id = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if (id < 0 || id >= pDevice->getSensorCount()) {
        return JSONWriter::failure("Invalid or missing parameter 'sensorIndex'");
      }
      DeviceSensorValue_t result = pDevice->getDeviceSensorValueEx(id);
      JSONWriter json;
      json.add("sensorIndex", id);
      json.add("sensorValue", result.value);
      return json.successJSON();
    } else if (_request.getMethod() == "getSensorValue2") {
      int type = strToIntDef(_request.getParameter("sensorType"), -1);
      int id = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if (id < 0) {
        if (type > 0) {
          boost::shared_ptr<DeviceSensor_t> pSensor = pDevice->getSensorByType(static_cast<SensorType>(type));
          id = pSensor->m_sensorIndex;
        } else {
          return JSONWriter::failure("Invalid or missing parameter 'sensorIndex'");
        }
      }
      DeviceSensorValue_t value = pDevice->getDeviceSensorValueEx(id);
      JSONWriter json;
      json.add("sensorIndex", id);
      json.add("value", value.value);
      json.add("age", value.valueAge);
      json.add("timestamp", value.timestamp.toISO8601_ms());
      json.add("contextId", value.contextId);
      json.add("contextMsg", value.contextMsg);
      return json.successJSON();
    } else if (_request.getMethod() == "getSensorType") {
      int id = strToIntDef(_request.getParameter("sensorIndex"), -1);
      if((id < 0) || (id > 255)) {
        return JSONWriter::failure("Invalid or missing parameter 'sensorIndex'");
      }
      boost::shared_ptr<DeviceSensor_t> sensor = pDevice->getSensor(id);
      JSONWriter json;
      json.add("sensorIndex", id);
      json.add("sensorType", sensor->m_sensorType);
      return json.successJSON();

    } else if (_request.getMethod() == "getSensorEventTableEntry") {
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
      json.add("minimalDuration", event.minimalDuration);
      if (event.action == 2) {
        json.add("buttonNumber", event.buttonNumber);
        json.add("clickType", event.clickType);
      }
      return json.successJSON();

    } else if (_request.getMethod() == "setSensorEventTableEntry") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      if (_request.hasParameter("minimalDuration")) {
        int minimalDuration = strToIntDef(_request.getParameter("minimalDuration"), -1);
        if (minimalDuration < 0 || minimalDuration > 0xff) {
          return JSONWriter::failure("Invalid or missing parameter 'minimalDuration'");
        }
        event.minimalDuration = minimalDuration;
      }
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
    } else if (_request.getMethod() == "addToArea") {
      int areaScene = strToIntDef(_request.getParameter("areaScene"), -1);
      if (areaScene < 0) {
        return JSONWriter::failure("Missing parameter 'areaScene'");
      }
      pDevice->configureAreaMembership(areaScene, true);
      return JSONWriter::success();
    } else if (_request.getMethod() == "removeFromArea") {
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
        uint8_t channelIndex = pDevice->getOutputChannelIndex(channels->at(i).first);
        boost::shared_ptr<DeviceChannel_t> pChannel = pDevice->getOutputChannel(channelIndex);
        json.startObject();
        json.add("channel", pChannel->m_channelName);
        json.add("channelId", pChannel->m_channelId);
        json.add("channelType", pChannel->m_channelType);
        json.add("index", channelIndex);
        json.add("value",
                convertFromOutputChannelValue(
                    pChannel->m_channelType,
                    pDevice->getDeviceOutputChannelValue(channelIndex)));
        json.endObject();
        // don't flood the bus on bulk requests
        if (!pDevice->isVdcDevice() && (channels->size() > 1) && (i < channels->size() - 1)) {
          sleep(1);
        }
      }
      json.endArray();

      return json.successJSON();
    } else if (_request.getMethod() == "setOutputChannelValue") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      bool applyNow = strToIntDef(_request.getParameter("applyNow"), 1);
      std::string vals = _request.getParameter("channelvalues");
      if (vals.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channelvalues'");
      }

      boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > channels =  parseOutputChannelsWithValues(vals);

      for (size_t i = 0; i < channels->size(); i++) {
        uint8_t channelIndex = pDevice->getOutputChannelIndex(boost::get<0>(channels->at(i)));
        pDevice->setDeviceOutputChannelValue(channelIndex,
                                             boost::get<1>(channels->at(i)),
                                             boost::get<2>(channels->at(i)),
                                             applyNow && (i == (channels->size() - 1)));
        // don't flood the bus on bulk requests
        if (!pDevice->isVdcDevice() && (channels->size() > 1) && (i < channels->size() - 1)) {
          sleep(1);
        }
      }

      return JSONWriter::success();
    } else if (_request.getMethod() == "getOutputChannelValue2") {
      if (!pDevice->isVdcDevice()) {
        return JSONWriter::failure("This call is currently only supported on vDCs");
      }

      google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
      vdcapi::PropertyElement* channelStates = query.Add();
      channelStates->set_name("channelStates");

      std::string str_chan = _request.getParameter("channels");
      std::vector<std::string> chans = dss::splitString(str_chan, ';');
      for (size_t i = 0; i < chans.size(); i++) {
        vdcapi::PropertyElement *channelIndexProp = channelStates->add_elements();
        channelIndexProp->set_name(chans[i]);
      }

      vdcapi::Message message = pDevice->getVdcProperty(query);
      vdcapi::vdc_ResponseGetProperty response = message.vdc_response_get_property();
      VdcElementReader reader(message.vdc_response_get_property().properties());

      JSONWriter json;
      json.add("channels");
      ProtobufToJSon::processElementsPretty(reader["channelStates"].childElements(), json);
      return json.successJSON();

    } else if (_request.getMethod() == "setOutputChannelValue2") {
      bool applyNow = strToIntDef(_request.getParameter("applyNow"), 1);
      std::string inputjson = _request.getParameter("channels");
      if (inputjson.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channels'");
      }

      rapidjson::Document d;
      d.Parse(inputjson.c_str());
      if (!d.IsObject()) {
        return JSONWriter::failure("Invalid parameter channels: not a JSON object");
      }
      rapidjson::Value::ConstMemberIterator lastElement = d.MemberBegin();
      if (d.MemberCount() > 0) {
        lastElement = d.MemberEnd() - 1;
      }
      for (rapidjson::Value::ConstMemberIterator i = d.MemberBegin(); i != d.MemberEnd(); ++i) {
        // verify channelId is valid and available
        boost::shared_ptr<DeviceChannel_t> chaninfo = pDevice->getOutputChannelById(i->name.GetString());
        std::pair<uint8_t, uint8_t> chantype =  getOutputChannelIdAndSize(chaninfo->m_channelId);

        const rapidjson::Value& valueObj =
            i->value.IsObject() && i->value.HasMember("value") ? i->value["value"] : i->value;
        double value;

        if (valueObj.IsNumber()) {
          value = valueObj.GetDouble();
        } else if (valueObj.IsBool()) {
          value = valueObj.GetBool() ? 1 : 0;
        } else if (valueObj.IsString()) {
          value = strToDouble(valueObj.GetString());
        } else if (valueObj.IsObject() || valueObj.IsNull()) {
          continue; // ignore empty objects, like: "airFlowAuto":{ }
        } else {
          return JSONWriter::failure("Invalid parameter channels: value type unsupported");
        }
        pDevice->setDeviceOutputChannelValue(chaninfo->m_channelIndex,
            chantype.second,
            (uint16_t) value,
            applyNow && (i == lastElement));

        // don't flood the bus on bulk requests
        if (!pDevice->isVdcDevice() && (i != lastElement)) {
          sleep(1);
        }
      }
      return JSONWriter::success();

    } else if (_request.getMethod() == "getOutputChannelSceneValue2") {
      if (!pDevice->isVdcDevice()) {
        return JSONWriter::failure("This call is currently only supported on vDCs");
      }

      int sceneNum = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((sceneNum < 0) || (sceneNum > MaxSceneNumber)) {
        return JSONWriter::failure("Missing or invalid parameter 'sceneNumber'");
      }

      google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
      vdcapi::PropertyElement* scenes = query.Add();
      scenes->set_name("scenes");
      vdcapi::PropertyElement* sn = scenes->add_elements();
      sn->set_name(_request.getParameter("sceneNumber"));

      std::string str_chan = _request.getParameter("channels");
      std::vector<std::string> chans = dss::splitString(str_chan, ';');
      if (chans.size() > 0) {
        vdcapi::PropertyElement* cn = sn->add_elements();
        cn->set_name("channels");
        for (size_t i = 0; i < chans.size(); i++) {
          vdcapi::PropertyElement *channelIndexProp = cn->add_elements();
          channelIndexProp->set_name(chans[i]);
        }
      }

      vdcapi::Message message = pDevice->getVdcProperty(query);
      vdcapi::vdc_ResponseGetProperty response = message.vdc_response_get_property();
      VdcElementReader reader(message.vdc_response_get_property().properties());

      JSONWriter json;
      json.add("scenes");
      ProtobufToJSon::processElementsPretty(reader["scenes"].childElements(), json);
      return json.successJSON();

    } else if (_request.getMethod() == "setOutputChannelSceneValue2") {
      if (!pDevice->isVdcDevice()) {
        return JSONWriter::failure("This call is currently only supported on vDCs");
      }

      std::string inputjson = _request.getParameter("channels");
      if (inputjson.empty()) {
        return JSONWriter::failure("Missing or invalid parameter 'channels'");
      }

      int sceneNum = strToIntDef(_request.getParameter("sceneNumber"), -1);
      if ((sceneNum < 0) || (sceneNum > MaxSceneNumber)) {
        return JSONWriter::failure("Missing or invalid parameter 'sceneNumber'");
      }

      rapidjson::Document d;
      d.Parse(inputjson.c_str());

      if (!d.IsObject()) {
        return JSONWriter::failure("Invalid parameter channels: not a JSON object");
      }

      google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
      vdcapi::PropertyElement* scenes = query.Add();
      scenes->set_name("scenes");
      vdcapi::PropertyElement* scene = scenes->add_elements();
      scene->set_name(_request.getParameter("sceneNumber"));

      vdcapi::PropertyElement* channels = scene->add_elements();
      channels->set_name("channels");

      for (rapidjson::Value::ConstMemberIterator i = d.MemberBegin(); i != d.MemberEnd(); ++i) {
        vdcapi::PropertyElement* protochannel = channels->add_elements();
        protochannel->set_name(i->name.GetString());
        const rapidjson::Value& valueObj =
            i->value.IsObject() && i->value.HasMember("value") ? i->value["value"] : i->value;

        if (i->value.IsObject()) {
          if (i->value.HasMember("dontCare") && i->value["dontCare"].IsBool()) {
            vdcapi::PropertyElement* dc = protochannel->add_elements();
            dc->set_name("dontCare");
            dc->mutable_value()->set_v_bool(i->value["dontCare"].GetBool());
          }
          if (i->value.HasMember("command") && i->value["command"].IsString()) {
            vdcapi::PropertyElement* automatic = protochannel->add_elements();
            automatic->set_name("command");
            automatic->mutable_value()->set_v_string(i->value["command"].GetString());
          }
        }
        if (valueObj.IsNumber()) {
          vdcapi::PropertyElement* val = protochannel->add_elements();
          val->set_name("value");
          val->mutable_value()->set_v_double(valueObj.GetDouble());
        } else if (valueObj.IsBool()) {
          vdcapi::PropertyElement* val = protochannel->add_elements();
          val->set_name("value");
          val->mutable_value()->set_v_bool(valueObj.GetBool());
        } else if (valueObj.IsString()) {
          vdcapi::PropertyElement* val = protochannel->add_elements();
          val->set_name("value");
          val->mutable_value()->set_v_string(valueObj.GetString());
        }
      }

      if (channels->elements_size() > 0) {
          pDevice->setVdcProperty(query);
      }

      return JSONWriter::success();
    } else if (_request.getMethod() == "setOutputChannelDontCareFlag") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      json.startArray("channels");

      uint16_t value = pDevice->getDeviceOutputChannelDontCareFlags(scene);
      for (int i = 0; i < pDevice->getOutputChannelCount(); i++) {
        boost::shared_ptr<DeviceChannel_t> pChannel = pDevice->getOutputChannel(i);
        json.startObject(pChannel->m_channelId);
        json.add("index", pChannel->m_channelIndex);
        json.add("channel", getOutputChannelName(pChannel->m_channelType));
        json.add("dontCare", ((value & (1 << i)) > 0));
        json.endObject();
      }
      json.endArray();
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
        json.add("value", convertFromOutputChannelValue(channels->at(i).first, pDevice->getDeviceOutputChannelSceneValue(channels->at(i).first, scene)));
        json.endObject();
      }
      json.endArray();

      return json.successJSON();
    } else if (_request.getMethod() == "setOutputChannelSceneValue") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
        if (!pDevice->isVdcDevice() && (channels->size() > 1) && (i < channels->size() - 1)) {
          sleep(1);
        }
      }

      return JSONWriter::success();
    } else if (_request.getMethod() == "setValveTimerMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceValveTimerSpec_t timerSpec;
      device->getDeviceValveTimer(timerSpec);

      unsigned int protTimer;
      if (_request.getParameter("valveProtectionTimer", protTimer)) {
        if (protTimer > std::numeric_limits<uint16_t>::max()) {
          return JSONWriter::failure("valveProtectionTimer too large");
        }
        timerSpec.protectionTimer = protTimer;
      };

      int emergencyValue;
      if (_request.getParameter("emergencyValue", emergencyValue)) {
        if (emergencyValue < -100 || emergencyValue > 100) {
          return JSONWriter::failure("emergencyValue out of [-100:100] range");
        }
        timerSpec.emergencyControlValue = emergencyValue;
      };

      unsigned int emergencyTimer;
      if (_request.getParameter("emergencyTimer", emergencyTimer)) {
        if (emergencyTimer > std::numeric_limits<uint8_t>::max()) {
          return JSONWriter::failure("emergencyTimer too big");
        }
        timerSpec.emergencyTimer = emergencyTimer;
      };

      device->setDeviceValveTimer(timerSpec);
      return JSONWriter::success();

    } else if (_request.getMethod() == "getValveTimerMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceValveTimerSpec_t timerSpec;
      device->getDeviceValveTimer(timerSpec);

      JSONWriter json;
      json.add("valveProtectionTimer", timerSpec.protectionTimer);
      json.add("emergencyValue", timerSpec.emergencyControlValue);
      json.add("emergencyTimer", timerSpec.emergencyTimer);
      return json.successJSON();

    } else if (_request.getMethod() == "setValvePwmMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }
      DeviceValvePwmSpec_t pwmSpec;
      device->getDeviceValvePwm(pwmSpec);

      unsigned pwmPeriod;
      if (_request.getParameter("pwmPeriod", pwmPeriod)) {
        if (pwmPeriod > std::numeric_limits<uint16_t>::max()) {
          return JSONWriter::failure("valveProtectionTimer too large");
        }
        pwmSpec.pwmPeriod = pwmPeriod;
      };
      int value;
      if (_request.getParameter("pwmMinX", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMinX out of [0:100] range");
        }
        pwmSpec.pwmMinX = value;
      };
      if (_request.getParameter("pwmMaxX", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMaxX out of [0:100] range");
        }
        pwmSpec.pwmMaxX = value;
      };
      if (_request.getParameter("pwmMinY", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMinY out of [0:100] range");
        }
        pwmSpec.pwmMinY = value;
      };
      if (_request.getParameter("pwmMaxY", value)) {
        if (value < 0  || value > 100) {
          return JSONWriter::failure("pwmMaxY out of [0:100] range");
        }
        pwmSpec.pwmMaxY = value;
      };
      int offset;
      if (_request.getParameter("pwmOffset", offset)) {
        if (offset < -100 || offset > 100) {
          return JSONWriter::failure("PWM offset out of [-100:100] range");
        }
        pwmSpec.pwmOffset = offset;
      };

      device->setDeviceValvePwm(pwmSpec);
      return JSONWriter::success();

    } else if (_request.getMethod() == "getValvePwmMode") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      DeviceValvePwmSpec_t pwmSpec;
      device->getDeviceValvePwm(pwmSpec);

      JSONWriter json;
      json.add("pwmPeriod", pwmSpec.pwmPeriod);
      json.add("pwmMinX", pwmSpec.pwmMinX);
      json.add("pwmMaxX", pwmSpec.pwmMaxX);
      json.add("pwmMinY", pwmSpec.pwmMinY);
      json.add("pwmMaxY", pwmSpec.pwmMaxY);
      json.add("pwmOffset", pwmSpec.pwmOffset);
      return json.successJSON();

    } else if (_request.getMethod() == "getValvePwmState") {
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("No device for given dsuid");
      }

      DeviceValvePwmStateSpec_t pwmStateSpec;
      device->getDeviceValvePwmState(pwmStateSpec);

      JSONWriter json;
      json.add("pwmValue", pwmStateSpec.pwmValue);
      json.add("pwmPriorityMode", pwmStateSpec.pwmPriorityMode);
      return json.successJSON();

    } else if (_request.getMethod() == "setValveControlMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
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
        device->setDeviceBlinkRepetitions((uint8_t)blinkCount);
      }

      if (onDelay != -1) {
        device->setDeviceBlinkOnDelay(onDelay);
      }

      if (offDelay != -1) {
         device->setDeviceBlinkOffDelay(offDelay);
      }

      DeviceSceneSpec_t config;
      pDevice->getDeviceSceneMode(SceneImpulse, config);

      if (config.flashmode != 1) {
        config.flashmode = 1;
        pDevice->setDeviceSceneMode(SceneImpulse, config);
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
      device->getDeviceBlinkSettings(&umr_ondelay, &umr_offdelay, &umr_count);
      json.add("count", umr_count);
      json.add("ondelay", umr_ondelay);
      json.add("offdelay", umr_offdelay);
      return json.successJSON();

    } else if (_request.getMethod() == "setCardinalDirection") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      CardinalDirection_t direction;
      JSONWriter json;
      std::string tmp;

      if (!_request.getParameter("direction", tmp)) {
        return json.failure("missing paramter : direction");
      }
      if (!parseCardinalDirection(tmp, &direction)) {
        return json.failure("invalid direction paramter : " + tmp);
      }
      pDevice->setCardinalDirection(direction);
      return json.successJSON();

    } else if (_request.getMethod() == "getCardinalDirection") {
      JSONWriter json;
      json.add("direction", toString(pDevice->getCardinalDirection()));
      return json.successJSON();

    } else if (_request.getMethod() == "setWindProtectionClass") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      WindProtectionClass_t protection;
      JSONWriter json;
      int tmp;

      if (!_request.getParameter("class", tmp)) {
        return json.failure("missing parameter : class");
      }
      if (!convertWindProtectionClass(tmp, &protection)) {
        return json.failure("invalid protection class: " + intToString(tmp));
      }
      pDevice->setWindProtectionClass(protection);
      return json.successJSON();

    } else if (_request.getMethod() == "getWindProtectionClass") {
      JSONWriter json;
      json.add("class", intToString(pDevice->getWindProtectionClass()));
      return json.successJSON();

    } else if (_request.getMethod() == "setFloor") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int floor;
      JSONWriter json;

      if (!_request.getParameter("floor", floor)) {
        return json.failure("missing parameter : floor");
      }
      pDevice->setFloor(floor);
      return json.successJSON();

    } else if (_request.getMethod() == "getFloor") {
      JSONWriter json;
      json.add("floor", intToString(pDevice->getFloor()));
      return json.successJSON();

    } else if (_request.getMethod() == "setOutputAfterImpulse") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      JSONWriter json;

      std::string output = _request.getParameter("output");
      int value = 0;
      int dontcare = true;

      if (output == "on") {
        value = 255;
        dontcare = false;
      } else if (output == "off") {
        value = 0;
        dontcare = false;
      } else if (output == "retain") {
        value = 0;
        dontcare = true;
      } else {
        return json.failure("invalid or missing \"output\" parameter");
      }

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)SceneImpulse)) {
        return json.failure("Device settings are being updated, please try again later");
      }

      DeviceSceneSpec_t config;
      pDevice->getDeviceSceneMode(SceneImpulse, config);

      if ((config.dontcare != dontcare) || (config.flashmode != 1)) {
        config.flashmode = 1;
        config.dontcare = dontcare;
        pDevice->setDeviceSceneMode(SceneImpulse, config);
      }

      pDevice->setSceneValue(SceneImpulse, value);

      return json.successJSON();
    } else if (_request.getMethod() == "getOutputAfterImpulse") {
      JSONWriter json;

      if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)SceneImpulse)) {
        return json.failure("Device settings are being updated, please try again later");
      }

      DeviceSceneSpec_t config;
      pDevice->getDeviceSceneMode(SceneImpulse, config);

      int value = pDevice->getSceneValue(SceneImpulse);

      if ((config.dontcare == false) && (value <= 127)) {
        json.add("output", "off");
      } else if ((config.dontcare == false) && (value >= 128)) {
        json.add("output", "on");
      } else if ((config.dontcare == true) && (value <= 127)) {
        json.add("output", "retain");
      } else {
        return json.failure("Encountered invalid output after impulse confiugration");
      }
      return json.successJSON();
    } else if (_request.getMethod() == "getFirstSeen") {
      JSONWriter json;
      DateTime firstSeen = pDevice->getFirstSeen();
      std::string date = firstSeen.toISO8601();
      json.add("time", date);
      return json.successJSON();
    } else if (_request.getMethod() == "setMaxMotionTime") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      JSONWriter json;

      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("no device for given dsuid");
      }

      DeviceFeatures_t features = device->getDeviceFeatures();
      if (!features.posTimeMax) {
        return JSONWriter::failure("maximum motion time setting not supported by "
                            "this device");
      }

      int value = strToIntDef(_request.getParameter("seconds"), -1);
      if((value  < 0) || (value > 655)) {
        return JSONWriter::failure("missing or invalid 'seconds' parameter");
      }

      device->setDeviceMaxMotionTime(value);

      return json.successJSON();
    } else if (_request.getMethod() == "getMaxMotionTime") {
      JSONWriter json;
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("no device for given dsuid");
      }

      uint16_t value = 0;
      DeviceFeatures_t features = device->getDeviceFeatures();
      if (features.posTimeMax) {
        value = device->getDeviceMaxMotionTime();
      }

      json.add("supported", features.posTimeMax);
      json.add("value", value);
      return json.successJSON();
    } else if (_request.getMethod() == "setVisibility") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      JSONWriter json;
      std::string param;
      if (!_request.getParameter("visibility", param)) {
        return json.failure("missing parameter: visibility");
      }

      if ((param != "0") && (param != "1")) {
        return json.failure("invalid value for visibility parameter");
      }

      int v = strToInt(param);

      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("no device for given dsuid");
      }

      bool wasVisible = device->isVisible();
      device->setDeviceVisibility(v);
      bool isVisible = device->isVisible();

      const DeviceReference d(device, &m_Apartment);
      if (wasVisible && !isVisible) {
        dsuid_t main = device->getMainDeviceDSUID();
        dsuid_t dsuid = device->getDSID();

        if (!dsuid_equal(&dsuid, &main)) {
          boost::shared_ptr<Device> main_device;
          try {
            main_device = m_Apartment.getDeviceByDSID(main);
            boost::shared_ptr<Zone> zone = m_Apartment.getZone(
                                                    main_device->getZoneID());
            m_manipulator.addDeviceToZone(device, zone);
          } catch (ItemNotFoundException &e) {
            Logger::getInstance()->log("DeviceRequestHandler: could not move paired device into main devices\' zone");
          }
        }
        json.startArray("devices");
        toJSON(d, json);
        json.endArray();
        json.add("action", "remove");
      } else if (!wasVisible && isVisible) {
        json.startArray("devices");
        toJSON(d, json);
        json.endArray();
        json.add("action", "add");
      } else {
        json.add("action", "none");
      }

      return json.successJSON();
    } else if (_request.getMethod() == "setSwitchThreshold") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      JSONWriter json;

      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("no device for given dsuid");
      }

      int value = strToIntDef(_request.getParameter("threshold"), -1);
      if((value  < 1) || (value > 254)) {
        return JSONWriter::failure("missing or invalid 'threshold' parameter");
      }

      device->setSwitchThreshold(value);

      return json.successJSON();
    } else if (_request.getMethod() == "getSwitchThreshold") {
      JSONWriter json;
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("no device for given dsuid");
      }

      uint8_t value = device->getSwitchThreshold();

      json.add("threshold", value);
      return json.successJSON();
    } else if (_request.getMethod() == "setTemperatureOffset") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      JSONWriter json;

      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("no device for given dsuid");
      }

      int value = strToIntDef(_request.getParameter("offset"), -1);
      if((value  < -128) || (value > 127)) {
        return JSONWriter::failure("missing or invalid 'offset' parameter");
      }

      device->setTemperatureOffset(value);

      return json.successJSON();
    } else if (_request.getMethod() == "getTemperatureOffset") {
      JSONWriter json;
      boost::shared_ptr<Device> device;
      try {
        device = getDeviceByDSID(_request);
      } catch(std::runtime_error& e) {
        return JSONWriter::failure("no device for given dsuid");
      }

      int8_t value = device->getTemperatureOffset();

      json.add("offset", value);
      return json.successJSON();
    } else if (_request.getMethod() == "setSK204Config") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      std::string mode = _request.getParameter("mode");
      if (mode.empty()) {
        return JSONWriter::failure("missing parameter: mode");
      }

      if ((mode != "temperaturecontrol") &&
          (mode != "temperaturecontrolventilation")) {
        return JSONWriter::failure("invalid value for parameter \"mode\"");
      }

      if (!((pDevice->getDeviceType() == DEVICE_TYPE_SK) &&
            (pDevice->getDeviceNumber() == 204)) || !pDevice->isMainDevice())  {
        JSONWriter::failure("request not supported on this device");
      }

      JSONWriter json;
      std::string action = "none";

      auto pPartnerDevice = pDevice->getPartnerDevice();
      bool targetVisibility = false; // only "temperaturecontrol"
      bool currentVisibility = pPartnerDevice->isVisible();

      if (mode == "temperaturecontrolventilation") {
        targetVisibility = true;
      }

      if (targetVisibility != currentVisibility) {
        if (currentVisibility == true) {
          action = "remove";
        } else {
          action = "add";
        }

        pPartnerDevice->setVisibility(targetVisibility);

        if (targetVisibility == true) {
          // follow the same logic with paired devices as everywhere else
          if (pDevice->getZoneID() != pPartnerDevice->getZoneID()) {
            boost::shared_ptr<Zone> zone = m_Apartment.getZone(
                                                      pDevice->getZoneID());
            m_manipulator.addDeviceToZone(pPartnerDevice, zone);
          }

          // #3450 - remove slave devices from clusters
          m_manipulator.deviceRemoveFromGroups(pPartnerDevice);
        }
      }

      json.add("action", action);
      DeviceReference dr(pPartnerDevice, &m_Apartment);
      json.add("device");
      toJSON(dr, json);

      return json.successJSON();
    } else if (_request.getMethod() == "getSK204Config") {

      if (!((pDevice->getDeviceType() == DEVICE_TYPE_SK) &&
            (pDevice->getDeviceNumber() == 204)) || !pDevice->isMainDevice()) {
        JSONWriter::failure("request not supported on this device");
      }

      JSONWriter json;
      auto pPartnerDevice = pDevice->getPartnerDevice();
      std::string mode = "temperaturecontrol";

      if (pPartnerDevice->isVisible()) {
        mode = "temperaturecontrolventilation";
      }

      json.add("mode", mode);
      return json.successJSON();
    } else if (_request.getMethod() == "setSK204DisplayMode") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      int humidity = -1;
      int room_setpoint = -1;

      if (!((pDevice->getDeviceType() == DEVICE_TYPE_SK) &&
            (pDevice->getDeviceNumber() == 204)) || !pDevice->isMainDevice())  {
        JSONWriter::failure("request not supported on this device");
      }

      if (_request.hasParameter("roomsetpoint")) {
        int value = strToIntDef(_request.getParameter("roomsetpoint"), -1);
        if ((value != 0) && (value != 1)) {
          return JSONWriter::failure("Invalid 'room_setpoint' parameter value");
        } else {
          room_setpoint = value;
        }
      }

      if (_request.hasParameter("humidity")) {
        int value = strToIntDef(_request.getParameter("humidity"), -1);
        if ((value != 0) && (value != 1)) {
          return JSONWriter::failure("Invalid 'humidity' parameter value");
        } else {
          humidity = value;
        }
      }

      if ((room_setpoint == -1) && (humidity == -1)) {
        return JSONWriter::success();
      }

      uint8_t config = pDevice->getDeviceConfig(CfgClassConfigDataSK, CfgFunction_SK_Config);
      uint8_t read_config = config;

      if (room_setpoint != -1) {
        // Bit 1: room setpoint
        config ^= (-room_setpoint ^ config) & (1 << 1);
      }

      if (humidity != -1) {
        // Bit 3: humidity
        config ^= (-humidity ^ config) & (1 << 3);
      }

      if (config != read_config) {
        pDevice->setDeviceConfig(CfgClassConfigDataSK, CfgFunction_SK_Config, config);
      }

      return JSONWriter::success();
    } else if (_request.getMethod() == "getSK204DisplayMode") {

      if (!((pDevice->getDeviceType() == DEVICE_TYPE_SK) &&
            (pDevice->getDeviceNumber() == 204)) || !pDevice->isMainDevice())  {
        JSONWriter::failure("request not supported on this device");
      }

      uint8_t config = pDevice->getDeviceConfig(CfgClassConfigDataSK, CfgFunction_SK_Config);

      // Bit 1: room setpoint
      uint8_t room_setpoint = (config >> 1) & 1;
      // Bit 3: humidity
      uint8_t humidity = (config >> 3) & 1;

      JSONWriter json;
      json.add("roomsetpoint", (bool)room_setpoint);
      json.add("humidity", (bool)humidity);
      return json.successJSON();
    } else if (_request.getMethod() == "setSK204BacklightTimeout") {
      boost::recursive_mutex::scoped_lock lock(m_setConfigMutex);
      if (((pDevice->getDeviceType() != DEVICE_TYPE_SK) &&
           (pDevice->getDeviceNumber() != 204)) || !pDevice->isMainDevice()) {
        JSONWriter::failure("request not supported on this device");
      }

      int value = strToIntDef(_request.getParameter("seconds"), -1);
      if ((value < 1) || (value > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid 'seconds' parameter value");
      }

      pDevice->setDeviceConfig(CfgClassConfigDataSK, CfgFunction_SK_BacklightDuration, (uint8_t)value);

      return JSONWriter::success();
    } else if (_request.getMethod() == "getSK204BacklightTimeout") {

      if (!((pDevice->getDeviceType() == DEVICE_TYPE_SK) &&
            (pDevice->getDeviceNumber() == 204)) || !pDevice->isMainDevice())  {
        JSONWriter::failure("request not supported on this device");
      }

      uint8_t seconds = pDevice->getDeviceConfig(CfgClassConfigDataSK, CfgFunction_SK_BacklightDuration);

      JSONWriter json;
      json.add("seconds", (int)seconds);
      return json.successJSON();

    } else if (_request.getMethod() == "setConsumptionVisualization") {
      auto value = strToIntDef(_request.getParameter("value"), -1);
      switch (value) {
        case 0:
          pDevice->setConsumptionVisualizationEnabled(false);
          return JSONWriter::success();
        case 1:
          pDevice->setConsumptionVisualizationEnabled(true);
          return JSONWriter::success();
        default:
          return JSONWriter::failure("invalid value");
      }

    } else if (_request.getMethod() == "getConsumptionVisualization") {
      JSONWriter json;
      json.add("value", pDevice->getConsumptionVisualizationEnabled());
      return json.successJSON();

    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // jsonHandleRequest
} // namespace dss
