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
  class DS485Frame;
  class DSIDInterface;
  class DSModulatorSim;

  class DSIDCreator {
  private:
    string m_Identifier;
  public:
    DSIDCreator(const string& _identifier);
    virtual ~DSIDCreator() {};

    const string& GetIdentifier() const { return m_Identifier; }
    virtual DSIDInterface* CreateDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) = 0;
  };

  class DSIDFactory {
  private:
    boost::ptr_vector<DSIDCreator> m_RegisteredCreators;
  public:
    DSIDInterface* CreateDSID(const string& _identifier, const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator);

    void RegisterCreator(DSIDCreator* _creator);
  };

  class DSSim : public Subsystem,
                public DS485FrameProvider {
  private:
    DSIDFactory m_DSIDFactory;
    bool m_Initialized;
    boost::ptr_vector<DSModulatorSim> m_Modulators;
  private:
    void LoadPlugins();

    void LoadFromConfig();
  protected:
    virtual void DoStart() {}
  public:
    DSSim(DSS* _pDSS);
    virtual ~DSSim() {}
    virtual void Initialize();

    void process(DS485Frame& _frame);
    bool isSimAddress(const uint8_t _address);

    bool isReady();

    DSIDFactory& getDSIDFactory() { return m_DSIDFactory; }

    void DistributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
  }; // DSSim

  typedef map< const pair<const int, const int>,  vector<DSIDInterface*> > IntPairToDSIDSimVector;

  class DSModulatorSim {
  private:
    DSSim* m_pSimulation;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_ID;
    dsid_t m_ModulatorDSID;
    vector<DSIDInterface*> m_SimulatedDevices;
    map< const int, vector<DSIDInterface*> > m_Zones;
    IntPairToDSIDSimVector m_DevicesOfGroupInZone;
    vector<DS485Frame*> m_PendingFrames;
    map<const DSIDInterface*, int> m_ButtonToGroupMapping;
    map<const DSIDInterface*, int> m_DeviceZoneMapping;
    map<const int, int> m_LastCalledSceneForGroup;
    map<const int, vector<int> > m_GroupsPerDevice;
    map<const int, string> m_DeviceNames;
    string m_Name;
  private:
    void AddDeviceToGroup(DSIDInterface* _device, int _groupID);
    void LoadDevices(XMLNodeList& _nodes, const int _zoneID);
    void LoadGroups(XMLNodeList& _nodes, const int _zoneID);
    void LoadZones(XMLNodeList& _nodes);

    DSIDInterface& LookupDevice(const devid_t _id);
    boost::shared_ptr<DS485CommandFrame> CreateResponse(DS485CommandFrame& _request, uint8_t _functionID);
    boost::shared_ptr<DS485CommandFrame> CreateAck(DS485CommandFrame& _request, uint8_t _functionID);
    boost::shared_ptr<DS485CommandFrame> CreateReply(DS485CommandFrame& _request);

    void DistributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
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
    void GroupSetValue(const int _zoneID, const int _groupID, const int _value);
  protected:
    virtual void DoStart() {}
    void Log(const string& _message, aLogSeverity _severity = lsDebug);
  public:
    DSModulatorSim(DSSim* _pSimulator);
    virtual ~DSModulatorSim() {}

    bool InitializeFromNode(XMLNode& _node);

    int GetID() const;

    void Process(DS485Frame& _frame);

    DSIDInterface* GetSimulatedDevice(const dsid_t _dsid);
  }; // DSModulatorSim

  class DSIDInterface {
  private:
    dsid_t m_DSID;
    devid_t m_ShortAddress;
    const DSModulatorSim& m_Simulator;
    int m_ZoneID;
  public:
    DSIDInterface(const DSModulatorSim& _simulator, dsid_t _dsid, devid_t _shortAddress)
    : m_DSID(_dsid), m_ShortAddress(_shortAddress), m_Simulator(_simulator) {}

    virtual ~DSIDInterface() {}

    virtual void Initialize() {};

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

    virtual void SetConfigParameter(const string& _name, const string& _value) = 0;
    virtual string GetConfigParameter(const string& _name) const = 0;

    virtual void SetZoneID(const int _value) { m_ZoneID = _value; }
    virtual int GetZoneID() const { return m_ZoneID; }
  }; // DSIDInterface


}

#endif /*DSSIM_H_*/
