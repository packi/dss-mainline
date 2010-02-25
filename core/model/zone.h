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
#include <boost/shared_ptr.hpp>

#include "modeltypes.h"
#include "devicecontainer.h"
#include "nonaddressablemodelitem.h"
#include "physicalmodelitem.h"

namespace dss {
  class Apartment;
  class DSMeter;
  class Group;
  class UserGroup;
  class Set;
  class DeviceReference;
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

    /** Represents a Zone.
   * A Zone houses multiple devices. It can span over multiple dsMeters.
   */
  class Zone : public DeviceContainer,
               public NonAddressableModelItem,
               public PhysicalModelItem,
               public boost::noncopyable {
  private:
    int m_ZoneID;
    DeviceVector m_Devices;
    std::vector<const DSMeter*> m_DSMeters;
    std::vector<Group*> m_Groups;
    int m_FirstZoneOnDSMeter;
    Apartment* m_pApartment;
    PropertyNodePtr m_pPropertyNode;
  public:
    Zone(const int _id, Apartment* _pApartment)
    : m_ZoneID(_id),
      m_FirstZoneOnDSMeter(-1),
      m_pApartment(_pApartment)
    {}
    virtual ~Zone();
    virtual Set getDevices() const;

    /** Adds the Zone to a dsMeter. */
    void addToDSMeter(DSMeter& _dsMeter);
    /** Removes the Zone from a dsMeter. */
    void removeFromDSMeter(DSMeter& _dsMeter);

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

    /** Returns the ID of the dsMeter the zone is the first
      * zone on.
      * @return The dsMeter id, or -1
      */
    int getFirstZoneOnDSMeter() { return m_FirstZoneOnDSMeter; }
    void setFirstZoneOnDSMeter(const int _value) { m_FirstZoneOnDSMeter = _value; }

    bool registeredOnDSMeter(const DSMeter& _dsMeter) const;
    bool isRegisteredOnAnyMeter() const;

    virtual void nextScene();
    virtual void previousScene();

    void publishToPropertyTree();

    virtual unsigned long getPowerConsumption();
    /** Returns a vector of groups present on the zone. */
    std::vector<Group*> getGroups() { return m_Groups; }
  protected:
    virtual std::vector<AddressableModelItem*> splitIntoAddressableItems();
  }; // Zone


} // namespace dss

#endif // ZONE_H
