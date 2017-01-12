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

#ifndef DEVICEINTERFACEREQUESTHANDLER_H_
#define DEVICEINTERFACEREQUESTHANDLER_H_

#include <boost/shared_ptr.hpp>

#include "src/model/deviceinterface.h"
#include "src/web/webrequests.h"

namespace dss {

class IDeviceInterface;
class RestfulRequest;

typedef boost::shared_ptr<IDeviceInterface> DeviceInterfacePtr;

class DeviceInterfaceRequestHandler : public WebServerRequestHandlerJSON {
public:
  std::string handleDeviceInterfaceRequest(
      const RestfulRequest& request, DeviceInterfacePtr interface, boost::shared_ptr<Session> session);

private:
  std::string turnOn(const RestfulRequest& request, DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory,
      const std::string& sessionToken);
  std::string turnOff(const RestfulRequest& request, DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory,
      const std::string& sessionToken);
  std::string increaseValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);
  std::string decreaseValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);
  std::string setValue(const RestfulRequest& request, DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory,
      const std::string& sessionToken);
  std::string callScene(const RestfulRequest& request, DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory,
      const std::string& sessionToken);
  std::string callSceneMin(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);
  std::string saveScene(const RestfulRequest& request, DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory,
      const std::string& sessionToken);
  std::string undoScene(const RestfulRequest& request, DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory,
      const std::string& sessionToken);
  std::string getConsumption(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);
  std::string blink(const RestfulRequest& request, DeviceInterfacePtr& interface, SceneAccessCategory sceneCategory,
      const std::string& sessionToken);
  std::string increaseOutputChannelValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);
  std::string decreaseOutputChannelValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);
  std::string stopOutputChannelValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);
  std::string pushSensorValue(const RestfulRequest& request, DeviceInterfacePtr& interface,
      SceneAccessCategory sceneCategory, const std::string& sessionToken);

protected:
  std::string getCategory(const RestfulRequest& _request);
  bool isDeviceInterfaceCall(const RestfulRequest& _request);
};

} // namespace dss

#endif /* DEVICEINTERFACEREQUESTHANDLER_H_ */
