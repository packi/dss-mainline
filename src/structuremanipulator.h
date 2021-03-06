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

#include "model/behavior.h"
#include "src/businterface.h"
#include "src/model/devicereference.h"

namespace dss {

  class Apartment;
  class DSMeter;
  class Device;
  class Zone;
  class Group;
  class Cluster;
  class Set;

  class StructureManipulator {
  private:
    Apartment& m_Apartment;
    StructureModifyingBusInterface& m_Interface;
    class StructureQueryBusInterface& m_QueryInterface;
    void checkSensorsOnDeviceRemoval(Zone &_zone, Device &_device);
    void wipeZoneOnMeter(dsuid_t _meterDSUID, int _zoneID);
    void setZoneHeatingConfigImpl(const boost::shared_ptr<const DSMeter>& meter, const boost::shared_ptr<Zone>& zone);

  public:
    StructureManipulator(StructureModifyingBusInterface& _interface,
                         StructureQueryBusInterface& _queryInterface , Apartment& _apartment)
    : m_Apartment(_apartment), m_Interface(_interface), m_QueryInterface(_queryInterface)
    { } // ctor

    void createZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    void addDeviceToZone(boost::shared_ptr<Device> _device, boost::shared_ptr<Zone> _zone);
    void removeZoneOnDSMeter(boost::shared_ptr<Zone> _zone, boost::shared_ptr<DSMeter> _dsMeter);
    void removeZoneOnDSMeters(boost::shared_ptr<Zone>);
    void removeDeviceFromDSMeter(boost::shared_ptr<Device> _device);
    /**
     * @ret list of devices (slaves included) that have been deleted
     */
    std::vector<boost::shared_ptr<Device> > removeDevice(boost::shared_ptr<Device> _pDevice);
    void deviceSetName(boost::shared_ptr<Device> _pDevice, const std::string& _name);
    void meterSetName(boost::shared_ptr<DSMeter> _pMeter, const std::string& _name);
    int persistSet(Set& _set, const std::string& _originalSet);
    int persistSet(Set& _set, const std::string& _originalSet, int _groupNumber);
    void unpersistSet(std::string _setDescription);

    void createGroup(boost::shared_ptr<Zone> _zone, int _groupNumber, ApplicationType applicationType, const int applicationConfiguration, const std::string& _name);
    void removeGroup(boost::shared_ptr<Zone> _zone, int _groupNumber);
    void groupSetName(boost::shared_ptr<Group> _group, const std::string& _name);
    void groupSetApplication(boost::shared_ptr<Group> _group, ApplicationType applicationType, const int applicationConfiguration);

    void sceneSetName(boost::shared_ptr<Group> _group, int _sceneNumber, const std::string& _name);

    void clusterSetName(boost::shared_ptr<Cluster> _cluster, const std::string& _name);
    void clusterSetApplication(boost::shared_ptr<Cluster> _cluster, ApplicationType applicationType, const int applicationConfiguration);
    void clusterSetConfigurationLock(boost::shared_ptr<Cluster> _cluster, bool _locked);

    void deviceAddToGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group);
    void deviceRemoveFromGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group, bool _adaptInputs = true);
    void deviceRemoveFromGroups(boost::shared_ptr<Device> device);

    void setZoneHeatingConfig(boost::shared_ptr<Zone> zone, const ZoneHeatingConfigSpec_t& spec);
    void updateZoneTemperatureControlMeters();
    void setZoneSensor(Zone &_zone, SensorType _sensorType, boost::shared_ptr<Device> _dev);
    void resetZoneSensor(Zone &_zone, SensorType _sensorType);
    void autoAssignZoneSensors(Zone &_zone);
    void synchronizeZoneSensorAssignment(Zone &zone);
    /**
     * @ret device moved
     */
    bool setJokerGroup(boost::shared_ptr<Device> device, int groupId);

    void setProperty(boost::shared_ptr<DSMeter> _dsMeter, const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties);
    vdcapi::Message getProperty(boost::shared_ptr<DSMeter> _dsMeter, const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query);
}; // StructureManipulator


} // namespace dss

#endif /* STRUCTUREMANIPULATOR_H_ */
