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
    scrubVector(m_DSMeters);
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

  void Apartment::dsMeterReady(int _dsMeterBusID) {
    log("DSMeter with id: " + intToString(_dsMeterBusID) + " is ready", lsInfo);
    try {
      try {
        DSMeter& mod = getDSMeterByBusID(_dsMeterBusID);
        BusScanner scanner(*m_pDS485Interface->getStructureQueryBusInterface(), *this);
        if(scanner.scanDSMeter(mod)) {
          boost::shared_ptr<Event> dsMeterReadyEvent(new Event("dsMeter_ready"));
          dsMeterReadyEvent->setProperty("dsMeter", mod.getDSID().toString());
          raiseEvent(dsMeterReadyEvent);
        }
      } catch(DS485ApiError& e) {
        log(std::string("Exception caught while scanning dsMeter " + intToString(_dsMeterBusID) + " : ") + e.what(), lsFatal);

        ModelEvent* pEvent = new ModelEvent(ModelEvent::etDSMeterReady);
        pEvent->addParameter(_dsMeterBusID);
        addModelEvent(pEvent);
      }
    } catch(ItemNotFoundException& e) {
      log("No dsMeter for bus-id (" + intToString(_dsMeterBusID) + ") found, re-discovering devices");
      discoverDS485Devices();
    }
  } // dsMeterReady

  void Apartment::setPowerConsumption(int _dsMeterBusID, unsigned long _value) {
    getDSMeterByBusID(_dsMeterBusID).setPowerConsumption(_value);
  } // powerConsumption

  void Apartment::setEnergyMeterValue(int _dsMeterBusID, unsigned long _value) {
    getDSMeterByBusID(_dsMeterBusID).setEnergyMeterValue(_value);
  } // energyMeterValue

  void Apartment::discoverDS485Devices() {
    // temporary mark all dsMeters as absent
    foreach(DSMeter* pDSMeter, m_DSMeters) {
      pDSMeter->setIsPresent(false);
    }

    // Request the dsid of all dsMeters
    DS485CommandFrame requestFrame;
    requestFrame.getHeader().setBroadcast(true);
    requestFrame.getHeader().setDestination(0);
    requestFrame.setCommand(CommandRequest);
    requestFrame.getPayload().add<uint8_t>(FunctionDSMeterGetDSID);
    if(DSS::hasInstance()) {
      DSS::getInstance()->getDS485Interface().getFrameSenderInterface()->sendFrame(requestFrame);
    }
  } // discoverDS485Devices

  void Apartment::writeConfiguration() {
    if(DSS::hasInstance()) {
      ModelPersistence persistence(*this);
      persistence.writeConfigurationToXML(DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile"));
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
      case ModelEvent::etNewDSMeter:
        discoverDS485Devices();
        break;
      case ModelEvent::etLostDSMeter:
        discoverDS485Devices();
        break;
      case ModelEvent::etDSMeterReady:
        if(event.getParameterCount() != 1) {
          log("Expected exactly 1 parameter for ModelEvent::etDSMeterReady");
        } else {
          try{
            DSMeter& mod = getDSMeterByBusID(event.getParameter(0));
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
             getDSMeterByDSID(newDSID).setBusID(busID);
             log ("dSM present");
             getDSMeterByDSID(newDSID).setIsPresent(true);
          } catch(ItemNotFoundException& e) {
             log ("dSM not present");
             DSMeter& dsMeter = allocateDSMeter(newDSID);
             dsMeter.setBusID(busID);
             dsMeter.setIsPresent(true);
             dsMeter.setIsValid(false);
             ModelEvent* pEvent = new ModelEvent(ModelEvent::etDSMeterReady);
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
      foreach(DSMeter* pDSMeter, m_DSMeters) {
        if(pDSMeter->isPresent()) {
          if(!pDSMeter->isValid()) {
            dsMeterReady(pDSMeter->getBusID());
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
      ModelPersistence persistence(*this);
      persistence.readConfigurationFromXML(configFileName);
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

    // load devices/dsMeters/etc. from a config-file
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

  Device& Apartment::getDeviceByShortAddress(const DSMeter& _dsMeter, const devid_t _deviceID) const {
    foreach(Device* dev, m_Devices) {
      if((dev->getShortAddress() == _deviceID) &&
          (_dsMeter.getBusID() == dev->getDSMeterID())) {
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

  DSMeter& Apartment::getDSMeter(const std::string& _modName) {
    foreach(DSMeter* dsMeter, m_DSMeters) {
      if(dsMeter->getName() == _modName) {
        return *dsMeter;
      }
    }
    throw ItemNotFoundException(_modName);
  } // getDSMeter(name)

  DSMeter& Apartment::getDSMeterByBusID(const int _busId) {
    foreach(DSMeter* dsMeter, m_DSMeters) {
      if(dsMeter->getBusID() == _busId) {
        return *dsMeter;
      }
    }
    throw ItemNotFoundException(intToString(_busId));
  } // getDSMeterByBusID

  DSMeter& Apartment::getDSMeterByDSID(const dsid_t _dsid) {
    foreach(DSMeter* dsMeter, m_DSMeters) {
      if(dsMeter->getDSID() == _dsid) {
        return *dsMeter;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDSMeterByDSID

  std::vector<DSMeter*>& Apartment::getDSMeters() {
    return m_DSMeters;
  } // getDSMeters

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

  DSMeter& Apartment::allocateDSMeter(const dsid_t _dsid) {
    foreach(DSMeter* dsMeter, m_DSMeters) {
      if((dsMeter)->getDSID() == _dsid) {
        return *dsMeter;
      }
    }

    DSMeter* pResult = new DSMeter(_dsid);
    m_DSMeters.push_back(pResult);
    return *pResult;
  } // allocateDSMeter

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

  void Apartment::removeDSMeter(dsid_t _dsMeter) {
    for(std::vector<DSMeter*>::iterator ipDSMeter = m_DSMeters.begin(), e = m_DSMeters.end();
        ipDSMeter != e; ++ipDSMeter) {
      DSMeter* pDSMeter = *ipDSMeter;
      if(pDSMeter->getDSID() == _dsMeter) {
        m_DSMeters.erase(ipDSMeter);
        delete pDSMeter;
        return;
      }
    }
  } // removeDSMeter

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

  void Apartment::onDeviceCallScene(const int _dsMeterID, const int _deviceID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onDeviceCallScene: _sceneID is out of bounds. dsMeter-id '" + intToString(_dsMeterID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID), lsError);
        return;
      }
      DSMeter& mod = getDSMeterByBusID(_dsMeterID);
      try {
        log("OnDeviceCallScene: dsMeter-id '" + intToString(_dsMeterID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID));
        DeviceReference devRef = mod.getDevices().getByBusID(_deviceID, _dsMeterID);
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          devRef.getDevice().setLastCalledScene(_sceneID & 0x00ff);
        }
      } catch(ItemNotFoundException& e) {
        log("OnDeviceCallScene: Could not find device with bus-id '" + intToString(_deviceID) + "' on dsMeter '" + intToString(_dsMeterID) + "' scene:" + intToString(_sceneID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnDeviceCallScene: Could not find dsMeter with bus-id '" + intToString(_dsMeterID) + "'", lsError);
    }
  } // onDeviceCallScene

  void Apartment::onAddDevice(const int _modID, const int _zoneID, const int _devID, const int _functionID) {
    // get full dsid
    log("New Device found");
    log("  DSMeter: " + intToString(_modID));
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));
    log("  FID:       " + intToString(_functionID));

    dsid_t dsid = getDSS().getDS485Interface().getStructureQueryBusInterface()->getDSIDOfDevice(_modID, _devID);
    Device& dev = allocateDevice(dsid);
    DeviceReference devRef(dev, this);

    log("  DSID:      " + dsid.toString());

    // remove from old dsMeter
    try {
      DSMeter& oldDSMeter = getDSMeterByBusID(dev.getDSMeterID());
      oldDSMeter.removeDevice(devRef);
    } catch(std::runtime_error&) {
    }

    // remove from old zone
    if(dev.getZoneID() != 0) {
      try {
        Zone& oldZone = getZone(dev.getZoneID());
        oldZone.removeDevice(devRef);
        // TODO: check if the zone is empty on the dsMeter and remove it in that case
      } catch(std::runtime_error&) {
      }
    }

    // update device
    dev.setDSMeterID(_modID);
    dev.setZoneID(_zoneID);
    dev.setShortAddress(_devID);
    dev.setFunctionID(_functionID);
    dev.setIsPresent(true);

    // add to new dsMeter
    DSMeter& dsMeter = getDSMeterByBusID(_modID);
    dsMeter.addDevice(devRef);

    // add to new zone
    Zone& newZone = allocateZone(_zoneID);
    newZone.addToDSMeter(dsMeter);
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
    log("  DSMeter: " + intToString(_modID));
    log("  DevID:     " + intToString(_devID));
    log("  Priority:  " + intToString(_priority));

    try {
      DSMeter& dsMeter = getDSMeterByBusID(_modID);
      try {
        Device& device = getDeviceByShortAddress(dsMeter, _devID);
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
      log("Apartment::onDSLinkInterrupt: Unknown DSMeter with ID " + intToString(_modID), lsFatal);
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
