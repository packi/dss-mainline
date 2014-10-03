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
#include <json.h>

#include "src/foreach.h"
#include "src/base.h"
#include "src/dss.h"
#include "src/event.h"
#include "src/businterface.h"
#include "src/propertysystem.h"
#include "src/model/modelconst.h"
#include "src/metering/metering.h"
#include "src/security/security.h"
#include "src/structuremanipulator.h"

#include "apartment.h"
#include "zone.h"
#include "group.h"
#include "device.h"
#include "devicereference.h"
#include "eventinterpreterplugins.h"
#include "internaleventrelaytarget.h"
#include "modulator.h"
#include "state.h"
#include "set.h"
#include "modelpersistence.h"
#include "busscanner.h"
#include "scenehelper.h"
#include "src/ds485/dsdevicebusinterface.h"
#include "http_client.h"
#include "boost/filesystem.hpp"
#include "util.h"
#include "vdc-connection.h"

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
      if((_changedNode->getName() == "callsPresent") &&
         (_changedNode->getValueType() == vTypeBoolean)) {
        handleCallsPresent(_changedNode);
      }
    }

  private:
    void handleLocalPriority(PropertyNodePtr _changedNode) {
      if((_changedNode->getParentNode() != NULL) &&
         (_changedNode->getParentNode()->getParentNode() != NULL)) {
        PropertyNode* deviceNode =
          _changedNode->getParentNode()->getParentNode();
        boost::shared_ptr<Device> dev =
          m_pApartment->getDeviceByDSID(str2dsuid(deviceNode->getName()));
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
    void handleCallsPresent(PropertyNodePtr _changedNode) {
      if((_changedNode->getParentNode() != NULL) &&
         (_changedNode->getParentNode()->getParentNode() != NULL)) {
        PropertyNode* deviceNode =
          _changedNode->getParentNode()->getParentNode();
        boost::shared_ptr<Device> dev =
          m_pApartment->getDeviceByDSID(str2dsuid(deviceNode->getName()));
        bool value = _changedNode->getBoolValue();
        if(DSS::hasInstance()) {
          StructureModifyingBusInterface* pInterface;
          pInterface = DSS::getInstance()->getBusInterface().getStructureModifyingBusInterface();
          pInterface->setButtonCallsPresent(dev->getDSMeterDSID(),
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
    m_pStructureQueryBusInterface(NULL),
    m_pStructureModifyingBusInterface(NULL),
    m_taskProcessor(boost::shared_ptr<TaskProcessor>(new TaskProcessor()))
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

      DSS::getInstance()->getPropertySystem().setStringValue(
          getConfigPropertyBasePath() + "iconBasePath",
          DSS::getInstance()->getPropertySystem().getStringValue(
              "/config/datadirectory") + "images/", true, false);

      checkConfigFile(filename);

      m_pStructureQueryBusInterface = DSS::getInstance()->getBusInterface().getStructureQueryBusInterface();
      m_pStructureModifyingBusInterface = DSS::getInstance()->getBusInterface().getStructureModifyingBusInterface();

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
    if (m_pStructureQueryBusInterface != NULL) {
      try {
        std::vector<DSMeterSpec_t> meters = m_pStructureQueryBusInterface->getDSMeters();
        foreach (DSMeterSpec_t& spec, meters) {

          boost::shared_ptr<DSMeter> dsMeter;
          try{
             dsMeter = m_pApartment->getDSMeterByDSID(spec.DSID);
             log ("dS485 Bus Device known: " + dsuid2str(spec.DSID) + ", type:" + intToString(spec.DeviceType));
          } catch(ItemNotFoundException& e) {
             dsMeter = m_pApartment->allocateDSMeter(spec.DSID);
             log ("dS485 Bus Device NEW: " + dsuid2str(spec.DSID)  + ", type: " + intToString(spec.DeviceType), lsWarning);
          }

          try {
            Set devices = m_pApartment->getDevices().getByLastKnownDSMeter(spec.DSID);
            for (int i = 0; i < devices.length(); i++) {
              devices[i].getDevice()->setIsValid(false);
            }
          } catch(ItemNotFoundException& e) {
            log("discoverDS485Devices: error resetting device valid flags: " + std::string(e.what()));
          }

          // #7558 - force a rescan unconditionally
          if (true) {
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

  void ModelMaintenance::handleDeferredModelStateChanges(callOrigin_t _origin, int _zoneID, int _groupID, int _sceneID) {
    std::vector<boost::shared_ptr<Zone> > zonesToUpdate;
    if (_zoneID == 0) {
      zonesToUpdate = m_pApartment->getZones();
    } else {
      zonesToUpdate.push_back(m_pApartment->getZone(_zoneID));
    }
    foreach(boost::shared_ptr<Zone> pZone, zonesToUpdate) {
      if (_groupID == 0) {
        foreach(boost::shared_ptr<Group> pGroup, pZone->getGroups()) {
          pGroup->setOnState(_origin, _sceneID);
        }
      } else {
        boost::shared_ptr<Group> pGroup = pZone->getGroup(_groupID);
        if(pGroup != NULL) {
          pGroup->setOnState(_origin, _sceneID);
        }
      }
    }
  }

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
        callOrigin_t callOrigin = mEvent->getCallOrigin();
        std::string originToken = mEvent->getOriginToken();

        try {
          boost::shared_ptr<Zone> zone = m_pApartment->getZone(zoneID);
          boost::shared_ptr<Group> group = zone->getGroup(groupID);
          dsuid_t originDSUID;
          dsuid_t evtDSID = mEvent->getSource();
          if (!IsNullDsuid(evtDSID) && (originDeviceID != 0)) {
            DeviceReference devRef = m_pApartment->getDevices().getByBusID(originDeviceID, mEvent->getSource());
            originDSUID = devRef.getDSID();
          }

          if (mEvent->isDue()) {
            if (! (mEvent->isCalled())) {
              boost::shared_ptr<Event> pEvent;
              pEvent.reset(new Event(EventName::CallScene, group));
              pEvent->setProperty("sceneID", intToString(sceneID));
              pEvent->setProperty("groupID", intToString(groupID));
              pEvent->setProperty("zoneID", intToString(zoneID));
              pEvent->setProperty("originDSUID", dsuid2str(originDSUID));
              pEvent->setProperty("callOrigin", intToString(callOrigin));
              pEvent->setProperty("originToken", originToken);
              if (mEvent->getForcedFlag()) {
                pEvent->setProperty("forced", "true");
              }
              handleDeferredModelStateChanges(callOrigin, zoneID, groupID, sceneID);
              raiseEvent(pEvent);
            }
            // finished deferred processing of this event
          } else if (SceneHelper::isDimSequence(sceneID)) {
            if (! (mEvent->isCalled())) {
              boost::shared_ptr<Event> pEvent;
              pEvent.reset(new Event(EventName::CallScene, group));
              pEvent->setProperty("sceneID", intToString(sceneID));
              pEvent->setProperty("groupID", intToString(groupID));
              pEvent->setProperty("zoneID", intToString(zoneID));
              pEvent->setProperty("originDSUID", dsuid2str(originDSUID));
              pEvent->setProperty("callOrigin", intToString(callOrigin));
              pEvent->setProperty("originToken", originToken);
              if (mEvent->getForcedFlag()) {
                pEvent->setProperty("forced", "true");
              }
              handleDeferredModelStateChanges(callOrigin, zoneID, groupID, sceneID);
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
              + dsuid2str(bEvent->getSource()), lsError);
        } catch(SecurityException& e) {
          log("handleDeferredModelEvents: security error accessing device with dsid "
              + dsuid2str(bEvent->getSource()), lsError);
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
      ModelEventWithStrings* pEventWithStrings =
        dynamic_cast<ModelEventWithStrings*>(&event);
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
        if(event.getParameterCount() == 5) {
          onDeviceCallScene(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), (callOrigin_t)event.getParameter(3), event.getParameter(4), event.getSingleStringParameter());
        } else {
          log("Unexpected parameter count for ModelEvent::etCallSceneDevice");
        }
        break;
      case ModelEvent::etBlinkDevice:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() == 3) {
          onDeviceBlink(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), (callOrigin_t)event.getParameter(2), event.getSingleStringParameter());
        } else {
          log("Unexpected parameter count for ModelEvent::etBlinkDevice");
        }
        break;
      case ModelEvent::etCallSceneDeviceLocal:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() == 5) {
          onDeviceCallScene(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), (callOrigin_t)event.getParameter(3), event.getParameter(4), event.getSingleStringParameter());
        } else {
          log("Unexpected parameter count for ModelEvent::etCallSceneDeviceLocal");
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
        assert(pEventWithDSID != NULL);
        if (event.getParameterCount() == 6) {
          onGroupCallScene(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3), (callOrigin_t)event.getParameter(4), event.getParameter(5), event.getSingleStringParameter());
          if (pEventWithDSID) {
            onGroupCallSceneFiltered(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3), (callOrigin_t)event.getParameter(4), event.getParameter(5), event.getSingleStringParameter());
          }
        } else {
          log("Expected 6 parameters for ModelEvent::etCallSceneGroup");
        }
        break;
      case ModelEvent::etUndoSceneGroup:
        if(event.getParameterCount() != 5) {
          log("Expected 5 parameters for ModelEvent::etUndoSceneGroup");
        } else {
          onGroupUndoScene(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3), (callOrigin_t)event.getParameter(4), event.getSingleStringParameter());
        }
        break;
      case ModelEvent::etBlinkGroup:
        assert(pEventWithDSID != NULL);
        if (event.getParameterCount() != 4) {
          log("Expected at least 3 parameter for ModelEvent::etBlinkGroup");
        } else {
          onGroupBlink(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), (callOrigin_t)event.getParameter(3), event.getSingleStringParameter());
        }
        break;
      case ModelEvent::etModelDirty:
        eraseModelEventsFromQueue(ModelEvent::etModelDirty);
        eraseEventFromList = false;
        writeConfiguration();
        if (DSS::getInstance()->getPropertySystem().getBoolValue("/config/webservice-api/enabled")) {
          raiseEvent(ModelChangedEvent::createApartmentChanged()); /* raiseTimedEvent */
        }
        break;
      case ModelEvent::etLostDSMeter:
        assert(pEventWithDSID != NULL);
        onLostDSMeter(pEventWithDSID->getDSID());
        break;
      case ModelEvent::etDS485DeviceDiscovered:
        assert(pEventWithDSID != NULL);
        onJoinedDSMeter(pEventWithDSID->getDSID());
        break;
      case ModelEvent::etDSMeterReady:
        if(event.getParameterCount() != 0) {
          log("Expected exactly 0 parameter for ModelEvent::etDSMeterReady");
        } else {
          assert(pEventWithDSID != NULL);
          dsuid_t meterID = pEventWithDSID->getDSID();
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
      case ModelEvent::etBusDown:
        log("Got bus down event.", lsInfo);
        try {
          std::vector<boost::shared_ptr<DSMeter> > meters = m_pApartment->getDSMeters();
          foreach(boost::shared_ptr<DSMeter> meter, meters) {
            onLostDSMeter(meter->getDSID());
          }
        } catch(ItemNotFoundException& _e) {
        }
        break;
      case ModelEvent::etMeteringValues:
        if(event.getParameterCount() != 2) {
          log("Expected exactly 1 parameter for ModelEvent::etMeteringValues");
        } else {
          assert(pEventWithDSID != NULL);
          dsuid_t meterID = pEventWithDSID->getDSID();
          unsigned int power = event.getParameter(0);
          unsigned int energy = event.getParameter(1);
          try {
            boost::shared_ptr<DSMeter> meter = m_pApartment->getDSMeterByDSID(meterID);
            meter->setPowerConsumption(power);
            meter->updateEnergyMeterValue(energy);
            m_pMetering->postMeteringEvent(meter, power, (unsigned long long)(meter->getCachedEnergyMeterValue() + 0.5), DateTime());
          } catch(ItemNotFoundException& _e) {
          }
        }
        break;
      case ModelEvent::etDeviceSensorEvent:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() < 2) {
          log("Expected at least 2 parameter for ModelEvent::etDeviceSensorEvent");
        } else {
          onSensorEvent(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1));
        }
        break;
      case ModelEvent::etDeviceBinaryStateEvent:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() < 4) {
          log("Expected at least 4 parameter for ModelEvent::etDeviceBinaryStateEvent");
        } else {
          onBinaryInputEvent(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3));
        }
        break;
      case ModelEvent::etDeviceSensorValue:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() < 3) {
          log("Expected at least 3 parameter for ModelEvent::etDeviceSensorValue");
        } else {
          onSensorValue(pEventWithDSID->getDSID(), event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etZoneSensorValue:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() < 5) {
          log("Expected at least 5 parameter for ModelEvent::etZoneSensorValue");
        } else {
          onZoneSensorValue(pEventWithDSID->getDSID(), event.getSingleStringParameter(),
              event.getParameter(0),
              event.getParameter(1),
              event.getParameter(2),
              event.getParameter(3),
              event.getParameter(4));
        }
        break;
      case ModelEvent::etDeviceEANReady:
        assert(pEventWithDSID != NULL);
        if(event.getParameterCount() != 9) {
          log("Expected 8 parameters for ModelEvent::etDeviceEANReady");
        } else {
          onEANReady(pEventWithDSID->getDSID(),
                     event.getParameter(0),
                     (const DeviceOEMState_t)event.getParameter(1),
                     (const DeviceOEMInetState_t)event.getParameter(2),
                     ((unsigned long long)event.getParameter(3)) << 32 | ((unsigned long long)event.getParameter(4) & 0xFFFFFFFF),
                     event.getParameter(5),
                     event.getParameter(6),
                     event.getParameter(7),
                     event.getParameter(8));
        }
        break;
      case ModelEvent::etDeviceOEMDataReady:
        assert(pEventWithStrings != NULL);
        if((event.getParameterCount() != 2) && (pEventWithStrings->getStringParameterCount() != 4)) {
          log("Expected 5 parameters for ModelEvent::etDeviceOEMDataReady");
        } else {
          onOEMDataReady(pEventWithDSID->getDSID(),
                         event.getParameter(0),
                         (DeviceOEMState_t)event.getParameter(1),
                         pEventWithStrings->getStringParameter(0),
                         pEventWithStrings->getStringParameter(1),
                         pEventWithStrings->getStringParameter(2),
                         pEventWithStrings->getStringParameter(3));
        }
        break;
      case ModelEvent::etControllerConfig:
        assert(pEventWithDSID != NULL);
        onHeatingControllerConfig(
            pEventWithDSID->getDSID(),
            event.getParameter(0),
            event.getSingleObjectParameter());
        break;
      case ModelEvent::etControllerState:
        assert(pEventWithDSID != NULL);
        onHeatingControllerState(
            pEventWithDSID->getDSID(),
            event.getParameter(0),
            event.getParameter(1));
        break;
      case ModelEvent::etControllerValues:
        assert(pEventWithDSID != NULL);
        onHeatingControllerValues(
            pEventWithDSID->getDSID(),
            event.getParameter(0),
            event.getSingleObjectParameter());
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

  void ModelMaintenance::setApartmentState() {
    if (!DSS::hasInstance()) {
      return;
    }

    // logic discussed with mtr:
    //
    // if dSMs do not return the same state, then go with whatever is in
    // the dSS
    //
    // dSMs that report "unknown" do not fall into consideration
    //
    // if all dSMs have the same state set (either present or absent but
    // not unknown), then this state shall be set in the dSS
    //
    // if neither the dSMs nor the dSS have a valid known state, we will use
    // "present" as default as defined by our PO. this case is extremely rare.
    //
    // "as defined above, the dSS keeps the last known state persistent and will
    // set this state when starting up. So the dSS can always provide a state,
    // even after a 3h break down.  We neither need another default state
    // (except for the very first dSS startup where it should be present) nor a
    // timeout."

    // start with whatever state was saved in the dSS
    uint8_t dssaptstate = DSM_APARTMENT_STATE_UNKNOWN;
    boost::shared_ptr<State> state;
    try {
      state = m_pApartment->getState(StateType_Service, "presence");
    } catch (ItemNotFoundException &ex) {
      log("setApartmentState: error accessing apartment presence state", lsError);
      return;
    }
    if (state->getState() == State_Active) {
      dssaptstate = DSM_APARTMENT_STATE_PRESENT;
    } else if (state->getState() == State_Inactive) {
      dssaptstate = DSM_APARTMENT_STATE_ABSENT;
    }

    uint8_t lastdsmaptstate = DSM_APARTMENT_STATE_UNKNOWN;
    boost::shared_ptr<DSMeter> pDSMeter;
    for (size_t i = 0; i < m_pApartment->getDSMeters().size(); i++) {
      pDSMeter = m_pApartment->getDSMeters().at(i);
      if (pDSMeter->isPresent() && pDSMeter->isValid()) {
        uint8_t dsmaptstate = pDSMeter->getApartmentState();

        // dSMs that do not know their state will be ignored
        if (dsmaptstate == DSM_APARTMENT_STATE_UNKNOWN) {
          continue;
        }

        // remember last state in order to be able to compare the dSMs
        if (lastdsmaptstate == DSM_APARTMENT_STATE_UNKNOWN) {
          lastdsmaptstate = dsmaptstate;
        }

        // if dSMs disagree go with the dSS state
        if (lastdsmaptstate != dsmaptstate) {
          lastdsmaptstate = DSM_APARTMENT_STATE_UNKNOWN;
          break;
        }
      }
    }

    // result from the dSMs was conclusive
    if (lastdsmaptstate != DSM_APARTMENT_STATE_UNKNOWN) {
      dssaptstate = lastdsmaptstate;
    }

    // dSS state is still not known and dSMs were inconclusive:
    // use present as default as defined by the PO
    if (dssaptstate == DSM_APARTMENT_STATE_UNKNOWN) {
      dssaptstate = DSM_APARTMENT_STATE_PRESENT;
    }

    std::string strstate;
    if (dssaptstate == DSM_APARTMENT_STATE_PRESENT) {
      strstate = "present";
    } else if (dssaptstate == DSM_APARTMENT_STATE_ABSENT) {
      strstate = "absent";
    }

    log("setApartmentState: apartment state set to " + strstate, lsDebug);
    state->setState(coSystem, strstate);
  }

  void ModelMaintenance::autoAssignSensors() {
    StructureManipulator manipulator(*m_pStructureModifyingBusInterface,
                                     *m_pStructureQueryBusInterface,
                                     *m_pApartment);
    std::vector<boost::shared_ptr<Zone> > zones = m_pApartment->getZones();
    for (size_t i = 0; i < zones.size(); i++) {
      boost::shared_ptr<Zone> zone = zones.at(i);
      if (!zone) {
        continue;
      }

      manipulator.autoAssignZoneSensors(zone);
    }
  }

  void ModelMaintenance::readOutPendingMeter() {
    bool hadToUpdate = false;
    foreach(boost::shared_ptr<DSMeter> pDSMeter, m_pApartment->getDSMeters()) {
      if (pDSMeter->isPresent() &&
          (!pDSMeter->isValid())) {
          dsMeterReady(pDSMeter->getDSID());
          hadToUpdate = true;
          break;
      }
    }

    // If dSMeter configuration has changed we need to synchronize user-groups
    if (hadToUpdate && !m_IsInitializing) {
      synchronizeGroups(m_pApartment, m_pStructureModifyingBusInterface);
    }

    // If we didn't have to update for one cycle, assume that we're done
    if (!hadToUpdate && m_IsInitializing) {
      synchronizeGroups(m_pApartment, m_pStructureModifyingBusInterface);

      log("******** Finished loading model from dSM(s)...", lsInfo);
      m_IsInitializing = false;

      if (m_IsDirty) {
        m_IsDirty = false;
        addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
      }

      setApartmentState();
      autoAssignSensors();
      {
        boost::shared_ptr<Event> readyEvent(new Event("model_ready"));
        raiseEvent(readyEvent);
        try {
          CommChannel::getInstance()->resumeUpdateTask();
          CommChannel::getInstance()->requestLockedScenes();
        } catch (std::runtime_error &err) {
          log(err.what(), lsError);
        }

        setupWebUpdateEvent();
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

  void ModelMaintenance::dsMeterReady(const dsuid_t& _dsMeterBusID) {
    log("Scanning dS485 bus device: " + dsuid2str(_dsMeterBusID), lsInfo);
    try {

      boost::shared_ptr<DSMeter> mod;
      try {
        mod = m_pApartment->getDSMeterByDSID(_dsMeterBusID);
      } catch(ItemNotFoundException& e) {
        log("Error scanning dS485 bus device: " + dsuid2str(_dsMeterBusID) + " not found in data model", lsError);
        return; // nothing we could do here ...
      }

      try {
        BusScanner scanner(*m_pStructureQueryBusInterface, *m_pApartment, *this);
        if (!scanner.scanDSMeter(mod)) {
          log("Error scanning dS485 device: " + dsuid2str(_dsMeterBusID) + ", data model incomplete", lsError);
          return;
        }

        if (mod->getCapability_HasDevices()) {
          Set devices = mod->getDevices();
          for (int i = 0; i < devices.length(); i++) {
            devices[i].getDevice()->setIsConnected(true);
          }

          // synchronize devices with binary inputs
          if (mod->hasPendingEvents()) {
            scanner.syncBinaryInputStates(mod, boost::shared_ptr<Device>());
          } else {
            log(std::string("Event counter match on dSM ") + dsuid2str(_dsMeterBusID), lsDebug);
          }

          // additionally set all previously connected device to valid now
          devices = m_pApartment->getDevices().getByLastKnownDSMeter(_dsMeterBusID);
          for (int i = 0; i < devices.length(); i++) {
            devices[i].getDevice()->setIsValid(true);
          }
        }

        boost::shared_ptr<Event> dsMeterReadyEvent(new Event("dsMeter_ready"));
        dsMeterReadyEvent->setProperty("dsMeter", dsuid2str(mod->getDSID()));
        raiseEvent(dsMeterReadyEvent);

      } catch(BusApiError& e) {
        log(std::string("Bus error scanning dSM " + dsuid2str(_dsMeterBusID) + " : ") + e.what(), lsFatal);
        ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etDSMeterReady, _dsMeterBusID);
        addModelEvent(pEvent);
      }
    } catch(ItemNotFoundException& e) {
      log("dsMeterReady " + dsuid2str(_dsMeterBusID) + ": item not found: " + std::string(e.what()), lsError);
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
      if (boost::filesystem::exists(configFileName)) {
        persistence.readConfigurationFromXML(configFileName);
      }
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

  void ModelMaintenance::onGroupCallScene(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, const callOrigin_t _origin, const bool _forced, std::string _token) {
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

          boost::shared_ptr<Zone> pZone = m_pApartment->getZone(_zoneID);
          if(_groupID == GroupIDBroadcast) {
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

        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event(EventName::CallSceneBus, group));
        pEvent->setProperty("sceneID", intToString(_sceneID));
        pEvent->setProperty("groupID", intToString(_groupID));
        pEvent->setProperty("zoneID", intToString(_zoneID));
        pEvent->setProperty("token", _token);
        dsuid_t originDSUID = _source;
        if ((!IsNullDsuid(_source)) && (_originDeviceID != 0)) {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(_originDeviceID, _source);
          originDSUID = devRef.getDSID();
        }
        pEvent->setProperty("originDSUID", dsuid2str(originDSUID));
        if (_forced) {
          pEvent->setProperty("forced", "true");
        }
        pEvent->setProperty("callOrigin", intToString(_origin));
        raiseEvent(pEvent);
      } else {
        log("OnGroupCallScene: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupCallScene: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }
  } // onGroupCallScene

  void ModelMaintenance::onGroupUndoScene(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, callOrigin_t _origin, const std::string _token) {
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
              pGroup->setOnState(_origin, pGroup->getLastCalledScene());
            }
          } else {
            boost::shared_ptr<Group> pGroup = pZone->getGroup(_groupID);
            if(pGroup != NULL) {
              if (_sceneID >= 0) {
                pGroup->setLastButOneCalledScene(_sceneID);
              } else {
                pGroup->setLastButOneCalledScene();
              }
              pGroup->setOnState(_origin, pGroup->getLastCalledScene());
            }
          }
        }

        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event(EventName::UndoScene, group));
        pEvent->setProperty("sceneID", intToString(_sceneID));
        pEvent->setProperty("groupID", intToString(_groupID));
        pEvent->setProperty("zoneID", intToString(_zoneID));
        dsuid_t originDSUID;
        if ((!IsNullDsuid(_source)) && (_originDeviceID != 0)) {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(_originDeviceID, _source);
          originDSUID = devRef.getDSID();
        }
        pEvent->setProperty("callOrigin", intToString(_origin));
        pEvent->setProperty("originDSUID", dsuid2str(originDSUID));
        pEvent->setProperty("originToken", _token);
        raiseEvent(pEvent);
      } else {
        log("OnGroupUndoScene: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupUndoScene: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }
  } // onGroupUndoScene

  void ModelMaintenance::onGroupCallSceneFiltered(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, const callOrigin_t _origin, const bool _forced, std::string _token) {
    // Filter Strategy:
    // Check for Source != 0 and per Zone and Group
    // - delayed On-Scene processing, for Scene1/Scene2/Scene3/Scene4
    // - report only the first Inc/Dec

    bool passThrough = true;

    if (SceneHelper::isMultiTipSequence(_sceneID)) {
      // do not filter calls from myself
      if (IsNullDsuid(_source)) {
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
      log("CallSceneFilter: pass through, from source " + dsuid2str(_source) +
          ": Zone=" + intToString(_zoneID) +
          ", Gruppe=" + intToString(_groupID) +
          ", OriginDevice=" + intToString(_originDeviceID) +
          ", CallOrigin=" + intToString(_origin) +
          ", OriginToken=" + _token +
          ", Scene=" + intToString(_sceneID), lsDebug);

      // flush all pending events from same origin
      foreach(boost::shared_ptr<ModelDeferredEvent> evt, m_DeferredEvents) {
        boost::shared_ptr<ModelDeferredSceneEvent> pEvent = boost::dynamic_pointer_cast <ModelDeferredSceneEvent> (evt);
        dsuid_t tmp_dsuid = pEvent->getSource();
        if ((pEvent != NULL) && IsEqualDsuid(tmp_dsuid, _source) && (pEvent->getOriginDeviceID() == _originDeviceID)) {
          pEvent->clearTimestamp();
        }
      }

      boost::shared_ptr<ModelDeferredSceneEvent> mEvent(new ModelDeferredSceneEvent(_source, _zoneID, _groupID, _originDeviceID, _sceneID, _origin, _forced, _token));
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
        dsuid_t tmp_dsuid = pEvent->getSource();
        if (IsEqualDsuid(tmp_dsuid, _source) && ((pEvent->getOriginDeviceID() == _originDeviceID) || (_originDeviceID == 0))) {
          // dimming, adjust the old event's timestamp to keep it active
          if (SceneHelper::isDimSequence(_sceneID) && ((pEvent->getSceneID() == _sceneID) || (_sceneID == SceneDimArea))) {
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
                ", CallOrigin=" + intToString(_origin) +
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
                ", CallOrigin=" + intToString(_origin) +
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
                ", CallOrigin=" + intToString(_origin) +
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
                ", CallOrigin=" + intToString(_origin) +
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
                ", CallOrigin=" + intToString(_origin) +
                ", Scene=" + intToString(_sceneID));

            pEvent->setScene(_sceneID);
            return;
          }
        }
      }
    }

    log("CallSceneFilter: deferred processing, from source " + dsuid2str(_source) +
        ": Zone=" + intToString(_zoneID) +
        ", Group=" + intToString(_groupID) +
        ", OriginDevice=" + intToString(_originDeviceID) +
        ", CallOrigin=" + intToString(_origin) +
        ", OriginToken=" + _token +
        ", Scene=" + intToString(_sceneID), lsDebug);

    boost::shared_ptr<ModelDeferredSceneEvent> mEvent(new ModelDeferredSceneEvent(_source, _zoneID, _groupID, _originDeviceID, _sceneID, _origin, _forced, _token));
    m_DeferredEvents.push_back(mEvent);
  } // onGroupCallSceneFiltered

  void ModelMaintenance::onGroupBlink(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const callOrigin_t _origin, const std::string _token) {
    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      boost::shared_ptr<Group> group = zone->getGroup(_groupID);
      if(group != NULL) {
        log("OnGroupBlink: group-id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID));
        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event("blink", group));
        pEvent->setProperty("groupID", intToString(_groupID));
        pEvent->setProperty("zoneID", intToString(_zoneID));
        dsuid_t originDSUID;
        if ((!IsNullDsuid(_source)) && (_originDeviceID != 0)) {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(_originDeviceID, _source);
          originDSUID = devRef.getDSID();
        }
        pEvent->setProperty("callOrigin", intToString(_origin));
        pEvent->setProperty("originDSUID", dsuid2str(originDSUID));
        pEvent->setProperty("originToken", _token);
        raiseEvent(pEvent);
      } else {
        log("OnGroupBlink: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupBlink: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }
  } // onGroupBlink

  void ModelMaintenance::onDeviceActionFiltered(dsuid_t _source, const int _deviceID, const int _buttonNr, const int _clickType) {

    // Filter Strategy:
    // Check for Source != 0 and per DeviceID
    // - delayed Tipp processing, for 1T..4T
    // - generate final hold event after HE

    bool passThrough = false;

    // do not filter calls from myself
    if (IsNullDsuid(_source)) {
      passThrough = true;
    }

    try {
      boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_source);
      boost::shared_ptr<Device> dev = mod->getDevices().getByBusID(_deviceID, _source).getDevice();
      // ButtonInputMode Turbo (1) or Switched (2) do not generate multi-tip-sequences
      if (dev->getButtonInputMode() == 1 || dev->getButtonInputMode() == 2) {
        passThrough = true;
      }
    } catch (ItemNotFoundException& ex) {
    }

    if (passThrough) {
      log("DeviceActionFilter: pass through, from source " + dsuid2str(_source) +
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
      dsuid_t tmp_dsuid = pEvent->getSource();
      if ((pEvent->getDeviceID() == _deviceID) && (pEvent->getButtonIndex() == _buttonNr) && IsEqualDsuid(tmp_dsuid, _source)) {
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

    log("DeviceActionFilter: deferred processing, from source " + dsuid2str(_source) + "/" + intToString(_deviceID) +
        ": buttonIndex=" + intToString(_buttonNr) +
        ", clickType=" + intToString(_clickType), lsDebug);

    boost::shared_ptr<ModelDeferredButtonEvent> mEvent(new ModelDeferredButtonEvent(_source, _deviceID, _buttonNr, _clickType));
    m_DeferredEvents.push_back(mEvent);
  } // onDeviceActionFiltered

  void ModelMaintenance::onDeviceNameChanged(dsuid_t _meterID,
                                             const devid_t _deviceID,
                                             const std::string& _name) {
    log("Device name changed on the bus. Meter: " + dsuid2str(_meterID) +
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

  void ModelMaintenance::onDsmNameChanged(dsuid_t _meterID,
                                          const std::string& _name)
  {
      log("dSM name changed on the bus. Meter: " + dsuid2str(_meterID), lsInfo);
      try {
        boost::shared_ptr<DSMeter> pMeter =
          m_pApartment->getDSMeterByDSID(_meterID);
        pMeter->setName(_name);

      } catch(ItemNotFoundException& e) {
        log("onDsmNameChanged: Item not found: " + std::string(e.what()));
      }
  }

  void ModelMaintenance::onDsmFlagsChanged(dsuid_t _meterID,
                                          const std::bitset<8> _flags)
  {
      log("dSM flags changed on the bus. Meter: " + dsuid2str(_meterID), lsInfo);
      try {
        boost::shared_ptr<DSMeter> pMeter =
          m_pApartment->getDSMeterByDSID(_meterID);
        pMeter->setPropertyFlags(_flags);

      } catch(ItemNotFoundException& e) {
        log("onDsmFlagsChanged: Item not found: " + std::string(e.what()));
      }
  }

  void ModelMaintenance::onDeviceCallScene(const dsuid_t& _dsMeterID, const int _deviceID, const int _originDeviceID, const int _sceneID, const callOrigin_t _origin, const bool _forced, const std::string _token) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onDeviceCallScene: _sceneID is out of bounds. dsMeter-id '" + dsuid2str(_dsMeterID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID), lsError);
        return;
      }
      boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_dsMeterID);
      try {
        log("OnDeviceCallScene: dsMeter-id '" + dsuid2str(_dsMeterID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID));
        DeviceReference devRef = mod->getDevices().getByBusID(_deviceID, _dsMeterID);
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          devRef.getDevice()->setLastCalledScene(_sceneID & 0x00ff);
        }
        boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));
        boost::shared_ptr<Event> event(new Event(EventName::CallScene, pDevRev));
        event->setProperty("sceneID", intToString(_sceneID));
        event->setProperty("callOrigin", intToString(_origin));
        event->setProperty("originToken", _token);
        if (_forced) {
          event->setProperty("forced", "true");
        }
        raiseEvent(event);
      } catch(ItemNotFoundException& e) {
        log("OnDeviceCallScene: Could not find device with bus-id '" + intToString(_deviceID) + "' on dsMeter '" + dsuid2str(_dsMeterID) + "' scene:" + intToString(_sceneID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnDeviceCallScene: Could not find dsMeter with bus-id '" + dsuid2str(_dsMeterID) + "'", lsError);
    }
  } // onDeviceCallScene

  void ModelMaintenance::onDeviceBlink(const dsuid_t& _dsMeterID, const int _deviceID, const int _originDeviceID, const callOrigin_t _origin, const std::string _token) {
    try {
      boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_dsMeterID);
      try {
        log("OnDeviceBlink: dsMeter-id '" + dsuid2str(_dsMeterID) + "' for device '" + intToString(_deviceID));
        DeviceReference devRef = mod->getDevices().getByBusID(_deviceID, _dsMeterID);
        boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));
        boost::shared_ptr<Event> event(new Event("blink", pDevRev));
        event->setProperty("callOrigin", intToString(_origin));
        event->setProperty("originToken", _token);
        raiseEvent(event);
      } catch(ItemNotFoundException& e) {
        log("OnDeviceBlink: Could not find device with bus-id '" + intToString(_deviceID) + "' on dsMeter '" + dsuid2str(_dsMeterID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnDeviceBlink: Could not find dsMeter with bus-id '" + dsuid2str(_dsMeterID) + "'", lsError);
    }
  } // onDeviceBlink

  void ModelMaintenance::onDeviceActionEvent(const dsuid_t& _dsMeterID, const int _deviceID, const int _buttonNr, const int _clickType) {
    try {
      boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_dsMeterID);
      log("onDeviceActionEvent: dsMeter-id '" + dsuid2str(_dsMeterID) + "' for device '" + intToString(_deviceID) +
          "' button: " + intToString(_buttonNr) + ", click: " + intToString(_clickType));
      try {
        DeviceReference devRef = mod->getDevices().getByBusID(_deviceID, _dsMeterID);
        boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));
        boost::shared_ptr<Event> event(new Event("buttonClickBus", pDevRev));
        event->setProperty("clickType", intToString(_clickType));
        event->setProperty("buttonIndex", intToString(_buttonNr));
        raiseEvent(event);
      } catch(ItemNotFoundException& e) {
        log("onDeviceActionEvent: Could not find device with bus-id '" + intToString(_deviceID) + "' on dsMeter '" + dsuid2str(_dsMeterID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("onDeviceActionEvent: Could not find dsMeter with bus-id '" + dsuid2str(_dsMeterID) + "'", lsError);
    }
  } // onDeviceActionEvent

  void ModelMaintenance::onAddDevice(const dsuid_t& _dsMeterID, const int _zoneID, const int _devID) {
    log("Device discovered:");
    try {
      DeviceReference devRef = m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_devID, _dsMeterID);
      log("  DSID   : " +  dsuid2str(devRef.getDSID()));
      {
        boost::shared_ptr<DeviceReference> pDevRef(new DeviceReference(devRef));
        boost::shared_ptr<Event> mEvent(new Event("DeviceEvent", pDevRef));
        mEvent->setProperty("action", "added");
        if(DSS::hasInstance()) {
          DSS::getInstance()->getEventQueue().pushEvent(mEvent);
        }
      }
    } catch(ItemNotFoundException& e) {}
    log("  DSMeter: " +  dsuid2str(_dsMeterID));
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));

    rescanDevice(_dsMeterID, _devID);
    // model_ready sets initializing flag to false and we want to perform
    // this check only if a new device was discovered after the model ready
    // event
    if (!m_IsInitializing) {

      boost::shared_ptr<Device> device;
      try {
        DeviceReference devRef = m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_devID, _dsMeterID);
        device = devRef.getDevice();
      } catch (ItemNotFoundException &ex) {
        log("Device with id " + intToString(_devID) +
            " not found, not checking sensor assignments");
        return;
      }

      // if the newly added devices provides any sensors that are not
      // yet assigned in the zone: assign it automatically, UC 1.1, including
      // UC 8.1 check
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      StructureManipulator manipulator(*m_pStructureModifyingBusInterface,
                                       *m_pStructureQueryBusInterface,
                                       *m_pApartment);
      manipulator.autoAssignZoneSensors(zone);
    }
  } // onAddDevice

  void ModelMaintenance::onRemoveDevice(const dsuid_t& _dsMeterID, const int _zoneID, const int _devID) {
    log("Device disappeared:");
    try {
      DeviceReference devRef = m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_devID, _dsMeterID);
      log("  DSID   : " +  dsuid2str(devRef.getDSID()));
      {
        boost::shared_ptr<DeviceReference> pDevRef(new DeviceReference(devRef));
        boost::shared_ptr<Event> mEvent(new Event("DeviceEvent", pDevRef));
        mEvent->setProperty("action", "removed");
        if(DSS::hasInstance()) {
          DSS::getInstance()->getEventQueue().pushEvent(mEvent);
        }
      }
    } catch(ItemNotFoundException& e) {}
    log("  DSMeter: " +  dsuid2str(_dsMeterID));
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));

    try {
      boost::shared_ptr<DSMeter> dsMeter = m_pApartment->getDSMeterByDSID(_dsMeterID);
      boost::shared_ptr<Device> device = dsMeter->getDevices().getByBusID(_devID, _dsMeterID).getDevice();
      device->setIsPresent(false);

      // save time when device went inactive to apartment.xml
      addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    } catch(ItemNotFoundException& e) {
      log("Removed device " + dsuid2str(_dsMeterID) + " is not known to us");
    }
  } // onRemoveDevice

  void ModelMaintenance::onJoinedDSMeter(const dsuid_t& _dSMeterID) {
    boost::shared_ptr<DSMeter> meter;

    log("onJoinedDSMeter: dSM Connected: " + dsuid2str(_dSMeterID));
    try {
      meter = m_pApartment->getDSMeterByDSID(_dSMeterID);
    } catch (ItemNotFoundException& e) {
      log("dSM is ready, but it is not yet known, re-discovering devices");
      discoverDS485Devices();
      return;
    }
    meter->setIsPresent(true);
    meter->setIsConnected(true);
    meter->setIsValid(false);

    try {
      Set devices = m_pApartment->getDevices().getByLastKnownDSMeter(_dSMeterID);
      for (int i = 0; i < devices.length(); i++) {
        devices[i].getDevice()->setIsValid(false);
      }
    } catch(ItemNotFoundException& e) {
      log("onJoinedDSMeter: error resetting device valid flags: " + std::string(e.what()));
    }
  } // onJoinedDSMeter

  void ModelMaintenance::onLostDSMeter(const dsuid_t& _dSMeterID) {
    log("dSM disconnected: " + dsuid2str(_dSMeterID));
    try {
      boost::shared_ptr<DSMeter> meter =
                              m_pApartment->getDSMeterByDSID(_dSMeterID);
      meter->setIsPresent(false);
      meter->setIsConnected(false);
      meter->setPowerConsumption(0);
      Set devices = meter->getDevices();
      for (int i = 0; i < devices.length(); i++) {
        devices[i].getDevice()->setIsConnected(false);
      }
    } catch(ItemNotFoundException& e) {
      log("Lost dSM " + dsuid2str(_dSMeterID) + " was not in our list");
    }
  }

  void ModelMaintenance::onDeviceConfigChanged(const dsuid_t& _dsMeterID, int _deviceID,
                                               int _configClass, int _configIndex, int _value) {
    try {
      DeviceReference devRef = m_pApartment->getDevices().getByBusID(_deviceID, _dsMeterID);
      boost::shared_ptr<Device> device = devRef.getDevice();
      if(_configClass == CfgClassFunction) {
        if(_configIndex == CfgFunction_Mode) {
          device->setOutputMode(_value);
        } else if(_configIndex == CfgFunction_ButtonMode) {
          device->setButtonID(_value & 0xf);
          device->setButtonGroupMembership((_value >> 4) & 0xf);
        } else if(_configIndex == CfgFunction_LTMode) {
          device->setButtonInputMode(_value);
        }
      } else if (_configClass == CfgClassDevice) {
        if(_configIndex >= 0x40 && _configIndex <= 0x40 + 8 * 3) {
          uint8_t inputIndex = (_configIndex - 0x40) / 3;
          uint8_t offset = (_configIndex - 0x40) % 3;
          switch (offset) {
          case 0:
            device->setBinaryInputTarget(inputIndex, _value >> 6, _value & 0x3f);
            break;
          case 1:
            device->setBinaryInputType(inputIndex, _value & 0xff);
            break;
          case 2:
            device->setBinaryInputId(inputIndex, _value >> 4);
            break;
          }
        }
      }
      {
        boost::shared_ptr<DeviceReference> pDevRef(new DeviceReference(devRef));
        boost::shared_ptr<Event> mEvent(new Event("DeviceEvent", pDevRef));
        mEvent->setProperty("action", "configure");
        if(DSS::hasInstance()) {
          DSS::getInstance()->getEventQueue().pushEvent(mEvent);
        }
      }
    } catch(std::runtime_error& e) {
      log(std::string("Error updating config of device: ") + e.what(), lsError);
    }
  } // onDeviceConfigChanged

  void ModelMaintenance::onSensorEvent(dsuid_t _meterID,
                                       const devid_t _deviceID,
                                       const int& _eventIndex) {
    try {
      boost::shared_ptr<DSMeter> pMeter =
        m_pApartment->getDSMeterByDSID(_meterID);
      DeviceReference devRef = pMeter->getDevices().getByBusID(_deviceID, pMeter);
      boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));

      std::string eventName = pDevRev->getSensorEventName(_eventIndex);
      if (eventName.empty()) {
        eventName = "event" + intToString(_eventIndex);
      }
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("deviceSensorEvent", pDevRev));
      pEvent->setProperty("sensorEvent", eventName);
      pEvent->setProperty("sensorIndex", "event" + intToString(_eventIndex));
      raiseEvent(pEvent);
    } catch(ItemNotFoundException& e) {
      log("onSensorEvent: Datamodel failure: " + std::string(e.what()), lsWarning);
    }
  } // onSensorEvent

  void ModelMaintenance::onBinaryInputEvent(dsuid_t _meterID,
      const devid_t _deviceID, const int& _eventIndex, const int& _eventType, const int& _state) {
    try {
      boost::shared_ptr<DSMeter> pMeter = m_pApartment->getDSMeterByDSID(_meterID);
      DeviceReference devRef = pMeter->getDevices().getByBusID(_deviceID, pMeter);
      boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));

      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("deviceBinaryInputEvent", pDevRev));
      pEvent->setProperty("inputIndex", intToString(_eventIndex));
      pEvent->setProperty("inputType", intToString(_eventType));
      pEvent->setProperty("inputState", intToString(_state));
      raiseEvent(pEvent);

      boost::shared_ptr<State> pState;
      try {
        pState = devRef.getDevice()->getBinaryInputState(_eventIndex);
      } catch(std::runtime_error& e) {}

      if (pState != NULL) {
        if (_state == 0) {
          pState->setState(coSystem, State_Inactive);
        } else if (_state == 1) {
          pState->setState(coSystem, State_Active);
        }
      }

      // increment event counter as last step to catch possible
      // data model exceptions in above sequence
      try {
        pMeter->incrementBinaryInputEventCount();
      } catch(DSSException& e) {
        log("onBinaryInputEvent: " + std::string(e.what()), lsWarning);
      }

    } catch(ItemNotFoundException& e) {
      log("onBinaryInputEvent: Datamodel failure: " + std::string(e.what()), lsWarning);
    }
  } // onBinaryInputEvent

  void ModelMaintenance::onSensorValue(dsuid_t _meterID,
                                       const devid_t _deviceID,
                                       const int& _sensorIndex,
                                       const int& _sensorValue) {
    try {
      boost::shared_ptr<DSMeter> pMeter = m_pApartment->getDSMeterByDSID(_meterID);
      DeviceReference devRef = pMeter->getDevices().getByBusID(_deviceID, pMeter);
      boost::shared_ptr<DeviceReference> pDevRev(new DeviceReference(devRef));
      boost::shared_ptr<Device> pDev = devRef.getDevice();

      // binary input state synchronization event
      if (_sensorIndex == 32) {
        for (int index = 0; index < pDev->getBinaryInputCount(); index++) {
          boost::shared_ptr<State> state;
          try {
            state = pDev->getBinaryInputState(index);
          } catch (ItemNotFoundException& e) {
            continue;
          }
          eState oldState = state->getState();
          eState newState;
          if ((_sensorValue & (1 << index)) > 0) {
            newState = State_Active;
          } else {
            newState = State_Inactive;
          }
          if (newState != oldState) {
            state->setState(coSystem, newState);

            boost::shared_ptr<Event> pEvent;
            pEvent.reset(new Event("deviceBinaryInputEvent", pDevRev));
            pEvent->setProperty("inputIndex", intToString(index));
            pEvent->setProperty("inputType", intToString(pDev->getDeviceBinaryInputType(index)));
            pEvent->setProperty("inputState", intToString(newState));
            raiseEvent(pEvent);
          }
        }

      // device status and error event
      } else if (_sensorIndex <= 31 && _sensorIndex >= 16) {
        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event(EventName::DeviceStatus, pDevRev));
        pEvent->setProperty("statusIndex", intToString(_sensorIndex));
        pEvent->setProperty("statusValue", intToString(_sensorValue));
        raiseEvent(pEvent);

      // regular sensor value event
      } else if (_sensorIndex <= 15) {
        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event(EventName::DeviceSensorValue, pDevRev));
        pEvent->setProperty("sensorIndex", intToString(_sensorIndex));
        pEvent->setProperty("sensorValue", intToString(_sensorValue));
        try {
          pDev->setSensorValue(_sensorIndex, (const unsigned int) _sensorValue);

          boost::shared_ptr<DeviceSensor_t> pdSensor = pDev->getSensor(_sensorIndex);
          uint8_t sensorType = pdSensor->m_sensorType;
          pEvent->setProperty("sensorType", intToString(sensorType));

          double fValue = SceneHelper::sensorToFloat12(sensorType, _sensorValue);
          pEvent->setProperty("sensorValueFloat", doubleToString(fValue));
        } catch (ItemNotFoundException& e) {}
        raiseEvent(pEvent);
      }
    } catch(ItemNotFoundException& e) {
      log("onSensorValue: Datamodel failure: " + std::string(e.what()), lsWarning);
    }
  } // onSensorValue

  void ModelMaintenance::onZoneSensorValue(dsuid_t _meterID,
                                           const std::string& _sourceDevice,
                                           const int& _zoneID,
                                           const int& _groupID,
                                           const int& _sensorType,
                                           const int& _sensorValue,
                                           const int& _precision) {
    try {
      boost::shared_ptr<Event> pEvent;
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      boost::shared_ptr<Group> group = zone->getGroup(_groupID);

      double fValue = SceneHelper::sensorToFloat12(_sensorType, _sensorValue);
      group->sensorPush(_sourceDevice, _sensorType, fValue);

      pEvent.reset(new Event(EventName::ZoneSensorValue, group));
      pEvent->setProperty("sensorType", intToString(_sensorType));
      pEvent->setProperty("sensorValue", intToString(_sensorValue));
      pEvent->setProperty("sensorValueFloat", doubleToString(fValue));
      pEvent->setProperty("originDSID", _sourceDevice);
      raiseEvent(pEvent);
    } catch(ItemNotFoundException& e) {
      log("onZoneSensorValue: Datamodel failure: " + std::string(e.what()), lsWarning);
    }
  } // onZoneSensorValue

  void ModelMaintenance::onEANReady(dsuid_t _dsMeterID,
                                        const devid_t _deviceID,
                                        const DeviceOEMState_t _state,
                                        const DeviceOEMInetState_t _iNetState,
                                        const unsigned long long _eanNumber,
                                        const int _serialNumber,
                                        const int _partNumber,
                                        const bool _isIndependent,
                                        const bool _isConfigLocked) {
    try {
      DeviceReference devRef = m_pApartment->getDevices().getByBusID(_deviceID, _dsMeterID);
      if (_state == DEVICE_OEM_VALID) {
        devRef.getDevice()->setOemInfo(_eanNumber, _serialNumber, _partNumber, _iNetState, _isIndependent);
        if ((_iNetState == DEVICE_OEM_EAN_INTERNET_ACCESS_OPTIONAL) ||
            (_iNetState == DEVICE_OEM_EAN_INTERNET_ACCESS_MANDATORY)) {
          // query Webservice
          getTaskProcessor()->addEvent(boost::shared_ptr<OEMWebQuery>(new OEMWebQuery(devRef.getDevice())));
          devRef.getDevice()->setOemProductInfoState(DEVICE_OEM_LOADING);
        } else {
          devRef.getDevice()->setOemProductInfoState(DEVICE_OEM_NONE);
        }
      }
      devRef.getDevice()->setOemInfoState(_state);
      devRef.getDevice()->setConfigLock(_isConfigLocked);
    } catch(std::runtime_error& e) {
      log(std::string("Error updating OEM data of device: ") + e.what(), lsWarning);
    }
  } // onEANReady

  void ModelMaintenance::onOEMDataReady(dsuid_t _dsMeterID,
                                             const devid_t _deviceID,
                                             const DeviceOEMState_t _state,
                                             const std::string& _productName,
                                             const std::string& _iconPath,
                                             const std::string& _productURL,
                                             const std::string& _defaultName) {
    try {
      DeviceReference devRef = m_pApartment->getDevices().getByBusID(_deviceID, _dsMeterID);
      if (_state == DEVICE_OEM_VALID) {
        devRef.getDevice()->setOemProductInfo(_productName, _iconPath, _productURL);
        if (devRef.getDevice()->getName().empty()) {
          devRef.getDevice()->setName(_defaultName);
        }
      }
      devRef.getDevice()->setOemProductInfoState(_state);
    } catch(std::runtime_error& e) {
      log(std::string("Error updating OEM data of device: ") + e.what(), lsWarning);
    }
  } // onEANReady

  void ModelMaintenance::onHeatingControllerConfig(dsuid_t _dsMeterID, const int _zoneID, boost::shared_ptr<void> _spec) {
    boost::shared_ptr<ZoneHeatingConfigSpec_t> config =
        boost::static_pointer_cast<ZoneHeatingConfigSpec_t> (_spec);
    ZoneHeatingProperties_t hProp;

    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      hProp = zone->getHeatingProperties();
      zone->setHeatingControlMode(config->ControllerMode, config->Offset, config->SourceZoneId, config->ManualValue, _dsMeterID);
    } catch(ItemNotFoundException& e) {
      log(std::string("Error on heating control config event, item not found: ") + e.what(), lsWarning);
      return;
    }

    log(std::string("onHeatingControllerConfig:  dsMeter " + dsuid2str(_dsMeterID) +
        ", current controller " + dsuid2str(hProp.m_HeatingControlDSUID), lsInfo));

    boost::shared_ptr<Event> pEvent;
    pEvent.reset(new Event(EventName::HeatingControllerSetup));
    pEvent->setProperty("ZoneID", intToString(_zoneID));
    pEvent->setProperty("ControlDSUID", dsuid2str(_dsMeterID));
    pEvent->setProperty("ControlMode", intToString(config->ControllerMode));
    pEvent->setProperty("EmergencyValue", intToString(config->EmergencyValue - 100));
    if (config->ControllerMode == HeatingControlModeIDPID) {
      pEvent->setProperty("CtrlKp", intToString(config->Kp));
      pEvent->setProperty("CtrlTs", intToString(config->Ts));
      pEvent->setProperty("CtrlTi", intToString(config->Ti));
      pEvent->setProperty("CtrlKd", intToString(config->Kd));
      pEvent->setProperty("CtrlImin", intToString(config->Imin));
      pEvent->setProperty("CtrlImax", intToString(config->Imax));
      pEvent->setProperty("CtrlYmin", intToString(config->Ymin));
      pEvent->setProperty("CtrlYmax", intToString(config->Ymax));
      pEvent->setProperty("CtrlAntiWindUp", (config->AntiWindUp > 0) ? "true" : "false");
      pEvent->setProperty("CtrlKeepFloorWarm", (config->KeepFloorWarm > 0) ? "true" : "false");
    } else if (config->ControllerMode == HeatingControlModeIDZoneFollower) {
      pEvent->setProperty("ReferenceZone", intToString(config->SourceZoneId));
      pEvent->setProperty("CtrlOffset", intToString(config->Offset - 100));
    } else if (config->ControllerMode == HeatingControlModeIDManual) {
      pEvent->setProperty("ManualValue", intToString(config->ManualValue - 100));
    }
    raiseEvent(pEvent);
  } // onHeatingControllerConfig

  void ModelMaintenance::onHeatingControllerValues(dsuid_t _dsMeterID, const int _zoneID, boost::shared_ptr<void> _spec) {
    boost::shared_ptr<ZoneHeatingOperationModeSpec_t> values =
        boost::static_pointer_cast<ZoneHeatingOperationModeSpec_t> (_spec);
    ZoneHeatingProperties_t hProp;

    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      hProp = zone->getHeatingProperties();
    } catch(ItemNotFoundException& e) {
      log(std::string("Error on heating control value event, item not found: ") + e.what(), lsWarning);
      return;
    }

    log(std::string("onHeatingControllerValues:  dsMeter " + dsuid2str(_dsMeterID) +
        ", current controller " + dsuid2str(hProp.m_HeatingControlDSUID), lsInfo));

    boost::shared_ptr<Event> pEvent;
    pEvent.reset(new Event(EventName::HeatingControllerValue));
    pEvent->setProperty("ZoneID", intToString(_zoneID));
    pEvent->setProperty("ControlDSUID", dsuid2str(_dsMeterID));
    if (hProp.m_HeatingControlMode == HeatingControlModeIDPID) {
      pEvent->setProperty("NominalTemperature_Off",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, values->OpMode0)));
      pEvent->setProperty("NominalTemperature_Comfort",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, values->OpMode1)));
      pEvent->setProperty("NominalTemperature_Economy",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, values->OpMode2)));
      pEvent->setProperty("NominalTemperature_NotUsed",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, values->OpMode3)));
      pEvent->setProperty("NominalTemperature_Night",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, values->OpMode4)));
      pEvent->setProperty("NominalTemperature_Holiday",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, values->OpMode5)));
    } else if (hProp.m_HeatingControlMode == HeatingControlModeIDFixed) {
      pEvent->setProperty("ControlValue_Off",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, values->OpMode0)));
      pEvent->setProperty("ControlValue_Comfort",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, values->OpMode1)));
      pEvent->setProperty("ControlValue_Economy",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, values->OpMode2)));
      pEvent->setProperty("ControlValue_NotUsed",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, values->OpMode3)));
      pEvent->setProperty("ControlValue_Night",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, values->OpMode4)));
      pEvent->setProperty("ControlValue_Holiday",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, values->OpMode5)));
    }
    raiseEvent(pEvent);
  } // onHeatingControllerValues

  void ModelMaintenance::onHeatingControllerState(dsuid_t _dsMeterID, const int _zoneID, const int _State) {

    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      zone->setHeatingControlState(_State);
    } catch(ItemNotFoundException& e) {
      log(std::string("Error on heating state event, item not found: ") + e.what(), lsWarning);
    }

    boost::shared_ptr<Event> pEvent;
    pEvent.reset(new Event(EventName::HeatingControllerState));
    pEvent->setProperty("ZoneID", intToString(_zoneID));
    pEvent->setProperty("ControlDSUID", dsuid2str(_dsMeterID));
    pEvent->setProperty("ControlState", intToString(_State));
    raiseEvent(pEvent);
  } // onHeatingControllerState

  void ModelMaintenance::rescanDevice(const dsuid_t& _dsMeterID, const int _deviceID) {
    BusScanner
      scanner(
        *m_pStructureQueryBusInterface,
        *m_pApartment,
        *this);
    try {
      boost::shared_ptr<DSMeter> dsMeter = m_pApartment->getDSMeterByDSID(_dsMeterID);
      scanner.scanDeviceOnBus(dsMeter, _deviceID);
      boost::shared_ptr<Device> device = dsMeter->getDevices().getByBusID(_deviceID, dsMeter).getDevice();
      if (device->getBinaryInputCount() > 0) {
        scanner.syncBinaryInputStates(dsMeter, device);
      }
    } catch(ItemNotFoundException& e) {
      log(std::string("Error scanning device, item not found: ") + e.what(), lsWarning);
    } catch(std::runtime_error& e) {
      log(std::string("Error scanning device: ") + e.what(), lsWarning);
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

  void ModelMaintenance::setStructureModifyingBusInterface(StructureModifyingBusInterface* _value) {
    m_pStructureModifyingBusInterface = _value;
  } // setStructureModifyingBusInterface

  ModelMaintenance::OEMWebQuery::OEMWebQuery(boost::shared_ptr<Device> _device)
    : Task()
  {
    m_deviceAdress = _device->getShortAddress();
    m_dsmId = _device->getDSMeterDSID();
    m_EAN = _device->getOemEanAsString();
    m_partNumber = _device->getOemPartNumber();
    m_serialNumber = _device->getOemSerialNumber();
  }

  ModelMaintenance::OEMWebQuery::OEMWebQueryCallback::OEMWebQueryCallback(dsuid_t dsmId, devid_t deviceAddress)
    : m_dsmId(dsmId)
    , m_deviceAddress(deviceAddress)
  {
  }

  void ModelMaintenance::OEMWebQuery::OEMWebQueryCallback::result(long code, const std::string &result)
  {
    std::string productURL;
    std::string productName;
    boost::filesystem::path iconFile;
    std::string defaultName;
    DeviceOEMState_t state = DEVICE_OEM_UNKOWN;

    if (code == 200) {
      Logger::getInstance()->log(std::string("OEMWebQueryCallback::result: result: ") + result);
      struct json_tokener* tok;

      tok = json_tokener_new();
      json_object* json_request = json_tokener_parse_ex(tok, result.c_str(), -1);

      boost::filesystem::path remoteIconPath;
      if (tok->err == json_tokener_success) {
        int resultCode = 99;
        json_object* obj;
        if (json_object_object_get_ex(json_request, "ReturnCode", &obj)) {
          resultCode = json_object_get_int(obj);
        }
        if (resultCode != 0) {
          std::string errorMessage;
          if (json_object_object_get_ex(json_request, "ReturnMessage", &obj)) {
            errorMessage = json_object_get_string(obj);
          }
          Logger::getInstance()->log(std::string("OEMWebQueryCallback::result: JSON-ERROR: ") + errorMessage, lsError);
        } else {
          json_object *result;
          if (json_object_object_get_ex(json_request, "Response", &result)) {
            Logger::getInstance()->log(std::string("OEMWebQueryCallback::result: no 'result' object in response"), lsError);
          } else {
            if (json_object_object_get_ex(result, "ArticleName", &obj)) {
              productName = json_object_get_string(obj);
            }

            if (json_object_object_get_ex(result, "ArticleIcon", &obj)) {
              remoteIconPath = json_object_get_string(obj);
            }

            if (json_object_object_get_ex(result,
                                          "ArticleDescriptionForCustomer",
                                          &obj)) {
              productURL = json_object_get_string(obj);
            }

            if (json_object_object_get_ex(result, "DefaultName", &obj)) {
              defaultName = json_object_get_string(obj);
            }
          }
        }
      }
      json_object_put(json_request);
      json_tokener_free(tok);

      state = DEVICE_OEM_VALID;
      iconFile = remoteIconPath.filename();
      if (!remoteIconPath.empty()) {
        HttpClient url;
        long res;
        std::string iconURL = remoteIconPath.string();

        boost::filesystem::path iconBasePath;
        PropertySystem propSys = DSS::getInstance()->getPropertySystem();
        if (DSS::hasInstance()) {
          DSS::getInstance()->getSecurity().loginAsSystemUser("OEMWebQuery needs system-rights");
          iconBasePath = propSys.getStringValue(
                  "/config/subsystems/Apartment/iconBasePath");
        } else {
          return;
        }

        boost::filesystem::path iconPath = iconBasePath / iconFile;
        res = url.downloadFile(iconURL, iconPath.string());
        if (res != 200) {
          Logger::getInstance()->log("OEMWebQueryCallback::result: could not download OEM "
              "icon from: " + iconURL + std::string("; error: ") + intToString(res), lsWarning);
          iconFile.clear();
          try {
            if (boost::filesystem::exists(iconPath)) {
              boost::filesystem::remove(iconPath);
            }
          } catch (boost::filesystem::filesystem_error& e) {
            Logger::getInstance()->log("OEMWebQueryCallback::result: cannot delete "
                "(incomplete) icon: " + std::string(e.what()), lsWarning);
          }
        }
      }
    } else {
      Logger::getInstance()->log("OEMWebQueryCallback::result: could not download OEM "
          "data. Error: " + intToString(code) + "Message: " + result, lsWarning);
    }

    ModelEventWithStrings* pEvent = new ModelEventWithStrings(ModelEvent::etDeviceOEMDataReady, m_dsmId);
    pEvent->addParameter(m_deviceAddress);
    pEvent->addParameter(state);
    pEvent->addStringParameter(productName);
    pEvent->addStringParameter(iconFile.string());
    pEvent->addStringParameter(productURL);
    pEvent->addStringParameter(defaultName);
    if(DSS::hasInstance()) {
      DSS::getInstance()->getModelMaintenance().addModelEvent(pEvent);
    }
  }

  void ModelMaintenance::OEMWebQuery::run()
  {
    PropertySystem propSys = DSS::getInstance()->getPropertySystem();
    std::string mac = propSys.getStringValue("/system/host/interfaces/eth0/mac");
    std::string country = propSys.getStringValue("/config/geodata/country");
    std::string language = propSys.getStringValue("/system/language/locale");

    std::string parameters = "ean=" + m_EAN +
                             "&partNr=" + intToString(m_partNumber) +
                             "&oemSerialNumber=" + intToString(m_serialNumber) +
                             "&macAddress=" + mac +
                             "&countryCode=" + country +
                             "&languageCode=" + language;

    boost::shared_ptr<OEMWebQuery::OEMWebQueryCallback> cb(new OEMWebQuery::OEMWebQueryCallback(m_dsmId, m_deviceAdress));
    WebserviceConnection::getInstance()->request("public/MasterDataManagement/Article/v1_0/ArticleData/GetArticleData", parameters, GET, cb, false);
  }

  ModelMaintenance::VdcDataQuery::VdcDataQuery(boost::shared_ptr<Device> _device)
    : Task(),
      m_Device(_device)
  {}

  void ModelMaintenance::VdcDataQuery::run()
  {
    try {
      boost::shared_ptr<VdsdSpec_t> props = VdcHelper::getSpec(m_Device->getDSMeterDSID(), m_Device->getDSID());
      m_Device->setVdcModelGuid(props->modelGuid);
      m_Device->setVdcVendorGuid(props->vendorGuid);
      m_Device->setVdcOemGuid(props->oemGuid);
      m_Device->setVdcConfigURL(props->configURL);
      m_Device->setVdcHardwareGuid(props->hardwareGuid);
      m_Device->setVdcHardwareInfo(props->hardwareInfo);
      m_Device->setVdcHardwareVersion(props->hardwareVersion);
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("VdcDataQuery: could not query properties from " +
          dsuid2str(m_Device->getDSID()) + ", Message: " + e.what(), lsWarning);
    }

    uint8_t *data;
    size_t dataSize = 0;

    try {
      VdcHelper::getIcon(m_Device->getDSMeterDSID(), m_Device->getDSID(), &dataSize, &data);
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("VdcDataQuery: could not query icon data from " +
          dsuid2str(m_Device->getDSID()) + ", Message: " + e.what(), lsWarning);
    }
    if (dataSize > 0) {
      boost::filesystem::path iconBasePath;
      boost::filesystem::path iconFile;
      PropertySystem propSys = DSS::getInstance()->getPropertySystem();
      iconBasePath = propSys.getStringValue("/config/subsystems/Apartment/iconBasePath");
      iconFile = dsuid2str(m_Device->getDSID()) + ".png";
      boost::filesystem::path iconPath = iconBasePath / iconFile;

      /* write binary file from memory */
      FILE *binFile = fopen(iconPath.c_str(), "wb");
      if (binFile) {
        fwrite(data, sizeof(uint8_t), dataSize, binFile);
        fclose(binFile);
        m_Device->setVdcIconPath(iconFile.string());
      }
      free(data);
    }
  }

  const std::string ModelMaintenance::kWebUpdateEventName = "ModelMaintenace_updateWebData";

  void ModelMaintenance::setupWebUpdateEvent() {
    if (!DSS::hasInstance()) {
      log(std::string("ModelMaintenance::setupWebUpdateEvent: no DSS instance available; not starting WebUpdateProcess"));
      return;
    }
    EventInterpreterInternalRelay* pRelay =
      dynamic_cast<EventInterpreterInternalRelay*>(DSS::getInstance()->getEventInterpreter().getPluginByName(EventInterpreterInternalRelay::getPluginName()));
    m_pRelayTarget = boost::shared_ptr<InternalEventRelayTarget>(new InternalEventRelayTarget(*pRelay));

    boost::shared_ptr<EventSubscription> updateWebSubscription(
            new dss::EventSubscription(
                kWebUpdateEventName,
                EventInterpreterInternalRelay::getPluginName(),
                DSS::getInstance()->getEventInterpreter(),
                boost::shared_ptr<SubscriptionOptions>())
    );
    m_pRelayTarget->subscribeTo(updateWebSubscription);
    m_pRelayTarget->setCallback(boost::bind(&ModelMaintenance::updateWebData, this, _1, _2));
    sendWebUpdateEvent(5);
  } // setupCleanupEvent

  void ModelMaintenance::updateWebData(Event& _event, const EventSubscription& _subscription) {
    std::vector<boost::shared_ptr<Device> > deviceVec = m_pApartment->getDevicesVector();
    foreach(boost::shared_ptr<Device> device, deviceVec) {
      if ((device->getOemInfoState() == DEVICE_OEM_VALID) &&
           ((device->getOemProductInfoState() == DEVICE_OEM_VALID) ||
            (device->getOemProductInfoState() == DEVICE_OEM_UNKOWN))) {
        // query Webservice
        getTaskProcessor()->addEvent(boost::shared_ptr<OEMWebQuery>(new OEMWebQuery(device)));
        device->setOemProductInfoState(DEVICE_OEM_LOADING);
      }
    }
    sendWebUpdateEvent();
  } // cleanupTerminatedScripts

  void ModelMaintenance::sendWebUpdateEvent(int _interval) {
    boost::shared_ptr<Event> pEvent(new Event(kWebUpdateEventName));
    pEvent->setProperty("time", "+" + intToString(_interval));
    DSS::getInstance()->getEventInterpreter().getQueue().pushEvent(pEvent);
  } // sendCleanupEvent

} // namespace dss
