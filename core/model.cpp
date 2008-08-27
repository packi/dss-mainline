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
#include "DS485Interface.h"

#include "dss.h"
#include "logger.h"

namespace dss {

  //================================================== Device

  Device::Device(dsid_t _dsid, Apartment* _pApartment)
  : m_DSID(_dsid),
    m_ShortAddress(0xFF),
    m_pApartment(_pApartment)
  {
  }

  void Device::TurnOn() {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdTurnOn, *this);
  } // TurnOn

  void Device::TurnOff() {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdTurnOff, *this);
  } // TurnOff

  void Device::IncreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdIncreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdIncreaseParam, *this);
    }
  } // IncreaseValue

  void Device::DecreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdDecreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdDecreaseParam, *this);
    }
  } // DecreaseValue

  void Device::Enable() {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdEnable, *this);
  } // Enable

  void Device::Disable() {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdDisable, *this);
  } // Disable

  void Device::StartDim(const bool _directionUp, const int _parameterNr) {
    if(_directionUp) {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdStartDimUp, *this);
    } else {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdStartDimDown, *this);
    }
  } // StartDim

  void Device::EndDim(const int _parameterNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdStopDim, *this);
  } // EndDim

  bool Device::IsOn() {
    vector<int> res = DSS::GetInstance()->GetDS485Interface().SendCommand(cmdGetOnOff, *this);
    return res.front() != 0;
  } // IsOn

  int Device::GetFunctionID() {
    return DSS::GetInstance()->GetDS485Interface().SendCommand(cmdGetFunctionID, *this).front();
  } // GetFunctionID

  bool Device::IsSwitch() {
    return GetFunctionID() == FunctionIDSwitch;
  } // IsSwitch

  void Device::SetValue(const double _value, const int _parameterNr) {
  } // SetValue

  double Device::GetValue(const int _parameterNr) {
    vector<int> res = DSS::GetInstance()->GetDS485Interface().SendCommand(cmdGetValue, *this);
    return res.front();
  } // GetValue

  string Device::GetName() const {
    return m_Name;
  } // GetName

  void Device::SetName(const string& _name) {
    m_Name = _name;
  } // SetName

  bool Device::operator==(const Device& _other) const {
    return _other.m_ShortAddress == m_ShortAddress;
  } // operator==

  devid_t Device::GetShortAddress() const {
    return m_ShortAddress;
  } // GetShortAddress

  void Device::SetShortAddress(const devid_t _shortAddress) {
    m_ShortAddress = _shortAddress;
  } // SetShortAddress

  dsid_t Device::GetDSID() const {
    return m_DSID;
  } // GetDSID;

  int Device::GetModulatorID() const {
    return m_ModulatorID;
  } // GetModulatorID

  void Device::SetModulatorID(const int _modulatorID) {
    m_ModulatorID = _modulatorID;
  } // SetModulatorID

  int Device:: GetGroupIdByIndex(const int _index) const {
    return m_Groups[_index];
  } // GetGroupIdByIndex

  Group& Device::GetGroupByIndex(const int _index) {
    return m_pApartment->GetGroup(GetGroupIdByIndex(_index));
  } // GetGroupByIndex

  int Device::GetGroupsCount() const {
    return m_Groups.size();
  } // GetGroupsCount

  bitset<63>& Device::GetGroupBitmask() {
    return m_GroupBitmask;
  } // GetGroupBitmask

  bool Device::IsInGroup(const int _groupID) const {
    return m_GroupBitmask.test(_groupID - 1);
  }

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
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdTurnOn, *this);
  } // TurnOn

  void Set::TurnOff() {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdTurnOff, *this);
  } // TurnOff

  void Set::IncreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdIncreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdIncreaseParam, *this);
    }
  } // IncreaseValue

  void Set::DecreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdDecreaseValue, *this);
    } else {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdDecreaseParam, *this);
    }
  } // DecreaseValue

  void Set::Enable() {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdEnable, *this);
  } // Enable

  void Set::Disable() {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdDisable, *this);
  } // Disable

  void Set::StartDim(bool _directionUp, const int _parameterNr) {
    if(_directionUp) {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdStartDimUp, *this);
    } else {
      DSS::GetInstance()->GetDS485Interface().SendCommand(cmdStartDimDown, *this);
    }
  } // StartDim

  void Set::EndDim(const int _parameterNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdStopDim, *this);
  } // EndDim

  void Set::SetValue(const double _value, int _parameterNr) {
  } // SetValue

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
  public:
    ByGroupSelector(const int _groupNr)
    : m_GroupNr(_groupNr)
    {}

    virtual ~ByGroupSelector() {}

    virtual bool SelectDevice(const Device& _device) const {
      return _device.IsInGroup(m_GroupNr);
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
    virtual ~ByNameSelector() {};

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
    virtual ~ByIDSelector() {};

    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetShortAddress() == m_ID;
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
    out << "Device ID " << _dt.GetShortAddress();
    if(_dt.GetName().size() > 0) {
      out << " name: " << _dt.GetName();
    }
    return out;
  }

  //================================================== Arguments

  bool Arguments::HasValue(const string& _name) const {
    return m_ArgumentList.find(_name) != m_ArgumentList.end();
  } // HasValue

  string Arguments::GetValue(const string& _name) const {
    if(HasValue(_name)) {
      HashMapConstStringString::const_iterator it = m_ArgumentList.find(_name);
      return it->second;
    }
    throw ItemNotFoundException(_name);
  } // GetValue

  void Arguments::SetValue(const string& _name, const string& _value) {
    m_ArgumentList[_name] = _value;
  } // SetValue

  //================================================== Apartment

  Apartment::Apartment()
  : Thread("Apartment"),
    m_NextSubscriptionNumber(1)
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
    grp = new Group(8, *this);
    grp->SetName("white");
    m_Groups.push_back(grp);
  } // ctor

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

  void Apartment::Execute() {
    // Load devices/modulators/etc. from a config-file
    string configFileName = DSS::GetInstance()->GetConfig().GetOptionAs<string>("apartment_config", "data/apartment.xml");
    if(!FileExists(configFileName)) {
      Logger::GetInstance()->Log(string("Could not open config-file for apartment: '") + configFileName + "'", lsWarning);
    } else {
      ReadConfigurationFromXML(configFileName);
    }

    DS485Interface& interface = DSS::GetInstance()->GetDS485Interface();

    SleepMS(10000);
    while(!m_Terminated) {
     // TODO: reimplement proxy.WaitForProxyEvent();
      Logger::GetInstance()->Log("Apartment::Execute received proxy event, enumerating apartment / dSMs");

      vector<int> modIDs = interface.GetModulators();
      for(vector<int>::iterator iModulatorID = modIDs.begin(); iModulatorID != modIDs.end(); ++iModulatorID) {
        int modID = *iModulatorID;
        Logger::GetInstance()->Log(string("Found modulator with id: ") + IntToString(modID));
        dsid_t modDSID = interface.GetDSIDOfModulator(modID);
        Logger::GetInstance()->Log(string("  DSID: ") + UIntToString(modDSID));
        Modulator& modulator = AllocateModulator(modDSID);
        modulator.SetBusID(modID);

        vector<int> roomIDs = interface.GetRooms(modID);
        for(vector<int>::iterator iRoomID = roomIDs.begin(); iRoomID != roomIDs.end(); ++iRoomID) {
          int roomID = *iRoomID;
          Logger::GetInstance()->Log(string("  Found room with id: ") + IntToString(roomID));

          vector<int> devices = interface.GetDevicesInRoom(modID, roomID);
          for(vector<int>::iterator iDevice = devices.begin(); iDevice != devices.end(); ++iDevice) {
            int devID = *iDevice;
            Logger::GetInstance()->Log(string("    Found device with id: ") + IntToString(devID));
            dsid_t dsid = interface.GetDSIDOfDevice(modID, devID);
            Device& dev = AllocateDevice(dsid);
            dev.SetShortAddress(devID);
            dev.SetModulatorID(modID);
          }
          vector<int> groupIDs = interface.GetGroups(modID, roomID);
          for(vector<int>::iterator iGroup = groupIDs.begin(), e = groupIDs.end();
              iGroup != e; ++iGroup)
          {
            int groupID = *iGroup;
            Logger::GetInstance()->Log(string("    Found group with id: ") + IntToString(groupID));
            vector<int> devingroup = interface.GetDevicesInGroup(modID, roomID, groupID);
            for(vector<int>::iterator iDevice = devingroup.begin(), e = devingroup.end();
                iDevice != e; ++iDevice)
            {
              int devID = *iDevice;
              dsid_t dsid = interface.GetDSIDOfDevice(modID, devID);
              Device& dev = AllocateDevice(dsid);
              dev.SetShortAddress(devID);
              dev.GetGroupBitmask().set(groupID);
            }
          }
        }
      }
      break;
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
        dsid_t dsid = StrToUInt(iNode->GetAttributes()["dsid"]);
        string name;
        try {
          XMLNode& nameNode = iNode->GetChildByName("name");
          if(nameNode.GetChildren().size() > 0) {
            name = (nameNode.GetChildren()[0]).GetContent();
          }
        } catch(XMLException& e) {
          /* discard node not found exceptions */
        }
        Device* newDevice = new Device(dsid, this);
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

  Device& Apartment::GetDeviceByDSID(const dsid_t _dsid) const {
    for(vector<Device*>::const_iterator ipDevice = m_Devices.begin(); ipDevice != m_Devices.end(); ++ipDevice) {
      if((*ipDevice)->GetDSID() == _dsid) {
        return **ipDevice;
      }
    }
    throw ItemNotFoundException(IntToString(_dsid));
  } // GetDeviceByShortAddress

  Device& Apartment::GetDeviceByName(const string& _name) {
    for(vector<Device*>::const_iterator ipDevice = m_Devices.begin(); ipDevice != m_Devices.end(); ++ipDevice) {
      if((*ipDevice)->GetName() == _name) {
        return **ipDevice;
      }
    }
    throw ItemNotFoundException(_name);
  } // GetDeviceByName

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
    throw ItemNotFoundException(_roomName);
  } // GetRoom(name)

  Room& Apartment::GetRoom(const int _id) {
    for(vector<Room*>::iterator iRoom = m_Rooms.begin(); iRoom != m_Rooms.end(); ++iRoom) {
      if((*iRoom)->GetRoomID() == _id) {
        return **iRoom;
      }
    }
    throw ItemNotFoundException(IntToString(_id));
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
    throw ItemNotFoundException(_modName);
  } // GetModulator(name)

  Modulator& Apartment::GetModulatorByBusID(const int _busId) {
    for(vector<Modulator*>::iterator iModulator = m_Modulators.begin(); iModulator != m_Modulators.end(); ++iModulator) {
      if((*iModulator)->GetBusID() == _busId) {
        return **iModulator;
      }
    }
    throw ItemNotFoundException(IntToString(_busId));
  } // GetModulatorByBusID

  Modulator& Apartment::GetModulatorByDSID(const dsid_t _dsid) {
    for(vector<Modulator*>::iterator iModulator = m_Modulators.begin(); iModulator != m_Modulators.end(); ++iModulator) {
      if((*iModulator)->GetDSID() == _dsid) {
        return **iModulator;
      }
    }
    throw ItemNotFoundException(IntToString(_dsid));
  } // GetModulatorByDSID

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
    throw ItemNotFoundException(_name);
  } // GetGroup(name)

  Group& Apartment::GetGroup(const int _id) {
    for(vector<Group*>::iterator ipGroup = m_Groups.begin(); ipGroup != m_Groups.end(); ++ipGroup) {
      if((*ipGroup)->GetID() == _id) {
        return **ipGroup;
      }
    }
    throw ItemNotFoundException(IntToString(_id));
  } // GetGroup(id)

  vector<Group*>& Apartment::GetGroups() {
    return m_Groups;
  } // GetGroups

  Subscription& Apartment::Subscribe(Action& _action, Arguments& _actionArgs, vector<int> _eventIDs, vector<int> _sourceIDs) {
    Subscription* pResult = new Subscription(m_NextSubscriptionNumber++, _action, _actionArgs, _eventIDs, _sourceIDs);
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

  int Apartment::GetSubscriptionCount() {
    return m_Subscriptions.size();
  } // GetSubscriptionCount

  Subscription& Apartment::GetSubscription(const int _index) {
    return *m_Subscriptions.at(_index);
  } // GetSubscription

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
    stringstream sstream;
    sstream << "Raised event: " << _event.GetID() << " from source " << _event.GetSource();
    Logger::GetInstance()->Log(sstream.str());
    for_each(m_Subscriptions.begin(), m_Subscriptions.end(), handle_event(_event));
  } // OnEvent

  Device& Apartment::AllocateDevice(const dsid_t _dsid) {
    // search for existing device
    for(vector<Device*>::iterator iDevice = m_Devices.begin(); iDevice != m_Devices.end(); ++iDevice) {
      if((*iDevice)->GetDSID() == _dsid) {
        return **iDevice;
      }
    }

    // search for stale devices
    for(vector<Device*>::iterator iDevice = m_StaleDevices.begin(); iDevice != m_StaleDevices.end(); ++iDevice) {
      if((*iDevice)->GetDSID() == _dsid) {
        Device* pResult = *iDevice;
        m_Devices.push_back(pResult);
        m_StaleDevices.erase(iDevice);
        return *pResult;
      }
    }

    Device* pResult = new Device(_dsid, this);
    m_Devices.push_back(pResult);
    return *pResult;
  } // AllocateDevice

  Modulator& Apartment::AllocateModulator(const dsid_t _dsid) {
    // searth in the stale modulators first
    for(vector<Modulator*>::iterator iModulator = m_StaleModulators.begin(), e = m_StaleModulators.end();
        iModulator != e; ++iModulator)
    {
      if((*iModulator)->GetDSID() == _dsid) {
        m_Modulators.push_back(*iModulator);
        m_StaleModulators.erase(iModulator);
        return **iModulator;
      }
    }

    for(vector<Modulator*>::iterator iModulator = m_Modulators.begin(), e = m_Modulators.end();
        iModulator != e; ++iModulator)
    {
      if((*iModulator)->GetDSID() == _dsid) {
        return **iModulator;
      }
    }

    // TODO: check for existing Modulator?
    Modulator* pResult = new Modulator(_dsid);
    m_Modulators.push_back(pResult);
    return *pResult;
  } // AllocateModulator

  void Apartment::AddAction(Action* _action) {
    m_Actions.push_back(_action);
  } // AddAction

  Action& Apartment::GetAction(const string& _name) {
    for(boost::ptr_vector<Action>::iterator iAction = m_Actions.begin(), e = m_Actions.end();
        iAction != e; ++iAction)
    {
      if(iAction->GetName() == _name) {
        return *iAction;
      }
    }
    throw ItemNotFoundException(string("Could not find action: ") + _name);
  } // GetAction

  //================================================== Modulator

  Modulator::Modulator(const dsid_t _dsid)
  : m_DSID(_dsid),
    m_BusID(0xFF)
  {
  } // ctor

  Set Modulator::GetDevices() const {
    return m_ConnectedDevices;
  } // GetDevices

  dsid_t Modulator::GetDSID() const {
    return m_DSID;
  } // GetDSID

  int Modulator::GetBusID() const {
    return m_BusID;
  } // GetBusID

  void Modulator::SetBusID(const int _busID) {
    m_BusID = _busID;
  } // SetBusID

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

  const vector<int>& Subscription::GetSourceIDs() const {
    return m_SourceIDs;
  } // GetSourceIDs

  const vector<int>& Subscription::GetEventIDs() const {
    return m_EventIDs;
  } // GetEventIDs

  const string& Subscription::GetName() const {
    return m_Name;
  } // GetName

  void Subscription::SetName(const string& _value) {
    m_Name = _value;
  } // SetName

  //================================================== DeviceReference

  DeviceReference::DeviceReference(const DeviceReference& _copy)
  : m_DSID(_copy.m_DSID),
    m_Apartment(_copy.m_Apartment)
  {
  } // ctor(copy)

  DeviceReference::DeviceReference(const dsid_t _dsid, const Apartment& _apartment)
  : m_DSID(_dsid),
    m_Apartment(&_apartment)
  {
  } // ctor(dsid)

  DeviceReference::DeviceReference(const Device& _device, const Apartment& _apartment)
  : m_DSID(_device.GetDSID()),
    m_Apartment(&_apartment)
  {
  } // ctor(device)

  Device& DeviceReference::GetDevice() {
    return m_Apartment->GetDeviceByDSID(m_DSID);
  } // GetDevice

  dsid_t DeviceReference::GetDSID() const {
    return m_DSID;
  } // GetID

  void DeviceReference::TurnOn() {
    GetDevice().TurnOn();
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

  bool DeviceReference::IsOn() {
    return GetDevice().IsOn();
  }

  bool DeviceReference::IsSwitch() {
    return GetDevice().IsSwitch();
  }

}
