/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2010 futureLAB AG, Winterthur, Switzerland

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
#include "core/DS485Interface.h"

namespace Poco {
  namespace XML {
    class Node;
  }
}

namespace dss {

  class Zone;
  class Modulator;
  class Device;
  class Group;
  class Event;
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class BusRequestDispatcher;
  class BusRequest;

  /** Represents an Apartment
    * This is the root of the datamodel of the dss. The Apartment is responsible for delivering
    * and loading all subitems.
    */
  class Apartment : public boost::noncopyable,
                    public DeviceContainer,
                    public Subsystem,
                    public LockableObject,
                    public Thread
  {
  private:
    std::vector<Zone*> m_Zones;
    std::vector<Modulator*> m_Modulators;
    std::vector<Device*> m_Devices;
    bool m_IsInitializing;

    PropertyNodePtr m_pPropertyNode;

    boost::ptr_vector<ModelEvent> m_ModelEvents;
    Mutex m_ModelEventsMutex;
    SyncEvent m_NewModelEvent;
    DS485Interface* m_pDS485Interface;
    BusRequestDispatcher* m_pBusRequestDispatcher;
  private:
    void loadDevices(Poco::XML::Node* _node);
    void loadModulators(Poco::XML::Node* _node);
    void loadZones(Poco::XML::Node* _node);

    void addDefaultGroupsToZone(Zone& _zone);
    void handleModelEvents();
    void modulatorReady(int _modulatorBusID);
    void setPowerConsumption(int _modulatorBusID, unsigned long _value);
    void setEnergyMeterValue(int _modulatorBusID, unsigned long _value);
    void discoverDS485Devices();

    void raiseEvent(boost::shared_ptr<Event> _pEvent);
    void waitForInterface();
  protected:
    virtual void doStart();
  public:
    Apartment(DSS* _pDSS, DS485Interface* _pDS485Interface);
    virtual ~Apartment();

    virtual void initialize();

    /** Returns a set containing all devices of the set */
    virtual Set getDevices() const;

    /** Loads the datamodel and marks the contained items as "stale" */
    void readConfigurationFromXML(const std::string& _fileName);
    void readConfiguration();

    void writeConfigurationToXML(const std::string& _fileName);
    void writeConfiguration();

    /** Returns a reference to the device with the DSID \a _dsid */
    Device& getDeviceByDSID(const dsid_t _dsid) const;
    /** @copydoc getDeviceByDSID */
    Device& getDeviceByDSID(const dsid_t _dsid);
    /** Returns a reference to the device with the name \a _name*/
    Device& getDeviceByName(const std::string& _name);
    /** Returns a device by it's short-address and modulator */
    Device& getDeviceByShortAddress(const Modulator& _modulator, const devid_t _deviceID) const;

    /** Allocates a device and returns a reference to it.
     *  If there is a stale device with the same dsid, this device gets "activated"
     */
    Device& allocateDevice(const dsid_t _dsid);

    /** Returns the Zone by name */
    Zone& getZone(const std::string& _zoneName);
    /** Returns the Zone by its id */
    Zone& getZone(const int _id);
    /** Returns a vector of all zones */
    std::vector<Zone*>& getZones();

    /** Allocates a zone and returns a reference to it. Should a zone with
      * the given _zoneID already exist, a reference to the existing zone will
      * be returned.
      * NOTE: Outside code should never call this function
      */
    Zone& allocateZone(int _zoneID);

    Modulator& allocateModulator(const dsid_t _dsid);

    /** Returns a Modulator by name */
    Modulator& getModulator(const std::string& _modName);
    /** Returns a Modulator by DSID  */
    Modulator& getModulatorByDSID(const dsid_t _dsid);
    /** Returns a Modulator by bus-id */
    Modulator& getModulatorByBusID(const int _busID);
    /** Returns a vector of all modulators */
    std::vector<Modulator*>& getModulators();

    /** Returns a Group by name */
    Group& getGroup(const std::string& _name);
    /** Returns a Group by id */
    Group& getGroup(const int _id);

    /** Allocates a group */
    UserGroup& allocateGroup(const int _id);

    /** Returns wheter the apartment is still initializing or already running. */
    bool isInitializing() const { return m_IsInitializing; }

    void removeZone(int _zoneID);
    void removeDevice(dsid_t _device);
    void removeModulator(dsid_t _modulator);
  public:

    /** Returns the root-node for the apartment tree */
    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }

    /** Adds a model event to the queue.
     * The ownership of the event will reside with the Apartment. ModelEvents arriving while initializing will be discarded.
     */
    void addModelEvent(ModelEvent* _pEvent);

    /** Called by the DS485Proxy if a group-call-scene frame was intercepted.
     *  Updates the state of all devices contained in the group. */
    void onGroupCallScene(const int _zoneID, const int _groupID, const int _sceneID);
    /** Called by the DS485Proxy if a device-call-scene frame was intercepted.
     *  Updates the state of the device. */
    void onDeviceCallScene(const int _modulatorID, const int _deviceID, const int _sceneID);
    /** Called by the DS485Proxy if an add-device frame was intercepted.
     *  Adds the device to the model. */
    void onAddDevice(const int _modID, const int _zoneID, const int _devID, const int _functionID);
    void onDSLinkInterrupt(const int _modID, const int _devID, const int _priority);
    /** Starts the event-processing */
    virtual void execute();
  public:
    void setDS485Interface(DS485Interface* _value) { m_pDS485Interface = _value; }
    DeviceBusInterface* getDeviceBusInterface();
    void setBusRequestDispatcher(BusRequestDispatcher* _value) { m_pBusRequestDispatcher = _value; }
    void dispatchRequest(boost::shared_ptr<BusRequest> _pRequest);
  }; // Apartment
  
  /** Exception that will be thrown if a given item could not be found */
  class ItemNotFoundException : public DSSException {
  public:
    ItemNotFoundException(const std::string& _name) : DSSException(std::string("Could not find item ") + _name) {};
    ~ItemNotFoundException() throw() {}
  };

} // namespace dss

#endif // APARTMENT_H
