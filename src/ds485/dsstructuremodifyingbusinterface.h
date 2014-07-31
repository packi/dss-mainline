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

#include "src/businterface.h"

#include "dsbusinterfaceobj.h"

namespace dss {

  class DSStructureModifyingBusInterface : public DSBusInterfaceObj,
                                           public StructureModifyingBusInterface {
  public:
    DSStructureModifyingBusInterface()
    : DSBusInterfaceObj()
    {
      SetBroadcastDsuid(m_BroadcastDSID);
    }

    virtual void setZoneID(const dsuid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID);
    virtual void createZone(const dsuid_t& _dsMeterID, const int _zoneID);
    virtual void removeZone(const dsuid_t& _dsMeterID, const int _zoneID);

    virtual void addToGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeFromGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeDeviceFromDSMeter(const dsuid_t& _dsMeterID, const int _deviceID);
    virtual void removeDeviceFromDSMeters(const dsuid_t& _DeviceDSID);

    virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name);
    virtual void deviceSetName(dsuid_t _meterDSID, devid_t _deviceID, const std::string& _name);
    virtual void meterSetName(dsuid_t _meterDSID, const std::string& _name);

    virtual void createGroup(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name);
    virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID);
    virtual void groupSetStandardID(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID);
    virtual void groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name);

    virtual void setButtonSetsLocalPriority(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority);
    virtual void setButtonCallsPresent(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _callsPresent);
private:
    dsuid_t m_BroadcastDSID;
  }; // DSStructureModifyingBusInterface

} // namespace dss

#endif
