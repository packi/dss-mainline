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
#include "logger.h"

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
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdTurnOn, *this);
  } // TurnOn
  
  void Device::TurnOff() {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdTurnOff, *this);
  } // TurnOff
  
  void Device::IncreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdIncreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdIncreaseParam, *this);
    }
  } // IncreaseValue
  
  void Device::DecreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdDecreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdDecreaseParam, *this);
    }
  } // DecreaseValue
  
  void Device::Enable() {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdEnable, *this);
  } // Enable
  
  void Device::Disable() {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdDisable, *this);
  } // Disable
  
  void Device::StartDim(const bool _directionUp, const int _parameterNr) {
    if(_directionUp) {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdStartDimUp, *this);
    } else {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdStartDimDown, *this);
    }
  } // StartDim
  
  void Device::EndDim(const int _parameterNr) {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdStopDim, *this);
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
  
  int Device::GetModulatorID() const {
    return m_ModulatorID;
  } // GetModulatorID
  
  int Device:: GetGroupIdByIndex(const int _index) const {
    return m_Groups[_index];
  } // GetGroupIdByIndex
  
  Group& Device::GetGroupByIndex(const int _index) {
    return m_pApartment->GetGroup(GetGroupIdByIndex(_index));
  } // GetGroupByIndex
  
  int Device::GetGroupsCount() const {
    return m_Groups.size();
  } // GetGroupsCount
  
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
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdTurnOn, *this);
  } // TurnOn
  
  void Set::TurnOff() {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdTurnOff, *this);
  } // TurnOff
  
  void Set::IncreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdIncreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdIncreaseParam, *this);
    }
  } // IncreaseValue
  
  void Set::DecreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdDecreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdDecreaseParam, *this);
    }
  } // DecreaseValue
  
  void Set::Enable() {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdEnable, *this);
  } // Enable
  
  void Set::Disable() {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdDisable, *this);
  } // Disable
  
  void Set::StartDim(bool _directionUp, const int _parameterNr) {
    if(_directionUp) {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdStartDimUp, *this);
    } else {
      DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdStartDimDown, *this);
    }
  } // StartDim
  
  void Set::EndDim(const int _parameterNr) {
    DSS::GetInstance()->GetDS485Proxy().SendCommand(cmdStopDim, *this);
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
  
  Set Set::GetByGroup(const Group& _group) {
    return GetByGroup(_group.GetID());
  }
  
  Set Set::GetByGroup(const string& _name) {
    Set result;
    if(IsEmpty()) {
      return result;
    } else {
      Group& g = Get(0).GetDevice().GetApartment().GetGroup(_name);
      return GetByGroup(g.GetID());
    }
  }
  
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
  
  bool Set::Contains(const Device& _device) const {
    return Contains(DeviceReference(_device, _device.GetApartment()));
  }
  
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
    grp->SetName("yellow");
    m_Groups.push_back(grp);
    grp = new Group(1, *this);
    grp->SetName("gray");
    m_Groups.push_back(grp);
    grp = new Group(2, *this);
    grp->SetName("blue");
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
    grp->SetName("green");
    m_Groups.push_back(grp);
    grp = new Group(7, *this);
    grp->SetName("black");
    m_Groups.push_back(grp);
    grp = new Group(7, *this);
    grp->SetName("white");
    m_Groups.push_back(grp);
  } // ctor

  // TODO: move to base.h
  template<class t>
  void ScrubVector(vector<t*>& _vector) {
    while(!_vector.empty()) {
      t* elem = *_vector.begin();
      _vector.erase(_vector.begin());
      delete elem;
    }
  } // ScrubVector
  
  Apartment::~Apartment() {
    ScrubVector(m_Subscriptions);
    ScrubVector(m_Devices);
    ScrubVector(m_Groups);
    ScrubVector(m_Rooms);
    ScrubVector(m_Modulators);
    
    ScrubVector(m_StaleDevices);
    ScrubVector(m_StaleGroups);
    ScrubVector(m_StaleModulators);
    ScrubVector(m_StaleRooms);
  } // dtor
  
  void Apartment::Run() {
    // Load devices/modulators/etc. from a config-file
    string configFileName = DSS::GetInstance()->GetConfig().GetOptionAs<string>("apartment_config", "/Users/packi/sources/dss/trunk/data/apartment.xml");
    if(!FileExists(configFileName)) {
      Logger::GetInstance()->Log(string("Could not open config-file for apartment: '") + configFileName + "'", lsWarning);
    } else {
      ReadConfigurationFromXML(configFileName);
    }
    
  } // Run
    
  void Apartment::ReadConfigurationFromXML(const string& _fileName) {
    const int apartmentConfigVersion = 1;
    XMLDocumentFileReader reader(_fileName);
    
    XMLNode rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == "config") {
      if(StrToInt(rootNode.GetAttributes()["version"]) == apartmentConfigVersion) {
        XMLNodeList nodes = rootNode.GetChildren();
        for(XMLNodeList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode) {
          string nodeName = iNode->GetName();
          if(nodeName == "devices") {
            LoadDevices(*iNode);
          } else if(nodeName == "modulators") {
            LoadModulators(*iNode);
          } else if(nodeName == "rooms") {
            LoadRooms(*iNode);
          }
        }
      } else {
        Logger::GetInstance()->Log("Log file has the wrong version");
      }
    }
  } // ReadConfigurationFromXML
  
  void Apartment::LoadDevices(XMLNode& _node) {
    XMLNodeList devices = _node.GetChildren();
    for(XMLNodeList::iterator iNode = devices.begin(); iNode != devices.end(); ++iNode) {
      if(iNode->GetName() == "device") {
        int id = StrToInt(iNode->GetAttributes()["id"]);
        string name;
        try {
          XMLNode& nameNode = iNode->GetChildByName("name");
          if(nameNode.GetChildren().size() > 0) {
            name = (nameNode.GetChildren()[0]).GetContent();
          }
        } catch(XMLException* e) {
        }
        Device* newDevice = new Device(id, this);
        if(name.size() > 0) {
          newDevice->SetName(name);
        }
        m_StaleDevices.push_back(newDevice);
      }
    }
  } // LoadDevices
  
  void Apartment::LoadModulators(XMLNode& _node) {
    XMLNodeList modulators = _node.GetChildren();
    for(XMLNodeList::iterator iModulator = modulators.begin(); iModulator != modulators.end(); ++iModulator) {
      if(iModulator->GetName() == "modulator") {
        int id = StrToInt(iModulator->GetAttributes()["id"]);
        string name;
        XMLNode& nameNode = iModulator->GetChildByName("name");
        if(nameNode.GetChildren().size() > 0) {
          name = (nameNode.GetChildren()[0]).GetContent();
        }
        Modulator* newModulator = new Modulator(id);
        if(name.size() > 0) {
          newModulator->SetName(name);
        }
        m_StaleModulators.push_back(newModulator);
      }
    }
  } // LoadModulators
  
  void Apartment::LoadRooms(XMLNode& _node) {
  } // LoadRooms
  
  Device& Apartment::GetDeviceByID(const devid_t _id) const {
    for(vector<Device*>::const_iterator ipDevice = m_Devices.begin(); ipDevice != m_Devices.end(); ++ipDevice) {
      if((*ipDevice)->GetID() == _id) {
        return **ipDevice;
      }
    }
    throw new ItemNotFoundException(IntToString(_id));
  } // GetDeviceByID
  
  Set Apartment::GetDevices() const {
    DeviceVector devs;
    for(vector<Device*>::const_iterator ipDevice = m_Devices.begin(); ipDevice != m_Devices.end(); ++ipDevice) {
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
    // search in the stale devices first
    for(vector<Device*>::iterator iDevice = m_StaleDevices.begin(); iDevice != m_StaleDevices.end(); ++iDevice) {
      if((*iDevice)->GetID() == _id) {
        return **iDevice;
      }
    }
    
    Device* pResult = new Device(_id, this);
    m_Devices.push_back(pResult);
    return *pResult;
  } 
  
  //================================================== Modulator
  
  Modulator::Modulator(const int _id) 
  : m_LocalID(_id)
  {
  } // ctor
  
  Set Modulator::GetDevices() const {
    return m_ConnectedDevices;
  } // GetDevices
  
  int Modulator::GetID() const {
    return m_LocalID;
  } // GetID
  
  //================================================== Room
  
  Set Room::GetDevices() const {
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
  
  Set Group::GetDevices() const {
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
  
  //================================================== DeviceReference
  
  devid_t DeviceReference::GetID() const {
    return m_DeviceID;
  } // GetID
  
  void DeviceReference::TurnOn() {
    GetDevice(),TurnOn(); 
  } // TurnOn
  
  void DeviceReference::TurnOff() {
    GetDevice().TurnOff();
  } // TurnOff
  
  void DeviceReference::IncreaseValue(const int _parameterNr) {
    GetDevice().IncreaseValue(_parameterNr);
  } // IncreaseValue
  
  void DeviceReference::DecreaseValue(const int _parameterNr) {
    GetDevice().DecreaseValue(_parameterNr);
  } // DecreaseValue
    
  void DeviceReference::Enable() {
    GetDevice().Enable();
  } // Enable
  
  void DeviceReference::Disable() {
    GetDevice().Disable();
  } // Disable
  
  void DeviceReference::StartDim(const bool _directionUp, const int _parameterNr) {
    GetDevice().StartDim(_directionUp, _parameterNr);
  } // StartDim
  
  void DeviceReference::EndDim(const int _parameterNr) {
    GetDevice().EndDim(_parameterNr);
  } // EndDim
  
  void DeviceReference::SetValue(const double _value, const int _parameterNr) {
    GetDevice().SetValue(_value, _parameterNr);
  } // SetValue
  
}