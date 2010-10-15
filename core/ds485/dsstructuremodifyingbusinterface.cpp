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

#include "dsstructuremodifyingbusinterface.h"

#include "dsbusinterface.h"

#include "core/dsidhelper.h"

namespace dss {

  //================================================== DSStructureModifyingBusInterface

  void DSStructureModifyingBusInterface::setZoneID(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceProperties_set_zone(m_DSMApiHandle, meterDSID, _deviceID, _zoneID);
    DSBusInterface::checkResultCode(ret);
  } // setZoneID

  void DSStructureModifyingBusInterface::createZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = ZoneModify_add(m_DSMApiHandle, meterDSID, _zoneID);
    DSBusInterface::checkResultCode(ret);
  } // createZone

  void DSStructureModifyingBusInterface::removeZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = ZoneModify_remove(m_DSMApiHandle, meterDSID, _zoneID);
    DSBusInterface::checkResultCode(ret);
  } // removeZone

  void DSStructureModifyingBusInterface::addToGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceGroupMembershipModify_add(m_DSMApiHandle, meterDSID, _deviceID, _groupID);
    DSBusInterface::checkResultCode(ret);
  } // addToGroup

  void DSStructureModifyingBusInterface::removeFromGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceGroupMembershipModify_remove(m_DSMApiHandle, meterDSID, _deviceID, _groupID);
    DSBusInterface::checkResultCode(ret);
  } // removeFromGroup

  void DSStructureModifyingBusInterface::removeInactiveDevices(const dss_dsid_t& _dsMeterID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = CircuitRemoveInactiveDevices(m_DSMApiHandle, meterDSID);
    DSBusInterface::checkResultCode(ret);
  } // removeInactiveDevices

} // namespace dss
