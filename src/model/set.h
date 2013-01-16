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

#ifndef SET_H
#define SET_H

#include <boost/shared_ptr.hpp>

#include "nonaddressablemodelitem.h"
#include "modeltypes.h"

namespace dss {

  class Device;
  class Group;
  class DSMeter;

  /** Abstract interface to select certain Devices from a set */
  class IDeviceSelector {
  public:
    /** The implementor should return true if the device should appear in the
     * resulting set. */
    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const = 0;
    virtual ~IDeviceSelector() {}
  };

  /** Abstract interface to perform an Action on each device of a set */
  class IDeviceAction {
  public:
    /** This action will be performed for every device contained in the set. */
    virtual bool perform(boost::shared_ptr<Device> _device) = 0;
    virtual ~IDeviceAction() {}
  };

  /** A set holds an arbitrary list of devices.
    * A Command sent to an instance of this class will replicate the command to all
    * contained devices.
    * Only references to devices will be stored.
   */
  class Set : public NonAddressableModelItem {
  private:
    DeviceVector m_ContainedDevices;
  public:
    /** Constructor for an empty Set.*/
    Set();
    /** Copy constructor. */
    Set(const Set& _copy);
    /** Constructor for a set containing only \a _device. */
    Set(boost::shared_ptr<const Device> _device);
    /** Constructor for a set containing only \a _reference. */
    Set(DeviceReference& _reference);
    /** Constructor for a set containing \a _devices. */
    Set(DeviceVector _devices);
    virtual ~Set() {};

    /** Performs the given action on all contained devices */
    void perform(IDeviceAction& _deviceAction);

    /** Returns a subset selected by the given selector
     * A device will be included in the resulting set if _selector.selectDevice(...) return true.
     */
    Set getSubset(const IDeviceSelector& _selector) const;
    /** Returns a subset of the devices which are member of the given group
    * Note that these groups could be spanned over multiple dsMeters.
     */
    Set getByGroup(int _groupNr) const;
    /** Returns a subset of the devices which are member of the given group
     * Note that these groups could be spanned over multiple dsMeters.
     */
    Set getByGroup(boost::shared_ptr<const Group> _group) const;
    /** Returns a subset of the devices which are member of the given group
     * Note that these groups could be spanned over multiple dsMeters.
     */
    Set getByGroup(const std::string& _name) const;

    /** Returns a subset of devices with the given function-id. */
    Set getByFunctionID(const int _functionID) const;

    /** Returns a subset that contains all devices belonging to Zone \a _zoneID. */
    Set getByZone(int _zoneID) const;

    /** Returns a subset that contains all devices belonging to Zone \a _zoneName */
    Set getByZone(const std::string& _zoneName) const;

    /** Returns a subset that contains all devices belonging to DSMeter \a _dsMeter */
    Set getByDSMeter(boost::shared_ptr<const DSMeter> _dsMeter) const;
    /** Returns a subset that contains all devices belonging to DSMeter identified by \a _dsMeterDSID */
    Set getByDSMeter(const dss_dsid_t& _dsMeterDSID) const;
    /** Returns a subset that contains all devices known to be registered last on DSMeter */
    Set getByLastKnownDSMeter(const dss_dsid_t& _dsMeterDSID) const;

    /** Returns a subset that contains all devices that have the presence state of \a _present */
    Set getByPresence(const bool _present) const;

    Set getByTag(const std::string& _tagName) const;

    /** Returns the device indicated by _name
     */
    DeviceReference getByName(const std::string& _name) const;
    /** Returns the device indicated by \a _busid */
    DeviceReference getByBusID(const devid_t _busid, const dss_dsid_t& _dsMeterID) const;
    DeviceReference getByBusID(const devid_t _busid, boost::shared_ptr<const DSMeter> _meter) const;

    /** Returns the device indicated by \a _dsid */
    DeviceReference getByDSID(const dss_dsid_t _dsid)  const;

    /** Returns the number of devices contained in this set */
    int length() const;
    /** Returns true if the set is empty */
    bool isEmpty() const;

    /** Returns a set that's combined with the set _other.
     * Duplicates get filtered out.
     */
    Set combine(Set& _other) const;
    /* Returns a set with all device in _other removed */
    Set remove(const Set& _other) const;

    /** Returns the \a _index'th device */
    const DeviceReference& get(int _index) const;
    /** @copydoc get */
    const DeviceReference& operator[](const int _index) const;

    /** @copydoc get */
    DeviceReference& get(int _index);
    /** @copydoc get */
    DeviceReference& operator[](const int _index);

    /** Returns true if the set contains \a _device */
    bool contains(const DeviceReference& _device) const;
    /** Returns true if the set contains \a _device */
    bool contains(boost::shared_ptr<const Device> _device) const;

    /** Adds the device \a _device to the set */
    void addDevice(const DeviceReference& _device);
    /** Adds the device \a _device to the set */
    void addDevice(boost::shared_ptr<const Device> _device);

    /** Removes the device \a _device from the set */
    void removeDevice(const DeviceReference& _device);
    /** Removes the device \a _device from the set */
    void removeDevice(boost::shared_ptr<const Device> _device);

    virtual void nextScene(const callOrigin_t _origin);
    virtual void previousScene(const callOrigin_t _origin);

    virtual unsigned long getPowerConsumption();
  protected:
    virtual std::vector<boost::shared_ptr<AddressableModelItem> > splitIntoAddressableItems();
  }; // Set


} // namespace dss

#endif // SET_H
