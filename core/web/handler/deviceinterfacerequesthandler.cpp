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

#include "deviceinterfacerequesthandler.h"

#include <limits.h>
#include <stdexcept>

#include "core/web/json.h"
#include "core/model/deviceinterface.h"
#include "core/model/modelconst.h"
#include "core/model/scenehelper.h"

namespace dss {


  //=========================================== DeviceInterfaceRequestHandler

  boost::shared_ptr<JSONObject> DeviceInterfaceRequestHandler::handleDeviceInterfaceRequest(const RestfulRequest& _request, boost::shared_ptr<IDeviceInterface> _interface) {
    assert(_interface != NULL);
    assert(isDeviceInterfaceCall(_request));
    if(_request.getMethod() == "turnOn") {
      _interface->turnOn();
      return success();
    } else if(_request.getMethod() == "turnOff") {
      _interface->turnOff();
      return success();
    } else if(_request.getMethod() == "increaseValue") {
      _interface->increaseValue();
      return success();
    } else if(_request.getMethod() == "decreaseValue") {
      _interface->decreaseValue();
      return success();
    } else if(_request.getMethod() == "setValue") {
      std::string valueStr = _request.getParameter("value");
      int value = strToIntDef(valueStr, -1);
      if((value  < 0) || (value > UCHAR_MAX)) {
        return failure("Invalid or missing parameter value: '" + valueStr + "'");
      } else {
        _interface->setValue(value);
      }
      return success();
    } else if(_request.getMethod() == "callScene") {
      std::string sceneStr = _request.getParameter("sceneNumber");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        if(SceneHelper::isInRange(sceneID, 0)) {
          _interface->callScene(sceneID);
        } else {
          return failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
        }
      } else {
        return failure("Invalid sceneNumber: '" + sceneStr + "'");
      }
      return success();
    } else if(_request.getMethod() == "saveScene") {
      std::string sceneStr = _request.getParameter("sceneNumber");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        if(SceneHelper::isInRange(sceneID, 0)) {
          _interface->saveScene(sceneID);
        } else {
          return failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
        }
      } else {
        return failure("Invalid sceneNumber: '" + sceneStr + "'");
      }
      return success();
    } else if(_request.getMethod() == "undoScene") {
      _interface->undoScene();
      return success();
    } else if(_request.getMethod() == "getConsumption") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("consumption", _interface->getPowerConsumption());
      return success(resultObj);
    } else if(_request.getMethod() == "blink") {
      _interface->blink();
      return success();
    }
    throw std::runtime_error("Unknown function");
  } // handleRequest

  bool DeviceInterfaceRequestHandler::isDeviceInterfaceCall(const RestfulRequest& _request) {
    return _request.getMethod() == "turnOn"
        || _request.getMethod() == "turnOff"
        || _request.getMethod() == "increaseValue"
        || _request.getMethod() == "decreaseValue"
        || _request.getMethod() == "setValue"
        || _request.getMethod() == "callScene"
        || _request.getMethod() == "saveScene"
        || _request.getMethod() == "undoScene"
        || _request.getMethod() == "getConsumption"
        || _request.getMethod() == "blink";
  } // isDeviceInterfaceCall

} // namespace dss
