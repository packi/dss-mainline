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

#include "dssim.h"
#include "dsidsim.h"

#include <dlfcn.h>

#include <string>
#include <stdexcept>
#include <iostream>

#include <boost/filesystem.hpp>

#include "core/ds485const.h"
#include "core/base.h"
#include "core/logger.h"
#include "core/dss.h"
#include "include/dsid_plugin.h"
#include "core/DS485Interface.h"
#include "core/foreach.h"
#include "core/propertysystem.h"
#include "dsid_js.h"
#include "dsid_plugin.h"

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/SAX/SAXException.h>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::ProcessingInstruction;
using Poco::XML::AutoPtr;
using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::Node;

namespace fs = boost::filesystem;

namespace dss {

  //================================================== DSIDSimCreator

  class DSIDSimCreator : public DSIDCreator {
  public:
    DSIDSimCreator()
    : DSIDCreator("standard.simple")
    {}

    virtual ~DSIDSimCreator() {};

    virtual DSIDInterface* createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSDSMeterSim& _dsMeter) {
      return new DSIDSim(_dsMeter, _dsid, _shortAddress);
    }
  };

  //================================================== DSIDSimCreator

  class DSIDSimSwitchCreator : public DSIDCreator {
  public:
    DSIDSimSwitchCreator()
    : DSIDCreator("standard.switch")
    {}

    virtual ~DSIDSimSwitchCreator() {};

    virtual DSIDInterface* createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSDSMeterSim& _dsMeter) {
      return new DSIDSimSwitch(_dsMeter, _dsid, _shortAddress, 9);
    }
  };

  //================================================== DSSim

  DSSim::DSSim(DSS* _pDSS)
  : Subsystem(_pDSS, "DSSim")
  {}

  void DSSim::initialize() {
    Subsystem::initialize();
    m_DSIDFactory.registerCreator(new DSIDSimCreator());
    m_DSIDFactory.registerCreator(new DSIDSimSwitchCreator());

    PropertyNodePtr pNode = getDSS().getPropertySystem().getProperty(getConfigPropertyBasePath() + "js-devices");
    if(pNode != NULL) {
      for(int iNode = 0; iNode < pNode->getChildCount(); iNode++) {
        PropertyNodePtr pChildNode = pNode->getChild(iNode);
        PropertyNodePtr scriptFileNode = pChildNode->getPropertyByName("script-file");
        PropertyNodePtr simIDNode = pChildNode->getPropertyByName("id");
        if(scriptFileNode != NULL) {
          if(simIDNode != NULL) {
            std::string scriptFile = scriptFileNode->getAsString();
            if(!boost::filesystem::exists(scriptFile)) {
              log("DSSim::initialize: cannot find script file '" + scriptFile + "', skipping", lsError);
              continue;
            }
            log("Found js-device with script '" + scriptFile + "' and id: '" + simIDNode->getAsString() + "'", lsInfo);
            m_DSIDFactory.registerCreator(new DSIDJSCreator(scriptFile, simIDNode->getAsString(), *this));
          } else {
            log("DSSim::initialize: Missing property id", lsError);
          }
        } else {
          log("DSSim::initialize: Missing property script-file", lsError);
        }
      }
    }

    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "configfile", getDSS().getConfigDirectory() + "sim.xml", true, false);

    loadPlugins();
    loadFromConfig();

    m_Initialized = true;
  } // initialize

  void DSSim::loadFromConfig() {
    const int theConfigFileVersion = 1;
    std::string filename = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile");
    std::ifstream inFile(filename.c_str());

    try {
      InputSource input(inFile);
      DOMParser parser;
      AutoPtr<Document> pDoc = parser.parse(&input);
      Element* rootNode = pDoc->documentElement();

      if(rootNode->localName() == "simulation") {
        if(rootNode->hasAttribute("version") && (strToInt(rootNode->getAttribute("version")) == theConfigFileVersion)) {
          Node* curNode = rootNode->firstChild();
          while(curNode != NULL) {
            if(curNode->localName() == "modulator") {
              DSDSMeterSim* dsMeter = NULL;
              try {
                dsMeter = new DSDSMeterSim(this);
                dsMeter->initializeFromNode(curNode);
                m_DSMeters.push_back(dsMeter);
              } catch(std::runtime_error&) {
                delete dsMeter;
              }
            }
            curNode = curNode->nextSibling();
          }
        }
      } else {
        log(filename + " must have a root-node named 'simulation'", lsFatal);
      }
    } catch(Poco::XML::SAXParseException& e) {
      log("Error parsing file: " + filename + ". message: " + e.message());
    }
  } // loadFromConfig

  void DSSim::loadPlugins() {
    fs::directory_iterator end_iter;
    try {
      for ( fs::directory_iterator dir_itr(getDSS().getDataDirectory() + "plugins");
            dir_itr != end_iter;
            ++dir_itr )
      {
        try {
          if (fs::is_regular(dir_itr->status())) {
            if(endsWith(dir_itr->leaf(), ".so")) {
              log("LoadPlugins: Trying to load '" + dir_itr->string() + "'", lsInfo);
              void* handle = dlopen(dir_itr->string().c_str(), RTLD_LAZY);
              if(handle == NULL) {
                log("LoadPlugins: Could not load plugin \"" + dir_itr->leaf() + "\" message: " + dlerror(), lsError);
                continue;
              }

              dlerror();
              int (*version)();
              version = (int (*)())dlsym(handle, "dsid_getversion");
              char* error;
              if((error = dlerror()) != NULL) {
                 log("LoadPlugins: Could not get symbol 'dsid_getversion' from plugin: \"" + dir_itr->leaf() + "\":" + error, lsError);
                 continue;
              }

              int ver = (*version)();
              if(ver != DSID_PLUGIN_API_VERSION) {
                log("LoadPlugins: Version mismatch (plugin: " + intToString(ver) + " api: " + intToString(DSID_PLUGIN_API_VERSION) + ")", lsError);
                continue;
              }

              const char* (*get_name)();
              get_name = (const char*(*)())dlsym(handle, "dsid_get_plugin_name");
              if((error = dlerror()) != NULL) {
                log("LoadPlugins: could get name from \"" + dir_itr->leaf() + "\":" + error, lsError);
                continue;
              }
              const char* pluginName = (*get_name)();
              if(pluginName == NULL) {
                log("LoadPlugins: could get name from \"" + dir_itr->leaf() + "\":" + error, lsError);
                continue;
              }
              log("LoadPlugins: Plugin provides " + std::string(pluginName), lsInfo);
              m_DSIDFactory.registerCreator(new DSIDPluginCreator(handle, pluginName));
            }
          }
        } catch (const std::exception & ex) {
          log("LoadPlugins: Caught exception while loading " + dir_itr->leaf() + " '" + ex.what() + "'", lsError);
        }
      }
    } catch(const std::exception& ex) {
      log(std::string("Error loading plugins: '") + ex.what() + "'");
    }
  } // loadPlugins

  bool DSSim::isReady() {
    return m_Initialized;
  } // ready

  bool DSSim::isSimAddress(const uint8_t _address) {
    foreach(DSDSMeterSim& dsMeter, m_DSMeters) {
      if(dsMeter.getID() == _address) {
        return true;
      }
    }
    return false;
  } // isSimAddress

  void DSSim::process(DS485Frame& _frame) {
    foreach(DSDSMeterSim& dsMeter, m_DSMeters) {
      dsMeter.process(_frame);
    }
  } // process

  void DSSim::distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    DS485FrameProvider::distributeFrame(_frame);
  } // distributeFrame

  DSIDInterface* DSSim::getSimulatedDevice(const dsid_t& _dsid) {
    DSIDInterface* result = NULL;
    foreach(DSDSMeterSim& dsMeter, m_DSMeters) {
      result = dsMeter.getSimulatedDevice(_dsid);
      if(result != NULL) {
        break;
      }
    }
    return result;
  } // getSimulatedDevice


  //================================================== DSDSMeterSim

  DSDSMeterSim::DSDSMeterSim(DSSim* _pSimulation)
  : m_pSimulation(_pSimulation),
    m_EnergyLevelOrange(200),
    m_EnergyLevelRed(400)
  {
    m_DSMeterDSID = dsid_t(DSIDHeader, SimulationPrefix);
    m_ID = 70;
    m_Name = "Simulated dSM";
  } // dSDSMeterSim

  void DSDSMeterSim::log(const std::string& _message, aLogSeverity _severity) {
    m_pSimulation->log(_message, _severity);
  } // log

  bool DSDSMeterSim::initializeFromNode(Node* _node) {
    Element* elem = dynamic_cast<Element*>(_node);
    if(elem == NULL) {
      return false;
    }
    if(elem->hasAttribute("busid")) {
      m_ID = strToIntDef(elem->getAttribute("busid"), 70);
    }
    if(elem->hasAttribute("dsid")) {
      m_DSMeterDSID = dsid_t::fromString(elem->getAttribute("dsid"));
      m_DSMeterDSID.upper = (m_DSMeterDSID.upper & 0x000000000000000Fll) | DSIDHeader;
      m_DSMeterDSID.lower = (m_DSMeterDSID.lower & 0x002FFFFF) | SimulationPrefix;
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

  void DSDSMeterSim::loadDevices(Node* _node, const int _zoneID) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      Element* elem = dynamic_cast<Element*>(curNode);
      if(curNode->localName() == "device" && elem != NULL) {
        dsid_t dsid = NullDSID;
        int busid = -1;
        if(elem->hasAttribute("dsid")) {
          dsid = dsid_t::fromString(elem->getAttribute("dsid"));
          dsid.upper = (dsid.upper & 0x000000000000000Fll) | DSIDHeader;
          dsid.lower = (dsid.lower & 0x002FFFFF) | SimulationPrefix;
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

          DSIDSimSwitch* sw = dynamic_cast<DSIDSimSwitch*>(newDSID);
          if(sw != NULL) {
            log("LoadDevices:   it's a switch");
            if(elem->hasAttribute("bell")) {
              sw->setIsBell(elem->getAttribute("bell") == "true");
              if(sw->isBell()) {
                log("LoadDevices:   switch is bell");
              }
            }
          }

          newDSID->initialize();
          log("LoadDevices: found device");
        } else {
          log("LoadDevices: could not create instance for type \"" + type + "\"");
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadDevices

  void DSDSMeterSim::loadGroups(Node* _node, const int _zoneID) {
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

  void DSDSMeterSim::addDeviceToGroup(DSIDInterface* _device, int _groupID) {
    m_DevicesOfGroupInZone[std::pair<const int, const int>(_device->getZoneID(), _groupID)].push_back(_device);
    m_GroupsPerDevice[_device->getShortAddress()].push_back(_groupID);
  } // addDeviceToGroup

  void DSDSMeterSim::loadZones(Node* _node) {
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

  void DSDSMeterSim::deviceCallScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).callScene(_sceneID);
  } // deviceCallScene

  void DSDSMeterSim::groupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
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

  void DSDSMeterSim::deviceSaveScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).saveScene(_sceneID);
  } // deviceSaveScene

  void DSDSMeterSim::groupSaveScene(const int _zoneID, const int _groupID, const int _sceneID) {
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

  void DSDSMeterSim::deviceUndoScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).undoScene(_sceneID);
  } // deviceUndoScene

  void DSDSMeterSim::groupUndoScene(const int _zoneID, const int _groupID, const int _sceneID) {
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

  void DSDSMeterSim::groupStartDim(const int _zoneID, const int _groupID, bool _up, int _parameterNr) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->startDim(_up, _parameterNr);
      }
    }
  } // groupStartDim

  void DSDSMeterSim::groupEndDim(const int _zoneID, const int _groupID, const int _parameterNr) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->endDim(_parameterNr);
      }
    }
  } // groupEndDim

  void DSDSMeterSim::groupDecValue(const int _zoneID, const int _groupID, const int _parameterNr) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->decreaseValue(_parameterNr);
      }
    }
  } // groupDecValue

  void DSDSMeterSim::groupIncValue(const int _zoneID, const int _groupID, const int _parameterNr) {
    std::pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(std::vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->increaseValue(_parameterNr);
      }
    }
  } // groupIncValue

  void DSDSMeterSim::groupSetValue(const int _zoneID, const int _groupID, const int _value) {
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

  void DSDSMeterSim::process(DS485Frame& _frame) {
    const uint8_t HeaderTypeToken = 0;
    const uint8_t HeaderTypeCommand = 1;

    try {
      DS485Header& header = _frame.getHeader();
      if(!(header.getDestination() == m_ID || header.isBroadcast())) {
        return;
      }
      if(header.getType() == HeaderTypeToken) {
        // Transmit pending things
      } else if(header.getType() == HeaderTypeCommand) {
        DS485CommandFrame& cmdFrame = dynamic_cast<DS485CommandFrame&>(_frame);
        PayloadDissector pd(cmdFrame.getPayload());
        if((cmdFrame.getCommand() == CommandRequest) && !pd.isEmpty()) {
          int cmdNr = pd.get<uint8_t>();
          boost::shared_ptr<DS485CommandFrame> response;
          switch(cmdNr) {
            case FunctionDeviceCallScene:
              {
                devid_t devID = pd.get<devid_t>();
                int sceneID = pd.get<uint16_t>();
                deviceCallScene(devID, sceneID);
                distributeFrame(boost::shared_ptr<DS485CommandFrame>(createAck(cmdFrame, cmdNr)));
              }
              break;
            case FunctionGroupCallScene:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint8_t groupID = pd.get<uint16_t>();
                uint8_t sceneID = pd.get<uint16_t>();
                groupCallScene(zoneID, groupID, sceneID);
              }
              break;
            case FunctionDeviceSaveScene:
              {
                devid_t devID = pd.get<devid_t>();
                int sceneID = pd.get<uint16_t>();
                deviceSaveScene(devID, sceneID);
                distributeFrame(boost::shared_ptr<DS485CommandFrame>(createAck(cmdFrame, cmdNr)));
              }
              break;
            case FunctionGroupSaveScene:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint8_t groupID = pd.get<uint16_t>();
                uint8_t sceneID = pd.get<uint16_t>();
                groupSaveScene(zoneID, groupID, sceneID);
              }
              break;
            case FunctionDeviceUndoScene:
              {
                devid_t devID = pd.get<devid_t>();
                int sceneID = pd.get<uint16_t>();
                deviceUndoScene(devID, sceneID);
                distributeFrame(boost::shared_ptr<DS485CommandFrame>(createAck(cmdFrame, cmdNr)));
              }
              break;
            case FunctionGroupUndoScene:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint16_t groupID = pd.get<uint16_t>();
                uint16_t sceneID = pd.get<uint16_t>();
                groupUndoScene(zoneID, groupID, sceneID);
              }
              break;
            case FunctionGroupIncreaseValue:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint16_t groupID = pd.get<uint16_t>();
                groupIncValue(zoneID, groupID, 0);
              }
              break;
            case FunctionGroupDecreaseValue:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint16_t groupID = pd.get<uint16_t>();
                groupDecValue(zoneID, groupID, 0);
              }
              break;
            case FunctionGroupSetValue:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint16_t groupID = pd.get<uint16_t>();
                uint16_t value = pd.get<uint16_t>();
                groupSetValue(zoneID, groupID, value);
              }
              break;
            case FunctionDeviceIncreaseValue:
              {
                uint16_t devID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);
                dev.increaseValue();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(1);
                distributeFrame(response);
              }
              break;
            case FunctionDeviceDecreaseValue:
              {
                uint16_t devID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);
                dev.decreaseValue();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(1);
                distributeFrame(response);
              }
              break;
            case FunctionDeviceGetFunctionID:
              {
                devid_t devID = pd.get<devid_t>();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(0x0001); // everything ok
                response->getPayload().add<uint16_t>(lookupDevice(devID).getFunctionID());
                distributeFrame(response);
              }
              break;
            case FunctionDeviceSetParameterValue:
              {
                uint16_t devID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);
                uint16_t parameterID = pd.get<uint16_t>();
                /* uint16_t size = */ pd.get<uint16_t>();
                uint16_t value = pd.get<uint16_t>();
                dev.setValue(value, parameterID);
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(1);
                distributeFrame(response);
              }
              break;
            case FunctionDeviceSetValue:
              {
                uint16_t devID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);
                uint16_t value = pd.get<uint16_t>();
                dev.setValue(value);
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(1);
                distributeFrame(response);
              }
              break;
            case FunctionDeviceGetParameterValue:
              {
                int devID = pd.get<uint16_t>();
                int paramID = pd.get<uint16_t>();
                double result = lookupDevice(devID).getValue(paramID);
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(static_cast<uint8_t>(result));
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterGetZonesSize:
              {
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_Zones.size());
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterGetZoneIdForInd:
              {
                uint8_t index = pd.get<uint16_t>();
                std::map< const int, std::vector<DSIDInterface*> >::iterator it = m_Zones.begin();
                std::advance(it, index);
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(it->first);
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterCountDevInZone:
              {
                uint16_t index = pd.get<uint16_t>();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_Zones[index].size());
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterDevKeyInZone:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint16_t deviceIndex = pd.get<devid_t>();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_Zones[zoneID].at(deviceIndex)->getShortAddress());
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterGetGroupsSize:
              {
                response = createResponse(cmdFrame, cmdNr);
                int zoneID = pd.get<uint16_t>();
                IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInZone.begin();
                IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInZone.end();
                int result = 0;
                while(it != end) {
                  if(it->first.first == zoneID) {
                    result++;
                  }
                  it++;
                }
                response->getPayload().add<uint16_t>(result);
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterGetDSID:
              {
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<dsid_t>(m_DSMeterDSID);
                distributeFrame(response);
              }
              break;
            case FunctionGroupGetDeviceCount:
              {
                response = createResponse(cmdFrame, cmdNr);
                int zoneID = pd.get<uint16_t>();
                int groupID = pd.get<uint16_t>();
                int result = m_DevicesOfGroupInZone[std::pair<const int, const int>(zoneID, groupID)].size();
                response->getPayload().add<uint16_t>(result);
                distributeFrame(response);
              }
              break;
            case FunctionGroupGetDevKeyForInd:
              {
                response = createResponse(cmdFrame, cmdNr);
                int zoneID = pd.get<uint16_t>();
                int groupID = pd.get<uint16_t>();
                int index = pd.get<uint16_t>();
                int result = m_DevicesOfGroupInZone[std::pair<const int, const int>(zoneID, groupID)].at(index)->getShortAddress();
                response->getPayload().add<uint16_t>(result);
                distributeFrame(response);
              }
              break;
            case FunctionGroupGetLastCalledScene:
              {
                int zoneID = pd.get<uint16_t>();
                int groupID = pd.get<uint16_t>();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_LastCalledSceneForZoneAndGroup[std::make_pair(zoneID, groupID)]);
                distributeFrame(response);
              }
              break;
            case FunctionZoneGetGroupIdForInd:
              {
                response = createResponse(cmdFrame, cmdNr);
                int zoneID = pd.get<uint16_t>();
                int groupIndex= pd.get<uint16_t>();

                IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInZone.begin();
                IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInZone.end();
                int result = -1;
                while(it != end) {
                  if(it->first.first == zoneID) {
                    if(groupIndex == 0) {
                      result = it->first.second; // yes, that's the group id
                      break;
                    }
                    groupIndex--;
                  }
                  it++;
                }
                response->getPayload().add<uint16_t>(result);
                distributeFrame(response);
              }
              break;
            case FunctionDeviceGetOnOff:
              {
                response = createResponse(cmdFrame, cmdNr);
                int devID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);
                response->getPayload().add<uint16_t>(dev.isTurnedOn());
                distributeFrame(response);
              }
              break;
            case FunctionDeviceGetDSID:
              {
                response = createResponse(cmdFrame, cmdNr);
                int devID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);
                response->getPayload().add<uint16_t>(1); // return-code (device found, all well)
                response->getPayload().add<dsid_t>(dev.getDSID());
                distributeFrame(response);
              }
              break;
            case FunctionGetTypeRequest:
              {
                response = createResponse(cmdFrame, cmdNr);
                // Type 2 bytes (high, low)
                response->getPayload().add<uint8_t>(0x00);
                response->getPayload().add<uint8_t>(0x01);
                // HW-Version 2 bytes (high, low)
                response->getPayload().add<uint8_t>(0x00);
                response->getPayload().add<uint8_t>(0x01);
                // SW-Version 2 bytes (high, low)
                response->getPayload().add<uint8_t>(0x00);
                response->getPayload().add<uint8_t>(0x01);
                // free text
                response->getPayload().add<uint8_t>('d');
                response->getPayload().add<uint8_t>('S');
                response->getPayload().add<uint8_t>('M');
                response->getPayload().add<uint8_t>('S');
                response->getPayload().add<uint8_t>('i');
                response->getPayload().add<uint8_t>('m');
                distributeFrame(response);
              }
              break;
            case FunctionDeviceGetGroups:
              {
                devid_t devID = pd.get<uint16_t>();
                lookupDevice(devID);
                std::vector<int>& groupsList = m_GroupsPerDevice[devID];


                // format: (8,7,6,5,4,3,2,1) (16,15,14,13,12,11,10,9), etc...
                unsigned char bytes[8];
                memset(bytes, '\0', sizeof(bytes));
                foreach(int groupID, groupsList) {
                  if(groupID != 0) {
                    int iBit = (groupID - 1) % 8;
                    int iByte = (groupID -1) / 8;
                    bytes[iByte] |= 1 << iBit;
                  }
                }

                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(1);
                for(int iByte = 0; iByte < 8; iByte++) {
                  response->getPayload().add<uint8_t>(bytes[iByte]);
                }
                distributeFrame(response);
              }
              break;
            case FunctionDeviceAddToGroup:
              {
                devid_t devID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);
                int groupID = pd.get<uint16_t>();
                addDeviceToGroup(&dev, groupID);
                response->getPayload().add<uint16_t>(1);
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterGetPowerConsumption:
              {
                response = createResponse(cmdFrame, cmdNr);
                uint32_t val = 0;
                foreach(DSIDInterface* interface, m_SimulatedDevices) {
                  val += interface->getConsumption();
                }
                response->getPayload().add<uint32_t>(val);
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterGetEnergyMeterValue:
              {
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint32_t>(0);
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterAddZone:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                response = createResponse(cmdFrame, cmdNr);
                bool isValid = true;
                for(std::map< const int, std::vector<DSIDInterface*> >::iterator iZoneEntry = m_Zones.begin(), e = m_Zones.end();
                    iZoneEntry != e; ++iZoneEntry)
                {
                  if(iZoneEntry->first == zoneID) {
                    response->getPayload().add<uint16_t>(static_cast<uint16_t>(-7));
                    isValid = false;
                  }
                }
                if(isValid) {
                  // make the zone visible in the map
                  m_Zones[zoneID].size();
                  response->getPayload().add<uint16_t>(1);
                }
                distributeFrame(response);
              }
              break;
            case FunctionDeviceSetZoneID:
              {
                devid_t devID = pd.get<devid_t>();
                uint16_t zoneID = pd.get<uint16_t>();
                DSIDInterface& dev = lookupDevice(devID);

                int oldZoneID = m_DeviceZoneMapping[&dev];
                std::vector<DSIDInterface*>::iterator oldEntry = find(m_Zones[oldZoneID].begin(), m_Zones[oldZoneID].end(), &dev);
                m_Zones[oldZoneID].erase(oldEntry);
                m_Zones[zoneID].push_back(&dev);
                m_DeviceZoneMapping[&dev] = zoneID;
                dev.setZoneID(zoneID);
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(1);
                distributeFrame(response);
              }
              break;
            case FunctionDSMeterGetEnergyLevel:
              {
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_EnergyLevelOrange);
                response->getPayload().add<uint16_t>(m_EnergyLevelRed);
                distributeFrame(response);
              }
              break;
            case FunctionDSLinkSendDevice:
              {
                devid_t devID = pd.get<devid_t>();
                DSIDInterface& dev = lookupDevice(devID);
                uint16_t valueToSend = pd.get<uint16_t>();
                uint16_t flags = pd.get<uint16_t>();    
                
                bool handled = false;
                uint8_t value = dev.dsLinkSend(valueToSend, flags, handled);
                if(handled && ((flags & DSLinkSendWriteOnly) == 0)) {
                  response = createReply(cmdFrame);
                  response->setCommand(CommandRequest);
                  response->getPayload().add<uint8_t>(FunctionDSLinkReceive);
                  response->getPayload().add<devid_t>(0x00); // garbage
                  response->getPayload().add<devid_t>(devID);
                  response->getPayload().add<uint16_t>(value);
                  distributeFrame(response);
                }
              }
              break;
            default:
              Logger::getInstance()->log("Invalid function id for sim: " + intToString(cmdNr), lsError);
          }
        }
      }
    } catch(std::runtime_error& e) {
      Logger::getInstance()->log(std::string("DSDSMeterSim: Exeption while processing packet. Message: '") + e.what() + "'");
    }
  } // process

  void DSDSMeterSim::distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) const {
    m_pSimulation->distributeFrame(_frame);
  } // distributeFrame

  void DSDSMeterSim::dSLinkInterrupt(devid_t _shortAddress) const {
    boost::shared_ptr<DS485CommandFrame> result(new DS485CommandFrame());
    result->getHeader().setDestination(0);
    result->getHeader().setSource(m_ID);
    result->getHeader().setBroadcast(true);
    result->getHeader().setCounter(0);
    result->setCommand(CommandEvent);
    result->getPayload().add<uint8_t>(EventDSLinkInterrupt);
    result->getPayload().add<devid_t>(_shortAddress);
    result->getPayload().add<uint16_t>(0); // priority
    distributeFrame(result);
  } // dSLinkInterrupt

  boost::shared_ptr<DS485CommandFrame> DSDSMeterSim::createReply(DS485CommandFrame& _request) const {
    boost::shared_ptr<DS485CommandFrame> result(new DS485CommandFrame());
    result->getHeader().setDestination(_request.getHeader().getSource());
    result->getHeader().setSource(m_ID);
    result->getHeader().setBroadcast(false);
    result->getHeader().setCounter(_request.getHeader().getCounter());
    return result;
  } // createReply

  boost::shared_ptr<DS485CommandFrame> DSDSMeterSim::createAck(DS485CommandFrame& _request, uint8_t _functionID) const {
    boost::shared_ptr<DS485CommandFrame> result = createReply(_request);
    result->setCommand(CommandAck);
    result->getPayload().add(_functionID);
    return result;
  } // createAck

  boost::shared_ptr<DS485CommandFrame> DSDSMeterSim::createResponse(DS485CommandFrame& _request, uint8_t _functionID) const {
    boost::shared_ptr<DS485CommandFrame> result = createReply(_request);
    result->setCommand(CommandResponse);
    result->getPayload().add(_functionID);
    return result;
  } // createResponse

  DSIDInterface& DSDSMeterSim::lookupDevice(const devid_t _shortAddress) {
    for(std::vector<DSIDInterface*>::iterator ipSimDev = m_SimulatedDevices.begin(); ipSimDev != m_SimulatedDevices.end(); ++ipSimDev) {
      if((*ipSimDev)->getShortAddress() == _shortAddress)  {
        return **ipSimDev;
      }
    }
    throw std::runtime_error(std::string("could not find device with id: ") + intToString(_shortAddress));
  } // lookupDevice

  int DSDSMeterSim::getID() const {
    return m_ID;
  } // getID

  DSIDInterface* DSDSMeterSim::getSimulatedDevice(const dsid_t _dsid) {
    for(std::vector<DSIDInterface*>::iterator iDSID = m_SimulatedDevices.begin(), e = m_SimulatedDevices.end();
        iDSID != e; ++iDSID)
    {
      if((*iDSID)->getDSID() == _dsid) {
        return *iDSID;
      }
    }
    return NULL;
  } // getSimulatedDevice


  //================================================== DSIDCreator

  DSIDCreator::DSIDCreator(const std::string& _identifier)
  : m_Identifier(_identifier)
  {} // ctor


  //================================================== DSIDFactory

  DSIDInterface* DSIDFactory::createDSID(const std::string& _identifier, const dsid_t _dsid, const devid_t _shortAddress, const DSDSMeterSim& _dsMeter) {
    boost::ptr_vector<DSIDCreator>::iterator
    iCreator = m_RegisteredCreators.begin(),
    e = m_RegisteredCreators.end();
    while(iCreator != e) {
      if(iCreator->getIdentifier() == _identifier) {
        return iCreator->createDSID(_dsid, _shortAddress, _dsMeter);
      }
      ++iCreator;
    }
    Logger::getInstance()->log(std::string("Could not find creator for DSID type '") + _identifier + "'");
    throw std::runtime_error(std::string("Could not find creator for DSID type '") + _identifier + "'");
  } // createDSID

  void DSIDFactory::registerCreator(DSIDCreator* _creator) {
    m_RegisteredCreators.push_back(_creator);
  } // registerCreator

}
