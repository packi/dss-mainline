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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "modelpersistence.h"

#include <stdexcept>

#include <digitalSTROM/dsuid.h>

#include "src/foreach.h"
#include "src/base.h"
#include "src/util.h"
#include "src/backtrace.h"
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
    const char *dsuid = NULL;
    const char *present = NULL;
    const char *fs = NULL;
    const char *lastmeter = NULL;
    const char *lastzone = NULL;
    const char *lastshort = NULL;
    const char *is = NULL;
    const char *oemState = NULL;
    const char *oemEan = NULL;
    const char *oemSerial = NULL;
    const char *oemPar = NULL;
    const char *oemIndep = NULL;
    const char *oemInet = NULL;
    const char *oemProdState = NULL;
    const char *oemProdName = NULL;
    const char *oemProdIcon = NULL;
    const char *oemProdURL = NULL;
    const char *configLocked = NULL;
    const char *valve = NULL;

    if (strcmp(_name, "device") != 0) {
      return;
    }

    for (int i = 0; _attrs[i]; i += 2)
    {
      if (strcmp(_attrs[i], "dsid") == 0) {
        dsid = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "dsuid") == 0) {
        dsuid = _attrs[i + 1];
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
      } else if (strcmp(_attrs[i], "oemState") == 0) {
        oemState = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemEanNumber") == 0) {
        oemEan = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemSerialNumber") == 0) {
        oemSerial = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemPartNumber") == 0) {
        oemPar = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemIsIndependent") == 0) {
        oemIndep = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemInetState") == 0) {
        oemInet = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemProductState") == 0) {
        oemProdState = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemProductName") == 0) {
        oemProdName = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemProductIcon") == 0) {
        oemProdIcon = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "oemProductURL") == 0) {
        oemProdURL = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "configurationLocked") == 0) {
        configLocked = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "valveType") == 0) {
        valve = _attrs[i + 1];
      }
    }

    m_tempDevice.reset();

    dsuid_t _dsuid;
    if (dsuid == NULL) {
      if (dsid == NULL) {
        return;
      }
      dsid_t _dsid;
      try {
        _dsid = str2dsid(dsid);
        _dsuid = dsuid_from_dsid(&_dsid);
      } catch (std::runtime_error &ex) {
        Logger::getInstance()->log("ModelPersistence: could not convert dSID to dSUID for device " + std::string(dsid) + ": " + ex.what(), lsError);
        return;
      }
    } else {
      try {
        _dsuid = str2dsuid(dsuid);
      } catch (std::runtime_error &ex) {
        Logger::getInstance()->log("ModelPersistence: could not convert dSID to dSUID for device " + std::string(dsid) + ": " + ex.what(), lsError);
        return;
      }
    }

    m_tempDevice = m_Apartment.allocateDevice(_dsuid);
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
        try {
          // TODO probably some ISO format matched better here
          firstSeen = DateTime::parsePrettyString(std::string(fs));
        } catch(std::invalid_argument&) {
          // go with the NULL date
          Logger::getInstance()->log("firstSeen date invalid: " +
                                     std::string(fs), lsInfo);
        }
      }
    }

    DateTime inactiveSince = DateTime::NullDate;
    if (is != NULL) {
      try {
        time_t timestamp = strToUInt(is);
        inactiveSince = DateTime(timestamp);
      } catch(std::invalid_argument&) {
        try {
          // TODO probably some ISO format matched better here
          inactiveSince = DateTime::parsePrettyString(std::string(is));
        } catch(std::invalid_argument&) {
          // go with the NULL date
          Logger::getInstance()->log("inactiveSince invalid: " + std::string(is),
                                     lsInfo);
        }
      }
    }

    dsuid_t lastKnownDsMeter = DSUID_NULL;

    if (lastmeter != NULL) {
      try {
        lastKnownDsMeter = str2dsuid(lastmeter);
      } catch (std::runtime_error&) {
        try {
          dsid_t lm = str2dsid(lastmeter);
          lastKnownDsMeter = dsuid_from_dsid(&lm);
        } catch (std::runtime_error &ex) {
          Logger::getInstance()->log("ModelPersistence: could not convert dSID to dSUID for meter " + std::string(dsid) + ": " + ex.what(), lsError);
        }
      }
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

    DeviceOEMState_t oemEanState = DEVICE_OEM_UNKOWN;
    if (oemState != NULL) {
      oemEanState = m_tempDevice->getOemStateFromString(oemState);
    }

    unsigned long long oemEanNumber = 0;
    int oemSerialNumber = 0;
    int oemPartNumber = 0;
    bool isIndependent = false;
    DeviceOEMInetState_t oemInetState = DEVICE_OEM_EAN_NO_EAN_CONFIGURED;
    if (oemEanState == DEVICE_OEM_VALID) {
      if (oemEan != NULL) {
        oemEanNumber = strToULongLongDef(oemEan, 0);
      }
      if (oemSerial != NULL) {
        oemSerialNumber = strToUIntDef(oemSerial, 0);
      }
      if (oemPar != NULL) {
        oemPartNumber = strToUIntDef(oemPar, 0);
      }
      if (oemIndep != NULL) {
        isIndependent = (strToIntDef(oemIndep, 0) == 1);
      }
      if (oemInet != NULL) {
        oemInetState = m_tempDevice->getOemInetStateFromString(oemInet);
      }

      m_tempDevice->setOemInfo(oemEanNumber, oemSerialNumber, oemPartNumber,
                               oemInetState, isIndependent);

      DeviceOEMState_t productInfoState = DEVICE_OEM_UNKOWN;
      if (oemProdState != NULL) {
        productInfoState = m_tempDevice->getOemStateFromString(oemProdState);
      }

      if (productInfoState == DEVICE_OEM_VALID) {
        if (oemProdName != NULL && oemProdIcon != NULL && oemProdURL != NULL) {
          m_tempDevice->setOemProductInfo(oemProdName, oemProdIcon, oemProdURL);
        }
        m_tempDevice->setOemProductInfoState(productInfoState);
      }
    }
    m_tempDevice->setOemInfoState(oemEanState);

    bool isConfigLocked = false;
    if (configLocked != NULL) {
      try {
        int p = strToUInt(configLocked);
        isConfigLocked = p > 0;
      } catch(std::invalid_argument&) {}
    }
    m_tempDevice->setConfigLock(isConfigLocked);

    if (valve != NULL) {
      m_tempDevice->setValveTypeAsString(valve, true);
    }

    m_tempDevice->setIsValid(true);
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

    dsuid_t dsuid;
    try {
      dsuid = str2dsuid(id);
    } catch (std::runtime_error&) {
      try {
        dsid_t dsid = str2dsid(id);
        dsuid = dsuid_from_dsid(&dsid);
      } catch (std::runtime_error &ex) {
        Logger::getInstance()->log("ModelPersistence: could not convert dSID to dSUID for meter " + std::string(id) + ": " + ex.what(), lsError);
        return;
      }
    }
    std::string ownDSUID;
    PropertySystem* pPropSystem = m_Apartment.getPropertySystem();
    if (pPropSystem) {
      ownDSUID = pPropSystem->getStringValue(pp_sysinfo_dsuid);
    }

    if (dsuid2str(dsuid) == ownDSUID) {
      return;
    }
    m_tempMeter = m_Apartment.allocateDSMeter(dsuid);
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

    const char *gid = NULL;
    const char *grName = NULL;
    for (int i = 0; _attrs[i]; i += 2)
    {
      if (strcmp(_attrs[i], "id") == 0) {
        gid = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "name") == 0) {
        grName = _attrs[i + 1];
      }
    }

    if (gid == NULL) {
      return;
    }
    int groupID = strToIntDef(gid, -1);
    if (groupID == -1) {
      return;
    }

    m_tempGroup.reset();
    m_tempGroup = m_tempZone->getGroup(groupID);
    if (m_tempGroup == NULL) {
      m_tempGroup.reset(new Group(groupID, m_tempZone, m_Apartment));
      m_tempZone->addGroup(m_tempGroup);
    }
    if (grName) {
      m_tempGroup->setName(grName);
    }
    m_tempGroup->setIsValid(true);
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

  void ModelPersistence::parseSensor(const char *_name, const char **_attrs) {
    if ((m_tempZone == NULL) || (strcmp(_name, "sensor") != 0)) {
      return;
    }

    const char *dsuid = NULL;
    const char *sensorType = NULL;
    const char *sensorIndex = NULL;

    for (int i = 0; _attrs[i]; i += 2)
    {
      if (strcmp(_attrs[i], "dsuid") == 0) {
        dsuid = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "sensorType") == 0) {
        sensorType = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "sensorIndex") == 0) {
        sensorIndex = _attrs[i + 1];
      }
    }

    if (dsuid == NULL || sensorType == NULL || sensorIndex == NULL) {
      return;
    }

    dsuid_t _dsuid;
    try {
      _dsuid = str2dsuid(dsuid);
    } catch (std::runtime_error &ex) {
      Logger::getInstance()->log("ModelPersistence: could not convert dSUID for device " + std::string(dsuid) + ": " + ex.what(), lsError);
      return;
    }

    int _sensorType;
    try {
      _sensorType = strToInt(sensorType);
    } catch (std::runtime_error &ex) {
      Logger::getInstance()->log("ModelPersistence: could not convert sensortype for device " + std::string(sensorType) + ": " + ex.what(), lsError);
      return;
    }

    int _sensorIndex;
    try {
      _sensorIndex = strToInt(sensorIndex);
    } catch (std::runtime_error &ex) {
      Logger::getInstance()->log("ModelPersistence: could not convert sensor index for device " + std::string(sensorIndex) + ": " + ex.what(), lsError);
      return;
    }

    boost::shared_ptr<MainZoneSensor_t> ms = boost::make_shared<MainZoneSensor_t>();
    ms->m_DSUID = _dsuid;
    ms->m_sensorType = _sensorType;
    ms->m_sensorIndex = _sensorIndex;

    m_tempZone->setSensor(ms);
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
      // <datamodelModification>, <sensors>
      } else if (m_level == 3) {
        if (((m_state == ps_device) || (m_state == ps_zone)) &&
            (strcmp(_name, "name") == 0)) {
          m_expectString = true;
        } else if ((m_state == ps_zone) && (strcmp(_name, "groups") == 0)) {
          m_state = ps_group;
        } else if ((m_state == ps_zone) && (strcmp(_name, "sensors") == 0)) {
          m_state = ps_sensor;
        } else if ((m_state == ps_zone) && (strcmp(_name, "heatingOperation") == 0)) {
          m_expectString = true;
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
        } else if (m_state == ps_sensor) {
          parseSensor(_name, _attrs);
        }
      // level 5 supports <property>, <value>, <name>, <scenes>, <associatedSet>, <color>
      } else if (m_level == 5) {
        if ((m_state == ps_group) &&
            ((strcmp(_name, "name") == 0) ||
             (strcmp(_name, "color") == 0) ||
             (strcmp(_name, "associatedSet") == 0))) {
          m_expectString = true;
        } else if ((m_state == ps_group) && (strcmp(_name, "scenes") == 0)) {
          m_state = ps_scene;
        }
      // level 6 supports <property>, <value>, <scene>
      } else if (m_level == 6) {
        if (m_state == ps_scene) {
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
      Backtrace::logBacktrace();
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
            } else if ((m_state == ps_zone) && (m_tempZone != NULL)) {
              if (isName) {
                m_tempZone->setName(m_chardata);
              } else if (strcmp(_name ,"heatingOperation") == 0) {
                m_tempZone->setHeatingOperationMode(strToInt(m_chardata));
              }
            } else if ((m_state == ps_meter) && (m_tempMeter != NULL)) {
              if (isName) {
                m_tempMeter->setName(m_chardata);
              } else if (strcmp(_name, "datamodelHash") == 0) {
                m_tempMeter->setDatamodelHash(strToInt(m_chardata));
              } else if (strcmp(_name, "datamodelModification") == 0) {
                m_tempMeter->setDatamodelModificationcount(strToInt(m_chardata));
              } else if (strcmp(_name, "deviceType") == 0) {
                m_tempMeter->setBusMemberType((BusMemberDevice_t) strToInt(m_chardata));
              }
            }
          }
          if ((m_state == ps_group) && (strcmp(_name, "groups") == 0)) {
            m_state = ps_zone;
          } else if ((m_state == ps_sensor) && (strcmp(_name, "sensors") == 0)) {
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
          } else if (strcmp(_name, "color") == 0) {
            m_tempGroup->setStandardGroupID(strToUIntDef(m_chardata, 0));
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

  void ModelPersistence::readConfigurationFromXML(const std::string& _fileName,
                                                  const std::string& _backup) {
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

    if (!parseFile(_fileName)) {
      Logger::getInstance()->log("apartment.xml is invalid, will backup up to "
                                 + _backup, lsError);
      int ret = rename(_fileName.c_str(), _backup.c_str());
      if (ret < 0) {
        Logger::getInstance()->log("failed to write file " +
                                   _backup, lsWarning);
      }
    }
    if (m_Apartment.getName().empty()) {
      m_Apartment.setName("dSS");
    }
    return;
  } // readConfigurationFromXML

  void deviceToXML(boost::shared_ptr<const Device> _pDevice, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<device dsuid=\""
         << dsuid2str(_pDevice->getDSID()) << "\""
         << " isPresent=\"" << (_pDevice->isPresent() ? "1" : "0") << "\""
         << " firstSeen=\""
         << intToString(_pDevice->getFirstSeen().secondsSinceEpoch()) << "\""
         << " inactiveSince=\""
         << _pDevice->getInactiveSince().toString() << "\"";

    if (_pDevice->getLastKnownDSMeterDSID() != DSUID_NULL) {
      _ofs <<  " lastKnownDSMeter=\"" << dsuid2str(_pDevice->getLastKnownDSMeterDSID()) << "\"";
    }
    if(_pDevice->getLastKnownZoneID() != 0) {
      _ofs << " lastKnownZoneID=\"" << intToString(_pDevice->getLastKnownZoneID()) << "\"";
    }
    if(_pDevice->getLastKnownShortAddress() != ShortAddressStaleDevice) {
      _ofs << " lastKnownShortAddress=\"" << intToString(_pDevice->getLastKnownShortAddress()) << "\"";
    }
    if(_pDevice->getOemInfoState() == DEVICE_OEM_NONE) {
      _ofs << " oemState=\"" << _pDevice->getOemStateAsString() << "\"";
    } else if(_pDevice->getOemInfoState() == DEVICE_OEM_VALID) {
      _ofs << " oemState=\"" << _pDevice->getOemStateAsString() << "\"";
      _ofs << " oemEanNumber=\"" << _pDevice->getOemEanAsString() << "\"";
      _ofs << " oemSerialNumber=\"" << intToString(_pDevice->getOemSerialNumber()) << "\"";
      _ofs << " oemPartNumber=\"" << intToString(_pDevice->getOemPartNumber()) << "\"";
      _ofs << " oemIsIndependent=\"" << intToString(_pDevice->getOemIsIndependent() ? 1 : 0) << "\"";
      _ofs << " oemInetState=\"" << _pDevice->getOemInetStateAsString() << "\"";
      if(_pDevice->getOemProductInfoState() == DEVICE_OEM_VALID) {
        _ofs << " oemProductState=\"" << _pDevice->getOemProductInfoStateAsString() << "\"";
        _ofs << " oemProductName=\"" << XMLStringEscape(_pDevice->getOemProductName()) << "\"";
        _ofs << " oemProductIcon=\"" << XMLStringEscape(_pDevice->getOemProductIcon()) << "\"";
        _ofs << " oemProductURL=\"" << XMLStringEscape(_pDevice->getOemProductURL()) << "\"";
      }
    }
    _ofs << " configurationLocked=\"" << (_pDevice->isConfigLocked() ? "1" : "0") << "\"";

    /*
     * Problem: we can't rely on isValveDevice, because the
     * corresponding DSM might not have been read out yet,
     * hence the type of device might be unknown
     * On the other side, getValveTyp != UNKNOWN must have
     * been set by the user, it can't be read out from the
     * device, hence never lose that setting!
     */
    if (_pDevice->getValveType() != DEVICE_VALVE_UNKNOWN) {
        _ofs << " valveType=\"" << _pDevice->getValveTypeAsString() <<"\"";
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
    if (isAppUserGroup(_pGroup->getID())) {
      if (!_pGroup->getName().empty()) {
        _ofs << doIndent(_indent + 1) << "<name>" << XMLStringEscape(_pGroup->getName()) << "</name>" << std::endl;
      }
      if (!_pGroup->getAssociatedSet().empty()) {
        _ofs << doIndent(_indent + 1) << "<associatedSet>" << XMLStringEscape(_pGroup->getAssociatedSet()) << "</associatedSet>" << std::endl;
      }
      _ofs << doIndent(_indent + 1) << "<color>" << intToString(_pGroup->getStandardGroupID()) << "</color>" << std::endl;
    }
    _ofs << doIndent(_indent + 1) << "<scenes>" << std::endl;
    for (int iScene = 0; iScene < MaxSceneNumber; iScene++) {
      std::string name = _pGroup->getSceneName(iScene);
      if (!name.empty()) {
        _ofs << doIndent(_indent + 2) << "<scene id=\"" << intToString(iScene) << "\">" << std::endl;
        _ofs << doIndent(_indent + 3) << "<name>" << XMLStringEscape(name) << "</name>" << std::endl;
        _ofs << doIndent(_indent + 2) << "</scene>" << std::endl;
      }
    }
    _ofs << doIndent(_indent + 1) << "</scenes>" << std::endl;
    _ofs << doIndent(_indent) << "</group>" << std::endl;
  } // groupToXML

  void zoneSensorToXML(boost::shared_ptr<MainZoneSensor_t> _zoneSensor, std::ofstream& _ofs, const int _indent)
  {
    _ofs << doIndent(_indent) << "<sensor dsuid=\""
         << dsuid2str(_zoneSensor->m_DSUID) << "\""
         << " sensorType=\"" << intToString(_zoneSensor->m_sensorType) << "\""
         << " sensorIndex=\"" << intToString(_zoneSensor->m_sensorIndex)  << "\"/>"
         << std::endl;
  } // zoneSensorToXML

  void zoneToXML(boost::shared_ptr<Zone> _pZone, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<zone id=\"" << intToString(_pZone->getID()) << "\">" << std::endl;
    if(!_pZone->getName().empty()) {
      _ofs << doIndent(_indent + 1) << "<name>" << XMLStringEscape(_pZone->getName()) << "</name>" << std::endl;
    }
    _ofs << doIndent(_indent + 1) << "<groups>" << std::endl;

    if (_pZone->getID() == 0) {
      // store unique "apartment user-groups" in zone 0
      foreach(boost::shared_ptr<Group> pGroup, _pZone->getGroups()) {
        if (isAppUserGroup(pGroup->getID())) {
          groupToXML(pGroup, _ofs, _indent + 2);
        }
      }
    } else {
      // store real user-groups per zone
      foreach(boost::shared_ptr<Group> pGroup, _pZone->getGroups()) {
        groupToXML(pGroup, _ofs, _indent + 2);
      }
    }
    _ofs << doIndent(_indent + 1) << "</groups>" << std::endl;

    // Zone sensors
    if (_pZone->getID() != 0) {
      std::vector<boost::shared_ptr<MainZoneSensor_t> > slist = _pZone->getAssignedSensors();

      if ( !slist.empty() ) {
        _ofs << doIndent(_indent + 1) << "<sensors>" << std::endl;
        for (std::vector<boost::shared_ptr<MainZoneSensor_t> >::iterator it = slist.begin();
            it != slist.end();
            it ++) {
          boost::shared_ptr<MainZoneSensor_t> devSensor = *it;
          zoneSensorToXML(devSensor, _ofs, _indent+2);
        }
        _ofs << doIndent(_indent + 1) << "</sensors>" << std::endl;
      }
    }
    _ofs << doIndent(_indent) << "</zone>" << std::endl;
  } // zoneToXML

  void dsMeterToXML(const boost::shared_ptr<DSMeter> _pDSMeter, std::ofstream& _ofs, const int _indent) {
    _ofs <<  doIndent(_indent) << "<dsMeter id=\"" + dsuid2str(_pDSMeter->getDSID()) << "\">" << std::endl;
    if(!_pDSMeter->getName().empty()) {
      _ofs << doIndent(_indent + 1) << "<name>" + XMLStringEscape(_pDSMeter->getName()) << "</name>" << std::endl;
    }

    _ofs << doIndent(_indent + 1) << "<datamodelHash>" <<
                                        intToString(_pDSMeter->getDatamodelHash()) <<
                                     "</datamodelHash>" << std::endl;

    _ofs << doIndent(_indent + 1) << "<datamodelModification>" <<
                                        intToString(_pDSMeter->getDatamodelModificationCount()) <<
                                     "</datamodelModification>" << std::endl;

    _ofs << doIndent(_indent + 1) << "<deviceType>" <<
                                        intToString(_pDSMeter->getBusMemberType()) <<
                                     "</deviceType>" << std::endl;

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

      saveValidatedXML(tmpOut, _fileName);
    }
  } // writeConfigurationToXML
}
