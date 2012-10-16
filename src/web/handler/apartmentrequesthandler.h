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

#ifndef APARTMENTREQUESTHANDLER_H_
#define APARTMENTREQUESTHANDLER_H_

#include "deviceinterfacerequesthandler.h"

namespace dss {

  class Set;
  class Apartment;
  class ModelMaintenance;

  class ApartmentRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    ApartmentRequestHandler(Apartment& _apartment, ModelMaintenance& _modelMaintenance);
    boost::shared_ptr<JSONObject> removeMeter(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> removeInactiveMeters(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> getReachableGroups(const RestfulRequest& _request);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    Set getUnassignedDevices();
    Apartment& m_Apartment;
    ModelMaintenance& m_ModelMaintenance;
  }; // ApartmentRequestHandler

} // namespace dss

#endif /* APARTMENTREQUESTHANDLER_H_ */
