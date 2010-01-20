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

#ifndef DEVICE_H
#define DEVICE_H

#include <iosfwd>
#include <bitset>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "core/ds485types.h"
#include "core/datetools.h"
#include "addressablemodelitem.h"

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class Group;
  class DSMeter;

  /** Represents a dsID */
  class Device : public AddressableModelItem,
                 public boost::noncopyable {
  private:
    std::string m_Name;
    dsid_t m_DSID;
    devid_t m_ShortAddress;
    int m_ZoneID;
    int m_LastKnownZoneID;
    int m_DSMeterID;
    dsid_t m_LastKnownMeterDSID;
    std::bitset<63> m_GroupBitmask;
    std::vector<int> m_Groups;
    int m_FunctionID;
    int m_LastCalledScene;
    unsigned long m_Consumption;
    DateTime m_LastDiscovered;
    DateTime m_FirstSeen;

    PropertyNodePtr m_pPropertyNode;
    PropertyNodePtr m_pAliasNode;
  protected:
    /** Sends the application a note that something has changed.
     * This will cause the \c apartment.xml to be updated. */
    void dirty();
  public:
    /** Creates and initializes a device. */
    Device(const dsid_t _dsid, Apartment* _pApartment);
    virtual ~Device() {};

    void enable();
    void disable();

    /** @copydoc DeviceReference::isOn() */
    virtual bool isOn() const;

    void setRawValue(const uint16_t _value, const int _parameterNr, const int _size);
    /** Returns the value of _parameterNr.
     * @note not yet implemented */
    double getValue(const int _parameterNr = -1);

    virtual void nextScene();
    virtual void previousScene();

    /** Returns the function ID of the device.
     * A function ID specifies a certain subset of functionality that
     * a device implements.
     */
    int getFunctionID() const;
    /** Sets the functionID to \a _value */
    void setFunctionID(const int _value);
    bool hasSwitch() const;

    /** Returns the name of the device. */
    std::string getName() const;
    /** Sets the name of the device.
     * @note This will cause the apartment to store
     * \c apartment.xml
     */
    void setName(const std::string& _name);

    /** Returns the group bitmask (1 based) of the device */
    std::bitset<63>& getGroupBitmask();
    /** @copydoc getGroupBitmask() */
    const std::bitset<63>& getGroupBitmask() const;

    /** Returns wheter the device is in group \a _groupID or not. */
    bool isInGroup(const int _groupID) const;
    /** Adds the device to group \a _groupID. */
    void addToGroup(const int _groupID);
    /** Removes the device from group \a _groupID */
    void removeFromGroup(const int _groupID);

    /** Returns the group id of the \a _index'th group */
    int getGroupIdByIndex(const int _index) const;
    /** Returns \a _index'th group of the device */
    Group& getGroupByIndex(const int _index);
    /** Returns the number of groups the device is a member of */
    int getGroupsCount() const;

    /** Removes the device from all group.
     * The device will remain in the broadcastgroup though.
     */
    void resetGroups();

    /** Returns the last called scene.
     * At the moment this information is used to determine wheter a device is
     * turned on or not. */
    int getLastCalledScene() const { return m_LastCalledScene; }
    /** Sets the last called scene. */
    void setLastCalledScene(const int _value) { m_LastCalledScene = _value; }

    /** Returns the short address of the device. This is the address
     * the device got from the dSM. */
    devid_t getShortAddress() const;
    /** Sets the short-address of the device. */
    void setShortAddress(const devid_t _shortAddress);
    /** Returns the DSID of the device */
    dsid_t getDSID() const;
    /** Returns the id of the dsMeter the device is connected to */
    int getDSMeterID() const;
    const dsid_t& getLastKnownDSMeterDSID() const;
    void setLastKnownDSMeterDSID(const dsid_t& _value);
    void setDSMeter(const DSMeter& _dsMeter);

    /** Returns the zone ID the device resides in. */
    int getZoneID() const;
    /** Sets the zone ID of the device. */
    void setZoneID(const int _value);
    int getLastKnownZoneID() const;
    void setLastKnownZoneID(const int _value);
    /** Returns the apartment the device resides in. */
    Apartment& getApartment() const;

    const DateTime& getLastDiscovered() const { return m_LastDiscovered; }
    const DateTime& getFirstSeen() const { return m_FirstSeen; }
    void setFirstSeen(const DateTime& _value) { m_FirstSeen = _value; }

    virtual unsigned long getPowerConsumption();

    /** Sends a byte to the device using dsLink. If \a _writeOnly is \c true,
     * the result of the function is not defined. */
    uint8_t dsLinkSend(uint8_t _value, bool _lastByte, bool _writeOnly);
    int getSensorValue(const int _sensorID);

    const PropertyNodePtr& getPropertyNode() const { return m_pPropertyNode; }
    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }

    /** Publishes the device to the property tree.
     * @see DSS::getPropertySystem */
    void publishToPropertyTree();

    /** Returns wheter two devices are equal.
     * Devices are considered equal if their DSID are a match.*/
    bool operator==(const Device& _other) const;
  }; // Device

  std::ostream& operator<<(std::ostream& out, const Device& _dt);

} // namespace dss

#endif // DEVICE_H
