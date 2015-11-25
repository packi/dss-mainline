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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#define BOOST_CHRONO_HEADER_ONLY
#include <boost/make_shared.hpp>
#include <boost/chrono.hpp>

#include "modelmaintenance.h"

#include <unistd.h>
#include <json.h>

#include "foreach.h"
#include "base.h"
#include "dss.h"
#include "businterface.h"
#include "propertysystem.h"
#include "model/modelconst.h"
#include "model/autoclustermaintenance.h"
#include "metering/metering.h"
#include "security/security.h"
#include "structuremanipulator.h"

#include "apartment.h"
#include "zone.h"
#include "group.h"
#include "cluster.h"
#include "device.h"
#include "devicereference.h"
#include "event.h"
#include "event/event_create.h"
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
#include "model-features.h"
#include "handler/system_states.h"
#include "sqlite3_wrapper.h"


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

 //=============================================== MeterMaintenance
  __DEFINE_LOG_CHANNEL__(MeterMaintenance, lsInfo);


  static const int MAX_RETRY_COUNT = 30;

  MeterMaintenance::MeterMaintenance(DSS* _pDSS, const std::string& _name) :
    Thread(_name),
    m_pDSS(_pDSS),
    m_pApartment(NULL), // lazy initialization
    m_pModifyingBusInterface(NULL),
    m_pQueryBusInterface(NULL),
    m_IsInitializing(true),
    m_triggerSynchronize(false),
    m_retryCount(0)
  {
  }

  void MeterMaintenance::triggerMeterSynchronization()
  {
    boost::mutex::scoped_lock lock(m_syncMutex);
    m_triggerSynchronize = true;
  }

  void MeterMaintenance::execute()
  {
    static const int METER_DELAY_TIME = 1000;

    if(m_pDSS == NULL) {
      log("DSS Instance is NULL. Can not continue", lsFatal);
      return;
    }
    m_pApartment = &m_pDSS->getApartment();
    m_pModifyingBusInterface = m_pDSS->getBusInterface().getStructureModifyingBusInterface();
    m_pQueryBusInterface =  m_pDSS->getBusInterface().getStructureQueryBusInterface();

    assert(m_pApartment);
    assert(m_pModifyingBusInterface);
    assert(m_pQueryBusInterface);

    if(DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser("ModelMaintenance needs system-rights");
    }

    while (!m_Terminated) {
      readOutPendingMeter();
      synchronizeMeters();
      sleepMS(METER_DELAY_TIME);
    }
  }

  void MeterMaintenance::synchronizeMeters() {
    if (m_triggerSynchronize) {
      {
        boost::mutex::scoped_lock lock(m_syncMutex);
        m_triggerSynchronize = false;
      }
      synchronizeZoneSensorAssignment();
      autoAssignSensors();
    }
  }

  int MeterMaintenance::getNumValiddSMeters() const {
    int cntReadOutMeters = 0;
    foreach(boost::shared_ptr<DSMeter> pDSMeter,  m_pApartment->getDSMeters()) {
      if (busMemberIsdSM(pDSMeter->getBusMemberType()) &&
          pDSMeter->isPresent() &&
          pDSMeter->isValid()) {
        ++cntReadOutMeters;
      }
    }
    return cntReadOutMeters;
  }

  void MeterMaintenance::readOutPendingMeter() {
    bool hadToUpdate = false;
    foreach(boost::shared_ptr<DSMeter> pDSMeter,  m_pApartment->getDSMeters()) {
      if (pDSMeter->isPresent() &&
          (!pDSMeter->isValid())) {
        // call for all bus participants (vdc, dsm,..)
        dsMeterReady(pDSMeter->getDSID());
        // only for non virtual devices
        if (busMemberIsdSM(pDSMeter->getBusMemberType())) {
          hadToUpdate = true;
          break;
        }
      }
    }

    // If dSMeter configuration has changed we need to synchronize user-groups
    if (!m_IsInitializing && hadToUpdate) {
      synchronizeGroups(m_pApartment, m_pModifyingBusInterface);
    }

    // If we didn't have to update for one cycle, assume that we're done
    if (m_IsInitializing && !hadToUpdate) {
      int cntReadOutMeters = getNumValiddSMeters();
      int busMemberCount = getdSMBusMemberCount();
      log("Initializing: ReadOutMeters: " +
          intToString(cntReadOutMeters) +
          " BusMemberCounts: " + intToString(busMemberCount), lsDebug);
      if (cntReadOutMeters == busMemberCount) {
        setupInitializedState();
      } else {
        monitorInitialization();
      }
    }
  } // readOutPendingMeter

  void MeterMaintenance::setupInitializedState() {
    synchronizeGroups(m_pApartment, m_pModifyingBusInterface);
    log("******** Finished loading model from dSM(s)...", lsInfo);
    m_IsInitializing = false;
    m_pDSS->getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    setApartmentState();
    triggerMeterSynchronization();
    {
      m_pDSS->getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etMeterReady));
      boost::shared_ptr<Event> readyEvent = boost::make_shared<Event>(EventName::ModelReady);
      raiseEvent(readyEvent);
      try {
        CommChannel::getInstance()->resumeUpdateTask();
        CommChannel::getInstance()->requestLockedScenes();
      } catch (std::runtime_error &err) {
        log(err.what(), lsError);
      }
    }
    AutoClusterMaintenance maintenance(m_pApartment);
    maintenance.joinIdenticalClusters();
  } // setupInitializedState

  void MeterMaintenance::monitorInitialization() {
    ++m_retryCount;
    if (m_retryCount >= MAX_RETRY_COUNT) {
      m_retryCount  = 0;
      log("", lsInfo);
      ModelEvent* pEvent = new ModelEvent(ModelEvent::etBusReady);
      m_pDSS->getModelMaintenance().addModelEvent(pEvent);
    }
  } // monitorInitialization

  int MeterMaintenance::getdSMBusMemberCount() {
    if (m_pQueryBusInterface == NULL) {
      return -1;
    }
    int busMemberCount = 0;
    try {
      std::vector<DSMeterSpec_t> vecMeterSpec =  m_pQueryBusInterface->getBusMembers();
      foreach (DSMeterSpec_t spec, vecMeterSpec) {
        log("getdSMBusMemberCount Device: " + dsuid2str(spec.DSID) + 
            " Device Type: " + intToString(spec.DeviceType), lsDebug);
        if (busMemberIsdSM(spec.DeviceType)) {
          // ignore dSMx with older api version
          if (spec.APIVersion >= 0x300) {
            ++busMemberCount;
            log("getdSMBusMemberCount Device: " + dsuid2str(spec.DSID) + " counting.", lsDebug);
          }
        }
      }
      return busMemberCount;
    } catch(BusApiError& e) {
      log(std::string("Bus error getting dSM bus member count. ") + e.what(), lsError);
    }
    return -1;
  }

  void MeterMaintenance::dsMeterReady(const dsuid_t& _dsMeterBusID) {
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

        BusScanner scanner(*m_pQueryBusInterface, *m_pApartment, *m_pApartment->getModelMaintenance());
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

        boost::shared_ptr<Event> dsMeterReadyEvent = boost::make_shared<Event>(EventName::DSMeterReady);
        dsMeterReadyEvent->setProperty("dsMeter", dsuid2str(mod->getDSID()));
        raiseEvent(dsMeterReadyEvent);

      } catch(BusApiError& e) {
        log(std::string("Bus error scanning dSM " + dsuid2str(_dsMeterBusID) + " : ") + e.what(), lsFatal);
        ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etDSMeterReady, _dsMeterBusID);
        m_pApartment->getModelMaintenance()->addModelEvent(pEvent);
      }
    } catch(ItemNotFoundException& e) {
      log("dsMeterReady " + dsuid2str(_dsMeterBusID) + ": item not found: " + std::string(e.what()), lsError);
    }
  } // dsMeterReady

  void MeterMaintenance::setApartmentState() {
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

  void MeterMaintenance::autoAssignSensors() {
    StructureManipulator manipulator(*m_pModifyingBusInterface,
                                     *m_pQueryBusInterface,
                                     *m_pApartment);

    foreach (boost::shared_ptr<Zone> zone, m_pDSS->getApartment().getZones()) {
      manipulator.autoAssignZoneSensors(zone);
    }
  }

  void MeterMaintenance::synchronizeZoneSensorAssignment() {

    StructureManipulator manipulator(*m_pModifyingBusInterface,
                                     *m_pQueryBusInterface,
                                     *m_pApartment);

    manipulator.synchronizeZoneSensorAssignment(DSS::getInstance()->getApartment().getZones());
  }

  void MeterMaintenance::raiseEvent(boost::shared_ptr<Event> _pEvent) {
    if(DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(_pEvent);
    }
  } // raiseEvent

 //=============================================== ModelMaintenance

  ModelMaintenance::ModelMaintenance(DSS* _pDSS, const int _eventTimeoutMS)
  : ThreadedSubsystem(_pDSS, "Apartment"),
    m_IsInitializing(true),
    m_processedEvents(UINT_MAX - 2), // force early wrap around
    m_IsDirty(false),
    m_pendingSaveRequest(false),
    m_suppressSaveRequestNotify(m_processedEvents),
    m_pApartment(NULL),
    m_pMetering(NULL),
    m_EventTimeoutMS(_eventTimeoutMS),
    m_pStructureQueryBusInterface(NULL),
    m_pStructureModifyingBusInterface(NULL),
    m_taskProcessor(boost::make_shared<TaskProcessor>()),
    m_pMeterMaintenance(boost::make_shared<MeterMaintenance>(_pDSS, "MeterMaintenance"))
  { }

  void ModelMaintenance::shutdown() {
    if (m_pendingSaveRequest) {
      writeConfiguration();
    }
    m_pMeterMaintenance->shutdown();
    ThreadedSubsystem::shutdown();
  }

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

      DSS::getInstance()->getPropertySystem().setStringValue(
              getConfigPropertyBasePath() + "invalidbackup",
              getDSS().getDataDirectory() + "invalid-apartment.xml", true, false);

      boost::filesystem::path filename(
              DSS::getInstance()->getPropertySystem().getStringValue(
                                   getConfigPropertyBasePath() + "configfile"));

      DSS::getInstance()->getPropertySystem().setStringValue(
          getConfigPropertyBasePath() + "iconBasePath",
          DSS::getInstance()->getPropertySystem().getStringValue(
              "/config/datadirectory") + "images/", true, false);

      DSS::getInstance()->getPropertySystem().setStringValue(
          getConfigPropertyBasePath() + "logCollectionBasePath",
          "/var/log/collection/", true, false);

      checkConfigFile(filename);

      m_pStructureQueryBusInterface = DSS::getInstance()->getBusInterface().getStructureQueryBusInterface();
      m_pStructureModifyingBusInterface = DSS::getInstance()->getBusInterface().getStructureModifyingBusInterface();

      // load devices/dsMeters/etc. from a config-file
      readConfiguration();
    }
  } // initialize

  bool ModelMaintenance::pendingChangesBarrier(int waitSeconds) {
    boost::mutex::scoped_lock lock(m_rvMut);

    unsigned syncState = indexOfNextSyncState();
    // rollover: syncState:2, m_processed:UINT_MAX -> 3 (0, 1, 2)
    // if delta > INT_MAX, delta is negative
    while (static_cast<int>(syncState - m_processedEvents) > 0) {
      if (m_rvCond.wait_for(lock, boost::chrono::seconds(waitSeconds)) ==
          boost::cv_status::timeout) {
        // timeout, better throw an exception?
        return false;
      }
    }
    return true;
  }

  void ModelMaintenance::notifyModelConsistent() {
    boost::mutex::scoped_lock lock(m_rvMut);
    m_rvCond.notify_all();
  }

  void ModelMaintenance::doStart() {
    run();
    m_pMeterMaintenance->run();
  } // start

  void ModelMaintenance::execute() {
    if(DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser("ModelMaintenance needs system-rights");
    }

    log("ModelMaintenance::execute: Enumerating model", lsNotice);
    discoverDS485Devices();

    boost::shared_ptr<ApartmentTreeListener> treeListener
      = boost::shared_ptr<ApartmentTreeListener>(
          new ApartmentTreeListener(this, m_pApartment));

    while (!m_Terminated) {
      handleModelEvents();
      handleDeferredModelEvents();
      delayedConfigWrite();
    }
  } // execute

  void ModelMaintenance::discoverDS485Devices() {
    if (m_pStructureQueryBusInterface != NULL) {
      try {
        std::vector<DSMeterSpec_t> busMembers = m_pStructureQueryBusInterface->getBusMembers();
        BusScanner scanner(*m_pStructureQueryBusInterface, *m_pApartment, *this);
        foreach (DSMeterSpec_t& spec, busMembers) {
          boost::shared_ptr<DSMeter> dsMeter;
          try{
            dsMeter = m_pApartment->getDSMeterByDSID(spec.DSID);
            scanner.synchronizeDSMeterData(dsMeter, spec);

            log ("dS485 Bus Device known: " + dsuid2str(spec.DSID) + ", type:" + intToString(spec.DeviceType));
            if (dsMeter->isPresent() && !busMemberIsDSMeter(dsMeter->getBusMemberType())) {
              m_pApartment->removeDSMeter(spec.DSID);
              log ("removing uninteresting Bus Device: " + dsuid2str(dsMeter->getDSID()) + ", type:" + intToString(dsMeter->getBusMemberType()));
            }
          } catch(ItemNotFoundException& e) {
            if (!busMemberIsDSMeter(spec.DeviceType)) {
              // recover old dSM11 fw < 1.8.3
              if (!DsmApiIsdSM(spec.DSID)) {
                log ("ignore dS485 Bus Device: " + dsuid2str(spec.DSID)  + ", type: " + intToString(spec.DeviceType), lsWarning);
                continue;
              }
            }
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

  void ModelMaintenance::scheduleConfigWrite()
  {
    boost::mutex::scoped_lock lock(m_SaveRequestMutex);
    if (!m_pendingSaveRequest) {
      m_pendingSaveRequest = true;
      m_pendingSaveRequestTS = DateTime();
    }
  }

  void ModelMaintenance::delayedConfigWrite()
  {
    boost::mutex::scoped_lock lock(m_SaveRequestMutex);
    if (!m_pendingSaveRequest ||
        DateTime().difference(m_pendingSaveRequestTS) < 30) {
      return;
    }

    writeConfiguration();
    m_pendingSaveRequest = false;
  }

  void ModelMaintenance::writeConfiguration() {
    if (!DSS::hasInstance()) {
      return;
    }

    PropertySystem &props(DSS::getInstance()->getPropertySystem());
    ModelPersistence persistence(*m_pApartment);
    persistence.writeConfigurationToXML(props.getStringValue(getConfigPropertyBasePath()
                                                             + "configfile"));
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
    // TODO locking, seems events are only pushed from mainloop
    if (m_DeferredEvents.empty()) {
      return false;
    }

    m_DeferredEvents_t mDefEvents;
    mDefEvents.swap(m_DeferredEvents);

    foreach (boost::shared_ptr<ModelDeferredEvent> evt, mDefEvents) {
      boost::shared_ptr<ModelDeferredSceneEvent> mEvent = boost::dynamic_pointer_cast <ModelDeferredSceneEvent> (evt);
      if (mEvent != NULL) {
        int sceneID = mEvent->getSceneID();
        int groupID = mEvent->getGroupID();
        int zoneID = mEvent->getZoneID();
        int originDeviceID = mEvent->getOriginDeviceID();

        try {
          boost::shared_ptr<Zone> zone = m_pApartment->getZone(zoneID);
          boost::shared_ptr<Group> group = zone->getGroup(groupID);
          dsuid_t originDSUID = mEvent->getSource();
          if ((originDSUID != DSUID_NULL) && (originDeviceID != 0)) {
            DeviceReference devRef = m_pApartment->getDevices().getByBusID(originDeviceID, mEvent->getSource());
            originDSUID = devRef.getDSID();
          }

          if (mEvent->isDue()) {
            if (!mEvent->isCalled()) {
              handleDeferredModelStateChanges(mEvent->getCallOrigin(), zoneID,
                                              groupID, sceneID);
              raiseEvent(createGroupCallSceneEvent(group, sceneID, groupID,
                                                   zoneID,
                                                   mEvent->getCallOrigin(),
                                                   originDSUID,
                                                   mEvent->getOriginToken(),
                                                   mEvent->getForcedFlag()));
            }
            // finished deferred processing of this event
          } else if (SceneHelper::isDimSequence(sceneID)) {
            if (!mEvent->isCalled()) {
              handleDeferredModelStateChanges(mEvent->getCallOrigin(), zoneID,
                                              groupID, sceneID);
              raiseEvent(createGroupCallSceneEvent(group, sceneID, groupID,
                                                   zoneID,
                                                   mEvent->getCallOrigin(),
                                                   originDSUID,
                                                   mEvent->getOriginToken(),
                                                   mEvent->getForcedFlag()));
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

      boost::shared_ptr<ModelDeferredButtonEvent> bEvent = boost::dynamic_pointer_cast <ModelDeferredButtonEvent> (evt);
      if (bEvent != NULL) {
        int deviceID = bEvent->getDeviceID();
        int buttonIndex = bEvent->getButtonIndex();
        int clickType = bEvent->getClickType();

        try {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(deviceID, bEvent->getSource());
          boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);

          if (bEvent->isDue() || (clickType == ClickTypeHE)) {
            boost::shared_ptr<Event> event = boost::make_shared<Event>(EventName::DeviceButtonClick, pDevRev);
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
    m_ModelEvents_t::auto_type event;

    {
      boost::mutex::scoped_lock lock(m_ModelEventsMutex);
      if (m_ModelEvents.empty() &&
          (m_NewModelEvent.wait_for(lock, m_EventTimeoutMS) ==
           boost::cv_status::timeout)) {
        notifyModelConsistent(); // no pending changes
        return false;
      }

      if (m_ModelEvents.empty()) {
        // man pthread_cond_wait -> Condition Wait Semantics
        // condition needs to re-checked, really happens!
        // observed wait to return without a prior notify call
        return false;
      }

      assert(!m_ModelEvents.empty());
      event = m_ModelEvents.pop_front();
      m_processedEvents++;
    }

    ModelEventWithDSID* pEventWithDSID =
      dynamic_cast<ModelEventWithDSID*>(event.get());
    ModelEventWithStrings* pEventWithStrings =
      dynamic_cast<ModelEventWithStrings*>(event.get());

    switch (event->getEventType()) {
    case ModelEvent::etNewDevice:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 2) {
        log("Expected exactly 2 parameter for ModelEvent::etNewDevice");
      } else {
        onAddDevice(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1));
      }
      break;
    case ModelEvent::etLostDevice:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 2) {
        log("Expected exactly 2 parameter for ModelEvent::etLostDevice");
      } else {
        onRemoveDevice(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1));
      }
      break;
    case ModelEvent::etDeviceChanged:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 1) {
        log("Expected exactly 1 parameter for ModelEvent::etDeviceChanged");
      } else {
        rescanDevice(pEventWithDSID->getDSID(), event->getParameter(0));
      }
      break;
    case ModelEvent::etDeviceConfigChanged:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 4) {
        log("Expected exactly 4 parameter for ModelEvent::etDeviceConfigChanged");
      } else {
        onDeviceConfigChanged(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1),
                                                         event->getParameter(2), event->getParameter(3));
      }
      break;
    case ModelEvent::etCallSceneDevice:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() == 5) {
        onDeviceCallScene(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2), (callOrigin_t)event->getParameter(3), event->getParameter(4), event->getSingleStringParameter());
      } else {
        log("Unexpected parameter count for ModelEvent::etCallSceneDevice");
      }
      break;
    case ModelEvent::etBlinkDevice:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() == 3) {
        onDeviceBlink(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), (callOrigin_t)event->getParameter(2), event->getSingleStringParameter());
      } else {
        log("Unexpected parameter count for ModelEvent::etBlinkDevice");
      }
      break;
    case ModelEvent::etCallSceneDeviceLocal:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() == 5) {
        onDeviceCallScene(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2), (callOrigin_t)event->getParameter(3), event->getParameter(4), event->getSingleStringParameter());
      } else {
        log("Unexpected parameter count for ModelEvent::etCallSceneDeviceLocal");
      }
      break;
    case ModelEvent::etButtonClickDevice:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 3) {
        log("Expected exactly 3 parameter for ModelEvent::etButtonClickDevice");
      } else {
        onDeviceActionEvent(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2));
        onDeviceActionFiltered(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2));
      }
      break;
    case ModelEvent::etButtonDirectActionDevice:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 2) {
        log("Expected exactly 2 parameter for ModelEvent::etButtonDirectActionDevice");
      } else {
        onDeviceDirectActionEvent(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1));
      }
      break;
    case ModelEvent::etCallSceneGroup:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() == 6) {
        onGroupCallScene(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2), event->getParameter(3), (callOrigin_t)event->getParameter(4), event->getParameter(5), event->getSingleStringParameter());
        if (pEventWithDSID) {
          onGroupCallSceneFiltered(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2), event->getParameter(3), (callOrigin_t)event->getParameter(4), event->getParameter(5), event->getSingleStringParameter());
        }
      } else {
        log("Expected 6 parameters for ModelEvent::etCallSceneGroup");
      }
      break;
    case ModelEvent::etUndoSceneGroup:
      if (event->getParameterCount() != 5) {
        log("Expected 5 parameters for ModelEvent::etUndoSceneGroup");
      } else {
        onGroupUndoScene(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2), event->getParameter(3), (callOrigin_t)event->getParameter(4), event->getSingleStringParameter());
      }
      break;
    case ModelEvent::etBlinkGroup:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 4) {
        log("Expected at least 3 parameter for ModelEvent::etBlinkGroup");
      } else {
        onGroupBlink(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2), (callOrigin_t)event->getParameter(3), event->getSingleStringParameter());
      }
      break;
    case ModelEvent::etModelDirty:
      if (DSS::hasInstance() && DSS::getInstance()->getPropertySystem().getBoolValue(pp_websvc_enabled)) {
        // hack: if difference > INT_MAX, than difference is negative
        if (static_cast<int>(m_processedEvents - m_suppressSaveRequestNotify) > 0) {
          raiseEvent(ModelChangedEvent::createApartmentChanged()); /* raiseTimedEvent */
          // skip etModelDirty events already on the queue
          m_suppressSaveRequestNotify = m_processedEvents;
        }
      }
      notifyModelConsistent(); //< some transaction completed
      scheduleConfigWrite();
      break;
    case ModelEvent::etModelOperationModeChanged:
      notifyModelConsistent(); //< some transaction completed
      scheduleConfigWrite();
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
      if (event->getParameterCount() != 0) {
        log("Expected exactly 0 parameter for ModelEvent::etDSMeterReady");
      } else {
        assert(pEventWithDSID != NULL);
        dsuid_t meterID = pEventWithDSID->getDSID();
        try{
          boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(meterID);
          mod->setIsConnected(true);
          mod->setIsValid(false);
        } catch (ItemNotFoundException& e) {
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
      } catch (ItemNotFoundException& _e) {
      }
      break;
    case ModelEvent::etMeteringValues:
      if (event->getParameterCount() != 2) {
        log("Expected exactly 2 parameter for ModelEvent::etMeteringValues");
      } else {
        assert(pEventWithDSID != NULL);
        dsuid_t meterID = pEventWithDSID->getDSID();
        unsigned int power = event->getParameter(0);
        unsigned int energy = event->getParameter(1);
        try {
          boost::shared_ptr<DSMeter> meter = m_pApartment->getDSMeterByDSID(meterID);
          meter->setPowerConsumption(power);
          meter->updateEnergyMeterValue(energy);
          m_pMetering->postMeteringEvent(meter, power, (unsigned long long)(meter->getCachedEnergyMeterValue() + 0.5), DateTime());
        } catch (ItemNotFoundException& _e) {
        }
      }
      break;
    case ModelEvent::etDeviceSensorEvent:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() < 2) {
        log("Expected at least 2 parameter for ModelEvent::etDeviceSensorEvent");
      } else {
        onSensorEvent(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1));
      }
      break;
    case ModelEvent::etDeviceBinaryStateEvent:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() < 4) {
        log("Expected at least 4 parameter for ModelEvent::etDeviceBinaryStateEvent");
      } else {
        onBinaryInputEvent(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2), event->getParameter(3));
      }
      break;
    case ModelEvent::etDeviceSensorValue:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() < 3) {
        log("Expected at least 3 parameter for ModelEvent::etDeviceSensorValue");
      } else {
        onSensorValue(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2));
      }
      break;
    case ModelEvent::etZoneSensorValue:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() < 5) {
        log("Expected at least 5 parameter for ModelEvent::etZoneSensorValue");
      } else {
        onZoneSensorValue(pEventWithDSID->getDSID(),
                          str2dsuid(event->getSingleStringParameter()),
                          event->getParameter(0),
                          event->getParameter(1),
                          event->getParameter(2),
                          event->getParameter(3),
                          event->getParameter(4));
      }
      break;
    case ModelEvent::etDeviceEANReady:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() != 11) {
        log("Expected 8 parameters for ModelEvent::etDeviceEANReady");
      } else {
        onEANReady(pEventWithDSID->getDSID(),
                   event->getParameter(0),
                   (const DeviceOEMState_t)event->getParameter(1),
                   (const DeviceOEMInetState_t)event->getParameter(2),
                   ((unsigned long long)event->getParameter(3)) << 32 | ((unsigned long long)event->getParameter(4) & 0xFFFFFFFF),
                   event->getParameter(5),
                   event->getParameter(6),
                   event->getParameter(7),
                   event->getParameter(8),
                   event->getParameter(9),
                   event->getParameter(10));
      }
      break;
    case ModelEvent::etDeviceOEMDataReady:
      assert(pEventWithStrings != NULL);
      if ((event->getParameterCount() != 1) && (pEventWithStrings->getStringParameterCount() != 4)) {
        log("Expected 5 parameters for ModelEvent::etDeviceOEMDataReady");
      } else {
        onOEMDataReady(pEventWithDSID->getDSID(),
                       (DeviceOEMState_t)event->getParameter(0),
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
          event->getParameter(0),
          event->getSingleObjectParameter());
      break;
    case ModelEvent::etControllerState:
      assert(pEventWithDSID != NULL);
      onHeatingControllerState(
          pEventWithDSID->getDSID(),
          event->getParameter(0),
          event->getParameter(1));
      break;
    case ModelEvent::etControllerValues:
      assert(pEventWithDSID != NULL);
      onHeatingControllerValues(
          pEventWithDSID->getDSID(),
          event->getParameter(0),
          event->getSingleObjectParameter());
      break;
    case ModelEvent::etClusterConfigLock:
      onClusterConfigLock(event->getParameter(0), event->getParameter(1));
      break;
    case ModelEvent::etClusterLockedScenes:
    {
      uint8_t lockedScenes[16];
      for (int i = 0; i < 16; ++i) {
        lockedScenes[i] = event->getParameter(i + 1);
      }
      std::vector<int> pLockedScenes = parseBitfield(lockedScenes, 128);
      onClusterLockedScenes(event->getParameter(0), pLockedScenes);
      break;
    }
    case ModelEvent::etGenericEvent:
    {
      unsigned origin = event->getParameter(1);
      if (!validOrigin(origin)) {
        log("etGenericEvent: invalid origin" + intToString(origin), lsNotice);
        break;
      }
      onGenericEvent(static_cast<GenericEventType_t> (event->getParameter(0)),
                     boost::static_pointer_cast<GenericEventPayload_t> (event->getSingleObjectParameter()),
                     static_cast<callOrigin_t> (event->getParameter(1)));
      break;
    }
    case ModelEvent::etOperationLock:
    {
      unsigned origin = event->getParameter(3);
      if (!validOrigin(origin)) {
        log("etOperationLock: invalid origin" + intToString(origin), lsNotice);
        break;
      }

      // parm> 0:zoneId 1:groupId 2:lock 3:origin
      onOperationLock(event->getParameter(0), event->getParameter(1), event->getParameter(2),
                      static_cast<callOrigin_t>(event->getParameter(3)));
      break;
    }
    case ModelEvent::etDeviceDirty:
      assert(pEventWithDSID != NULL);
      onAutoClusterMaintenance(pEventWithDSID->getDSID());
    case ModelEvent::etDummyEvent:
      // unit test
      break;
    case ModelEvent::etClusterCleanup:
      onAutoClusterCleanup();
      break;
    case ModelEvent::etMeterReady:
      onMeterReady();
      break;
    case ModelEvent::etDeviceDataReady:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() < 3) {
        log("Expected at least 3 parameter for ModelEvent::etDeviceDataReady");
      } else {
        onDeviceDataReady(pEventWithDSID->getDSID(), event->getParameter(0), event->getParameter(1), event->getParameter(2));
      }
      break;
    case ModelEvent::etDsmStateChange:
      assert(pEventWithDSID != NULL);
      if (event->getParameterCount() < 1) {
        log("Expected at least 1 parameter for ModelEvent::etDsmStateChange");
      } else {
        onDsmStateChange(pEventWithDSID->getDSID(), event->getParameter(0));
      }
      break;

    default:
      assert(false);
      break;
    }

    return true;
  } // handleModelEvents

  unsigned ModelMaintenance::indexOfNextSyncState() {
    boost::mutex::scoped_lock lock(m_ModelEventsMutex);
    int count = m_processedEvents + m_ModelEvents.size();

    // assume etModelDirty as sync states, ignore trailing events
    foreach_r (ModelEvent evt, m_ModelEvents) {
      if (evt.getEventType() == ModelEvent::etModelDirty) {
        break;
      }
      count -= 1;
    }
    return count;
  }

  void ModelMaintenance::addModelEvent(ModelEvent* _pEvent) {
    // filter out dirty events, as this will rewrite apartment.xml
    if (m_IsInitializing && 
       (_pEvent->getEventType() == ModelEvent::etModelDirty)) {
      m_IsDirty = true;
      delete _pEvent;
      // notify_one not necessary, since event not added to m_ModelEvents
    } else {
      boost::mutex::scoped_lock lock(m_ModelEventsMutex);
      m_ModelEvents.push_back(_pEvent);
      m_NewModelEvent.notify_one(); // trigger m_NewModelEvent.wait_for
    }
  } // addModelEvent

  void ModelMaintenance::readConfiguration() {
    if(DSS::hasInstance()) {
      std::string configFileName = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile");
      std::string backupFileName = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "invalidbackup");
      ModelPersistence persistence(*m_pApartment);
      if (boost::filesystem::exists(configFileName)) {
        persistence.readConfigurationFromXML(configFileName, backupFileName);
      }
      log("processed apartment.xml", lsNotice);
    }
  } // readConfiguration

  void ModelMaintenance::raiseEvent(boost::shared_ptr<Event> _pEvent) {
    if(DSS::hasInstance()) {
      getDSS().getEventQueue().pushEvent(_pEvent);
    }
  } // raiseEvent

  void ModelMaintenance::synchronizeHeatingAssignment(const dsuid_t& _dSMeterID) {
    boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::UpdateAutoselect);
    pEvent->setProperty("meterDSUID", dsuid2str(_dSMeterID));
    log("synchronizeHeatingAssignment", lsNotice);
    if (DSS::getInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } // synchronizeHeatingAssignment

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
        dsuid_t originDSUID = _source;
        if ((_source != DSUID_NULL) && (_originDeviceID != 0)) {
          DeviceReference devRef = m_pApartment->getDevices().getByBusID(_originDeviceID, _source);
          originDSUID = devRef.getDSID();
        }
        if (_forced) {
          pEvent->setProperty("forced", "true");
        }
        pEvent->setProperty("callOrigin", intToString(_origin));
        pEvent->setProperty("originDSUID", dsuid2str(originDSUID));
        pEvent->setProperty("originToken", _token);
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

        dsuid_t originDSUID = _source;
        if ((_source != DSUID_NULL) && (_originDeviceID != 0)) {
          DeviceReference devRef = m_pApartment->getDevices().
            getByBusID(_originDeviceID, _source);
          originDSUID = devRef.getDSID();
        }
        raiseEvent(createGroupUndoSceneEvent(group, _sceneID, _groupID,
                                             _zoneID, _origin, originDSUID,
                                             _token));
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
      if (_source == DSUID_NULL) {
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

      // flush all pending scene call events from same origin
      foreach(boost::shared_ptr<ModelDeferredEvent> evt, m_DeferredEvents) {
        boost::shared_ptr<ModelDeferredSceneEvent> pEvent = boost::dynamic_pointer_cast <ModelDeferredSceneEvent> (evt);
        // object may be NULL in case of ModelDeferredButtonEvent's
        if (pEvent == NULL) {
          continue;
        }
        if ((pEvent->getSource() == _source) &&
            (pEvent->getOriginDeviceID() == _originDeviceID)) {
          pEvent->clearTimestamp();
        }
      }

      boost::shared_ptr<ModelDeferredSceneEvent> mEvent = boost::make_shared<ModelDeferredSceneEvent>(_source, _zoneID, _groupID, _originDeviceID, _sceneID, _origin, _forced, _token);
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
        if ((pEvent->getSource() == _source) &&
            ((pEvent->getOriginDeviceID() == _originDeviceID) ||
             (_originDeviceID == 0))) {
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

    boost::shared_ptr<ModelDeferredSceneEvent> mEvent = boost::make_shared<ModelDeferredSceneEvent>(_source, _zoneID, _groupID, _originDeviceID, _sceneID, _origin, _forced, _token);
    m_DeferredEvents.push_back(mEvent);
  } // onGroupCallSceneFiltered

  void ModelMaintenance::onGroupBlink(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const callOrigin_t _origin, const std::string _token) {
    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      boost::shared_ptr<Group> group = zone->getGroup(_groupID);
      if(group != NULL) {
        log("OnGroupBlink: group-id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID));
        boost::shared_ptr<Event> pEvent;
        pEvent.reset(new Event(EventName::IdentifyBlink, group));
        pEvent->setProperty("groupID", intToString(_groupID));
        pEvent->setProperty("zoneID", intToString(_zoneID));
        dsuid_t originDSUID = _source;
        if ((_source != DSUID_NULL) && (_originDeviceID != 0)) {
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
    if (_source == DSUID_NULL) {
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

      boost::shared_ptr<ModelDeferredButtonEvent> mEvent = boost::make_shared<ModelDeferredButtonEvent>(_source, _deviceID, _buttonNr, _clickType);
      mEvent->clearTimestamp();  // force immediate event processing
      m_DeferredEvents.push_back(mEvent);
      return;
    }

    foreach(boost::shared_ptr<ModelDeferredEvent> evt, m_DeferredEvents) {
      boost::shared_ptr<ModelDeferredButtonEvent> pEvent = boost::dynamic_pointer_cast <ModelDeferredButtonEvent> (evt);
      if (pEvent == NULL) {
        continue;
      }
      if ((pEvent->getDeviceID() == _deviceID) &&
          (pEvent->getButtonIndex() == _buttonNr) &&
          (pEvent->getSource() == _source)) {
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

    boost::shared_ptr<ModelDeferredButtonEvent> mEvent = boost::make_shared<ModelDeferredButtonEvent>(_source, _deviceID, _buttonNr, _clickType);
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
        boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);
        boost::shared_ptr<Event> event = boost::make_shared<Event>(EventName::CallScene, pDevRev);
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
        boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);
        boost::shared_ptr<Event> event = boost::make_shared<Event>(EventName::IdentifyBlink, pDevRev);
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
        boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);
        boost::shared_ptr<Event> event = boost::make_shared<Event>(EventName::ButtonClickBus, pDevRev);
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

  void ModelMaintenance::onDeviceDirectActionEvent(const dsuid_t& _dsMeterID, const int _deviceID, const int _actionID) {
    try {
      boost::shared_ptr<DSMeter> mod = m_pApartment->getDSMeterByDSID(_dsMeterID);
      log("onDeviceDirectActionEvent: dsMeter-id '" + dsuid2str(_dsMeterID) + "' for device '" + intToString(_deviceID) +
          "' action: " + intToString(_actionID));
      try {
        DeviceReference devRef = mod->getDevices().getByBusID(_deviceID, _dsMeterID);
        boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);
        boost::shared_ptr<Event> event = boost::make_shared<Event>(EventName::ButtonDeviceAction, pDevRev);
        event->setProperty("actionID", intToString(_actionID));
        raiseEvent(event);
      } catch(ItemNotFoundException& e) {
        log("onDeviceDirectActionEvent: Could not find device with bus-id '" + intToString(_deviceID) + "' on dsMeter '" + dsuid2str(_dsMeterID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("onDeviceDirectActionEvent: Could not find dsMeter with bus-id '" + dsuid2str(_dsMeterID) + "'", lsError);
    }
  } // onDeviceDirectActionEvent

  void ModelMaintenance::onAddDevice(const dsuid_t& _dsMeterID, const int _zoneID, const int _devID) {
    log("Device discovered:");
    try {
      DeviceReference devRef = m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_devID, _dsMeterID);
      log("  DSID   : " +  dsuid2str(devRef.getDSID()));
      {
        boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(devRef);
        boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
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
      boost::shared_ptr<DeviceReference> pDevRef;  
      try {
        DeviceReference devRef = m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_devID, _dsMeterID);
        pDevRef = boost::make_shared<DeviceReference>(devRef);
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

      pollSensors(pDevRef);
    }
  } // onAddDevice

  void ModelMaintenance::onRemoveDevice(const dsuid_t& _dsMeterID, const int _zoneID, const int _devID) {
    log("Device disappeared:");
    try {
      DeviceReference devRef = m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_devID, _dsMeterID);
      log("  DSID   : " +  dsuid2str(devRef.getDSID()));
      {
        boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(devRef);
        boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
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

    // synchronize dss with dsm zone sensors
    // in case a dsm crashed during zone sensor assignment.
    m_pMeterMaintenance->triggerMeterSynchronization();

    // synchronize vdc heating assignment in case of vdsm migration 
    if (BusMember_vDC == meter->getBusMemberType()) {
      synchronizeHeatingAssignment(_dSMeterID);
    }

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
        boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(devRef);
        boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
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
      boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);

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
      boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);

      raiseEvent(createDeviceBinaryInputEvent(pDevRev, _eventIndex, _eventType,
                                              _state));
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
      boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);
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
            raiseEvent(createDeviceBinaryInputEvent(pDevRev, index,
                                                    pDev->getDeviceBinaryInputType(index),
                                                    newState));
          }
        }

      // device status and error event
      } else if (_sensorIndex <= 31 && _sensorIndex >= 16) {
        raiseEvent(createDeviceStatusEvent(pDevRev, _sensorIndex,
                                           _sensorValue));

      // regular sensor value event
      } else if (_sensorIndex <= 15) {
        uint8_t sensorType = 0;
        try {
          pDev->setSensorValue(_sensorIndex, (const unsigned int) _sensorValue);
          boost::shared_ptr<DeviceSensor_t> pdSensor = pDev->getSensor(_sensorIndex);
          sensorType = pdSensor->m_sensorType;
        } catch (ItemNotFoundException& e) {
          log(std::string("onSensorValue: ") + e.what(), lsNotice);
        }
        raiseEvent(createDeviceSensorValueEvent(pDevRev, _sensorIndex,
                                                sensorType, _sensorValue));
      }
    } catch(ItemNotFoundException& e) {
      log("onSensorValue: Datamodel failure: " + std::string(e.what()), lsWarning);
    }
  } // onSensorValue

  void ModelMaintenance::onZoneSensorValue(const dsuid_t& _meterID,
                                           const dsuid_t& _sourceDevice,
                                           const int& _zoneID,
                                           const int& _groupID,
                                           const int& _sensorType,
                                           const int& _sensorValue,
                                           const int& _precision) {
    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      boost::shared_ptr<Group> group = zone->getGroup(_groupID);

      double fValue = SceneHelper::sensorToFloat12(_sensorType, _sensorValue);
      group->sensorPush(_sourceDevice, _sensorType, fValue);

      // check for a valid dsuid
      dsuid_t devdsuid = _sourceDevice;
      if (_sourceDevice.id[0] == 0x00) {
        uint32_t dsuidType =
            (_sourceDevice.id[10] << 24) |
            (_sourceDevice.id[9] << 16) |
            (_sourceDevice.id[8] << 8) |
            (_sourceDevice.id[7]);
        // the dsm sends a type "0" dsuid but with the serial number only,
        // need to replace with the real sgtin, #10887
        if (dsuidType == 0) {
          try {
            uint32_t serialNumber;
            if (dsuid_get_serial_number(&devdsuid, &serialNumber) == 0) {
              DeviceReference devRef = m_pApartment->getDevices().getBySerial(serialNumber);
              devdsuid = devRef.getDSID();
            }
          } catch (ItemNotFoundException& e) {}
        }
      }
      raiseEvent(createZoneSensorValueEvent(group, _sensorType, _sensorValue, devdsuid));
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
                                        const bool _isConfigLocked,
                                        const int _pairedDevices,
                                        const bool _isVisible) {
    try {
      DeviceReference devRef = m_pApartment->getDevices().getByBusID(_deviceID, _dsMeterID);
      if (_state == DEVICE_OEM_VALID) {
        devRef.getDevice()->setOemInfo(_eanNumber, _serialNumber, _partNumber, _iNetState, _isIndependent);
        if ((_iNetState == DEVICE_OEM_EAN_INTERNET_ACCESS_OPTIONAL) ||
            (_iNetState == DEVICE_OEM_EAN_INTERNET_ACCESS_MANDATORY)) {
          // query Webservice
          getTaskProcessor()->addEvent(boost::make_shared<OEMWebQuery>(devRef.getDevice()));
          devRef.getDevice()->setOemProductInfoState(DEVICE_OEM_LOADING);
        } else {
          devRef.getDevice()->setOemProductInfoState(DEVICE_OEM_NONE);
        }
      }
      devRef.getDevice()->setOemInfoState(_state);
      devRef.getDevice()->setConfigLock(_isConfigLocked);
      devRef.getDevice()->setVisibility(_isVisible);
      devRef.getDevice()->setPairedDevices(_pairedDevices);
    } catch(std::runtime_error& e) {
      log(std::string("Error updating OEM data of device: ") + e.what(), lsWarning);
    }
  } // onEANReady

  void ModelMaintenance::onOEMDataReady(dsuid_t _deviceID,
                                             const DeviceOEMState_t _state,
                                             const std::string& _productName,
                                             const std::string& _iconPath,
                                             const std::string& _productURL,
                                             const std::string& _defaultName) {
    try {
      boost::shared_ptr<Device> pDevice = m_pApartment->getDeviceByDSID(_deviceID);
      if (_state == DEVICE_OEM_VALID) {
        pDevice->setOemProductInfo(_productName, _iconPath, _productURL);
        if (pDevice->getName().empty()) {
          pDevice->setName(_defaultName);
        }
      }
      pDevice->setOemProductInfoState(_state);
      {
        boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(pDevice, m_pApartment);
        boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
        mEvent->setProperty("action", "oemDataChanged");
        if(DSS::hasInstance()) {
          DSS::getInstance()->getEventQueue().pushEvent(mEvent);
        }
      }
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
    raiseEvent(createHeatingControllerConfig(_zoneID, _dsMeterID, *config));
  } // onHeatingControllerConfig

  void ModelMaintenance::onHeatingControllerValues(dsuid_t _dsMeterID, const int _zoneID, boost::shared_ptr<void> _spec) {
    boost::shared_ptr<ZoneHeatingOperationModeSpec_t> values =
        boost::static_pointer_cast<ZoneHeatingOperationModeSpec_t> (_spec);
    ZoneHeatingProperties_t hProp;
    ZoneHeatingStatus_t hStatus;

    boost::shared_ptr<Zone> zone;
    try {
      zone = m_pApartment->getZone(_zoneID);
      hProp = zone->getHeatingProperties();
      hStatus = zone->getHeatingStatus();
    } catch(ItemNotFoundException& e) {
      log(std::string("Error on heating control value event, item not found: ") + e.what(), lsWarning);
      return;
    }

    log(std::string("onHeatingControllerValues:  dsMeter " + dsuid2str(_dsMeterID) +
        ", current controller " + dsuid2str(hProp.m_HeatingControlDSUID), lsInfo));

    raiseEvent(createHeatingControllerValue(_zoneID, _dsMeterID, hProp, *values));
    raiseEvent(createHeatingControllerValueDsHub(_zoneID,
                                                 zone->getHeatingOperationMode(),
                                                 hProp, hStatus));
  } // onHeatingControllerValues

  void ModelMaintenance::onHeatingControllerState(dsuid_t _dsMeterID, const int _zoneID, const int _State) {

    try {
      boost::shared_ptr<Zone> zone = m_pApartment->getZone(_zoneID);
      zone->setHeatingControlState(_State);
    } catch(ItemNotFoundException& e) {
      log(std::string("Error on heating state event, item not found: ") + e.what(), lsWarning);
    }

    raiseEvent(createHeatingControllerState(_zoneID, _dsMeterID, _State));
  } // onHeatingControllerState

  void ModelMaintenance::onClusterConfigLock(const int _clusterID, const bool _configurationLock) {
    try {
      boost::shared_ptr<Cluster> cluster = m_pApartment->getCluster(_clusterID);
      cluster->setConfigurationLocked(_configurationLock);
      raiseEvent(createClusterConfigLockEvent(cluster, 0, _clusterID, _configurationLock, coDsmApi));
      addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    } catch(ItemNotFoundException& e) {
      log(std::string("Error on cluster config lock change, cluster not found: ") + e.what(), lsWarning);
    }
  } // onClusterConfigLock

  void ModelMaintenance::onClusterLockedScenes(const int _clusterID, const std::vector<int> &_lockedScenes) {
    try {
      boost::shared_ptr<Cluster> cluster = m_pApartment->getCluster(_clusterID);
      cluster->setLockedScenes(_lockedScenes);
      addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    } catch(ItemNotFoundException& e) {
      log(std::string("Error on cluster locked scenes change, cluster not found: ") + e.what(), lsWarning);
    }
  } // onClusterConfigLock

  void ModelMaintenance::onGenericEvent(const GenericEventType_t _eventType, const boost::shared_ptr<GenericEventPayload_t> &_pPayload, const callOrigin_t _origin) {
    /*
     * Generic Events
        0 no event
        1 Sun         {active=1, inactive=2} {direction}
        2 Frost       {active=1, inactive=2}
        3 HeatingMode {Off=0, Heat=1, Cold=2, Auto=3} sent by the heating system, indicates type of energy delivered into rooms
        4 Service     {active=1, inactive=2}
     */
    switch (_eventType) {
    case ge_sun:
      {
        if (_pPayload->length != 2) {
          break;
        }

        unsigned value = _pPayload->payload[0];
        CardinalDirection_t direction =
          static_cast<CardinalDirection_t>(_pPayload->payload[1]);

        // arg0: value {active=1, inactive=2}
        // arg1: {direction}
        if (!valid(direction) || value == 0 || value > 2) {
          log(std::string("generic sunshine: invalid value: " + intToString(value) +
                          " direction: " + intToString(direction)), lsInfo);
          break;
        }

        raiseEvent(createGenericSignalSunshine(value, direction, _origin));
      }
      break;

    case ge_frost:
      {
        if (_pPayload->length != 1) {
          break;
        }

        unsigned value = _pPayload->payload[0];

        // arg0: {active=1, inactive=2}
        if (value != 1 && value != 2) {
          log(std::string("generic frost: invalid value: " + intToString(value)), lsInfo);
          break;
        }

        raiseEvent(createGenericSignalFrostProtection(value, _origin));
      }
      break;

    case ge_heating_mode:
      {
        if (_pPayload->length != 1) {
          break;
        }

        unsigned value = _pPayload->payload[0];

        // sent by the heating system, indicates type of energy delivered into rooms
        // arg0: {Off=0, Heat=1, Cold=2, Auto=3}
        if (value > 3) {
          log(std::string("generic heating mode: invalid value: " + intToString(value)), lsInfo);
          break;
        }
        if (value == 3) {
          log("generic heating mode: prevent auto value", lsWarning);
          break;
        }

        raiseEvent(createGenericSignalHeatingModeSwitch(value, _origin));
      }
      break;

    case ge_service:
      {
        if (_pPayload->length != 1) {
          break;
        }

        unsigned value = _pPayload->payload[0];

        // arg0: {active=1, inactive=2}
        if (value != 1 && value != 2) {
          log(std::string("generic building service: invalid value: " + intToString(value)), lsInfo);
          break;
        }

        raiseEvent(createGenericSignalBuildingService(_pPayload->payload[0], _origin));
      }
      break;

    default:
      log(std::string("unknown generic event: " + intToString(_eventType)));
      break;
    }
  } // onGenericEvent

  void ModelMaintenance::onOperationLock(const int _zoneID, const int _groupID, bool _lock, callOrigin_t _callOrigin) {
    if (!isAppUserGroup(_groupID)) {
      return;
    }
    try {
      boost::shared_ptr<Cluster> cluster = m_pApartment->getCluster(_groupID);
      cluster->setOperationLock(_lock, _callOrigin);
      raiseEvent(createOperationLockEvent(cluster, _zoneID, _groupID, _lock, _callOrigin));
    } catch(ItemNotFoundException& e) {
      log(std::string("Unknown cluster: ") + e.what(), lsWarning);
    }
  }

  void ModelMaintenance::onAutoClusterMaintenance(dsuid_t _deviceID)
  {
    try {
      boost::shared_ptr<Device> device = m_pApartment->getDeviceByDSID(_deviceID);
      AutoClusterMaintenance maintenance(m_pApartment);
      maintenance.consistencyCheck(*device.get());
      addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    } catch(ItemNotFoundException& e) {
      log(std::string("Error cluster consistency check,  item not found: ") + e.what(), lsWarning);
    }
  }

  void ModelMaintenance::onAutoClusterCleanup() {
    try {
      AutoClusterMaintenance maintenance(m_pApartment);
      maintenance.cleanupEmptyCluster();
      addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    } catch(ItemNotFoundException& e) {
      log(std::string("Error cluster consistency check,  item not found: ") + e.what(), lsWarning);
    }
  }

  void ModelMaintenance::onMeterReady() {
    if (m_IsInitializing) {
      setupWebUpdateEvent();
      m_IsInitializing = false;

      // handle delayed model dirty
      if (m_IsDirty) {
        m_IsDirty = false;
        addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
      }
    }
  }

  void ModelMaintenance::onDeviceDataReady(dsuid_t _meterID,
                                       const devid_t _deviceID,
                                       const int& _pairedDevices,
                                       const bool& _visible) {
    try {
      boost::shared_ptr<DSMeter> pMeter = m_pApartment->getDSMeterByDSID(_meterID);
      DeviceReference devRef = pMeter->getDevices().getByBusID(_deviceID, pMeter);
      boost::shared_ptr<DeviceReference> pDevRev = boost::make_shared<DeviceReference>(devRef);
      boost::shared_ptr<Device> pDev = devRef.getDevice();

      pDev->setPairedDevices(_pairedDevices);
      pDev->setVisibility(_visible);
    } catch(ItemNotFoundException& e) {
      log("onSensorValue: Datamodel failure: " + std::string(e.what()), lsWarning);
    }
  } // onDeviceDataReady

  void ModelMaintenance::onDsmStateChange(dsuid_t _meterID,
                                          const uint8_t& _state) {
    try {
      boost::shared_ptr<DSMeter> pMeter = m_pApartment->getDSMeterByDSID(_meterID);
      pMeter->setState(_state);
      if (_state == DSM_STATE_IDLE) {
        boost::mutex::scoped_lock lock(m_readoutTasksMutex);
        std::vector<std::pair<dsuid_t, boost::shared_ptr<Task> > >::iterator it;
        for (it = m_deviceReadoutTasks.begin(); it != m_deviceReadoutTasks.end();) {
          dsuid_t id = (*it).first;
          if (dsuid_equal(&id, &_meterID)) {
            log("onDsmStateChange: scheduling device readout task on dSM " +
                dsuid2str(_meterID));
            m_taskProcessor->addEvent((*it).second);
            it = m_deviceReadoutTasks.erase(it);
          } else {
            it++;
          }
        }
      }
    } catch(ItemNotFoundException& e) {
      log("onDsmStateChange: Datamodel failure: " + std::string(e.what()), lsWarning);
    }
  } // onDsmStateChange


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
    m_deviceDSUID = _device->getDSID();
    m_EAN = _device->getOemEanAsString();
    m_partNumber = _device->getOemPartNumber();
    m_serialNumber = _device->getOemSerialNumber();
  }

  ModelMaintenance::OEMWebQuery::OEMWebQueryCallback::OEMWebQueryCallback(dsuid_t _deviceDSUID)
    : m_deviceDSUID(_deviceDSUID)
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
          json_object *response;
          if (!json_object_object_get_ex(json_request, "Response", &response)) {
            Logger::getInstance()->log(std::string("OEMWebQueryCallback::result: no 'Response' object in response"), lsError);
          } else {
            if (json_object_object_get_ex(response, "ArticleName", &obj)) {
              productName = json_object_get_string(obj);
            }

            if (json_object_object_get_ex(response, "ArticleIcon", &obj)) {
              remoteIconPath = json_object_get_string(obj);
            }

            if (json_object_object_get_ex(response,
                                          "ArticleDescriptionForCustomer",
                                          &obj)) {
              productURL = json_object_get_string(obj);
            }

            if (json_object_object_get_ex(response, "DefaultName", &obj)) {
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

    ModelEventWithStrings* pEvent = new ModelEventWithStrings(ModelEvent::etDeviceOEMDataReady, m_deviceDSUID);
    pEvent->addParameter(state);
    pEvent->addStringParameter(productName);
    pEvent->addStringParameter(iconFile.string());
    pEvent->addStringParameter(productURL);
    pEvent->addStringParameter(defaultName);
    if(DSS::hasInstance()) {
      DSS::getInstance()->getModelMaintenance().addModelEvent(pEvent);
    } else {
      delete pEvent;
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

    boost::shared_ptr<OEMWebQuery::OEMWebQueryCallback> cb = boost::make_shared<OEMWebQuery::OEMWebQueryCallback>(m_deviceDSUID);
    WebserviceConnection::getInstanceMsHub()->request("public/MasterDataManagement/Article/v1_0/ArticleData/GetArticleData", parameters, GET, cb, false);
  }

  ModelMaintenance::VdcDataQuery::VdcDataQuery(boost::shared_ptr<Device> _device)
    : Task(),
      m_Device(_device)
  {}

  void ModelMaintenance::VdcDataQuery::run()
  {
    try {
      boost::shared_ptr<VdsdSpec_t> props = VdcHelper::getSpec(m_Device->getDSMeterDSID(), m_Device->getDSID());
      m_Device->setVdcHardwareModelGuid(props->hardwareModelGuid);
      m_Device->setVdcModelUID(props->modelUID);
      m_Device->setVdcVendorGuid(props->vendorGuid);
      m_Device->setVdcOemGuid(props->oemGuid);
      m_Device->setVdcConfigURL(props->configURL);
      m_Device->setVdcHardwareGuid(props->hardwareGuid);
      m_Device->setVdcHardwareInfo(props->hardwareInfo);
      m_Device->setVdcHardwareVersion(props->hardwareVersion);

      try {
        ModelFeatures::getInstance()->setFeatures(m_Device->getDeviceClass(), props->modelUID, props->modelFeatures);
      } catch (std::runtime_error& err) {
        Logger::getInstance()->log("Could not set model features for device " +
            dsuid2str(m_Device->getDSID()) + ", Message: " +
            err.what(), lsWarning);
      }
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
    }
    free(data);
  }

  ModelMaintenance::DatabaseDownload::DatabaseDownload(std::string _script_id,
                                                      std::string _url)
    : Task(),
      m_scriptId(_script_id), m_url(_url)
  {}

  void ModelMaintenance::DatabaseDownload::run()
  {
    if (m_scriptId.empty() || m_url.empty()) {
        Logger::getInstance()->log("DatabaseDownload: invalid configuration, "
                                   "missing parameters", lsError);
      return;
    }

    boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::DatabaseImported);

    pEvent->setProperty("scripd_id", m_scriptId);
    try {
      HttpClient http;
      std::string result;

      long req = http.get(m_url, &result);
      if (req != 200) {
        throw std::runtime_error("Could not download database from " + m_url +
                                 ": HTTP code " + intToString(req));
      }

      std::string database = DSS::getInstance()->getDatabaseDirectory() +
                             m_scriptId + ".db";
      if (!result.empty()) {
        SQLite3 sqlite(database, false);
        sqlite.exec(result);
        pEvent->setProperty("success", "1");
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    } catch (std::runtime_error &ex) {
      Logger::getInstance()->log(std::string("Exception when importing "
            "database: ") + ex.what(), lsError);
      pEvent->setProperty("success", "0");
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
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
    m_pRelayTarget = boost::make_shared<InternalEventRelayTarget>(boost::ref<EventInterpreterInternalRelay>(*pRelay));

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
        getTaskProcessor()->addEvent(boost::make_shared<OEMWebQuery>(device));
        device->setOemProductInfoState(DEVICE_OEM_LOADING);
      }
    }
    sendWebUpdateEvent();
  } // cleanupTerminatedScripts

  void ModelMaintenance::sendWebUpdateEvent(int _interval) {
    boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(kWebUpdateEventName);
    pEvent->setProperty("time", "+" + intToString(_interval));
    DSS::getInstance()->getEventInterpreter().getQueue().pushEvent(pEvent);
  } // sendCleanupEvent

  void ModelMaintenance::pollSensors(boost::shared_ptr<DeviceReference> pDevRef) {
    if (!pDevRef) {
      return;
    }

    boost::shared_ptr<Device> device = pDevRef->getDevice();
    if (!device) {
      return;
    }

    boost::shared_ptr<Zone> zone = m_pApartment->getZone(device->getZoneID());
    if (!zone) {
      return;
    }

    if (device->getSensorCount()) {
      DeviceBusInterface* busItf = m_pApartment->getBusInterface()->getDeviceBusInterface();

      foreach (boost::shared_ptr<DeviceSensor_t> sensor, device->getSensors()) {
        if (sensor->m_sensorPollInterval == 0) {
          continue;
        }
        try {
          if (zone->isZoneSensor(device, sensor->m_sensorType)) {
            busItf->getSensorValue(*device.get(), sensor->m_sensorIndex);
          }
        } catch (std::runtime_error& e) {
          Logger::getInstance()->log("pollSensors: could not query sensor from " +
          dsuid2str(device->getDSID()) + ", Message: " + e.what(), lsWarning);
        }
      }
    }
  } // pollSensors

  void ModelMaintenance::scheduleDeviceReadout(const dsuid_t &_dSMeterID,
                                               boost::shared_ptr<Task> task) {
    try {
      boost::shared_ptr<DSMeter> pMeter = m_pApartment->getDSMeterByDSID(_dSMeterID);
      if (pMeter->getState() == DSM_STATE_IDLE) {
        m_taskProcessor->addEvent(task);
      } else {
        boost::mutex::scoped_lock lock(m_readoutTasksMutex);
        m_deviceReadoutTasks.push_back(std::make_pair(_dSMeterID, task));
      }
    } catch(ItemNotFoundException& e) {
      log("scheduleDeviceReadou: Datamodel failure: " + std::string(e.what()), lsWarning);
    }

  }
} // namespace dss
