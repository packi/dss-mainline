#ifndef DSSIM_H_
#define DSSIM_H_

#include "core/ds485types.h"
#include "core/ds485const.h"
#include "unix/ds485.h"
#include "core/xmlwrapper.h"
#include "core/subsystem.h"

#include <map>
#include <vector>

using namespace std;

#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {
  class DSIDSim;
  class DS485Frame;
  class DSIDInterface;
  class DSModulatorSim;
  class DSIDSimSwitch;

  class DSIDCreator {
  private:
    string m_Identifier;
    const DSModulatorSim& m_DSSim;
  protected:
    const DSModulatorSim& GetSimulator() { return m_DSSim; }
  public:
    DSIDCreator(const DSModulatorSim& _simulator, const string& _identifier);
    virtual ~DSIDCreator() {};

    const string& GetIdentifier() { return m_Identifier; }
    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress) = 0;
  };

  class DSIDFactory {
  private:
    boost::ptr_vector<DSIDCreator> m_RegisteredCreators;
  public:
    DSIDInterface* CreateDSID(const string& _identifier, const dsid_t _dsid, const devid_t _shortAddress);

    void RegisterCreator(DSIDCreator* _creator);
  };

  typedef map< const pair<const int, const int>,  vector<DSIDInterface*> > IntPairToDSIDSimVector;

  class DSModulatorSim : public DS485FrameProvider,
                         public Subsystem {
  private:
    int m_ID;
    dsid_t m_ModulatorDSID;
    bool m_Initialized;
    vector<DSIDInterface*> m_SimulatedDevices;
    map< const int, vector<DSIDInterface*> > m_Zones;
    IntPairToDSIDSimVector m_DevicesOfGroupInZone;
    vector<DS485Frame*> m_PendingFrames;
    DSIDFactory m_DSIDFactory;
    map<const DSIDInterface*, int> m_ButtonToGroupMapping;
    map<const DSIDInterface*, int> m_DeviceZoneMapping;
    map<const int, int> m_LastCalledSceneForGroup;
  private:
    void LoadFromConfig();
    void LoadDevices(XMLNodeList& _nodes, const int _zoneID);
    void LoadGroups(XMLNodeList& _nodes, const int _zoneID);
    void LoadZones(XMLNodeList& _nodes);

    void LoadPlugins();

    DSIDInterface& LookupDevice(const devid_t _id);
    DS485CommandFrame* CreateResponse(DS485CommandFrame& _request, uint8_t _functionID);
    DS485CommandFrame* CreateAck(DS485CommandFrame& _request, uint8_t _functionID);
    DS485CommandFrame* CreateReply(DS485CommandFrame& _request);
  private:
    void DeviceCallScene(const int _deviceID, const int _sceneID);
    void GroupCallScene(const int _zoneID, const int _groupID, const int _sceneID);
    void DeviceSaveScene(const int _deviceID, const int _sceneID);
    void GroupSaveScene(const int _zoneID, const int _groupID, const int _sceneID);
    void DeviceUndoScene(const int _deviceID, const int _sceneID);
    void GroupUndoScene(const int _zoneID, const int _groupID, const int _sceneID);
    void GroupStartDim(const int _zoneID, const int _groupID, bool _up, const int _parameterNr);
    void GroupEndDim(const int _zoneID, const int _groupID, const int _parameterNr);
    void GroupDecValue(const int _zoneID, const int _groupID, const int _parameterNr);
    void GroupIncValue(const int _zoneID, const int _groupID, const int _parameterNr);
  public:
    DSModulatorSim(DSS* _pDSS);
    virtual ~DSModulatorSim() {}
    virtual void Initialize();

    bool Ready();

    int GetID() const;

    void Process(DS485Frame& _frame);

    void ProcessButtonPress(const DSIDSimSwitch& _switch, int _buttonNr, const ButtonPressKind _kind);

    DSIDInterface* GetSimulatedDevice(const dsid_t _dsid);
    int GetGroupForSwitch(const DSIDSimSwitch* _switch);
  }; // DSModulatorSim

  class DSIDInterface {
  private:
    dsid_t m_DSID;
    devid_t m_ShortAddress;
    const DSModulatorSim& m_Simulator;
  public:
    DSIDInterface(const DSModulatorSim& _simulator, dsid_t _dsid, devid_t _shortAddress)
    : m_DSID(_dsid), m_ShortAddress(_shortAddress), m_Simulator(_simulator) {}

    virtual ~DSIDInterface() {}

    virtual dsid_t GetDSID() const { return m_DSID; }
    virtual devid_t GetShortAddress() const { return m_ShortAddress; }
    virtual void SetShortAddress(const devid_t _value) { m_ShortAddress = _value; }

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

    virtual int GetConsumption() = 0;

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void EndDim(const int _parameterNr = -1) = 0;
    virtual void SetValue(const double _value, int _parameterNr = -1) = 0;

    virtual double GetValue(int _parameterNr = -1) const = 0;

    virtual uint8_t GetFunctionID() = 0;
  }; // DSIDInterface

  class DSIDSim : public DSIDInterface {
  private:
    bool m_On;
    bool m_Enabled;
    bool m_Dimming;
    time_t m_DimmStartTime;
    bool m_DimmingUp;
    vector<int> m_Parameters;
    DSModulatorSim* m_Modulator;
    vector<uint8_t> m_ValuesForScene;
    uint8_t m_CurrentValue;
    int m_DimTimeMS;
    int m_SimpleConsumption;
  public:
    DSIDSim(const DSModulatorSim& _simulator, const dsid_t _dsid, const devid_t _shortAddress);
    virtual ~DSIDSim() {}

    virtual void CallScene(const int _sceneNr);
    virtual void SaveScene(const int _sceneNr);
    virtual void UndoScene(const int _sceneNr);

    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);

    virtual void Enable();
    virtual void Disable();

    virtual int GetConsumption();

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, int _parameterNr = -1);

    virtual double GetValue(int _parameterNr = -1) const;

    virtual uint8_t GetFunctionID();
    void SetSimpleConsumption(const int _value) { m_SimpleConsumption = _value; }
  }; // DSIDSim


  class DSIDSimSwitch : public DSIDSim {
  private:
    const int m_NumberOfButtons;
    int m_DefaultColor;
    bool m_IsBell;
  public:
    DSIDSimSwitch(const DSModulatorSim& _simulator, const dsid_t _dsid, const devid_t _shortAddress, const int _numButtons)
    : DSIDSim(_simulator, _dsid, _shortAddress),
      m_NumberOfButtons(_numButtons),
      m_DefaultColor(GroupIDYellow),
      m_IsBell(false)
    {};
    ~DSIDSimSwitch() {}

    void PressKey(const ButtonPressKind _kind, const int _buttonNr);

    const int GetDefaultColor() const { return m_DefaultColor; }

    const int GetNumberOfButtons() const { return m_NumberOfButtons; }

    bool IsBell() const { return m_IsBell; }
    void SetIsBell(const bool _value) { m_IsBell = _value; }

    virtual uint8_t GetFunctionID() { return FunctionIDSwitch; }
  }; // DSIDSimSwitch
}

#endif /*DSSIM_H_*/
