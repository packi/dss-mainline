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

#ifndef ZONEREQUESTHANDLER_H_
#define ZONEREQUESTHANDLER_H_

#include "deviceinterfacerequesthandler.h"

namespace dss {

  class Apartment;
  class StructureModifyingBusInterface;
  class StructureQueryBusInterface;

  class ZoneRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    ZoneRequestHandler(Apartment& _apartment,
                       StructureModifyingBusInterface* _pBusInterface,
                       StructureQueryBusInterface* _pQueryBusInterface);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection);
  private:
    Apartment& m_Apartment;
    StructureModifyingBusInterface* m_pStructureBusInterface;
    StructureQueryBusInterface* m_pStructureQueryBusInterface;

    std::string getLastCalledScene(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string setName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string sceneSetName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string sceneGetName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getReachableScenes(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getTemperatureControlStatus(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getTemperatureControlConfig(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string setTemperatureControlConfig(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getTemperatureControlValues(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string setTemperatureControlValues(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string setTemperatureControlState(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getTemperatureControlInternals(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string setSensorSource(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string clearSensorSource(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getAssignedSensors(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);
    std::string getSensorValues(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request);

  }; // ZoneRequestHandler

}

#endif /* ZONEREQUESTHANDLER_H_ */
