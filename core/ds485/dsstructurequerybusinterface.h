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

#ifndef DSSTRUCTUREQUERYBUSINTERFACE_H_
#define DSSTRUCTUREQUERYBUSINTERFACE_H_

#include "core/businterface.h"

#include <digitalSTROM/dsm-api-v2/dsm-api.h>

namespace dss {

  class DSStructureQueryBusInterface : public StructureQueryBusInterface {
  public:
    DSStructureQueryBusInterface(DsmApiHandle_t _dsmApiHandle)
    : m_DSMApiHandle(_dsmApiHandle)
    {
      SetBroadcastId(m_BroadcastDSID);
    }

    virtual std::vector<DSMeterSpec_t> getDSMeters();
    virtual DSMeterSpec_t getDSMeterSpec(const dsid_t& _dsMeterID);
    virtual std::vector<int> getZones(const dsid_t& _dsMeterID);
    virtual int getZoneCount(const dsid_t& _dsMeterID);
    virtual std::vector<int> getDevicesInZone(const dsid_t& _dsMeterID, const int _zoneID);
    virtual int getDevicesCountInZone(const dsid_t& _dsMeterID, const int _zoneID);
    virtual int getGroupCount(const dsid_t& _dsMeterID, const int _zoneID);
    virtual std::vector<int> getGroups(const dsid_t& _dsMeterID, const int _zoneID);
    virtual std::vector<int> getGroupsOfDevice(const dsid_t& _dsMeterID, const int _deviceID);
    virtual dss_dsid_t getDSIDOfDevice(const dsid_t& _dsMeterID, const int _deviceID);
    virtual int getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID);
    virtual bool getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper);
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID);
    virtual bool isLocked(boost::shared_ptr<const Device> _device);
  private:
    DsmApiHandle_t m_DSMApiHandle;
    dsid_t m_BroadcastDSID;
  }; // DSStructureQueryBusInterface

} // namespace dss

#endif
