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

#ifndef APARTMENT_H
#define APARTMENT_H

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "devicecontainer.h"
#include "modeltypes.h"
#include "modelevent.h"
#include "group.h"
#include "core/subsystem.h"
#include "core/mutex.h"
#include "core/thread.h"
#include "core/syncevent.h"
#include "core/businterface.h"

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class Zone;
  class DSMeter;
  class Device;
  class Group;
  class Event;
  class BusRequestDispatcher;
  class BusRequest;
  class ModelMaintenance;
  class PropertySystem;

  /** Represents an Apartment
    * This is the root of the datamodel of the dss. The Apartment is responsible for delivering
    * and loading all subitems.
    */
  class Apartment : public boost::noncopyable,
                    public DeviceContainer,
                    public LockableObject
  {
  private:
    std::vector<boost::shared_ptr<Zone> > m_Zones;
    std::vector<boost::shared_ptr<DSMeter> > m_DSMeters;
    std::vector<boost::shared_ptr<Device> > m_Devices;

    BusInterface* m_pBusInterface;
    BusRequestDispatcher* m_pBusRequestDispatcher;
    PropertyNodePtr m_pPropertyNode;
    ModelMaintenance* m_pModelMaintenance;
    PropertySystem* m_pPropertySystem;
  private:
    void addDefaultGroupsToZone(boost::shared_ptr<Zone> _zone);
  public:
    Apartment(DSS* _pDSS);
    virtual ~Apartment();

    /** Returns a set containing all devices of the set */
    virtual Set getDevices() const;

    /** Returns a reference to the device with the DSID \a _dsid */
    boost::shared_ptr<Device> getDeviceByDSID(const dss_dsid_t _dsid) const;
    /** @copydoc getDeviceByDSID */
    boost::shared_ptr<Device> getDeviceByDSID(const dss_dsid_t _dsid);
    /** Returns a reference to the device with the name \a _name*/
    boost::shared_ptr<Device> getDeviceByName(const std::string& _name);
    /** Returns a device by it's short-address and dsMeter */
    boost::shared_ptr<Device> getDeviceByShortAddress(boost::shared_ptr<const DSMeter> _dsMeter, const devid_t _deviceID) const;
    std::vector<boost::shared_ptr<Device> > getDevicesVector() { return m_Devices; }
    /** Allocates a device and returns a reference to it. */
    boost::shared_ptr<Device> allocateDevice(const dss_dsid_t _dsid);

    /** Returns the Zone by name */
    boost::shared_ptr<Zone> getZone(const std::string& _zoneName);
    /** Returns the Zone by its id */
    boost::shared_ptr<Zone> getZone(const int _id);
    /** Returns a vector of all zones */
    std::vector<boost::shared_ptr<Zone> > getZones();

    /** Allocates a zone and returns a reference to it. Should a zone with
      * the given _zoneID already exist, a reference to the existing zone will
      * be returned.
      * NOTE: Outside code should never call this function
      */
    boost::shared_ptr<Zone> allocateZone(int _zoneID);

    boost::shared_ptr<DSMeter> allocateDSMeter(const dss_dsid_t _dsid);

    /** Returns a DSMeter by name */
    boost::shared_ptr<DSMeter> getDSMeter(const std::string& _modName);
    /** Returns a DSMeter by DSID  */
    boost::shared_ptr<DSMeter> getDSMeterByDSID(const dss_dsid_t _dsid);
    /** Returns a vector of all dsMeters */
    std::vector<boost::shared_ptr<DSMeter> > getDSMeters();

    /** Returns a Group by name */
    boost::shared_ptr<Group> getGroup(const std::string& _name);
    /** Returns a Group by id */
    boost::shared_ptr<Group> getGroup(const int _id);

    void removeZone(int _zoneID);
    void removeDevice(dss_dsid_t _device);
    void removeDSMeter(dss_dsid_t _dsMeter);
  public:
    void setBusInterface(BusInterface* _value) { m_pBusInterface = _value; }
    DeviceBusInterface* getDeviceBusInterface();
    void setBusRequestDispatcher(BusRequestDispatcher* _value) { m_pBusRequestDispatcher = _value; }
    void dispatchRequest(boost::shared_ptr<BusRequest> _pRequest);
    /** Returns the root-node for the apartment tree */
    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }
    void setModelMaintenance(ModelMaintenance* _value) { m_pModelMaintenance = _value; }
    ModelMaintenance* getModelMaintenance() { return m_pModelMaintenance; }
    void setPropertySystem(PropertySystem* _value);
    PropertySystem* getPropertySystem() { return m_pPropertySystem; }
  }; // Apartment

  /** Exception that will be thrown if a given item could not be found */
  class ItemNotFoundException : public DSSException {
  public:
    ItemNotFoundException(const std::string& _name) : DSSException(std::string("Could not find item ") + _name) {};
    ~ItemNotFoundException() throw() {}
  };

} // namespace dss

#endif // APARTMENT_H
