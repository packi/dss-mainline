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

#include <unistd.h>

#include "core/foreach.h"
#include "core/base.h"
#include "core/dss.h"
#include "core/event.h"
#include "core/businterface.h"
#include "core/propertysystem.h"
#include "core/model/modelconst.h"
#include "core/metering/metering.h"
#include "core/security/security.h"

#include "apartment.h"
#include "zone.h"
#include "group.h"
#include "device.h"
#include "devicereference.h"
#include "modulator.h"
#include "set.h"
#include "modelpersistence.h"
#include "busscanner.h"
#include "scenehelper.h"

namespace dss {
  //=============================================== ApartmentTreeListener

  /** Raises a ModelDirty event if something below the apartment node gets changed. */
  class ApartmentTreeListener : public PropertyListener {
  public:
    ApartmentTreeListener(ModelMaintenance* _pModelMaintenance, Apartment* _pApartment)
    : m_pModelMaintenance(_pModelMaintenance),
      m_pApartment(_pApartment)
    {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pApartment->getPropertyNode()->addListener(this);
      }
    }

  protected:
    virtual void propertyChanged(PropertyNodePtr _caller, PropertyNodePtr _changedNode) {
      if(_changedNode->hasFlag(PropertyNode::Archive)) {
        ModelEvent* pEvent = new ModelEvent(ModelEvent::etModelDirty);
        m_pModelMaintenance->addModelEvent(pEvent);
      }
      if((_changedNode->getName() == "setsLocalPriority") &&
         (_changedNode->getValueType() == vTypeBoolean)) {
        handleLocalPriority(_changedNode);
      }
    }
  
  private:
    void handleLocalPriority(PropertyNodePtr _changedNode) {
      if((_changedNode->getParentNode() != NULL) &&
         (_changedNode->getParentNode()->getParentNode() != NULL)) {
        PropertyNode* deviceNode = 
          _changedNode->getParentNode()->getParentNode();
        boost::shared_ptr<Device> dev = 
          m_pApartment->getDeviceByDSID(dss_dsid_t::fromString(deviceNode->getName()));
        bool value = _changedNode->getBoolValue();
        if(DSS::hasInstance()) {
          StructureModifyingBusInterface* pInterface;
          pInterface = DSS::getInstance()->getBusInterface().getStructureModifyingBusInterface();
          pInterface->setButtonSetsLocalPriority(dev->getDSMeterDSID(), 
                                                 dev->getShortAddress(), 
                                                 value);
        }
      }
    }
  private:
    ModelMaintenance* m_pModelMaintenance;
    Apartment* m_pApartment;
  }; // ApartmentTreeListener


 //=============================================== ModelMaintenance

  ModelMaintenance::ModelMaintenance(DSS* _pDSS, const int _eventTimeoutMS)
  : ThreadedSubsystem(_pDSS, "Apartment"),
    m_IsInitializing(true),
    m_pApartment(NULL),
    m_pMetering(NULL),
    m_EventTimeoutMS(_eventTimeoutMS),
    m_pStructureQueryBusInterface(NULL)
  { }

  void ModelMaintenance::checkConfigFile(boost::filesystem::path _filename) {
    if (boost::filesystem::exists(_filename)) {
      log("Found " + _filename.string(), lsInfo);
      if (!rwAccess(_filename.string())) {
        throw std::runtime_error("Apartment file " + _filename.string() +
                                  " is not readable and writable!");
      }
    } else {
      log("Could not find " + _filename.string(), lsWarning);
      boost::filesystem::path dir = _filename.parent_path();
      if (!boost::filesystem::is_directory(dir)) {
        throw std::runtime_error("Path " + dir.string() +
                                 " to apartment file is invalid!");
      }

      if (!rwAccess(dir.string())) {
        throw std::runtime_error("Directory " + dir.string() +
                                 " for apartment file is not readable and " +
                                 " writable!");
      }
    }
  }

  void ModelMaintenance::initialize() {
    Subsystem::initialize();
    if(m_pApartment == NULL) {
      throw std::runtime_error("Need apartment to work...");
    }
    if(DSS::hasInstance()) {
      DSS::getInstance()->getPropertySystem().setStringValue(
              getConfigPropertyBasePath() + "configfile", 
              getDSS().getDataDirectory() + "apartment.xml", true, false);

      boost::filesystem::path filename(
              DSS::getInstance()->getPropertySystem().getStringValue(
                                   getConfigPropertyBasePath() + "configfile"));

      checkConfigFile(filename);

      m_pStructureQueryBusInterface = DSS::getInstance()->getBusInterface().getStructureQueryBusInterface();

      // load devices/dsMeters/etc. from a config-file
      readConfiguration();
    }
  } // initialize

  void ModelMaintenance::doStart() {
    run();
  } // start

  void ModelMaintenance::execute() {
   if(DSS::hasInstance()) {
     DSS::getInstance()->getSecurity().loginAsSystemUser("ModelMaintenance needs system-rights");
   }

    log("ModelMaintenance::execute: Enumerating model", lsInfo);
    discoverDS485Devices();

    boost::shared_ptr<ApartmentTreeListener> treeListener
      = boost::shared_ptr<ApartmentTreeListener>(
          new ApartmentTreeListener(this, m_pApartment));

    while(!m_Terminated) {
      if(!handleModelEvents()) {
        readOutPendingMeter();
        readSceneNames();
      }
    }
  } // execute

  void ModelMaintenance::discoverDS485Devices() {
    if(m_pStructureQueryBusInterface != NULL) {
      try {
        std::vector<DSMeterSpec_t> meters =
          m_pStructureQueryBusInterface->getDSMeters();
        foreach (DSMeterSpec_t& spec, meters) {
          ModelEventWithDSID* pEvent =
            new ModelEventWithDSID(ModelEvent::etDS485DeviceDiscovered,
                                   spec.DSID);
          addModelEvent(pEvent);
        }
      } catch(BusApiError& e) {
        log("Failed to discover ds485 devices", lsError);
      }
    }
  } // discoverDS485Devices

  void ModelMaintenance::writeConfiguration() {
    if(DSS::hasInstance()) {
      ModelPersistence persistence(*m_pApartment);
      persistence.writeConfigurationToXML(DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile"));
    }
  } // writeConfiguration

  bool ModelMaintenance::handleModelEvents() {
    if(!m_ModelEvents.empty()) {
      bool eraseEventFromList = true;
      ModelEvent& event = m_ModelEvents.front();
      ModelEventWithDSID* pEventWithDSID =
        dynamic_cast<ModelEventWithDSID*>(&event);
      switch(event.getEventType()) {
      case ModelEvent::etNewDevice:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etNewDevice");
        } else {
          onAddDevice(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1));
        }
        break;
      case ModelEvent::etLostDevice:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etLostDevice");
        } else {
          onRemoveDevice(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1));
        }
        break;
      case ModelEvent::etDeviceChanged:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() != 1) {
          log("Expected exactly 1 parameter for ModelEvent::etDeviceChanged");
        } else {
          rescanDevice(pEventWithDSID->getDSID(), event.getParameter(0));
        }
        break;
      case ModelEvent::etCallSceneDevice:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etCallSceneDevice");
        } else {
          onDeviceCallScene(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1));
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
        eraseModelEventsFromQueue(ModelEvent::etModelDirty);
        eraseEventFromList = false;
        writeConfiguration();
        break;
      case ModelEvent::etLostDSMeter:
        assert(pEventWithDSID != NULL);
        onLostDSMeter(pEventWithDSID->getDSID());
        break;
      case ModelEvent::etDSMeterReady:
        if(event.getParameterCount() != 0) {
          log("Expected exactly 0 parameter for ModelEvent::etDSMeterReady");
        } else {
          ModelEventWithDSID* pEventWithDSID =
            dynamic_cast<ModelEventWithDSID*>(&event);
          assert(pEventWithDSID != NULL);
          dss_dsid_t meterID = pEventWithDSID->getDSID();
          try{
            boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(meterID);
            mod->setIsPresent(true);
            mod->setIsValid(false);
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
      case ModelEvent::etMeteringValues:
        if(event.getParameterCount() != 2) {
          log("Expected exactly 1 parameter for ModelEvent::etMeteringValues");
        } else {
          assert(pEventWithDSID != NULL);
          dss_dsid_t meterID = pEventWithDSID->getDSID();
          int power = event.getParameter(0);
          int energy = event.getParameter(1);
          try {
            boost::shared_ptr<DSMeter> meter = m_pApartment->getDSMeterByDSID(meterID);
            meter->setPowerConsumption(power);
            meter->setEnergyMeterValue(energy);
            m_pMetering->postEnergyEvent(meter, energy, DateTime());
            m_pMetering->postConsumptionEvent(meter, power, DateTime());
          } catch(ItemNotFoundException& _e) {
            log("Received metering data for unknown meter, discarding", lsWarning);
          }
        }
        break;
      case ModelEvent::etDS485DeviceDiscovered:
        if(event.getParameterCount() != 0) {
          log("Expected exactly 0 parameter for ModelEvent::etDS485DeviceDiscovered");
        } else {
          assert(pEventWithDSID != NULL);
          dss_dsid_t newDSID = pEventWithDSID->getDSID();
          log ("Discovered device with DSID: " + newDSID.toString());
          boost::shared_ptr<DSMeter> dsMeter;

          try{
             log ("dSM present");
             dsMeter = m_pApartment->getDSMeterByDSID(newDSID);
          } catch(ItemNotFoundException& e) {
             log ("dSM not present");
             dsMeter = m_pApartment->allocateDSMeter(newDSID);
          }

          dsMeter->setIsPresent(true);
          dsMeter->setIsValid(false);
          ModelEventWithDSID* pEvent =
                   new ModelEventWithDSID(ModelEvent::etDSMeterReady, newDSID);
          addModelEvent(pEvent);
        }
        break;
      default:
        assert(false);
        break;
      }

      if(eraseEventFromList) {
        m_ModelEventsMutex.lock();
        m_ModelEvents.erase(m_ModelEvents.begin());
        m_ModelEventsMutex.unlock();
      }
      return true;
    } else {
      return m_NewModelEvent.waitFor(m_EventTimeoutMS);
    }
  } // handleModelEvents

  void ModelMaintenance::readOutPendingMeter() {
    bool hadToUpdate = false;
    foreach(boost::shared_ptr<DSMeter> pDSMeter, m_pApartment->getDSMeters()) {
      if(pDSMeter->isPresent()) {
        if(!pDSMeter->isValid()) {
          dsMeterReady(pDSMeter->getDSID());
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
  } // readOutPendingMeter

  void ModelMaintenance::readSceneNames() {
    if(m_IsInitializing) {
      return;
    }
    std::vector<boost::shared_ptr<Zone>  > zones = m_pApartment->getZones();
    foreach(boost::shared_ptr<Zone> zone, zones) {
      // skip broadcast zone
      if(zone->getID() == 0) {
        continue;
      }
      foreach(boost::shared_ptr<Group> group, zone->getGroups()) {
        if(!group->isInitializedFromBus() && group->isPresent()) {
          log("Reading scene-names of Zone " + intToString(group->getZoneID()) +
              " Group " + intToString(group->getID()));
          // ask the first non-simulated meter
          boost::shared_ptr<const DSMeter> meterToAsk;
          foreach(boost::shared_ptr<const DSMeter> meter, zone->getDSMeters()) {
            if(meter->isPresent()) {
              meterToAsk = meter;
              if(!meter->getDSID().isSimulated()) {
                break;
              }
            }
          }
          try {
            if(meterToAsk != NULL) {
              log("Getting scene-names data from " + meterToAsk->getDSID().toString());
              const unsigned int sceneNumbersToRead =
                (zone->getID() == 0 ? MaxSceneNumber :
                                      MaxSceneNumberOutsideZoneZero);
              for(unsigned int sceneNumber = 0; sceneNumber <= sceneNumbersToRead; sceneNumber++) {
                std::string sceneName =
                  m_pStructureQueryBusInterface->getSceneName(
                    meterToAsk->getDSID(),
                    group,
                    sceneNumber);
                log("Scene " + intToString(sceneNumber) + ": '" + sceneName + "'");
                group->setSceneName(sceneNumber, sceneName);
              }
              group->setIsInitializedFromBus(true);
            }
          } catch(BusApiError& _err) {
            log("Error reading scene values from " + meterToAsk->getDSID().toString() +
                ". message: '" + _err.what() + "'", lsError);
          }
        }
      }
    }
  } // readSceneNames

  void ModelMaintenance::eraseModelEventsFromQueue(ModelEvent::EventType _type) {
    m_ModelEventsMutex.lock();
    for(boost::ptr_vector<ModelEvent>::iterator it = m_ModelEvents.begin(); it != m_ModelEvents.end(); ) {
      if(it->getEventType() == _type) {
        it = m_ModelEvents.erase(it);
      } else {
        ++it;
      }
    }
    m_ModelEventsMutex.unlock();
  } // eraseModelEventsFromQueue

  void ModelMaintenance::dsMeterReady(const dss_dsid_t& _dsMeterBusID) {
    log("DSMeter with DSID: " + _dsMeterBusID.toString() + " is ready", lsInfo);
    try {
      try {
        boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_dsMeterBusID);
        BusScanner scanner(*m_pStructureQueryBusInterface, *m_pApartment, *this);
        if(scanner.scanDSMeter(mod)) {
          boost::shared_ptr<Event> dsMeterReadyEvent(new Event("dsMeter_ready"));
          dsMeterReadyEvent->setProperty("dsMeter", mod->getDSID().toString());
          raiseEvent(dsMeterReadyEvent);
        }
      } catch(BusApiError& e) {
        log(std::string("Exception caught while scanning dsMeter " + _dsMeterBusID.toString() + " : ") + e.what(), lsFatal);

        ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etDSMeterReady, _dsMeterBusID);
        addModelEvent(pEvent);
      }
    } catch(ItemNotFoundException& e) {
      log("No dsMeter for bus-id (" + _dsMeterBusID.toString() + ") found, re-discovering devices");
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

    virtual bool perform(boost::shared_ptr<Device> _device) {
      _device->setLastCalledScene(m_SceneID);
      return true;
    }
  };

  void ModelMaintenance::onGroupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onGroupCallScene: Scene number is out of bounds. zoneID: " + intToString(_zoneID) + " groupID: " + intToString(_groupID) + " scene: " + intToString(_sceneID), lsError);
        return;
      }
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      boost::shared_ptr<Group> group = zone->getGroup(_groupID);
      if(group != NULL) {
        log("OnGroupCallScene: group-id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "' scene: " + intToString(_sceneID));
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          Set s = zone->getDevices().getByGroup(_groupID);
          SetLastCalledSceneAction act(_sceneID & 0x00ff);
          s.perform(act);

          std::vector<boost::shared_ptr<Zone> > zonesToUpdate;
          if(_zoneID == 0) {
            zonesToUpdate = m_pApartment->getZones();
          } else {
            zonesToUpdate.push_back(zone);
          }
          foreach(boost::shared_ptr<Zone> pZone, zonesToUpdate) {
            if(_groupID == 0) {
              foreach(boost::shared_ptr<Group> pGroup, pZone->getGroups()) {
                pGroup->setLastCalledScene(_sceneID & 0x00ff);
              }
            } else {
              boost::shared_ptr<Group> pGroup = pZone->getGroup(_groupID);
              if(pGroup != NULL) {
                pGroup->setLastCalledScene(_sceneID & 0x00ff);
              }
            }
          }
        }
        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event("callScene", group));
        pEvent->setProperty("sceneID", intToString(_sceneID));
        raiseEvent(pEvent);
      } else {
        log("OnGroupCallScene: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupCallScene: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }
  } // onGroupCallScene

  void ModelMaintenance::onDeviceNameChanged(dss_dsid_t _meterID, 
                                             const devid_t _deviceID, 
                                             const std::string& _name) {
    log("Device name changed on the bus. Meter: " + _meterID.toString() + 
        " bus-id: " + intToString(_deviceID), lsInfo);
    try {
      boost::shared_ptr<DSMeter> pMeter = 
        m_pApartment->getDSMeterByDSID(_meterID);
      DeviceReference ref = pMeter->getDevices().getByBusID(_deviceID, pMeter);
      ref.getDevice()->setName(_name);
    } catch(ItemNotFoundException& e) {
      log("onDeviceNameChanged: Item not found: " + std::string(e.what()));
    }
  } // onDeviceNameChanged

  void ModelMaintenance::onDsmNameChanged(dss_dsid_t _meterID,
                                          const std::string& _name)
  {
      log("dSM name changed on the bus. Meter: " + _meterID.toString(), lsInfo);
      try {
        boost::shared_ptr<DSMeter> pMeter =
          m_pApartment->getDSMeterByDSID(_meterID);
        pMeter->setName(_name);

      } catch(ItemNotFoundException& e) {
        log("onDsmNameChanged: Item not found: " + std::string(e.what()));
      }
  }

  void ModelMaintenance::onDeviceCallScene(const dss_dsid_t& _dsMeterID, const int _deviceID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onDeviceCallScene: _sceneID is out of bounds. dsMeter-id '" + _dsMeterID.toString() + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID), lsError);
        return;
      }
      boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_dsMeterID);
      try {
        log("OnDeviceCallScene: dsMeter-id '" + _dsMeterID.toString() + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID));
        DeviceReference devRef = mod->getDevices().getByBusID(_deviceID, _dsMeterID);
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          devRef.getDevice()->setLastCalledScene(_sceneID & 0x00ff);
        }
        boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));
        boost::shared_ptr<Event> event(new Event("callScene", pDevRev));
        event->setProperty("sceneID", intToString(_sceneID));
        raiseEvent(event);
      } catch(ItemNotFoundException& e) {
        log("OnDeviceCallScene: Could not find device with bus-id '" + intToString(_deviceID) + "' on dsMeter '" + _dsMeterID.toString() + "' scene:" + intToString(_sceneID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnDeviceCallScene: Could not find dsMeter with bus-id '" + _dsMeterID.toString() + "'", lsError);
    }
  } // onDeviceCallScene

  void ModelMaintenance::onAddDevice(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _devID) {
    log("New Device found");
    log("  DSMeter: " +  _dsMeterID.toString());
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));

    rescanDevice(_dsMeterID, _devID);
  } // onAddDevice

  void ModelMaintenance::onRemoveDevice(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _devID) {
    log("Device disappeared");
    log("  DSMeter: " +  _dsMeterID.toString());
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));

    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      boost::shared_ptr<DSMeter> dsMeter = m_pApartment->getDSMeterByDSID(_dsMeterID);
      boost::shared_ptr<Device> device = dsMeter->getDevices().getByBusID(_devID, _dsMeterID).getDevice();
      DeviceReference devRef(device, m_pApartment);

      if(_zoneID == 0) {
        // TODO: remove zone from meter if it's the last device
        // already handled in structuremanipulator
        zone->removeDevice(devRef);
      }

      dsMeter->removeDevice(devRef);
      device->setIsPresent(false);
    } catch(ItemNotFoundException& e) {
      log("Removed device " + _dsMeterID.toString() + " is not known to us");
    }
  } // onRemoveDevice

  void ModelMaintenance::onLostDSMeter(const dss_dsid_t& _dSMeterID) {
    try {
      boost::shared_ptr<DSMeter> meter =
                              m_pApartment->getDSMeterByDSID(_dSMeterID);
      meter->setIsPresent(false);
      Set devices = meter->getDevices();
      for (int i = 0; i < devices.length(); i++) {
        devices[i].getDevice()->setIsPresent(false);
      }
    } catch(ItemNotFoundException& e) {
      log("Lost dSM " + _dSMeterID.toString() + " was not in our list");
    }
  }
  
  void ModelMaintenance::rescanDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) {
    BusScanner
      scanner(
        *m_pStructureQueryBusInterface,
        *m_pApartment,
        *this);
    try {
      boost::shared_ptr<DSMeter> dsMeter = m_pApartment->getDSMeterByDSID(_dsMeterID);
      scanner.scanDeviceOnBus(dsMeter, _deviceID);
    } catch(std::runtime_error& e) {
      log(std::string("Error scanning device: ") + e.what());
    }
  } // rescanDevice

  void ModelMaintenance::setApartment(Apartment* _value) {
    m_pApartment = _value;
    if(m_pApartment != NULL) {
      m_pApartment->setModelMaintenance(this);
    }
  } // setApartment

  void ModelMaintenance::setMetering(Metering* _value) {
    m_pMetering = _value;
  } // setMetering

  void ModelMaintenance::setStructureQueryBusInterface(StructureQueryBusInterface* _value) {
    m_pStructureQueryBusInterface = _value;
  } // setStructureQueryBusInterface

} // namespace dss
