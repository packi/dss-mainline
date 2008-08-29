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

#include <vector>
#include <string>

using namespace std;

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

namespace dss {

  class Device;
  class Set;
  class DeviceContainer;
  class Apartment;
  class Group;

  /** Interface to a single or multiple devices.
   */
  class IDeviceInterface {
  public:
    /** Turns the device on.
     *  This will normaly invoke the first scene stored on the device.
     */
    virtual void TurnOn() = 0;
    /** Turns the device off.
      */
    virtual void TurnOff() = 0;

    /** Increases the value of the given parameter by one,
     * If no parameter gets passed, it will increase the default value of the device(s).
     */
    virtual void IncreaseValue(const int _parameterNr = -1) = 0;
    /** Decreases the value of the given parameter by one.
     * If no parameter gets passed, it will decrease the default value of the device(s).
     */
    virtual void DecreaseValue(const int _parameterNr = -1) = 0;

    /** Enables a previously disabled device.
     */
    virtual void Enable() = 0;
    /** Disables a device.
      * A disabled device does only react to Enable().
      */
    virtual void Disable() = 0;

    /** Starts dimming the given parameter.
     * If _directionUp is true, the value gets increased over time. Else its getting decreased
     */
    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) = 0;
    /** Stops the dimming */
    virtual void EndDim(const int _parameterNr = -1)= 0;
    /** Sets the value of the given parameter */
    virtual void SetValue(const double _value, int _parameterNr = -1) = 0;

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
    DeviceReference(const DeviceReference& _copy);
    DeviceReference(const dsid_t _dsid, const Apartment& _apartment);
    DeviceReference(const Device& _device, const Apartment& _apartment);
    virtual ~DeviceReference() {};

    Device& GetDevice();
    dsid_t GetDSID() const;

    int GetFunctionID();
    bool IsSwitch();

    bool operator==(const DeviceReference& _other) const {
      return m_DSID == _other.m_DSID;
    }

    virtual void TurnOn();
    virtual void TurnOff();

    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);

    virtual void Enable();
    virtual void Disable();

    virtual bool IsOn();

    virtual void StartDim(const bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, const int _parameterNr = -1);
  };

  typedef vector<DeviceReference> DeviceVector;
  typedef DeviceVector::iterator DeviceIterator;

  /** Represents a dsID */
  class Device : public IDeviceInterface {
  private:
    string m_Name;
    dsid_t m_DSID;
    devid_t m_ShortAddress;
    int m_ModulatorID;
    Apartment* m_pApartment;
    bitset<63> m_GroupBitmask;
    vector<int> m_Groups;
    int m_SubscriptionEventID;
  public:
    Device(const dsid_t _dsid, Apartment* _pApartment);
    virtual ~Device() {};

    virtual void TurnOn();
    virtual void TurnOff();

    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);

    virtual void Enable();
    virtual void Disable();

    virtual bool IsOn();

    virtual void StartDim(const bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, const int _parameterNr = -1);
    double GetValue(const int _parameterNr = -1);

    int GetFunctionID();
    bool IsSwitch();

    string GetName() const;
    void SetName(const string& _name);

    bitset<63>& GetGroupBitmask();
    bool IsInGroup(const int _groupID) const;

    /** Returns the group id of the _index'th group */
    int GetGroupIdByIndex(const int _index) const;
    /** Returns _index'th group of the device */
    Group& GetGroupByIndex(const int _index);
    /** Returns the number of groups the device is a member of */
    int GetGroupsCount() const;

    /** Returns the short address of the device. This is the address
     * the device got from the dSM. */
    devid_t GetShortAddress() const;
    void SetShortAddress(const devid_t _shortAddress);
    /** Returns the DSID of the device */
    dsid_t GetDSID() const;
    /** Returns the id of the modulator the device is connected to */
    int GetModulatorID() const;
    void SetModulatorID(const int _modulatorID);
    /** Returns the apartment the device resides in. */
    Apartment& GetApartment() const;

    bool HasSubscription() const;
    int GetSubscriptionEventID() const;
    void SetSubscriptionEventID(const int _value);

    bool operator==(const Device& _other) const;
  };

  ostream& operator<<(ostream& out, const Device& _dt);

  /** Abstract interface to select certain Devices from a set */
  class IDeviceSelector {
  public:
    virtual bool SelectDevice(const Device& _device) const = 0;
    virtual ~IDeviceSelector() {};
  };

  /** Abstract interface to perform an Action on each device of a set */
  class IDeviceAction {
  public:
    virtual bool Perform(Device& _device) = 0;
    virtual ~IDeviceAction() {};
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
    Set(Device& _device);
    Set(DeviceReference& _reference);
    Set(DeviceVector _devices);
    virtual ~Set() {};

    virtual void TurnOn();
    virtual void TurnOff();

    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);

    virtual void Enable();
    virtual void Disable();

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, int _parameterNr = -1);
    //HashMapDeviceDouble GetValue(const int _parameterNr = -1);

    /** Performs the given action on all contained devices */
    void Perform(IDeviceAction& _deviceAction);

    /** Returns a subset selected by the given selector
     * A device will be included in the resulting set if _selector.SelectDevice(...) return true.
     */
    Set GetSubset(const IDeviceSelector& _selector);
    /** Returns a subset of the devices which are member of the given group
    * Note that these groups could be spanned over multiple modulators.
     */
    Set GetByGroup(int _groupNr);
    /** Returns a subset of the devices which are member of the given group
     * Note that these groups could be spanned over multiple modulators.
     */
    Set GetByGroup(const Group& _group);
    /** Returns a subset of the devices which are member of the given group
     * Note that these groups could be spanned over multiple modulators.
     */
    Set GetByGroup(const string& _name);
    /** Returns the device indicated by _name
     */
    DeviceReference GetByName(const string& _name);
    /** Returns the device indicated by _busid */
    DeviceReference GetByBusID(const devid_t _busid);

    /** Returns the device indicated by _dsid */
    DeviceReference GetByDSID(const dsid_t _dsid);

    /* Returns the number of devices contained in this set */
    int Length() const;
    /* Returns true if the set is empty */
    bool IsEmpty() const;

    /* Returns a set that's combined with the set _other.
     * Duplicates get filtered out.
     */
    Set Combine(Set& _other) const;
    /* Returns a set with all device in _other removed */
    Set Remove(Set& _other) const;

    /** Returns the _index'th device */
    DeviceReference& Get(int _index);
    DeviceReference& operator[](const int _index);

    /** Returns true if the set contains _device */
    bool Contains(const DeviceReference& _device) const;
    /** Returns true if the set contains _device */
    bool Contains(const Device& _device) const;

    /** Adds the device _device to the set */
    void AddDevice(const DeviceReference& _device);
    /** Adds the device _device to the set */
    void AddDevice(const Device& _device);

    /** Removes the device _device from the set */
    void RemoveDevice(const DeviceReference& _device);
    /** Removes the device _device from the set */
    void RemoveDevice(const Device& _device);
  }; // Set


  /** A class derived from DeviceContainer can deliver a Set of its Devices */
  class DeviceContainer {
  private:
    string m_Name;
  public:
    virtual Set GetDevices() const = 0;
    /** Returns a subset of the devices contained, selected by _selector */
    virtual Set GetDevices(const IDeviceSelector& _selector) const {
      return GetDevices().GetSubset(_selector);
    }

    virtual void SetName(const string& _name) { m_Name = _name; };
    string GetName() { return m_Name; };

    virtual ~DeviceContainer() {};
  }; // DeviceContainer

  /** Represents a Modulator */
  class Modulator : public DeviceContainer {
  private:
    dsid_t m_DSID;
    int m_BusID;
    DeviceVector m_ConnectedDevices;
  public:
    Modulator(const dsid_t _dsid);
    virtual ~Modulator() {};
    virtual Set GetDevices() const;

    dsid_t GetDSID() const;
    int GetBusID() const;
    void SetBusID(const int _busID);
  }; // Modulator

  /** Represents a predefined group */
  class Group : public DeviceContainer {
  protected:
    DeviceVector m_Devices;
    Apartment& m_Apartment;
    const int m_GroupID;
  public:
    Group(const int _id, Apartment& _apartment);
    virtual ~Group() {};
    virtual Set GetDevices() const;

    /** Returns the id of the group */
    int GetID() const;

    /** This function throws an error */
    virtual void AddDevice(const DeviceReference& _device);
    virtual void RemoveDevice(const DeviceReference& _device);
  }; // Group


  /** Represents a user-defined-group */
  class UserGroup : public Group {
  private:
  public:
    /** Adds a device to the group.
     * This will permanently add the device to the group.
     */
    virtual void AddDevice(const Device& _device);
    /** Removes a device from the group.
     * This will permanently remove the device from the group.
     */
    virtual void RemoveDevice(const Device& _device);
  }; // UserGroup

  /** Represents a Room
    */
  class Room : public DeviceContainer {
  private:
    int m_RoomID;
    DeviceVector m_Devices;
  public:
	virtual ~Room() {};
    virtual Set GetDevices() const;

    /** Adds a device to the room.
     * This will permanently add the device to the room.
     */
    void AddDevice(const DeviceReference& _device);
    /** Removes a device from the room.
     * This will permanently remove the device from the room.
     */
    void RemoveDevice(const DeviceReference& _device);

    /** Returns the group with the name _name */
    Group GetGroup(const string& _name);
    /** Returns the group with the id _id */
    Group GetGroup(const int _id);

    /** Adds a group to the room */
    void AddGroup(UserGroup& _group);
    /** Removes a group from the room */
    void RemoveGroup(UserGroup& _group);

    /** Returns the rooms id */
    int GetRoomID() const;
  }; // Room


  /** Arguments to be passed to an action / event */
  class Arguments {
  private:
    HashMapConstStringString m_ArgumentList;
  public:
    /** Returns true if the argument list contains a value named _name */
    bool HasValue(const string& _name) const;
    /** Returns the value of _name */
    string GetValue(const string& _name) const;
    /** Sets the value of _name to _value */
    void SetValue(const string& _name, const string& _value);
  };

  /** Abstract Action to be executed with a list of Arguments */
  class Action {
  private:
    string m_Name;
    string m_NameForUser;
    int m_ID;
  public:
    Action(const string& _name, const string& _nameForUser)
    : m_Name(_name), m_NameForUser(_nameForUser) {};

    virtual ~Action() {};

    /** Performs the action with _args */
    virtual void Perform(const Arguments& _args) = 0;

    const string& GetName() { return m_Name; };
    const string& GetNameForUser() { return m_NameForUser; };
  };

  /**
   * Event that's been raise by either a hardware or a software component.
   * Hardware-Events shall have unique id's where as user-defined software-events shall be above a certain number (e.g. 1024)
   */
  class Event {
  private:
    string m_Name;
    string m_NameForUser;
    int m_ID;
    devid_t m_Source;
    Arguments m_Arguments;
  public:
    Event(int _id, int _sourceID)
    : m_ID(_id), m_Source(_sourceID) {};

    /** Returns the id of the event */
    int GetID() const { return m_ID; };
    /** Returns the source of the event */
    devid_t GetSource() const { return m_Source; };
  };

  /** Subscription to one or many event-ids which may be restricted by one or many source-ids */
  class Subscription {
  private:
    const int m_ID;
    Arguments m_ActionArguments;
    Action& m_ActionToExecute;
    vector<int> m_SourceIDs;
    vector<int> m_EventIDs;
    string m_Name;
  public:
    Subscription(const int _id, Action& _action, Arguments& _actionArgs, vector<int> _eventIDs, vector<int> _sourceIDs)
    : m_ID(_id), m_ActionArguments(_actionArgs), m_ActionToExecute(_action), m_SourceIDs(_sourceIDs), m_EventIDs(_eventIDs) {};
    virtual ~Subscription() {};

    virtual void OnEvent(const Event& _event) { m_ActionToExecute.Perform(m_ActionArguments); };

    /** Returns the id of the subscription */
    int GetID() const { return m_ID; };

    const string& GetName() const;
    void SetName(const string& _value);

    const vector<int>& GetSourceIDs() const;
    const vector<int>& GetEventIDs() const;

    /** Returns true if the subscription is subscribed to the event */
    bool HandlesEvent(const Event& _event) const;
  };

  /** Represents an Apartment
    * This is the root of the datamodel of the dss. The Apartment is responsible for delivering
    * and loading all subitems.
    */
  class Apartment : public DeviceContainer,
                    public Thread
  {
  private:
    vector<Device*> m_StaleDevices;
    vector<Modulator*> m_StaleModulators;
    vector<Room*> m_StaleRooms;
    vector<Group*> m_StaleGroups;

    vector<Room*> m_Rooms;
    vector<Modulator*> m_Modulators;
    vector<Device*> m_Devices;
    vector<Subscription*> m_Subscriptions;
    vector<Group*> m_Groups;
    boost::ptr_vector<Action> m_Actions;
  private:
    int m_NextSubscriptionNumber;
  private:
    void LoadDevices(XMLNode& _node);
    void LoadModulators(XMLNode& _node);
    void LoadRooms(XMLNode& _node);
    Modulator& AllocateModulator(const dsid_t _dsid);
  public:
    Apartment();
    virtual ~Apartment();

    /** Returns a set containing all devices of the set */
    virtual Set GetDevices() const;

    /** Starts the event-processing */
    virtual void Execute();
    /** Loads the datamodel and marks the contained items as "stale" */
    void ReadConfigurationFromXML(const string& _fileName);

    /** Returns a reference to the device with the DSID _dsid */
    Device& GetDeviceByDSID(const dsid_t _dsid) const;
    /** Returns a reference to the device with the name _name*/
    Device& GetDeviceByName(const string& _name);

    /** Allocates a device and returns a reference to it.
     *  If there is a stale device with the same dsid, this device gets "activated"
     */
    Device& AllocateDevice(const dsid_t _dsid);

    /** Returns the Room by name */
    Room& GetRoom(const string& _roomName);
    /** Returns the Room by its id */
    Room& GetRoom(const int _id);
    /** Returns a vector of all rooms */
    vector<Room*>& GetRooms();

    /** Returns a Modulator by name */
    Modulator& GetModulator(const string& _modName);
    /** Returns a Modulator by DSID  */
    Modulator& GetModulatorByDSID(const dsid_t _dsid);
    /** Returns a Modulator by bus-id */
    Modulator& GetModulatorByBusID(const int _busID);
    /** Returns a vector of all modulators */
    vector<Modulator*>& GetModulators();

    /** Returns a Group by name */
    Group& GetGroup(const string& _name);
    /** Returns a Group by id */
    Group& GetGroup(const int _id);
    /** Returns a vector of all groups */
    vector<Group*>& GetGroups();

    /** Allocates a group */
    UserGroup& AllocateGroup(const int _id);

  public:
    void AddAction(Action* _action);
    Action& GetAction(const string& _name);

    /** Adds a subscription.
     * @param _action Action to be executed if a matching event gets raised
     * @param _eventIDs Ids to subscribe to
     * @_sourceIDs If non-empty only events from these ids will execute _action
     */
    Subscription& Subscribe(Action& _action, Arguments& _argsForAction, vector<int> _eventIDs, vector<int> _sourceIDs = vector<int>());
    /** Cancels a subscription for a event */
    void Unsubscribe(const int _subscriptionID);
    int GetSubscriptionCount();
    Subscription& GetSubscription(const int _index);

    /** Feeds an event to the processing-queue */
    void OnEvent(const Event& _event);

    void OnKeypress(const dsid_t& _dsid, const ButtonPressKind _kind, const int _number);
  }; // Apartment


  /** Combines an Event with a Schedule
    * These events get raised according to their Schedule
   */
  class ScheduledEvent {
  private:
    boost::shared_ptr<Event> m_Event;
    boost::shared_ptr<Schedule> m_Schedule;
    string m_Name;
  public:
    ScheduledEvent(boost::shared_ptr<Event> _evt, boost::shared_ptr<Schedule> _schedule)
    : m_Event(_evt), m_Schedule(_schedule) {};

    Event& GetEvent() const { return *m_Event; };
    Schedule& GetSchedule() const { return *m_Schedule; };
    const string& GetName() const { return m_Name; };
    void SetName(const string& _value) { m_Name = _value; };
  };


  //============================================= Helper definitions
  typedef bool (*DeviceSelectorFun)(const Device& _device);

  /** Device selector that works on simple function instead of classes */
  class DeviceSelector : public IDeviceSelector {
  private:
    DeviceSelectorFun m_SelectorFunction;
  public:
    DeviceSelector(DeviceSelectorFun& _selectorFun) : m_SelectorFunction(_selectorFun) {}
    virtual bool SelectDevice(const Device& _device) { return m_SelectorFunction(_device); }

    virtual ~DeviceSelector() {};
  }; // DeviceSelector

  /** Exception that will be thrown if a given item could not be found */
  class ItemNotFoundException : public DSSException {
  public:
    ItemNotFoundException(const string& _name) : DSSException(string("Could not find item ") + _name) {};
    ~ItemNotFoundException() throw() {};
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
  struct hash< const dss::Modulator* >  {
    size_t operator()( const dss::Modulator* x ) const  {
      return x->GetDSID();
    }
  };

}

//#endif // DOC
#endif // MODEL_H_INCLUDED
