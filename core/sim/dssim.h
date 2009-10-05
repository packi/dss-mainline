/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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
#include "core/ds485const.h"
#include "unix/ds485.h"
#include "core/xmlwrapper.h"
#include "core/subsystem.h"

#include <map>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {
  class DS485Frame;
  class DSIDInterface;
  class DSModulatorSim;

  class DSIDCreator {
  private:
    std::string m_Identifier;
  public:
    DSIDCreator(const std::string& _identifier);
    virtual ~DSIDCreator() {};

    const std::string& getIdentifier() const { return m_Identifier; }
    virtual DSIDInterface* createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) = 0;
  };

  class DSIDFactory {
  private:
    boost::ptr_vector<DSIDCreator> m_RegisteredCreators;
  public:
    DSIDInterface* createDSID(const std::string& _identifier, const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator);

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
    DSIDInterface* getSimulatedDevice(const dsid_t& _dsid);
  }; // DSSim

  typedef std::map< const std::pair<const int, const int>,  std::vector<DSIDInterface*> > IntPairToDSIDSimVector;

  class DSModulatorSim {
  private:
    DSSim* m_pSimulation;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_ID;
    dsid_t m_ModulatorDSID;
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
    void addDeviceToGroup(DSIDInterface* _device, int _groupID);
    void loadDevices(XMLNodeList& _nodes, const int _zoneID);
    void loadGroups(XMLNodeList& _nodes, const int _zoneID);
    void loadZones(XMLNodeList& _nodes);

    DSIDInterface& lookupDevice(const devid_t _id);
    boost::shared_ptr<DS485CommandFrame> createResponse(DS485CommandFrame& _request, uint8_t _functionID) const;
    boost::shared_ptr<DS485CommandFrame> createAck(DS485CommandFrame& _request, uint8_t _functionID) const;
    boost::shared_ptr<DS485CommandFrame> createReply(DS485CommandFrame& _request) const;

    void distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) const;
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
    DSModulatorSim(DSSim* _pSimulator);
    virtual ~DSModulatorSim() {}

    bool initializeFromNode(XMLNode& _node);

    int getID() const;

    void process(DS485Frame& _frame);

    DSIDInterface* getSimulatedDevice(const dsid_t _dsid);
    void dSLinkInterrupt(devid_t _shortAddress) const;
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

    virtual void setConfigParameter(const std::string& _name, const std::string& _value) = 0;
    virtual std::string getConfigParameter(const std::string& _name) const = 0;

    virtual void setZoneID(const int _value) { m_ZoneID = _value; }
    virtual int getZoneID() const { return m_ZoneID; }
    virtual uint8_t dsLinkSend(uint8_t _value, uint8_t _flags, bool& _handled) {
      _handled = false;
      return 0;
    }

    /** Signals the modulator that a interrupt has occurred */
    void dSLinkInterrupt() {
      m_Simulator.dSLinkInterrupt(m_ShortAddress);
    }
  }; // DSIDInterface


}

#endif /*DSSIM_H_*/
