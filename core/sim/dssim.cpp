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

#include "dssim.h"

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


namespace fs = boost::filesystem;

namespace dss {

  //================================================== DSIDSimCreator

  class DSIDSimCreator : public DSIDCreator {
  public:
    DSIDSimCreator()
    : DSIDCreator("standard.simple")
    {}

    virtual ~DSIDSimCreator() {};

    virtual DSIDInterface* createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
      return new DSIDSim(_modulator, _dsid, _shortAddress);
    }
  };

  //================================================== DSIDSimCreator

  class DSIDSimSwitchCreator : public DSIDCreator {
  public:
    DSIDSimSwitchCreator()
    : DSIDCreator("standard.switch")
    {}

    virtual ~DSIDSimSwitchCreator() {};

    virtual DSIDInterface* createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
      return new DSIDSimSwitch(_modulator, _dsid, _shortAddress, 9);
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
            if(!fileExists(scriptFile)) {
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

    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "configfile", getDSS().getDataDirectory() + "sim.xml", true, false);

    loadPlugins();
    loadFromConfig();

    m_Initialized = true;
  } // initialize

  void DSSim::loadFromConfig() {
    const int theConfigFileVersion = 1;
    std::string filename = getConfigPropertyBasePath() + "configfile";
    XMLDocumentFileReader reader(getDSS().getPropertySystem().getStringValue(filename));
    XMLNode rootNode = reader.getDocument().getRootNode();

    if(rootNode.getName() == "simulation") {
      std::string fileVersionStr = rootNode.getAttributes()["version"];
      if(strToIntDef(fileVersionStr, -1) == theConfigFileVersion) {
        XMLNodeList& modulators = rootNode.getChildren();
        foreach(XMLNode& node, modulators) {
          if(node.getName() == "modulator") {
            DSModulatorSim* modulator = NULL;
            try {
              modulator = new DSModulatorSim(this);
              modulator->initializeFromNode(node);
              m_Modulators.push_back(modulator);
            } catch(runtime_error&) {
              delete modulator;
            }
          }
        }
      } else {
        log("Version mismatch, or missing version attribute. Expected ''" + intToString(theConfigFileVersion) + "'" + fileVersionStr + "' ");
      }
    } else {
      log(filename + " must have a root-node named 'simulation'", lsFatal);
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
          log("LoadPlugins: Cought exception while loading " + dir_itr->leaf() + " '" + ex.what() + "'", lsError);
        }
      }
    } catch(const std::exception& ex) {
      log(string("Error loading plugins: '") + ex.what() + "'");
    }
  } // loadPlugins

  bool DSSim::isReady() {
    return m_Initialized;
  } // ready

  bool DSSim::isSimAddress(const uint8_t _address) {
    foreach(DSModulatorSim& modulator, m_Modulators) {
      if(modulator.getID() == _address) {
        return true;
      }
    }
    return false;
  } // isSimAddress

  void DSSim::process(DS485Frame& _frame) {
    foreach(DSModulatorSim& modulator, m_Modulators) {
      modulator.process(_frame);
    }
  } // process

  void DSSim::distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    DS485FrameProvider::distributeFrame(_frame);
  } // distributeFrame

  DSIDInterface* DSSim::getSimulatedDevice(const dsid_t& _dsid) {
    DSIDInterface* result = NULL;
    foreach(DSModulatorSim& modulator, m_Modulators) {
      result = modulator.getSimulatedDevice(_dsid);
      if(result != NULL) {
        break;
      }
    }
    return result;
  } // getSimulatedDevice


  //================================================== DSModulatorSim

  DSModulatorSim::DSModulatorSim(DSSim* _pSimulation)
  : m_pSimulation(_pSimulation),
    m_EnergyLevelOrange(200),
    m_EnergyLevelRed(400)
  {
    m_ModulatorDSID = dsid_t(DSIDHeader, SimulationPrefix);
    m_ID = 70;
    m_Name = "Simulated dSM";
  } // dSModulatorSim

  void DSModulatorSim::log(const std::string& _message, aLogSeverity _severity) {
    m_pSimulation->log(_message, _severity);
  } // log

  bool DSModulatorSim::initializeFromNode(XMLNode& _node) {
    HashMapConstStringString& attrs = _node.getAttributes();
    if(attrs["busid"].size() != 0) {
      m_ID = strToIntDef(attrs["busid"], 70);
    }
    if(attrs["dsid"].size() != 0) {
      m_ModulatorDSID = dsid_t::fromString(attrs["dsid"]);
      m_ModulatorDSID.upper = (m_ModulatorDSID.upper & 0x000000000000000Fll) | DSIDHeader;
      m_ModulatorDSID.lower = (m_ModulatorDSID.lower & 0x002FFFFF) | SimulationPrefix;
    }
    m_EnergyLevelOrange = strToIntDef(attrs["orange"], m_EnergyLevelOrange);
    m_EnergyLevelRed = strToIntDef(attrs["red"], m_EnergyLevelRed);
    try {
      XMLNode& nameNode = _node.getChildByName("name");
      if(!nameNode.getChildren().empty()) {
        m_Name = nameNode.getChildren()[0].getContent();
      }
    } catch(XMLException&) {
    }

    XMLNodeList& nodes = _node.getChildren();
    loadDevices(nodes, 0);
    loadGroups(nodes, 0);
    loadZones(nodes);
    return true;
  } // initializeFromNode

  void DSModulatorSim::loadDevices(XMLNodeList& _nodes, const int _zoneID) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->getName() == "device") {
        HashMapConstStringString& attrs = iNode->getAttributes();
        dsid_t dsid = NullDSID;
        int busid = -1;
        if(!attrs["dsid"].empty()) {
          dsid = dsid_t::fromString(attrs["dsid"]);
          dsid.upper = (dsid.upper & 0x000000000000000Fll) | DSIDHeader;
          dsid.lower = (dsid.lower & 0x002FFFFF) | SimulationPrefix;
        }
        if(!attrs["busid"].empty()) {
          busid = strToInt(attrs["busid"]);
        }
        if((dsid == NullDSID) || (busid == -1)) {
          log("missing dsid or busid of device");
          continue;
        }
        std::string type = "standard.simple";
        if(!attrs["type"].empty()) {
          type = attrs["type"];
        }

        DSIDInterface* newDSID = m_pSimulation->getDSIDFactory().createDSID(type, dsid, busid, *this);
        try {
          foreach(XMLNode& iParam, iNode->getChildren()) {
            if(iParam.getName() == "parameter") {
              std::string paramName = iParam.getAttributes()["name"];
              std::string paramValue = iParam.getChildren()[0].getContent();
              if(paramName.empty()) {
                log("Missing attribute name of parameter node");
                continue;
              }
              log("LoadDevices:   Found parameter '" + paramName + "' with value '" + paramValue + "'");
              newDSID->setConfigParameter(paramName, paramValue);
            } else if(iParam.getName() == "name") {
              m_DeviceNames[busid] = iParam.getChildren()[0].getContent();
            }
          }
        } catch(runtime_error&) {
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
            if(attrs["bell"] == "true") {
              sw->setIsBell(true);
              log("LoadDevices:   switch is bell");
            }
          }

          newDSID->initialize();
          log("LoadDevices: found device");
        } else {
          log("LoadDevices: could not create instance for type \"" + type + "\"");
        }
      }
    }
  } // loadDevices

  void DSModulatorSim::loadGroups(XMLNodeList& _nodes, const int _zoneID) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->getName() == "group") {
        HashMapConstStringString& attrs = iNode->getAttributes();
        if(!attrs["id"].empty()) {
          int groupID = strToIntDef(attrs["id"], -1);
          XMLNodeList& children = iNode->getChildren();
          for(XMLNodeList::iterator iChildNode = children.begin(), e = children.end();
              iChildNode != e; ++iChildNode)
          {
            if(iChildNode->getName() == "device") {
              attrs = iChildNode->getAttributes();
              if(!attrs["busid"].empty()) {
                unsigned long busID = strToUInt(attrs["busid"]);
                DSIDInterface& dev = lookupDevice(busID);
                addDeviceToGroup(&dev, groupID);
                log("LoadGroups: Adding device " + attrs["busid"] + " to group " + intToString(groupID) + " in zone " + intToString(_zoneID));
              }
            }
          }
        } else {
          log("LoadGroups: Could not find attribute id of group, skipping entry", lsError);
        }
      }
    }
  } // loadGroups

  void DSModulatorSim::addDeviceToGroup(DSIDInterface* _device, int _groupID) {
    m_DevicesOfGroupInZone[pair<const int, const int>(_device->getZoneID(), _groupID)].push_back(_device);
    m_GroupsPerDevice[_device->getShortAddress()].push_back(_groupID);
  } // addDeviceToGroup

  void DSModulatorSim::loadZones(XMLNodeList& _nodes) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->getName() == "zone") {
        HashMapConstStringString& attrs = iNode->getAttributes();
        int zoneID = -1;
        if(!attrs["id"].empty()) {
          zoneID = strToIntDef(attrs["id"], -1);
        }
        if(zoneID != -1) {
          log("LoadZones: found zone (" + intToString(zoneID) + ")");
          loadDevices(iNode->getChildren(), zoneID);
          loadGroups(iNode->getChildren(), zoneID);
        } else {
          log("LoadZones: could not find/parse id for zone");
        }
      }
    }
  } // loadZones

  void DSModulatorSim::deviceCallScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).callScene(_sceneID);
  } // deviceCallScene

  void DSModulatorSim::groupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
	vector<DSIDInterface*> dsids;
	m_LastCalledSceneForZoneAndGroup[make_pair(_zoneID, _groupID)] = _sceneID;
	if(_groupID == GroupIDBroadcast) {
	  if(_zoneID == 0) {
	    dsids = m_SimulatedDevices;
	  } else {
      dsids = m_Zones[_zoneID];
	  }
	} else {
      pair<const int, const int> zonesGroup(_zoneID, _groupID);
      if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
        dsids = m_DevicesOfGroupInZone[zonesGroup];
      }
	}
    for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
        iDSID != e; ++iDSID)
    {
      (*iDSID)->callScene(_sceneID);
    }
  } // groupCallScene

  void DSModulatorSim::deviceSaveScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).saveScene(_sceneID);
  } // deviceSaveScene

  void DSModulatorSim::groupSaveScene(const int _zoneID, const int _groupID, const int _sceneID) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->saveScene(_sceneID);
      }
    }
  } // groupSaveScene

  void DSModulatorSim::deviceUndoScene(int _deviceID, const int _sceneID) {
    lookupDevice(_deviceID).undoScene(_sceneID);
  } // deviceUndoScene

  void DSModulatorSim::groupUndoScene(const int _zoneID, const int _groupID, const int _sceneID) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->undoScene(_sceneID);
      }
    }
  } // groupUndoScene

  void DSModulatorSim::groupStartDim(const int _zoneID, const int _groupID, bool _up, int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->startDim(_up, _parameterNr);
      }
    }
  } // groupStartDim

  void DSModulatorSim::groupEndDim(const int _zoneID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->endDim(_parameterNr);
      }
    }
  } // groupEndDim

  void DSModulatorSim::groupDecValue(const int _zoneID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->decreaseValue(_parameterNr);
      }
    }
  } // groupDecValue

  void DSModulatorSim::groupIncValue(const int _zoneID, const int _groupID, const int _parameterNr) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->increaseValue(_parameterNr);
      }
    }
  } // groupIncValue

  void DSModulatorSim::groupSetValue(const int _zoneID, const int _groupID, const int _value) {
    pair<const int, const int> zonesGroup(_zoneID, _groupID);
    if(m_DevicesOfGroupInZone.find(zonesGroup) != m_DevicesOfGroupInZone.end()) {
      std::vector<DSIDInterface*> dsids = m_DevicesOfGroupInZone[zonesGroup];
      for(vector<DSIDInterface*>::iterator iDSID = dsids.begin(), e = dsids.end();
          iDSID != e; ++iDSID)
      {
        (*iDSID)->setValue(_value);
      }
    }
  } // groupSetValue

  void DSModulatorSim::process(DS485Frame& _frame) {
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
            case FunctionDeviceGetName:
              {
                devid_t devID = pd.get<devid_t>();
                std::string name = m_DeviceNames[devID];
                response = createResponse(cmdFrame, cmdNr);

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
            case FunctionModulatorGetZonesSize:
              {
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_Zones.size());
                distributeFrame(response);
              }
              break;
            case FunctionModulatorGetZoneIdForInd:
              {
                uint8_t index = pd.get<uint16_t>();
                map< const int, std::vector<DSIDInterface*> >::iterator it = m_Zones.begin();
                advance(it, index);
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(it->first);
                distributeFrame(response);
              }
              break;
            case FunctionModulatorCountDevInZone:
              {
                uint16_t index = pd.get<uint16_t>();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_Zones[index].size());
                distributeFrame(response);
              }
              break;
            case FunctionModulatorDevKeyInZone:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                uint16_t deviceIndex = pd.get<devid_t>();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_Zones[zoneID].at(deviceIndex)->getShortAddress());
                distributeFrame(response);
              }
              break;
            case FunctionModulatorGetGroupsSize:
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
            case FunctionModulatorGetDSID:
              {
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<dsid_t>(m_ModulatorDSID);
                distributeFrame(response);
              }
              break;
            case FunctionGroupGetDeviceCount:
              {
                response = createResponse(cmdFrame, cmdNr);
                int zoneID = pd.get<uint16_t>();
                int groupID = pd.get<uint16_t>();
                int result = m_DevicesOfGroupInZone[pair<const int, const int>(zoneID, groupID)].size();
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
                int result = m_DevicesOfGroupInZone[pair<const int, const int>(zoneID, groupID)].at(index)->getShortAddress();
                response->getPayload().add<uint16_t>(result);
                distributeFrame(response);
              }
              break;
            case FunctionGroupGetLastCalledScene:
              {
                int zoneID = pd.get<uint16_t>();
                int groupID = pd.get<uint16_t>();
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(m_LastCalledSceneForZoneAndGroup[make_pair(zoneID, groupID)]);
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
            case FunctionModulatorGetPowerConsumption:
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
            case FunctionModulatorGetEnergyMeterValue:
              {
                response = createResponse(cmdFrame, cmdNr);
                response->getPayload().add<uint16_t>(0);
                distributeFrame(response);
              }
              break;
            case FunctionModulatorAddZone:
              {
                uint16_t zoneID = pd.get<uint16_t>();
                response = createResponse(cmdFrame, cmdNr);
                bool isValid = true;
                for(map< const int, std::vector<DSIDInterface*> >::iterator iZoneEntry = m_Zones.begin(), e = m_Zones.end();
                    iZoneEntry != e; ++iZoneEntry)
                {
                  if(iZoneEntry->first == zoneID) {
                    response->getPayload().add<uint16_t>(static_cast<uint16_t>(-7));
                    isValid = false;
                    break;
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
            case FunctionModulatorGetEnergyBorder:
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
                
                uint8_t value = dev.dsLinkSend(valueToSend, flags);
                if((flags & DSLinkSendWriteOnly) == 0) {
                  response = createReply(cmdFrame);
                  response->setCommand(CommandRequest);
                  response->getPayload().add<uint8_t>(FunctionDSLinkReceive);
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
    } catch(runtime_error& e) {
      Logger::getInstance()->log(string("DSModulatorSim: Exeption while processing packet. Message: '") + e.what() + "'");
    }
  } // process

  void DSModulatorSim::distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) const {
    m_pSimulation->distributeFrame(_frame);
  } // distributeFrame

  void DSModulatorSim::dSLinkInterrupt(devid_t _shortAddress) const {
    boost::shared_ptr<DS485CommandFrame> result(new DS485CommandFrame());
    result->getHeader().setDestination(0);
    result->getHeader().setSource(m_ID);
    result->getHeader().setBroadcast(true);
    result->getHeader().setCounter(0);
    result->setCommand(CommandEvent);
    result->getPayload().add<uint8_t>(FunctionDSLinkInterrupt);
    result->getPayload().add<devid_t>(_shortAddress);
    distributeFrame(result);
  } // dSLinkInterrupt

  boost::shared_ptr<DS485CommandFrame> DSModulatorSim::createReply(DS485CommandFrame& _request) const {
    boost::shared_ptr<DS485CommandFrame> result(new DS485CommandFrame());
    result->getHeader().setDestination(_request.getHeader().getSource());
    result->getHeader().setSource(m_ID);
    result->getHeader().setBroadcast(false);
    result->getHeader().setCounter(_request.getHeader().getCounter());
    return result;
  } // createReply

  boost::shared_ptr<DS485CommandFrame> DSModulatorSim::createAck(DS485CommandFrame& _request, uint8_t _functionID) const {
    boost::shared_ptr<DS485CommandFrame> result = createReply(_request);
    result->setCommand(CommandAck);
    result->getPayload().add(_functionID);
    return result;
  } // createAck

  boost::shared_ptr<DS485CommandFrame> DSModulatorSim::createResponse(DS485CommandFrame& _request, uint8_t _functionID) const {
    boost::shared_ptr<DS485CommandFrame> result = createReply(_request);
    result->setCommand(CommandResponse);
    result->getPayload().add(_functionID);
    return result;
  } // createResponse

  DSIDInterface& DSModulatorSim::lookupDevice(const devid_t _shortAddress) {
    for(vector<DSIDInterface*>::iterator ipSimDev = m_SimulatedDevices.begin(); ipSimDev != m_SimulatedDevices.end(); ++ipSimDev) {
      if((*ipSimDev)->getShortAddress() == _shortAddress)  {
        return **ipSimDev;
      }
    }
    throw runtime_error(string("could not find device with id: ") + intToString(_shortAddress));
  } // lookupDevice

  int DSModulatorSim::getID() const {
    return m_ID;
  } // getID

  DSIDInterface* DSModulatorSim::getSimulatedDevice(const dsid_t _dsid) {
    for(vector<DSIDInterface*>::iterator iDSID = m_SimulatedDevices.begin(), e = m_SimulatedDevices.end();
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

  DSIDInterface* DSIDFactory::createDSID(const std::string& _identifier, const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
    boost::ptr_vector<DSIDCreator>::iterator
    iCreator = m_RegisteredCreators.begin(),
    e = m_RegisteredCreators.end();
    while(iCreator != e) {
      if(iCreator->getIdentifier() == _identifier) {
        return iCreator->createDSID(_dsid, _shortAddress, _modulator);
      }
      ++iCreator;
    }
    Logger::getInstance()->log(string("Could not find creator for DSID type '") + _identifier + "'");
    throw new runtime_error(string("Could not find creator for DSID type '") + _identifier + "'");
  } // createDSID

  void DSIDFactory::registerCreator(DSIDCreator* _creator) {
    m_RegisteredCreators.push_back(_creator);
  } // registerCreator

}
