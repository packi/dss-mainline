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
    DSMeterSpec_t result;
    result.APIVersion = 0;
    result.DSID = _dsMeterID;
    result.HardwareVersion = 0;
    result.Name = "DSMSim11";
    result.SoftwareRevisionARM = 0;
    result.SoftwareRevisionDSP = 0;
    return result;
  } // getDSMeterSpec

  std::vector<int> SimStructureQueryBusInterface::getZones(const dss_dsid_t& _dsMeterID) {
    std::vector<int> result;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      result = pMeter->getZones();
    }
    return result;
  } // getZones

  std::vector<DeviceSpec_t> SimStructureQueryBusInterface::getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    std::vector<DeviceSpec_t> result;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      std::vector<DSIDInterface*> devices = pMeter->getDevicesOfZone(_zoneID);
      foreach(DSIDInterface* pDevice, devices) {
        DeviceSpec_t spec;
        spec.FunctionID = pDevice->getFunctionID();
        spec.ProductID = pDevice->getProductID();
        spec.Version = pDevice->getProductRevision();
        spec.ShortAddress = pDevice->getShortAddress();
        spec.Locked = pDevice->isLocked();
        spec.OutputMode = 0;
        spec.DSID = pDevice->getDSID();
        spec.SerialNumber = 0;
        spec.Groups = pMeter->getGroupsOfDevice(pDevice->getShortAddress());
        spec.ActiveGroup = 0;
        spec.ButtonID = 0;
        spec.GroupMembership = 0;
        spec.SetsLocalPriority = 0;
        spec.ZoneID = pDevice->getZoneID();
        result.push_back(spec);
      }
    }
    return result;
  } // getDevicesInZone

  std::vector<DeviceSpec_t> SimStructureQueryBusInterface::getInactiveDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    return std::vector<DeviceSpec_t>();
  } // getInactiveDevicesInZone

  std::vector<int> SimStructureQueryBusInterface::getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    std::vector<int> result;
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      result = pMeter->getGroupsOfZone(_zoneID);
    }
    return result;
  } // getGroups

  int SimStructureQueryBusInterface::getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID) {
    return SceneOff; // TODO: implement if it gets implemented on the dSM11
  } // getLastCalledScene

  bool SimStructureQueryBusInterface::getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) {
    return false; // TODO: implement if it gets implemented on the dSM11
  } // getEnergyBorder

  DeviceSpec_t SimStructureQueryBusInterface::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    DeviceSpec_t result;
    if(pMeter != NULL) {
      DSIDInterface& device = pMeter->lookupDevice(_id);
      result.FunctionID = device.getFunctionID();
      result.ProductID = device.getProductID();
      result.Version = device.getProductRevision();
      result.ShortAddress = device.getShortAddress();
      result.Locked = device.isLocked();
      result.OutputMode = 0;
      result.DSID = device.getDSID();
      result.SerialNumber = 0;
      result.ActiveGroup = 0;
      result.ButtonID = 0;
      result.GroupMembership = 0;
      result.SetsLocalPriority = 0;
      result.Groups = pMeter->getGroupsOfDevice(device.getShortAddress());
      result.ZoneID = device.getZoneID();
    }
    return result;
  } // deviceGetSpec

  std::string SimStructureQueryBusInterface::getSceneName(dss_dsid_t _dsMeterID,
                                  boost::shared_ptr<Group> _group,
                                  const uint8_t _sceneNumber) {
    // TODO: implement on DSMeterSim
    return "Scene" + intToString(_sceneNumber);
  }

  DSMeterHash_t SimStructureQueryBusInterface::getDSMeterHash(const dss_dsid_t& _dsMeterID) {
    DSMeterHash_t result;
    result.Hash = 0xdeadbeef;
    result.ModificationCount = 13;
    return result;
  }

} // namespace dss
