#include "dssim.h"

#include "../ds485const.h"
#include "../base.h"
#include "../logger.h"
#include "include/dsid_plugin.h"

#include <dlfcn.h>

#include <iostream>

#include <string>
#include <stdexcept>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace dss {

  //================================================== DSIDSimCreator

  class DSIDSimCreator : public DSIDCreator {
  public:
    DSIDSimCreator(const DSModulatorSim& _simulator)
    : DSIDCreator(_simulator, "standard.simple")
    {}

    virtual ~DSIDSimCreator() {};

    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress) {
      return new DSIDSim(GetSimulator(), _dsid, _shortAddress);
    }
  };

  //================================================== DSIDSimCreator

  class DSIDSimSwitchCreator : public DSIDCreator {
  public:
    DSIDSimSwitchCreator(const DSModulatorSim& _simulator)
    : DSIDCreator(_simulator, "standard.switch")
    {}

    virtual ~DSIDSimSwitchCreator() {};

    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress) {
      return new DSIDSimSwitch(GetSimulator(), _dsid, _shortAddress, 9);
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

    virtual void CallScene(const int _sceneNr) {
      (*m_Interface->call_scene)(m_Handle, _sceneNr);
    }

    virtual void SaveScene(const int _sceneNr) {
      (*m_Interface->save_scene)(m_Handle, _sceneNr);
    }

    virtual void UndoScene(const int _sceneNr) {
      (*m_Interface->undo_scene)(m_Handle, _sceneNr);
    }

    virtual void IncreaseValue(const int _parameterNr = -1) {
      (*m_Interface->increase_value)(m_Handle, _parameterNr);
    }

    virtual void DecreaseValue(const int _parameterNr = -1) {
      (*m_Interface->decrease_value)(m_Handle, _parameterNr);
    }

    virtual void Enable() {
      (*m_Interface->enable)(m_Handle);
    }

    virtual void Disable() {
      (*m_Interface->disable)(m_Handle);
    }

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) {
      (*m_Interface->start_dim)(m_Handle, _directionUp, _parameterNr);
    }

    virtual void EndDim(const int _parameterNr = -1) {
      (*m_Interface->end_dim)(m_Handle, _parameterNr);
    }

    virtual void SetValue(const double _value, int _parameterNr = -1) {
      (*m_Interface->set_value)(m_Handle, _parameterNr, _value);
    }

    virtual double GetValue(int _parameterNr = -1) const {
      return (*m_Interface->get_value)(m_Handle, _parameterNr);
    }

    virtual uint8 GetFunctionID() {
      // TODO: implement get function id for plugins
      return 0;
    }

  }; // DSIDPlugin


  //================================================== DSIDPluginCreator

  class DSIDPluginCreator : public DSIDCreator {
  private:
    void* m_SOHandle;
    int (*createInstance)();
  public:
    DSIDPluginCreator(const DSModulatorSim& _simulator, void* _soHandle, const char* _pluginName)
    : DSIDCreator(_simulator, _pluginName),
      m_SOHandle(_soHandle)
    {
      *(void**)(&createInstance) = dlsym(m_SOHandle, "dsid_create_instance");
      char* error;
      if((error = dlerror()) != NULL) {
        Logger::GetInstance()->Log("sim: error getting pointer to dsid_create_instance");
      }
    }

    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress) {
      int handle = (*createInstance)();
      return new DSIDPlugin(GetSimulator(), _dsid, _shortAddress, m_SOHandle, handle);
    }
  }; // DSIDPluginCreator

  //================================================== DSModulatorSim

  DSModulatorSim::DSModulatorSim()
  {
    m_ModulatorDSID = SimulationPrefix | 0x0000FFFF;
  } // DSModulatorSim

  void DSModulatorSim::Initialize() {
    m_ID = 70;
    m_DSIDFactory.RegisterCreator(new DSIDSimCreator(*this));
    m_DSIDFactory.RegisterCreator(new DSIDSimSwitchCreator(*this));
    LoadPlugins();

    LoadFromConfig();
  } // Initialize

  void DSModulatorSim::LoadPlugins() {
    fs::directory_iterator end_iter;
     for ( fs::directory_iterator dir_itr("data/plugins");
           dir_itr != end_iter;
           ++dir_itr )
     {
       try {
         if (fs::is_regular(dir_itr->status())) {
           if(EndsWith(dir_itr->leaf(), ".so")) {
             void* handle = dlopen(dir_itr->string().c_str(), RTLD_LAZY);
             if(handle == NULL) {
               Logger::GetInstance()->Log(string("Sim: Could not load plugin \"") + dir_itr->leaf() + "\" message: " + dlerror());
               continue;
             }

             dlerror();
             int (*version)();
             *(void**) (&version) = dlsym(handle, "dsid_getversion");
             char* error;
             if((error = dlerror()) != NULL) {
                Logger::GetInstance()->Log(string("Sim: could get version from \"") + dir_itr->leaf() + "\":" + error);
                continue;
             }

             int ver = (*version)();
             if(ver != DSID_PLUGIN_API_VERSION) {
               Logger::GetInstance()->Log(string("Sim: Versionmismatch (plugin: ") + IntToString(ver) + " api:" + IntToString(DSID_PLUGIN_API_VERSION) + ")");
               continue;
             }

             const char* (*get_name)();
             *(void**)(&get_name) = dlsym(handle, "dsid_get_plugin_name");
             if((error = dlerror()) != NULL) {
                Logger::GetInstance()->Log(string("Sim: could get name from \"") + dir_itr->leaf() + "\":" + error);
                continue;
             }
             const char* pluginName = (*get_name)();
             if(pluginName == NULL) {
               Logger::GetInstance()->Log(string("Sim: could get name from \"") + dir_itr->leaf() + "\":" + error);
               continue;
             }
             Logger::GetInstance()->Log(string("Sim: Plugin provides ") + pluginName);
             m_DSIDFactory.RegisterCreator(new DSIDPluginCreator(*this, handle, pluginName));
           }
         }
       } catch (const std::exception & ex) {
         Logger::GetInstance()->Log(dir_itr->leaf() + " " + ex.what());
       }
     }
    } // LoadPlugins

  void DSModulatorSim::LoadFromConfig() {
    XMLDocumentFileReader reader("data/sim.xml");
    XMLNode rootNode = reader.GetDocument().GetRootNode();

    if(rootNode.GetName() == "modulator") {
      HashMapConstStringString& attrs = rootNode.GetAttributes();
      if(attrs["busid"].size() != 0) {
        m_ID = StrToIntDef(attrs["busid"], 70);
      }
      if(attrs["dsid"].size() != 0) {
        m_ModulatorDSID = SimulationPrefix | StrToIntDef(attrs["dsid"], 50);
      }

      XMLNodeList& nodes = rootNode.GetChildren();
      LoadDevices(nodes, 0);
      LoadGroups(nodes, 0);
      LoadRooms(nodes);
    }

  } // LoadFromConfig

  void DSModulatorSim::LoadDevices(XMLNodeList& _nodes, const int _roomID) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "device") {
        HashMapConstStringString& attrs = iNode->GetAttributes();
        dsid_t dsid = 0xFFFFFFFF;
        int busid = -1;
        if(attrs["dsid"].size() > 0) {
          dsid = SimulationPrefix | StrToUInt(attrs["dsid"]);
        }
        if(attrs["busid"].size() > 0) {
          busid = StrToInt(attrs["busid"]);
        }
        if((dsid == 0xFFFFFFFF) || (busid == -1)) {
          Logger::GetInstance()->Log("Sim: missing dsid or busid of device");
          continue;
        }
        string type = "standard.simple";
        if(attrs["type"].size() != 0) {
          type = attrs["type"];
        }

        DSIDInterface* newDSID = m_DSIDFactory.CreateDSID(type, dsid, busid);
        if(newDSID != NULL) {
          m_SimulatedDevices.push_back(newDSID);
          m_Rooms[_roomID].push_back(newDSID);
          Logger::GetInstance()->Log("Sim: found device");
        } else {
          Logger::GetInstance()->Log(string("Sim: could not create instance for type \"") + type + "\"");
        }
      }
    }
  } // LoadDevices

  void DSModulatorSim::LoadGroups(XMLNodeList& _nodes, const int _roomID) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "group") {
        HashMapConstStringString& attrs = iNode->GetAttributes();
        if(attrs["id"].size() > 0) {
          int groupID = StrToIntDef(attrs["id"], -1);
          XMLNodeList& children = iNode->GetChildren();
          for(XMLNodeList::iterator iChildNode = children.begin(), e = children.end();
              iChildNode != e; ++iChildNode)
          {
            if(iChildNode->GetName() == "device") {
              attrs = iChildNode->GetAttributes();
              if(attrs["busid"].size() > 0) {
                DSIDInterface& dev = LookupDevice(StrToUInt(attrs["busid"]));
                m_DevicesOfGroupInRoom[pair<const int, const int>(_roomID, groupID)].push_back(&dev);
              }
            }
          }
        } else {
          Logger::GetInstance()->Log("Sim: Could not find attribute id of group, skipping entry", lsError);
        }
      }
    }
  } // LoadGroups

  void DSModulatorSim::LoadRooms(XMLNodeList& _nodes) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "room") {
        HashMapConstStringString& attrs = iNode->GetAttributes();
        int roomID = -1;
        if(attrs["id"].size() > 0) {
          roomID = StrToIntDef(attrs["id"], -1);
        }
        if(roomID != -1) {
          Logger::GetInstance()->Log("Sim: found room");
          LoadDevices(iNode->GetChildren(), roomID);
          LoadGroups(iNode->GetChildren(), roomID);
        } else {
          Logger::GetInstance()->Log("Sim: could not find/parse id for room");
        }
      }
    }
  } // LoadRooms

  void DSModulatorSim::DeviceCallScene(int _deviceID, const int _sceneID) {
    LookupDevice(_deviceID).CallScene(_sceneID);
  }

  void DSModulatorSim::GroupCallScene(const int _roomID, const int _groupID, const int _sceneID) {
    pair<const int, const int> roomsGroup(_roomID, _groupID);
    if(m_DevicesOfGroupInRoom.find(roomsGroup) != m_DevicesOfGroupInRoom.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInRoom[roomsGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->CallScene(_sceneID);
      }
    }
  }

  void DSModulatorSim::GroupStartDim(const int _roomID, const int _groupID, bool _up, int _parameterNr) {
    pair<const int, const int> roomsGroup(_roomID, _groupID);
    if(m_DevicesOfGroupInRoom.find(roomsGroup) != m_DevicesOfGroupInRoom.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInRoom[roomsGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->StartDim(_up, _parameterNr);
      }
    }
  } // GroupStartDim

  void DSModulatorSim::GroupEndDim(const int _roomID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> roomsGroup(_roomID, _groupID);
    if(m_DevicesOfGroupInRoom.find(roomsGroup) != m_DevicesOfGroupInRoom.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInRoom[roomsGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->EndDim(_parameterNr);
      }
    }
  } // GroupEndDim

  void DSModulatorSim::GroupDecValue(const int _roomID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> roomsGroup(_roomID, _groupID);
    if(m_DevicesOfGroupInRoom.find(roomsGroup) != m_DevicesOfGroupInRoom.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInRoom[roomsGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->DecreaseValue(_parameterNr);
      }
    }
  } // GroupDecValue

  void DSModulatorSim::GroupIncValue(const int _roomID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> roomsGroup(_roomID, _groupID);
    if(m_DevicesOfGroupInRoom.find(roomsGroup) != m_DevicesOfGroupInRoom.end()) {
      vector<DSIDInterface*> dsids = m_DevicesOfGroupInRoom[roomsGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->IncreaseValue(_parameterNr);
      }
    }
  } // GroupIncValue

  void DSModulatorSim::Process(DS485Frame& _frame) {
    const uint8 HeaderTypeToken = 0;
    const uint8 HeaderTypeCommand = 1;

    DS485Header& header = _frame.GetHeader();
    if(header.GetType() == HeaderTypeToken) {
      // Transmit pending things
    } else if(header.GetType() == HeaderTypeCommand) {
      DS485CommandFrame& cmdFrame = dynamic_cast<DS485CommandFrame&>(_frame);
      PayloadDissector pd(cmdFrame.GetPayload());
      if(cmdFrame.GetCommand() == CommandRequest) {
        int cmdNr = pd.Get<uint8>();
        DS485CommandFrame* response;
        switch(cmdNr) {
          case FunctionDeviceCallScene:
            {
              devid_t devID = pd.Get<devid_t>();
              int sceneID = pd.Get<uint8>();
              DeviceCallScene(devID, sceneID);
              DistributeFrame(boost::shared_ptr<DS485CommandFrame>(CreateAck(cmdFrame, cmdNr)));
            }
            break;
          case FunctionDeviceGetFunctionID:
            {
              devid_t devID = pd.Get<devid_t>();
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(LookupDevice(devID).GetFunctionID());
              DistributeFrame(response);
            }
            break;
          case FunctionDeviceGetParameterValue:
            {
              int devID = pd.Get<uint8>();
              double result = LookupDevice(devID).GetValue(-1);
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(static_cast<uint8>(result));
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorGetRoomsSize:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(m_Rooms.size());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorGetRoomIdForInd:
            {
              uint8 index = pd.Get<uint8>();
              map< const int, vector<DSIDInterface*> >::iterator it = m_Rooms.begin();
              advance(it, index);
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(it->first);
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorCountDevInRoom:
            {
              uint8 index = pd.Get<uint8>();
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint16_t>(m_Rooms[index].size());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorDevKeyInRoom:
            {
              uint8 roomID = pd.Get<uint8>();
              uint8 deviceIndex = pd.Get<devid_t>();
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(m_Rooms[roomID].at(deviceIndex)->GetShortAddress());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorGetGroupsSize:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int roomID = pd.Get<uint8>();
              IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInRoom.begin();
							IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInRoom.end();
							int result = 0;
							while(it != end) {
								if(it->first.first == roomID) {
									result++;
								}
								it++;
							}
              response->GetPayload().Add<uint8>(result);
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
              int roomID = pd.Get<uint8>();
              int groupID = pd.Get<uint8>();
              int result = m_DevicesOfGroupInRoom[pair<const int, const int>(roomID, groupID)].size();
              response->GetPayload().Add<uint8>(result);
              DistributeFrame(response);
            }
            break;
          case FunctionGroupGetDevKeyForInd:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int roomID = pd.Get<uint8>();
              int groupID = pd.Get<uint8>();
              int index = pd.Get<uint8>();
              int result = m_DevicesOfGroupInRoom[pair<const int, const int>(roomID, groupID)].at(index)->GetShortAddress();
              response->GetPayload().Add<uint8>(result);
              DistributeFrame(response);
            }
            break;
          case FunctionRoomGetGroupIdForInd:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int roomID = pd.Get<uint8>();
              int groupIndex= pd.Get<uint8>();

              IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInRoom.begin();
              IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInRoom.end();
              int result = -1;
              while(it != end) {
                if(it->first.first == roomID) {
                  if(groupIndex == 0) {
                    result = it->first.second; // yes, that's the group id
                    break;
                  }
                  groupIndex--;
                }
                it++;
              }
              response->GetPayload().Add<uint8>(result);
              DistributeFrame(response);
            }
            break;
          case FunctionDeviceGetOnOff:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int devID = pd.Get<uint8>();
              DSIDInterface& dev = LookupDevice(devID);
              response->GetPayload().Add<uint8>(dev.IsTurnedOn());
              DistributeFrame(response);
            }
            break;
          case FunctionDeviceGetDSID:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int devID = pd.Get<uint8>();
              DSIDInterface& dev = LookupDevice(devID);
              response->GetPayload().Add<dsid_t>(dev.GetDSID());
              DistributeFrame(response);
            }
            break;
          case FunctionDeviceSubscribe:
            {
              uint8 groupID = pd.Get<uint8>();
              devid_t devID = pd.Get<devid_t>();
              DSIDInterface& dev = LookupDevice(devID);
              m_ButtonSubscriptionFlag[&dev] = 70;
            }
            break;
          case FunctionGetTypeRequest:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              // Type 2 bytes (high, low)
              response->GetPayload().Add<uint8>(0x00);
              response->GetPayload().Add<uint8>(0x01);
              // HW-Version 2 bytes (high, low)
              response->GetPayload().Add<uint8>(0x00);
              response->GetPayload().Add<uint8>(0x01);
              // SW-Version 2 bytes (high, low)
              response->GetPayload().Add<uint8>(0x00);
              response->GetPayload().Add<uint8>(0x01);
              // free text
              response->GetPayload().Add<uint8>('d');
              response->GetPayload().Add<uint8>('S');
              response->GetPayload().Add<uint8>('M');
              response->GetPayload().Add<uint8>('S');
              response->GetPayload().Add<uint8>('i');
              response->GetPayload().Add<uint8>('m');
              DistributeFrame(response);
            }
            break;
          default:
        	  Logger::GetInstance()->Log(string("Invalid function id for sim: " + IntToString(cmdNr)), lsError);
            //throw runtime_error(string("DSModulatorSim: Invalid function id: ") + IntToString(cmdNr));
        }
      }
    }
  } // Process


  DS485CommandFrame* DSModulatorSim::CreateReply(DS485CommandFrame& _request) {
    DS485CommandFrame* result = new DS485CommandFrame();
    result->GetHeader().SetDestination(_request.GetHeader().GetSource());
    result->GetHeader().SetSource(m_ID);
    result->GetHeader().SetBroadcast(false);
    result->GetHeader().SetCounter(_request.GetHeader().GetCounter());
    return result;
  } // CreateReply


  DS485CommandFrame* DSModulatorSim::CreateAck(DS485CommandFrame& _request, uint8 _functionID) {
    DS485CommandFrame* result = CreateReply(_request);
    result->SetCommand(CommandAck);
    result->GetPayload().Add(_functionID);
    return result;
  }

  DS485CommandFrame* DSModulatorSim::CreateResponse(DS485CommandFrame& _request, uint8 _functionID) {
    DS485CommandFrame* result = CreateReply(_request);
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
    cerr << "id: " << _shortAddress << endl;
    throw runtime_error("could not find device");
  } // LookupDevice

  int DSModulatorSim::GetID() const {
    return m_ID;
  } // GetID

  void DSModulatorSim::ProcessButtonPress(const DSIDSimSwitch& _switch, int _buttonNr, const ButtonPressKind _kind) {
     // if we have subscriptions, notify the listeners
    if(m_ButtonSubscriptionFlag.find(&_switch) != m_ButtonSubscriptionFlag.end() && m_ButtonSubscriptionFlag[&_switch] != -1) {
      // send keypress frame
      DS485CommandFrame* frame = new DS485CommandFrame();
      frame->SetCommand(CommandRequest);
      frame->GetHeader().SetDestination(m_ButtonSubscriptionFlag[&_switch]);
      frame->GetHeader().SetSource(m_ID);
      frame->GetHeader().SetBroadcast(false);

      frame->GetPayload().Add<uint8>(FunctionKeyPressed);
                          /* room */          /* group */      /* short message */
      uint16_t param = ((0 & 0xFF) << 8) | ((0 & 0x7F) << 1) | 0x0001;
      frame->GetPayload().Add<uint16_t>(param);
               /* from switch */   /* switch address */
      param = (0x80 << 8) | ((_switch.GetShortAddress() & 0x7F) << 8) | ((_buttonNr & 0x0F) << 4) | (_kind & 0x0F);
      frame->GetPayload().Add<uint16_t>(param);

      DistributeFrame(frame);
    } else {
      // redistribute according to the selected color, etc...
      int groupToControl = GetGroupForSwitch(&_switch);
      if(_switch.GetNumberOfButtons() == 9) {
        // for 9 button switches we'll need to track the groups ourselves
        // 1 2 3
        // 4 5 6
        // 7 8 9
        // 1 sets the group to light, 3 is security
        // 7 is for audio and 9 toggles between the others
        // 9 toggles between on and off
        // click: 2 Scene(1) on, 8 Scene(0) off,
        //
        if(_buttonNr == 1) {
          if(_kind == Click || _kind == TouchEnd) {
            m_ButtonToGroupMapping[&_switch] = GroupIDYellow;
          }
        } else if(_buttonNr == 3) {
          if(_kind == Click || _kind == TouchEnd) {
            m_ButtonToGroupMapping[&_switch] = GroupIDRed;
          }
        } else if(_buttonNr == 7) {
          if(_kind == Click || _kind == TouchEnd) {
            m_ButtonToGroupMapping[&_switch] = GroupIDCyan;
          }
        } else if(_buttonNr == 9) {
          if(_kind == Click || _kind == TouchEnd) {
            if((groupToControl + 1) > GroupIDMax) {
              m_ButtonToGroupMapping[&_switch] = GroupIDYellow;
            } else {
              m_ButtonToGroupMapping[&_switch] = groupToControl + 1;
            }
          }
        } else if(_buttonNr == 2) {
          // TODO: select correct room
          if(_kind == Click) {
            //GroupCallScene(0, groupToControl, Scene1);
            GroupIncValue(0, groupToControl, 1);
          } else if(_kind == Touch) {
            GroupStartDim(0, groupToControl, true, 0);
          } else if(_kind == TouchEnd) {
            GroupEndDim(0, groupToControl, 0);
          }
        } else if(_buttonNr == 8) {
          // TODO: select correct room
          if(_kind == Click) {
//            GroupCallScene(0, groupToControl, SceneOff);
            GroupDecValue(0, groupToControl, 1);
          } else if(_kind == Touch) {
            GroupStartDim(0, groupToControl, false, 0);
          } else if(_kind == TouchEnd) {
            GroupEndDim(0, groupToControl, 0);
          }
        } else if(_buttonNr == 4) {
          if(_kind == Click) {
            GroupDecValue(0, groupToControl, 0);
          }
        } else if(_buttonNr == 6) {
          if(_kind == Click) {
            GroupIncValue(0, groupToControl, 0);
          }
        } else if(_buttonNr == 5) {
          if(_kind == Click) {
            GroupCallScene(0, groupToControl, Scene2);
          } else if(_kind == Touch) {
            GroupCallScene(0, groupToControl, SceneOff);
          }
        }
      } else if(_switch.GetNumberOfButtons() == 5) {
      }
    }
  } // ProcessButtonPress

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

  int DSModulatorSim::GetGroupForSwitch(const DSIDSimSwitch* _switch) {
    if(_switch != NULL) {
      if(m_ButtonToGroupMapping.find(_switch) != m_ButtonToGroupMapping.end()) {
        return m_ButtonToGroupMapping[_switch];
      }
      return _switch->GetDefaultColor();
    }
    return GroupIDYellow;
  } // GetGroupForSwitch


  //================================================== DSIDSim

  DSIDSim::DSIDSim(const DSModulatorSim& _simulator, const dsid_t _dsid, const devid_t _shortAddress)
  : DSIDInterface(_simulator, _dsid, _shortAddress),
    m_Enabled(true),
    m_CurrentValue(0)
  {
    m_ValuesForScene.push_back(0);   // OFF
    m_ValuesForScene.push_back(255); // Scene1
    m_ValuesForScene.push_back(255); // Scene2
    m_ValuesForScene.push_back(255); // Scene3
    m_ValuesForScene.push_back(255); // Scene4
    m_ValuesForScene.push_back(0);   // SceneStandby
    m_ValuesForScene.push_back(0);   // SceneDeepOff
  } // ctor

  void DSIDSim::CallScene(const int _sceneNr) {
    if(m_Enabled) {
      m_CurrentValue = m_ValuesForScene.at(_sceneNr);
    }
  } // CallScene

  void DSIDSim::SaveScene(const int _sceneNr) {
    if(m_Enabled) {
      m_ValuesForScene[_sceneNr] = m_CurrentValue;
    }
  } // SaveScene

  void DSIDSim::UndoScene(const int _sceneNr) {
    if(m_Enabled) {
      m_CurrentValue = m_ValuesForScene.at(_sceneNr);
    }
  } // UndoScene

  void DSIDSim::IncreaseValue(const int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue++;
    }
  } // IncreaseValue

  void DSIDSim::DecreaseValue(const int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue--;
    }
  } // DecreaseValue

  void DSIDSim::Enable() {
    m_Enabled = true;
  } // Enable

  void DSIDSim::Disable() {
    m_Enabled = false;
  } // Disable

  void DSIDSim::StartDim(bool _directionUp, const int _parameterNr) {
    if(m_Enabled) {
      m_DimmingUp = _directionUp;
      m_Dimming = true;
      time(&m_DimmStartTime);
    }
  } // StartDim

  void DSIDSim::EndDim(const int _parameterNr) {
    if(m_Enabled) {
      time_t now;
      time(&now);
      if(m_DimmingUp) {
        m_CurrentValue = static_cast<int>(max(m_CurrentValue + difftime(m_DimmStartTime, now) * 5, 255.0));
      } else {
        m_CurrentValue = static_cast<int>(min(m_CurrentValue - difftime(m_DimmStartTime, now) * 5, 255.0));
      }
    }
  } // EndDim

  void DSIDSim::SetValue(const double _value, int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue = static_cast<int>(_value);
    }
  } // SetValue

  double DSIDSim::GetValue(int _parameterNr) const {
    return static_cast<double>(m_CurrentValue);
  } // GetValue

  uint8 DSIDSim::GetFunctionID() {
    return FunctionIDDevice;
  }

  //================================================== DSIDCreator

  DSIDCreator::DSIDCreator(const DSModulatorSim& _simulator, const string& _identifier)
  : m_Identifier(_identifier),
    m_DSSim(_simulator)
  {} // ctor

  //================================================== DSIDFactory

  DSIDInterface* DSIDFactory::CreateDSID(const string& _identifier, const dsid_t _dsid, const devid_t _shortAddress) {
    boost::ptr_vector<DSIDCreator>::iterator
      iCreator = m_RegisteredCreators.begin(),
      e = m_RegisteredCreators.end();
    while(iCreator != e) {
      if(iCreator->GetIdentifier() == _identifier) {
        return iCreator->CreateDSID(_dsid, _shortAddress);
      }
      ++iCreator;
    }
    return NULL;
  }

  void DSIDFactory::RegisterCreator(DSIDCreator* _creator) {
    m_RegisteredCreators.push_back(_creator);
  }

}
