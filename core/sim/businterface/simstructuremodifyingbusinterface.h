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

#ifndef SIMSTRUCTUREMODIFYINGBUSINTERFACE_H_
#define SIMSTRUCTUREMODIFYINGBUSINTERFACE_H_

#include "core/businterface.h"

namespace dss {

  class DSSim;

  class SimStructureModifyingBusInterface : public StructureModifyingBusInterface {
  public:
    SimStructureModifyingBusInterface(boost::shared_ptr<DSSim> _pSimulation)
    : m_pSimulation(_pSimulation)
    { }

    virtual void setZoneID(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID);
    virtual void createZone(const dss_dsid_t& _dsMeterID, const int _zoneID);
    virtual void removeZone(const dss_dsid_t& _dsMeterID, const int _zoneID);
    virtual void addToGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeFromGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeInactiveDevices(const dss_dsid_t& _dsMeterID);
  private:
    boost::shared_ptr<DSSim> m_pSimulation;
  }; // StructureModifyingBusInterface

} // namespace dss

#endif