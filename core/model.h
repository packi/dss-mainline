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

#include "base.h"
#include "datetools.h"
#include "ds485types.h"
#include "xmlwrapper.h"

#include <vector>
#include <string>

using namespace std;

namespace dss {
  
  class Device;
  class Set;
  class DeviceContainer;
  class Apartment;
  
  class IDeviceInterface {
  public:
    virtual void TurnOn() = 0;
    virtual void TurnOff() = 0;
    
    virtual void IncreaseValue(const int _parameterNr = -1) = 0;
    virtual void DecreaseValue(const int _parameterNr = -1) = 0;
    
    virtual void Enable() = 0;
    virtual void Disable() = 0;
    
    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void EndDim(const int _parameterNr = -1)= 0;
    virtual void SetValue(const double _value, int _parameterNr = -1) = 0;
  };
  
  class DeviceReference : public IDeviceInterface {
  private:
    devid_t m_DeviceID;
    const Apartment* m_Apartment;
  public:
    DeviceReference(const DeviceReference& _copy);
    DeviceReference(const devid_t _deviceID, const Apartment& _apartment);
    DeviceReference(const Device& _device, const Apartment& _apartment);
    
    Device& GetDevice();
    devid_t GetID() const;
    
    bool operator==(const DeviceReference& _other) const {
      return m_DeviceID == _other.m_DeviceID;
    }
    
    virtual void TurnOn();
    virtual void TurnOff();
    
    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);
    
    virtual void Enable();
    virtual void Disable();
    
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
    devid_t m_ID;
    Apartment* m_pApartment;
    long long m_GroupBitmask;
  public:
    Device(devid_t _id, Apartment* _pApartment);
    
    virtual void TurnOn();
    virtual void TurnOff();
    
    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);
    
    virtual void Enable();
    virtual void Disable();
    
    virtual void StartDim(const bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, const int _parameterNr = -1);
    double GetValue(const int _parameterNr = -1);
    
    string GetName() const;
    void SetName(const string& _name);
    
    long long GetGroupBitmask() const;
    
    devid_t GetID() const;
    Apartment& GetApartment() const;
    
    bool operator==(const Device& _other) const;
  };
  
  ostream& operator<<(ostream& out, const Device& _dt);
  
  /** Abstract interface to select certain Devices from a set */
  class IDeviceSelector {
  public:
    virtual bool SelectDevice(const Device& _device) const = 0;
  };
  
  /** Abstract interface to perform an Action on each device of a set */
  class IDeviceAction {
  public:
    virtual bool Perform(Device& _device) = 0;
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
    
    void Perform(IDeviceAction& _deviceAction);
    
    Set GetSubset(const IDeviceSelector& _selector); 
    Set GetByGroup(int _groupNr);
    DeviceReference GetByName(const string& _name);
    DeviceReference GetByID(const int _id);
    
    int Length() const;
    bool IsEmpty() const;
    
    Set Combine(Set& _other) const;
    Set Remove(Set& _other) const;
    
    DeviceReference& Get(int _index);
    
    DeviceReference& operator[](const int _index);
    
    bool Contains(const DeviceReference& _device) const;
    
    void AddDevice(const DeviceReference& _device);
    void AddDevice(const Device& _device);
    
    void RemoveDevice(const DeviceReference& _device);
    void RemoveDevice(const Device& _device);
  }; // Set
  
  
  /** A class derived from DeviceContainer can deliver a Set of its Devices */
  class DeviceContainer {
  private:
    string m_Name;
  public:
    virtual Set GetDevices() = 0;
    virtual Set GetDevices(const IDeviceSelector& _selector) {
      return GetDevices().GetSubset(_selector);
    }
    
    virtual void SetName(const string& _name) { m_Name = _name; };
    string GetName() { return m_Name; };
  }; // DeviceContainer
  
  /** Represents a Modulator */
  class Modulator : public DeviceContainer {
  private:
    int m_LocalID;
    DeviceVector m_ConnectedDevices;
  public:
    Modulator(const int _id);
    virtual Set GetDevices();

  //  void SendDS485Frame(const char* _frame, int _len);
    
    int GetID() const;
  }; // Modulator
  
  /** Represents a predefined group */
  class Group : public DeviceContainer {
  protected:
    const int m_GroupID;
    DeviceVector m_Devices;
    Apartment& m_Apartment;
  public:
    Group(const int _id, Apartment& _apartment);
    virtual ~Group() {};
    virtual Set GetDevices();
    
    int GetID() const;

    virtual void AddDevice(const DeviceReference& _device);
    virtual void RemoveDevice(const DeviceReference& _device);
  }; // Group
  
  
  /** Represents a user-defined-group */
  class UserGroup : public Group {
  private:
  public:
    virtual void AddDevice(const Device& _device);
    virtual void RemoveDevice(const Device& _device);
  }; // UserGroup
  
  class Room : public DeviceContainer {
  private:
    int m_RoomID;
    DeviceVector m_Devices;
  public:
    virtual Set GetDevices();
    
    void AddDevice(const DeviceReference& _device);
    void RemoveDevice(const DeviceReference& _device);
    
    Group GetGroup(const string& _name);
    Group GetGroup(const int _id);
    
    void AddGroup(UserGroup& _group);
    void RemoveGroup(UserGroup& _group);
    
    int GetRoomID() const;
  }; // Room

  
  /** Arguments to be passed to an action / event */
  class Arguments {
  private:
    HashMapConstStringString m_ArgumentList;
  public:
    bool HasValue(const string& _name);
    string GetValue(const string& _name);
    void SetValue(const string& _name, const string& _value);
  };
  
  /** Abstract Action to be executed with a list of Arguments */
  class Action {
  private:
    string m_Name;
    string m_NameForUser;
    int m_ID;
  public:
    virtual void Perform(Arguments& _args) = 0;
  };
  
  /** Event that's been raise by either a hardware or a software component.
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
    Event(int _id) : m_ID(_id) {};
    
    int GetID() const { return m_ID; };
    devid_t GetSource() const { return m_ID; };
  };
  
  /** Subscription to one or many event-ids which may be restricted by one or many source-ids */
  class Subscription {
  private:
    vector<int> m_SourceIDs;
    vector<int> m_EventIDs;
    Action& m_ActionToExecute;
    Arguments m_ActionArguments;
    int m_ID;
  public:
    Subscription(const int _id, Action& _action, vector<int> _eventIDs, vector<int> _sourceIDs)
    : m_ID(_id), m_ActionToExecute(_action), m_SourceIDs(_sourceIDs), m_EventIDs(_eventIDs) {};
    
    virtual void OnEvent(const Event& _event) { m_ActionToExecute.Perform(m_ActionArguments); };
    
    int GetID() const { return m_ID; };
    
    bool HandlesEvent(const Event& _event) const;
  };
  
  /** Represents an Apartment 
    * This is the root of the datamodel of the dss. The Apartment is responsible for delivering
    * and loading all subitems.
    */
  class Apartment : public DeviceContainer {
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
  private:
    int m_NextSubscriptionNumber;
  private:
    void LoadDevices(XMLNode& _node);
    void LoadModulators(XMLNode& _node);
    void LoadRooms(XMLNode& _node);
  public:
    Apartment();
    ~Apartment();
    
    virtual Set GetDevices();
    
    virtual void Run();
    void ReadConfigurationFromXML(const string& _fileName);
    
    Device& GetDeviceByID(const devid_t _id) const;
    Device& AllocateDevice(const devid_t _id);
    
    // Room queries
    Room& GetRoom(const string& _roomName);
    Room& GetRoom(const int _id);
    vector<Room*>& GetRooms();
    
    // Modulator queries
    Modulator& GetModulator(const string& _modName);
    Modulator& GetModulator(const int _id);
    vector<Modulator*>& GetModulators();
    
    // Group queries
    Group& GetGroup(const string& _name);
    Group& GetGroup(const int _id);
    vector<Group*>& GetGroups();
    
    UserGroup& AllocateGroup(const int _id);
    
    // Event stuff
    Subscription& Subscribe(Action& _action, vector<int> _eventIDs, vector<int> _sourceIDs = vector<int>());
    void Unsubscribe(const int _subscriptionID);
    
    void OnEvent(const Event& _event);
  }; // Apartment
  

  /** Combines an Event with a Schedule
    * These events get raised according to their Schedule
   */
  class ScheduledEvent {
  private:
    Event& m_Event;
    Schedule& m_Schedule;
  public:
    ScheduledEvent(Event& _evt, Schedule& _schedule);
  };

  
  //============================================= Helper definitions
  typedef bool (*DeviceSelectorFun)(const Device& _device); 
  
  class DeviceSelector : public IDeviceSelector {
  private:
    DeviceSelectorFun m_SelectorFunction;
  public:
    DeviceSelector(DeviceSelectorFun& _selectorFun) : m_SelectorFunction(_selectorFun) {}
    virtual bool SelectDevice(const Device& _device) { return m_SelectorFunction(_device); }    
  }; // DeviceSelector  
  
  class ItemNotFoundException : public DSSException {
  public:
    ItemNotFoundException(const string& _name) : DSSException(string("Could not find item ") + _name) {};
    ~ItemNotFoundException() throw() {};
  };
  
}

#endif
