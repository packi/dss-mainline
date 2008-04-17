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

#include "datetools.h"

#include <vector>
#include <string>

using namespace std;

namespace dss {

  typedef long devid_t;
  
  class Device;
  class Set;
  class DeviceContainer;
  
  typedef vector<Device> DeviceVector;
  
  /** Represents a dsID */
  class Device {
  private:
    string m_Name;
    devid_t m_ID;
  public:
    Device(devid_t _id);
    
    void TurnOn();
    void TurnOff();
    
    void IncreaseValue(const int _parameterNr = -1);
    void DecreaseValue(const int _parameterNr = -1);
    
    void Enable();
    void Disable();
    
    void StartDim(const bool _directionUp, const int _parameterNr = -1);
    void EndDim(const int _parameterNr = -1);
    void SetValue(const double _value, const int _parameterNr = -1);
    double GetValue(const int _parameterNr = -1);
    
    string GetName();
    void SetName(const string& _name);
  };
  
  /** Abstract interface to select certain Devices from a set */
  class IDeviceSelector {
  public:
    virtual bool SelectDevice(const Device& _device) const = 0;
  };
  
  /** Abstract interface to perform an Action on each device of a set */
  class IDeviceAction {
  public:
    virtual bool Perform(const Device& _device) = 0;
  };
  
  /** A set holds an arbitrary list of devices.
    * A Command sent to an instance of this class will replicate the command to all
    * contained devices.
   */
  class Set {
  private:
    DeviceVector m_ContainedDevices;
  public:
    Set(DeviceVector _devices);
    
    void TurnOn();
    void TurnOff();
    
    void IncreaseValue(const int _parameterNr = -1);
    void DecreaseValue(const int _parameterNr = -1);
    
    void Enable();
    void Disable();
    
    void StartDim(bool _directionUp, const int _parameterNr = -1);
    void EndDim(const int _parameterNr = -1);
    void SetValue(const double _value, int _parameterNr = -1);
    //HashMapDeviceDouble GetValue(const int _parameterNr = -1);
    
    void Perform(const IDeviceAction& _deviceAction);
    
    Set GetSubset(const IDeviceSelector& _selector); 
    Set GetByGroup(int _groupNr);
    Device GetByName(const string& _name);
    Device GetByID(const int _id);
    
    int Length() const;
    
    Set Combine(Set& _other) const;
    Set Remove(Set& _other) const;
    
    void AddDevice(Device& _device);
    void RemoveDevice(Device& _device);
  }; // Set
  
  
  /** A class derived from DeviceContainer can deliver a Set of its Devices */
  class DeviceContainer {
  public:
    virtual Set GetDevices() = 0;
    virtual Set GetDevices(const IDeviceSelector& _selector) {
      return GetDevices().GetSubset(_selector);
    }
  }; // DeviceContainer
  
  /** Represents a Modulator */
  class Modulator : public DeviceContainer {
  private:
    int m_LocalID;
    vector<Device> m_ConnectedDevices;
  public:
    virtual Set GetDevices();

    void SendDS485Frame(const char* _frame, int _len);
  }; // Modulator
  
  /** Represents a predefined group */
  class Group : public DeviceContainer {
  protected:
    int m_GroupID;
    vector<Device> m_Devices;
  public:
    virtual Set GetDevices();

    virtual void AddDevice(const Device& _device) { /* do nothing or throw? */ };
    virtual void RemoveDevice(const Device& _device) { /* do nothing or throw? */ };
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
    vector<Device> m_Devices;
  public:
    virtual Set GetDevices();
    
    void AddDevice(const Device& _device);
    void RemoveDevice(const Device& _device);
    
    Group GetGroup(const string& _name);
    Group GetGroup(const int _id);
    
    void AddGroup(UserGroup& _group);
    void RemoveGroup(UserGroup& _group);
  }; // Room

  
  /** Arguments to be passed to an argument / event */
  class Arguments {
  public:
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
    Action();
    
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
    Event(int _id);
  };
  
  /** Subscription to one or many event-ids which may be restricted by one or many source-ids */
  class Subscription {
  private:
    vector<int> m_SourceIDs;
    vector<int> m_EventIDs;
    Action& m_ActionToExecute;
    Arguments m_ActionArguments;
  public:
    virtual void OnEvent(const Event& _event) = 0;
  };
  
  /** Represents an Apartment 
    * This is the root of the datamodel of the dss. The Apartment is responsible for delivering
    * and loading all subitems.
    */
  class Apartment : public DeviceContainer {
  private:
    vector<Room> m_Rooms;
    vector<Modulator> m_Modulators;
    vector<Device> m_Devices;
    vector<Subscription> m_Subscriptions;
  public:
    virtual Set GetDevices();
    
    // Room queries
    Room GetRoom(const string& _roomName);
    Room GetRoom(const int _id);
    vector<Room> GetRooms();
    
    // Modulator queries
    Modulator GetModulator(const string& _modName);
    Modulator GetModulator(const int _id);
    vector<Modulator> GetModulators();
    
    // Group queries
    Group GetGroup(const string& _name);
    Group GetGroup(const int _id);
    vector<Group> GetGroups();
    
    // Event stuff
    void Subscribe(const Subscription& _subscription);
    void Unsubscribe(const Subscription& _subscription);
    
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
  
}
