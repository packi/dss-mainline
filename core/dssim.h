#ifndef DSSIM_H_
#define DSSIM_H_

#include "ds485types.h"
#include "../unix/ds485.h"
#include "xmlwrapper.h"


#include <map>
#include <vector>

using namespace std;

namespace dss {
  class DSIDSim;
  class DS485Frame;
  class DSIDInterface;

  typedef map< const pair<const int, const int>,  vector<DSIDInterface*> > IntPairToDSIDSimVector;

  class DSModulatorSim : public DS485FrameProvider {
  private:
    int m_ID;
    dsid_t m_ModulatorDSID;
    vector<DSIDInterface*> m_SimulatedDevices;
    map< const int, vector<DSIDInterface*> > m_Rooms;
    IntPairToDSIDSimVector m_DevicesOfGroupInRoom;
    vector<DS485Frame*> m_PendingFrames;
  private:
    void LoadFromConfig();
    void LoadDevices(XMLNodeList& _nodes, const int _roomID);
    void LoadRooms(XMLNodeList& _nodes);

    DSIDInterface& LookupDevice(const devid_t _id);
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

  class DSIDInterface {
  public:
    virtual ~DSIDInterface() {};

    virtual dsid_t GetDSID() const = 0;
    virtual devid_t GetShortAddress() const = 0;
    virtual void SetShortAddress(const devid_t _shortAddress) = 0;

    virtual void CallScene(const int _sceneNr) = 0;
    virtual void SaveScene(const int _sceneNr) = 0;
    virtual void UndoScene(const int _sceneNr) = 0;

    virtual void IncreaseValue(const int _parameterNr = -1) = 0;
    virtual void DecreaseValue(const int _parameterNr = -1) = 0;

    bool IsTurnedOn() const {
      return GetValue() > 0.0;
    }

    virtual void Enable() = 0;
    virtual void Disable() = 0;

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void EndDim(const int _parameterNr = -1) = 0;
    virtual void SetValue(const double _value, int _parameterNr = -1) = 0;

    virtual double GetValue(int _parameterNr = -1) const = 0;
  }; // DSIDInterface

  class DSIDSim : public DSIDInterface {
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
    virtual ~DSIDSim() {};

    virtual dsid_t GetDSID() const;
    virtual devid_t GetShortAddress() const;
    virtual void SetShortAddress(const devid_t _shortAddress);

    virtual void CallScene(const int _sceneNr);
    virtual void SaveScene(const int _sceneNr);
    virtual void UndoScene(const int _sceneNr);

    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);

    virtual void Enable();
    virtual void Disable();

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, int _parameterNr = -1);

    virtual double GetValue(int _parameterNr = -1) const;
  }; // DSIDSim


}

#endif /*DSSIM_H_*/
