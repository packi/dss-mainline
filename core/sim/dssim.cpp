#include "dssim.h"

#include "core/ds485const.h"
#include "core/base.h"
#include "core/logger.h"
#include "include/dsid_plugin.h"
#include "core/dss.h"
#include "core/DS485Interface.h"
#include "core/foreach.h"
#include "core/propertysystem.h"

#include <dlfcn.h>

#include <string>
#include <stdexcept>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace dss {

  //================================================== DSIDSimCreator

  class DSIDSimCreator : public DSIDCreator {
  public:
    DSIDSimCreator()
    : DSIDCreator("standard.simple")
    {}

    virtual ~DSIDSimCreator() {};

    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
      return new DSIDSim(_modulator, _dsid, _shortAddress);
    }
  };

  //================================================== DSIDSimCreator

  class DSIDSimSwitchCreator : public DSIDCreator {
  public:
    DSIDSimSwitchCreator()
    : DSIDCreator("standard.switch")
    {}

    virtual ~DSIDSimSwitchCreator() {};

    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
      return new DSIDSimSwitch(_modulator, _dsid, _shortAddress, 9);
    }
  };

  //================================================== DSIDPlugin

  class DSIDPlugin : public DSIDInterface {
  private:
    void* m_SOHandle;
    const int m_Handle;
    struct dsid_interface* m_Interface;
  public:
    DSIDPlugin(const DSModulatorSim& _simulator, const dsid_t _dsid, const devid_t _shortAddress, void* _soHandle, const int _handle)
    : DSIDInterface(_simulator, _dsid, _shortAddress),
      m_SOHandle(_soHandle),
      m_Handle(_handle)
    {
      struct dsid_interface* (*get_interface)();
      *(void**)(&get_interface) = dlsym(m_SOHandle, "dsid_get_interface");
      char* error;
      if((error = dlerror()) != NULL) {
        Logger::GetInstance()->Log("sim: error getting interface");
      }

      m_Interface = (*get_interface)();
      if(m_Interface == NULL) {
        Logger::GetInstance()->Log("sim: got a null interface");
      }
    }

    virtual int GetConsumption() {
      return 0;
    }

    virtual void CallScene(const int _sceneNr) {
      if(m_Interface->call_scene != NULL) {
        (*m_Interface->call_scene)(m_Handle, _sceneNr);
      }
    }

    virtual void SaveScene(const int _sceneNr) {
      if(m_Interface->save_scene != NULL) {
        (*m_Interface->save_scene)(m_Handle, _sceneNr);
      }
    }

    virtual void UndoScene(const int _sceneNr) {
      if(m_Interface->undo_scene != NULL) {
        (*m_Interface->undo_scene)(m_Handle, _sceneNr);
      }
    }

    virtual void IncreaseValue(const int _parameterNr = -1) {
      if(m_Interface->increase_value != NULL) {
        (*m_Interface->increase_value)(m_Handle, _parameterNr);
      }
    }

    virtual void DecreaseValue(const int _parameterNr = -1) {
      if(m_Interface->decrease_value != NULL) {
        (*m_Interface->decrease_value)(m_Handle, _parameterNr);
      }
    }

    virtual void Enable() {
      if(m_Interface->enable != NULL) {
        (*m_Interface->enable)(m_Handle);
      }
    }

    virtual void Disable() {
      if(m_Interface->disable != NULL) {
        (*m_Interface->disable)(m_Handle);
      }
    }

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) {
      if(m_Interface->start_dim != NULL) {
        (*m_Interface->start_dim)(m_Handle, _directionUp, _parameterNr);
      }
    }

    virtual void EndDim(const int _parameterNr = -1) {
      if(m_Interface->end_dim != NULL) {
        (*m_Interface->end_dim)(m_Handle, _parameterNr);
      }
    }

    virtual void SetValue(const double _value, int _parameterNr = -1) {
      if(m_Interface->set_value != NULL) {
        (*m_Interface->set_value)(m_Handle, _parameterNr, _value);
      }
    }

    virtual double GetValue(int _parameterNr = -1) const {
      if(m_Interface->get_value != NULL) {
        return (*m_Interface->get_value)(m_Handle, _parameterNr);
      } else {
        return 0.0;
      }
    }

    virtual uint8_t GetFunctionID() {
      // TODO: implement get function id for plugins
      return 0;
    }

    virtual void SetConfigParameter(const string& _name, const string& _value) {
      if(m_Interface->set_configuration_parameter != NULL) {
        (*m_Interface->set_configuration_parameter)(m_Handle, _name.c_str(), _value.c_str());
      }
    } // SetConfigParameter

    virtual string GetConfigParameter(const string& _name) const {
      // TODO: pass on
      return "";
    }


  }; // DSIDPlugin


  //================================================== DSIDPluginCreator

  class DSIDPluginCreator : public DSIDCreator {
  private:
    void* m_SOHandle;
    int (*createInstance)();
  public:
    DSIDPluginCreator(void* _soHandle, const char* _pluginName)
    : DSIDCreator(_pluginName),
      m_SOHandle(_soHandle)
    {
      *(void**)(&createInstance) = dlsym(m_SOHandle, "dsid_create_instance");
      char* error;
      if((error = dlerror()) != NULL) {
        Logger::GetInstance()->Log("sim: error getting pointer to dsid_create_instance");
      }
    }

    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
      int handle = (*createInstance)();
      return new DSIDPlugin(_modulator, _dsid, _shortAddress, m_SOHandle, handle);
    }
  }; // DSIDPluginCreator

  //================================================== DSSim

  DSSim::DSSim(DSS* _pDSS)
  : Subsystem(_pDSS, "DSSim")
  {}

  void DSSim::Initialize() {
    Subsystem::Initialize();
    m_DSIDFactory.RegisterCreator(new DSIDSimCreator());
    m_DSIDFactory.RegisterCreator(new DSIDSimSwitchCreator());

    GetDSS().GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "configfile", GetDSS().GetDataDirectory() + "sim.xml", true, false);


    LoadPlugins();
    LoadFromConfig();

    m_Initialized = true;
  } // Initialize


  void DSSim::LoadFromConfig() {
    const int theConfigFileVersion = 1;
    string filename = GetConfigPropertyBasePath() + "configfile";
    XMLDocumentFileReader reader(GetDSS().GetPropertySystem().GetStringValue(filename));
    XMLNode rootNode = reader.GetDocument().GetRootNode();

    if(rootNode.GetName() == "simulation") {
      string fileVersionStr = rootNode.GetAttributes()["version"];
      if(StrToIntDef(fileVersionStr, -1) == theConfigFileVersion) {
        XMLNodeList& modulators = rootNode.GetChildren();
        foreach(XMLNode& node, modulators) {
          if(node.GetName() == "modulator") {
            DSModulatorSim* modulator = NULL;
            try {
              modulator = new DSModulatorSim(this);
              modulator->InitializeFromNode(node);
              m_Modulators.push_back(modulator);
            } catch(runtime_error&) {
              delete modulator;
            }
          }
        }
      } else {
        Log("Version mismatch, or missing version attribute. Expected ''" + IntToString(theConfigFileVersion) + "'" + fileVersionStr + "' ");
      }
    } else {
      Log(filename + " must have a root-node named 'simulation'", lsFatal);
    }
  } // LoadFromConfig

  void DSSim::LoadPlugins() {
    fs::directory_iterator end_iter;
    try {
      for ( fs::directory_iterator dir_itr(GetDSS().GetDataDirectory() + "plugins");
            dir_itr != end_iter;
            ++dir_itr )
      {
        try {
          if (fs::is_regular(dir_itr->status())) {
            if(EndsWith(dir_itr->leaf(), ".so")) {
              Log("LoadPlugins: Trying to load '" + dir_itr->string() + "'", lsInfo);
              void* handle = dlopen(dir_itr->string().c_str(), RTLD_LAZY);
              if(handle == NULL) {
                Log("LoadPlugins: Could not load plugin \"" + dir_itr->leaf() + "\" message: " + dlerror(), lsError);
                continue;
              }

              dlerror();
              int (*version)();
              *(void**) (&version) = dlsym(handle, "dsid_getversion");
              char* error;
              if((error = dlerror()) != NULL) {
                 Log("LoadPlugins: could get version from \"" + dir_itr->leaf() + "\":" + error, lsError);
                 continue;
              }

              int ver = (*version)();
              if(ver != DSID_PLUGIN_API_VERSION) {
                Log("LoadPlugins: Versionmismatch (plugin: " + IntToString(ver) + " api:" + IntToString(DSID_PLUGIN_API_VERSION) + ")", lsError);
                continue;
              }

              const char* (*get_name)();
              *(void**)(&get_name) = dlsym(handle, "dsid_get_plugin_name");
              if((error = dlerror()) != NULL) {
                Log("LoadPlugins: could get name from \"" + dir_itr->leaf() + "\":" + error, lsError);
                continue;
              }
              const char* pluginName = (*get_name)();
              if(pluginName == NULL) {
                Log("LoadPlugins: could get name from \"" + dir_itr->leaf() + "\":" + error, lsError);
                continue;
              }
              Log("LoadPlugins: Plugin provides " + string(pluginName), lsInfo);
              m_DSIDFactory.RegisterCreator(new DSIDPluginCreator(handle, pluginName));
            }
          }
        } catch (const std::exception & ex) {
          Log("LoadPlugins: Cought exception while loading " + dir_itr->leaf() + " '" + ex.what() + "'", lsError);
        }
      }
    } catch(const std::exception& ex) {
      Log(string("Error loading plugins: '") + ex.what() + "'");
    }
  } // LoadPlugins

  bool DSSim::isReady() {
    return m_Initialized;
  } // Ready

  bool DSSim::isSimAddress(const uint8_t _address) {
    foreach(DSModulatorSim& modulator, m_Modulators) {
      if(modulator.GetID() == _address) {
        return true;
      }
    }
    return false;
  } // isSimAddress

  void DSSim::process(DS485Frame& _frame) {
    foreach(DSModulatorSim& modulator, m_Modulators) {
      modulator.Process(_frame);
    }
  } // process

  void DSSim::DistributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    DS485FrameProvider::DistributeFrame(_frame);
  } // DistributeFrame

  //================================================== DSModulatorSim

  DSModulatorSim::DSModulatorSim(DSSim* _pSimulation)
  : m_pSimulation(_pSimulation),
    m_EnergyLevelOrange(200),
    m_EnergyLevelRed(400)
  {
    m_ModulatorDSID = dsid_t(0, SimulationPrefix);
    m_ID = 70;
    m_Name = "Simulated dSM";
  } // DSModulatorSim

  void DSModulatorSim::Log(const string& _message, aLogSeverity _severity) {
    m_pSimulation->Log(_message, _severity);
  } // Log

  bool DSModulatorSim::InitializeFromNode(XMLNode& _node) {
    HashMapConstStringString& attrs = _node.GetAttributes();
    if(attrs["busid"].size() != 0) {
      m_ID = StrToIntDef(attrs["busid"], 70);
    }
    if(attrs["dsid"].size() != 0) {
      m_ModulatorDSID = dsid_t::FromString(attrs["dsid"]);
      m_ModulatorDSID.lower |= SimulationPrefix;
    }
    m_EnergyLevelOrange = StrToIntDef(attrs["orange"], m_EnergyLevelOrange);
    m_EnergyLevelRed = StrToIntDef(attrs["red"], m_EnergyLevelRed);
    try {
      XMLNode& nameNode = _node.GetChildByName("name");
      if(!nameNode.GetChildren().empty()) {
        m_Name = nameNode.GetChildren()[0].GetContent();
      }
    } catch(XMLException&) {
    }

    XMLNodeList& nodes = _node.GetChildren();
    LoadDevices(nodes, 0);
    LoadGroups(nodes, 0);
    LoadZones(nodes);
    return true;
  } // InitializeFromNode

  void DSModulatorSim::LoadDevices(XMLNodeList& _nodes, const int _zoneID) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "device") {
        HashMapConstStringString& attrs = iNode->GetAttributes();
        dsid_t dsid = NullDSID;
        int busid = -1;
        if(!attrs["dsid"].empty()) {
          dsid = dsid_t::FromString(attrs["dsid"]);
          dsid.lower |= SimulationPrefix;
        }
        if(!attrs["busid"].empty()) {
          busid = StrToInt(attrs["busid"]);
        }
        if((dsid == NullDSID) || (busid == -1)) {
          Log("missing dsid or busid of device");
          continue;
        }
        string type = "standard.simple";
        if(!attrs["type"].empty()) {
          type = attrs["type"];
        }

        DSIDInterface* newDSID = m_pSimulation->getDSIDFactory().CreateDSID(type, dsid, busid, *this);
        try {
          foreach(XMLNode& iParam, iNode->GetChildren()) {
            if(iParam.GetName() == "parameter") {
              string paramName = iParam.GetAttributes()["name"];
              string paramValue = iParam.GetChildren()[0].GetContent();
              if(paramName.empty()) {
                Log("Missing attribute name of parameter node");
                continue;
              }
              Log("LoadDevices:   Found parameter '" + paramName + "' with value '" + paramValue + "'");
              newDSID->SetConfigParameter(paramName, paramValue);
            } else if(iParam.GetName() == "name") {
              m_DeviceNames[busid] = iParam.GetChildren()[0].GetContent();
            }
          }
        } catch(runtime_error&) {
        }
        if(newDSID != NULL) {
          m_SimulatedDevices.push_back(newDSID);
          if(_zoneID != 0) {
            m_Zones[_zoneID].push_back(newDSID);
          }
          m_DeviceZoneMapping[newDSID] = _zoneID;
          newDSID->SetZoneID(_zoneID);

          DSIDSimSwitch* sw = dynamic_cast<DSIDSimSwitch*>(newDSID);
          if(sw != NULL) {
            Log("LoadDevices:   it's a switch");
            if(attrs["bell"] == "true") {
              sw->SetIsBell(true);
              Log("LoadDevices:   switch is bell");
            }
          }

          newDSID->Initialize();
          Log("LoadDevices: found device");
        } else {
          Log("LoadDevices: could not create instance for type \"" + type + "\"");
        }
      }
    }
  } // LoadDevices

  void DSModulatorSim::LoadGroups(XMLNodeList& _nodes, const int _zoneID) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "group") {
        HashMapConstStringString& attrs = iNode->GetAttributes();
        if(!attrs["id"].empty()) {
          int groupID = StrToIntDef(attrs["id"], -1);
          XMLNodeList& children = iNode->GetChildren();
          for(XMLNodeList::iterator iChildNode = children.begin(), e = children.end();
              iChildNode != e; ++iChildNode)
          {
            if(iChildNode->GetName() == "device") {
              attrs = iChildNode->GetAttributes();
              if(!attrs["busid"].empty()) {
                unsigned long busID = StrToUInt(attrs["busid"]);
                DSIDInterface& dev = LookupDevice(busID);
                AddDeviceToGroup(&dev, groupID);
                Log("LoadGroups: Adding device " + attrs["busid"] + " to group " + IntToString(groupID) + " in zone " + IntToString(_zoneID));
              }
            }
          }
        } else {
          Log("LoadGroups: Could not find attribute id of group, skipping entry", lsError);
        }
      }
    }
  } // LoadGroups

  void DSModulatorSim::AddDeviceToGroup(DSIDInterface* _device, int _groupID) {
    m_DevicesOfGroupInZone[pair<const int, const int>(_device->GetZoneID(), _groupID)].push_back(_device);
    m_GroupsPerDevice[_device->GetShortAddress()].push_back(_groupID);
  } // AddDeviceToGroup

  void DSModulatorSim::LoadZones(XMLNodeList& _nodes) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "zone") {
        HashMapConstStringString& attrs = iNode->GetAttributes();
        int zoneID = -1;
        if(!attrs["id"].empty()) {
          zoneID = StrToIntDef(attrs["id"], -1);
        }
        if(zoneID != -1) {
          Log("LoadZones: found zone (" + IntToString(zoneID) + ")");
          LoadDevices(iNode->GetChildren(), zoneID);
          LoadGroups(iNode->GetChildren(), zoneID);
        } else {
          Log("LoadZones: could not find/parse id for zone");
        }
      }
    }
  } // LoadZones

  void DSModulatorSim::DeviceCallScene(int _deviceID, const int _sceneID) {
    LookupDevice(_deviceID).CallScene(_sceneID);
  } // DeviceCallScene

  void DSModulatorSim::GroupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
	vector<DSIDInterface*> dsids;
	m_LastCalledSceneForZoneAndGroup[make_pair(_zoneID, _groupID)] = _sceneID;
	if(_groupID == GroupIDBroadcast) {
	  if(_zoneID == 0) {
	    dsids = m_SimulatedDevices;
	  } else {
      dsids = m_Zones[_zoneID];
	  }
	} else {
      pair<const int, const int> zonesGroup(_zoneID, _groupID);
      if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
        dsids = m_DevicesOfGroupInZone[zonesGroup];
      }
	}
    for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
        iDSID != e; ++iDSID)
    {
      (*iDSID)->CallScene(_sceneID);
    }
  } // GroupCallScene

  void DSModulatorSim::DeviceSaveScene(int _deviceID, const int _sceneID) {
    LookupDevice(_deviceID).SaveScene(_sceneID);
  } // DeviceSaveScene

  void DSModulatorSim::GroupSaveScene(const int _zoneID, const int _groupID, const int _sceneID) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->SaveScene(_sceneID);
      }
    }
  } // GroupSaveScene

  void DSModulatorSim::DeviceUndoScene(int _deviceID, const int _sceneID) {
    LookupDevice(_deviceID).UndoScene(_sceneID);
  } // DeviceUndoScene

  void DSModulatorSim::GroupUndoScene(const int _zoneID, const int _groupID, const int _sceneID) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->UndoScene(_sceneID);
      }
    }
  } // GroupUndoScene

  void DSModulatorSim::GroupStartDim(const int _zoneID, const int _groupID, bool _up, int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->StartDim(_up, _parameterNr);
      }
    }
  } // GroupStartDim

  void DSModulatorSim::GroupEndDim(const int _zoneID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->EndDim(_parameterNr);
      }
    }
  } // GroupEndDim

  void DSModulatorSim::GroupDecValue(const int _zoneID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->DecreaseValue(_parameterNr);
      }
    }
  } // GroupDecValue

  void DSModulatorSim::GroupIncValue(const int _zoneID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->IncreaseValue(_parameterNr);
      }
    }
  } // GroupIncValue

  void DSModulatorSim::GroupSetValue(const int _zoneID, const int _groupID, const int _value) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->SetValue(_value);
      }
    }
  } // GroupSetValue

  void DSModulatorSim::Process(DS485Frame& _frame) {
    const uint8_t HeaderTypeToken = 0;
    const uint8_t HeaderTypeCommand = 1;

    try {
      DS485Header& header = _frame.GetHeader();
      if(!(header.GetDestination() == m_ID || header.IsBroadcast())) {
        return;
      }
      if(header.GetType() == HeaderTypeToken) {
        // Transmit pending things
      } else if(header.GetType() == HeaderTypeCommand) {
        DS485CommandFrame& cmdFrame = dynamic_cast<DS485CommandFrame&>(_frame);
        PayloadDissector pd(cmdFrame.GetPayload());
        if((cmdFrame.GetCommand() == CommandRequest) && !pd.IsEmpty()) {
          int cmdNr = pd.Get<uint8_t>();
          boost::shared_ptr<DS485CommandFrame> response;
          switch(cmdNr) {
            case FunctionDeviceCallScene:
              {
                devid_t devID = pd.Get<devid_t>();
                int sceneID = pd.Get<uint16_t>();
                DeviceCallScene(devID, sceneID);
                DistributeFrame(boost::shared_ptr<DS485CommandFrame>(CreateAck(cmdFrame, cmdNr)));
              }
              break;
            case FunctionGroupCallScene:
              {
                uint8_t zoneID = pd.Get<uint16_t>();
                uint8_t groupID = pd.Get<uint16_t>();
                uint8_t sceneID = pd.Get<uint16_t>();
                GroupCallScene(zoneID, groupID, sceneID);
              }
              break;
            case FunctionDeviceSaveScene:
              {
                devid_t devID = pd.Get<devid_t>();
                int sceneID = pd.Get<uint16_t>();
                DeviceSaveScene(devID, sceneID);
                DistributeFrame(boost::shared_ptr<DS485CommandFrame>(CreateAck(cmdFrame, cmdNr)));
              }
              break;
            case FunctionGroupSaveScene:
              {
                uint8_t zoneID = pd.Get<uint16_t>();
                uint8_t groupID = pd.Get<uint16_t>();
                uint8_t sceneID = pd.Get<uint16_t>();
                GroupSaveScene(zoneID, groupID, sceneID);
              }
              break;
            case FunctionDeviceUndoScene:
              {
                devid_t devID = pd.Get<devid_t>();
                int sceneID = pd.Get<uint16_t>();
                DeviceUndoScene(devID, sceneID);
                DistributeFrame(boost::shared_ptr<DS485CommandFrame>(CreateAck(cmdFrame, cmdNr)));
              }
              break;
            case FunctionGroupUndoScene:
              {
                uint16_t zoneID = pd.Get<uint16_t>();
                uint16_t groupID = pd.Get<uint16_t>();
                uint16_t sceneID = pd.Get<uint16_t>();
                GroupUndoScene(zoneID, groupID, sceneID);
              }
              break;
            case FunctionGroupIncreaseValue:
              {
                uint16_t zoneID = pd.Get<uint16_t>();
                uint16_t groupID = pd.Get<uint16_t>();
                GroupIncValue(zoneID, groupID, 0);
              }
              break;
            case FunctionGroupDecreaseValue:
              {
                uint16_t zoneID = pd.Get<uint16_t>();
                uint16_t groupID = pd.Get<uint16_t>();
                GroupDecValue(zoneID, groupID, 0);
              }
              break;
            case FunctionGroupSetValue:
              {
                uint16_t zoneID = pd.Get<uint16_t>();
                uint16_t groupID = pd.Get<uint16_t>();
                uint16_t value = pd.Get<uint16_t>();
                GroupSetValue(zoneID, groupID, value);
              }
              break;
            case FunctionDeviceIncreaseValue:
              {
                uint16_t devID = pd.Get<uint16_t>();
                DSIDInterface& dev = LookupDevice(devID);
                dev.IncreaseValue();
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(1);
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceDecreaseValue:
              {
                uint16_t devID = pd.Get<uint16_t>();
                DSIDInterface& dev = LookupDevice(devID);
                dev.DecreaseValue();
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(1);
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceGetFunctionID:
              {
                devid_t devID = pd.Get<devid_t>();
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(0x0001); // everything ok
                response->GetPayload().Add<uint16_t>(LookupDevice(devID).GetFunctionID());
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceSetValue:
              {
                uint16_t devID = pd.Get<uint16_t>();
                DSIDInterface& dev = LookupDevice(devID);
                uint16_t value = pd.Get<uint16_t>();
                dev.SetValue(value);
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(1);
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceGetName:
              {
                devid_t devID = pd.Get<devid_t>();
                string name = m_DeviceNames[devID];
                response = CreateResponse(cmdFrame, cmdNr);

              }
              break;
            case FunctionDeviceGetParameterValue:
              {
                int devID = pd.Get<uint16_t>();
                int paramID = pd.Get<uint16_t>();
                double result = LookupDevice(devID).GetValue(paramID);
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(static_cast<uint8_t>(result));
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceSetParameterValue:
              {
                int devID = pd.Get<uint16_t>();
                int paramID = pd.Get<uint16_t>();
                uint8_t value = pd.Get<uint16_t>();
                LookupDevice(devID).SetValue(value, paramID);
                DistributeFrame(boost::shared_ptr<DS485CommandFrame>(CreateAck(cmdFrame, cmdNr)));
              }
              break;
            case FunctionModulatorGetZonesSize:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(m_Zones.size());
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorGetZoneIdForInd:
              {
                uint8_t index = pd.Get<uint16_t>();
                map< const int, vector<DSIDInterface*> >::iterator it = m_Zones.begin();
                advance(it, index);
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(it->first);
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorCountDevInZone:
              {
                uint8_t index = pd.Get<uint16_t>();
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(m_Zones[index].size());
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorDevKeyInZone:
              {
                uint8_t zoneID = pd.Get<uint16_t>();
                uint8_t deviceIndex = pd.Get<devid_t>();
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(m_Zones[zoneID].at(deviceIndex)->GetShortAddress());
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorGetGroupsSize:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                int zoneID = pd.Get<uint16_t>();
                IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInZone.begin();
                IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInZone.end();
                int result = 0;
                while(it != end) {
                  if(it->first.first == zoneID) {
                    result++;
                  }
                  it++;
                }
                response->GetPayload().Add<uint16_t>(result);
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorGetDSID:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<dsid_t>(m_ModulatorDSID);
                DistributeFrame(response);
              }
              break;
            case FunctionGroupGetDeviceCount:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                int zoneID = pd.Get<uint16_t>();
                int groupID = pd.Get<uint16_t>();
                int result = m_DevicesOfGroupInZone[pair<const int, const int>(zoneID, groupID)].size();
                response->GetPayload().Add<uint16_t>(result);
                DistributeFrame(response);
              }
              break;
            case FunctionGroupGetDevKeyForInd:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                int zoneID = pd.Get<uint16_t>();
                int groupID = pd.Get<uint16_t>();
                int index = pd.Get<uint16_t>();
                int result = m_DevicesOfGroupInZone[pair<const int, const int>(zoneID, groupID)].at(index)->GetShortAddress();
                response->GetPayload().Add<uint16_t>(result);
                DistributeFrame(response);
              }
              break;
            case FunctionGroupGetLastCalledScene:
              {
                int zoneID = pd.Get<uint16_t>();
                int groupID = pd.Get<uint16_t>();
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(m_LastCalledSceneForZoneAndGroup[make_pair(zoneID, groupID)]);
                DistributeFrame(response);
              }
              break;
            case FunctionZoneGetGroupIdForInd:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                int zoneID = pd.Get<uint16_t>();
                int groupIndex= pd.Get<uint16_t>();

                IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInZone.begin();
                IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInZone.end();
                int result = -1;
                while(it != end) {
                  if(it->first.first == zoneID) {
                    if(groupIndex == 0) {
                      result = it->first.second; // yes, that's the group id
                      break;
                    }
                    groupIndex--;
                  }
                  it++;
                }
                response->GetPayload().Add<uint16_t>(result);
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceGetOnOff:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                int devID = pd.Get<uint16_t>();
                DSIDInterface& dev = LookupDevice(devID);
                response->GetPayload().Add<uint16_t>(dev.IsTurnedOn());
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceGetDSID:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                int devID = pd.Get<uint16_t>();
                DSIDInterface& dev = LookupDevice(devID);
                response->GetPayload().Add<uint16_t>(1); // return-code (device found, all well)
                response->GetPayload().Add<dsid_t>(dev.GetDSID());
                DistributeFrame(response);
              }
              break;
            case FunctionGetTypeRequest:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                // Type 2 bytes (high, low)
                response->GetPayload().Add<uint8_t>(0x00);
                response->GetPayload().Add<uint8_t>(0x01);
                // HW-Version 2 bytes (high, low)
                response->GetPayload().Add<uint8_t>(0x00);
                response->GetPayload().Add<uint8_t>(0x01);
                // SW-Version 2 bytes (high, low)
                response->GetPayload().Add<uint8_t>(0x00);
                response->GetPayload().Add<uint8_t>(0x01);
                // free text
                response->GetPayload().Add<uint8_t>('d');
                response->GetPayload().Add<uint8_t>('S');
                response->GetPayload().Add<uint8_t>('M');
                response->GetPayload().Add<uint8_t>('S');
                response->GetPayload().Add<uint8_t>('i');
                response->GetPayload().Add<uint8_t>('m');
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceGetGroups:
              {
                devid_t devID = pd.Get<uint16_t>();
                LookupDevice(devID);
                vector<int>& groupsList = m_GroupsPerDevice[devID];


                // format: (8,7,6,5,4,3,2,1) (16,15,14,13,12,11,10,9), etc...
                unsigned char bytes[8];
                foreach(int groupID, groupsList) {
                  if(groupID != 0) {
                    int iBit = (groupID - 1) % 8;
                    int iByte = (groupID -1) / 8;
                    bytes[iByte] |= 1 << iBit;
                  }
                }

                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(1);
                for(int iByte = 0; iByte < 8; iByte++) {
                  response->GetPayload().Add<uint8_t>(bytes[iByte]);
                }
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceAddToGroup:
              {
                devid_t devID = pd.Get<uint16_t>();
                DSIDInterface& dev = LookupDevice(devID);
                int groupID = pd.Get<uint16_t>();
                AddDeviceToGroup(&dev, groupID);
                response->GetPayload().Add<uint16_t>(1);
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorGetPowerConsumption:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                uint32_t val = 0;
                foreach(DSIDInterface* interface, m_SimulatedDevices) {
                  val += interface->GetConsumption();
                }
                response->GetPayload().Add<uint32_t>(val);
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorGetEnergyMeterValue:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(0);
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorAddZone:
              {
                uint8_t zoneID = pd.Get<uint16_t>();
                response = CreateResponse(cmdFrame, cmdNr);
                bool isValid = true;
                for(map< const int, vector<DSIDInterface*> >::iterator iZoneEntry = m_Zones.begin(), e = m_Zones.end();
                    iZoneEntry != e; ++iZoneEntry)
                {
                  if(iZoneEntry->first == zoneID) {
                    response->GetPayload().Add<uint16_t>(static_cast<uint16_t>(-7));
                    isValid = false;
                    break;
                  }
                }
                if(isValid) {
                  // make the zone visible in the map
                  m_Zones[zoneID].size();
                  response->GetPayload().Add<uint16_t>(1);
                }
                DistributeFrame(response);
              }
              break;
            case FunctionDeviceSetZoneID:
              {
                uint8_t zoneID = pd.Get<uint16_t>();
                devid_t devID = pd.Get<devid_t>();
                DSIDInterface& dev = LookupDevice(devID);

                int oldZoneID = m_DeviceZoneMapping[&dev];
                vector<DSIDInterface*>::iterator oldEntry = find(m_Zones[oldZoneID].begin(), m_Zones[oldZoneID].end(), &dev);
                m_Zones[oldZoneID].erase(oldEntry);
                m_Zones[zoneID].push_back(&dev);
                m_DeviceZoneMapping[&dev] = zoneID;
                dev.SetZoneID(zoneID);
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(1);
                DistributeFrame(response);
              }
              break;
            case FunctionModulatorGetEnergyBorder:
              {
                response = CreateResponse(cmdFrame, cmdNr);
                response->GetPayload().Add<uint16_t>(m_EnergyLevelOrange);
                response->GetPayload().Add<uint16_t>(m_EnergyLevelRed);
                DistributeFrame(response);
              }
              break;
            default:
              Logger::GetInstance()->Log("Invalid function id for sim: " + IntToString(cmdNr), lsError);
          }
        }
      }
    } catch(runtime_error& e) {
      Logger::GetInstance()->Log(string("DSModulatorSim: Exeption while processing packet. Message: '") + e.what() + "'");
    }
  } // Process

  void DSModulatorSim::DistributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    m_pSimulation->DistributeFrame(_frame);
  } // DistributeFrame

  boost::shared_ptr<DS485CommandFrame> DSModulatorSim::CreateReply(DS485CommandFrame& _request) {
    boost::shared_ptr<DS485CommandFrame> result(new DS485CommandFrame());
    result->GetHeader().SetDestination(_request.GetHeader().GetSource());
    result->GetHeader().SetSource(m_ID);
    result->GetHeader().SetBroadcast(false);
    result->GetHeader().SetCounter(_request.GetHeader().GetCounter());
    return result;
  } // CreateReply


  boost::shared_ptr<DS485CommandFrame> DSModulatorSim::CreateAck(DS485CommandFrame& _request, uint8_t _functionID) {
    boost::shared_ptr<DS485CommandFrame> result = CreateReply(_request);
    result->SetCommand(CommandAck);
    result->GetPayload().Add(_functionID);
    return result;
  }

  boost::shared_ptr<DS485CommandFrame> DSModulatorSim::CreateResponse(DS485CommandFrame& _request, uint8_t _functionID) {
    boost::shared_ptr<DS485CommandFrame> result = CreateReply(_request);
    result->SetCommand(CommandResponse);
    result->GetPayload().Add(_functionID);
    return result;
  } // CreateResponse


  DSIDInterface& DSModulatorSim::LookupDevice(const devid_t _shortAddress) {
    for(vector<DSIDInterface*>::iterator ipSimDev = m_SimulatedDevices.begin(); ipSimDev != m_SimulatedDevices.end(); ++ipSimDev) {
      if((*ipSimDev)->GetShortAddress() == _shortAddress)  {
        return **ipSimDev;
      }
    }
    throw runtime_error(string("could not find device with id: ") + IntToString(_shortAddress));
  } // LookupDevice

  int DSModulatorSim::GetID() const {
    return m_ID;
  } // GetID
  DSIDInterface* DSModulatorSim::GetSimulatedDevice(const dsid_t _dsid) {
    for(vector<DSIDInterface*>::iterator iDSID = m_SimulatedDevices.begin(), e = m_SimulatedDevices.end();
        iDSID != e; ++iDSID)
    {
      if((*iDSID)->GetDSID() == _dsid) {
        return *iDSID;
      }
    }
    return NULL;
  } // GetSimulatedDevice


  //================================================== DSIDCreator

  DSIDCreator::DSIDCreator(const string& _identifier)
  : m_Identifier(_identifier)
  {} // ctor


  //================================================== DSIDFactory

  DSIDInterface* DSIDFactory::CreateDSID(const string& _identifier, const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
    boost::ptr_vector<DSIDCreator>::iterator
    iCreator = m_RegisteredCreators.begin(),
    e = m_RegisteredCreators.end();
    while(iCreator != e) {
      if(iCreator->GetIdentifier() == _identifier) {
        return iCreator->CreateDSID(_dsid, _shortAddress, _modulator);
      }
      ++iCreator;
    }
    Logger::GetInstance()->Log(string("Could not find creator for DSID type '") + _identifier + "'");
    throw new runtime_error(string("Could not find creator for DSID type '") + _identifier + "'");
  }

  void DSIDFactory::RegisterCreator(DSIDCreator* _creator) {
    m_RegisteredCreators.push_back(_creator);
  }

}
