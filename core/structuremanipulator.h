/*
 * structuremodifier.h
 *
 *  Created on: Aug 26, 2009
 *      Author: patrick
 */

#ifndef STRUCTUREMANIPULATOR_H_
#define STRUCTUREMANIPULATOR_H_

namespace dss {

  class Apartment;
  class DS485Interface;
  class Modulator;
  class Device;
  class Zone;

  class StructureManipulator {
  private:
    Apartment& m_Apartment;
    DS485Interface& m_Interface;
  public:
    StructureManipulator(DS485Interface& _interface, Apartment& _apartment)
    : m_Apartment(_apartment), m_Interface(_interface)
    { } // ctor

    void createZone(Modulator& _modulator, Zone& _zone);
    void addDeviceToZone(Device& _device, Zone& _zone);
    void removeZoneOnModulator(Zone& _zone, Modulator& _modulator);
  }; // StructureManipulator


} // namespace dss

#endif /* STRUCTUREMANIPULATOR_H_ */
