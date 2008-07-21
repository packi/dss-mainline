#include "dssim.h"

#include "ds485const.h"
#include "base.h"
#include "logger.h"

#include <string>
#include <stdexcept>

namespace dss {

  //================================================== DSModulatorSim

  DSModulatorSim::DSModulatorSim()
  {
    m_ModulatorDSID = SimulationPrefix | 0x0000FFFF;
  } // DSModulatorSim


  void DSModulatorSim::Initialize() {
    m_ID = 70;

    LoadFromConfig();
  }

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
        DSIDSim* newDSID = new DSIDSim(dsid);
        newDSID->SetShortAddress(busid);
        m_SimulatedDevices.push_back(newDSID);
        m_Rooms[_roomID].push_back(newDSID);
        Logger::GetInstance()->Log("Sim: found device");
      }
    }
  } // LoadDevices

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
        } else {
          Logger::GetInstance()->Log("Sim: could not find/parse id for room");
        }
      }
    }
  } // LoadRooms

  const uint8 HeaderTypeToken = 0;
  const uint8 HeaderTypeCommand = 1;

  void DSModulatorSim::Send(DS485Frame& _frame) {
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
              int devID = pd.Get<uint8>();
              int sceneID = pd.Get<uint8>();
              LookupDevice(devID).CallScene(sceneID);
              DistributeFrame(boost::shared_ptr<DS485CommandFrame>(CreateAck(cmdFrame, cmdNr)));
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
              map< const int, vector<DSIDSim*> >::iterator it = m_Rooms.begin();
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
              response->GetPayload().Add<uint8>(m_Rooms[index].size());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorDevKeyInRoom:
            {
              uint8 roomID = pd.Get<uint8>();
              uint8 deviceIndex = pd.Get<uint8>();
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(m_Rooms[roomID].at(deviceIndex)->GetShortAddress());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorGetGroupsSize:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(m_DevicesOfGroupInRoom.size());
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
                  groupIndex--;
                  if(groupIndex == 0) {
                    result = it->first.second; // yes, that's the group id
                    break;
                  }
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
              DSIDSim& dev = LookupDevice(devID);
              response->GetPayload().Add<uint8>(dev.IsTurnedOn());
              DistributeFrame(response);
            }
            break;
          case FunctionDeviceGetDSID:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int devID = pd.Get<uint8>();
              DSIDSim& dev = LookupDevice(devID);
              response->GetPayload().Add<dsid_t>(dev.GetDSID());
              DistributeFrame(response);
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
              // HW-Version 2 bytes (high, low)
              response->GetPayload().Add<uint8>(0x00);
              response->GetPayload().Add<uint8>(0x01);
              // free text
              response->GetPayload().Add<uint8>('d');
              response->GetPayload().Add<uint8>('S');
              response->GetPayload().Add<uint8>('M');
              response->GetPayload().Add<uint8>(' ');
              response->GetPayload().Add<uint8>('S');
              response->GetPayload().Add<uint8>('i');
              response->GetPayload().Add<uint8>('m');
              response->GetPayload().Add<uint8>(0x00);
              DistributeFrame(response);
            }
            break;
          default:
            throw new runtime_error(string("DSModulatorSim: Invalid function id: ") + IntToString(cmdNr));
        }
      }
    }
  } // Send


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


  DSIDSim& DSModulatorSim::LookupDevice(const devid_t _shortAddress) {
    for(vector<DSIDSim*>::iterator ipSimDev = m_SimulatedDevices.begin(); ipSimDev != m_SimulatedDevices.end(); ++ipSimDev) {
      if((*ipSimDev)->GetShortAddress() == _shortAddress)  {
        return **ipSimDev;
      }
    }
    throw new runtime_error("could not find device");

  } // LookupDevice

  int DSModulatorSim::GetID() const {
    return m_ID;
  } // GetID

  //================================================== DSIDSim

  DSIDSim::DSIDSim(const dsid_t _dsid)
  : m_DSID(_dsid),
    m_ShortAddress(0xFF),
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

  dsid_t DSIDSim::GetDSID() const {
    return m_DSID;
  } // GetDSID

  devid_t DSIDSim::GetShortAddress() const {
    return m_ShortAddress;
  } // GetShortAddress

  void DSIDSim::SetShortAddress(const devid_t _shortAddress) {
    m_ShortAddress = _shortAddress;
  }

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

  bool DSIDSim::IsTurnedOn() const {
    return m_CurrentValue > 0;
  } // IsTurnedOn

}
