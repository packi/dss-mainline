/*
 *  model.cpp
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

#include "model.h"
#include "dss.h"

namespace dss {
  
  //================================================== DeviceReference
  
  
  DeviceReference::DeviceReference(const DeviceReference& _copy) 
  : m_DeviceID(_copy.m_DeviceID),
    m_Apartment(_copy.m_Apartment)
  {
  }
  
  DeviceReference::DeviceReference(const devid_t _deviceID, const Apartment& _apartment) 
  : m_DeviceID(_deviceID),
    m_Apartment(&_apartment)
  {
  } // ctor
  
  DeviceReference::DeviceReference(const Device& _device, const Apartment& _apartment) 
  : m_DeviceID(_device.GetID()),
    m_Apartment(&_apartment)
  {
  } // ctor
  
  Device& DeviceReference::GetDevice() {
    return m_Apartment->GetDeviceByID(m_DeviceID);
  } // GetDevice$
  
  //================================================== Device
  
  Device::Device(devid_t _id, Apartment* _pApartment)
  : m_ID(_id),
    m_pApartment(_pApartment)
  {
  }
  
  void Device::TurnOn() {
//    DSS::GetInstance()->GetDS485Proxy().TurnOn( *this );
  } // TurnOn
  
  void Device::TurnOff() {
//    DSS::GetInstance()->GetDS485Proxy().TurnOff( *this );
  } // TurnOff
  
  void Device::IncreaseValue(const int _parameterNr) {
  } // IncreaseValue
  
  void Device::DecreaseValue(const int _parameterNr) {
  } // DecreaseValue
  
  void Device::Enable() {
  } // Enable
  
  void Device::Disable() {
  } // Disable
  
  void Device::StartDim(const bool _directionUp, const int _parameterNr) {
  } // StartDim
  
  void Device::EndDim(const int _parameterNr) {
  } // EndDim
  
  void Device::SetValue(const double _value, const int _parameterNr) {
  } // SetValue
  
  double Device::GetValue(const int _parameterNr) {
    return -1;
  } // GetValue
  
  string Device::GetName() const {
    return m_Name;
  } // GetName
  
  void Device::SetName(const string& _name) {
    m_Name = _name;
  } // SetName
  
  bool Device::operator==(const Device& _other) const {
    return _other.m_ID == m_ID;
  } // operator==
  
  devid_t Device::GetID() const {
    return m_ID;
  } // GetID
  
  long long Device::GetGroupBitmask() const {
    return m_GroupBitmask;
  } // GetGroupBitmask
  
  Apartment& Device::GetApartment() const {
    return *m_pApartment;
  }
  
  //================================================== Set
  
  Set::Set() {
  } // ctor
  
  Set::Set(Device& _device) {
    m_ContainedDevices.push_back(DeviceReference(_device, _device.GetApartment()));
  } // ctor(Device)
  
  Set::Set(DeviceVector _devices) {
    m_ContainedDevices = _devices;
  } // ctor(DeviceVector)
  
  void Set::TurnOn() {
  } // TurnOn
  
  void Set::TurnOff() {
  } // TurnOff
  
  void Set::IncreaseValue(const int _parameterNr) {
  } // IncreaseValue
  
  void Set::DecreaseValue(const int _parameterNr) {
  } // DecreaseValue
  
  void Set::Enable() {
  } // Enable
  
  void Set::Disable() {
  } // Disable
  
  void Set::StartDim(bool _directionUp, const int _parameterNr) {
  } // StartDim
  
  void Set::EndDim(const int _parameterNr) {
  } // EndDim
  
  void Set::SetValue(const double _value, int _parameterNr) {
  } // SetValue
  //HashMapDeviceDouble Set::GetValue(const int _parameterNr);
  
  void Set::Perform(IDeviceAction& _deviceAction) {
    for(DeviceIterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      _deviceAction.Perform(iDevice->GetDevice());
    }
  } // Perform
  
  Set Set::GetSubset(const IDeviceSelector& _selector) {
    Set result;
    for(DeviceIterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      if(_selector.SelectDevice(iDevice->GetDevice())) {
        result.AddDevice(*iDevice);
      }
    }
    return result;
  } // GetSubset
  
  class ByGroupSelector : public IDeviceSelector {
  private:
    const int m_GroupNr;
    const long long m_GroupMask;
  public:
    ByGroupSelector(const int _groupNr)
    : m_GroupNr(_groupNr),
      m_GroupMask(1 << _groupNr)
    {}
    
    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetGroupBitmask() & m_GroupMask == m_GroupMask;
    }
  };
  
  Set Set::GetByGroup(int _groupNr) {
    return GetSubset(ByGroupSelector(_groupNr));
  } // GetByGroup
  
  class ByNameSelector : public IDeviceSelector {
  private:
    const string m_Name;
  public:
    ByNameSelector(const string& _name) : m_Name(_name) {};
    
    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetName() == m_Name;
    }
  };
  
  DeviceReference Set::GetByName(const string& _name) {
    Set resultSet = GetSubset(ByNameSelector(_name));
    if(resultSet.Length() == 0) {
      throw ItemNotFoundException(_name);
    }
    return resultSet.m_ContainedDevices.front();
  } // GetByName
  
  
  class ByIDSelector : public IDeviceSelector {
  private:
    const int m_ID;
  public:
    ByIDSelector(const int _id) : m_ID(_id) {}
    
    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetID() == m_ID;
    }
  };
  
  DeviceReference Set::GetByID(const int _id) {
    Set resultSet = GetSubset(ByIDSelector(_id));
    if(resultSet.Length() == 0) {
      throw ItemNotFoundException(string("with id ") + IntToString(_id));
    }
    return resultSet.m_ContainedDevices.front();
  } // GetByID
  
  int Set::Length() const {
    return m_ContainedDevices.size();
  } // Length
  
  bool Set::IsEmpty() const {
    return Length() == 0;
  }
  
  Set Set::Combine(Set& _other) const {
    Set resultSet(_other);
    for(DeviceVector::const_iterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      if(!resultSet.Contains(*iDevice)) {
        resultSet.AddDevice(*iDevice);
      }
    }
    return resultSet;
  } // Combine
  
  Set Set::Remove(Set& _other) const {
    Set resultSet(*this);
    for(DeviceIterator iDevice = _other.m_ContainedDevices.begin(); iDevice != _other.m_ContainedDevices.end(); ++iDevice) {
      resultSet.RemoveDevice(*iDevice);
    }    
    return resultSet;
  } // Remove
  
  bool Set::Contains(const DeviceReference& _device) const {
    DeviceVector::const_iterator pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    return pos != m_ContainedDevices.end();
  } // Contains
  
  void Set::AddDevice(const DeviceReference& _device) {
    if(!Contains(_device)) {
      m_ContainedDevices.push_back(_device);
    }
  } // AddDevice
  
  void Set::AddDevice(const Device& _device) {
    AddDevice(DeviceReference(_device, _device.GetApartment()));
  } // AddDevice
  
  void Set::RemoveDevice(const DeviceReference& _device) {
    DeviceVector::iterator pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    if(pos != m_ContainedDevices.end()) {
      m_ContainedDevices.erase(pos);
    }
  } // RemoveDevice
  
  void Set::RemoveDevice(const Device& _device) {
    RemoveDevice(DeviceReference(_device, _device.GetApartment()));
  } // AddDevice
  
  DeviceReference& Set::Get(int _index) {
    return m_ContainedDevices.at(_index);
  } // Get
  
  DeviceReference& Set::operator[](const int _index) {
    return Get(_index);
  } // operator[]
  
  
  ostream& operator<<(ostream& out, const Device& _dt) {
    out << "Device ID " << _dt.GetID();
    if(_dt.GetName().size() > 0) {
      out << " name: " << _dt.GetName();
    }
    return out;
  }
 
  //================================================== Arguments
  
  bool Arguments::HasValue(const string& _name) {
    return m_ArgumentList.find(_name) != m_ArgumentList.end();
  } // HasValue
  
  string Arguments::GetValue(const string& _name) {
    if(HasValue(_name)) {
      return m_ArgumentList[_name];
    }
    throw new ItemNotFoundException(_name);
  } // GetValue
  
  void Arguments::SetValue(const string& _name, const string& _value) {
  } // SetValue

  
  //================================================== Apartment
  
  Apartment::Apartment() 
  : m_NextSubscriptionNumber(1)
  {  
    Group* grp = new Group(0, *this);
    grp->SetName("black");
    m_Groups.push_back(grp);
    grp = new Group(1, *this);
    grp->SetName("blue");
    m_Groups.push_back(grp);
    grp = new Group(2, *this);
    grp->SetName("green");
    m_Groups.push_back(grp);
    grp = new Group(3, *this);
    grp->SetName("cyan");
    m_Groups.push_back(grp);
    grp = new Group(4, *this);
    grp->SetName("red");
    m_Groups.push_back(grp);
    grp = new Group(5, *this);
    grp->SetName("violet");
    m_Groups.push_back(grp);
    grp = new Group(6, *this);
    grp->SetName("yellow");
    m_Groups.push_back(grp);
    grp = new Group(7, *this);
    grp->SetName("gray");
    m_Groups.push_back(grp);
  } // ctor
  
  Apartment::~Apartment() {
    while(!m_Subscriptions.empty()) {
      Subscription* elem = *m_Subscriptions.begin();
      m_Subscriptions.erase(m_Subscriptions.begin());
      delete elem;
    }
    while(!m_Devices.empty()) {
      Device* dev = *m_Devices.begin();
      m_Devices.erase(m_Devices.begin());
      delete dev;
    }
    while(!m_Groups.empty()) {
      Group* grp = *m_Groups.begin();
      m_Groups.erase(m_Groups.begin());
      delete grp;
    }
    while(!m_Rooms.empty()) {
      Room* room = *m_Rooms.begin();
      m_Rooms.erase(m_Rooms.begin());
      delete room;
    }
    while(!m_Modulators.empty()) {
      Modulator* mod = *m_Modulators.begin();
      m_Modulators.erase(m_Modulators.begin());
      delete mod;
    }
  } // dtor
  
  Device& Apartment::GetDeviceByID(const devid_t _id) const {
    for(vector<Device*>::const_iterator ipDevice = m_Devices.begin(); ipDevice != m_Devices.end(); ++ipDevice) {
      if((*ipDevice)->GetID() == _id) {
        return **ipDevice;
      }
    }
    throw new ItemNotFoundException(IntToString(_id));
  } // GetDeviceByID
  
  Set Apartment::GetDevices() {
    DeviceVector devs;
    for(vector<Device*>::iterator ipDevice = m_Devices.begin(); ipDevice != m_Devices.end(); ++ipDevice) {
      devs.push_back(DeviceReference(**ipDevice, *this));
    }
    
    return Set(devs);
  } // GetDevices
  
  Room& Apartment::GetRoom(const string& _roomName) {
    for(vector<Room*>::iterator iRoom = m_Rooms.begin(); iRoom != m_Rooms.end(); ++iRoom) {
      if((*iRoom)->GetName() == _roomName) {
        return **iRoom;
      }
    }
    throw new ItemNotFoundException(_roomName);
  } // GetRoom(name)
  
  Room& Apartment::GetRoom(const int _id) {
    for(vector<Room*>::iterator iRoom = m_Rooms.begin(); iRoom != m_Rooms.end(); ++iRoom) {
      if((*iRoom)->GetRoomID() == _id) {
        return **iRoom;
      }
    }
    throw new ItemNotFoundException(IntToString(_id));
  } // GetRoom(id)
  
  vector<Room*>& Apartment::GetRooms() {
    return m_Rooms;
  } // GetRooms
  
  Modulator& Apartment::GetModulator(const string& _modName) {
    for(vector<Modulator*>::iterator iModulator = m_Modulators.begin(); iModulator != m_Modulators.end(); ++iModulator) {
      if((*iModulator)->GetName() == _modName) {
        return **iModulator;
      }
    }
    throw new ItemNotFoundException(_modName);
  } // GetModulator(name)
    
  Modulator& Apartment::GetModulator(const int _id) {
    for(vector<Modulator*>::iterator iModulator = m_Modulators.begin(); iModulator != m_Modulators.end(); ++iModulator) {
      if((*iModulator)->GetID() == _id) {
        return **iModulator;
      }
    }
    throw new ItemNotFoundException(IntToString(_id));
  } // GetModulator(id)
  
  vector<Modulator*>& Apartment::GetModulators() {
    return m_Modulators;
  } // GetModulators
  
  // Group queries
  Group& Apartment::GetGroup(const string& _name) {
    for(vector<Group*>::iterator ipGroup = m_Groups.begin(); ipGroup != m_Groups.end(); ++ipGroup) {
      if((*ipGroup)->GetName() == _name) {
        return **ipGroup;
      }
    }
    throw new ItemNotFoundException(_name);
  } // GetGroup(name)
  
  Group& Apartment::GetGroup(const int _id) {
    for(vector<Group*>::iterator ipGroup = m_Groups.begin(); ipGroup != m_Groups.end(); ++ipGroup) {
      if((*ipGroup)->GetID() == _id) {
        return **ipGroup;
      }
    }
    throw new ItemNotFoundException(IntToString(_id));
  } // GetGroup(id)
  vector<Group*>& Apartment::GetGroups() {
    return m_Groups;
  } // GetGroups
    
  Subscription& Apartment::Subscribe(Action& _action, vector<int> _eventIDs, vector<int> _sourceIDs) {
    Subscription* pResult = new Subscription(m_NextSubscriptionNumber++, _action, _eventIDs, _sourceIDs);
    m_Subscriptions.push_back(pResult);
    return *pResult;
  } // Subscribe
  
  class BySubscriptionID {
  private:
    const int m_ID;
  public:
    BySubscriptionID(const int _id) : m_ID(_id) {}
    bool operator()(const Subscription* _other) {
      return m_ID == _other->GetID();
    }
  };
    
  void Apartment::Unsubscribe(const int _subscriptionID) {
    vector<Subscription*>::iterator pos = find_if(m_Subscriptions.begin(), m_Subscriptions.end(), BySubscriptionID(_subscriptionID));
    if(pos != m_Subscriptions.end()) {
      Subscription* elem = *pos;
      m_Subscriptions.erase(pos);
      delete elem;
    }
  } // Unsubscribe
  

  struct handle_event : public unary_function<Subscription*, void>
  {
    handle_event(const Event& _event) : m_Event(_event) {};
    void operator()(Subscription* _subscription) {
      if(_subscription->HandlesEvent(m_Event)) {
        _subscription->OnEvent(m_Event);
      }
    }
    const Event& m_Event;
  };
  
  void Apartment::OnEvent(const Event& _event) {
    for_each(m_Subscriptions.begin(), m_Subscriptions.end(), handle_event(_event));
  } // OnEvent
  
  Device& Apartment::AllocateDevice(const devid_t _id) {
    Device* pResult = new Device(_id, this);
    m_Devices.push_back(pResult);
    return *pResult;
  } 
  
  //================================================== Modulator
  
  Set Modulator::GetDevices() {
    return m_ConnectedDevices;
  } // GetDevices
  
  int Modulator::GetID() const {
    return m_LocalID;
  } // GetID
  
  //================================================== Room
  
  Set Room::GetDevices() {
    return Set(m_Devices);
  } // GetDevices
  
  void Room::AddDevice(const DeviceReference& _device) {
    m_Devices.push_back(_device);
  } // AddDevice
  
  void Room::RemoveDevice(const DeviceReference& _device) {
    DeviceIterator pos = find(m_Devices.begin(), m_Devices.end(), _device);
    if(pos != m_Devices.end()) {
      m_Devices.erase(pos);
    }
  } // RemoveDevice
  
  Group GetGroup(const string& _name);
  Group GetGroup(const int _id);
  
  void AddGroup(UserGroup& _group);
  void RemoveGroup(UserGroup& _group);
  
  int Room::GetRoomID() const {
    return m_RoomID;
  }
  
  //============================================= Group

  Group::Group(const int _id, Apartment& _apartment) 
  : m_Apartment(_apartment),
    m_GroupID(_id)
  {
  }

  int Group::GetID() const {
    return m_GroupID;
  } // GetID
  
  Set Group::GetDevices() {
    return m_Apartment.GetDevices().GetByGroup(m_GroupID);
  }
  
  void Group::AddDevice(const DeviceReference& _device) { /* do nothing or throw? */ };
  void Group::RemoveDevice(const DeviceReference& _device) { /* do nothing or throw? */ };

  
  //============================================= Subscription
  
  bool Subscription::HandlesEvent(const Event& _event) const {
    if(Contains(m_EventIDs, _event.GetID())) {
      return m_SourceIDs.empty() || Contains(m_SourceIDs, static_cast<int>(_event.GetSource()));
    }
    return false;
  } // HandlesEvent
  
}