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

#ifndef SIMSTRUCTUREQUERYBUSINTERFACE_H_
#define SIMSTRUCTUREQUERYBUSINTERFACE_H_


#include "core/businterface.h"

namespace dss {

  class DSSim;

  class SimStructureQueryBusInterface : public StructureQueryBusInterface {
  public:
    SimStructureQueryBusInterface(boost::shared_ptr<DSSim> _pSimulation)
    : m_pSimulation(_pSimulation)
    { }

    virtual std::vector<DSMeterSpec_t> getDSMeters();
    virtual DSMeterSpec_t getDSMeterSpec(const dss_dsid_t& _dsMeterID);
    virtual std::vector<int> getZones(const dss_dsid_t& _dsMeterID);
    virtual std::vector<int> getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID);
    virtual std::vector<int> getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID);
    virtual std::vector<int> getGroupsOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID);
    virtual dss_dsid_t getDSIDOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID);
    virtual int getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID);
    virtual bool getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper);
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID);
    virtual bool isLocked(boost::shared_ptr<const Device> _device);
    virtual bool outputHasLoad(boost::shared_ptr<const Device> _device);
    virtual std::string getSceneName(dss_dsid_t _dsMeterID,
                                     boost::shared_ptr<Group> _group,
                                     const uint8_t _sceneNumber);
  private:
    boost::shared_ptr< DSSim > m_pSimulation;
  }; // SimStructureQueryBusInterface

} // namespace dss

#endif
