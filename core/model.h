/*
 *  model.h
 *  dSS
 *
 *  Copyright:
 *  (c) 2008 by
 *  futureLAB AG
 *  Schwalmenackerstrasse 4
 *  CH-8400 Winterthur / Schweiz
 *  Alle Rechte vorbehalten.
 *  Jede Art der Vervielfaeltigung, Verbreitung,
 *  Auswertung oder Veraenderung - auch auszugsweise -
 *  ist ohne vorgaengige schriftliche Genehmigung durch
 *  die futureLAB AG untersagt.
 *
 * Last change $Date: 2008-04-07 14:16:35 +0200 (Mon, 07 Apr 2008) $
 * by $Author: pstaehlin $
 */

#ifndef _MODEL_H_INCLUDED
#define _MODEL_H_INCLUDED

#include <bitset>

#include "base.h"
#include "datetools.h"
#include "ds485types.h"
#include "xmlwrapper.h"
#include "thread.h"
#include "subsystem.h"
#include "mutex.h"
#include "syncevent.h"

#include <vector>
#include <string>

using namespace std;

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
  class PropertyNode;

  /** Interface to a single or multiple devices.
   */
  class IDeviceInterface {
  public:
    /** Turns the device on.
     *  This will normaly invoke the first scene stored on the device.
     */
    virtual void turnOn() = 0;
    /** Turns the device off.
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
      * A disabled device does only react to enable().
      */
    virtual void disable() = 0;

    /** Starts dimming the given parameter.
     * If _directionUp is true, the value gets increased over time. Else its getting decreased
     */
    virtual void startDim(bool _directionUp, const int _parameterNr = -1) = 0;
    /** Stops the dimming */
    virtual void endDim(const int _parameterNr = -1)= 0;
    /** Sets the value of the given parameter */
    virtual void setValue(const double _value, int _parameterNr = -1) = 0;

    virtual void callScene(const int _sceneNr) = 0;
    virtual void saveScene(const int _sceneNr) = 0;
    virtual void undoScene(const int _sceneNr) = 0;

    /** Returns the consumption in mW */
    virtual unsigned long getPowerConsumption() = 0;

    virtual void nextScene() = 0;
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
    PropertyNode* m_pPropertyNode;
  public:
    DeviceReference(const DeviceReference& _copy);
    DeviceReference(const dsid_t _dsid, const Apartment& _apartment);
    DeviceReference(const Device& _device, const Apartment& _apartment);
    virtual ~DeviceReference() {};

    Device& getDevice();
    const Device& getDevice() const;
    dsid_t getDSID() const;

    int getFunctionID() const;
    bool hasSwitch() const;

    bool operator==(const DeviceReference& _other) const {
      return m_DSID == _other.m_DSID;
    }

    string getName() const;

    virtual void turnOn();
    virtual void turnOff();

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

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
 };

  typedef vector<DeviceReference> DeviceVector;
  typedef DeviceVector::iterator DeviceIterator;
  typedef DeviceVector::const_iterator DeviceConstIterator;
  typedef boost::tuple<double, double, double> DeviceLocation;

  /** Represents a dsID */
  class Device : public IDeviceInterface,
                 public boost::noncopyable {
  private:
    string m_Name;
    dsid_t m_DSID;
    devid_t m_ShortAddress;
    int m_ModulatorID;
    int m_ZoneID;
    Apartment* m_pApartment;
    bitset<63> m_GroupBitmask;
    vector<int> m_Groups;
    int m_FunctionID;
    int m_LastCalledScene;
    unsigned long m_Consumption;

    PropertyNode* m_pPropertyNode;
    DeviceLocation m_Location;
  protected:
    void publishToPropertyTree();
    void dirty();
  public:
    Device(const dsid_t _dsid, Apartment* _pApartment);
    virtual ~Device() {};

    virtual void turnOn();
    virtual void turnOff();

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

    virtual bool isOn() const;

    virtual void startDim(const bool _directionUp, const int _parameterNr = -1);
    virtual void endDim(const int _parameterNr = -1);
    virtual void setValue(const double _value, const int _parameterNr = -1);
    double getValue(const int _parameterNr = -1);

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);

    virtual void nextScene();
    virtual void previousScene();

    int getFunctionID() const;
    void setFunctionID(const int _value);
    bool hasSwitch() const;

    string getName() const;
    void setName(const string& _name);

    const DeviceLocation& getLocation() const;
    void setLocation(const DeviceLocation& _value);

    double getLocationX() const;
    double getLocationY() const;
    double getLocationZ() const;

    void setLocationX(const double _value);
    void setLocationY(const double _value);
    void setLocationZ(const double _value);

    bitset<63>& getGroupBitmask();
    bool isInGroup(const int _groupID) const;
    void addToGroup(const int _groupID);

    /** Returns the group id of the _index'th group */
    int getGroupIdByIndex(const int _index) const;
    /** Returns _index'th group of the device */
    Group& getGroupByIndex(const int _index);
    /** Returns the number of groups the device is a member of */
    int getGroupsCount() const;

    void resetGroups();

    int getLastCalledScene() const { return m_LastCalledScene; }
    void setLastCalledScene(const int _value) { m_LastCalledScene = _value; }

    /** Returns the short address of the device. This is the address
     * the device got from the dSM. */
    devid_t getShortAddress() const;
    void setShortAddress(const devid_t _shortAddress);
    /** Returns the DSID of the device */
    dsid_t getDSID() const;
    /** Returns the id of the modulator the device is connected to */
    int getModulatorID() const;
    void setModulatorID(const int _modulatorID);

    int getZoneID() const;
    void setZoneID(const int _value);
    /** Returns the apartment the device resides in. */
    Apartment& getApartment() const;

    virtual unsigned long getPowerConsumption();

    bool operator==(const Device& _other) const;
  };

  ostream& operator<<(ostream& out, const Device& _dt);

  /** Abstract interface to select certain Devices from a set */
  class IDeviceSelector {
  public:
    virtual bool selectDevice(const Device& _device) const = 0;
    virtual ~IDeviceSelector() {}
  };

  /** Abstract interface to perform an Action on each device of a set */
  class IDeviceAction {
  public:
    virtual bool perform(Device& _device) = 0;
    virtual ~IDeviceAction() {}
  };

  /** A set holds an arbitrary list of devices.
    * A Command sent to an instance of this class will replicate the command to all
    * contained devices.
   */
  class Set : public IDeviceInterface {
  private:
    DeviceVector m_ContainedDevices;
  public:
    Set();
    Set(const Set& _copy);
    Set(Device& _device);
    Set(DeviceReference& _reference);
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
    Set getByGroup(const string& _name) const;

    Set getByFunctionID(const int _functionID) const;

    Set getByZone(int _zoneID) const;
    /** Returns the device indicated by _name
     */
    DeviceReference getByName(const string& _name) const;
    /** Returns the device indicated by _busid */
    DeviceReference getByBusID(const devid_t _busid) const;

    /** Returns the device indicated by _dsid */
    DeviceReference getByDSID(const dsid_t _dsid)  const;

    /* Returns the number of devices contained in this set */
    int length() const;
    /* Returns true if the set is empty */
    bool isEmpty() const;

    /* Returns a set that's combined with the set _other.
     * Duplicates get filtered out.
     */
    Set combine(Set& _other) const;
    /* Returns a set with all device in _other removed */
    Set remove(const Set& _other) const;

    /** Returns the _index'th device */
    const DeviceReference& get(int _index) const;
    const DeviceReference& operator[](const int _index) const;

    DeviceReference& get(int _index);
    DeviceReference& operator[](const int _index);

    /** Returns true if the set contains _device */
    bool contains(const DeviceReference& _device) const;
    /** Returns true if the set contains _device */
    bool contains(const Device& _device) const;

    /** Adds the device _device to the set */
    void addDevice(const DeviceReference& _device);
    /** Adds the device _device to the set */
    void addDevice(const Device& _device);

    /** Removes the device _device from the set */
    void removeDevice(const DeviceReference& _device);
    /** Removes the device _device from the set */
    void removeDevice(const Device& _device);

    virtual void nextScene();
    virtual void previousScene();

    virtual unsigned long getPowerConsumption();
  }; // Set


  /** A class derived from DeviceContainer can deliver a Set of its Devices */
  class DeviceContainer {
  private:
    string m_Name;
  public:
    virtual Set getDevices() const = 0;
    /** Returns a subset of the devices contained, selected by _selector */
    virtual Set getDevices(const IDeviceSelector& _selector) const {
      return getDevices().getSubset(_selector);
    }

    virtual void setName(const string& _name);
    const string& getName() const { return m_Name; };

    virtual ~DeviceContainer() {};
  }; // DeviceContainer

  /** Represents a Modulator */
  class Modulator : public DeviceContainer {
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
  public:
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

    void removeDevice(const DeviceReference& _device);

    /** Returns the consumption in mW */
    unsigned long getPowerConsumption();
    /** Returns the meter value in Wh */
    unsigned long getEnergyMeterValue();

    int getEnergyLevelOrange() const { return m_EnergyLevelOrange; }
    int getEnergyLevelRed() const { return m_EnergyLevelRed; }
    void setEnergyLevelRed(const int _value) { m_EnergyLevelRed = _value; }
    void setEnergyLevelOrange(const int _value) { m_EnergyLevelOrange = _value; }
  }; // Modulator

  /** Represents a predefined group */
  class Group : public DeviceContainer,
                public IDeviceInterface {
  protected:
    DeviceVector m_Devices;
    Apartment& m_Apartment;
    int m_ZoneID;
    int m_GroupID;
    int m_LastCalledScene;
  public:
    Group(const int _id, const int _zoneID, Apartment& _apartment);
    virtual ~Group() {};
    virtual Set getDevices() const;

    /** Returns the id of the group */
    int getID() const;

    /** As of now, this function throws an error */
    virtual void addDevice(const DeviceReference& _device);
    /** As of now, this function throws an error */
    virtual void removeDevice(const DeviceReference& _device);


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

    int getLastCalledScene() const { return m_LastCalledScene; }
    void setLastCalledScene(const int _value) { m_LastCalledScene = _value; }

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

  /** Represents a Zone
    */
  class Zone : public DeviceContainer,
               public IDeviceInterface,
               public boost::noncopyable {
  private:
    int m_ZoneID;
    DeviceVector m_Devices;
    vector<Modulator*> m_Modulators;
    vector<Group*> m_Groups;
  public:
  	Zone(const int _id)
  	: m_ZoneID(_id)
  	{}
    virtual ~Zone();
    virtual Set getDevices() const;

    void addToModulator(Modulator& _modulator);
    void removeFromModulator(Modulator& _modulator);

    /** Adds a device to the zone.
     * This will permanently add the device to the zone.
     */
    void addDevice(const DeviceReference& _device);

    /** Removes a device from the zone.
     * This will permanently remove the device from the zone.
     */
    void removeDevice(const DeviceReference& _device);

    /** Returns the group with the name _name */
    Group* getGroup(const string& _name) const;
    /** Returns the group with the id _id */
    Group* getGroup(const int _id) const;

    /** Adds a group to the zone */
    void addGroup(Group* _group);
    /** Removes a group from the zone */
    void removeGroup(UserGroup* _group);

    /** Returns the zones id */
    int getZoneID() const;
    /** Sets the zones id */
    void setZoneID(const int _value);

    vector<int> getModulators() const;

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
    vector<Group*> getGroups() { return m_Groups; }
  }; // Zone


  class ModelEvent {
  public:
    typedef enum { etCallSceneGroup, etCallSceneDevice, etNewDevice, etModelDirty } EventType;
  private:
    EventType m_EventType;
    vector<int> m_Parameter;
  public:
    ModelEvent(EventType _type)
    : m_EventType(_type)
    {}

    void addParameter(const int _param) { m_Parameter.push_back(_param); }
    int getParameter(const int _index) const { return m_Parameter.at(_index); }
    int getParameterCount() const { return m_Parameter.size(); }
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
    vector<Device*> m_StaleDevices;
    vector<Modulator*> m_StaleModulators;
    vector<Zone*> m_StaleZones;
    vector<Group*> m_StaleGroups;

    vector<Zone*> m_Zones;
    vector<Modulator*> m_Modulators;
    vector<Device*> m_Devices;
    bool m_IsInitializing;

    PropertyNode* m_pPropertyNode;

    boost::ptr_vector<ModelEvent> m_ModelEvents;
    Mutex m_ModelEventsMutex;
    SyncEvent m_NewModelEvent;
  private:
    void loadDevices(XMLNode& _node);
    void loadModulators(XMLNode& _node);
    void loadZones(XMLNode& _node);
    Modulator& allocateModulator(const dsid_t _dsid);

    void addDefaultGroupsToZone(Zone& _zone);
    /** Starts the event-processing */
    virtual void execute();
  protected:
    virtual void doStart();
  public:
    Apartment(DSS* _pDSS);
    virtual ~Apartment();

    virtual void initialize();

    /** Returns a set containing all devices of the set */
    virtual Set getDevices() const;

    /** Loads the datamodel and marks the contained items as "stale" */
    void readConfigurationFromXML(const string& _fileName);

    void writeConfigurationToXML(const string& _fileName);

    /** Returns a reference to the device with the DSID _dsid */
    Device& getDeviceByDSID(const dsid_t _dsid) const;
    Device& getDeviceByDSID(const dsid_t _dsid);
    /** Returns a reference to the device with the name _name*/
    Device& getDeviceByName(const string& _name);
    /** Returns a device by it's short-address and modulator */
    Device& getDeviceByShortAddress(const Modulator& _modulator, const devid_t _deviceID) const;

    /** Allocates a device and returns a reference to it.
     *  If there is a stale device with the same dsid, this device gets "activated"
     */
    Device& allocateDevice(const dsid_t _dsid);

    /** Returns the Zone by name */
    Zone& getZone(const string& _zoneName);
    /** Returns the Zone by its id */
    Zone& getZone(const int _id);
    /** Returns a vector of all zones */
    vector<Zone*>& getZones();

    /** Allocates a zone and returns a reference to it. Should a zone with
      * the given _zoneID already exist, a reference to the existing zone will
      * be returned.
      */
    Zone& allocateZone(Modulator& _modulator, int _zoneID);

    /** Returns a Modulator by name */
    Modulator& getModulator(const string& _modName);
    /** Returns a Modulator by DSID  */
    Modulator& getModulatorByDSID(const dsid_t _dsid);
    /** Returns a Modulator by bus-id */
    Modulator& getModulatorByBusID(const int _busID);
    /** Returns a vector of all modulators */
    vector<Modulator*>& getModulators();

    /** Returns a Group by name */
    Group& getGroup(const string& _name);
    /** Returns a Group by id */
    Group& getGroup(const int _id);

    /** Allocates a group */
    UserGroup& allocateGroup(const int _id);

    bool isInitializing() const { return m_IsInitializing; }

  public:

    /** Returns the root-node for the apartment tree */
    PropertyNode* getPropertyNode() { return m_pPropertyNode; }

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
    ItemNotFoundException(const string& _name) : DSSException(string("Could not find item ") + _name) {};
    ~ItemNotFoundException() throw() {}
  };

  class SceneHelper {
  private:
    static std::bitset<64> m_ZonesToIgnore;
    static bool m_Initialized;

    static void initialize();
  public:
    static bool rememberScene(const unsigned int _sceneID);
    static unsigned int getNextScene(const unsigned int _currentScene);
    static unsigned int getPreviousScene(const unsigned int _currentScene);
  };

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
      return x->getZoneID();
    }
  };
}

//#endif // DOC
#endif // MODEL_H_INCLUDED
