/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "structuremanipulator.h"

#include "DS485Interface.h"
#include "mutex.h"
#include "core/model/device.h"
#include "core/model/apartment.h"
#include "core/model/modulator.h"
#include "core/model/zone.h"
#include "core/model/set.h"

#include <stdexcept>

namespace dss {

  void StructureManipulator::createZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    AssertLocked apartmentLocked(&m_Apartment);
    if(!_dsMeter->isPresent()) {
      throw std::runtime_error("Need dsMeter to be present");
    }
    m_Interface.createZone(_dsMeter->getBusID(), _zone->getID());
    _zone->addToDSMeter(_dsMeter);
    _zone->setIsPresent(true);
  } // createZone

  void StructureManipulator::addDeviceToZone(Device& _device, boost::shared_ptr<Zone> _zone) {
    AssertLocked apartmentLocked(&m_Apartment);
    if(!_device.isPresent()) {
      throw std::runtime_error("Need device to be present");
    }
    int oldZoneID = _device.getZoneID();
    boost::shared_ptr<DSMeter> targetDSMeter = m_Apartment.getDSMeterByBusID(_device.getDSMeterID());
    if(!_zone->registeredOnDSMeter(targetDSMeter)) {
      createZone(targetDSMeter, _zone);
    }
    m_Interface.setZoneID(targetDSMeter->getBusID(), _device.getShortAddress(), _zone->getID());
    _device.setZoneID(_zone->getID());
    DeviceReference ref(_device, &m_Apartment);
    _zone->addDevice(ref);

    // check if we can remove the zone from the dsMeter
    if(oldZoneID != 0) {
      Logger::getInstance()->log("StructureManipulator::addDeviceToZone: Removing device from old zone " + intToString(oldZoneID), lsInfo);
      boost::shared_ptr<Zone> oldZone = m_Apartment.getZone(oldZoneID);
      oldZone->removeDevice(ref);

      Set presentDevicesInZoneOfDSMeter = oldZone->getDevices().getByDSMeter(targetDSMeter).getByPresence(true);
      if(presentDevicesInZoneOfDSMeter.length() == 0) {
        Logger::getInstance()->log("StructureManipulator::addDeviceToZone: Removing zone from meter " + targetDSMeter->getDSID().toString(), lsInfo);
        removeZoneOnDSMeter(oldZone, targetDSMeter);
      }
    } else {
      Logger::getInstance()->log("StructureManipulator::addDeviceToZone: No previous zone...", lsWarning);
    }
  } // addDeviceToZone

  void StructureManipulator::removeZoneOnDSMeter(boost::shared_ptr<Zone> _zone, boost::shared_ptr<DSMeter> _dsMeter) {
    AssertLocked apartmentLocked(&m_Apartment);
    Set presentDevicesInZoneOfDSMeter = _zone->getDevices().getByDSMeter(_dsMeter).getByPresence(true);
    if(presentDevicesInZoneOfDSMeter.length() != 0) {
      throw std::runtime_error("cannot delete zone if there are still devices present");
    }
    m_Interface.removeZone(_dsMeter->getBusID(), _zone->getID());
    _zone->removeFromDSMeter(_dsMeter);
    if(_zone->isRegisteredOnAnyMeter()) {
      _zone->setIsPresent(false);
    }
  } // removeZoneOnDSMeter

  void StructureManipulator::removeInactiveDevices(boost::shared_ptr<DSMeter> _dsMeter) {
    m_Interface.removeInactiveDevices(_dsMeter->getBusID());
  }

} // namespace dss
