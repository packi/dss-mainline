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

#include "core/sim/dssim.h"
#include "core/sim/dsmetersim.h"


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

  void SimStructureModifyingBusInterface::removeInactiveDevices(const dss_dsid_t& _dsMeterID) {
    // nop
  } // removeInactiveDevices

  void SimStructureModifyingBusInterface::sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
    // nop
  }

} // namespace dss
