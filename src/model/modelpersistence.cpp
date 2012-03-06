/*
    Copyright (c) 2010,2012 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#include "src/foreach.h"
#include "src/base.h"
#include "src/propertysystem.h"
#include "src/ds485types.h"
#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/zone.h"
#include "src/model/modulator.h"
#include "src/model/group.h"
#include "src/model/modelconst.h"

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

          bool isPresent = false;
          if(elem->hasAttribute("isPresent")) {
            try {
              int present = strToUInt(elem->getAttribute("isPresent"));
              isPresent = present > 0;
            } catch(std::invalid_argument&) {
            }
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
          newDevice->setIsPresent(isPresent);
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
        pGroup->setIsInitializedFromBus(true);
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

  void deviceToXML(boost::shared_ptr<const Device> _pDevice, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<device dsid=\"" << _pDevice->getDSID().toString() << "\"" <<
                                        " isPresent=\"" << (_pDevice->isPresent() ? "1" : "0") << "\"" <<
                                        " firstSeen=\"" << intToString(_pDevice->getFirstSeen().secondsSinceEpoch()) << "\"";
    if(_pDevice->getLastKnownDSMeterDSID() != NullDSID) {
      _ofs <<  " lastKnownDSMeter=\"" << _pDevice->getLastKnownDSMeterDSID().toString() << "\"";
    }
    if(_pDevice->getLastKnownZoneID() != 0) {
      _ofs << " lastKnownZoneID=\"" << intToString(_pDevice->getLastKnownZoneID()) << "\"";
    }
    if(_pDevice->getLastKnownShortAddress() != ShortAddressStaleDevice) {
      _ofs << " lastKnownShortAddress=\"" << intToString(_pDevice->getLastKnownShortAddress()) << "\"";
    }
    _ofs << ">" << std::endl;

    if(!_pDevice->getName().empty()) {
      _ofs << doIndent(_indent + 1) << "<name>" << XMLStringEscape(_pDevice->getName()) << "</name>" << std::endl;
    }
    if(_pDevice->getPropertyNode() != NULL) {
      _ofs << doIndent(_indent + 1) << "<properties>" << std::endl;
      _pDevice->getPropertyNode()->saveChildrenAsXML(_ofs, _indent + 2, PropertyNode::Archive);
      _ofs << doIndent(_indent + 1) << "</properties>" << std::endl;
    }

    _ofs << doIndent(_indent) + "</device>" << std::endl;
  } // deviceToXML

  void groupToXML(boost::shared_ptr<Group> _pGroup, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<group id=\"" << intToString(_pGroup->getID()) << "\">" << std::endl;
    if(!_pGroup->getName().empty()) {
      _ofs << doIndent(_indent + 1) << "<name>" << XMLStringEscape(_pGroup->getName()) << "</name>" << std::endl;
    }
    if(!_pGroup->getAssociatedSet().empty()) {
      _ofs << doIndent(_indent + 1) << "<associatedSet>" << _pGroup->getAssociatedSet() << "</associatedSet>" << std::endl;
    }
    if(_pGroup->isInitializedFromBus()) {
      _ofs << doIndent(_indent + 1) << "<scenes>" << std::endl;
      for(int iScene = 0; iScene < MaxSceneNumber; iScene++) {
        std::string name = _pGroup->getSceneName(iScene);
        if(!name.empty()) {
          _ofs << doIndent(_indent + 2) << "<scene id=\"" << intToString(iScene) << "\">" << std::endl;
          _ofs << doIndent(_indent + 3) << "<name>" << XMLStringEscape(name) << "</name>" << std::endl;
          _ofs << doIndent(_indent + 2) << "</scene>" << std::endl;
        }
      }
      _ofs << doIndent(_indent + 1) << "</scenes>" << std::endl;
    }
    _ofs << doIndent(_indent) << "</group>" << std::endl;
  } // groupToXML

  void zoneToXML(boost::shared_ptr<Zone> _pZone, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<zone id=\"" << intToString(_pZone->getID()) << "\">" << std::endl;
    if(!_pZone->getName().empty()) {
      _ofs << doIndent(_indent + 1) << "<name>" << XMLStringEscape(_pZone->getName()) << "</name>" << std::endl;
    }
    _ofs << doIndent(_indent + 1) << "<groups>" << std::endl;
    foreach(boost::shared_ptr<Group> pGroup, _pZone->getGroups()) {
      groupToXML(pGroup, _ofs, _indent + 2);
    }
    _ofs << doIndent(_indent + 1) << "</groups>" << std::endl;
    _ofs << doIndent(_indent) << "</zone>" << std::endl;
  } // zoneToXML

 void dsMeterToXML(const boost::shared_ptr<DSMeter> _pDSMeter, std::ofstream& _ofs, const int _indent) {
   _ofs <<  doIndent(_indent) << "<dsMeter id=\"" + _pDSMeter->getDSID().toString() << "\">" << std::endl;
    if(!_pDSMeter->getName().empty()) {
      _ofs << doIndent(_indent + 1) << "<name>" + XMLStringEscape(_pDSMeter->getName()) << "</name>" << std::endl;
    }

    _ofs << doIndent(_indent + 1) << "<datamodelHash>" <<
                                        intToString(_pDSMeter->getDatamodelHash()) <<
                                     "</datamodelHash>" << std::endl;

    _ofs << doIndent(_indent + 1) << "<datamodelModification>" <<
                                        intToString(_pDSMeter->getDatamodelModificationCount()) <<
                                     "</datamodelModification>" << std::endl;

    _ofs << doIndent(_indent) << "</dsMeter>" << std::endl;
  } // dsMeterToXML

  void ModelPersistence::writeConfigurationToXML(const std::string& _fileName) {
    Logger::getInstance()->log("Writing apartment config to '" + _fileName + "'", lsInfo);

    std::string tmpOut = _fileName + ".tmp";
    std::ofstream ofs(tmpOut.c_str());

    if(ofs) {
      ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
      int indent = 0;

      ofs << doIndent(indent) << "<config version=\"1\">" << std::endl;
      indent++;

      // apartment
      ofs << doIndent(indent) << "<apartment>" << std::endl;
      ofs << doIndent(indent + 1) << "<name>" + XMLStringEscape(m_Apartment.getName()) << "</name>" << std::endl;
      ofs << doIndent(indent) << "</apartment>" << std::endl;

      // devices
      ofs << doIndent(indent) << "<devices>" << std::endl;
      foreach(boost::shared_ptr<Device> pDevice, m_Apartment.getDevicesVector()) {
        deviceToXML(pDevice, ofs, indent + 1);
      }
      ofs << doIndent(indent) << "</devices>" << std::endl;

      // zones
      ofs << doIndent(indent) << "<zones>" << std::endl;
      foreach(boost::shared_ptr<Zone> pZone, m_Apartment.getZones()) {
        zoneToXML(pZone, ofs, indent + 1);
      }
      ofs << doIndent(indent) << "</zones>" << std::endl;

      // dsMeters
      ofs << doIndent(indent) << "<dsMeters>" << std::endl;
      foreach(boost::shared_ptr<DSMeter> pDSMeter, m_Apartment.getDSMeters()) {
        dsMeterToXML(pDSMeter, ofs, indent + 1);
      }
      ofs << doIndent(indent) << "</dsMeters>" << std::endl;

      indent--;
      ofs << doIndent(indent) << "</config>" << std::endl;

      ofs.close();

      syncFile(tmpOut);

      // move it to the desired location
      rename(tmpOut.c_str(), _fileName.c_str());
    } else {
      Logger::getInstance()->log("Could not open file '" + tmpOut + "' for writing", lsFatal);
    }
  } // writeConfigurationToXML

}
