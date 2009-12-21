/*
 * structuremanipulator.cpp
 *
 *  Created on: Aug 26, 2009
 *      Author: patrick
 */

#include "structuremanipulator.h"

#include "model.h"
#include "DS485Interface.h"

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
