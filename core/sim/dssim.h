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

    const string& getIdentifier() const { return m_Identifier; }
    virtual DSIDInterface* createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) = 0;
  };

  class DSIDFactory {
  private:
    boost::ptr_vector<DSIDCreator> m_RegisteredCreators;
  public:
    DSIDInterface* createDSID(const string& _identifier, const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator);

    void registerCreator(DSIDCreator* _creator);
  };

  class DSSim : public Subsystem,
                public DS485FrameProvider {
  private:
    DSIDFactory m_DSIDFactory;
    bool m_Initialized;
    boost::ptr_vector<DSModulatorSim> m_Modulators;
  private:
    void loadPlugins();

    void loadFromConfig();
  protected:
    virtual void doStart() {}
  public:
    DSSim(DSS* _pDSS);
    virtual ~DSSim() {}
    virtual void initialize();

    void process(DS485Frame& _frame);
    bool isSimAddress(const uint8_t _address);

    bool isReady();

    DSIDFactory& getDSIDFactory() { return m_DSIDFactory; }

    void distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
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
    map<const int, vector<int> > m_GroupsPerDevice;
    map<const int, string> m_DeviceNames;
    map< const pair<const int, const int>, int> m_LastCalledSceneForZoneAndGroup;
    string m_Name;
  private:
    void addDeviceToGroup(DSIDInterface* _device, int _groupID);
    void loadDevices(XMLNodeList& _nodes, const int _zoneID);
    void loadGroups(XMLNodeList& _nodes, const int _zoneID);
    void loadZones(XMLNodeList& _nodes);

    DSIDInterface& lookupDevice(const devid_t _id);
    boost::shared_ptr<DS485CommandFrame> createResponse(DS485CommandFrame& _request, uint8_t _functionID);
    boost::shared_ptr<DS485CommandFrame> createAck(DS485CommandFrame& _request, uint8_t _functionID);
    boost::shared_ptr<DS485CommandFrame> createReply(DS485CommandFrame& _request);

    void distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
  private:
    void deviceCallScene(const int _deviceID, const int _sceneID);
    void groupCallScene(const int _zoneID, const int _groupID, const int _sceneID);
    void deviceSaveScene(const int _deviceID, const int _sceneID);
    void groupSaveScene(const int _zoneID, const int _groupID, const int _sceneID);
    void deviceUndoScene(const int _deviceID, const int _sceneID);
    void groupUndoScene(const int _zoneID, const int _groupID, const int _sceneID);
    void groupStartDim(const int _zoneID, const int _groupID, bool _up, const int _parameterNr);
    void groupEndDim(const int _zoneID, const int _groupID, const int _parameterNr);
    void groupDecValue(const int _zoneID, const int _groupID, const int _parameterNr);
    void groupIncValue(const int _zoneID, const int _groupID, const int _parameterNr);
    void groupSetValue(const int _zoneID, const int _groupID, const int _value);
  protected:
    virtual void doStart() {}
    void log(const string& _message, aLogSeverity _severity = lsDebug);
  public:
    DSModulatorSim(DSSim* _pSimulator);
    virtual ~DSModulatorSim() {}

    bool initializeFromNode(XMLNode& _node);

    int getID() const;

    void process(DS485Frame& _frame);

    DSIDInterface* getSimulatedDevice(const dsid_t _dsid);
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

    virtual void initialize() {};

    virtual dsid_t getDSID() const { return m_DSID; }
    virtual devid_t getShortAddress() const { return m_ShortAddress; }
    virtual void setShortAddress(const devid_t _value) { m_ShortAddress = _value; }

    virtual void callScene(const int _sceneNr) = 0;
    virtual void saveScene(const int _sceneNr) = 0;
    virtual void undoScene(const int _sceneNr) = 0;

    virtual void increaseValue(const int _parameterNr = -1) = 0;
    virtual void decreaseValue(const int _parameterNr = -1) = 0;

    bool isTurnedOn() const {
      return getValue() > 0.0;
    }

    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual int getConsumption() = 0;

    virtual void startDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void endDim(const int _parameterNr = -1) = 0;
    virtual void setValue(const double _value, int _parameterNr = -1) = 0;

    virtual double getValue(int _parameterNr = -1) const = 0;

    virtual uint16_t getFunctionID() = 0;

    virtual void setConfigParameter(const string& _name, const string& _value) = 0;
    virtual string getConfigParameter(const string& _name) const = 0;

    virtual void setZoneID(const int _value) { m_ZoneID = _value; }
    virtual int getZoneID() const { return m_ZoneID; }
  }; // DSIDInterface


}

#endif /*DSSIM_H_*/
