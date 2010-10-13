/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef DSSIM_H_
#define DSSIM_H_

#include "core/ds485types.h"
#include "core/model/modelconst.h"
#include "core/subsystem.h"

#include <map>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>

namespace Poco {
  namespace XML {
    class Node;
  }
}

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class DS485Frame;
  class DSIDInterface;
  class DSDSMeterSim;

  class DSIDCreator {
  private:
    std::string m_Identifier;
  public:
    DSIDCreator(const std::string& _identifier);
    virtual ~DSIDCreator() {};

    const std::string& getIdentifier() const { return m_Identifier; }
    virtual DSIDInterface* createDSID(const dss_dsid_t _dsid, const devid_t _shortAddress, const DSDSMeterSim& _dsMeter) = 0;
  };

  class DSIDFactory {
  private:
    boost::ptr_vector<DSIDCreator> m_RegisteredCreators;
  public:
    DSIDInterface* createDSID(const std::string& _identifier, const dss_dsid_t _dsid, const devid_t _shortAddress, const DSDSMeterSim& _dsMeter);

    void registerCreator(DSIDCreator* _creator);
  };

  class DSSim : public Subsystem {
  private:
    DSIDFactory m_DSIDFactory;
    bool m_Initialized;
    boost::ptr_vector<DSDSMeterSim> m_DSMeters;
  private:
    void loadPlugins();

    void loadFromConfig();
    void createJSPluginFrom(PropertyNodePtr _node);
  protected:
    virtual void doStart() {}
  public:
    DSSim(DSS* _pDSS);
    virtual ~DSSim() {}
    virtual void initialize();

    void loadFromFile(const std::string& _file);

    void process(DS485Frame& _frame);
    bool isSimAddress(const uint8_t _address);

    bool isReady();

    DSIDFactory& getDSIDFactory() { return m_DSIDFactory; }

    // TODO: libdsm
    // void distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
    DSIDInterface* getSimulatedDevice(const dss_dsid_t& _dsid);

    int getDSMeterCount() const { return m_DSMeters.size(); }
    DSDSMeterSim& getDSMeter(const int _index) { return m_DSMeters[_index]; }

    static dss_dsid_t makeSimulatedDSID(const dss_dsid_t& _dsid);
    static bool isSimulatedDSID(const dss_dsid_t& _dsid);
  }; // DSSim

  typedef std::map< const std::pair<const int, const int>,  std::vector<DSIDInterface*> > IntPairToDSIDSimVector;

  class DSDSMeterSim {
  private:
    DSSim* m_pSimulation;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_ID;
    dss_dsid_t m_DSMeterDSID;
    std::vector<DSIDInterface*> m_SimulatedDevices;
    std::map< const int, std::vector<DSIDInterface*> > m_Zones;
    IntPairToDSIDSimVector m_DevicesOfGroupInZone;
    std::vector<DS485Frame*> m_PendingFrames;
    std::map<const DSIDInterface*, int> m_ButtonToGroupMapping;
    std::map<const DSIDInterface*, int> m_DeviceZoneMapping;
    std::map<const int, std::vector<int> > m_GroupsPerDevice;
    std::map<const int, std::string> m_DeviceNames;
    std::map< const std::pair<const int, const int>, int> m_LastCalledSceneForZoneAndGroup;
    std::string m_Name;
  private:
    void loadDevices(Poco::XML::Node* _node, const int _zoneID);
    void loadGroups(Poco::XML::Node* _node, const int _zoneID);
    void loadZones(Poco::XML::Node* _node);

    DSIDInterface& lookupDevice(const devid_t _id);

    // TODO: libdsm
    // boost::shared_ptr<DS485CommandFrame> createResponse(DS485CommandFrame& _request, uint8_t _functionID) const;
    // boost::shared_ptr<DS485CommandFrame> createAck(DS485CommandFrame& _request, uint8_t _functionID) const;
    // boost::shared_ptr<DS485CommandFrame> createReply(DS485CommandFrame& _request) const;
    //
    // void distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) const;
    // void sendDelayedResponse(boost::shared_ptr<DS485CommandFrame> _response, int _delayMS);
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
    void log(const std::string& _message, aLogSeverity _severity = lsDebug);
  public:
    DSDSMeterSim(DSSim* _pSimulator);
    virtual ~DSDSMeterSim() {}

    bool initializeFromNode(Poco::XML::Node* _node);

    int getID() const;
    void setID(const int _value) { m_ID = _value; }

    void setDSID(const dss_dsid_t& _value) { m_DSMeterDSID = _value; }

    void process(DS485Frame& _frame);

    DSIDInterface* getSimulatedDevice(const dss_dsid_t _dsid);
    void addSimulatedDevice(DSIDInterface* _device);
    void dSLinkInterrupt(devid_t _shortAddress) const;

    void addDeviceToGroup(DSIDInterface* _device, int _groupID);
  }; // DSDSMeterSim

  class DSIDInterface {
  private:
    dss_dsid_t m_DSID;
    devid_t m_ShortAddress;
    const DSDSMeterSim& m_Simulator;
    int m_ZoneID;
    bool m_IsLocked;
  public:
    DSIDInterface(const DSDSMeterSim& _simulator, dss_dsid_t _dsid, devid_t _shortAddress)
    : m_DSID(_dsid), m_ShortAddress(_shortAddress), m_Simulator(_simulator),
      m_IsLocked(false) {}

    virtual ~DSIDInterface() {}

    virtual void initialize() {};

    virtual dss_dsid_t getDSID() const { return m_DSID; }
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

    virtual void setConfigParameter(const std::string& _name, const std::string& _value) = 0;
    virtual std::string getConfigParameter(const std::string& _name) const = 0;

    virtual void setZoneID(const int _value) { m_ZoneID = _value; }
    virtual int getZoneID() const { return m_ZoneID; }
    virtual uint8_t dsLinkSend(uint8_t _value, uint8_t _flags, bool& _handled) {
      _handled = false;
      return 0;
    }

    /** Signals the dsMeter that a interrupt has occurred */
    void dSLinkInterrupt() {
      m_Simulator.dSLinkInterrupt(m_ShortAddress);
    }

    bool isLocked() const { return m_IsLocked; }
    void setIsLocked(const bool _value) { m_IsLocked = _value; }
  }; // DSIDInterface


}

#endif /*DSSIM_H_*/
