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

#include "ds485const.h"
#include "dss.h"
#include "logger.h"
#include "propertysystem.h"

#include "foreach.h"

namespace dss {

  //================================================== Device

  const devid_t ShortAddressStaleDevice = 0xFFFF;

  Device::Device(dsid_t _dsid, Apartment* _pApartment)
  : m_DSID(_dsid),
    m_ShortAddress(ShortAddressStaleDevice),
    m_ZoneID(0),
    m_pApartment(_pApartment),
    m_SubscriptionEventID(-1),
    m_pPropertyNode(NULL)
  {
  }

  void Device::PublishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      // don't publish stale devices for now
      if(m_ShortAddress == ShortAddressStaleDevice) {
        return;
      } else {
        if(m_pApartment->GetPropertyNode() != NULL) {
          m_pPropertyNode = m_pApartment->GetPropertyNode()->CreateProperty("zones/zone" + IntToString(m_ZoneID) + "/device+");
//          m_pPropertyNode->CreateProperty("name")->LinkToProxy(PropertyProxyMemberFunction<Device, string>(*this, &Device::GetName, &Device::SetName));
          m_pPropertyNode->CreateProperty("name")->LinkToProxy(PropertyProxyReference<string>(m_Name));
          m_pPropertyNode->CreateProperty("ModulatorID")->LinkToProxy(PropertyProxyReference<int>(m_ModulatorID, false));
          m_pPropertyNode->CreateProperty("ZoneID")->LinkToProxy(PropertyProxyReference<int>(m_ZoneID, false));
        }
      }
    }
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

  bool Device::IsOn() const {
    vector<int> res = DSS::GetInstance()->GetDS485Interface().SendCommand(cmdGetOnOff, *this);
    return res.front() != 0;
  } // IsOn

  int Device::GetFunctionID() const {
    return m_FunctionID;
  } // GetFunctionID

  void Device::SetFunctionID(const int _value) {
    m_FunctionID = _value;
  } // SetFunctionID

  bool Device::HasSwitch() const {
    return GetFunctionID() == FunctionIDSwitch;
  } // HasSwitch

  void Device::SetValue(const double _value, const int _parameterNr) {
  } // SetValue

  double Device::GetValue(const int _parameterNr) {
    vector<int> res = DSS::GetInstance()->GetDS485Interface().SendCommand(cmdGetValue, *this);
    return res.front();
  } // GetValue

  void Device::CallScene(const int _sceneNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdCallScene, *this, _sceneNr);
  } // CallScene

  void Device::SaveScene(const int _sceneNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdSaveScene, *this, _sceneNr);
  } // SaveScene

  void Device::UndoScene(const int _sceneNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdUndoScene, *this, _sceneNr);
  } // UndoScene

  string Device::GetName() const {
    return m_Name;
  } // GetName

  void Device::SetName(const string& _name) {
    m_Name = _name;
  } // SetName

  bool Device::HasSubscription() const {
    return m_SubscriptionEventID != -1;
  } // HasSubscription

  int Device::GetSubscriptionEventID() const {
    return m_SubscriptionEventID;
  } // GetSubscriptionEventID

  void Device::SetSubscriptionEventID(const int _value) {
    m_SubscriptionEventID = _value;
  } // SetSubscriptionEventID

  bool Device::operator==(const Device& _other) const {
    return _other.m_ShortAddress == m_ShortAddress;
  } // operator==

  devid_t Device::GetShortAddress() const {
    return m_ShortAddress;
  } // GetShortAddress

  void Device::SetShortAddress(const devid_t _shortAddress) {
    m_ShortAddress = _shortAddress;
    PublishToPropertyTree();
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

  int Device::GetZoneID() const {
  	return m_ZoneID;
  } // GetZoneID

  void Device::SetZoneID(const int _value) {
  	m_ZoneID = _value;
  } // SetZoneID

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
  } // IsInGroup

  Apartment& Device::GetApartment() const {
    return *m_pApartment;
  } // GetApartment

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

  void Set::CallScene(const int _sceneNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdCallScene, *this, _sceneNr);
  } // CallScene

  void Set::SaveScene(const int _sceneNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdSaveScene, *this, _sceneNr);
  } // SaveScene

  void Set::UndoScene(const int _sceneNr) {
    DSS::GetInstance()->GetDS485Interface().SendCommand(cmdUndoScene, *this, _sceneNr);
  } // UndoScene

  void Set::Perform(IDeviceAction& _deviceAction) {
    for(DeviceIterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      _deviceAction.Perform(iDevice->GetDevice());
    }
  } // Perform

  Set Set::GetSubset(const IDeviceSelector& _selector) const {
    Set result;
    foreach(DeviceReference iDevice, m_ContainedDevices) {
      if(_selector.SelectDevice(iDevice.GetDevice())) {
        result.AddDevice(iDevice);
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

  Set Set::GetByGroup(int _groupNr) const {
    return GetSubset(ByGroupSelector(_groupNr));
  } // GetByGroup(id)

  Set Set::GetByGroup(const Group& _group) const {
    return GetByGroup(_group.GetID());
  } // GetByGroup(ref)

  Set Set::GetByGroup(const string& _name) const {
    Set result;
    if(IsEmpty()) {
      return result;
    } else {
      Group& g = Get(0).GetDevice().GetApartment().GetGroup(_name);
      return GetByGroup(g.GetID());
    }
  } // GetByGroup(name)

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
    const devid_t m_ID;
  public:
    ByIDSelector(const devid_t _id) : m_ID(_id) {}
    virtual ~ByIDSelector() {};

    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetShortAddress() == m_ID;
    }
  };

  DeviceReference Set::GetByBusID(const devid_t _id) {
    Set resultSet = GetSubset(ByIDSelector(_id));
    if(resultSet.Length() == 0) {
      throw ItemNotFoundException(string("with busid ") + IntToString(_id));
    }
    return resultSet.m_ContainedDevices.front();
  } // GetByBusID

  class ByDSIDSelector : public IDeviceSelector {
  private:
    const dsid_t m_ID;
  public:
    ByDSIDSelector(const dsid_t _id) : m_ID(_id) {}
    virtual ~ByDSIDSelector() {};

    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetDSID() == m_ID;
    }
  };

  DeviceReference Set::GetByDSID(const dsid_t _dsid) {
    Set resultSet = GetSubset(ByDSIDSelector(_dsid));
    if(resultSet.Length() == 0) {
      throw ItemNotFoundException("with dsid " + _dsid.ToString());
    }
    return resultSet.m_ContainedDevices.front();
  } // GetByDSID

  int Set::Length() const {
    return m_ContainedDevices.size();
  } // Length

  bool Set::IsEmpty() const {
    return Length() == 0;
  } // IsEmpty

  Set Set::Combine(Set& _other) const {
    Set resultSet(_other);
    foreach(const DeviceReference& iDevice, m_ContainedDevices) {
      if(!resultSet.Contains(iDevice)) {
        resultSet.AddDevice(iDevice);
      }
    }
    return resultSet;
  } // Combine

  Set Set::Remove(const Set& _other) const {
    Set resultSet(*this);
    foreach(const DeviceReference& iDevice, m_ContainedDevices) {
      resultSet.RemoveDevice(iDevice);
    }
    return resultSet;
  } // Remove

  bool Set::Contains(const DeviceReference& _device) const {
    DeviceVector::const_iterator pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    return pos != m_ContainedDevices.end();
  } // Contains

  bool Set::Contains(const Device& _device) const {
    return Contains(DeviceReference(_device, _device.GetApartment()));
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
  } // RemoveDevice

  const DeviceReference& Set::Get(int _index) const {
    return m_ContainedDevices.at(_index);
  } // Get

  const DeviceReference& Set::operator[](const int _index) const {
    return Get(_index);
  } // operator[]

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
  } // operator<<

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

  Apartment::Apartment(DSS* _pDSS)
  : Subsystem(_pDSS, "Apartment"),
    Thread("Apartment"),
    m_pPropertyNode(NULL)
  {
    Zone* zoneZero = new Zone(0);
    AddDefaultGroupsToZone(*zoneZero);
    m_Zones.push_back(zoneZero);
    m_IsInitializing = true;
  } // ctor

  Apartment::~Apartment() {
    ScrubVector(m_Devices);
    ScrubVector(m_Groups);
    ScrubVector(m_Zones);
    ScrubVector(m_Modulators);

    ScrubVector(m_StaleDevices);
    ScrubVector(m_StaleGroups);
    ScrubVector(m_StaleModulators);
    ScrubVector(m_StaleZones);
  } // dtor

  void Apartment::Initialize() {
    Subsystem::Initialize();
    DSS::GetInstance()->GetPropertySystem().SetStringValue(GetPropertyBasePath() + "configfile", "data/apartment.xml", true);
    m_pPropertyNode = DSS::GetInstance()->GetPropertySystem().CreateProperty("/apartment");
  } // Initialize

  void Apartment::Start() {
    Subsystem::Start();
    Run();
  } // Start

  void Apartment::AddDefaultGroupsToZone(Zone& _zone) {
    int zoneID = _zone.GetZoneID();

    Group* grp = new Group(GroupIDBroadcast, zoneID, *this);
    grp->SetName("broadcast");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDYellow, zoneID, *this);
    grp->SetName("yellow");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDGray, zoneID, *this);
    grp->SetName("gray");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDBlue, zoneID, *this);
    grp->SetName("blue");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDCyan, zoneID, *this);
    grp->SetName("cyan");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDRed, zoneID, *this);
    grp->SetName("red");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDViolet, zoneID, *this);
    grp->SetName("magenta");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDGreen, zoneID, *this);
    grp->SetName("green");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDBlack, zoneID, *this);
    grp->SetName("black");
    m_Groups.push_back(grp);
    grp = new Group(GroupIDWhite, zoneID, *this);
    grp->SetName("white");
    m_Groups.push_back(grp);
  } // AddDefaultGroupsToZone

  void Apartment::Execute() {
    // Load devices/modulators/etc. from a config-file
    string configFileName = DSS::GetInstance()->GetPropertySystem().GetStringValue(GetPropertyBasePath() + "configfile");
    if(!FileExists(configFileName)) {
      Logger::GetInstance()->Log(string("Could not open config-file for apartment: '") + configFileName + "'", lsWarning);
    } else {
      ReadConfigurationFromXML(configFileName);
    }

    DS485Interface& interface = DSS::GetInstance()->GetDS485Interface();

    while(!interface.IsReady() && !m_Terminated) {
      SleepMS(1000);
    }
/*
    while(!m_Terminated) {
      SleepSeconds(2);
    }
*/

    while(!m_Terminated) {
      Logger::GetInstance()->Log("Apartment::Execute received proxy event, enumerating apartment / dSMs");

      vector<int> modIDs = interface.GetModulators();
      foreach(int modulatorID, modIDs) {
        Log("Found modulator with id: " + IntToString(modulatorID));
        dsid_t modDSID = interface.GetDSIDOfModulator(modulatorID);
        Log("  DSID: " + modDSID.ToString());
        Modulator& modulator = AllocateModulator(modDSID);
        modulator.SetBusID(modulatorID);

        vector<int> zoneIDs = interface.GetZones(modulatorID);
        foreach(int zoneID, zoneIDs) {
          Log("  Found zone with id: " + IntToString(zoneID));
          Zone& zone = AllocateZone(modulator, zoneID);

          vector<int> devices = interface.GetDevicesInZone(modulatorID, zoneID);
          foreach(int devID, devices) {
            dsid_t dsid = interface.GetDSIDOfDevice(modulatorID, devID);
            int functionID = interface.SendCommand(cmdGetFunctionID, devID, modulatorID).front();
            Log("    Found device with id: " + IntToString(devID));
            Log("    DSID:        " + dsid.ToString());
            Log("    Function ID: " + UnsignedLongIntToHexString(functionID));
            Device& dev = AllocateDevice(dsid);
            dev.SetShortAddress(devID);
            dev.SetModulatorID(modulatorID);
            dev.SetZoneID(zoneID);
            dev.SetFunctionID(functionID);
            zone.AddDevice(DeviceReference(dev, *this));
            modulator.AddDevice(DeviceReference(dev, *this));
          }

          vector<int> groupIDs = interface.GetGroups(modulatorID, zoneID);
          foreach(int groupID, groupIDs) {
            Log("    Found group with id: " + IntToString(groupID));
            vector<int> devingroup = interface.GetDevicesInGroup(modulatorID, zoneID, groupID);
            foreach(int devID, devingroup) {
              try {
                Log("     Adding device " + IntToString(devID) + " to group " + IntToString(groupID));
                Device& dev = GetDeviceByShortAddress(modulator, devID);
                dev.SetShortAddress(devID);
                dev.GetGroupBitmask().set(groupID-1);
              } catch(ItemNotFoundException& e) {
                Logger::GetInstance()->Log(string("Could not find device with short-address ") + IntToString(devID));
              }
            }
          }
        }
      }
      foreach(Device* dev, m_Devices) {
        if(dev->HasSubscription()) {
          interface.Subscribe(dev->GetModulatorID(), 0, dev->GetShortAddress());
        }
      }
      break;
    }
    Logger::GetInstance()->Log("******** Finished loading model from dSM(s)...", lsInfo);
    m_IsInitializing = false;
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
        dsid_t dsid = dsid_t::FromString(iNode->GetAttributes()["dsid"]);
        string name;
        try {
          XMLNode& nameNode = iNode->GetChildByName("name");
          if(nameNode.GetChildren().size() > 0) {
            name = (nameNode.GetChildren()[0]).GetContent();
          }
        } catch(XMLException& e) {
          /* discard node not found exceptions */
        }
        int eventid = -1;
        try {
          XMLNode& nameNode = iNode->GetChildByName("event");
          if(nameNode.GetChildren().size() > 0) {
            eventid = StrToIntDef((nameNode.GetChildren()[0]).GetContent(), -1);
          }
        } catch(XMLException& e) {
          /* discard node not found exceptions */
        }
        Device* newDevice = new Device(dsid, this);
        if(name.size() > 0) {
          newDevice->SetName(name);
        }
        if(eventid != -1) {
          newDevice->SetSubscriptionEventID(eventid);
        }
        m_StaleDevices.push_back(newDevice);
      }
    }
  } // LoadDevices

  void Apartment::LoadModulators(XMLNode& _node) {
    XMLNodeList modulators = _node.GetChildren();
    for(XMLNodeList::iterator iModulator = modulators.begin(); iModulator != modulators.end(); ++iModulator) {
      if(iModulator->GetName() == "modulator") {
        dsid_t id = dsid_t::FromString(iModulator->GetAttributes()["id"]);
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
        LoadZones(*iModulator, *newModulator);
      }
    }
  } // LoadModulators

  void Apartment::LoadZones(XMLNode& _node, Modulator& _modulator) {
    // TODO: A Zone can span over multiple modulators
    XMLNodeList zones = _node.GetChildren();
    for(XMLNodeList::iterator iZone = zones.begin(); iZone != zones.end(); ++iZone) {
      if(iZone->GetName() == "zone") {
        int id = StrToInt(iZone->GetAttributes()["id"]);
        string name;
        XMLNode& nameNode = iZone->GetChildByName("name");
        if(nameNode.GetChildren().size() > 0) {
          name = (nameNode.GetChildren()[0]).GetContent();
        }
        Zone* newZone = new Zone(id);
        if(name.size() > 0) {
          newZone->SetName(name);
        }
        newZone->AddToModulator(_modulator);
        m_StaleZones.push_back(newZone);
      }
    }
  } // LoadZones

  Device& Apartment::GetDeviceByDSID(const dsid_t _dsid) const {
    foreach(Device* dev, m_Devices) {
      if(dev->GetDSID() == _dsid) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_dsid.ToString());
  } // GetDeviceByShortAddress const

  Device& Apartment::GetDeviceByDSID(const dsid_t _dsid) {
    foreach(Device* dev, m_Devices) {
      if(dev->GetDSID() == _dsid) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_dsid.ToString());
  } // GetDeviceByShortAddress

  Device& Apartment::GetDeviceByShortAddress(const Modulator& _modulator, const devid_t _deviceID) const {
    foreach(Device* dev, m_Devices) {
      if((dev->GetShortAddress() == _deviceID) &&
          (_modulator.GetBusID() == dev->GetModulatorID())) {
        return *dev;
      }
    }
    throw ItemNotFoundException(IntToString(_deviceID));
  } // GetDeviceByShortAddress

  Device& Apartment::GetDeviceByName(const string& _name) {
    foreach(Device* dev, m_Devices) {
      if(dev->GetName() == _name) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_name);
  } // GetDeviceByName

  Set Apartment::GetDevices() const {
    DeviceVector devs;
    foreach(Device* dev, m_Devices) {
      devs.push_back(DeviceReference(*dev, *this));
    }

    return Set(devs);
  } // GetDevices

  Zone& Apartment::GetZone(const string& _zoneName) {
    foreach(Zone* zone, m_Zones) {
      if(zone->GetName() == _zoneName) {
        return *zone;
      }
    }
    throw ItemNotFoundException(_zoneName);
  } // GetZone(name)

  Zone& Apartment::GetZone(const int _id) {
    foreach(Zone* zone, m_Zones) {
      if(zone->GetZoneID() == _id) {
        return *zone;
      }
    }
    throw ItemNotFoundException(IntToString(_id));
  } // GetZone(id)

  vector<Zone*>& Apartment::GetZones() {
    return m_Zones;
  } // GetZones

  Modulator& Apartment::GetModulator(const string& _modName) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->GetName() == _modName) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(_modName);
  } // GetModulator(name)

  Modulator& Apartment::GetModulatorByBusID(const int _busId) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->GetBusID() == _busId) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(IntToString(_busId));
  } // GetModulatorByBusID

  Modulator& Apartment::GetModulatorByDSID(const dsid_t _dsid) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->GetDSID() == _dsid) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(_dsid.ToString());
  } // GetModulatorByDSID

  vector<Modulator*>& Apartment::GetModulators() {
    return m_Modulators;
  } // GetModulators

  // Group queries
  Group& Apartment::GetGroup(const string& _name) {
    foreach(Group* group, m_Groups) {
      if(group->GetName() == _name) {
        return *group;
      }
    }
    throw ItemNotFoundException(_name);
  } // GetGroup(name)

  Group& Apartment::GetGroup(const int _id) {
    foreach(Group* group, m_Groups) {
      if(group->GetID() == _id) {
        return *group;
      }
    }
    throw ItemNotFoundException(IntToString(_id));
  } // GetGroup(id)

  vector<Group*>& Apartment::GetGroups() {
    return m_Groups;
  } // GetGroups

  Device& Apartment::AllocateDevice(const dsid_t _dsid) {
    // search for existing device
    foreach(Device* device, m_Devices) {
      if(device->GetDSID() == _dsid) {
        GetZone(0).AddDevice(DeviceReference(*device, *this));
        return *device;
      }
    }

    // search for stale devices
    foreach(Device* device, m_StaleDevices) {
      if(device->GetDSID() == _dsid) {
        Device* pResult = device;
        m_Devices.push_back(pResult);
        m_StaleDevices.erase(std::find(m_StaleDevices.begin(), m_StaleDevices.end(),device));
        GetZone(0).AddDevice(DeviceReference(*pResult, *this));
        return *pResult;
      }
    }

    Device* pResult = new Device(_dsid, this);
    m_Devices.push_back(pResult);
    GetZone(0).AddDevice(DeviceReference(*pResult, *this));
    return *pResult;
  } // AllocateDevice

  Modulator& Apartment::AllocateModulator(const dsid_t _dsid) {
    foreach(Modulator* modulator, m_StaleModulators) {
      if((modulator)->GetDSID() == _dsid) {
        return *modulator;
      }
    }

    foreach(Modulator* modulator, m_StaleModulators) {
      if(modulator->GetDSID() == _dsid) {
        m_Modulators.push_back(modulator);
        m_StaleModulators.erase(std::find(m_StaleModulators.begin(), m_StaleModulators.end(), modulator));
        return *modulator;
      }
    }

    Modulator* pResult = new Modulator(_dsid);
    m_Modulators.push_back(pResult);
    return *pResult;
  } // AllocateModulator

  Zone& Apartment::AllocateZone(Modulator& _modulator, int _zoneID) {
    foreach(Zone* zone, m_StaleZones) {
  		if(zone->GetZoneID() == _zoneID) {
  			m_Zones.push_back(zone);
  			m_StaleZones.erase(std::find(m_StaleZones.begin(), m_StaleZones.end(), zone));
  	    if(!IsInitializing()) {
  	      DSS::GetInstance()->GetDS485Interface().CreateZone(_modulator.GetBusID(), _zoneID);
  	    }
  			return *zone;
  		}
  	}

    foreach(Zone* zone, m_Zones) {
  		if(zone->GetZoneID() == _zoneID) {
  		  vector<int> modsOfZone = zone->GetModulators();
  		  if(find(modsOfZone.begin(), modsOfZone.end(), _modulator.GetBusID()) == modsOfZone.end()) {
          DSS::GetInstance()->GetDS485Interface().CreateZone(_modulator.GetBusID(), _zoneID);
  		  }
  			return *zone;
  		}
  	}

  	Zone* zone = new Zone(_zoneID);
  	m_Zones.push_back(zone);
  	zone->AddToModulator(_modulator);
  	AddDefaultGroupsToZone(*zone);
    if(!IsInitializing()) {
      DSS::GetInstance()->GetDS485Interface().CreateZone(_modulator.GetBusID(), _zoneID);
    }
  	return *zone;
  } // AllocateZone

  void Apartment::OnKeypress(const dsid_t& _dsid, const ButtonPressKind _kind, const int _number) {
    Device& dev = GetDeviceByDSID(_dsid);
    if(dev.HasSubscription()) {
  /*
      Event evt(dev.GetSubscriptionEventID(), _dsid);
      OnEvent(evt);
      */
    }
  } // OnKeypress

  //================================================== Modulator

  Modulator::Modulator(const dsid_t _dsid)
  : m_DSID(_dsid),
    m_BusID(0xFF)
  {
  } // ctor

  Set Modulator::GetDevices() const {
    return m_ConnectedDevices;
  } // GetDevices

  void Modulator::AddDevice(const DeviceReference& _device) {
  	m_ConnectedDevices.push_back(_device);
  } // AddDevice

  dsid_t Modulator::GetDSID() const {
    return m_DSID;
  } // GetDSID

  int Modulator::GetBusID() const {
    return m_BusID;
  } // GetBusID

  void Modulator::SetBusID(const int _busID) {
    m_BusID = _busID;
  } // SetBusID

  unsigned long Modulator::GetPowerConsumption() {
    return DSS::GetInstance()->GetDS485Interface().GetPowerConsumption(m_BusID);
  } // GetPowerConsumption

  unsigned long Modulator::GetEnergyMeterValue() {
    return DSS::GetInstance()->GetDS485Interface().GetEnergyMeterValue(m_BusID);
  } // GetEnergyMeterValue


  //================================================== Zone

  Set Zone::GetDevices() const {
    return Set(m_Devices);
  } // GetDevices

  void Zone::AddDevice(const DeviceReference& _device) {
    const Device& dev = _device.GetDevice();
  	int oldZoneID = dev.GetZoneID();
  	if(oldZoneID != -1) {
  		try {
  		  Zone& oldZone = dev.GetApartment().GetZone(oldZoneID);
  		  oldZone.RemoveDevice(_device);
  		} catch(runtime_error&) {
  		}
  	}
    m_Devices.push_back(_device);
  	if(!dev.GetApartment().IsInitializing()) {
  		DSS::GetInstance()->GetDS485Interface().SetZoneID(dev.GetModulatorID(), dev.GetShortAddress(), m_ZoneID);
  	}
  } // AddDevice

  void Zone::RemoveDevice(const DeviceReference& _device) {
    DeviceIterator pos = find(m_Devices.begin(), m_Devices.end(), _device);
    if(pos != m_Devices.end()) {
      m_Devices.erase(pos);
    }
  } // RemoveDevice

  Group* Zone::GetGroup(const string& _name) const {
    for(vector<Group*>::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->GetName() == _name) {
          return *ipGroup;
        }
    }
    return NULL;
  } // GetGroup

  Group* Zone::GetGroup(const int _id) const {
    for(vector<Group*>::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->GetID() == _id) {
          return *ipGroup;
        }
    }
    return NULL;
  } // GetGroup

  void AddGroup(UserGroup& _group);
  void RemoveGroup(UserGroup& _group);

  int Zone::GetZoneID() const {
    return m_ZoneID;
  } // GetZoneID

  void Zone::SetZoneID(const int _value) {
    m_ZoneID = _value;
  } // SetZoneID

  void Zone::AddToModulator(Modulator& _modulator) {
    m_Modulators.push_back(&_modulator);
  } // AddToModulator

  void Zone::RemoveFromModulator(Modulator& _modulator) {
    m_Modulators.erase(find(m_Modulators.begin(), m_Modulators.end(), &_modulator));
  } // RemoveFromModulator

  vector<int> Zone::GetModulators() const {
  	vector<int> result;
  	for(vector<Modulator*>::const_iterator iModulator = m_Modulators.begin(), e = m_Modulators.end();
  	    iModulator != e; ++iModulator) {
  		result.push_back((*iModulator)->GetBusID());
  	}
  	return result;
  }

  //============================================= Group

  Group::Group(const int _id, const int _zoneID, Apartment& _apartment)
  : m_Apartment(_apartment),
    m_ZoneID(_zoneID),
    m_GroupID(_id)
  {
  }

  int Group::GetID() const {
    return m_GroupID;
  } // GetID

  Set Group::GetDevices() const {
    return m_Apartment.GetDevices().GetByGroup(m_GroupID);
  }

  void Group::AddDevice(const DeviceReference& _device) { /* do nothing or throw? */ }
  void Group::RemoveDevice(const DeviceReference& _device) { /* do nothing or throw? */ }

  void Group::TurnOn() {
    GetDevices().TurnOn();
  } // TurnOn

  void Group::TurnOff() {
    GetDevices().TurnOff();
  } // TurnOff

  void Group::IncreaseValue(const int _parameterNr) {
    GetDevices().IncreaseValue(_parameterNr);
  } // IncreaseValue

  void Group::DecreaseValue(const int _parameterNr) {
    GetDevices().DecreaseValue(_parameterNr);
  } // DecreaseValue

  void Group::Enable() {
    GetDevices().Enable();
  } // Enable

  void Group::Disable() {
    GetDevices().Disable();
  } // Disable

  void Group::StartDim(bool _directionUp, const int _parameterNr)  {
    GetDevices().StartDim(_directionUp, _parameterNr);
  } // StartDim

  void Group::EndDim(const int _parameterNr) {
    GetDevices().EndDim(_parameterNr);
  } // EndDim

  void Group::SetValue(const double _value, int _parameterNr) {
    GetDevices().SetValue(_value, _parameterNr);
  } // SetValue

  Group& Group::operator=(const Group& _other) {
    m_Devices = _other.m_Devices;
    //m_Apartment = _other.m_Apartment;
    m_GroupID = _other.m_GroupID;
    m_ZoneID = _other.m_ZoneID;
    return *this;
  } // operator=

  void Group::CallScene(const int _sceneNr) {
    GetDevices().CallScene(_sceneNr);
  } // CallScene

  void Group::SaveScene(const int _sceneNr) {
    GetDevices().SaveScene(_sceneNr);
  } // SaveScene

  void Group::UndoScene(const int _sceneNr) {
    GetDevices().UndoScene(_sceneNr);
  } // UndoScene

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

  const Device& DeviceReference::GetDevice() const {
    return m_Apartment->GetDeviceByDSID(m_DSID);
  } // GetDevice

  dsid_t DeviceReference::GetDSID() const {
    return m_DSID;
  } // GetID

  string DeviceReference::GetName() const {
    return GetDevice().GetName();
  } //GetName

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

  bool DeviceReference::IsOn() const {
    return GetDevice().IsOn();
  }

  bool DeviceReference::HasSwitch() const {
    return GetDevice().HasSwitch();
  }

  void DeviceReference::CallScene(const int _sceneNr) {
    GetDevice().CallScene(_sceneNr);
  } // CallScene

  void DeviceReference::SaveScene(const int _sceneNr) {
    GetDevice().SaveScene(_sceneNr);
  } // SaveScene

  void DeviceReference::UndoScene(const int _sceneNr) {
    GetDevice().UndoScene(_sceneNr);
  } // UndoScene


}
