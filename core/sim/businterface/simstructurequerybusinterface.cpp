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

#include "simstructurequerybusinterface.h"

#include "core/foreach.h"

#include "core/sim/dssim.h"
#include "core/sim/dsmetersim.h"

#include "core/model/device.h"

namespace dss {

  class DSSim;

  std::vector<DSMeterSpec_t> SimStructureQueryBusInterface::getDSMeters() {
    std::vector<DSMeterSpec_t> result;
    for(int iMeter = 0; iMeter < m_pSimulation->getDSMeterCount(); iMeter++) {
      result.push_back(getDSMeterSpec(m_pSimulation->getDSMeter(iMeter)->getDSID()));
    }
    return result;
  } // getDSMeters

  DSMeterSpec_t SimStructureQueryBusInterface::getDSMeterSpec(const dss_dsid_t& _dsMeterID) {
    return DSMeterSpec_t(_dsMeterID, 0, 0, 0, "dSMSim11");
  } // getDSMeterSpec

  std::vector<int> SimStructureQueryBusInterface::getZones(const dss_dsid_t& _dsMeterID) {
    std::vector<int> result;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      result = pMeter->getZones();
    }
    return result;
  } // getZones

  std::vector<int> SimStructureQueryBusInterface::getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    std::vector<int> result;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      std::vector<DSIDInterface*> devices = pMeter->getDevicesOfZone(_zoneID);
      foreach(DSIDInterface* pDevice, devices) {
        result.push_back(pDevice->getShortAddress());
      }
    }
    return result;
  } // getDevicesInZone

  std::vector<int> SimStructureQueryBusInterface::getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    std::vector<int> result;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      result = pMeter->getGroupsOfZone(_zoneID);
    }
    return result;
  } // getGroups

  std::vector<int> SimStructureQueryBusInterface::getGroupsOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) {
    std::vector<int> result;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      result = pMeter->getGroupsOfDevice(_deviceID);
    }
    return result;
  } // getGroupsOfDevice

  dss_dsid_t SimStructureQueryBusInterface::getDSIDOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) {
    dss_dsid_t result = NullDSID;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      result = pMeter->lookupDevice(_deviceID).getDSID();
    }
    return result;
  } // getDSIDOfDevice

  int SimStructureQueryBusInterface::getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID) {
    return SceneOff; // TODO: implement if it gets implemented on the dSM11
  } // getLastCalledScene

  bool SimStructureQueryBusInterface::getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) {
    return false; // TODO: implement if it gets implemented on the dSM11
  } // getEnergyBorder

  DeviceSpec_t SimStructureQueryBusInterface::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    int functionID = 0;
    int productID = 0;
    int revision = 0;
    int busAddress = 0;
    if(pMeter != NULL) {
      DSIDInterface& device = pMeter->lookupDevice(_id);
      functionID = device.getFunctionID();
      busAddress = device.getShortAddress();
    }
    return DeviceSpec_t(functionID, productID, revision, busAddress);
  } // deviceGetSpec

  bool SimStructureQueryBusInterface::isLocked(boost::shared_ptr<const Device> _device) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device->getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device->getDSID());
      if(pDevice != NULL) {
        return pDevice->isLocked();
      }
    }
    return false;
  } // isLocked

} // namespace dss
