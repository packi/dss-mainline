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
#include "src/property-parser.h"
#include "src/ds485types.h"
#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/zone.h"
#include "src/model/modulator.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/modelconst.h"
#include "propertysystem_common_paths.h"

namespace dss {

  const char WindProtectionClass[] = "WindProtectionClass";
  const char CardinalDirection[] = "CardinalDirection";
  const char Floor[] = "Floor";
  const char VdcDevice[] = "VdcDevice";

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
    const char *oemConfigLink = NULL;
    const char *configLocked = NULL;
    const char *valve = NULL;
    const char *cardinalDirection = NULL;
    const char *windProtectionClass = NULL;
    const char *floorString = NULL;
    const char *vdcDevice = NULL;
    const char *pairedDevices = NULL;

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
      } else if (strcmp(_attrs[i], "oemConfigLink") == 0) {
        oemConfigLink = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "configurationLocked") == 0) {
        configLocked = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "valveType") == 0) {
        valve = _attrs[i + 1];
      } else if (strcmp(_attrs[i], "pairedDevices") == 0) {
        pairedDevices = _attrs[i + 1];
      } else if (strcmp(_attrs[i], VdcDevice) == 0) {
        vdcDevice = _attrs[i + 1];
      } else if (strcmp(_attrs[i], WindProtectionClass) == 0) {
        windProtectionClass = _attrs[i + 1];
      } else if (strcmp(_attrs[i], CardinalDirection) == 0) {
        cardinalDirection = _attrs[i + 1];
      } else if (strcmp(_attrs[i], Floor) == 0) {
        floorString = _attrs[i + 1];
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

    DeviceOEMState_t oemEanState = DEVICE_OEM_UNKNOWN;
    if (oemState != NULL) {
      oemEanState = m_tempDevice->getOemStateFromString(oemState);
      // device was in the loading state when dSS shut down, repeat the
      // readout
      if (oemEanState == DEVICE_OEM_LOADING) {
        oemEanState = DEVICE_OEM_UNKNOWN;
      }
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

      DeviceOEMState_t productInfoState = DEVICE_OEM_UNKNOWN;
      if (oemProdState != NULL) {
        productInfoState = m_tempDevice->getOemStateFromString(oemProdState);
      }

      if (productInfoState == DEVICE_OEM_VALID) {
        if (oemProdName != NULL && oemProdIcon != NULL && oemProdURL != NULL && oemConfigLink != NULL) {
          m_tempDevice->setOemProductInfo(oemProdName, oemProdIcon, oemProdURL, oemConfigLink);
        }
        m_tempDevice->setOemProductInfoState(productInfoState);
      }
    }
    m_tempDevice->setOemInfoState(oemEanState);

    CardinalDirection_t dir;
    if (cardinalDirection && parseCardinalDirection(cardinalDirection, &dir)) {
      m_tempDevice->setCardinalDirection(dir, true);
    }

    WindProtectionClass_t protection;
    if (windProtectionClass &&
        convertWindProtectionClass(strToInt(windProtectionClass), &protection)) {
      m_tempDevice->setWindProtectionClass(protection, true);
    }

    if (floorString != NULL) {
      m_tempDevice->setFloor(strToUIntDef(floorString, 0));
    }

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

    if (pairedDevices != NULL) {
      try {
        int p = strToIntDef(pairedDevices, -1);
        if (p > 0) {
          m_tempDevice->setPairedDevices(p);
        }
      } catch(std::invalid_argument&) {}
    }

    if (vdcDevice != NULL) {
      m_tempDevice->setVdcDevice(true);
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

  void ModelPersistence::parseHeatingConfig(const char *_name, const char **_attrs) {
    if (m_tempZone == NULL) {
      return;
    }

    if (strcmp(_name, "heatingConfig") == 0) {
      ZoneHeatingProperties_t config;
      for (int i = 0; _attrs[i]; i += 2)
      {
        const char *tempVal = 0;
        tempVal = _attrs[i + 1];
        if (strcmp(_attrs[i], "Mode") == 0) {
          config.m_mode = static_cast<HeatingControlMode>(strToUIntDef(tempVal, 0));
        } else if (strcmp(_attrs[i], "Kp") == 0) {
          config.m_Kp = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "Ts") == 0) {
          config.m_Ts = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "Ti") == 0) {
          config.m_Ti = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "Kd") == 0) {
          config.m_Kd = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "Imin") == 0) {
          config.m_Imin = strToIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "IMax") == 0) {
          config.m_Imax = strToIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "Ymin") == 0) {
          config.m_Ymin = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "Ymax") == 0) {
          config.m_Ymax = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "AntiWindup") == 0) {
          config.m_AntiWindUp = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "FloorWarm") == 0) {
          config.m_KeepFloorWarm = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "State") == 0) {
          config.m_HeatingControlState = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "SourceZone") == 0) {
          config.m_HeatingMasterZone = strToIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "Offset") == 0) {
          config.m_CtrlOffset = strToIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "EmergencyVal") == 0) {
          config.m_EmergencyValue = strToUIntDef(tempVal, 0);
        } else if (strcmp(_attrs[i], "ManualVal") == 0) {
          config.m_ManualValue = strToIntDef(tempVal, 0);
        }
      }
      m_tempZone->setHeatingProperties(config);
    }

    if (strcmp(_name, "temperatureSetpoints") == 0) {
      ZoneHeatingProperties_t config = m_tempZone->getHeatingProperties();
      std::vector<std::string> valNames;
      valNames.reserve(HeatingOperationModeIDMax);

      for (int j = 0; j <= HeatingOperationModeIDMax; ++j) {
        valNames.emplace_back(ds::str("val", j));
      }

      for (int i = 0; _attrs[i]; i += 2) {
        const char *tempVal = _attrs[i + 1];

        for (int j = 0; j <= HeatingOperationModeIDMax; ++j) {
          if (strcmp(_attrs[i], valNames[j].c_str()) == 0) {
            config.m_TeperatureSetpoints[j] = strToDouble(tempVal, config.m_TeperatureSetpoints[j]);
            break;
          }
        }
      }

      m_tempZone->setHeatingProperties(config);
    }

    if (strcmp(_name, "controlValues") == 0) {
      ZoneHeatingProperties_t config = m_tempZone->getHeatingProperties();

      for (int i = 0; _attrs[i]; i += 2) {
        const char *tempVal = _attrs[i + 1];

        for (int j = 0; j <= HeatingOperationModeIDMax; ++j) {
          std::string attrName = ds::str("val", j);

          if (strcmp(_attrs[i], attrName.c_str()) == 0) {
            config.m_FixedControlValues[j] = strToDouble(tempVal, config.m_FixedControlValues[j]);
            break;
          }
        }
      }

      m_tempZone->setHeatingProperties(config);
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

    if (m_tempZone->getID() > 0 && isAppUserGroup(groupID)) {
      return;
    }

    m_tempGroup.reset();
    m_tempGroup = m_tempZone->getGroup(groupID);
    if (m_tempGroup == NULL) {
      if (m_tempZone->getID() == 0 && isAppUserGroup(groupID)) {
        m_tempGroup.reset(new Cluster(groupID, m_Apartment));
      } else {
        m_tempGroup.reset(new Group(groupID, m_tempZone));
      }
      m_tempZone->addGroup(m_tempGroup);
    }
    if (grName) {
      m_tempGroup->setName(grName);
    }
    m_tempGroup->setIsValid(true);
  }

  void ModelPersistence::parseCluster(const char *_name, const char **_attrs) {
    if (strcmp(_name, "cluster") != 0) {
      return;
    }

    const char *id = NULL;
    for (int i = 0; _attrs[i]; i += 2) {
      if (strcmp(_attrs[i], "id") == 0) {
        id = _attrs[i + 1];
      }
    }

    if (id == NULL) {
      return;
    }
    int groupID = strToIntDef(id, -1);
    if (groupID == -1) {
      return;
    }

    m_tempCluster.reset();
    try {
      m_tempCluster = boost::dynamic_pointer_cast<Cluster>(m_Apartment.getGroup(groupID));
    } catch (ItemNotFoundException &e) {
      m_tempCluster.reset(new Cluster(groupID, m_Apartment));
      m_Apartment.getZone(0)->addGroup(m_tempCluster);
    }

    m_tempCluster->setIsValid(true);
  }

  void ModelPersistence::parseLockedScenes(const char *_name, const char **_attrs) {
    if (m_tempCluster == NULL) {
      return;
    }

    if (strcmp(_name, "lockedScene") != 0) {
      return;
    }

    const char *sid = getSingleAttribute("id", _attrs);
    if (sid == NULL) {
      return;
    }

    int snum = strToIntDef(sid, -1);
    m_tempCluster->addLockedScene(snum);
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

    SensorType _sensorType;
    try {
      _sensorType = static_cast<SensorType>(strToInt(sensorType));
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

    MainZoneSensor_t ms;
    ms.m_DSUID = _dsuid;
    ms.m_sensorType = _sensorType;
    ms.m_sensorIndex = _sensorIndex;

    try {
      m_tempZone->setSensor(ms);
    } catch(std::runtime_error &ex) {
      Logger::getInstance()->log("ModelPersistence: could not assign zone sensor " + std::string(dsuid) + ": " + ex.what(), lsError);
    }
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

      // level 1 supports only <apartment>, <devices>, <zones>, <dsMeters>, <clusters>
      if (m_level == 1) {
        if (strcmp(_name, "apartment") == 0) {
          m_state = ps_apartment;
        } else if (strcmp(_name, "devices") == 0) {
          m_state = ps_device;
        } else if (strcmp(_name, "zones") == 0) {
          m_state = ps_zone;
        } else if (strcmp(_name, "clusters") == 0) {
          m_state = ps_cluster;
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

      // level 2 supports <device>, <zone>, <dsMeter>, <cluster>
      if (m_level == 2) {
        if ((m_state == ps_apartment) && (strcmp(_name, "name") == 0)) {
          m_expectString = true;
        } else if (m_state == ps_device) {
          parseDevice(_name, _attrs);
        } else if (m_state == ps_zone) {
          parseZone(_name, _attrs);
        } else if (m_state == ps_meter) {
          parseMeter(_name, _attrs);
        } else if (m_state == ps_cluster) {
          parseCluster(_name, _attrs);
        }
      // level 3 supports <name>, <properties>, <groups>, <datamodelHash>,
      // <datamodelModification>, <sensors>, <lockedScenes>
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
        } else if ((m_state == ps_zone) && (strcmp(_name, "heatingConfig") == 0)) {
          parseHeatingConfig(_name,_attrs);
        } else if (m_state == ps_meter) {
          m_expectString = true;
        } else if ((m_state == ps_device) &&
                   (strcmp(_name, "properties") == 0)) {
          m_state = ps_properties;
          if (m_tempDevice != NULL) {
            m_propParser->reset(m_tempDevice->getPropertyNode(), true);
            m_propParser->elementStartCb(_name, _attrs);
          }
        } else if ((m_state == ps_cluster) &&
              ((strcmp(_name, "name") == 0) ||
               (strcmp(_name, "associatedSet") == 0) ||
               (strcmp(_name, "color") == 0) ||
               (strcmp(_name, "location") == 0) ||
               (strcmp(_name, "protectionClass") == 0) ||
               (strcmp(_name, "floor") == 0) ||
               (strcmp(_name, "configurationLocked") == 0) ||
               (strcmp(_name, "automatic") == 0) ||
               (strcmp(_name, "configuration") == 0))) {
            m_expectString = true;
        } else if ((m_state == ps_cluster) &&
            (strcmp(_name, "lockedScenes") == 0)) {
          m_state = ps_lockedScenes;
        }
      // level 4 supports <property>, <group>, <sensor>, <lockedScene>, <temperatureSetpoints>, <controlValues>
      } else if (m_level == 4) {
        if (m_state == ps_group) {
          parseGroup(_name, _attrs);
        } else if (m_state == ps_sensor) {
          parseSensor(_name, _attrs);
        } else if (m_state == ps_lockedScenes) {
          parseLockedScenes(_name, _attrs);
        } else if ((m_state == ps_zone) &&
            ((strcmp(_name, "temperatureSetpoints") == 0) || (strcmp(_name, "controlValues") == 0))) {
          parseHeatingConfig(_name,_attrs);
        }
      // level 5 supports <property>, <value>, <name>, <scenes>, <associatedSet>, <color>, <configuration>
      } else if (m_level == 5) {
        if ((m_state == ps_group) &&
            ((strcmp(_name, "name") == 0) ||
             (strcmp(_name, "color") == 0) ||
             (strcmp(_name, "associatedSet") == 0) ||
             (strcmp(_name, "configuration") == 0))) {
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
              } else if (strcmp(_name ,"controllerMode") == 0) {
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
                m_tempMeter->setBusMemberType(static_cast<BusMemberDevice_t>(strToInt(m_chardata)));
              }
            } else if ((m_state == ps_cluster) && (m_tempCluster != NULL)) {
              if (strcmp(_name, "name") == 0) {
                m_tempCluster->setName(m_chardata);
              } else if (strcmp(_name, "associatedSet") == 0) {
                m_tempCluster->setAssociatedSet(m_chardata);
              } else if (strcmp(_name, "color") == 0) {
                m_tempCluster->setApplicationType(static_cast<ApplicationType>(strToUIntDef(m_chardata, 0)));
              } else if (strcmp(_name, "location") == 0) {
                CardinalDirection_t location = cd_none;
                if (parseCardinalDirection(m_chardata, &location)) {
                  m_tempCluster->setLocation(location);
                }
              } else if (strcmp(_name, "protectionClass") == 0) {
                WindProtectionClass_t protectionClass = wpc_none;
                if (convertWindProtectionClass(strToUIntDef(m_chardata, 0), &protectionClass)) {
                  m_tempCluster->setProtectionClass(protectionClass);
                }
              } else if (strcmp(_name, "floor") == 0) {
                m_tempCluster->setFloor(strToUIntDef(m_chardata, 0));
              } else if (strcmp(_name, "configurationLocked") == 0) {
                bool isConfigurationLocked = false;
                try {
                  int p = strToUInt(m_chardata);
                  isConfigurationLocked = p > 0;
                } catch (std::invalid_argument&) {}
                m_tempCluster->setConfigurationLocked(isConfigurationLocked);
              } else if (strcmp(_name, "automatic") == 0) {
                bool isAutomatic = false;
                try {
                  int p = strToUInt(m_chardata);
                  isAutomatic = p > 0;
                } catch (std::invalid_argument&) {}
                m_tempCluster->setAutomatic(isAutomatic);
              } else if (strcmp(_name, "configuration") == 0) {
                m_tempCluster->setApplicationConfiguration(strToUIntDef(m_chardata, 0));
              }
            }
          }
          if ((m_state == ps_group) && (strcmp(_name, "groups") == 0)) {
            m_state = ps_zone;
          } else if ((m_state == ps_sensor) && (strcmp(_name, "sensors") == 0)) {
            m_state = ps_zone;
          } else if ((m_state == ps_lockedScenes) && (strcmp(_name, "lockedScenes") == 0)) {
            m_state = ps_cluster;
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
            // the application type can be overwritten by configuration only for user groups not the default ones.
            if (!isDefaultGroup(m_tempGroup->getID()) && !isGlobalAppDsGroup(m_tempGroup->getID()) ) {
              m_tempGroup->setApplicationType(static_cast<ApplicationType>(strToUIntDef(m_chardata, 0)));
            }
          } else if (strcmp(_name, "configuration") == 0) {
            m_tempGroup->setApplicationConfiguration(strToUIntDef(m_chardata, 0));
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
    m_tempCluster.reset();
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

    // #13192: always clear out name for zone 0
    boost::shared_ptr<Zone> z = m_Apartment.getZone(0);
    if (z) {
      z->setName("");
    }
    return;
  } // readConfigurationFromXML

  std::ostream& addAttribute(std::ostream &_ofs, const std::string &_name,
                             const std::string &_value) {
    return _ofs << " " << _name << "=\"" << _value << "\"";
  }

  std::ostream& addElementSimple(std::ostream &_ofs, int _indent, const std::string &_name, const std::string &_value) {
    return _ofs << doIndent(_indent) << "<" << _name << ">" << XMLStringEscape(_value) << "</" << _name << ">" << std::endl;
  }

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
    if (_pDevice->getCardinalDirection() != cd_none) {
      addAttribute(_ofs, CardinalDirection, toString(_pDevice->getCardinalDirection()));
    }
    if (_pDevice->getWindProtectionClass() != wpc_none) {
      addAttribute(_ofs, WindProtectionClass, intToString(_pDevice->getWindProtectionClass()));
    }
    if (_pDevice->getFloor() != 0) {
      addAttribute(_ofs, Floor, intToString(_pDevice->getFloor()));
    }
    if ((_pDevice->getOemInfoState() == DEVICE_OEM_NONE) ||
        (_pDevice->getOemInfoState() == DEVICE_OEM_UNKNOWN) ||
        (_pDevice->getOemInfoState() == DEVICE_OEM_LOADING)) {
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
        _ofs << " oemConfigLink=\"" << XMLStringEscape(_pDevice->getOemConfigLink()) << "\"";
      }
    }
    _ofs << " configurationLocked=\"" << (_pDevice->isConfigLocked() ? "1" : "0") << "\"";

    if ((_pDevice->getDeviceType() == DEVICE_TYPE_TNY) ||
        ((_pDevice->getDeviceType() == DEVICE_TYPE_SK) &&
         (_pDevice->getDeviceNumber() == 204))) {
      _ofs << " pairedDevices=\"" << intToString(_pDevice->getPairedDevices()) << "\"";
    }

    if (_pDevice->isVdcDevice()) {
      addAttribute(_ofs, VdcDevice, "1");
    }
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
    bool headerWritten = false;
    bool sceneTagWritten = false;

    // in case of GA we always need to serialize the group even if it do not have custom scenes
    if (isGlobalAppGroup(_pGroup->getID())) {
      headerWritten = true;
      _ofs << doIndent(_indent) << "<group id=\"" << intToString(_pGroup->getID()) << "\">" << std::endl;
      if (!_pGroup->getName().empty()) {
        addElementSimple(_ofs, _indent + 1, "name", _pGroup->getName());
      }
      addElementSimple(_ofs, _indent + 1, "color", intToString(static_cast<int>(_pGroup->getApplicationType())));
      addElementSimple(_ofs, _indent + 1, "configuration", uintToString(_pGroup->getApplicationConfiguration(), true));
      if (!_pGroup->getAssociatedSet().empty()) {
        addElementSimple(_ofs, _indent + 1, "associatedSet", _pGroup->getAssociatedSet());
      }
    }

    for (int iScene = 0; iScene < MaxSceneNumber; iScene++) {
      std::string name = _pGroup->getSceneName(iScene);
      if (!name.empty()) {
        if (!headerWritten) {
          headerWritten = true;
          _ofs << doIndent(_indent) << "<group id=\"" << intToString(_pGroup->getID()) << "\">" << std::endl;
          if (!_pGroup->getName().empty()) {
            addElementSimple(_ofs, _indent + 1, "name", _pGroup->getName());
          }
          addElementSimple(_ofs, _indent + 1, "color", intToString(static_cast<int>(_pGroup->getApplicationType())));
          addElementSimple(_ofs, _indent + 1, "configuration", uintToString(_pGroup->getApplicationConfiguration(), true));
          if (!_pGroup->getAssociatedSet().empty()) {
            addElementSimple(_ofs, _indent + 1, "associatedSet", _pGroup->getAssociatedSet());
          }
        }

        if (!sceneTagWritten) {
          sceneTagWritten = true;
          _ofs << doIndent(_indent + 1) << "<scenes>" << std::endl;
        }

        _ofs << doIndent(_indent + 2) << "<scene id=\"" << intToString(iScene) << "\">" << std::endl;
        _ofs << doIndent(_indent + 3) << "<name>" << XMLStringEscape(name) << "</name>" << std::endl;
        _ofs << doIndent(_indent + 2) << "</scene>" << std::endl;
      }
    }

    if (sceneTagWritten) {
      _ofs << doIndent(_indent + 1) << "</scenes>" << std::endl;
    }

    if (headerWritten) {
      _ofs << doIndent(_indent) << "</group>" << std::endl;
    }
  } // groupToXML

  void clusterToXML(boost::shared_ptr<Cluster> _pCluster, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<cluster id=\"" << intToString(_pCluster->getID()) << "\">" << std::endl;

    if (!_pCluster->getName().empty()) {
      addElementSimple(_ofs, _indent + 1, "name", _pCluster->getName());
    }
    if (!_pCluster->getAssociatedSet().empty()) {
      addElementSimple(_ofs, _indent + 1, "associatedSet", _pCluster->getAssociatedSet());
    }
    addElementSimple(_ofs, _indent + 1, "color", intToString(static_cast<int>(_pCluster->getApplicationType())));
    addElementSimple(_ofs, _indent + 1, "location", toString(_pCluster->getLocation()));
    addElementSimple(_ofs, _indent + 1, "protectionClass", intToString(static_cast<int>(_pCluster->getProtectionClass())));
    addElementSimple(_ofs, _indent + 1, "floor", intToString(_pCluster->getFloor()));
    addElementSimple(_ofs, _indent + 1, "configurationLocked", (_pCluster->isConfigurationLocked() ? "1" : "0"));
    addElementSimple(_ofs, _indent + 1, "automatic", (_pCluster->isAutomatic() ? "1" : "0"));
    addElementSimple(_ofs, _indent + 1, "configuration", uintToString(_pCluster->getApplicationConfiguration(), true));
    _ofs << doIndent(_indent + 1) << "<lockedScenes>" << std::endl;
    const std::vector<int> lockedScenes = _pCluster->getLockedScenes();
    for (unsigned int iScene = 0; iScene < lockedScenes.size(); iScene++) {
      _ofs << doIndent(_indent + 2) << "<lockedScene id=\"" << intToString(lockedScenes[iScene]) << "\" />" << std::endl;
    }
    _ofs << doIndent(_indent + 1) << "</lockedScenes>" << std::endl;

    _ofs << doIndent(_indent) << "</cluster>" << std::endl;
  } // clusterToXML

  void zoneSensorToXML(const MainZoneSensor_t &_zoneSensor, std::ofstream& _ofs, const int _indent)
  {
    _ofs << doIndent(_indent) << "<sensor dsuid=\""
         << dsuid2str(_zoneSensor.m_DSUID) << "\""
         << " sensorType=\"" << intToString(static_cast<int>(_zoneSensor.m_sensorType)) << "\""
         << " sensorIndex=\"" << intToString(_zoneSensor.m_sensorIndex)  << "\"/>"
         << std::endl;
  } // zoneSensorToXML

  void heatingConfigToXML(const ZoneHeatingProperties_t& heatingConfig, std::ofstream& _ofs, const int _indent)
  {
    _ofs << doIndent(_indent) << "<heatingConfig";
    addAttribute(_ofs, "Mode", uintToString(static_cast<uint8_t>(heatingConfig.m_mode)));
    addAttribute(_ofs, "Kp", uintToString(heatingConfig.m_Kp));
    addAttribute(_ofs, "Ts", uintToString(heatingConfig.m_Ts));
    addAttribute(_ofs, "Ti", uintToString(heatingConfig.m_Ti));
    addAttribute(_ofs, "Kd", uintToString(heatingConfig.m_Kd));
    addAttribute(_ofs, "Imin", intToString(heatingConfig.m_Imin));
    addAttribute(_ofs, "IMax", intToString(heatingConfig.m_Imax));
    addAttribute(_ofs, "Ymin", uintToString(heatingConfig.m_Ymin));
    addAttribute(_ofs, "Ymax", uintToString(heatingConfig.m_Ymax));
    addAttribute(_ofs, "AntiWindup", uintToString(heatingConfig.m_AntiWindUp));
    addAttribute(_ofs, "FloorWarm", uintToString(heatingConfig.m_KeepFloorWarm));
    addAttribute(_ofs, "State", uintToString(heatingConfig.m_HeatingControlState));
    addAttribute(_ofs, "SourceZone", uintToString(heatingConfig.m_HeatingMasterZone));
    addAttribute(_ofs, "Offset",  intToString(heatingConfig.m_CtrlOffset));
    addAttribute(_ofs, "EmergencyVal", uintToString(heatingConfig.m_EmergencyValue));
    addAttribute(_ofs, "ManualVal", uintToString(heatingConfig.m_ManualValue));
    _ofs << ">" << std::endl;

    _ofs << doIndent(_indent+1) << "<temperatureSetpoints";
    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      addAttribute(_ofs, ds::str("val", i), ds::str(heatingConfig.m_TeperatureSetpoints[i]));
    }
    _ofs << "/>" << std::endl;

    _ofs << doIndent(_indent+1) << "<controlValues";
    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      addAttribute(_ofs, ds::str("val", i), ds::str(heatingConfig.m_FixedControlValues[i]));
    }
    _ofs << "/>" << std::endl;

    _ofs << doIndent(_indent) << "</heatingConfig>" << std::endl;
  } // heatingConfigToXML

  void zoneToXML(boost::shared_ptr<Zone> _pZone, std::ofstream& _ofs, const int _indent) {
    _ofs << doIndent(_indent) << "<zone id=\"" << intToString(_pZone->getID()) << "\">" << std::endl;
    if(!_pZone->getName().empty()) {
      _ofs << doIndent(_indent + 1) << "<name>" << XMLStringEscape(_pZone->getName()) << "</name>" << std::endl;
    }

    _ofs << doIndent(_indent + 1) << "<groups>" << std::endl;
    // store real user-groups per zone
    foreach(boost::shared_ptr<Group> pGroup, _pZone->getGroups()) {
      if (_pZone->getID() == 0 && isAppUserGroup(pGroup->getID())) {
        continue;  //This is cluster, do not serialize with groups
      }
      groupToXML(pGroup, _ofs, _indent + 2);
    }
    _ofs << doIndent(_indent + 1) << "</groups>" << std::endl;

    // Zone sensors
    auto&& slist = _pZone->getAssignedSensors();
    if ( !slist.empty() ) {
      _ofs << doIndent(_indent + 1) << "<sensors>" << std::endl;
      foreach (auto&& devSensor,  slist) {
        zoneSensorToXML(devSensor, _ofs, _indent+2);
      }
      _ofs << doIndent(_indent + 1) << "</sensors>" << std::endl;
    }

    // heating controller
    if (_pZone->getID() != 0) {
      if (_pZone->isHeatingPropertiesValid()) {
        const ZoneHeatingProperties_t& config =_pZone->getHeatingProperties();
        heatingConfigToXML(config, _ofs, _indent+1);
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
    // The correct lock order is: apartment -> property
    boost::recursive_mutex::scoped_lock apartment_lock(m_Apartment.getMutex());
    boost::recursive_mutex::scoped_lock lock(PropertyNode::m_GlobalMutex);

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

      // clusters
      ofs << doIndent(indent) << "<clusters>" << std::endl;
      // store unique "apartment user-groups" in zone 0
      foreach(boost::shared_ptr<Cluster> pCluster, m_Apartment.getClusters()) {
        clusterToXML(pCluster, ofs, indent + 1);
      }
      ofs << doIndent(indent) << "</clusters>" << std::endl;

      // dsMeters
      ofs << doIndent(indent) << "<dsMeters>" << std::endl;
      foreach(boost::shared_ptr<DSMeter> pDSMeter, m_Apartment.getDSMetersAll()) {
        dsMeterToXML(pDSMeter, ofs, indent + 1);
      }
      ofs << doIndent(indent) << "</dsMeters>" << std::endl;

      indent--;
      ofs << doIndent(indent) << "</config>" << std::endl;

      ofs.close();

      syncFile(tmpOut);

      saveValidatedXML(tmpOut, _fileName);
    }
    Logger::getInstance()->log("Finished writing apartment config to '" + _fileName + "'", lsDebug);
  } // writeConfigurationToXML
}
