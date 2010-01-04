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

  void StructureManipulator::createZone(Modulator& _modulator, Zone& _zone) {
    AssertLocked apartmentLocked(&m_Apartment);
    if(!_modulator.isPresent()) {
      throw std::runtime_error("Need modulator to be present");
    }
    m_Interface.createZone(_modulator.getBusID(), _zone.getID());
    _zone.addToModulator(_modulator);
    _zone.setIsPresent(true);
  } // createZone

  void StructureManipulator::addDeviceToZone(Device& _device, Zone& _zone) {
    AssertLocked apartmentLocked(&m_Apartment);
    if(!_device.isPresent()) {
      throw std::runtime_error("Need device to be present");
    }
    int oldZoneID = _device.getZoneID();
    Modulator& targetModulator = m_Apartment.getModulatorByBusID(_device.getModulatorID());
    if(!_zone.registeredOnModulator(targetModulator)) {
      createZone(targetModulator, _zone);
    }
    m_Interface.setZoneID(targetModulator.getBusID(), _device.getShortAddress(), _zone.getID());
    _device.setZoneID(_zone.getID());
    DeviceReference ref(_device, &m_Apartment);
    _zone.addDevice(ref);

    // check if we can remove the zone from the modulator
    if(oldZoneID != 0) {
      Zone& oldZone = m_Apartment.getZone(oldZoneID);
      oldZone.removeDevice(ref);

      Set presentDevicesInZoneOfModulator = oldZone.getDevices().getByModulator(targetModulator).getByPresence(true);
      if(presentDevicesInZoneOfModulator.length() == 0) {
        removeZoneOnModulator(oldZone, targetModulator);
      }
    }
  } // addDeviceToZone

  void StructureManipulator::removeZoneOnModulator(Zone& _zone, Modulator& _modulator) {
    AssertLocked apartmentLocked(&m_Apartment);
    Set presentDevicesInZoneOfModulator = _zone.getDevices().getByModulator(_modulator).getByPresence(true);
    if(presentDevicesInZoneOfModulator.length() != 0) {
      throw std::runtime_error("cannot delete zone if there are still devices present");
    }
    m_Interface.removeZone(_modulator.getBusID(), _zone.getID());
    _zone.removeFromModulator(_modulator);
    if(_zone.getModulators().empty()) {
      _zone.setIsPresent(false);
    }
  } // removeZoneOnModulator

} // namespace dss
