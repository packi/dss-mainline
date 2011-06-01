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

#ifndef STRUCTUREREQUESTHANDLER_H_
#define STRUCTUREREQUESTHANDLER_H_

#include "core/web/webrequests.h"

namespace dss {

  class Apartment;
  class ModelMaintenance;
  class StructureModifyingBusInterface;
  class StructureQueryBusInterface;

  class StructureRequestHandler : public WebServerRequestHandlerJSON {
  public:
    StructureRequestHandler(Apartment& _apartment, ModelMaintenance& _modelMaintenance,
                            StructureModifyingBusInterface& _interface,
                            StructureQueryBusInterface& _queryInterface);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    Apartment& m_Apartment;
    ModelMaintenance& m_ModelMaintenance;
    StructureModifyingBusInterface& m_Interface;
    StructureQueryBusInterface& m_QueryInterface;

    boost::shared_ptr<JSONObject> zoneAddDevice(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> removeDevice(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> addZone(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> removeZone(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> removeInactiveDevices(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> persistSet(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> unpersistSet(const RestfulRequest& _request);
  }; // StructureRequestHandler

} // namespace dss

#endif /* STRUCTUREREQUESTHANDLER_H_ */
