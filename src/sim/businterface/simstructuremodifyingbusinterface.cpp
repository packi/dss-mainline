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

#include "simstructuremodifyingbusinterface.h"

#include "src/base.h"
#include "src/sim/dssim.h"
#include "src/sim/dsmetersim.h"


namespace dss {

  //================================================== SimStructureModifyingBusInterface

  void SimStructureModifyingBusInterface::setZoneID(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      pMeter->moveDevice(_deviceID, _zoneID);
    }
  } // setZoneID

  void SimStructureModifyingBusInterface::createZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      pMeter->addZone(_zoneID);
    }
  } // createZone

  void SimStructureModifyingBusInterface::removeZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      pMeter->removeZone(_zoneID);
    }
  } // removeZone

  void SimStructureModifyingBusInterface::addToGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      DSIDInterface& dev = pMeter->lookupDevice(_deviceID);
      pMeter->addDeviceToGroup(&dev, _groupID);
    }
  } // addToGroup

  void SimStructureModifyingBusInterface::removeFromGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      DSIDInterface& dev = pMeter->lookupDevice(_deviceID);
      pMeter->removeDeviceFromGroup(&dev, _groupID);
    }
  } // removeFromGroup

  void SimStructureModifyingBusInterface::removeDeviceFromDSMeter(const dss_dsid_t& _dsMeterID, const int _deviceID) {
    // nop
  } // removeDevice

  void SimStructureModifyingBusInterface::removeDeviceFromDSMeters(const dss_dsid_t& _deviceDSID) {
    // nop
  } // removeDeviceFromDSMeters

  void SimStructureModifyingBusInterface::sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
    // nop
  } // sceneSetName

  void SimStructureModifyingBusInterface::deviceSetName(dss_dsid_t _meterDSID, devid_t _deviceID, const std::string& _name) {
  } // deviceSetName

  void SimStructureModifyingBusInterface::meterSetName(dss_dsid_t _meterDSID, const std::string& _name) {
  } // meterSetName

  void SimStructureModifyingBusInterface::createGroup(uint16_t _zoneID, uint8_t _groupID) {
    int numMeters = m_pSimulation->getDSMeterCount();
    for(int iMeter = 0; iMeter < numMeters; iMeter++) {
      boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(iMeter);
      if(contains<int>(pMeter->getZones(), _zoneID)) {
        pMeter->addGroup(_zoneID, _groupID);
      }
    }
  } // createGroup

  void SimStructureModifyingBusInterface::removeGroup(uint16_t _zoneID, uint8_t _groupID) {
    int numMeters = m_pSimulation->getDSMeterCount();
    for(int iMeter = 0; iMeter < numMeters; iMeter++) {
      boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(iMeter);
      if(contains<int>(pMeter->getZones(), _zoneID)) {
        if(contains<int>(pMeter->getGroupsOfZone(_zoneID), _groupID)) {
          pMeter->removeGroup(_zoneID, _groupID);
        }
      }
    }
  } // removeGroup

  void SimStructureModifyingBusInterface::sensorPush(uint16_t _zoneID, dss_dsid_t _sourceID, uint8_t _sensorType, uint16_t _sensorValue) {
    int numMeters = m_pSimulation->getDSMeterCount();
    for(int iMeter = 0; iMeter < numMeters; iMeter++) {
      boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(iMeter);
      if(contains<int>(pMeter->getZones(), _zoneID)) {
        pMeter->sensorPush(_zoneID, _sourceID.lower, _sensorType, _sensorValue);
      }
    }
  } // sensorPush
  
  void SimStructureModifyingBusInterface::setButtonSetsLocalPriority(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) {
  } // setButtonSetsLocalPriority

} // namespace dss
