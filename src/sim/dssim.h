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

#include "src/ds485types.h"
#include "src/model/modelconst.h"
#include "src/subsystem.h"

#include <map>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class DSIDInterface;
  class DSMeterSim;

  class DSIDCreator {
  private:
    std::string m_Identifier;
  public:
    DSIDCreator(const std::string& _identifier);
    virtual ~DSIDCreator() {};

    const std::string& getIdentifier() const { return m_Identifier; }
    virtual DSIDInterface* createDSID(const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter) = 0;
  };

  class DSIDFactory {
  private:
    boost::ptr_vector<DSIDCreator> m_RegisteredCreators;
  public:
    DSIDInterface* createDSID(const std::string& _identifier, const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter);

    void registerCreator(DSIDCreator* _creator);
  };

  class DSSim : public Subsystem {
  private:
    DSIDFactory m_DSIDFactory;
    bool m_Initialized;
    std::vector<boost::shared_ptr<DSMeterSim> > m_DSMeters;
  private:
    void loadFromConfig();
    void createJSPluginFrom(PropertyNodePtr _node);
  protected:
    virtual void doStart() {}
  public:
    DSSim(DSS* _pDSS);
    virtual ~DSSim() {}
    virtual void initialize();

    void loadFromFile(const std::string& _file);

    bool isReady();

    DSIDFactory& getDSIDFactory() { return m_DSIDFactory; }

    DSIDInterface* getSimulatedDevice(const dss_dsid_t& _dsid);

    int getDSMeterCount() const { return m_DSMeters.size(); }
    boost::shared_ptr<DSMeterSim> getDSMeter(const int _index) { return m_DSMeters[_index]; }
    boost::shared_ptr<DSMeterSim> getDSMeter(const dss_dsid_t& _dsid);

    static dss_dsid_t makeSimulatedDSID(const dss_dsid_t& _dsid);
  }; // DSSim

  class DSIDInterface {
  private:
    dss_dsid_t m_DSID;
    devid_t m_ShortAddress;
    const DSMeterSim& m_Simulator;
    int m_ZoneID;
    bool m_IsLocked;
  public:
    DSIDInterface(const DSMeterSim& _simulator, dss_dsid_t _dsid, devid_t _shortAddress)
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
    virtual void undoSceneLast() = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual uint32_t getPowerConsumption() = 0;
    virtual uint32_t getEnergyMeterValue() = 0;

    virtual void setDeviceConfig(const uint8_t _configClass,
                                 const uint8_t _configIndex,
                                 const uint8_t _value) = 0;

    virtual uint8_t getDeviceConfig(const uint8_t _configClass,
                                    const uint8_t _configIndex) = 0;

    virtual uint16_t getDeviceConfigWord(const uint8_t _configClass,
                                         const uint8_t _configIndex) = 0;

    virtual void setDeviceProgMode(const uint8_t _modeId) = 0;

    virtual uint32_t getDeviceSensorValue(const uint8_t _sensorIndex) = 0;
    virtual uint8_t getDeviceSensorType(const uint8_t _sensorIndex) = 0;
    virtual void sensorPush(const uint8_t _sensorType, uint16_t _sensorValue) = 0;

    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality() = 0;

    virtual void setValue(const uint8_t _value) = 0;
    virtual uint16_t getFunctionID() = 0;
    virtual uint16_t getProductID() = 0;
    virtual uint16_t getProductRevision() = 0;

    virtual void setConfigParameter(const std::string& _name, const std::string& _value) = 0;
    virtual std::string getConfigParameter(const std::string& _name) const = 0;

    virtual void setZoneID(const int _value) { m_ZoneID = _value; }
    virtual int getZoneID() const { return m_ZoneID; }

    bool isLocked() const { return m_IsLocked; }
    void setIsLocked(const bool _value) { m_IsLocked = _value; }
  }; // DSIDInterface


}

#endif /*DSSIM_H_*/