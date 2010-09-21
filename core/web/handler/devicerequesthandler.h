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

#ifndef DEVICEREQUESTHANDLER_H_
#define DEVICEREQUESTHANDLER_H_

#include "deviceinterfacerequesthandler.h"

namespace dss {

  class Device;

  class DeviceRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    DeviceRequestHandler(Apartment& _apartment);
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    boost::shared_ptr<Device> getDeviceFromRequest(const RestfulRequest& _request);
    boost::shared_ptr<Device> getDeviceByName(const RestfulRequest& _request);
    boost::shared_ptr<Device> getDeviceByDSID(const RestfulRequest& _request);
  private:
    Apartment& m_Apartment;
  }; // DeviceRequestHandler

}

#endif /* DEVICEREQUESTHANDLER_H_ */
