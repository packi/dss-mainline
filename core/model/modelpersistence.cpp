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

#include "modelpersistence.h"

#include <stdexcept>

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
#include <Poco/SAX/SAXException.h>

#include "core/foreach.h"
#include "core/base.h"
#include "core/propertysystem.h"
#include "core/ds485types.h"
#include "core/model/apartment.h"
#include "core/model/device.h"
#include "core/model/zone.h"
#include "core/model/modulator.h"
#include "core/model/group.h"
#include "core/model/modelconst.h"

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

namespace dss {

  ModelPersistence::ModelPersistence(Apartment& _apartment)
  : m_Apartment(_apartment)
  { } // ctor

  ModelPersistence::~ModelPersistence() {
  } // dtor

  const int ApartmentConfigVersion = 1;

  void ModelPersistence::readConfigurationFromXML(const std::string& _fileName) {
    m_Apartment.setName("dSS");
    std::ifstream inFile(_fileName.c_str());

    if (!inFile.is_open()) {
      Logger::getInstance()->log("Failed to open " + _fileName, lsInfo);
      return;
    }

    InputSource input(inFile);
    DOMParser parser;
    AutoPtr<Document> pDoc;
    try {
      pDoc = parser.parse(&input);
    } catch (Poco::XML::SAXParseException& spe) {
      // Note that we can hit this case both if it's invalid XML and
      // when the file doesn't exist at all
      throw std::runtime_error("ModelPersistence::readConfigurationFromXML: "
                               "Parse error in Model configuration: " +
                               _fileName + ": " + spe.message());
    }

    Element* rootNode = pDoc->documentElement();

    if(rootNode->localName() == "config") {
      if(rootNode->hasAttribute("version") && (strToInt(rootNode->getAttribute("version")) == ApartmentConfigVersion)) {
        Node* curNode = rootNode->firstChild();
        while(curNode != NULL) {
          std::string nodeName = curNode->localName();
          if(nodeName == "devices") {
            loadDevices(curNode);
          } else if((nodeName == "modulators") || (nodeName == "dsMeters")) {
            loadDSMeters(curNode);
          } else if(nodeName == "zones") {
            loadZones(curNode);
          } else if(nodeName == "apartment") {
            Element* elem = dynamic_cast<Element*>(curNode);
            if(elem != NULL) {
              Element* nameElem = elem->getChildElement("name");
              if(nameElem->hasChildNodes()) {
                m_Apartment.setName(nameElem->firstChild()->nodeValue());
              }
            }
          }
          curNode = curNode->nextSibling();
        }
      } else {
        Logger::getInstance()->log("Config file has the wrong version");
      }
    }
  } // readConfigurationFromXML

  void ModelPersistence::loadDevices(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "device") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("dsid")) {
          dss_dsid_t dsid = dss_dsid_t::fromString(elem->getAttribute("dsid"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }

          DateTime firstSeen;
          if(elem->hasAttribute("firstSeen")) {
            try {
              time_t timestamp = strToUInt(elem->getAttribute("firstSeen"));
              firstSeen = DateTime(timestamp);
            } catch(std::invalid_argument&) {
              firstSeen = DateTime(dateFromISOString(elem->getAttribute("firstSeen").c_str()));
            }
          }

          dss_dsid_t lastKnownDsMeter = NullDSID;
          if(elem->hasAttribute("lastKnownDSMeter")) {
            lastKnownDsMeter = dss_dsid_t::fromString(elem->getAttribute("lastKnownDSMeter"));
          }

          int lastKnownZoneID = 0;
          if(elem->hasAttribute("lastKnownZoneID")) {
            lastKnownZoneID = strToIntDef(elem->getAttribute("lastKnownZoneID"), 0);
          }

          devid_t lastKnownShortAddress = ShortAddressStaleDevice;
          if(elem->hasAttribute("lastKnownShortAddress")) {
            lastKnownShortAddress = strToUIntDef(elem->getAttribute("lastKnownShortAddress"),
                                                 ShortAddressStaleDevice);
          }

          boost::shared_ptr<Device> newDevice = m_Apartment.allocateDevice(dsid);
          if(!name.empty()) {
            newDevice->setName(name);
          }
          newDevice->setFirstSeen(firstSeen);
          newDevice->setLastKnownDSMeterDSID(lastKnownDsMeter);
          newDevice->setLastKnownZoneID(lastKnownZoneID);
          if(lastKnownZoneID != 0) {
            DeviceReference devRef(newDevice, &m_Apartment);
            m_Apartment.allocateZone(lastKnownZoneID)->addDevice(devRef);
          }
          newDevice->setLastKnownShortAddress(lastKnownShortAddress);
          Element* propertiesElem = elem->getChildElement("properties");
          if(propertiesElem != NULL) {
            newDevice->getPropertyNode()->loadChildrenFromNode(propertiesElem);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadDevices

  void ModelPersistence::loadDSMeters(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if((curNode->localName() == "modulator") || (curNode->localName() == "dsMeter")) {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("id")) {
          dss_dsid_t id = dss_dsid_t::fromString(elem->getAttribute("id"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }
          boost::shared_ptr<DSMeter> newDSMeter = m_Apartment.allocateDSMeter(id);
          if(!name.empty()) {
            newDSMeter->setName(name);
          }
          Node* hashNode = elem->getChildElement("datamodelHash");
          if (hashNode != NULL && hashNode->hasChildNodes()) {
            newDSMeter->setDatamodelHash(strToInt(hashNode->firstChild()->nodeValue()));
          }
          Node* modificationNode = elem->getChildElement("datamodelModification");
          if (modificationNode != NULL && modificationNode->hasChildNodes()) {
            newDSMeter->setDatamodelModificationcount(strToInt(modificationNode->firstChild()->nodeValue()));
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadDSMeters

  void ModelPersistence::loadZone(Poco::XML::Node* _node) {
    Element* elem = dynamic_cast<Element*>(_node);
    if((elem != NULL) && elem->hasAttribute("id")) {
      int id = strToInt(elem->getAttribute("id"));
      std::string name;
      Element* nameElem = elem->getChildElement("name");
      if((nameElem != NULL) && nameElem->hasChildNodes()) {
        name = nameElem->firstChild()->nodeValue();
      }
      boost::shared_ptr<Zone> newZone = m_Apartment.allocateZone(id);
      if(!name.empty()) {
        newZone->setName(name);
      }
      Node* groupsNode = elem->getChildElement("groups");
      if(groupsNode != NULL) {
        loadGroups(groupsNode, newZone);
      }
    }
  } // loadZone

  void ModelPersistence::loadGroups(Node* _node, boost::shared_ptr<Zone> _pZone) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "group") {
        loadGroup(curNode, _pZone);
      }
      curNode = curNode->nextSibling();
    }
  } // loadGroups

  void ModelPersistence::loadGroup(Poco::XML::Node* _node, boost::shared_ptr<Zone> _pZone) {
    Element* elem = dynamic_cast<Element*>(_node);
    if(elem != NULL && elem->hasAttribute("id")) {
      int groupID = strToIntDef(elem->getAttribute("id"), -1);
      Node* curNode = _node->firstChild();
      Node* scenesNode = NULL;
      std::string name;
      std::string associatedSet;
      while(curNode != NULL) {
        if(curNode->hasChildNodes()) {
          if(curNode->localName() == "name") {
            name = curNode->firstChild()->nodeValue();
          } else if(curNode->localName() == "scenes") {
            scenesNode = curNode;
          } else if(curNode->localName() == "associatedSet") {
            associatedSet = curNode->firstChild()->nodeValue();
          }
        }
        curNode = curNode->nextSibling();
      }
      if(groupID != -1) {
        boost::shared_ptr<Group> pGroup = _pZone->getGroup(groupID);
        if(pGroup == NULL) {
          pGroup.reset(new Group(groupID, _pZone, m_Apartment));
          _pZone->addGroup(pGroup);
        }
        if(!name.empty()) {
          pGroup->setName(name);
        }
        if(scenesNode != NULL) {
          loadScenes(scenesNode, pGroup);
        }
        if(!associatedSet.empty()) {
          pGroup->setAssociatedSet(associatedSet);
        }
        bool readSuccessfully = !name.empty() && scenesNode && scenesNode->hasChildNodes();
        pGroup->setIsInitializedFromBus(readSuccessfully);
      }
    }
  } // loadGroup

  void ModelPersistence::loadScenes(Poco::XML::Node* _node, boost::shared_ptr<dss::Group> _pGroup) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "scene") {
        loadScene(curNode, _pGroup);
      }
      curNode = curNode->nextSibling();
    }
  } // loadScenes

  void ModelPersistence::loadScene(Poco::XML::Node* _node, boost::shared_ptr<dss::Group> _pGroup) {
    Element* elem = dynamic_cast<Element*>(_node);
    if(elem != NULL && elem->hasAttribute("id")) {
      std::string name;
      int sceneNumber = strToIntDef(elem->getAttribute("id"), -1);
      Node* curNode = _node->firstChild();
      while(curNode != NULL) {
        if(curNode->hasChildNodes()) {
          if(curNode->localName() == "name") {
            name = curNode->firstChild()->nodeValue();
          }
        }
        curNode = curNode->nextSibling();
      }
      if((sceneNumber != -1) && !name.empty()) {
        _pGroup->setSceneName(sceneNumber, name);
      }
    }
  } // loadScene

  void ModelPersistence::loadZones(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "zone") {
        loadZone(curNode);
      }
      curNode = curNode->nextSibling();
    }
  } // loadZones

  void deviceToXML(boost::shared_ptr<const Device> _pDevice, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pDeviceNode = _pDocument->createElement("device");
    pDeviceNode->setAttribute("dsid", _pDevice->getDSID().toString());
    if(!_pDevice->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pDevice->getName());
      pNameNode->appendChild(txtNode);
      pDeviceNode->appendChild(pNameNode);
    }
    pDeviceNode->setAttribute("firstSeen", intToString(_pDevice->getFirstSeen().secondsSinceEpoch()));
    if(_pDevice->getLastKnownDSMeterDSID() != NullDSID) {
      pDeviceNode->setAttribute("lastKnownDSMeter", _pDevice->getLastKnownDSMeterDSID().toString());
    }
    if(_pDevice->getLastKnownZoneID() != 0) {
      pDeviceNode->setAttribute("lastKnownZoneID", intToString(_pDevice->getLastKnownZoneID()));
    }

    if(_pDevice->getLastKnownShortAddress() != ShortAddressStaleDevice) {
      pDeviceNode->setAttribute("lastKnownShortAddress", intToString(_pDevice->getLastKnownShortAddress()));
    }

    if(_pDevice->getPropertyNode() != NULL) {
      AutoPtr<Element> pPropertiesNode = _pDocument->createElement("properties");
      pDeviceNode->appendChild(pPropertiesNode);
      _pDevice->getPropertyNode()->saveChildrenAsXML(_pDocument, pPropertiesNode, PropertyNode::Archive);
    }

    _parentNode->appendChild(pDeviceNode);
  } // deviceToXML

  void groupToXML(boost::shared_ptr<Group> _pGroup, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pGroupNode = _pDocument->createElement("group");
    pGroupNode->setAttribute("id", intToString(_pGroup->getID()));
    if(!_pGroup->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pGroup->getName());
      pNameNode->appendChild(txtNode);
      pGroupNode->appendChild(pNameNode);
    }
    if(!_pGroup->getAssociatedSet().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("associatedSet");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pGroup->getAssociatedSet());
      pNameNode->appendChild(txtNode);
      pGroupNode->appendChild(pNameNode);
    }
    if(_pGroup->isInitializedFromBus()) {
      AutoPtr<Element> pScenesNode = _pDocument->createElement("scenes");
      for(int iScene = 0; iScene < MaxSceneNumber; iScene++) {
        std::string name = _pGroup->getSceneName(iScene);
        if(!name.empty()) {
          AutoPtr<Element> pSceneNode = _pDocument->createElement("scene");
          pSceneNode->setAttribute("id", intToString(iScene));
          AutoPtr<Element> pNameNode = _pDocument->createElement("name");
          AutoPtr<Text> txtNode = _pDocument->createTextNode(name);
          pNameNode->appendChild(txtNode);
          pSceneNode->appendChild(pNameNode);
          pScenesNode->appendChild(pSceneNode);
        }
      }
      pGroupNode->appendChild(pScenesNode);
    }
    _parentNode->appendChild(pGroupNode);
  } // groupToXML

  void zoneToXML(boost::shared_ptr<Zone> _pZone, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pZoneNode = _pDocument->createElement("zone");
    pZoneNode->setAttribute("id", intToString(_pZone->getID()));
    if(!_pZone->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pZone->getName());
      pNameNode->appendChild(txtNode);
      pZoneNode->appendChild(pNameNode);
    }
    AutoPtr<Element> pGroupsNode = _pDocument->createElement("groups");
    pZoneNode->appendChild(pGroupsNode);
    foreach(boost::shared_ptr<Group> pGroup, _pZone->getGroups()) {
      groupToXML(pGroup, pGroupsNode, _pDocument);
    }
    _parentNode->appendChild(pZoneNode);
  } // zoneToXML

 void dsMeterToXML(const boost::shared_ptr<DSMeter> _pDSMeter, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pDSMeterNode = _pDocument->createElement("dsMeter");
    pDSMeterNode->setAttribute("id", _pDSMeter->getDSID().toString());
    if(!_pDSMeter->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pDSMeter->getName());
      pNameNode->appendChild(txtNode);
      pDSMeterNode->appendChild(pNameNode);
    }

    AutoPtr<Element> pHashNode = _pDocument->createElement("datamodelHash");
    AutoPtr<Text> pHashText = _pDocument->createTextNode(intToString(_pDSMeter->getDatamodelHash()));
    pHashNode->appendChild(pHashText);
    pDSMeterNode->appendChild(pHashNode);

    AutoPtr<Element> pHashModNode = _pDocument->createElement("datamodelModification");
    AutoPtr<Text> pHashModText = _pDocument->createTextNode(intToString(_pDSMeter->getDatamodelModificationCount()));
    pHashModNode->appendChild(pHashModText);
    pDSMeterNode->appendChild(pHashModNode);

    _parentNode->appendChild(pDSMeterNode);
  } // dsMeterToXML

  void ModelPersistence::writeConfigurationToXML(const std::string& _fileName) {
    Logger::getInstance()->log("Writing apartment config to '" + _fileName + "'", lsInfo);
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
    AutoPtr<Text> pApartmentNameText = pDoc->createTextNode(m_Apartment.getName());
    pApartmentName->appendChild(pApartmentNameText);
    pApartment->appendChild(pApartmentName);

    // devices
    AutoPtr<Element> pDevices = pDoc->createElement("devices");
    pRoot->appendChild(pDevices);
    foreach(boost::shared_ptr<Device> pDevice, m_Apartment.getDevicesVector()) {
      deviceToXML(pDevice, pDevices, pDoc);
    }

    // zones
    AutoPtr<Element> pZones = pDoc->createElement("zones");
    pRoot->appendChild(pZones);
    foreach(boost::shared_ptr<Zone> pZone, m_Apartment.getZones()) {
      zoneToXML(pZone, pZones, pDoc);
    }

    // dsMeters
    AutoPtr<Element> pDSMeters = pDoc->createElement("dsMeters");
    pRoot->appendChild(pDSMeters);
    foreach(boost::shared_ptr<DSMeter> pDSMeter, m_Apartment.getDSMeters()) {
      dsMeterToXML(pDSMeter, pDSMeters, pDoc);
    }

    std::string tmpOut = _fileName + ".tmp";
    std::ofstream ofs(tmpOut.c_str());

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);
      writer.writeNode(ofs, pDoc);

      ofs.close();

      syncFile(tmpOut);

      // move it to the desired location
      rename(tmpOut.c_str(), _fileName.c_str());
    } else {
      Logger::getInstance()->log("Could not open file '" + tmpOut + "' for writing", lsFatal);
    }
  } // writeConfigurationToXML

}
