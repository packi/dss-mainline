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

#include "core/model/apartment.h"
#include "core/model/device.h"

#include "core/web/json.h"

namespace dss {


  //=========================================== DeviceRequestHandler

  DeviceRequestHandler::DeviceRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  boost::shared_ptr<JSONObject> DeviceRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    bool ok = true;
    std::string errorMessage;
    std::string deviceName = _request.getParameter("name");
    std::string deviceDSIDString = _request.getParameter("dsid");
    Device* pDevice = NULL;
    if(!deviceDSIDString.empty()) {
      dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
      if(!(deviceDSID == NullDSID)) {
        try {
          Device& device = m_Apartment.getDeviceByDSID(deviceDSID);
          pDevice = &device;
        } catch(std::runtime_error& e) {
          ok = false;
          errorMessage = "Could not find device with dsid '" + deviceDSIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse dsid '" + deviceDSIDString + "'";
      }
    } else if(!deviceName.empty()) {
      try {
        Device& device = m_Apartment.getDeviceByName(deviceName);
        pDevice = &device;
      } catch(std::runtime_error&  e) {
        ok = false;
        errorMessage = "Could not find device named '" + deviceName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or dsid to identify device";
    }
    if(ok) {
      if(isDeviceInterfaceCall(_request)) {
        return handleDeviceInterfaceRequest(_request, pDevice);
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
        pDevice->setName(_request.getParameter("newName"));
        return success();
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
        return resultObj;
      } else if(_request.getMethod() == "enable") {
        pDevice->enable();
        return success();
      } else if(_request.getMethod() == "disable") {
        pDevice->disable();
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
    } else {
      if(ok) {
        return success();
      } else {
        return failure(errorMessage);
      }
    }
  } // jsonHandleRequest

} // namespace dss
