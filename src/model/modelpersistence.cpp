/*
    Copyright (c) 2010,2012 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Christian Hitz, aizo AG <christian.hitz@aizo.com>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

namespace dss {

  ModelPersistence::ModelPersistence(Apartment& _apartment)
  : m_Apartment(_apartment), m_ignore(false), m_expectString(false),
                             m_level(0), m_state(ps_none), m_tempScene(-1)
  {
      m_chardata.clear();
  } // ctor

  ModelPersistence::~ModelPersistence() {
  } // dtor

  const int ApartmentConfigVersion = 1;

  void ModelPersistence::parseDevice(const char *_name, const char **_attrs) {
    const char *dsid = NULL;
    const char *present = NULL;
    const char *fs = NULL;
    const char *lastmeter = NULL;
    const char *lastzone = NULL;
    const char *lastshort = NULL;
    const char *is = NULL;

    if (strcmp(_name, "device") != 0) {
      return;
    }

    for (int i = 0; _attrs[i]; i += 2)
    {
      if (strcmp(_attrs[i], "dsid") == 0) {
        dsid = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "isPresent") == 0) {
        present = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "firstSeen") == 0) {
        fs = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "lastKnownDSMeter") == 0) {
        lastmeter = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "lastKnownZoneID") == 0) {
        lastzone = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "lastKnownShortAddress") == 0) {
        lastshort = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "inactiveSince") == 0) {
        is = _attrs[i + 1];
      }
    }

    m_tempDevice.reset();

    if (dsid == NULL) {
      return;
    }

    m_tempDevice = m_Apartment.allocateDevice(dss_dsid_t::fromString(dsid));
    bool isPresent = false;
    if (present != NULL) {
      try {
        int p = strToUInt(present);
        isPresent = p > 0;
      } catch(std::invalid_argument&) {}
    }

    DateTime firstSeen = DateTime::NullDate;
    if (fs != NULL) {
      try {
        time_t timestamp = strToUInt(fs);
        firstSeen = DateTime(timestamp);
      } catch(std::invalid_argument&) {
        firstSeen = DateTime(dateFromISOString(fs));
      }
    }

    DateTime inactiveSince = DateTime::NullDate;
    if (is != NULL) {
      try {
        time_t timestamp = strToUInt(is);
        inactiveSince = DateTime(timestamp);
      } catch(std::invalid_argument&) {
        inactiveSince = DateTime(dateFromISOString(is));
      }
    }

    dss_dsid_t lastKnownDsMeter = NullDSID;
    if (lastmeter != NULL) {
      lastKnownDsMeter = dss_dsid_t::fromString(lastmeter);
    }

    int lastKnownZoneID = 0;
    if (lastzone != NULL) {
      lastKnownZoneID = strToIntDef(lastzone, 0);
    }

    devid_t lastKnownShortAddress = ShortAddressStaleDevice;
    if (lastshort != NULL) {
      lastKnownShortAddress = strToUIntDef(lastshort,
                                           ShortAddressStaleDevice);
    }

    m_tempDevice->setIsPresent(isPresent);
    if (isPresent == false) {
      m_tempDevice->setInactiveSince(inactiveSince);
    }
    m_tempDevice->setFirstSeen(firstSeen);
    m_tempDevice->setLastKnownDSMeterDSID(lastKnownDsMeter);
    m_tempDevice->setLastKnownZoneID(lastKnownZoneID);
    if(lastKnownZoneID != 0) {
      DeviceReference devRef(m_tempDevice, &m_Apartment);
      m_Apartment.allocateZone(lastKnownZoneID)->addDevice(devRef);
    }
    m_tempDevice->setLastKnownShortAddress(lastKnownShortAddress);
    return;
  }

  void ModelPersistence::parseMeter(const char *_name, const char **_attrs) {

    if ((strcmp(_name, "dsMeter") != 0)  &&(strcmp(_name, "modulator") != 0)) {
      return;
    }
        
    m_tempMeter.reset();
    const char *id = getSingleAttribute("id", _attrs);
    if (id == NULL) {
      return;
    }

    dss_dsid_t dsid = dss_dsid_t::fromString(id);
    m_tempMeter = m_Apartment.allocateDSMeter(dsid);
  }

  void ModelPersistence::parseZone(const char *_name, const char **_attrs) {
    if(strcmp(_name, "zone") != 0) {
      return;
    }

    m_tempZone.reset();
    const char *zid = getSingleAttribute("id", _attrs);
    if (zid != NULL) {
      m_tempZone = m_Apartment.allocateZone(strToInt(zid));
    }
  }

  void ModelPersistence::parseGroup(const char *_name, const char **_attrs) {
    if ((m_tempZone == NULL) || (strcmp(_name, "group") != 0)) {
      return;
    }

    m_tempGroup.reset();
    const char *gid = getSingleAttribute("id", _attrs);
    if (gid == NULL) {
      return;
    }

    int groupID = strToIntDef(gid, -1);
    if (groupID == -1) {
      return;
    }

    m_tempGroup = m_tempZone->getGroup(groupID);
    if (m_tempGroup == NULL) {
      m_tempGroup.reset(new Group(groupID, m_tempZone, m_Apartment));
      m_tempZone->addGroup(m_tempGroup);
    }
    m_tempGroup->setIsInitializedFromBus(true);
  }

  void ModelPersistence::parseScene(const char *_name, const char **_attrs) {
    if (m_tempGroup == NULL) {
      return;
    }

    if (strcmp(_name, "scene") != 0) {
      return;
    }

    m_tempScene = -1;

    const char *sid = getSingleAttribute("id", _attrs);
    if (sid == NULL) {
      return;
    }

    int snum = strToIntDef(sid, -1);
    m_tempScene = snum;
  }

  const char *ModelPersistence::getSingleAttribute(const char* _name, 
                                                   const char **_attrs) {
    const char *ret = NULL;
    for (int i = 0; _attrs[i]; i += 2)
    {
      if (strcmp(_attrs[i], _name) == 0) {
        ret = _attrs[i + 1];
      }
    }
    return ret;
  }

  void ModelPersistence::elementStart(const char *_name, const char **_attrs) {
    if (m_forceStop) {
      return;
    }

    try {
      m_expectString = false;

      if (m_level == 0) {
        m_state = ps_none;
        // first node must be named "config"
        if (strcmp(_name, "config") != 0) {
          Logger::getInstance()->log("ModelPersistence: missing <config> node "
                                     "in apartment configuration", lsError);
          m_forceStop = true;
          return;
        }

        // now check version
        const char *version = getSingleAttribute("version", _attrs);
        if (version == NULL) {
          Logger::getInstance()->log("ModelPersistence: missing config "
                                     "version attribute", lsError);
          m_forceStop = true;
          return;
        }

        if (strToIntDef(version, -1) != ApartmentConfigVersion) {
          Logger::getInstance()->log("ModelPersistence: unsupported "
              "configuration version: " + std::string(version), lsError);
          m_forceStop = true;
          return;
        }

        m_chardata.clear();

        m_level++;
        return;
      }

      // level 1 supports only <apartment>, <devices>, <zones>, <dsMeters>
      if (m_level == 1) {
        if (strcmp(_name, "apartment") == 0) {
          m_state = ps_apartment;
        } else if (strcmp(_name, "devices") == 0) {
          m_state = ps_device;
        } else if (strcmp(_name, "zones") == 0) {
          m_state = ps_zone;
        } else if ((strcmp(_name, "dsMeters") == 0) ||
                   (strcmp(_name, "modulators") == 0)) {
          m_state = ps_meter;
        } else {
          m_state = ps_none;
          m_ignore = true;
          m_level++;
          return;
        }
      }

      // we are parsing a path that is being ignored, so do nothing
      if ((m_level > 1) && m_ignore)
      {
        m_level++;
        return;
      }

      if (m_state == ps_properties) {
        m_propParser->elementStartCb(_name, _attrs);
        m_level++;
        return;
      }

      // level 2 supprts <device>, <zone>, <dsMeter>
      if (m_level == 2) {
        if ((m_state == ps_apartment) && (strcmp(_name, "name") == 0)) {
          m_expectString = true;
        } else if (m_state == ps_device) {
          parseDevice(_name, _attrs);
        } else if (m_state == ps_zone) {
          parseZone(_name, _attrs);
        } else if (m_state == ps_meter) {
          parseMeter(_name, _attrs);
        }
      // level 3 supports <name>, <properties>, <groups>, <datamodelHash>,
      // <datamodelModification>
      } else if (m_level == 3) {
        if (((m_state == ps_device) || (m_state == ps_zone)) &&
            (strcmp(_name, "name") == 0)) {
          m_expectString = true;
        } else if ((m_state == ps_zone) && (strcmp(_name, "groups") == 0)) {
          m_state = ps_group;
        } else if (m_state == ps_meter) {
          m_expectString = true;
        } else if ((m_state == ps_device) &&
                   (strcmp(_name, "properties") == 0)) {
          m_state = ps_properties;
          if (m_tempDevice != NULL) {
            m_propParser->reset(m_tempDevice->getPropertyNode(), true);
            m_propParser->elementStartCb(_name, _attrs);
          }
        }
      // level 4 supports <property>, <group>
      } else if (m_level == 4) {
        if (m_state == ps_group) {
          parseGroup(_name, _attrs);
        }
      // level 5 supports <property>, <value>, <name>, <scenes>, <associatedSet>
      } else if (m_level == 5) {
        if ((m_state == ps_group) && 
            ((strcmp(_name, "name") == 0) || 
             (strcmp(_name, "associatedSet") == 0))) {
          m_expectString = true;
        } else if ((m_state == ps_group) && (strcmp(_name, "scenes") == 0)) {
          m_state = ps_scene;
        }
      // level 6 supports <property>, <value>, <scene>
      } else if (m_level == 6) {
        if ((m_state == ps_scene)) {
          parseScene(_name, _attrs); 
        }
      // level 7 supports <property>, <value>, <name>
      } else if (m_level == 7) {
        if ((m_state == ps_scene) && (strcmp(_name, "name") == 0)) {
          m_expectString = true;
        }
      }

      m_level++;

    } catch (std::runtime_error& ex) {
      m_forceStop = true;
      Logger::getInstance()->log(std::string("ModelPersistence::"
              "readConfigurationFromXML: element start handler caught "
              "exception: ") + ex.what() + " Will abort parsing!", lsError);
    } catch (...) {
      m_forceStop = true;
      Logger::getInstance()->log("ModelPersistence::readConfigurationFromXML: "
              "element start handler caught exception! Will abort parsing!",
              lsError);
    }
  }

  void ModelPersistence::elementEnd(const char *_name) {
    if (m_forceStop) {
      return;
    }

    m_expectString = false;
    m_level--;
    if (m_level < 0) {
        Logger::getInstance()->log("ModelPersistence::elementEnd: "
                                   "invalid document depth!", lsError);
        m_forceStop = true;
        m_level--;
        return;
    }

    if (m_ignore) {
      return;
    }

    if ((m_state == ps_properties) && (m_level > 2)) {
      m_propParser->elementEndCb(_name);
    }

    switch (m_level) {
      case 1:
        m_state = ps_none;
        break;
      case 2:
        if ((m_state == ps_apartment) && (strcmp(_name, "name") == 0)) {
          m_Apartment.setName(m_chardata);
        }
        break;
      case 3:
        {
          if ((m_state == ps_properties) && (strcmp(_name, "properties") == 0)){
            m_state = ps_device;
            break;
          }
          bool isName = !(strcmp(_name, "name"));
          if (!m_chardata.empty()) {
            if ((m_state == ps_device) && (m_tempDevice != NULL) && isName) {
              m_tempDevice->setName(m_chardata);
            } else if ((m_state == ps_zone) && (m_tempZone != NULL) && isName) {
              m_tempZone->setName(m_chardata);
            } else if ((m_state == ps_meter) && (m_tempMeter != NULL)) {
              if (isName) {
                m_tempMeter->setName(m_chardata);
              } else if (strcmp(_name, "datamodelHash") == 0) {
                m_tempMeter->setDatamodelHash(strToInt(m_chardata));
              } else if (strcmp(_name, "datamodelModification") == 0) {
                m_tempMeter->setDatamodelModificationcount(strToInt(m_chardata));
              }
            }
          }
          if ((m_state == ps_group) && (strcmp(_name, "groups") == 0)) {
            m_state = ps_zone;
          }
        }
        break;
      case 5:
        if ((m_state == ps_group) && (m_tempGroup != NULL) &&
            !m_chardata.empty()) {
          if (strcmp(_name, "name") == 0) {
            m_tempGroup->setName(m_chardata);
          } else if (strcmp(_name, "associatedSet") == 0) {
            m_tempGroup->setAssociatedSet(m_chardata);
          }
        } else if ((m_state == ps_scene) && (strcmp(_name, "scenes") == 0)) {
          m_state = ps_group;
        }
        break;
      case 7:
        if ((m_state == ps_scene) && (strcmp(_name, "name") == 0) &&
            (m_tempGroup != NULL) && (m_tempScene != -1) &&
            !m_chardata.empty()) {
          m_tempGroup->setSceneName(m_tempScene, m_chardata);
        }
        break;
      default:
        break;
     }

    m_expectString = false;
    m_chardata.clear();

  }

  void ModelPersistence::characterData(const XML_Char *_s, int _len) {
    if (m_forceStop) {
      return;
    }

    if ((m_state == ps_properties) && (m_level > 3)) {
      m_propParser->characterDataCb(_s, _len);
      return;
    }

    if ((!m_ignore) && (m_expectString) && (_len > 0)) {
      m_chardata += std::string(_s, _len);
    }
  }

  void ModelPersistence::readConfigurationFromXML(const std::string& _fileName) {
    m_Apartment.setName("dSS");
    m_ignore = false;
    m_expectString = false;
    m_level = 0;
    m_state = ps_none;
    m_chardata.clear();
    m_tempDevice.reset();
    m_tempZone.reset();
    m_tempMeter.reset();
    m_tempGroup.reset();
    m_tempScene = -1;

    m_propParser.reset(new PropertyParserProxy());

    bool ret = parseFile(_fileName);
    if (!ret) {
      throw std::runtime_error("ModelPersistence::readConfigurationFromXML: "
                               "Parse error in Model configuration");
    }
    return;
  } // readConfigurationFromXML

  void deviceToXML(boost::shared_ptr<const Device> _pDevice, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<device dsid=\""
         << _pDevice->getDSID().toString() << "\""
         << " isPresent=\"" << (_pDevice->isPresent() ? "1" : "0") << "\""
         << " firstSeen=\""
         << intToString(_pDevice->getFirstSeen().secondsSinceEpoch()) << "\""
         << " inactiveSince=\""
         << _pDevice->getInactiveSince().toString() << "\"";

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
