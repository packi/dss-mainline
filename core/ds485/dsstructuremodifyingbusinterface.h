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

#ifndef DSSTRUCTUREMODIFYINGBUSINTERFACE_H_
#define DSSTRUCTUREMODIFYINGBUSINTERFACE_H_

#include "core/businterface.h"

#include <digitalSTROM/dsm-api-v2/dsm-api.h>

namespace dss {

  class DSStructureModifyingBusInterface : public StructureModifyingBusInterface {
  public:
    DSStructureModifyingBusInterface(DsmApiHandle_t _dsmApiHandle)
    : m_DSMApiHandle(_dsmApiHandle)
    { }

    virtual void setZoneID(const dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID);
    virtual void createZone(const dsid_t& _dsMeterID, const int _zoneID);
    virtual void removeZone(const dsid_t& _dsMeterID, const int _zoneID);

    virtual void addToGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeFromGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID);

    virtual void removeInactiveDevices(const dsid_t& _dsMeterID);
  private:
    DsmApiHandle_t m_DSMApiHandle;
  }; // DSStructureModifyingBusInterface

} // namespace dss

#endif
