/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef DSMETERSIM_H_
#define DSMETERSIM_H_

#include <map>
#include <vector>

#include "src/ds485types.h"
#include "src/logger.h"

namespace Poco {
  namespace XML {
    class Node;
  }
}

namespace dss {
  class DSSim;
  class DSIDInterface;

  class DSMeterSim {
  private:
    typedef std::map< const std::pair<const int, const int>,  std::vector<DSIDInterface*> > IntPairToDSIDSimVector;

    DSSim* m_pSimulation;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_ID;
    dss_dsid_t m_DSMeterDSID;
    std::vector<DSIDInterface*> m_SimulatedDevices;
    std::map< const int, std::vector<DSIDInterface*> > m_Zones;
    IntPairToDSIDSimVector m_DevicesOfGroupInZone;
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
  public:
    void deviceCallScene(const int _deviceID, const int _sceneID);
    void groupCallScene(const int _zoneID, const int _groupID, const int _sceneID);
    void deviceSaveScene(const int _deviceID, const int _sceneID);
    void groupSaveScene(const int _zoneID, const int _groupID, const int _sceneID);
    void deviceUndoScene(const int _deviceID, const int _sceneID);
    void groupUndoScene(const int _zoneID, const int _groupID, const int _sceneID);
    void deviceUndoSceneLast(int _deviceID);
    void groupUndoSceneLast(const int _zoneID, const int _groupID);
    void deviceSetValue(const int _deviceID, const uint8_t _value);
    void groupSetValue(const int _zoneID, const int _groupID, const uint8_t _value);
  public:
    uint32_t getPowerConsumption();
    uint32_t getEnergyMeterValue();
  public:
    std::vector<int> getZones();
    std::vector<DSIDInterface*> getDevicesOfZone(const int _zoneID);
    std::vector<int> getGroupsOfZone(const int _zoneID);
    std::vector<int> getGroupsOfDevice(const int _deviceID);
  public:
    void moveDevice(const int _deviceID, const int _toZoneID);
    void addZone(const int _zoneID);
    void removeZone(const int _zoneID);
  protected:
    virtual void doStart() {}
    void log(const std::string& _message, aLogSeverity _severity = lsDebug);
  public:
    DSMeterSim(DSSim* _pSimulator);
    virtual ~DSMeterSim() {}

    bool initializeFromNode(Poco::XML::Node* _node);

    int getID() const;
    void setID(const int _value) { m_ID = _value; }

    void setDSID(const dss_dsid_t& _value) { m_DSMeterDSID = _value; }
    const dss_dsid_t& getDSID() { return m_DSMeterDSID; }

    DSIDInterface* getSimulatedDevice(const dss_dsid_t _dsid);
    DSIDInterface* getSimulatedDevice(const uint16_t _shortAddress);
    DSIDInterface& lookupDevice(const devid_t _id);
    void addSimulatedDevice(DSIDInterface* _device);

    void addDeviceToGroup(DSIDInterface* _device, int _groupID);
    void removeDeviceFromGroup(DSIDInterface* _pDevice, int _groupID);
    void addGroup(uint16_t _zoneID, uint8_t _groupID);
    void removeGroup(uint16_t _zoneID, uint8_t _groupID);
    void sensorPush(uint16_t _zoneID, uint32_t _sourceSerialNumber, uint8_t _sensorType, uint16_t _sensorValue);
  }; // DSMeterSim

}

#endif
