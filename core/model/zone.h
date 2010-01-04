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

#ifndef ZONE_H
#define ZONE_H

#include <vector>

#include <boost/noncopyable.hpp>

#include "modeltypes.h"
#include "devicecontainer.h"
#include "nonaddressablemodelitem.h"
#include "physicalmodelitem.h"

namespace dss {
  class Modulator;
  class Group;
  class UserGroup;
  class Set;
  class DeviceReference;
 
    /** Represents a Zone.
   * A Zone houses multiple devices. It can span over multiple modulators.
   */
  class Zone : public DeviceContainer,
               public NonAddressableModelItem,
               public PhysicalModelItem,
               public boost::noncopyable {
  private:
    int m_ZoneID;
    DeviceVector m_Devices;
    std::vector<const Modulator*> m_Modulators;
    std::vector<Group*> m_Groups;
    int m_FirstZoneOnModulator;
  public:
    Zone(const int _id)
    : m_ZoneID(_id),
          m_FirstZoneOnModulator(-1)
    {}
    virtual ~Zone();
    virtual Set getDevices() const;

    /** Adds the Zone to a modulator. */
    void addToModulator(const Modulator& _modulator);
    /** Removes the Zone from a modulator. */
    void removeFromModulator(const Modulator& _modulator);

    /** Adds a device to the zone.
     * This will permanently add the device to the zone.
     */
    void addDevice(DeviceReference& _device);

    /** Removes a device from the zone.
     * This will permanently remove the device from the zone.
     */
    void removeDevice(const DeviceReference& _device);

    /** Returns the group with the name \a _name */
    Group* getGroup(const std::string& _name) const;
    /** Returns the group with the id \a _id */
    Group* getGroup(const int _id) const;

    /** Adds a group to the zone */
    void addGroup(Group* _group);
    /** Removes a group from the zone */
    void removeGroup(UserGroup* _group);

    /** Returns the zones id */
    int getID() const;
    /** Sets the zones id */
    void setZoneID(const int _value);

    /** Returns the ID of the modulator the zone is the first
      * zone on.
      * @return The modulator id, or -1
      */
    int getFirstZoneOnModulator() { return m_FirstZoneOnModulator; }
    void setFirstZoneOnModulator(const int _value) { m_FirstZoneOnModulator = _value; }

    /** Returns a list of the modulators the zone is registered with. */
    std::vector<int> getModulators() const;
    bool registeredOnModulator(const Modulator& _modulator) const;

    virtual void nextScene();
    virtual void previousScene();

    virtual unsigned long getPowerConsumption();
    /** Returns a vector of groups present on the zone. */
    std::vector<Group*> getGroups() { return m_Groups; }
  protected:
    virtual std::vector<AddressableModelItem*> splitIntoAddressableItems();
  }; // Zone


} // namespace dss

#endif // ZONE_H
