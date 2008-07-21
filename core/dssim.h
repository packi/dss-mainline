#ifndef DSSIM_H_
#define DSSIM_H_

#include "ds485types.h"
#include "ds485.h"
#include "xmlwrapper.h"


#include <map>
#include <vector>

using namespace std;

namespace dss {
  class DSIDSim;
  
  typedef map< const pair<const int, const int>,  vector<DSIDSim*> > IntPairToDSIDSimVector; 
  
  class DSModulatorSim : public DS485FrameProvider {
  private:
    int m_ID;
    dsid_t m_ModulatorDSID;
    vector<DSIDSim*> m_SimulatedDevices;
    map< const int, vector<DSIDSim*> > m_Rooms;
    IntPairToDSIDSimVector m_DevicesOfGroupInRoom;
    vector<DS485Frame*> m_PendingFrames;
  private:  
    void LoadFromConfig();
    void LoadDevices(XMLNodeList& _nodes, const int _roomID);
    void LoadRooms(XMLNodeList& _nodes);
    
    DSIDSim& LookupDevice(const devid_t _id);
    DS485CommandFrame* CreateResponse(DS485CommandFrame& _request, uint8 _functionID);
    DS485CommandFrame* CreateAck(DS485CommandFrame& _request, uint8 _functionID);
    DS485CommandFrame* CreateReply(DS485CommandFrame& _request);
  public:
    DSModulatorSim();
    virtual ~DSModulatorSim() {};
    void Initialize();
    
    int GetID() const;

    void Send(DS485Frame& _frame);
  }; // DSModulatorSim
  
  class DSIDSim {
  private:
    const dsid_t m_DSID;
    int m_ShortAddress;
    bool m_On;
    bool m_Enabled;
    bool m_Dimming;
    time_t m_DimmStartTime;
    bool m_DimmingUp;
    vector<int> m_Parameters;
    DSModulatorSim* m_Modulator;
    vector<uint8> m_ValuesForScene;
    uint8 m_CurrentValue;
    int m_DimTimeMS;
  public:
    DSIDSim(const dsid_t _dsid);
    
    dsid_t GetDSID() const;
    devid_t GetShortAddress() const;
    void SetShortAddress(const devid_t _shortAddress);
    
    void CallScene(const int _sceneNr);
    void SaveScene(const int _sceneNr);
    void UndoScene(const int _sceneNr);
    
    void IncreaseValue(const int _parameterNr = -1);
    void DecreaseValue(const int _parameterNr = -1);
    
    bool IsTurnedOn() const;
    
    void Enable();
    void Disable();
    
    void StartDim(bool _directionUp, const int _parameterNr = -1);
    void EndDim(const int _parameterNr = -1);
    void SetValue(const double _value, int _parameterNr = -1);
  }; // DSIDSim
  

}

#endif /*DSSIM_H_*/
