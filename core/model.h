/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#ifndef _MODEL_H_INCLUDED
#define _MODEL_H_INCLUDED

#include <bitset>

#include "base.h"
#include "datetools.h"
#include "ds485types.h"
#include "thread.h"
#include "subsystem.h"
#include "mutex.h"
#include "syncevent.h"

#include <vector>
#include <string>

#include <boost/utility.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace dss {

  class Device;
  class Set;
  class DeviceContainer;
  class Apartment;
  class Group;
  class Modulator;
  class PropertyNode;
  class XMLNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  class PhysicalModelItem {
  private:
    bool m_IsPresent;
  public:
    PhysicalModelItem()
    : m_IsPresent(false)
    { }

    bool isPresent() const { return m_IsPresent; }
    void setIsPresent(const bool _value) { m_IsPresent = _value; }
  };

  /** Interface to a single or multiple devices.
   */
  class IDeviceInterface {
  public:
    /** Turns the device on.
     *  This will invoke scene "max".
     */
    virtual void turnOn() = 0;
    /** Turns the device off.
     * This will invoke scene "min"
     */
    virtual void turnOff() = 0;

    /** Increases the value of the given parameter by one,
     * If no parameter gets passed, it will increase the default value of the device(s).
     */
    virtual void increaseValue(const int _parameterNr = -1) = 0;
    /** Decreases the value of the given parameter by one.
     * If no parameter gets passed, it will decrease the default value of the device(s).
     */
    virtual void decreaseValue(const int _parameterNr = -1) = 0;

    /** Enables a previously disabled device.
     */
    virtual void enable() = 0;
    /** Disables a device.
      * A disabled device may only react to enable().
      */
    virtual void disable() = 0;

    /** Starts dimming the given parameter.
     * If _directionUp is true, the value gets increased over time. Else its getting decreased.
     */
    virtual void startDim(bool _directionUp, const int _parameterNr = -1) = 0;
    /** Stops the dimming */
    virtual void endDim(const int _parameterNr = -1)= 0;
    /** Sets the value of the given parameter */
    virtual void setValue(const double _value, int _parameterNr = -1) = 0;


    /** Sets the scene on the device.
     * The output value will be set according to the scene lookup table in the device.
     */
    virtual void callScene(const int _sceneNr) = 0;
    /** Stores the current output value into the scene lookup table.
     * The next time scene _sceneNr gets called the output will be set according to the lookup table.
     */
    virtual void saveScene(const int _sceneNr) = 0;
    /** Restores the last set value of _sceneNr
     */
    virtual void undoScene(const int _sceneNr) = 0;

    /** Returns the consumption in mW */
    virtual unsigned long getPowerConsumption() = 0;

    /** Calls the next scene according to the last called scene.
     * @see dss::SceneHelper::getNextScene
     */
    virtual void nextScene() = 0;
    /** Calls the previos scene according to the last called scene.
     * @see dss::SceneHelper::getPreviousScene
     */
    virtual void previousScene() = 0;

    virtual ~IDeviceInterface() {};
  };

  /** Internal reference to a device.
   * A DeviceReference is virtually interchangable with a device. It is used in places
     where a reference to a device is needed.
   */
  class DeviceReference : public IDeviceInterface {
  private:
    dsid_t m_DSID;
    const Apartment* m_Apartment;
  public:
    /** Copy constructor */
    DeviceReference(const DeviceReference& _copy);
    DeviceReference(const dsid_t _dsid, const Apartment& _apartment);
    DeviceReference(const Device& _device, const Apartment& _apartment);
    virtual ~DeviceReference() {};

    /** Returns a reference to the referenced device
     * @note This accesses the apartment which has to be valid.
     */
    Device& getDevice();
    /** @copydoc getDevice() */
    const Device& getDevice() const;
    /** Returns the DSID of the referenced device */
    dsid_t getDSID() const;

    /** Returns the function id.
     * @note This will lookup the device */
    int getFunctionID() const;
    bool hasSwitch() const;

    /** Compares two device references.
     * Device references are considered equal if their DSID match. */
    bool operator==(const DeviceReference& _other) const {
      return m_DSID == _other.m_DSID;
    }

    /** Returns the name of the referenced device.
     * @note This will lookup the device. */
    std::string getName() const;

    virtual void turnOn();
    virtual void turnOff();

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

    /** Returns wheter the device is turned on.
     * @note The detection is soly based on the last called scene. As soon as we've
     * got submetering data this should reflect the real state.
     */
    virtual bool isOn() const;

    virtual void startDim(const bool _directionUp, const int _parameterNr = -1);
    virtual void endDim(const int _parameterNr = -1);
    virtual void setValue(const double _value, const int _parameterNr = -1);

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);

    virtual void nextScene();
    virtual void previousScene();

    virtual unsigned long getPowerConsumption();
 }; // DeviceReference

  typedef std::vector<DeviceReference> DeviceVector;
  typedef DeviceVector::iterator DeviceIterator;
  typedef DeviceVector::const_iterator DeviceConstIterator;
  typedef boost::tuple<double, double, double> DeviceLocation;

  /** Represents a dsID */
  class Device : public IDeviceInterface,
                 public PhysicalModelItem,
                 public boost::noncopyable {
  private:
    std::string m_Name;
    dsid_t m_DSID;
    devid_t m_ShortAddress;
    int m_ModulatorID;
    int m_ZoneID;
    Apartment* m_pApartment;
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
    /** Publishes the device to the property tree.
     * @see DSS::getPropertySystem */
    void publishToPropertyTree();
    /** Sends the application a note that something has changed.
     * This will cause the \c apartment.xml to be updated. */
    void dirty();
  public:
    /** Creates and initializes a device. */
    Device(const dsid_t _dsid, Apartment* _pApartment);
    virtual ~Device() {};

    virtual void turnOn();
    virtual void turnOff();

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

    /** @copydoc DeviceReference::isOn() */
    virtual bool isOn() const;

    virtual void startDim(const bool _directionUp, const int _parameterNr = -1);
    virtual void endDim(const int _parameterNr = -1);
    virtual void setValue(const double _value, const int _parameterNr = -1);
    void setRawValue(const uint16_t _value, const int _parameterNr, const int _size);
    /** Returns the value of _parameterNr.
     * @note not yet implemented */
    double getValue(const int _parameterNr = -1);

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);

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
    /** Returns the id of the modulator the device is connected to */
    int getModulatorID() const;
    /** Sets the modulatorID of the device. */
    void setModulatorID(const int _modulatorID);

    /** Returns the zone ID the device resides in. */
    int getZoneID() const;
    /** Sets the zone ID of the device. */
    void setZoneID(const int _value);
    /** Returns the apartment the device resides in. */
    Apartment& getApartment() const;
    
    const DateTime& getLastDiscovered() const { return m_LastDiscovered; }
    const DateTime& getFirstSeen() const { return m_FirstSeen; }
    void setFirstSeen(const DateTime& _value) { m_FirstSeen = _value; }

    virtual unsigned long getPowerConsumption();

    /** Sends a byte to the device using dsLink. If \a _writeOnly is \c true,
     * the result of the function is not defined. */
    uint8_t dsLinkSend(uint8_t _value, bool _lastByte, bool _writeOnly);

    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }

    /** Returns wheter two devices are equal.
     * Devices are considered equal if their DSID are a match.*/
    bool operator==(const Device& _other) const;
  }; // Device

  std::ostream& operator<<(std::ostream& out, const Device& _dt);

  /** Abstract interface to select certain Devices from a set */
  class IDeviceSelector {
  public:
    /** The implementor should return true if the device should appear in the
     * resulting set. */
    virtual bool selectDevice(const Device& _device) const = 0;
    virtual ~IDeviceSelector() {}
  };

  /** Abstract interface to perform an Action on each device of a set */
  class IDeviceAction {
  public:
    /** This action will be performed for every device contained in the set. */
    virtual bool perform(Device& _device) = 0;
    virtual ~IDeviceAction() {}
  };

  /** A set holds an arbitrary list of devices.
    * A Command sent to an instance of this class will replicate the command to all
    * contained devices.
    * Only references to devices will be stored.
   */
  class Set : public IDeviceInterface {
  private:
    DeviceVector m_ContainedDevices;
  public:
    /** Constructor for an empty Set.*/
    Set();
    /** Copy constructor. */
    Set(const Set& _copy);
    /** Constructor for a set containing only \a _device. */
    Set(Device& _device);
    /** Constructor for a set containing only \a _reference. */
    Set(DeviceReference& _reference);
    /** Constructor for a set containing \a _devices. */
    Set(DeviceVector _devices);
    virtual ~Set() {};

    virtual void turnOn();
    virtual void turnOff();

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

    virtual void startDim(bool _directionUp, const int _parameterNr = -1);
    virtual void endDim(const int _parameterNr = -1);
    virtual void setValue(const double _value, int _parameterNr = -1);

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);

    /** Performs the given action on all contained devices */
    void perform(IDeviceAction& _deviceAction);

    /** Returns a subset selected by the given selector
     * A device will be included in the resulting set if _selector.selectDevice(...) return true.
     */
    Set getSubset(const IDeviceSelector& _selector) const;
    /** Returns a subset of the devices which are member of the given group
    * Note that these groups could be spanned over multiple modulators.
     */
    Set getByGroup(int _groupNr) const;
    /** Returns a subset of the devices which are member of the given group
     * Note that these groups could be spanned over multiple modulators.
     */
    Set getByGroup(const Group& _group) const;
    /** Returns a subset of the devices which are member of the given group
     * Note that these groups could be spanned over multiple modulators.
     */
    Set getByGroup(const std::string& _name) const;

    /** Returns a subset of devices with the given function-id. */
    Set getByFunctionID(const int _functionID) const;

    /** Returns a subset that contains all devices belonging to Zone \a _zoneID. */
    Set getByZone(int _zoneID) const;

    /** Returns a subset that contains all devices belonging to Zone \a _zoneName */
    Set getByZone(const std::string& _zoneName) const;

    /** Returns a subset that contains all devices belonging to Modulator \a _modulatorID */
    Set getByModulator(const int _modulatorID) const;
    /** Returns a subset that contains all devices belonging to Modulator \a _modulator */
    Set getByModulator(const Modulator& _modulator) const;

    /** Returns a subset that contains all devices that have the presence state of \a _present */
    Set getByPresence(const bool _present) const;

    /** Returns the device indicated by _name
     */
    DeviceReference getByName(const std::string& _name) const;
    /** Returns the device indicated by \a _busid */
    DeviceReference getByBusID(const devid_t _busid) const;

    /** Returns the device indicated by \a _dsid */
    DeviceReference getByDSID(const dsid_t _dsid)  const;

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
    bool contains(const Device& _device) const;

    /** Adds the device \a _device to the set */
    void addDevice(const DeviceReference& _device);
    /** Adds the device \a _device to the set */
    void addDevice(const Device& _device);

    /** Removes the device \a _device from the set */
    void removeDevice(const DeviceReference& _device);
    /** Removes the device \a _device from the set */
    void removeDevice(const Device& _device);

    virtual void nextScene();
    virtual void previousScene();

    virtual unsigned long getPowerConsumption();
  }; // Set


  /** A class derived from DeviceContainer can deliver a Set of its Devices */
  class DeviceContainer {
  private:
    std::string m_Name;
  public:
    /** Returns a set containing all devices of the container. */
    virtual Set getDevices() const = 0;
    /** Returns a subset of the devices contained, selected by \a _selector */
    virtual Set getDevices(const IDeviceSelector& _selector) const {
      return getDevices().getSubset(_selector);
    }

    /** Sets the name of the container. */
    virtual void setName(const std::string& _name);
    /** Returns the name of the container */
    const std::string& getName() const { return m_Name; };

    virtual ~DeviceContainer() {};
  }; // DeviceContainer

  /** Represents a Modulator */
  class Modulator : public DeviceContainer,
                    public PhysicalModelItem {
  private:
    dsid_t m_DSID;
    int m_BusID;
    DeviceVector m_ConnectedDevices;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_PowerConsumption;
    DateTime m_PowerConsumptionAge;
    int m_EnergyMeterValue;
    DateTime m_EnergyMeterValueAge;
    int m_HardwareVersion;
    int m_SoftwareVersion;
    std::string m_HardwareName;
    int m_DeviceType;
    bool m_IsValid;
  public:
    /** Constructs a modulator with the given dsid. */
    Modulator(const dsid_t _dsid);
    virtual ~Modulator() {};
    virtual Set getDevices() const;

    /** Returns the DSID of the Modulator */
    dsid_t getDSID() const;
    /** Returns the bus id of the Modulator */
    int getBusID() const;
    /** Sets the bus id of the Modulator */
    void setBusID(const int _busID);

    /** Adds a DeviceReference to the modulators devices list */
    void addDevice(const DeviceReference& _device);

    /** Removes the device identified by the reference. */
    void removeDevice(const DeviceReference& _device);

    /** Returns the consumption in mW */
    unsigned long getPowerConsumption();
    /** Returns the meter value in Wh */
    unsigned long getEnergyMeterValue();


    /** set the consumption in mW */
    void setPowerConsumption(unsigned long _value);
    /** set the meter value in Wh */
    void setEnergyMeterValue(unsigned long _value);

    /** Returns the last consumption in mW returned from dS485 Bus, but never request it*/
    unsigned long getCachedPowerConsumption();
    /** Returns the last meter value in Wh returned from dS485 Bus, but never request it*/
    unsigned long getCachedEnergyMeterValue();
    
    /** Returns the orange energy level */
    int getEnergyLevelOrange() const { return m_EnergyLevelOrange; }
    /** Returns the red energy level */
    int getEnergyLevelRed() const { return m_EnergyLevelRed; }
    /** Sets the orange energy level.
     * @note This has no effect on the modulator as of now. */
    void setEnergyLevelRed(const int _value) { m_EnergyLevelRed = _value; }
    /** Sets the red energy level.
     * @note This has no effect on the modulator as of now. */
    void setEnergyLevelOrange(const int _value) { m_EnergyLevelOrange = _value; }

    int getHardwareVersion() const { return m_HardwareVersion; }
    void setHardwareVersion(const int _value) { m_HardwareVersion = _value; }
    int getSoftwareVersion() const { return m_SoftwareVersion; }
    void setSoftwareVersion(const int _value) { m_SoftwareVersion = _value; }
    std::string getHardwareName() const { return m_HardwareName; }
    void setHardwareName(const std::string& _value) { m_HardwareName = _value; }
    int getDeviceType() { return m_DeviceType; }
    void setDeviceType(const int _value) { m_DeviceType = _value; }

    /** Returns true if the modulator has been read-out completely. */
    bool isValid() const { return m_IsValid; }
    void setIsValid(const bool _value) { m_IsValid = _value; }
  }; // Modulator

  /** Represents a predefined group */
  class Group : public DeviceContainer,
                public IDeviceInterface,
                public PhysicalModelItem  {
  protected:
    DeviceVector m_Devices;
    Apartment& m_Apartment;
    int m_ZoneID;
    int m_GroupID;
    int m_LastCalledScene;
  public:
    /** Constructs a group with the given id belonging to \a _zoneID. */
    Group(const int _id, const int _zoneID, Apartment& _apartment);
    virtual ~Group() {};
    virtual Set getDevices() const;

    /** Returns the id of the group */
    int getID() const;
    int getZoneID() const;

    virtual void turnOn();
    virtual void turnOff();

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

    virtual void startDim(bool _directionUp, const int _parameterNr = -1);
    virtual void endDim(const int _parameterNr = -1);
    virtual void setValue(const double _value, int _parameterNr = -1);

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);

    virtual void nextScene();
    virtual void previousScene();

    virtual unsigned long getPowerConsumption();

    /** @copydoc Device::getLastCalledScene */
    int getLastCalledScene() const { return m_LastCalledScene; }
    /** @copydoc Device::setLastCalledScene */
    void setLastCalledScene(const int _value) { m_LastCalledScene = _value; }

    /** Compares a group to another group.
     * Two groups are considered equal if they belong to the same group and zone. */
    Group& operator=(const Group& _other);
  }; // Group


  /** Represents a user-defined-group */
  class UserGroup : public Group {
  private:
  public:
    /** Adds a device to the group.
     * This will permanently add the device to the group.
     */
    virtual void addDevice(const Device& _device);
    /** Removes a device from the group.
     * This will permanently remove the device from the group.
     */
    virtual void removeDevice(const Device& _device);
  }; // UserGroup

  /** Represents a Zone.
   * A Zone houses multiple devices. It can span over multiple modulators.
   */
  class Zone : public DeviceContainer,
               public IDeviceInterface,
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

    virtual void turnOn();
    virtual void turnOff();

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

    virtual void startDim(bool _directionUp, const int _parameterNr = -1);
    virtual void endDim(const int _parameterNr = -1);
    virtual void setValue(const double _value, int _parameterNr = -1);

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);

    virtual void nextScene();
    virtual void previousScene();

    virtual unsigned long getPowerConsumption();
    /** Returns a vector of groups present on the zone. */
    std::vector<Group*> getGroups() { return m_Groups; }
  }; // Zone


  /** A Model event gets processed by the apartment asynchronously.
   * It consists of multiple integer parameter whose meanig is defined by ModelEvent::EventType
   */
  class ModelEvent {
  public:
    typedef enum { etCallSceneGroup,  /**< A group has changed the scene. */
                   etCallSceneDevice, /**< A device has changed the scene (only raised from the simulation at the moment). */
                   etNewDevice,       /**< A new device has been detected */
                   etModelDirty,      /**< A parameter that will be stored in \c apartment.xml has been changed. */
                   etDSLinkInterrupt,  /**< An interrupt has occured */
                   etNewModulator, /**< A new modulator has joined the bus */
                   etLostModulator, /**< We've lost a modulator on the bus */
                   etModulatorReady, /**< A modulator has completed its scanning cycle and is now ready */
                   etBusReady, /**< The bus transitioned into ready state */
                   etPowerConsumption, /**< Powerconsumption message happened */
                   etEnergyMeterValue, /**< Powerconsumption message happened */
                   etDS485DeviceDiscovered, /**< A new device has been discovered on the bus */
                 } EventType;
  private:
    EventType m_EventType;
    std::vector<int> m_Parameter;
  public:
    /** Constructs a ModelEvent with the given EventType. */
    ModelEvent(EventType _type)
    : m_EventType(_type)
    {}

    /** Adds an integer parameter. */
    void addParameter(const int _param) { m_Parameter.push_back(_param); }
    /** Returns the parameter at _index.
     * @note Check getParameterCount to check the bounds. */
    int getParameter(const int _index) const { return m_Parameter.at(_index); }
    /** Returns the parameter count. */
    int getParameterCount() const { return m_Parameter.size(); }
    /** Returns the type of the event. */
    EventType getEventType() { return m_EventType; }
  };

  /** Represents an Apartment
    * This is the root of the datamodel of the dss. The Apartment is responsible for delivering
    * and loading all subitems.
    */
  class Apartment : public boost::noncopyable,
                    public DeviceContainer,
                    public Subsystem,
                    private Thread
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
    int m_RescanBusIn;
  private:
    void loadDevices(XMLNode& _node);
    void loadModulators(XMLNode& _node);
    void loadZones(XMLNode& _node);

    void addDefaultGroupsToZone(Zone& _zone);
    /** Starts the event-processing */
    virtual void execute();
    void handleModelEvents();
    void modulatorReady(int _modulatorBusID);
    void setPowerConsumption(int _modulatorBusID, unsigned long _value);
    void setEnergyMeterValue(int _modulatorBusID, unsigned long _value);
    void discoverDS485Devices();
  protected:
    virtual void doStart();
  public:
    Apartment(DSS* _pDSS);
    virtual ~Apartment();

    virtual void initialize();

    /** Returns a set containing all devices of the set */
    virtual Set getDevices() const;

    /** Loads the datamodel and marks the contained items as "stale" */
    void readConfigurationFromXML(const std::string& _fileName);

    void writeConfigurationToXML(const std::string& _fileName);

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

    bool scanModulator(Modulator& _modulator);
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
  }; // Apartment

  //============================================= Helper definitions
  typedef bool (*DeviceSelectorFun)(const Device& _device);

  /** Device selector that works on simple function instead of classes */
  class DeviceSelector : public IDeviceSelector {
  private:
    DeviceSelectorFun m_SelectorFunction;
  public:
    DeviceSelector(DeviceSelectorFun& _selectorFun) : m_SelectorFunction(_selectorFun) {}
    virtual bool selectDevice(const Device& _device) { return m_SelectorFunction(_device); }

    virtual ~DeviceSelector() {}
  }; // DeviceSelector

  /** Exception that will be thrown if a given item could not be found */
  class ItemNotFoundException : public DSSException {
  public:
    ItemNotFoundException(const std::string& _name) : DSSException(std::string("Could not find item ") + _name) {};
    ~ItemNotFoundException() throw() {}
  };

  /** Helper functions for scene management. */
  class SceneHelper {
  private:
    static std::bitset<64> m_ZonesToIgnore;
    static bool m_Initialized;

    static void initialize();
  public:
    /** Returns wheter to remember a scene.
     * Certain scenes represent events thus they won't have to be remembered.
     */
    static bool rememberScene(const unsigned int _sceneID);
    /** Returns the next scene based on _currentScene.
     * From off to Scene1 -> Scene2 -> Scene3 -> Scene4 -> Scene1...
     * \param _currentScene Scene now active.
     */
    static unsigned int getNextScene(const unsigned int _currentScene);
    /** Returns the previous scene based on _currentScene.
     * From off to Scene1 -> Scene4 -> Scene3 -> Scene2 -> Scene1...
     * \param _currentScene Scene now active.
     */
    static unsigned int getPreviousScene(const unsigned int _currentScene);
  }; // SceneHelper

}

//#ifdef DOC

#ifndef WIN32
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#include <stdexcept>

#ifndef WIN32
using namespace __gnu_cxx;
#else
using namespace stdext;
#endif

namespace __gnu_cxx
{
  template<>
  struct hash<const dss::Modulator*>  {
    size_t operator()(const dss::Modulator* x) const  {
      return x->getDSID().lower;
    }
  };

  template<>
  struct hash<const dss::Zone*>  {
    size_t operator()(const dss::Zone* x) const  {
      return x->getID();
    }
  };
}

//#endif // DOC
#endif // MODEL_H_INCLUDED
