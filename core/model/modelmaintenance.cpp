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


#include "modelmaintenance.h"

#include <stdexcept>

#include "core/foreach.h"
#include "core/base.h"
#include "core/dss.h"
#include "core/event.h"
#include "core/DS485Interface.h"
#include "core/propertysystem.h"
#include "core/ds485const.h"
#include "core/model/modelconst.h"


#include "apartment.h"
#include "zone.h"
#include "device.h"
#include "devicereference.h"
#include "modulator.h"
#include "set.h"
#include "modelpersistence.h"
#include "busscanner.h"
#include "scenehelper.h"

namespace dss {

  ModelMaintenance::ModelMaintenance(DSS* _pDSS)
  : Subsystem(_pDSS, "Apartment"),
    Thread("Apartment"),
    m_IsInitializing(true)
  { }

  void ModelMaintenance::initialize() {
    Subsystem::initialize();
    if(m_pApartment == NULL) {
      throw std::runtime_error("Need apartment to work...");
    }
    if(DSS::hasInstance()) {
      DSS::getInstance()->getPropertySystem().setStringValue(getConfigPropertyBasePath() + "configfile", getDSS().getDataDirectory() + "apartment.xml", true, false);
    }
  } // initialize

  void ModelMaintenance::doStart() {
    run();
  } // start

  void ModelMaintenance::waitForInterface() {
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

  void ModelMaintenance::execute() {
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
  } // execute


  void ModelMaintenance::discoverDS485Devices() {
    // temporary mark all dsMeters as absent
    foreach(DSMeter* pDSMeter, m_pApartment->getDSMeters()) {
      pDSMeter->setIsPresent(false);
    }

    // Request the dsid of all dsMeters
    DS485CommandFrame requestFrame;
    requestFrame.getHeader().setBroadcast(true);
    requestFrame.getHeader().setDestination(0);
    requestFrame.setCommand(CommandRequest);
    requestFrame.getPayload().add<uint8_t>(FunctionDSMeterGetDSID);
    if(DSS::hasInstance()) {
      getDSS().getDS485Interface().getFrameSenderInterface()->sendFrame(requestFrame);
    }
  } // discoverDS485Devices

  void ModelMaintenance::writeConfiguration() {
    if(DSS::hasInstance()) {
      ModelPersistence persistence(*m_pApartment);
      persistence.writeConfigurationToXML(DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile"));
    }
  } // writeConfiguration

  void ModelMaintenance::handleModelEvents() {
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
            DSMeter& mod = m_pApartment->getDSMeterByBusID(event.getParameter(0));
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
	  int meterID = event.getParameter(0);
	  int consumption = event.getParameter(1);
	  DSMeter& meter = m_pApartment->getDSMeterByBusID(meterID);
	  meter.setPowerConsumption(consumption);
        }
        break;
      case ModelEvent::etEnergyMeterValue:
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etEnergyMeterValue");
        } else {
	  int meterID = event.getParameter(0);
	  int value = event.getParameter(1);
	  DSMeter& meter = m_pApartment->getDSMeterByBusID(meterID);
	  meter.setEnergyMeterValue(value);
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
             m_pApartment->getDSMeterByDSID(newDSID).setBusID(busID);
             log ("dSM present");
             m_pApartment->getDSMeterByDSID(newDSID).setIsPresent(true);
          } catch(ItemNotFoundException& e) {
             log ("dSM not present");
             DSMeter& dsMeter = m_pApartment->allocateDSMeter(newDSID);
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
      foreach(DSMeter* pDSMeter, m_pApartment->getDSMeters()) {
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

  void ModelMaintenance::dsMeterReady(int _dsMeterBusID) {
    log("DSMeter with id: " + intToString(_dsMeterBusID) + " is ready", lsInfo);
    try {
      try {
        DSMeter& mod = m_pApartment->getDSMeterByBusID(_dsMeterBusID);
        BusScanner scanner(*getDSS().getDS485Interface().getStructureQueryBusInterface(), *m_pApartment, *this);
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

  void ModelMaintenance::addModelEvent(ModelEvent* _pEvent) {
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

  void ModelMaintenance::readConfiguration() {
    if(DSS::hasInstance()) {
      std::string configFileName = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile");
      ModelPersistence persistence(*m_pApartment);
      persistence.readConfigurationFromXML(configFileName);
    }
  } // readConfiguration

  void ModelMaintenance::raiseEvent(boost::shared_ptr<Event> _pEvent) {
    if(DSS::hasInstance()) {
      getDSS().getEventQueue().pushEvent(_pEvent);
    }
  } // raiseEvent

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

  void ModelMaintenance::onGroupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onGroupCallScene: Scene number is out of bounds. zoneID: " + intToString(_zoneID) + " groupID: " + intToString(_groupID) + " scene: " + intToString(_sceneID), lsError);
        return;
      }
      Zone& zone = m_pApartment->getZone(_zoneID);
      Group* group = zone.getGroup(_groupID);
      if(group != NULL) {
        log("OnGroupCallScene: group-id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "' scene: " + intToString(_sceneID));
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          Set s = zone.getDevices().getByGroup(_groupID);
          SetLastCalledSceneAction act(_sceneID & 0x00ff);
          s.perform(act);

          std::vector<Zone*> zonesToUpdate;
          if(_zoneID == 0) {
            zonesToUpdate = m_pApartment->getZones();
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

  void ModelMaintenance::onDeviceCallScene(const int _dsMeterID, const int _deviceID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onDeviceCallScene: _sceneID is out of bounds. dsMeter-id '" + intToString(_dsMeterID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID), lsError);
        return;
      }
      DSMeter& mod = m_pApartment->getDSMeterByBusID(_dsMeterID);
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

  void ModelMaintenance::onAddDevice(const int _modID, const int _zoneID, const int _devID, const int _functionID) {
    // get full dsid
    log("New Device found");
    log("  DSMeter: " + intToString(_modID));
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));
    log("  FID:       " + intToString(_functionID));

    dsid_t dsid = getDSS().getDS485Interface().getStructureQueryBusInterface()->getDSIDOfDevice(_modID, _devID);
    Device& dev = m_pApartment->allocateDevice(dsid);
    DeviceReference devRef(dev, m_pApartment);

    log("  DSID:      " + dsid.toString());

    // remove from old dsMeter
    try {
      DSMeter& oldDSMeter = m_pApartment->getDSMeterByBusID(dev.getDSMeterID());
      oldDSMeter.removeDevice(devRef);
    } catch(std::runtime_error&) {
    }

    // remove from old zone
    if(dev.getZoneID() != 0) {
      try {
        Zone& oldZone = m_pApartment->getZone(dev.getZoneID());
        oldZone.removeDevice(devRef);
        // TODO: check if the zone is empty on the dsMeter and remove it in that case
      } catch(std::runtime_error&) {
      }
    }

    DSMeter& dsMeter = m_pApartment->getDSMeterByBusID(_modID);

    // update device
    dev.setDSMeter(dsMeter);
    dev.setZoneID(_zoneID);
    dev.setShortAddress(_devID);
    dev.setFunctionID(_functionID);
    dev.setIsPresent(true);

    dsMeter.addDevice(devRef);

    // add to new zone
    Zone& newZone = m_pApartment->allocateZone(_zoneID);
    newZone.addToDSMeter(dsMeter);
    newZone.addDevice(devRef);

    // get groups of device
    dev.resetGroups();
    std::vector<int> groups = getDSS().getDS485Interface().getStructureQueryBusInterface()->getGroupsOfDevice(_modID, _devID);
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

  void ModelMaintenance::onDSLinkInterrupt(const int _modID, const int _devID, const int _priority) {
    // get full dsid
    log("dSLinkInterrupt:");
    log("  DSMeter: " + intToString(_modID));
    log("  DevID:     " + intToString(_devID));
    log("  Priority:  " + intToString(_priority));

    try {
      DSMeter& dsMeter = m_pApartment->getDSMeterByBusID(_modID);
      try {
        Device& device = m_pApartment->getDeviceByShortAddress(dsMeter, _devID);
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
          DeviceReference devRef(device, m_pApartment);
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

  void ModelMaintenance::setApartment(Apartment* _value) {
    m_pApartment = _value;
    if(m_pApartment != NULL) {
      m_pApartment->setModelMaintenance(this);
    }
  } // setApartment

} // namespace dss
