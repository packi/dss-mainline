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

#include "src/base.h"
#include "src/comm-channel.h"
#include "src/ds485types.h"
#include "src/model/deviceinterface.h"
#include "src/model/modelconst.h"
#include "src/model/scenehelper.h"
#include "src/security/user.h"
#include "src/session.h"
#include "src/util.h"

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

std::string DeviceInterfaceRequestHandler::turnOn(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  interface->turnOn(coJSON, sceneCategory);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::turnOff(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  interface->turnOff(coJSON, sceneCategory);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::increaseValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  interface->increaseValue(coJSON, sceneCategory);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::decreaseValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  interface->decreaseValue(coJSON, sceneCategory);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::setValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  std::string valueStr = request.getParameter("value");
  int value = strToIntDef(valueStr, -1);
  if ((value < 0) || (value > UCHAR_MAX)) {
    return JSONWriter::failure("Invalid or missing parameter value: '" + valueStr + "'");
  } else {
    interface->setValue(coJSON, sceneCategory, value, sessionToken);
  }
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::callScene(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  std::string sceneStr = request.getParameter("sceneNumber");
  int sceneID = strToIntDef(sceneStr, -1);
  bool force = request.getParameter("force") == "true";
  if (sceneID != -1) {
    if (SceneHelper::isInRange(sceneID, 0)) {
      interface->callScene(coJSON, sceneCategory, sceneID, sessionToken, force);
    } else {
      return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
    }
  } else {
    return JSONWriter::failure("Invalid sceneNumber: '" + sceneStr + "'");
  }
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::callSceneMin(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  std::string sceneStr = request.getParameter("sceneNumber");
  int sceneID = strToIntDef(sceneStr, -1);
  if (sceneID != -1) {
    if (SceneHelper::isInRange(sceneID, 0)) {
      interface->callSceneMin(coJSON, sceneCategory, sceneID, sessionToken);
    } else {
      return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
    }
  } else {
    return JSONWriter::failure("Invalid sceneNumber: '" + sceneStr + "'");
  }
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::saveScene(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  std::string sceneStr = request.getParameter("sceneNumber");
  int sceneID = strToIntDef(sceneStr, -1);
  if (sceneID != -1) {
    if (DSS::hasInstance() && CommChannel::getInstance()->isSceneLocked((uint32_t)sceneID)) {
      return JSONWriter::failure("Device settings are being updated for selected activity, please try again later");
    }
    if (SceneHelper::isInRange(sceneID, 0)) {
      interface->saveScene(coJSON, sceneID, sessionToken);
    } else {
      return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + sceneStr + "')");
    }
  } else {
    return JSONWriter::failure("Invalid sceneNumber: '" + sceneStr + "'");
  }
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::undoScene(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  std::string sceneStr = request.getParameter("sceneNumber");
  int sceneID = strToIntDef(sceneStr, -1);
  if (sceneID == -1) {
    interface->undoSceneLast(coJSON, sceneCategory, sessionToken);
  } else {
    interface->undoScene(coJSON, sceneCategory, sceneID, sessionToken);
  }
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::getConsumption(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  JSONWriter json;
  json.add("consumption", static_cast<unsigned long long>(interface->getPowerConsumption()));
  return json.successJSON();
}
std::string DeviceInterfaceRequestHandler::blink(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  interface->blink(coJSON, sceneCategory, sessionToken);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::increaseOutputChannelValue(const RestfulRequest& request,
    DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  if (request.getParameter("channel").empty()) {
    return JSONWriter::failure("Missing parameter 'channel'");
  }
  std::pair<uint8_t, uint8_t> channel = getOutputChannelIdAndSize(request.getParameter("channel"));
  interface->increaseOutputChannelValue(coJSON, sceneCategory, channel.first, sessionToken);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::decreaseOutputChannelValue(const RestfulRequest& request,
    DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  if (request.getParameter("channel").empty()) {
    return JSONWriter::failure("Missing parameter 'channel'");
  }
  std::pair<uint8_t, uint8_t> channel = getOutputChannelIdAndSize(request.getParameter("channel"));
  interface->decreaseOutputChannelValue(coJSON, sceneCategory, channel.first, sessionToken);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::stopOutputChannelValue(const RestfulRequest& request,
    DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  if (request.getParameter("channel").empty()) {
    return JSONWriter::failure("Missing parameter 'channel'");
  }
  std::pair<uint8_t, uint8_t> channel = getOutputChannelIdAndSize(request.getParameter("channel"));
  interface->stopOutputChannelValue(coJSON, sceneCategory, channel.first, sessionToken);
  return JSONWriter::success();
}
std::string DeviceInterfaceRequestHandler::pushSensorValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
    SceneAccessCategory sceneCategory, const std::string& sessionToken) {
  dsuid_t sourceID;
  std::string deviceIDStr = request.getParameter("sourceDSID");
  std::string dsuidStr = request.getParameter("sourceDSUID");
  if (deviceIDStr.empty() && dsuidStr.empty()) {
    sourceID = DSUID_NULL;
  } else {
    sourceID = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);
  }
  int sensorType = strToIntDef(request.getParameter("sensorType"), -1);
  std::string sensorValueString = request.getParameter("sensorValue");
  if (sensorType == -1 || sensorValueString.length() == 0) {
    return JSONWriter::failure("Need valid parameter 'sensorType' and 'sensorValue'");
  }
  double sensorValue = ::strtod(sensorValueString.c_str(), 0);
  SensorType _sensorType = static_cast<SensorType>(sensorType);

  interface->pushSensor(coJSON, sceneCategory, sourceID, _sensorType, sensorValue, sessionToken);
  return JSONWriter::success();
}

std::string DeviceInterfaceRequestHandler::handleDeviceInterfaceRequest(
    const RestfulRequest& request, DeviceInterfacePtr interface, boost::shared_ptr<Session> session) {
  assert(interface != NULL);
  assert(isDeviceInterfaceCall(request));
  SceneAccessCategory sceneCategory = SceneAccess::stringToCategory(getCategory(request));
  std::string sessionToken = "";
  if (session->getUser() != NULL) {
    sessionToken = session->getUser()->getToken();
  }
  if (request.getMethod() == "turnOn") {
    return turnOn(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "turnOff") {
    return turnOff(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "increaseValue") {
    return increaseValue(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "decreaseValue") {
    return decreaseValue(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "setValue") {
    return setValue(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "callScene") {
    return callScene(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "callSceneMin") {
    return callSceneMin(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "saveScene") {
    return saveScene(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "undoScene") {
    return undoScene(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "getConsumption") {
    return getConsumption(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "blink") {
    return blink(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "increaseOutputChannelValue") {
    return increaseOutputChannelValue(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "decreaseOutputChannelValue") {
    return decreaseOutputChannelValue(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "stopOutputChannelValue") {
    return stopOutputChannelValue(request, interface, sceneCategory, sessionToken);
  } else if (request.getMethod() == "pushSensorValue") {
    return pushSensorValue(request, interface, sceneCategory, sessionToken);
  }
  throw std::runtime_error("Unknown function");
} // handleRequest

bool DeviceInterfaceRequestHandler::isDeviceInterfaceCall(const RestfulRequest& _request) {
  return _request.getMethod() == "turnOn" || _request.getMethod() == "turnOff" ||
         _request.getMethod() == "increaseValue" || _request.getMethod() == "decreaseValue" ||
         _request.getMethod() == "setValue" || _request.getMethod() == "callScene" ||
         _request.getMethod() == "callSceneMin" || _request.getMethod() == "saveScene" ||
         _request.getMethod() == "undoScene" || _request.getMethod() == "getConsumption" ||
         _request.getMethod() == "blink" || _request.getMethod() == "increaseOutputChannelValue" ||
         _request.getMethod() == "decreaseOutputChannelValue" || _request.getMethod() == "stopOutputChannelValue" ||
         _request.getMethod() == "pushSensorValue";
} // isDeviceInterfaceCall

} // namespace dss
