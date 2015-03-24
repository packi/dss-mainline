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

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <vector>
#include <string>

#include "deviceinterfacerequesthandler.h"
namespace dss {

  class Device;
  class StructureModifyingBusInterface;
  class StructureQueryBusInterface;

  class DeviceRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    DeviceRequestHandler(Apartment& _apartment,
                         StructureModifyingBusInterface* _pStructureBusInterface,
                         StructureQueryBusInterface* _pStructureQueryBusInterface);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);

    // parse string of semicolon separated integers, returns a pair of
    // channelid:valuesize
    boost::shared_ptr<std::vector<std::pair<int, int> > > parseOutputChannels(std::string _channels);
    // parse string of type key=value;key=value
    // returns a tuple of channelid:valuesize:value
    boost::shared_ptr<std::vector<boost::tuple<int, int, int> > > parseOutputChannelsWithValues(std::string _values);

  private:
    boost::shared_ptr<Device> getDeviceFromRequest(const RestfulRequest& _request);
    boost::shared_ptr<Device> getDeviceByName(const RestfulRequest& _request);
    boost::shared_ptr<Device> getDeviceByDSID(const RestfulRequest& _request);
  private:
    Apartment& m_Apartment;
    StructureModifyingBusInterface* m_pStructureBusInterface;
    StructureQueryBusInterface* m_pStructureQueryBusInterface;
    static boost::recursive_mutex m_LTMODEMutex;
  }; // DeviceRequestHandler

}

#endif /* DEVICEREQUESTHANDLER_H_ */
