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

#include "apartment.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/ProcessingInstruction.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/SAX/InputSource.h>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::ProcessingInstruction;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::DOMParser;
using Poco::XML::XMLWriter;
using Poco::XML::InputSource;
using Poco::XML::Node;

#include "core/DS485Interface.h"
#include "core/ds485const.h"
#include "core/dss.h"
#include "core/logger.h"
#include "core/propertysystem.h"
#include "core/event.h"
#include "core/foreach.h"

#include "core/busrequestdispatcher.h"
#include "core/model/busrequest.h"

#include "core/model/busscanner.h"
#include "core/model/scenehelper.h"
#include "core/model/modelevent.h"

#include "core/model/set.h"
#include "core/model/device.h"
#include "core/model/set.h"
#include "core/model/zone.h"
#include "core/model/group.h"
#include "core/model/modulator.h"

namespace dss {


  //================================================== Apartment

  Apartment::Apartment(DSS* _pDSS, DS485Interface* _pDS485Interface)
  : Subsystem(_pDSS, "Apartment"),
    Thread("Apartment"),
    m_IsInitializing(true),
    m_pPropertyNode(),
    m_pDS485Interface(_pDS485Interface),
    m_pBusRequestDispatcher(NULL)
  { } // ctor

  Apartment::~Apartment() {
    scrubVector(m_Devices);
    scrubVector(m_Zones);
    scrubVector(m_Modulators);
  } // dtor

  void Apartment::initialize() {
    Subsystem::initialize();
    // create default zone
    Zone* zoneZero = new Zone(0);
    addDefaultGroupsToZone(*zoneZero);
    m_Zones.push_back(zoneZero);
    zoneZero->setIsPresent(true);
    if(DSS::hasInstance()) {
      m_pPropertyNode = DSS::getInstance()->getPropertySystem().createProperty("/apartment");
      DSS::getInstance()->getPropertySystem().setStringValue(getConfigPropertyBasePath() + "configfile", getDSS().getDataDirectory() + "apartment.xml", true, false);
    }
  } // initialize

  void Apartment::doStart() {
    run();
  } // start

  void Apartment::addDefaultGroupsToZone(Zone& _zone) {
    int zoneID = _zone.getID();

    Group* grp = new Group(GroupIDBroadcast, zoneID, *this);
    grp->setName("broadcast");
    _zone.addGroup(grp);
    grp = new Group(GroupIDYellow, zoneID, *this);
    grp->setName("yellow");
    _zone.addGroup(grp);
    grp = new Group(GroupIDGray, zoneID, *this);
    grp->setName("gray");
    _zone.addGroup(grp);
    grp = new Group(GroupIDBlue, zoneID, *this);
    grp->setName("blue");
    _zone.addGroup(grp);
    grp = new Group(GroupIDCyan, zoneID, *this);
    grp->setName("cyan");
    _zone.addGroup(grp);
    grp = new Group(GroupIDRed, zoneID, *this);
    grp->setName("red");
    _zone.addGroup(grp);
    grp = new Group(GroupIDViolet, zoneID, *this);
    grp->setName("magenta");
    _zone.addGroup(grp);
    grp = new Group(GroupIDGreen, zoneID, *this);
    grp->setName("green");
    _zone.addGroup(grp);
    grp = new Group(GroupIDBlack, zoneID, *this);
    grp->setName("black");
    _zone.addGroup(grp);
    grp = new Group(GroupIDWhite, zoneID, *this);
    grp->setName("white");
    _zone.addGroup(grp);
    grp = new Group(GroupIDDisplay, zoneID, *this);
    grp->setName("display");
    _zone.addGroup(grp);
  } // addDefaultGroupsToZone

  void Apartment::modulatorReady(int _modulatorBusID) {
    log("Modulator with id: " + intToString(_modulatorBusID) + " is ready", lsInfo);
    try {
      try {
        Modulator& mod = getModulatorByBusID(_modulatorBusID);
        BusScanner scanner(*m_pDS485Interface->getStructureQueryBusInterface(), *this);
        if(scanner.scanModulator(mod)) {
          boost::shared_ptr<Event> modulatorReadyEvent(new Event("modulator_ready"));
          modulatorReadyEvent->setProperty("modulator", mod.getDSID().toString());
          raiseEvent(modulatorReadyEvent);
        }
      } catch(DS485ApiError& e) {
        log(std::string("Exception caught while scanning modulator " + intToString(_modulatorBusID) + " : ") + e.what(), lsFatal);

        ModelEvent* pEvent = new ModelEvent(ModelEvent::etModulatorReady);
        pEvent->addParameter(_modulatorBusID);
        addModelEvent(pEvent);
      }
    } catch(ItemNotFoundException& e) {
      log("No modulator for bus-id (" + intToString(_modulatorBusID) + ") found, re-discovering devices");
      discoverDS485Devices();
    }
  } // modulatorReady

  void Apartment::setPowerConsumption(int _modulatorBusID, unsigned long _value) {
    getModulatorByBusID(_modulatorBusID).setPowerConsumption(_value);
  } // powerConsumption

  void Apartment::setEnergyMeterValue(int _modulatorBusID, unsigned long _value) {
    getModulatorByBusID(_modulatorBusID).setEnergyMeterValue(_value);
  } // energyMeterValue

  void Apartment::discoverDS485Devices() {
    // temporary mark all modulators as absent
    foreach(Modulator* pModulator, m_Modulators) {
      pModulator->setIsPresent(false);
    }

    // Request the dsid of all modulators
    DS485CommandFrame requestFrame;
    requestFrame.getHeader().setBroadcast(true);
    requestFrame.getHeader().setDestination(0);
    requestFrame.setCommand(CommandRequest);
    requestFrame.getPayload().add<uint8_t>(FunctionModulatorGetDSID);
    if(DSS::hasInstance()) {
      DSS::getInstance()->getDS485Interface().sendFrame(requestFrame);
    }
  } // discoverDS485Devices

  void Apartment::writeConfiguration() {
    if(DSS::hasInstance()) {
      writeConfigurationToXML(DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile"));
    }
  } // writeConfiguration

  void Apartment::handleModelEvents() {
    if(!m_ModelEvents.empty()) {
      ModelEvent& event = m_ModelEvents.front();
      switch(event.getEventType()) {
      case ModelEvent::etNewDevice:
        if(event.getParameterCount() != 4) {
          log("Expected exactly 4 parameter for ModelEvent::etNewDevice");
        } else {
          onAddDevice(event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3));
        }
        break;
      case ModelEvent::etCallSceneDevice:
        if(event.getParameterCount() != 3) {
          log("Expected exactly 3 parameter for ModelEvent::etCallSceneDevice");
        } else {
          onDeviceCallScene(event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etCallSceneGroup:
        if(event.getParameterCount() != 3) {
          log("Expected exactly 3 parameter for ModelEvent::etCallSceneGroup");
        } else {
          onGroupCallScene(event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etModelDirty:
        writeConfiguration();
        break;
      case ModelEvent::etDSLinkInterrupt:
        if(event.getParameterCount() != 3) {
          log("Expected exactly 3 parameter for ModelEvent::etDSLinkInterrupt");
        } else {
          onDSLinkInterrupt(event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etNewModulator:
        discoverDS485Devices();
        break;
      case ModelEvent::etLostModulator:
        discoverDS485Devices();
        break;
      case ModelEvent::etModulatorReady:
        if(event.getParameterCount() != 1) {
          log("Expected exactly 1 parameter for ModelEvent::etModulatorReady");
        } else {
          try{
            Modulator& mod = getModulatorByBusID(event.getParameter(0));
            mod.setIsPresent(true);
            mod.setIsValid(false);
          } catch(ItemNotFoundException& e) {
            log("dSM is ready, but it is not yet known, re-discovering devices");
            discoverDS485Devices();
          }
        }
        break;
      case ModelEvent::etBusReady:
        log("Got bus ready event.", lsInfo);
        discoverDS485Devices();
        break;
      case ModelEvent::etPowerConsumption:
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etPowerConsumption");
        } else {
          setPowerConsumption(event.getParameter(0), event.getParameter(1));
        }
        break;
      case ModelEvent::etEnergyMeterValue:
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etEnergyMeterValue");
        } else {
          setEnergyMeterValue(event.getParameter(0), event.getParameter(1));
        }
        break;
      case ModelEvent::etDS485DeviceDiscovered:
        if(event.getParameterCount() != 7) {
          log("Expected exactly 7 parameter for ModelEvent::etDS485DeviceDiscovered");
        } else {
          int busID = event.getParameter(0);
          uint64_t dsidUpper = (uint64_t(event.getParameter(1)) & 0x00ffff) << 48;
          dsidUpper |= (uint64_t(event.getParameter(2)) & 0x00ffff) << 32;
          dsidUpper |= (uint64_t(event.getParameter(3))  & 0x00ffff) << 16;
          dsidUpper |= (uint64_t(event.getParameter(4)) & 0x00ffff);
          dsid_t newDSID(dsidUpper,
                         ((uint32_t(event.getParameter(5)) & 0x00ffff) << 16) | (uint32_t(event.getParameter(6)) & 0x00ffff));
          log ("Discovered device with busID: " + intToString(busID) + " and dsid: " + newDSID.toString());
          try{
             getModulatorByDSID(newDSID).setBusID(busID);
             log ("dSM present");
             getModulatorByDSID(newDSID).setIsPresent(true);
          } catch(ItemNotFoundException& e) {
             log ("dSM not present");
             Modulator& modulator = allocateModulator(newDSID);
             modulator.setBusID(busID);
             modulator.setIsPresent(true);
             modulator.setIsValid(false);
             ModelEvent* pEvent = new ModelEvent(ModelEvent::etModulatorReady);
             pEvent->addParameter(busID);
             addModelEvent(pEvent);
          }
        }
        break;
      default:
        assert(false);
        break;
      }

      m_ModelEventsMutex.lock();
      m_ModelEvents.erase(m_ModelEvents.begin());
      m_ModelEventsMutex.unlock();
    } else {
      m_NewModelEvent.waitFor(1000);
      bool hadToUpdate = false;
      foreach(Modulator* pModulator, m_Modulators) {
        if(pModulator->isPresent()) {
          if(!pModulator->isValid()) {
            modulatorReady(pModulator->getBusID());
            hadToUpdate = true;
            break;
          }
        }
      }

      // If we didn't have to update for one cycle, assume that we're done
      if(!hadToUpdate && m_IsInitializing) {
        log("******** Finished loading model from dSM(s)...", lsInfo);
        m_IsInitializing = false;

        {
          boost::shared_ptr<Event> readyEvent(new Event("model_ready"));
          raiseEvent(readyEvent);
        }
      }
    }
  } // handleModelEvents

  void Apartment::readConfiguration() {
    if(DSS::hasInstance()) {
      std::string configFileName = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile");
      if(!boost::filesystem::exists(configFileName)) {
        log(std::string("Apartment::execute: Could not open config-file for apartment: '") + configFileName + "'", lsWarning);
      } else {
        readConfigurationFromXML(configFileName);
      }
    }
  } // readConfiguration

  void Apartment::raiseEvent(boost::shared_ptr<Event> _pEvent) {
    if(DSS::hasInstance()) {
      getDSS().getEventQueue().pushEvent(_pEvent);
    }
  } // raiseEvent

  void Apartment::waitForInterface() {
    if(DSS::hasInstance()) {
      DS485Interface& interface = DSS::getInstance()->getDS485Interface();

      log("Apartment::execute: Waiting for interface to get ready", lsInfo);

      while(!interface.isReady() && !m_Terminated) {
        sleepMS(1000);
      }
    }

    boost::shared_ptr<Event> readyEvent(new Event("interface_ready"));
    raiseEvent(readyEvent);
  } // waitForInterface

  void Apartment::execute() {
    {
      boost::shared_ptr<Event> runningEvent(new Event("running"));
      raiseEvent(runningEvent);
    }

    // load devices/modulators/etc. from a config-file
    readConfiguration();

    {
      boost::shared_ptr<Event> configReadEvent(new Event("config_read"));
      raiseEvent(configReadEvent);
    }

    waitForInterface();

    log("Apartment::execute: Interface is ready, enumerating model", lsInfo);
    discoverDS485Devices();

    while(!m_Terminated) {
      handleModelEvents();
    }
  } // run

  void Apartment::addModelEvent(ModelEvent* _pEvent) {
    // filter out dirty events, as this will rewrite apartment.xml
    if(m_IsInitializing && (_pEvent->getEventType() == ModelEvent::etModelDirty)) {
      delete _pEvent;
    } else {
      m_ModelEventsMutex.lock();
      m_ModelEvents.push_back(_pEvent);
      m_ModelEventsMutex.unlock();
      m_NewModelEvent.signal();
    }
  } // addModelEvent

  const int ApartmentConfigVersion = 1;

  void Apartment::readConfigurationFromXML(const std::string& _fileName) {
    setName("dSS");
    std::ifstream inFile(_fileName.c_str());

    InputSource input(inFile);
    DOMParser parser;
    AutoPtr<Document> pDoc = parser.parse(&input);
    Element* rootNode = pDoc->documentElement();

    if(rootNode->localName() == "config") {
      if(rootNode->hasAttribute("version") && (strToInt(rootNode->getAttribute("version")) == ApartmentConfigVersion)) {
        Node* curNode = rootNode->firstChild();
        while(curNode != NULL) {
          std::string nodeName = curNode->localName();
          if(nodeName == "devices") {
            loadDevices(curNode);
          } else if(nodeName == "modulators") {
            loadModulators(curNode);
          } else if(nodeName == "zones") {
            loadZones(curNode);
          } else if(nodeName == "apartment") {
            Element* elem = dynamic_cast<Element*>(curNode);
            if(elem != NULL) {
              Element* nameElem = elem->getChildElement("name");
              if(nameElem->hasChildNodes()) {
                setName(nameElem->firstChild()->nodeValue());
              }
            }
          }
          curNode = curNode->nextSibling();
        }
      } else {
        log("Config file has the wrong version");
      }
    }
  } // readConfigurationFromXML

  void Apartment::loadDevices(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "device") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("dsid")) {
          dsid_t dsid = dsid_t::fromString(elem->getAttribute("dsid"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }

          DateTime firstSeen;
          if(elem->hasAttribute("firstSeen")) {
            firstSeen = DateTime(dateFromISOString(elem->getAttribute("firstSeen").c_str()));
          }

          Device& newDevice = allocateDevice(dsid);
          if(!name.empty()) {
            newDevice.setName(name);
          }
          newDevice.setFirstSeen(firstSeen);
          Element* propertiesElem = elem->getChildElement("properties");
          if(propertiesElem != NULL) {
            newDevice.publishToPropertyTree();
            newDevice.getPropertyNode()->loadChildrenFromNode(propertiesElem);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadDevices

  void Apartment::loadModulators(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "modulator") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("id")) {
          dsid_t id = dsid_t::fromString(elem->getAttribute("id"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }
          Modulator& newModulator = allocateModulator(id);
          if(!name.empty()) {
            newModulator.setName(name);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadModulators

  void Apartment::loadZones(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "zone") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("id")) {
          int id = strToInt(elem->getAttribute("id"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }
          Zone& newZone = allocateZone(id);
          if(!name.empty()) {
            newZone.setName(name);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadZones

  void DeviceToXML(const Device* _pDevice, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pDeviceNode = _pDocument->createElement("device");
    pDeviceNode->setAttribute("dsid", _pDevice->getDSID().toString());
    if(!_pDevice->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pDevice->getName());
      pNameNode->appendChild(txtNode);
      pDeviceNode->appendChild(pNameNode);
    }
    pDeviceNode->setAttribute("firstSeen", _pDevice->getFirstSeen());
    if(_pDevice->getPropertyNode() != NULL) {
      AutoPtr<Element> pPropertiesNode = _pDocument->createElement("properties");
      pDeviceNode->appendChild(pPropertiesNode);
      _pDevice->getPropertyNode()->saveChildrenAsXML(_pDocument, pPropertiesNode, PropertyNode::Archive);
    }

    _parentNode->appendChild(pDeviceNode);
  } // deviceToXML

  void ZoneToXML(const Zone* _pZone, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pZoneNode = _pDocument->createElement("zone");
    pZoneNode->setAttribute("id", intToString(_pZone->getID()));
    if(!_pZone->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pZone->getName());
      pNameNode->appendChild(txtNode);
      pZoneNode->appendChild(pNameNode);
    }
    _parentNode->appendChild(pZoneNode);
  } // zoneToXML

  void ModulatorToXML(const Modulator* _pModulator, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pModulatorNode = _pDocument->createElement("modulator");
    pModulatorNode->setAttribute("id", _pModulator->getDSID().toString());
    if(!_pModulator->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pModulator->getName());
      pNameNode->appendChild(txtNode);
      pModulatorNode->appendChild(pNameNode);
    }
    _parentNode->appendChild(pModulatorNode);
  } // modulatorToXML

  void Apartment::writeConfigurationToXML(const std::string& _fileName) {
    log("Writing apartment config to '" + _fileName + "'", lsInfo);
    AutoPtr<Document> pDoc = new Document;

    AutoPtr<ProcessingInstruction> pXMLHeader = pDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    pDoc->appendChild(pXMLHeader);

    AutoPtr<Element> pRoot = pDoc->createElement("config");
    pRoot->setAttribute("version", intToString(ApartmentConfigVersion));
    pDoc->appendChild(pRoot);

    // apartment
    AutoPtr<Element> pApartment = pDoc->createElement("apartment");
    pRoot->appendChild(pApartment);
    AutoPtr<Element> pApartmentName = pDoc->createElement("name");
    AutoPtr<Text> pApartmentNameText = pDoc->createTextNode(getName());
    pApartmentName->appendChild(pApartmentNameText);
    pApartment->appendChild(pApartmentName);

    // devices
    AutoPtr<Element> pDevices = pDoc->createElement("devices");
    pRoot->appendChild(pDevices);
    foreach(Device* pDevice, m_Devices) {
      DeviceToXML(pDevice, pDevices, pDoc);
    }

    // zones
    AutoPtr<Element> pZones = pDoc->createElement("zones");
    pRoot->appendChild(pZones);
    foreach(Zone* pZone, m_Zones) {
      ZoneToXML(pZone, pZones, pDoc);
    }

    // modulators
    AutoPtr<Element> pModulators = pDoc->createElement("modulators");
    pRoot->appendChild(pModulators);
    foreach(Modulator* pModulator, m_Modulators) {
      ModulatorToXML(pModulator, pModulators, pDoc);
    }

    std::string tmpOut = _fileName + ".tmp";
    std::ofstream ofs(tmpOut.c_str());

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);
      writer.writeNode(ofs, pDoc);

      ofs.close();

      // move it to the desired location
      rename(tmpOut.c_str(), _fileName.c_str());
    } else {
      log("Could not open file '" + tmpOut + "' for writing", lsFatal);
    }
  } // writeConfigurationToXML

  Device& Apartment::getDeviceByDSID(const dsid_t _dsid) const {
    foreach(Device* dev, m_Devices) {
      if(dev->getDSID() == _dsid) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDeviceByShortAddress const

  Device& Apartment::getDeviceByDSID(const dsid_t _dsid) {
    foreach(Device* dev, m_Devices) {
      if(dev->getDSID() == _dsid) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDeviceByShortAddress

  Device& Apartment::getDeviceByShortAddress(const Modulator& _modulator, const devid_t _deviceID) const {
    foreach(Device* dev, m_Devices) {
      if((dev->getShortAddress() == _deviceID) &&
          (_modulator.getBusID() == dev->getModulatorID())) {
        return *dev;
      }
    }
    throw ItemNotFoundException(intToString(_deviceID));
  } // getDeviceByShortAddress

  Device& Apartment::getDeviceByName(const std::string& _name) {
    foreach(Device* dev, m_Devices) {
      if(dev->getName() == _name) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_name);
  } // getDeviceByName

  Set Apartment::getDevices() const {
    DeviceVector devs;
    foreach(Device* dev, m_Devices) {
      devs.push_back(DeviceReference(*dev, this));
    }

    return Set(devs);
  } // getDevices

  Zone& Apartment::getZone(const std::string& _zoneName) {
    foreach(Zone* zone, m_Zones) {
      if(zone->getName() == _zoneName) {
        return *zone;
      }
    }
    throw ItemNotFoundException(_zoneName);
  } // getZone(name)

  Zone& Apartment::getZone(const int _id) {
    foreach(Zone* zone, m_Zones) {
      if(zone->getID() == _id) {
        return *zone;
      }
    }
    throw ItemNotFoundException(intToString(_id));
  } // getZone(id)

  std::vector<Zone*>& Apartment::getZones() {
    return m_Zones;
  } // getZones

  Modulator& Apartment::getModulator(const std::string& _modName) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->getName() == _modName) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(_modName);
  } // getModulator(name)

  Modulator& Apartment::getModulatorByBusID(const int _busId) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->getBusID() == _busId) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(intToString(_busId));
  } // getModulatorByBusID

  Modulator& Apartment::getModulatorByDSID(const dsid_t _dsid) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->getDSID() == _dsid) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getModulatorByDSID

  std::vector<Modulator*>& Apartment::getModulators() {
    return m_Modulators;
  } // getModulators

  // Group queries
  Group& Apartment::getGroup(const std::string& _name) {
    Group* pResult = getZone(0).getGroup(_name);
    if(pResult != NULL) {
      return *pResult;
    }
    throw ItemNotFoundException(_name);
  } // getGroup(name)

  Group& Apartment::getGroup(const int _id) {
    Group* pResult = getZone(0).getGroup(_id);
    if(pResult != NULL) {
      return *pResult;
    }
    throw ItemNotFoundException(intToString(_id));
  } // getGroup(id)

  Device& Apartment::allocateDevice(const dsid_t _dsid) {
    // search for existing device
    foreach(Device* device, m_Devices) {
      if(device->getDSID() == _dsid) {
        DeviceReference devRef(*device, this);
        getZone(0).addDevice(devRef);
        return *device;
      }
    }

    Device* pResult = new Device(_dsid, this);
    pResult->setFirstSeen(DateTime());
    m_Devices.push_back(pResult);
    DeviceReference devRef(*pResult, this);
    getZone(0).addDevice(devRef);
    return *pResult;
  } // allocateDevice

  Modulator& Apartment::allocateModulator(const dsid_t _dsid) {
    foreach(Modulator* modulator, m_Modulators) {
      if((modulator)->getDSID() == _dsid) {
        return *modulator;
      }
    }

    Modulator* pResult = new Modulator(_dsid);
    m_Modulators.push_back(pResult);
    return *pResult;
  } // allocateModulator

  Zone& Apartment::allocateZone(int _zoneID) {
    if(getPropertyNode() != NULL) {
      getPropertyNode()->createProperty("zones/zone" + intToString(_zoneID));
    }

    foreach(Zone* zone, m_Zones) {
      if(zone->getID() == _zoneID) {
        return *zone;
      }
    }

    Zone* zone = new Zone(_zoneID);
    m_Zones.push_back(zone);
    addDefaultGroupsToZone(*zone);
    return *zone;
  } // allocateZone

  void Apartment::removeZone(int _zoneID) {
    for(std::vector<Zone*>::iterator ipZone = m_Zones.begin(), e = m_Zones.end();
        ipZone != e; ++ipZone) {
      Zone* pZone = *ipZone;
      if(pZone->getID() == _zoneID) {
        m_Zones.erase(ipZone);
        delete pZone;
        return;
      }
    }
  } // removeZone

  void Apartment::removeDevice(dsid_t _device) {
    for(std::vector<Device*>::iterator ipDevice = m_Devices.begin(), e = m_Devices.end();
        ipDevice != e; ++ipDevice) {
      Device* pDevice = *ipDevice;
      if(pDevice->getDSID() == _device) {
        m_Devices.erase(ipDevice);
        delete pDevice;
        return;
      }
    }
  } // removeDevice

  void Apartment::removeModulator(dsid_t _modulator) {
    for(std::vector<Modulator*>::iterator ipModulator = m_Modulators.begin(), e = m_Modulators.end();
        ipModulator != e; ++ipModulator) {
      Modulator* pModulator = *ipModulator;
      if(pModulator->getDSID() == _modulator) {
        m_Modulators.erase(ipModulator);
        delete pModulator;
        return;
      }
    }
  } // removeModulator

  class SetLastCalledSceneAction : public IDeviceAction {
  protected:
    int m_SceneID;
  public:
    SetLastCalledSceneAction(const int _sceneID)
    : m_SceneID(_sceneID) {}
    virtual ~SetLastCalledSceneAction() {}

    virtual bool perform(Device& _device) {
      _device.setLastCalledScene(m_SceneID);
      return true;
    }
  };

  void Apartment::onGroupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onGroupCallScene: Scene number is out of bounds. zoneID: " + intToString(_zoneID) + " groupID: " + intToString(_groupID) + " scene: " + intToString(_sceneID), lsError);
        return;
      }
      Zone& zone = getZone(_zoneID);
      Group* group = zone.getGroup(_groupID);
      if(group != NULL) {
        log("OnGroupCallScene: group-id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "' scene: " + intToString(_sceneID));
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          Set s = zone.getDevices().getByGroup(_groupID);
          SetLastCalledSceneAction act(_sceneID & 0x00ff);
          s.perform(act);

          std::vector<Zone*> zonesToUpdate;
          if(_zoneID == 0) {
            zonesToUpdate = m_Zones;
          } else {
            zonesToUpdate.push_back(&zone);
          }
          foreach(Zone* pZone, zonesToUpdate) {
            if(_groupID == 0) {
              foreach(Group* pGroup, pZone->getGroups()) {
                pGroup->setLastCalledScene(_sceneID & 0x00ff);
              }
            } else {
              Group* pGroup = pZone->getGroup(_groupID);
              if(pGroup != NULL) {
                pGroup->setLastCalledScene(_sceneID & 0x00ff);
              }
            }
          }
        }
      } else {
        log("OnGroupCallScene: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupCallScene: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }

  } // onGroupCallScene

  void Apartment::onDeviceCallScene(const int _modulatorID, const int _deviceID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onDeviceCallScene: _sceneID is out of bounds. modulator-id '" + intToString(_modulatorID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID), lsError);
        return;
      }
      Modulator& mod = getModulatorByBusID(_modulatorID);
      try {
        log("OnDeviceCallScene: modulator-id '" + intToString(_modulatorID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID));
        DeviceReference devRef = mod.getDevices().getByBusID(_deviceID);
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          devRef.getDevice().setLastCalledScene(_sceneID & 0x00ff);
        }
      } catch(ItemNotFoundException& e) {
        log("OnDeviceCallScene: Could not find device with bus-id '" + intToString(_deviceID) + "' on modulator '" + intToString(_modulatorID) + "' scene:" + intToString(_sceneID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnDeviceCallScene: Could not find modulator with bus-id '" + intToString(_modulatorID) + "'", lsError);
    }
  } // onDeviceCallScene

  void Apartment::onAddDevice(const int _modID, const int _zoneID, const int _devID, const int _functionID) {
    // get full dsid
    log("New Device found");
    log("  Modulator: " + intToString(_modID));
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));
    log("  FID:       " + intToString(_functionID));

    dsid_t dsid = getDSS().getDS485Interface().getStructureQueryBusInterface()->getDSIDOfDevice(_modID, _devID);
    Device& dev = allocateDevice(dsid);
    DeviceReference devRef(dev, this);

    log("  DSID:      " + dsid.toString());

    // remove from old modulator
    try {
      Modulator& oldModulator = getModulatorByBusID(dev.getModulatorID());
      oldModulator.removeDevice(devRef);
    } catch(std::runtime_error&) {
    }

    // remove from old zone
    if(dev.getZoneID() != 0) {
      try {
        Zone& oldZone = getZone(dev.getZoneID());
        oldZone.removeDevice(devRef);
        // TODO: check if the zone is empty on the modulator and remove it in that case
      } catch(std::runtime_error&) {
      }
    }

    // update device
    dev.setModulatorID(_modID);
    dev.setZoneID(_zoneID);
    dev.setShortAddress(_devID);
    dev.setFunctionID(_functionID);
    dev.setIsPresent(true);

    // add to new modulator
    Modulator& modulator = getModulatorByBusID(_modID);
    modulator.addDevice(devRef);

    // add to new zone
    Zone& newZone = allocateZone(_zoneID);
    newZone.addToModulator(modulator);
    newZone.addDevice(devRef);

    // get groups of device
    dev.resetGroups();
    std::vector<int> groups = m_pDS485Interface->getStructureQueryBusInterface()->getGroupsOfDevice(_modID, _devID);
    foreach(int iGroup, groups) {
      log("  Adding to Group: " + intToString(iGroup));
      dev.addToGroup(iGroup);
    }

    {
      boost::shared_ptr<Event> readyEvent(new Event("new_device"));
      readyEvent->setProperty("device", dsid.toString());
      readyEvent->setProperty("zone", intToString(_zoneID));
      raiseEvent(readyEvent);
    }
  } // onAddDevice

  void Apartment::onDSLinkInterrupt(const int _modID, const int _devID, const int _priority) {
    // get full dsid
    log("dSLinkInterrupt:");
    log("  Modulator: " + intToString(_modID));
    log("  DevID:     " + intToString(_devID));
    log("  Priority:  " + intToString(_priority));

    try {
      Modulator& modulator = getModulatorByBusID(_modID);
      try {
        Device& device = getDeviceByShortAddress(modulator, _devID);
        PropertyNodePtr deviceNode = device.getPropertyNode();
        if(deviceNode == NULL) {
          return;
        }
        PropertyNodePtr modeNode = deviceNode->getProperty("interrupt/mode");
        if(modeNode == NULL) {
          return;
        }
        std::string mode = modeNode->getStringValue();
        if(mode == "ignore") {
          log("ignoring interrupt");
        } else if(mode == "raise_event") {
          log("raising interrupt as event");
          std::string eventName = "dslink_interrupt";
          PropertyNodePtr eventNameNode = deviceNode->getProperty("interrupt/event/name");
          if(eventNameNode == NULL) {
            log("no node called 'interrupt/event' found, assuming name is 'dslink_interrupt'");
          } else {
            eventName = eventNameNode->getAsString();
          }

          // create event to be raised
          DeviceReference devRef(device, this);
          boost::shared_ptr<Event> evt(new Event(eventName, &devRef));
          evt->setProperty("device", device.getDSID().toString());
          std::string priorityString = "unknown";
          if(_priority == 0) {
            priorityString = "normal";
          } else if(_priority == 1) {
            priorityString = "high";
          }
          evt->setProperty("priority", priorityString);
          raiseEvent(evt);
        } else {
          log("unknown interrupt mode '" + mode + "'", lsError);
        }
      } catch (ItemNotFoundException& ex) {
        log("Apartment::onDSLinkInterrupt: Unknown device with ID " + intToString(_devID), lsFatal);
        return;
      }
    } catch(ItemNotFoundException& ex) {
      log("Apartment::onDSLinkInterrupt: Unknown Modulator with ID " + intToString(_modID), lsFatal);
      return;
    }
  } // onDSLinkInterrupt

  void Apartment::dispatchRequest(boost::shared_ptr<BusRequest> _pRequest) {
    if(m_pBusRequestDispatcher != NULL) {
      m_pBusRequestDispatcher->dispatchRequest(_pRequest);
    } else {
      throw std::runtime_error("Apartment::dispatchRequest: m_pBusRequestDispatcher is NULL");
    }
  } // dispatchRequest

  DeviceBusInterface* Apartment::getDeviceBusInterface() {
    return m_pDS485Interface->getDeviceBusInterface();
  } // getDeviceBusInterface


} // namespace dss
