/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef STRUCTUREMANIPULATOR_H_
#define STRUCTUREMANIPULATOR_H_

#include <boost/shared_ptr.hpp>
#include "src/businterface.h"

namespace dss {

  class Apartment;
  class StructureModifyingBusInterface;
  class StructureQueryBusInterface;
  class DSMeter;
  class Device;
  class Zone;
  class Group;
  class Set;

  class StructureManipulator {
  private:
    Apartment& m_Apartment;
    StructureModifyingBusInterface& m_Interface;
    class StructureQueryBusInterface& m_QueryInterface;
  public:
    StructureManipulator(StructureModifyingBusInterface& _interface,
                         StructureQueryBusInterface& _queryInterface , Apartment& _apartment)
    : m_Apartment(_apartment), m_Interface(_interface), m_QueryInterface(_queryInterface)
    { } // ctor

    void createZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    void addDeviceToZone(boost::shared_ptr<Device> _device, boost::shared_ptr<Zone> _zone);
    void removeZoneOnDSMeter(boost::shared_ptr<Zone> _zone, boost::shared_ptr<DSMeter> _dsMeter);
    void removeDeviceFromDSMeter(boost::shared_ptr<Device> _device);

    void sceneSetName(boost::shared_ptr<Group> _group, int _sceneNumber, const std::string& _name);
    void deviceSetName(boost::shared_ptr<Device> _pDevice, const std::string& _name);
    void meterSetName(boost::shared_ptr<DSMeter> _pMeter, const std::string& _name);
    int persistSet(Set& _set, const std::string& _originalSet);
    int persistSet(Set& _set, const std::string& _originalSet, int _groupNumber);
    void unpersistSet(std::string _setDescription);

    void createGroup(boost::shared_ptr<Zone> _zone, int _groupNumber);
    void deviceAddToGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group);
    void deviceRemoveFromGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group);

    void sensorPush(boost::shared_ptr<Zone> _zone, dss_dsid_t _sourceID, int _sensorType, int _sensorValue);
}; // StructureManipulator


} // namespace dss

#endif /* STRUCTUREMANIPULATOR_H_ */
