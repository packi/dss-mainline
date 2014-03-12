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

#include "dsmetersim.h"

#include "dssim.h"

#include "src/base.h"
#include "src/foreach.h"

namespace dss {

  //================================================== DSMeterSim

  DSMeterSim::DSMeterSim(DSSim* _pSimulation)
  : m_pSimulation(_pSimulation),
    m_EnergyLevelOrange(200),
    m_EnergyLevelRed(400)
  {
    m_DSMeterDSID = DSSim::makeSimulatedDSID(dss_dsid_t());
    m_ID = 70;
    m_Name = "Simulated dSM";
  } // dSDSMeterSim

  void DSMeterSim::log(const std::string& _message, aLogSeverity _severity) {
    m_pSimulation->log(_message, _severity);
  } // log

  bool DSMeterSim::initializeFromNode(PropertyNodePtr _node) {
    if(_node == NULL) {
      return false;
    }
    if(_node->getProperty("busid")) {
      m_ID = strToIntDef(_node->getProperty("busid")->getStringValue(), 70);
    }
    if(_node->getProperty("dsid")) {
      m_DSMeterDSID = DSSim::makeSimulatedDSID(dss_dsid_t::fromString(
          _node->getProperty("dsid")->getStringValue()));
    }
    if(_node->getProperty("name")) {
      m_Name = _node->getProperty("name")->getStringValue();
    }
    loadDevices(_node, 0);
    loadGroups(_node, 0);
    loadZones(_node);
    return true;
  } // initializeFromNode

  void DSMeterSim::loadDevices(PropertyNodePtr _node, const int _zoneID) {
  } // loadDevices

  void DSMeterSim::loadGroups(PropertyNodePtr _node, const int _zoneID) {
  } // loadGroups

  void DSMeterSim::loadZones(PropertyNodePtr _node) {
  } // loadZones

  void DSMeterSim::addDeviceToGroup(DSIDInterface* _device, int _groupID) {
    m_DevicesOfGroupInZone[std::pair<const int, const int>(_device->getZoneID(), _groupID)].push_back(_device);
    m_DevicesOfGroupInZone[std::pair<const int, const int>(0, _groupID)].push_back(_device);
    m_GroupsPerDevice[_device->getShortAddress()].push_back(_groupID);
  } // addDeviceToGroup

  void DSMeterSim::addGroup(uint16_t _zoneID, uint8_t _groupID) {
    m_DevicesOfGroupInZone[std::pair<const int, const int>(_zoneID, _groupID)];
  } // addGroup

  void DSMeterSim::removeGroup(uint16_t _zoneID, uint8_t _groupID) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInZone.find(zonesGroup);
    if(it != m_DevicesOfGroupInZone.end()) {
      m_DevicesOfGroupInZone.erase(it);
    }
  } // removeGroup

  void DSMeterSim::sensorPush(uint16_t _zoneID, uint8_t _groupID, uint32_t _sourceSerialNumber, uint8_t _sensorType, uint16_t _sensorValue) {
    // TODO: validate sourceID against local push table
    foreach(DSIDInterface* interface, m_SimulatedDevices) {
      interface->sensorPush(_sensorType, _sensorValue);
    }
  } // sensorPush

  void DSMeterSim::removeDeviceFromGroup(DSIDInterface* _pDevice, int _groupID) {
    std::pair<const int, const int> zoneGroupPair(_pDevice->getZoneID(), _groupID);
    std::vector<DSIDInterface*>& interfaceVector =
      m_DevicesOfGroupInZone[zoneGroupPair];
    std::vector<DSIDInterface*>::iterator iDevice =
      find(interfaceVector.begin(),
           interfaceVector.end(),
           _pDevice);
    if(iDevice != interfaceVector.end()) {
      interfaceVector.erase(iDevice);
    }
    std::vector<int>& groupsVector = m_GroupsPerDevice[_pDevice->getShortAddress()];
    std::vector<int>::iterator iGroup =
      find(groupsVector.begin(), groupsVector.end(), _groupID);
    if(iGroup != groupsVector.end()) {
      groupsVector.erase(iGroup);
    }
  } // removeDeviceFromGroup

  void DSMeterSim::deviceCallScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).callScene(_sceneID);
  } // deviceCallScene

  void DSMeterSim::groupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
    std::vector<DSIDInterface*> dsids;
    m_LastCalledSceneForZoneAndGroup[std::make_pair(_zoneID, _groupID)] = _sceneID;
    if(_groupID == GroupIDBroadcast) {
      if(_zoneID == 0) {
        dsids = m_SimulatedDevices;
      } else {
        dsids = m_Zones[_zoneID];
      }
    } else {
      std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
      if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
        dsids = m_DevicesOfGroupInZone[zonesGroup];
      }
  }
    for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
        iDSID != e; ++iDSID)
    {
      (*iDSID)->callScene(_sceneID);
    }
  } // groupCallScene

  void DSMeterSim::deviceSaveScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).saveScene(_sceneID);
  } // deviceSaveScene

  void DSMeterSim::groupSaveScene(const int _zoneID, const int _groupID, const int _sceneID) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->saveScene(_sceneID);
      }
    }
  } // groupSaveScene

  void DSMeterSim::deviceIncreaseOutputChannelValue(int _deviceID, const uint8_t _channel) {
    lookupDevice(_deviceID).increaseDeviceOutputChannelValue(_channel);
  } // increaseOutputChannelValue

  void DSMeterSim::groupIncreaseOutputChannelValue(const int _zoneID, const int _groupID, const uint8_t _channel) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->increaseDeviceOutputChannelValue(_channel);
      }
    }
  } // increaseOutputChannelValue

  void DSMeterSim::deviceDecreaseOutputChannelValue(int _deviceID, const uint8_t _channel) {
    lookupDevice(_deviceID).decreaseDeviceOutputChannelValue(_channel);
  } // decreaseOutputChannelValue

  void DSMeterSim::groupDecreaseOutputChannelValue(const int _zoneID, const int _groupID, const uint8_t _channel) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->decreaseDeviceOutputChannelValue(_channel);
      }
    }
  } // decreaseOutputChannelValue

  void DSMeterSim::deviceStopOutputChannelValue(int _deviceID, const uint8_t _channel) {
    lookupDevice(_deviceID).decreaseDeviceOutputChannelValue(_channel);
  } // stopOutputChannelValue

  void DSMeterSim::groupStopOutputChannelValue(const int _zoneID, const int _groupID, const uint8_t _channel) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->stopDeviceOutputChannelValue(_channel);
      }
    }
  } // stopOutputChannelValue

  void DSMeterSim::deviceUndoScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).undoScene(_sceneID);
  } // deviceUndoScene

  void DSMeterSim::groupUndoScene(const int _zoneID, const int _groupID, const int _sceneID) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->undoScene(_sceneID);
      }
    }
  } // groupUndoScene

  void DSMeterSim::deviceUndoSceneLast(int _deviceID) {
    lookupDevice(_deviceID).undoSceneLast();
  } // deviceUndoSceneLast

  void DSMeterSim::groupUndoSceneLast(const int _zoneID, const int _groupID) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->undoSceneLast();
      }
    }
  } // groupUndoSceneLast

  void DSMeterSim::groupSetValue(const int _zoneID, const int _groupID, const uint8_t _value) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->setValue(_value);
      }
    }
  } // groupSetValue

  void DSMeterSim::deviceSetValue(const int _deviceID, const uint8_t _value) {
    lookupDevice(_deviceID).setValue(_value);
  } // deviceSetValue

  uint32_t DSMeterSim::getPowerConsumption() {
    uint32_t val = 0;
    foreach(DSIDInterface* interface, m_SimulatedDevices) {
      val += interface->getPowerConsumption();
    }
    return val;
  } // getPowerConsumption

  uint32_t DSMeterSim::getEnergyMeterValue() {
    uint32_t val = 0;
    foreach(DSIDInterface* interface, m_SimulatedDevices) {
      val += interface->getEnergyMeterValue();
    }
    return val;
  } // getEnergyMeterValue

  std::vector<int> DSMeterSim::getZones() {
    std::vector<int> result;
    typedef std::map< const int, std::vector<DSIDInterface*> >::iterator
            ZoneIterator_t;
    for(ZoneIterator_t it = m_Zones.begin(), e = m_Zones.end(); it != e; ++it) {
      result.push_back(it->first);
    }
    return result;
  } // getZones

  std::vector<DSIDInterface*> DSMeterSim::getDevicesOfZone(const int _zoneID) {
    return m_Zones[_zoneID];
  } // getDevicesOfZone

  std::vector<int> DSMeterSim::getGroupsOfZone(const int _zoneID) {
    std::vector<int> result;
    IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInZone.begin();
    IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInZone.end();
    while(it != end) {
      if(it->first.first == _zoneID) {
        result.push_back(it->first.second);
      }
      it++;
    }
    return result;
  } // getGroupsOfZone

  std::vector<int> DSMeterSim::getGroupsOfDevice(const int _deviceID) {
    return m_GroupsPerDevice[_deviceID];
  } // getGroupsOfDevice

  void DSMeterSim::moveDevice(const int _deviceID, const int _toZoneID) {
    DSIDInterface& dev = lookupDevice(_deviceID);

    int oldZoneID = m_DeviceZoneMapping[&dev];
    std::vector<DSIDInterface*>::iterator oldEntry = find(m_Zones[oldZoneID].begin(), m_Zones[oldZoneID].end(), &dev);
    m_Zones[oldZoneID].erase(oldEntry);
    m_Zones[_toZoneID].push_back(&dev);
    m_DeviceZoneMapping[&dev] = _toZoneID;
    dev.setZoneID(_toZoneID);
  } // moveDevice

  void DSMeterSim::addZone(const int _zoneID) {
    m_Zones[_zoneID].size(); // accessing a nonexisting entry creates one
  } // addZone

  void DSMeterSim::removeZone(const int _zoneID) {
    for(std::map< const int, std::vector<DSIDInterface*> >::iterator iZoneEntry = m_Zones.begin(), e = m_Zones.end();
        iZoneEntry != e; ++iZoneEntry)
    {
      if(iZoneEntry->first == _zoneID) {
        if(iZoneEntry->second.empty()) {
          m_Zones.erase(iZoneEntry);
        } else {
          log("[DSMSim] Can't delete zone with id " + intToString(_zoneID) + " as it still contains some devices.", lsError);
          // TODO: throw?
        }
        break;
      }
    }
  } // removeZone

  DSIDInterface& DSMeterSim::lookupDevice(const devid_t _shortAddress) {
    for(std::vector<DSIDInterface*>::iterator ipSimDev = m_SimulatedDevices.begin(); ipSimDev != m_SimulatedDevices.end(); ++ipSimDev) {
      if((*ipSimDev)->getShortAddress() == _shortAddress)  {
        return **ipSimDev;
      }
    }
    throw std::runtime_error(std::string("could not find device with id: ") + intToString(_shortAddress));
  } // lookupDevice

  int DSMeterSim::getID() const {
    return m_ID;
  } // getID

  DSIDInterface* DSMeterSim::getSimulatedDevice(const dss_dsid_t _dsid) {
    for(std::vector<DSIDInterface*>::iterator iDSID = m_SimulatedDevices.begin(), e = m_SimulatedDevices.end();
        iDSID != e; ++iDSID)
    {
      if((*iDSID)->getDSID() == _dsid) {
        return *iDSID;
      }
    }
    return NULL;
  } // getSimulatedDevice

  DSIDInterface* DSMeterSim::getSimulatedDevice(const uint16_t _shortAddress) {
    for(std::vector<DSIDInterface*>::iterator iDSID = m_SimulatedDevices.begin(), e = m_SimulatedDevices.end();
        iDSID != e; ++iDSID)
    {
      if((*iDSID)->getShortAddress() == _shortAddress) {
        return *iDSID;
      }
    }
    return NULL;
  } // getSimulatedDevice

  void DSMeterSim::addSimulatedDevice(DSIDInterface* _device) {
    m_SimulatedDevices.push_back(_device);
  } // addSimulatedDevice

} // namespace dss
