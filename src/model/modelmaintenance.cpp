/*
    Copyright (c) 2010, 2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Christian Hitz, aizo AG <christian.hitz@aizo.com>
             Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#include "src/foreach.h"
#include "src/base.h"
#include "src/dss.h"
#include "src/event.h"
#include "src/businterface.h"
#include "src/propertysystem.h"
#include "src/model/modelconst.h"
#include "src/metering/metering.h"
#include "src/security/security.h"

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
    m_IsDirty(false),
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
      handleModelEvents();
      handleDeferredModelEvents();
      readOutPendingMeter();
    }
  } // execute

  void ModelMaintenance::discoverDS485Devices() {
    if(m_pStructureQueryBusInterface != NULL) {
      try {
        std::vector<DSMeterSpec_t> meters =
          m_pStructureQueryBusInterface->getDSMeters();
        foreach (DSMeterSpec_t& spec, meters) {

          boost::shared_ptr<DSMeter> dsMeter;
          try{
             dsMeter = m_pApartment->getDSMeterByDSID(spec.DSID);
             log ("dSM present");
          } catch(ItemNotFoundException& e) {
             dsMeter = m_pApartment->allocateDSMeter(spec.DSID);
             log ("dSM not present");
          }

          if (!dsMeter->isConnected()) {
            dsMeter->setIsPresent(true);
            dsMeter->setIsConnected(true);
            dsMeter->setIsValid(false);
            ModelEventWithDSID* pEvent =
                new ModelEventWithDSID(ModelEvent::etDSMeterReady, spec.DSID);
            addModelEvent(pEvent);
          }
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

  bool ModelMaintenance::handleDeferredModelEvents() {
    if (m_DeferredEvents.empty()) {
      return false;
    }

    std::list<boost::shared_ptr<ModelDeferredEvent> > mDefEvents(m_DeferredEvents);
    m_DeferredEvents.clear();
    for(std::list<boost::shared_ptr<ModelDeferredEvent> >::iterator evt = mDefEvents.begin();
        evt != mDefEvents.end();
        evt++)
    {
      boost::shared_ptr<ModelDeferredSceneEvent> mEvent = boost::dynamic_pointer_cast <ModelDeferredSceneEvent> (*evt);
      if (mEvent != NULL) {
        int sceneID = mEvent->getSceneID();
        int groupID = mEvent->getGroupID();
        int zoneID = mEvent->getZoneID();
        int originDeviceID = mEvent->getOriginDeviceID();

        try {
          boost::shared_ptr<Zone> zone = m_pApartment->getZone(zoneID);
          boost::shared_ptr<Group> group = zone->getGroup(groupID);
          dss_dsid_t originDSID;
          if ((mEvent->getSource() != NullDSID) && (originDeviceID != 0)) {
            DeviceReference devRef = m_pApartment->getDevices().getByBusID(originDeviceID, mEvent->getSource());
            originDSID = devRef.getDSID();
          } else {
            originDSID.lower = originDeviceID;
          }

          if (mEvent->isDue()) {
            if (! (mEvent->isCalled())) {
              boost::shared_ptr<Event> pEvent;
              pEvent.reset(new Event("callScene", group));
              pEvent->setProperty("sceneID", intToString(sceneID));
              pEvent->setProperty("groupID", intToString(groupID));
              pEvent->setProperty("zoneID", intToString(zoneID));
              pEvent->setProperty("originDeviceID", originDSID.toString());
              raiseEvent(pEvent);
            }
            // finished deferred processing of this event
          } else if (SceneHelper::isDimSequence(sceneID)) {
            if (! (mEvent->isCalled())) {
              boost::shared_ptr<Event> pEvent;
              pEvent.reset(new Event("callScene", group));
              pEvent->setProperty("sceneID", intToString(sceneID));
              pEvent->setProperty("groupID", intToString(groupID));
              pEvent->setProperty("zoneID", intToString(zoneID));
              pEvent->setProperty("originDeviceID", originDSID.toString());
              raiseEvent(pEvent);
              mEvent->setCalled();
            }
            m_DeferredEvents.push_back(mEvent);
          } else {
            m_DeferredEvents.push_back(mEvent);
          }

        } catch(ItemNotFoundException& e) {
          log("handleDeferredModelEvents: Could not find group/zone with id "
              + intToString(groupID) + "/" + intToString(zoneID), lsError);
        } catch(SecurityException& e) {
          log("handleDeferredModelEvents: security error accessing group/zone with id "
              + intToString(groupID) + "/" + intToString(zoneID), lsError);
        }
        continue;
      }

      boost::shared_ptr<ModelDeferredButtonEvent> bEvent = boost::dynamic_pointer_cast <ModelDeferredButtonEvent> (*evt);
      if (bEvent != NULL) {
        int deviceID = bEvent->getDeviceID();
        int buttonIndex = bEvent->getButtonIndex();
        int clickType = bEvent->getClickType();

        try {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(deviceID, bEvent->getSource());
          boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));

          if (bEvent->isDue() || (clickType == ClickTypeHE)) {
            boost::shared_ptr<Event> event(new Event("buttonClick", pDevRev));
            event->setProperty("clickType", intToString(clickType));
            event->setProperty("buttonIndex", intToString(buttonIndex));
            if (bEvent->getRepeatCount() > 0) {
              event->setProperty("holdCount", intToString(bEvent->getRepeatCount()));
            }
            raiseEvent(event);
            // finished deferred processing of this event
          } else {
            m_DeferredEvents.push_back(bEvent);
          }

        } catch(ItemNotFoundException& e) {
          log("handleDeferredModelEvents: Could not find device with dsid "
              + bEvent->getSource().toString(), lsError);
        } catch(SecurityException& e) {
          log("handleDeferredModelEvents: security error accessing device with dsid "
              + bEvent->getSource().toString(), lsError);
        }
        continue;
      }
    }
    mDefEvents.clear();
    return true;
  } // handleDeferredModelEvents

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
      case ModelEvent::etDeviceConfigChanged:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() != 4) {
          log("Expected exactly 4 parameter for ModelEvent::etDeviceConfigChanged");
        } else {
          onDeviceConfigChanged(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1),
                                                           event.getParameter(2), event.getParameter(3));
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
      case ModelEvent::etButtonClickDevice:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() != 3) {
          log("Expected exactly 3 parameter for ModelEvent::etButtonClickDevice");
        } else {
          onDeviceActionEvent(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2));
          onDeviceActionFiltered(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etCallSceneGroup:
        if(event.getParameterCount() != 4) {
          log("Expected exactly 4 parameter for ModelEvent::etCallSceneGroup");
        } else {
          onGroupCallScene(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3));
          if (pEventWithDSID) {
            onGroupCallSceneFiltered(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3));
          }
        }
        break;
      case ModelEvent::etUndoSceneGroup:
        if(event.getParameterCount() < 3) {
          log("Expected at least 3 parameter for ModelEvent::etUndoSceneGroup");
        } else {
          int sceneID = -1;
          if(event.getParameterCount() >= 4) {
            sceneID = event.getParameter(3);
          }
          onGroupUndoScene(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), sceneID);
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
          assert(pEventWithDSID != NULL);
          dss_dsid_t meterID = pEventWithDSID->getDSID();
          try{
            boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(meterID);
            mod->setIsConnected(true);
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
            meter->updateEnergyMeterValue(energy);
            m_pMetering->postMeteringEvent(meter, power, meter->getCachedEnergyMeterValue(), DateTime());
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
             dsMeter = m_pApartment->getDSMeterByDSID(newDSID);
             log ("dSM present");
          } catch(ItemNotFoundException& e) {
             dsMeter = m_pApartment->allocateDSMeter(newDSID);
             log ("dSM not present");
          }
          dsMeter->setIsPresent(true);
          dsMeter->setIsConnected(true);
          dsMeter->setIsValid(false);
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

      if (m_IsDirty) {
        m_IsDirty = false;
        addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
      }

      {
        boost::shared_ptr<Event> readyEvent(new Event("model_ready"));
        raiseEvent(readyEvent);
      }
    }
  } // readOutPendingMeter

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
        Set devices = mod->getDevices();
        for (int i = 0; i < devices.length(); i++) {
          devices[i].getDevice()->setIsConnected(true);
        }
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
      m_IsDirty = true;
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

  class SetLastButOneCalledSceneAction : public IDeviceAction {
  protected:
    int m_SceneID;
  public:
    SetLastButOneCalledSceneAction(const int _sceneID)
    : m_SceneID(_sceneID) {}
    SetLastButOneCalledSceneAction()
    : m_SceneID(-1) {}
    virtual ~SetLastButOneCalledSceneAction() {}

    virtual bool perform(boost::shared_ptr<Device> _device) {
      if (m_SceneID >= 0) {
        _device->setLastButOneCalledScene(m_SceneID);
      } else {
        _device->setLastButOneCalledScene();
      }
      return true;
    }
  };

  void ModelMaintenance::onGroupCallScene(dss_dsid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID) {
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
        pEvent.reset(new Event("callSceneBus", group));
        pEvent->setProperty("sceneID", intToString(_sceneID));
        pEvent->setProperty("groupID", intToString(_groupID));
        pEvent->setProperty("zoneID", intToString(_zoneID));
        dss_dsid_t originDSID;
        if ((_source != NullDSID) && (_originDeviceID != 0)) {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(_originDeviceID, _source);
          originDSID = devRef.getDSID();
        } else {
          originDSID.lower = _originDeviceID;
        }
        pEvent->setProperty("originDeviceID", originDSID.toString());
        raiseEvent(pEvent);
      } else {
        log("OnGroupCallScene: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupCallScene: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }
  } // onGroupCallScene

  void ModelMaintenance::onGroupUndoScene(dss_dsid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID) {
    try {
      if(_sceneID < -1 || _sceneID > MaxSceneNumber) {
        log("onGroupUndoScene: Scene number is out of bounds. zoneID: " + intToString(_zoneID) + " groupID: " + intToString(_groupID) + " scene: " + intToString(_sceneID), lsError);
        return;
      }
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      boost::shared_ptr<Group> group = zone->getGroup(_groupID);
      if(group != NULL) {
        log("OnGroupUndoScene: group-id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "' scene: " + intToString(_sceneID));
        Set s = zone->getDevices().getByGroup(_groupID);
        SetLastButOneCalledSceneAction act(_sceneID);
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
              if (_sceneID >= 0) {
                pGroup->setLastButOneCalledScene(_sceneID);
              } else {
                pGroup->setLastButOneCalledScene();
              }
            }
          } else {
            boost::shared_ptr<Group> pGroup = pZone->getGroup(_groupID);
            if(pGroup != NULL) {
              if (_sceneID >= 0) {
                pGroup->setLastButOneCalledScene(_sceneID);
              } else {
                pGroup->setLastButOneCalledScene();
              }
            }
          }
        }

        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event("undoScene", group));
        pEvent->setProperty("sceneID", intToString(_sceneID));
        pEvent->setProperty("groupID", intToString(_groupID));
        pEvent->setProperty("zoneID", intToString(_zoneID));
        dss_dsid_t originDSID;
        if ((_source != NullDSID) && (_originDeviceID != 0)) {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(_originDeviceID, _source);
          originDSID = devRef.getDSID();
        } else {
          originDSID.lower = _originDeviceID;
        }
        pEvent->setProperty("originDeviceID", originDSID.toString());
        raiseEvent(pEvent);
      } else {
        log("OnGroupUndoScene: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupUndoScene: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }
  } // onGroupUndoScene

  void ModelMaintenance::onGroupCallSceneFiltered(dss_dsid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID) {
    // Filter Strategy:
    // Check for Source != 0 and per Zone and Group
    // - delayed On-Scene processing, for Scene1/Scene2/Scene3/Scene4
    // - report only the first Inc/Dec

    bool passThrough = true;

    if (SceneHelper::isMultiTipSequence(_sceneID)) {
      // do not filter calls from myself
      if (_source.lower == 0 && _source.upper == 0) {
        passThrough = true;
      }
      // do not filter calls to broadcast
      else if (_groupID == 0) {
        passThrough = true;
      }
      else {
        passThrough = false;
      }
    } else if (SceneHelper::isDimSequence(_sceneID)) {
      // filter dimming sequences to only report the first dimming event
      passThrough = false;
    }

    if (passThrough) {
      log("CallSceneFilter: pass through, from source " + _source.toString() +
          ": Zone=" + intToString(_zoneID) +
          ", Gruppe=" + intToString(_groupID) +
          ", OriginDevice=" + intToString(_originDeviceID) +
          ", Scene=" + intToString(_sceneID), lsDebug);

      boost::shared_ptr<ModelDeferredSceneEvent> mEvent(new ModelDeferredSceneEvent(_source, _zoneID, _groupID, _originDeviceID, _sceneID));
      mEvent->clearTimestamp();  // force immediate event processing
      m_DeferredEvents.push_back(mEvent);
      return;
    }

    foreach(boost::shared_ptr<ModelDeferredEvent> evt, m_DeferredEvents) {
      boost::shared_ptr<ModelDeferredSceneEvent> pEvent = boost::dynamic_pointer_cast <ModelDeferredSceneEvent> (evt);
      if (pEvent == NULL) {
        continue;
      }
      if (pEvent->getZoneID() == _zoneID && pEvent->getGroupID() == _groupID) {
        if ((pEvent->getSource() == _source) && ((pEvent->getOriginDeviceID() == _originDeviceID) || (_originDeviceID == 0))) {
          // dimming, adjust the old event's timestamp to keep it active
          if (SceneHelper::isDimSequence(_sceneID) && (pEvent->getSceneID() == _sceneID)) {
            pEvent->setTimestamp();
            return;
          }
          // going through SceneOff
          if (((pEvent->getSceneID() == SceneOff) && (_sceneID == Scene2)) ||
              ((pEvent->getSceneID() == SceneOffE1) && (_sceneID == Scene12)) ||
              ((pEvent->getSceneID() == SceneOffE2) && (_sceneID == Scene22)) ||
              ((pEvent->getSceneID() == SceneOffE3) && (_sceneID == Scene32)) ||
              ((pEvent->getSceneID() == SceneOffE4) && (_sceneID == Scene42))) {

            log("CallSceneFilter: update sceneID from " + intToString(pEvent->getSceneID()) +
                ": Zone=" + intToString(_zoneID) +
                ", Group=" + intToString(_groupID) +
                ", OriginDevice=" + intToString(_originDeviceID) +
                ", Scene=" + intToString(_sceneID));

            pEvent->setScene(_sceneID);
            return;
          }
          // area-on-2-3-4 sequence
          if (((pEvent->getSceneID() == SceneA11) || (pEvent->getSceneID() == SceneA21) ||
               (pEvent->getSceneID() == SceneA31) || (pEvent->getSceneID() == SceneA41)) &&
              ((_sceneID == Scene2) || (_sceneID == Scene12) || (_sceneID == Scene22) ||
               (_sceneID == Scene32) || (_sceneID == Scene42))) {

            log("CallSceneFilter: update sceneID from " + intToString(pEvent->getSceneID()) +
                ": Zone=" + intToString(_zoneID) +
                ", Group=" + intToString(_groupID) +
                ", OriginDevice=" + intToString(_originDeviceID) +
                ", Scene=" + intToString(_sceneID));

            pEvent->setScene(_sceneID);
            return;
          }
          // area-off-2-3-4 sequence, check for triple- or 4x-click
          if (((pEvent->getSceneID() == SceneOffA1) || (pEvent->getSceneID() == SceneOffA2) ||
               (pEvent->getSceneID() == SceneOffA3) || (pEvent->getSceneID() == SceneOffA4)) &&
              (_sceneID >= Scene2) && (_sceneID <= Scene44)) {

            log("CallSceneFilter: update sceneID from " + intToString(pEvent->getSceneID()) +
                ": Zone=" + intToString(_zoneID) +
                ", Group=" + intToString(_groupID) +
                ", OriginDevice=" + intToString(_originDeviceID) +
                ", Scene=" + intToString(_sceneID));

            pEvent->setScene(_sceneID);
            return;
          }
          // previous state match
          if ((int) SceneHelper::getPreviousScene(_sceneID) == pEvent->getSceneID()) {

            log("CallSceneFilter: update sceneID from " + intToString(pEvent->getSceneID()) +
                ": Zone=" + intToString(_zoneID) +
                ", Group=" + intToString(_groupID) +
                ", OriginDevice=" + intToString(_originDeviceID) +
                ", Scene=" + intToString(_sceneID));

            pEvent->setScene(_sceneID);
            return;
          }
          // next state match
          if ((int) SceneHelper::getNextScene((unsigned int) pEvent->getSceneID()) == _sceneID) {

            log("CallSceneFilter: update sceneID from " + intToString(pEvent->getSceneID()) +
                ": Zone=" + intToString(_zoneID) +
                ", Group=" + intToString(_groupID) +
                ", OriginDevice=" + intToString(_originDeviceID) +
                ", Scene=" + intToString(_sceneID));

            pEvent->setScene(_sceneID);
            return;
          }
        }
      }
    }

    log("CallSceneFilter: deferred processing, from source " + _source.toString() +
        ": Zone=" + intToString(_zoneID) +
        ", Group=" + intToString(_groupID) +
        ", OriginDevice=" + intToString(_originDeviceID) +
        ", Scene=" + intToString(_sceneID), lsDebug);

    boost::shared_ptr<ModelDeferredSceneEvent> mEvent(new ModelDeferredSceneEvent(_source, _zoneID, _groupID, _originDeviceID, _sceneID));
    m_DeferredEvents.push_back(mEvent);
  } // onGroupCallSceneFiltered

  void ModelMaintenance::onDeviceActionFiltered(dss_dsid_t _source, const int _deviceID, const int _buttonNr, const int _clickType) {

    // Filter Strategy:
    // Check for Source != 0 and per DeviceID
    // - delayed Tipp processing, for 1T..4T
    // - generate final hold event after HE

    bool passThrough = false;

    // do not filter calls from myself
    if (_source.lower == 0 && _source.upper == 0) {
      passThrough = true;
    }

    if (passThrough) {
      log("DeviceActionFilter: pass through, from source " + _source.toString() +
          ": deviceID=" + intToString(_deviceID) +
          ": buttonIndex=" + intToString(_buttonNr) +
          ", clickType=" + intToString(_clickType), lsDebug);

      boost::shared_ptr<ModelDeferredButtonEvent> mEvent(new ModelDeferredButtonEvent(_source, _deviceID, _buttonNr, _clickType));
      mEvent->clearTimestamp();  // force immediate event processing
      m_DeferredEvents.push_back(mEvent);
      return;
    }

    foreach(boost::shared_ptr<ModelDeferredEvent> evt, m_DeferredEvents) {
      boost::shared_ptr<ModelDeferredButtonEvent> pEvent = boost::dynamic_pointer_cast <ModelDeferredButtonEvent> (evt);
      if (pEvent == NULL) {
        continue;
      }
      if ((pEvent->getDeviceID() == _deviceID) && (pEvent->getButtonIndex() == _buttonNr) && (pEvent->getSource() == _source)) {
        // holding, adjust the old event's timestamp to keep it active
        if ((_clickType == ClickTypeHR) &&
            ((pEvent->getClickType() == ClickTypeHS) || (pEvent->getClickType() == ClickTypeHR))) {

          log("ActionDeviceFilter: hold repeat, update ClickType from " + intToString(pEvent->getClickType()) +
              " to clickType=" + intToString(_clickType));

          pEvent->setTimestamp();
          pEvent->incRepeatCount();
          pEvent->setClickType(_clickType);
          return;
        }
        if ((_clickType == ClickTypeHE) &&
            ((pEvent->getClickType() == ClickTypeHR) || (pEvent->getClickType() == ClickTypeHS)))  {

          log("ActionDeviceFilter: hold end, update ClickType from " + intToString(pEvent->getClickType()) +
              " to clickType=" + intToString(_clickType));

          pEvent->incRepeatCount();
          pEvent->setClickType(_clickType);
          return;
        }
        // going through 1T..4T
        if (((pEvent->getClickType() == ClickType1T) && (_clickType == ClickType2T)) ||
            ((pEvent->getClickType() == ClickType2T) && (_clickType == ClickType3T)) ||
            ((pEvent->getClickType() == ClickType3T) && (_clickType == ClickType4T))) {

          log("ActionDeviceFilter: update ClickType from " + intToString(pEvent->getClickType()) +
              " to clickType=" + intToString(_clickType));

          pEvent->setClickType(_clickType);
          return;
        }
        // going through 1C..3C/3T..4T
        if (((pEvent->getClickType() == ClickType1C) && (_clickType == ClickType3C)) ||
            ((pEvent->getClickType() == ClickType2C) && (_clickType == ClickType3C)) ||
            ((pEvent->getClickType() == ClickType1C) && (_clickType == ClickType2T)) ||
            ((pEvent->getClickType() == ClickType1C) && (_clickType == ClickType3T)) ||
            ((pEvent->getClickType() == ClickType2C) && (_clickType == ClickType3T)) ||
            ((pEvent->getClickType() == ClickType3C) && (_clickType == ClickType4T))) {

          log("ActionDeviceFilter: update ClickType from " + intToString(pEvent->getClickType()) +
              " to clickType=" + intToString(_clickType));

          pEvent->setClickType(_clickType);
          return;
        }
      }
    }

    log("DeviceActionFilter: deferred processing, from source " + _source.toString() + "/" + intToString(_deviceID) +
        ": buttonIndex=" + intToString(_buttonNr) +
        ", clickType=" + intToString(_clickType), lsDebug);

    boost::shared_ptr<ModelDeferredButtonEvent> mEvent(new ModelDeferredButtonEvent(_source, _deviceID, _buttonNr, _clickType));
    m_DeferredEvents.push_back(mEvent);
  } // onDeviceActionFiltered

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

  void ModelMaintenance::onDeviceActionEvent(const dss_dsid_t& _dsMeterID, const int _deviceID, const int _buttonNr, const int _clickType) {
    try {
      boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_dsMeterID);
      log("onDeviceActionEvent: dsMeter-id '" + _dsMeterID.toString() + "' for device '" + intToString(_deviceID) +
          "' button: " + intToString(_buttonNr) + ", click: " + intToString(_clickType));
      try {
        DeviceReference devRef = mod->getDevices().getByBusID(_deviceID, _dsMeterID);
        boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));
        boost::shared_ptr<Event> event(new Event("buttonClickBus", pDevRev));
        event->setProperty("clickType", intToString(_clickType));
        event->setProperty("buttonIndex", intToString(_buttonNr));
        raiseEvent(event);
      } catch(ItemNotFoundException& e) {
        log("onDeviceActionEvent: Could not find device with bus-id '" + intToString(_deviceID) + "' on dsMeter '" + _dsMeterID.toString(), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("onDeviceActionEvent: Could not find dsMeter with bus-id '" + _dsMeterID.toString() + "'", lsError);
    }
  } // onDeviceActionEvent

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
    log("dSM disconnected: " + _dSMeterID.toString());
    try {
      boost::shared_ptr<DSMeter> meter =
                              m_pApartment->getDSMeterByDSID(_dSMeterID);
      meter->setIsConnected(false);
      Set devices = meter->getDevices();
      for (int i = 0; i < devices.length(); i++) {
        devices[i].getDevice()->setIsConnected(false);
      }
    } catch(ItemNotFoundException& e) {
      log("Lost dSM " + _dSMeterID.toString() + " was not in our list");
    }
  }

  void ModelMaintenance::onDeviceConfigChanged(const dss_dsid_t& _dsMeterID, int _deviceID,
                                               int _configClass, int _configIndex, int _value) {
    try {
      DeviceReference devRef = m_pApartment->getDevices().getByBusID(_deviceID, _dsMeterID);
      if(_configClass == CfgClassFunction) {
        if(_configIndex == CfgFunction_Mode) {
          devRef.getDevice()->setOutputMode(_value);
        } else if(_configIndex == CfgFunction_ButtonMode) {
          devRef.getDevice()->setButtonID(_value & 0xf);
          devRef.getDevice()->setButtonActiveGroup((_value >> 4) & 0xf);
        } else if(_configIndex == CfgFunction_LTMode) {
          devRef.getDevice()->setButtonInputMode(_value);
        }
      }
    } catch(std::runtime_error& e) {
      log(std::string("Error updating config of device: ") + e.what());
    }
  } // onDeviceConfigChanged

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
