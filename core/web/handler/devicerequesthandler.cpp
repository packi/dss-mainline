/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include "core/model/apartment.h"
#include "core/model/device.h"

#include "core/web/json.h"

namespace dss {


  //=========================================== DeviceRequestHandler

  DeviceRequestHandler::DeviceRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  class DeviceNotFoundException : public std::runtime_error {
  public:
    DeviceNotFoundException(const std::string& __arg)
    : std::runtime_error(__arg)
    { }
  };

  Device* DeviceRequestHandler::getDeviceFromRequest(const RestfulRequest& _request) {
    Device* result = getDeviceByDSID(_request);
    if(result == NULL) {
      result = getDeviceByName(_request);
    }
    if(result == NULL) {
      throw DeviceNotFoundException("Need parameter name or dsid to identify device");
    }
    return result;
  } // getDeviceFromRequest

  Device* DeviceRequestHandler::getDeviceByDSID(const RestfulRequest& _request) {
    Device* result = NULL;
    std::string deviceDSIDString = _request.getParameter("dsid");
    if(!deviceDSIDString.empty()) {
      try {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        try {
          Device& device = m_Apartment.getDeviceByDSID(deviceDSID);
          result = &device;
        } catch(std::runtime_error& e) {
          throw DeviceNotFoundException("Could not find device with dsid '" + deviceDSIDString + "'");
        }
      } catch(std::invalid_argument& e) {
        throw DeviceNotFoundException("Could not parse dsid '" + deviceDSIDString + "'");
      }
    }
    return result;
  } // getDeviceByDSID
  
  Device* DeviceRequestHandler::getDeviceByName(const RestfulRequest& _request) {
    Device* result = NULL;
    std::string deviceName = _request.getParameter("name");
    try {
      Device& device = m_Apartment.getDeviceByName(deviceName);
      result = &device;
    } catch(std::runtime_error&  e) {
      throw DeviceNotFoundException("Could not find device named '" + deviceName + "'");
    }
    return result;
  } // getDeviceByName

  boost::shared_ptr<JSONObject> DeviceRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    Device* pDevice = NULL;
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
          Group& group = pDevice->getGroupByIndex(iGroup);
          boost::shared_ptr<JSONObject> groupObj(new JSONObject());
          groups->addElement("", groupObj);

          groupObj->addProperty("id", group.getID());
          if(!group.getName().empty()) {
            groupObj->addProperty("name", group.getName());
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
        pDevice->setName(_request.getParameter("newName"));
        return success();
      } else {
        return failure("missing parameter 'name'");
      }
    } else if(_request.getMethod() == "addTag") {
      std::string tagName = _request.getParameter("tag");
      if(tagName.empty()) {
        return failure("missing parameter 'tag'");
      }
      pDevice->addTag(tagName);
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
    } else if(_request.getMethod() == "enable") {
      pDevice->enable();
      return success();
    } else if(_request.getMethod() == "disable") {
      pDevice->disable();
      return success();
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
    } else if(_request.getMethod() == "setRawValue") {
      int value = strToIntDef(_request.getParameter("value"), -1);
      if(value == -1) {
        return failure("Invalid or missing parameter 'value'");
      }
      int parameterID = strToIntDef(_request.getParameter("parameterID"), -1);
      if(parameterID == -1) {
        return failure("Invalid or missing parameter 'parameterID'");
      }
      int size = strToIntDef(_request.getParameter("size"), -1);
      if(size == -1) {
        return failure("Invalid or missing parameter 'size'");
      }

      pDevice->setRawValue(value, parameterID, size);
      return success();
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // jsonHandleRequest

} // namespace dss
