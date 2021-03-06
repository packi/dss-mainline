/*
    Copyright (c) 2010,2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "apartment.h"

#include <ds/log.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

#include "src/businterface.h"
#include "src/model/modelconst.h"
#include "src/logger.h"
#include "src/propertysystem.h"
#include "src/foreach.h"
#include "src/dss.h"
#include "src/event.h"

#include "src/model/busscanner.h"
#include "src/model/modelevent.h"
#include "src/model/modelmaintenance.h"

#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/set.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/modulator.h"
#include "src/model/state.h"
#include "src/metering/metering.h"

namespace dss {

  //================================================== Apartment

  Apartment::Apartment(DSS* dss)
  : m_dss(dss),
    m_pBusInterface(NULL),
    m_pModelMaintenance(NULL),
    m_pPropertySystem(NULL),
    m_pMetering(NULL)
  {
    // create default (broadcast) zone
    boost::shared_ptr<Zone> zoneZero = allocateZone(0);
    zoneZero->setIsPresent(true);
  } // ctor

  Apartment::~Apartment() {
    m_Zones.clear();
    m_DSMeters.clear();
    m_Devices.clear();
    m_States.clear();

    m_pPropertyNode.reset();
    m_pBusInterface = NULL;
    m_pModelMaintenance = NULL;
    m_pPropertySystem = NULL;
    m_pMetering = NULL;
  } // dtor

  void Apartment::addDefaultGroupsToZone(boost::shared_ptr<Zone> _zone) {
    boost::shared_ptr<Group> grp = boost::make_shared<Group>(GroupIDBroadcast, _zone);
    grp->setName("broadcast");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDYellow, _zone));
    grp->setName("yellow");
    grp->setApplicationType(ApplicationType::Lights);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDGray, _zone));
    grp->setName("gray");
    grp->setApplicationType(ApplicationType::Blinds);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDHeating, _zone));
    grp->setName("heating");
    grp->setApplicationType(ApplicationType::Heating);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDCyan, _zone));
    grp->setName("cyan");
    grp->setApplicationType(ApplicationType::Audio);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDViolet, _zone));
    grp->setName("magenta");
    grp->setApplicationType(ApplicationType::Video);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDRed, _zone));
    grp->setName("reserved1");
    grp->setApplicationType(ApplicationType::None);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDGreen, _zone));
    grp->setName("reserved2");
    grp->setApplicationType(ApplicationType::None);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDBlack, _zone));
    grp->setName("black");
    grp->setApplicationType(ApplicationType::Joker);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDCooling, _zone));
    grp->setName("cooling");
    grp->setApplicationType(ApplicationType::Cooling);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDVentilation, _zone));
    grp->setName("ventilation");
    grp->setApplicationType(ApplicationType::Ventilation);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDWindow, _zone));
    grp->setName("window");
    grp->setApplicationType(ApplicationType::Window);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDRecirculation, _zone));
    grp->setName("recirculation");
    grp->setApplicationType(ApplicationType::Recirculation);
    grp->setIsValid(true);
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDControlTemperature, _zone));
    grp->setName("controltemperature");
    grp->setApplicationType(ApplicationType::ControlTemperature);
    grp->setIsValid(true);
    _zone->addGroup(grp);
  } // addDefaultGroupsToZone

  boost::shared_ptr<Device> Apartment::tryGetDeviceByDSID(const dsuid_t dsid) const {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      if (dev->getDSID() == dsid) {
        return dev;
      }
    }
    return boost::shared_ptr<Device>();
  }

  boost::shared_ptr<Device> Apartment::getDeviceByDSID(const dsuid_t dsid) const {
    auto device = tryGetDeviceByDSID(dsid);
    if (!device) {
      throw ItemNotFoundException(dsuid2str(dsid));
    }
    return device;
  }

  boost::shared_ptr<Device> Apartment::getDeviceByName(const std::string& _name) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      if(dev->getName() == _name) {
        return dev;
      }
    }
    throw ItemNotFoundException(_name);
  } // getDeviceByName

  Set Apartment::getDevices() const {
    std::vector<DeviceReference> devs;
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      devs.push_back(DeviceReference(dev, this));
    }

    return Set(devs);
  } // getDevices

  boost::shared_ptr<Zone> Apartment::getZone(const std::string& _zoneName) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach(boost::shared_ptr<Zone> zone, m_Zones) {
      if(zone->getName() == _zoneName) {
        return zone;
      }
    }
    throw ItemNotFoundException(_zoneName);
  } // getZone(name)

  boost::weak_ptr<Zone> Apartment::tryGetZone(const int id) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach (auto&& zone, m_Zones) {
      if(zone->getID() == id) {
        return zone;
      }
    }
    return boost::weak_ptr<Zone>();
  }

  boost::shared_ptr<Zone> Apartment::getZone(const int id) {
    if (auto&& zone = tryGetZone(id).lock()) {
      return zone;
    }
    throw ItemNotFoundException(intToString(id));
  }

  std::vector<boost::shared_ptr<Zone> > Apartment::getZones() {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    std::vector<boost::shared_ptr<Zone> > result = m_Zones;
    return result;
  } // getZones

  boost::shared_ptr<DSMeter> Apartment::getDSMeter(const std::string& _modName) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if(dsMeter->getName() == _modName) {
        return dsMeter;
      }
    }
    throw ItemNotFoundException(_modName);
  } // getDSMeter(name)

  boost::shared_ptr<DSMeter> Apartment::getDSMeterByDSID(const dsuid_t _dsid) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if (dsMeter->getDSID() == _dsid) {
        return dsMeter;
      }
    }
    throw ItemNotFoundException(dsuid2str(_dsid));
  } // getDSMeterByDSID

  std::vector<boost::shared_ptr<DSMeter> > Apartment::getDSMeters() {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    std::vector<boost::shared_ptr<DSMeter> > result;
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      // TODO(someday): remove the filter and move this logic to ui - use HasDevices?
      // we should not return vDSMs as they are not currently filtered in ui
      if (dsMeter->getBusMemberType() != BusMember_vDSM) {
        result.push_back(dsMeter);
      }
    }
    return result;
  } // getDSMeters

  std::vector<boost::shared_ptr<DSMeter> > Apartment::getDSMetersAll() {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    std::vector<boost::shared_ptr<DSMeter> > result = m_DSMeters;
    return result;
  } // getAllDSMeters

  // Group queries
  boost::shared_ptr<Group> Apartment::getGroup(const std::string& _name) {
    boost::shared_ptr<Group> pResult = getZone(0)->getGroup(_name);
    if(pResult != NULL) {
      return pResult;
    }
    throw ItemNotFoundException(_name);
  } // getGroup(name)

  boost::shared_ptr<Group> Apartment::getGroup(const int _id) {
    boost::shared_ptr<Group> pResult = getZone(0)->getGroup(_id);
    if(pResult != NULL) {
      return pResult;
    }
    throw ItemNotFoundException(intToString(_id));
  } // getGroup(id)

  boost::shared_ptr<Cluster> Apartment::getCluster(const int _id) {
    if (isAppUserGroup(_id)) {
      boost::shared_ptr<Cluster> pResult = boost::dynamic_pointer_cast<Cluster>(getZone(0)->getGroup(_id));
      if(pResult != NULL) {
        return pResult;
      }
    }
    throw ItemNotFoundException(intToString(_id));
  } // getGroup(id)

  std::vector<boost::shared_ptr<Cluster> > Apartment::getClusters() {
    std::vector<boost::shared_ptr<Cluster> > result;
    foreach(boost::shared_ptr<Group> pGroup, getZone(0)->getGroups()) {
      if (isAppUserGroup(pGroup->getID())) {
        result.push_back(boost::dynamic_pointer_cast<Cluster>(pGroup));
      }
    }
    return result;
  }

  boost::shared_ptr<Cluster> Apartment::getEmptyCluster() {
    // find a group slot with unassigned state machine id
    foreach (boost::shared_ptr<Cluster> pCluster, getClusters()) {
      if (pCluster->getApplicationType() == ApplicationType::None) {
        return pCluster;
      }
    }
    return boost::shared_ptr<Cluster> ();
  }

  std::vector<boost::shared_ptr<Group> > Apartment::getGlobalApps() {
    std::vector<boost::shared_ptr<Group> > result;
    foreach(boost::shared_ptr<Group> pGroup, getZone(0)->getGroups()) {
      if (isGlobalAppGroup(pGroup->getID())) {
        result.push_back(pGroup);
      }
    }
    return result;
  }

  boost::shared_ptr<Device> Apartment::allocateDevice(const dsuid_t _dsid) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    boost::shared_ptr<Device> pResult;
    // search for existing device
    foreach(boost::shared_ptr<Device> device, m_Devices) {
      if (device->getDSID() == _dsid) {
        pResult = device;
        break;
      }
    }

    if(pResult == NULL) {
      pResult.reset(new Device(_dsid, this));
      pResult->setFirstSeen(DateTime());
      m_Devices.push_back(pResult);
    }
    DeviceReference devRef(pResult, this);
    getZone(0)->addDevice(devRef);
    pResult->publishToPropertyTree();
    return pResult;
  } // allocateDevice

  boost::shared_ptr<DSMeter> Apartment::allocateDSMeter(const dsuid_t _dsid) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if (dsMeter->getDSID() == _dsid) {
        return dsMeter;
      }
    }

    boost::shared_ptr<DSMeter> pResult = boost::make_shared<DSMeter>(_dsid, this);
    m_DSMeters.push_back(pResult);

    return pResult;
  } // allocateDSMeter

  boost::shared_ptr<Zone> Apartment::allocateZone(int _zoneID) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    boost::shared_ptr<Zone> result;
    foreach(boost::shared_ptr<Zone> zone, m_Zones) {
      if(zone->getID() == _zoneID) {
        result = zone;
        break;
      }
    }

    if(result == NULL) {
      result.reset(new Zone(_zoneID, this));
      result->publishToPropertyTree();
      addDefaultGroupsToZone(result);
      m_Zones.push_back(result);

      if (_zoneID == 0) {
        for (int i = GroupIDAppUserMin; i <= GroupIDAppUserMax; ++i) {
          boost::shared_ptr<Cluster> pCluster = boost::make_shared<Cluster>(i, boost::ref<Apartment>(*this));
          result->addGroup(pCluster);
        }

        // pre-create apartment ventilation group
        boost::shared_ptr<Group> group;
        group.reset(new Group(GroupIDGlobalAppDsVentilation, result));
        group->setName("apartmentVentilation");
        group->setApplicationType(ApplicationType::ApartmentVentilation);
        group->setIsValid(true);  // TODO(soon): this may not be needed for AV (maybe it is only valid when devices are present)
        result->addGroup(group);

        // TODO(soon): remove again before R1705
//        group.reset(new Group(GroupIDGlobalAppDsRecirculation, result));
//        group->setName("apartmentRecirculation");
//        group->setApplicationType(ApplicationType::ApartmentRecirculation);
//        group->setIsValid(true);  // TODO(soon): this may not be needed for AV (maybe it is only valid when devices are present)
//        result->addGroup(group);
      }
    } else {
      result->publishToPropertyTree();
    }
    return result;
  } // allocateZone

  void Apartment::removeZone(int _zoneID) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    for(std::vector<boost::shared_ptr<Zone> >::iterator ipZone = m_Zones.begin(), e = m_Zones.end();
        ipZone != e; ++ipZone) {
      boost::shared_ptr<Zone> pZone = *ipZone;
      if(pZone->getID() == _zoneID) {
        pZone->removeFromPropertyTree();
        m_Zones.erase(ipZone);
        return;
      }
    }
  } // removeZone

  void Apartment::removeDevice(dsuid_t _device) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    for(std::vector<boost::shared_ptr<Device> >::iterator ipDevice = m_Devices.begin(), e = m_Devices.end();
        ipDevice != e; ++ipDevice) {
      boost::shared_ptr<Device> pDevice = *ipDevice;
      if (pDevice->getDSID() == _device) {
        // Remove from zone
        int zoneID = pDevice->getZoneID();
        DeviceReference devRef = DeviceReference(pDevice, this);

        {
          boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(devRef);
          boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
          mEvent->setProperty("action", "deleted");
          mEvent->setProperty("dsuid", dsuid2str(pDevice->getDSID()));
          mEvent->setProperty("zoneID", intToString(pDevice->getZoneID()));
          if (DSS::hasInstance()) {
            DSS::getInstance()->getEventQueue().pushEvent(mEvent);
          }
        }

        if(zoneID != 0) {
          getZone(zoneID)->removeDevice(devRef);
        }
        getZone(0)->removeDevice(devRef);

        try {
          // Remove from dsm
          boost::shared_ptr<DSMeter> dsMeter = getDSMeterByDSID(pDevice->getDSMeterDSID());
          dsMeter->removeDevice(devRef);
        } catch (ItemNotFoundException& e) {
          Logger::getInstance()->log(std::string("Apartment::removeDevice: Unknown dSM: ") + e.what(), lsWarning);
        }

        pDevice->clearBinaryInputs(); // calls apartment->removeState from inside
        pDevice->clearStates(); // calls apartment->removeState() from inside

        // Erase
        m_Devices.erase(ipDevice);
        return;
      }
    }
  } // removeDevice

  void Apartment::removeDSMeter(dsuid_t _dsMeter) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    for(std::vector<boost::shared_ptr<DSMeter> >::iterator ipDSMeter = m_DSMeters.begin(), e = m_DSMeters.end();
        ipDSMeter != e; ++ipDSMeter) {
      boost::shared_ptr<DSMeter> pDSMeter = *ipDSMeter;
      if (pDSMeter->getDSID() == _dsMeter) {
        m_DSMeters.erase(ipDSMeter);
        return;
      }
    }
  } // removeDSMeter

  void Apartment::removeInactiveMeters() {
    std::vector<boost::shared_ptr<DSMeter> > dsMeters = getDSMeters();

    bool dirtyFlag = false;
    foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
      if(!dsMeter->isPresent()) {
        removeDSMeter(dsMeter->getDSID());
        dirtyFlag = true;
      }
    }

    if(dirtyFlag) {
      m_pModelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    }
  } // removeInactiveMeters

  boost::shared_ptr<State> Apartment::allocateState(const eStateType _type, const std::string& _stateName, const std::string& _scriptId) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    boost::shared_ptr<State> result;
    foreach (auto&& state, m_States) {
      if ((state->getName() == _stateName) && (state->getType() == _type) &&
          (state->getProviderService() == _scriptId)) {
        result = state;
        break;
      }
    }
    if(result == NULL) {
      if (_scriptId.length()) {
        result = boost::make_shared<State> (_type, _stateName, _scriptId);
      } else {
        result = boost::make_shared<State> (_stateName);
      }
      m_States.push_back(result);
    }
    return result;
  } // allocateState

  void Apartment::allocateState(boost::shared_ptr<State> _state) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    boost::shared_ptr<State> result;
    foreach (auto&& state, m_States) {
      if(state->getName() == _state->getName()) {
        throw ItemDuplicateException("Duplicate state " + _state->getName());
      }
    }
    m_States.push_back(_state);
  } // allocateState

  boost::shared_ptr<State> Apartment::tryGetState(const eStateType _type, const std::string& _name) const {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach (auto&& state, m_States) {
      if ((state->getType() == _type) && (state->getName() == _name)) {
        return state;
      }
    }
    return boost::shared_ptr<State>();
  } // tryGetState

  boost::shared_ptr<State> Apartment::getState(const eStateType _type, const std::string& _name) const {
    auto state = tryGetState(_type, _name);
    if (!state) {
      throw ItemNotFoundException(_name);
    }
    return state;
  } // getState

  boost::shared_ptr<State> Apartment::getNonScriptState(const std::string& _stateName) const {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach (auto&& state, m_States) {
      if ((state->getType() != StateType_Script) &&
          (state->getName() == _stateName)) {
        return state;
      }
    }
    throw ItemNotFoundException(_stateName);
  }

  boost::shared_ptr<State> Apartment::getState(const eStateType _type,
                                               const std::string& _identifier,
                                               const std::string& _stateName) const {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach (auto&& state, m_States) {
      if ((state->getType() == _type) &&
          (state->getProviderService() == _identifier) &&
          (state->getName() == _stateName)) {
        return state;
      }
    }
    throw ItemNotFoundException(_stateName);
  }

  boost::shared_ptr<State> Apartment::getState(const std::string& identifier, const std::string& name) const {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    foreach (auto&& state, m_States) {
      if ((state->getProviderService() == identifier) && (state->getName() == name)) {
        return state;
      }
    }
    DS_FAIL_REQUIRE("State not found", name);
  }

  // throws (dynamic_cast relies on consistent sensor names)
  void Apartment::updateSensorStates(const dsuid_t &dsuid, SensorType sensorType, double value, callOrigin_t origin)
  {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    // A single sensor can have multiple states, by setting different on/off threshold
    // TODO(someday) SensorState should have a field to identify the input sensor
    std::string prefix("dev." + dsuid2str(dsuid) + ".type" + intToString(static_cast<int>(sensorType)));

    foreach (auto&& state, m_States) {
      if (boost::starts_with(state->getName(), prefix)) {
        boost::dynamic_pointer_cast<StateSensor>(state)->newValue(origin, value);
      }
    }
  }

  // throws (dynamic_cast relies on consistent sensor names)
  void Apartment::updateSensorStates(int zoneId, int groupId, SensorType sensorType, double value, callOrigin_t origin)
  {
    boost::recursive_mutex::scoped_lock scoped_lock(m_mutex);
    // A single sensor can have multiple states, by setting different on/off threshold
    // TODO(someday) SensorState should have a field to identify the input sensor
    std::string prefix("zone.zone" + intToString(zoneId) + ".group" + intToString(groupId) + ".type" + intToString(static_cast<int>(sensorType)));

    foreach (auto&& state, m_States) {
      if (boost::starts_with(state->getName(), prefix)) {
        boost::dynamic_pointer_cast<StateSensor>(state)->newValue(origin, value);
      }
    }
  }

  void Apartment::removeState(boost::shared_ptr<State> _state) {
    m_States.erase(std::remove(m_States.begin(), m_States.end(), _state));
  }

  ActionRequestInterface* Apartment::getActionRequestInterface() {
    if(m_pBusInterface != NULL) {
      return m_pBusInterface->getActionRequestInterface();
    }
    return NULL;
  } // getActionRequestInterface

  DeviceBusInterface* Apartment::getDeviceBusInterface() {
    if(m_pBusInterface != NULL) {
      return m_pBusInterface->getDeviceBusInterface();
    }
    return NULL;
  } // getDeviceBusInterface

  void Apartment::setPropertySystem(PropertySystem* _value) {
    m_pPropertySystem = _value;
    if(m_pPropertySystem != NULL) {
      m_pPropertyNode = m_pPropertySystem->createProperty("/apartment");
      foreach(boost::shared_ptr<Zone> pZone, m_Zones) {
        pZone->publishToPropertyTree();
      }
    }
  } // setPropertySystem

  ApartmentSensorStatus_t Apartment::getSensorStatus() const {
    return m_SensorStatus;
  }

  void Apartment::setTemperature(double _value, DateTime& _ts) {
    m_SensorStatus.m_TemperatureValue = _value;
    m_SensorStatus.m_TemperatureTS = _ts;
  }

  void Apartment::setHumidityValue(double _value, DateTime& _ts) {
    m_SensorStatus.m_HumidityValue = _value;
    m_SensorStatus.m_HumidityTS = _ts;
  }

  void Apartment::setBrightnessValue(double _value, DateTime& _ts) {
    m_SensorStatus.m_BrightnessValue = _value;
    m_SensorStatus.m_BrightnessTS = _ts;
  }

  void Apartment::setWindSpeed(double _value, DateTime& _ts) {
    m_SensorStatus.m_WindSpeedValue = _value;
    m_SensorStatus.m_WindSpeedTS = _ts;
  }

  void Apartment::setWindDirection(double _value, DateTime& _ts) {
    m_SensorStatus.m_WindDirectionValue = _value;
    m_SensorStatus.m_WindDirectionTS = _ts;
  }

  void Apartment::setGustSpeed(double _value, DateTime& _ts) {
    m_SensorStatus.m_GustSpeedValue = _value;
    m_SensorStatus.m_GustSpeedTS = _ts;
  }

  void Apartment::setGustDirection(double _value, DateTime& _ts) {
    m_SensorStatus.m_GustDirectionValue = _value;
    m_SensorStatus.m_GustDirectionTS = _ts;
  }

  void Apartment::setPrecipitation(double _value, DateTime& _ts) {
    m_SensorStatus.m_PrecipitationValue = _value;
    m_SensorStatus.m_PrecipitationTS = _ts;
  }

  void Apartment::setAirPressure(double _value, DateTime& _ts) {
    m_SensorStatus.m_AirPressureValue = _value;
    m_SensorStatus.m_AirPressureTS = _ts;
  }

  void Apartment::setWeatherInformation(std::string& _iconId, std::string& _conditionId, std::string _serviceId, DateTime& _ts) {
    m_SensorStatus.m_WeatherIconId = _iconId;
    m_SensorStatus.m_WeatherConditionId = _conditionId;
    m_SensorStatus.m_WeatherServiceId = _serviceId;
    m_SensorStatus.m_WeatherTS = _ts;
  }

  std::pair<std::vector<DeviceLock_t>, std::vector<ZoneLock_t> > Apartment::getClusterLocks() {

    std::vector<DeviceLock_t> lockedDevices;
    std::vector<ZoneLock_t> lockedZones;

    std::vector<boost::shared_ptr<Zone> > zones = getZones();
    // loop through all zones
    for (size_t z = 0; z < zones.size(); z++) {

      if (zones.at(z)->getID() == 0) {
        continue;
      }

      Set devices = zones.at(z)->getDevices();

      std::vector<ClassLock_t> classLocks;

      // look for devices of each class in the zone
      for (int c = DEVICE_CLASS_GE; c <= DEVICE_CLASS_WE; c++) {

        if (c == DEVICE_CLASS_SW) {
          // ignore unconfigured SW devices
          continue;
        }

        std::set<int> lockedScenes;

        Set devclass = devices.getByGroup(c);

        // loop through all devices of a certain group
        for (int d = 0; d < devclass.length(); d++) {
          boost::shared_ptr<Device> device = devclass.get(d).getDevice();
          std::vector<int> ls = device->getLockedScenes();
          if (!ls.empty()) {
            DeviceLock_t lockedDevice;
            lockedDevice.dsuid = device->getDSID();
            lockedDevice.lockedScenes = ls;
            lockedDevices.push_back(lockedDevice);
            lockedScenes.insert(ls.begin(), ls.end());
          } else {
            if (device->isInLockedCluster()) {
              DeviceLock_t lockedDevice;
              lockedDevice.dsuid = device->getDSID();
              lockedDevices.push_back(lockedDevice);
            }
          }
        } // devices loop

        if (!lockedScenes.empty()) {
            ClassLock_t cl;
            cl.deviceClass = static_cast<DeviceClasses_t>(c);
            cl.lockedScenes = lockedScenes;
            classLocks.push_back(cl);
        }
      } // device class loop

      if (!classLocks.empty()) {
        ZoneLock_t zl;
        zl.zoneID = zones.at(z)->getID();
        zl.deviceClassLocks = classLocks;
        lockedZones.push_back(zl);
      }
    } // zone loop

    return std::make_pair(lockedDevices, lockedZones);
  }

  bool Apartment::setDevicesFirstSeen(const DateTime& dateTime) {
    static const DateTime allowedDate = DateTime::parseISO8601("2011-01-01T00:00:00Z");
    if (dateTime < allowedDate) {
      return false;
    }
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      const DateTime originDate = dev->getFirstSeen();
      if (originDate < allowedDate) {
        dev->setFirstSeen(dateTime);
      }
    }
    m_pModelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return true;
  }

} // namespace dss
