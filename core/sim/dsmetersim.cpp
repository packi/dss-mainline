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

#include "core/base.h"
#include "core/foreach.h"

#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>

using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::Node;


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

  bool DSMeterSim::initializeFromNode(Node* _node) {
    Element* elem = dynamic_cast<Element*>(_node);
    if(elem == NULL) {
      return false;
    }
    if(elem->hasAttribute("busid")) {
      m_ID = strToIntDef(elem->getAttribute("busid"), 70);
    }
    if(elem->hasAttribute("dsid")) {
      m_DSMeterDSID = DSSim::makeSimulatedDSID(dss_dsid_t::fromString(elem->getAttribute("dsid")));
    }
    if(elem->hasAttribute("orange")) {
      m_EnergyLevelOrange = strToIntDef(elem->getAttribute("orange"), m_EnergyLevelOrange);
    }
    if(elem->hasAttribute("red")) {
      m_EnergyLevelRed = strToIntDef(elem->getAttribute("red"), m_EnergyLevelRed);
    }
    Element* nameElem = elem->getChildElement("name");
    if(nameElem != NULL && nameElem->hasChildNodes()) {
      m_Name = nameElem->firstChild()->nodeValue();
    }

    loadDevices(_node, 0);
    loadGroups(_node, 0);
    loadZones(_node);
    return true;
  } // initializeFromNode

  void DSMeterSim::loadDevices(Node* _node, const int _zoneID) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      Element* elem = dynamic_cast<Element*>(curNode);
      if(curNode->localName() == "device" && elem != NULL) {
        dss_dsid_t dsid = NullDSID;
        int busid = -1;
        if(elem->hasAttribute("dsid")) {
          dsid = DSSim::makeSimulatedDSID(dss_dsid_t::fromString(elem->getAttribute("dsid")));
        }
        if(elem->hasAttribute("busid")) {
          busid = strToInt(elem->getAttribute("busid"));
        }
        if((dsid == NullDSID) || (busid == -1)) {
          log("missing dsid or busid of device");
          continue;
        }
        std::string type = "standard.simple";
        if(elem->hasAttribute("type")) {
          type = elem->getAttribute("type");
        }

        DSIDInterface* newDSID = m_pSimulation->getDSIDFactory().createDSID(type, dsid, busid, *this);
        Node* childNode = curNode->firstChild();
        while(childNode != NULL) {
          if(childNode->localName() == "parameter") {
            Element* childElem = dynamic_cast<Element*>(childNode);
            if(childElem != NULL) {
              if(childElem->hasAttribute("name") && childElem->hasChildNodes()) {
                std::string paramName = childElem->getAttribute("name");
                std::string paramValue = childElem->firstChild()->getNodeValue();
                log("LoadDevices:   Found parameter '" + paramName + "' with value '" + paramValue + "'");
                newDSID->setConfigParameter(paramName, paramValue);
              }
            }
          } else if(childNode->localName() == "name" && childNode->hasChildNodes()) {
            m_DeviceNames[busid] = childNode->firstChild()->nodeValue();
          }
          childNode = childNode->nextSibling();
        }
        if(newDSID != NULL) {
          m_SimulatedDevices.push_back(newDSID);
          if(_zoneID != 0) {
            m_Zones[_zoneID].push_back(newDSID);
          }
          m_DeviceZoneMapping[newDSID] = _zoneID;
          newDSID->setZoneID(_zoneID);
          newDSID->initialize();
          log("LoadDevices: found device");
        } else {
          log("LoadDevices: could not create instance for type \"" + type + "\"");
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadDevices

  void DSMeterSim::loadGroups(Node* _node, const int _zoneID) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "group") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if(elem != NULL) {
          if(elem->hasAttribute("id")) {
            int groupID = strToIntDef(elem->getAttribute("id"), -1);
            Node* childNode = curNode->firstChild();
            while(childNode != NULL) {
              if(childNode->localName() == "device") {
                Element* childElem = dynamic_cast<Element*>(childNode);
                if(childElem->hasAttribute("busid")) {
                  unsigned long busID = strToUInt(childElem->getAttribute("busid"));
                  DSIDInterface& dev = lookupDevice(busID);
                  addDeviceToGroup(&dev, groupID);
                  log("LoadGroups: Adding device " + intToString(busID) + " to group " + intToString(groupID) + " in zone " + intToString(_zoneID));
                }
              }
              childNode = childNode->nextSibling();
            }
          } else {
            log("LoadGroups: Could not find attribute id of group, skipping entry", lsError);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadGroups

  void DSMeterSim::addDeviceToGroup(DSIDInterface* _device, int _groupID) {
    m_DevicesOfGroupInZone[std::pair<const int, const int>(_device->getZoneID(), _groupID)].push_back(_device);
    m_GroupsPerDevice[_device->getShortAddress()].push_back(_groupID);
  } // addDeviceToGroup

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

  void DSMeterSim::loadZones(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "zone") {
        Element* elem = dynamic_cast<Element*>(curNode);
        int zoneID = -1;
        if(elem != NULL && elem->hasAttribute("id")) {
          zoneID = strToIntDef(elem->getAttribute("id"), -1);
        }
        if(zoneID != -1) {
          log("LoadZones: found zone (" + intToString(zoneID) + ")");
          loadDevices(curNode, zoneID);
          loadGroups(curNode, zoneID);
        } else {
          log("LoadZones: could not find/parse id for zone");
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadZones

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

  void DSMeterSim::deviceUndoScene(int _deviceID) {
    lookupDevice(_deviceID).undoScene();
  } // deviceUndoScene

  void DSMeterSim::groupUndoScene(const int _zoneID, const int _groupID) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->undoScene();
      }
    }
  } // groupUndoScene

  void DSMeterSim::groupSetValue(const int _zoneID, const int _groupID, const int _value) {
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

  void DSMeterSim::deviceSetValue(const int _deviceID, const int _value) {
    lookupDevice(_deviceID).setValue(_value);
  } // deviceSetValue

  uint32_t DSMeterSim::getEnergyMeterValue() {
    return 0;
  }

  uint32_t DSMeterSim::getPowerConsumption() {
    uint32_t val = 0;
    foreach(DSIDInterface* interface, m_SimulatedDevices) {
      val += interface->getConsumption();
    }
    return val;
  } // getPowerConsumption

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
