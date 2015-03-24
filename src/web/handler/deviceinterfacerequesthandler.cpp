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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "deviceinterfacerequesthandler.h"

#include <limits.h>
#include <stdexcept>

#include "src/security/user.h"
#include "src/session.h"
#include "src/model/deviceinterface.h"
#include "src/model/modelconst.h"
#include "src/model/scenehelper.h"
#include "src/comm-channel.h"
#include "src/util.h"
#include "src/ds485types.h"

namespace dss {


  //=========================================== DeviceInterfaceRequestHandler

  // compatibility helper which will be removed in R1.8
  std::string DeviceInterfaceRequestHandler::getCategory(const RestfulRequest& _request) {
    if (_request.hasParameter("category")) {
      return _request.getParameter("category");
    } else {
      return "manual";
    }
  }

  std::string DeviceInterfaceRequestHandler::handleDeviceInterfaceRequest(const RestfulRequest& _request, boost::shared_ptr<IDeviceInterface> _interface, boost::shared_ptr<Session> _session) {
    assert(_interface != NULL);
    assert(isDeviceInterfaceCall(_request));
    std::string categoryStr = getCategory(_request);
    std::string sessionToken = "";
    if (_session->getUser() != NULL) {
      sessionToken = _session->getUser()->getToken();
    }
    if(_request.getMethod() == "turnOn") {
      _interface->turnOn(coJSON, SceneAccess::stringToCategory(categoryStr));
      return JSONWriter::success();
    } else if(_request.getMethod() == "turnOff") {
      _interface->turnOff(coJSON, SceneAccess::stringToCategory(categoryStr));
      return JSONWriter::success();
    } else if(_request.getMethod() == "increaseValue") {
      _interface->increaseValue(coJSON, SceneAccess::stringToCategory(categoryStr));
      return JSONWriter::success();
    } else if(_request.getMethod() == "decreaseValue") {
      _interface->decreaseValue(coJSON, SceneAccess::stringToCategory(categoryStr));
      return JSONWriter::success();
    } else if(_request.getMethod() == "setValue") {
      std::string valueStr = _request.getParameter("value");
      int value = strToIntDef(valueStr, -1);
      if((value  < 0) || (value > UCHAR_MAX)) {
        return JSONWriter::failure("Invalid or missing parameter value: '" + valueStr + "'");
      } else {
        _interface->setValue(coJSON, SceneAccess::stringToCategory(categoryStr), value, sessionToken);
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "callScene") {
      std::string sceneStr = _request.getParameter("sceneNumber");
      int sceneID = strToIntDef(sceneStr, -1);
      bool force = _request.getParameter("force") == "true";
      SceneAccessCategory category = SceneAccess::stringToCategory(categoryStr);
      if(sceneID != -1) {
        if(SceneHelper::isInRange(sceneID, 0)) {
          _interface->callScene(coJSON, category, sceneID, sessionToken, force);
        } else {
          return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
        }
      } else {
        return JSONWriter::failure("Invalid sceneNumber: '" + sceneStr + "'");
      }
      return JSONWriter::success();
    } else if (_request.getMethod() == "callSceneMin") {
      std::string sceneStr = _request.getParameter("sceneNumber");
      int sceneID = strToIntDef(sceneStr, -1);
      SceneAccessCategory category = SceneAccess::stringToCategory(categoryStr);
      if(sceneID != -1) {
        if(SceneHelper::isInRange(sceneID, 0)) {
          _interface->callSceneMin(coJSON, category, sceneID, sessionToken);
        } else {
          return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
        }
      } else {
        return JSONWriter::failure("Invalid sceneNumber: '" + sceneStr + "'");
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "saveScene") {
      std::string sceneStr = _request.getParameter("sceneNumber");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)sceneID)) {
          return JSONWriter::failure("Device settings are being updated for selected activity, please try again later");
        }
        if(SceneHelper::isInRange(sceneID, 0)) {
          _interface->saveScene(coJSON, sceneID, sessionToken);
        } else {
          return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
        }
      } else {
        return JSONWriter::failure("Invalid sceneNumber: '" + sceneStr + "'");
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "undoScene") {
      std::string sceneStr = _request.getParameter("sceneNumber");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID == -1) {
        _interface->undoSceneLast(coJSON, SceneAccess::stringToCategory(categoryStr), sessionToken);
      } else {
        _interface->undoScene(coJSON, SceneAccess::stringToCategory(categoryStr), sceneID, sessionToken);
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "getConsumption") {
      JSONWriter json;
      json.add("consumption", (long long int)_interface->getPowerConsumption());
      return json.successJSON();
    } else if(_request.getMethod() == "blink") {
      _interface->blink(coJSON, SceneAccess::stringToCategory(categoryStr), sessionToken);
      return JSONWriter::success();
    } else if(_request.getMethod() == "increaseOutputChannelValue") {
      if (_request.getParameter("channel").empty()) {
        return JSONWriter::failure("Missing parameter 'channel'");
      }
      std::pair<uint8_t, uint8_t> channel = getOutputChannelIdAndSize(_request.getParameter("channel"));
      _interface->increaseOutputChannelValue(coJSON, SceneAccess::stringToCategory(categoryStr), channel.first, sessionToken);
      return JSONWriter::success();
    } else if(_request.getMethod() == "decreaseOutputChannelValue") {
      if (_request.getParameter("channel").empty()) {
        return JSONWriter::failure("Missing parameter 'channel'");
      }
      std::pair<uint8_t, uint8_t> channel = getOutputChannelIdAndSize(_request.getParameter("channel"));
      _interface->decreaseOutputChannelValue(coJSON, SceneAccess::stringToCategory(categoryStr), channel.first, sessionToken);
      return JSONWriter::success();
    } else if(_request.getMethod() == "stopOutputChannelValue") {
      if (_request.getParameter("channel").empty()) {
        return JSONWriter::failure("Missing parameter 'channel'");
      }
      std::pair<uint8_t, uint8_t> channel = getOutputChannelIdAndSize(_request.getParameter("channel"));
      _interface->stopOutputChannelValue(coJSON, SceneAccess::stringToCategory(categoryStr), channel.first, sessionToken);
      return JSONWriter::success();
    } else if(_request.getMethod() == "pushSensorValue") {
      dsuid_t sourceID;
      std::string deviceIDStr = _request.getParameter("sourceDSID");
      std::string dsuidStr = _request.getParameter("sourceDSUID");
      if (deviceIDStr.empty() && dsuidStr.empty()) {
        SetNullDsuid(sourceID);
      } else {
        sourceID = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);
      }
      int sensorType = strToIntDef(_request.getParameter("sensorType"), -1);
      std::string sensorValueString = _request.getParameter("sensorValue");
      if(sensorType == -1 || sensorValueString.length() == 0) {
        return JSONWriter::failure("Need valid parameter 'sensorType' and 'sensorValue'");
      }
      double sensorValue = ::strtod(sensorValueString.c_str(), 0);

      _interface->pushSensor(coJSON, SceneAccess::stringToCategory(categoryStr),
          sourceID, sensorType, sensorValue, sessionToken);
      return JSONWriter::success();
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
        || _request.getMethod() == "callSceneMin"
        || _request.getMethod() == "saveScene"
        || _request.getMethod() == "undoScene"
        || _request.getMethod() == "getConsumption"
        || _request.getMethod() == "blink"
        || _request.getMethod() == "increaseOutputChannelValue"
        || _request.getMethod() == "decreaseOutputChannelValue"
        || _request.getMethod() == "stopOutputChannelValue"
        || _request.getMethod() == "pushSensorValue";
  } // isDeviceInterfaceCall

} // namespace dss
