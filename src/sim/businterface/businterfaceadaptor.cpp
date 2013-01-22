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

#include "businterfaceadaptor.h"

#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/modulator.h"
#include "src/model/set.h"
#include "src/model/zone.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"

#include "src/sim/dssim.h"
#include "src/sim/dsmetersim.h"
#include "src/sim/businterface/simbusinterface.h"

namespace dss {

  class AdaptorBase {
  public:
    AdaptorBase(boost::shared_ptr<DSSim> _pSimulation)
    : m_pSimulation(_pSimulation)
    { }

    bool isHandledBySimulation(dss_dsid_t _meterDSID) {
      return m_pSimulation->getDSMeter(_meterDSID) != NULL;
    } // isHandledBySimulation

    bool isHandledBySimulation(int _meterDSID) {
      return m_pSimulation->getDSMeter(_meterDSID) != NULL;
    } // isHandledBySimulation

    boost::shared_ptr<DSSim> getSimulation() { return m_pSimulation; }
  private:
    boost::shared_ptr<DSSim> m_pSimulation;
  };

  class DeviceAdaptor : public AdaptorBase,
                        public DeviceBusInterface {
  public:
    DeviceAdaptor(boost::shared_ptr<DSSim> _pSimulation, DeviceBusInterface* _pInner, DeviceBusInterface* _pSimulationInterface)
    : AdaptorBase(_pSimulation),
      m_pInner(_pInner),
      m_pSimulationInterface(_pSimulationInterface)
    { }

    virtual uint8_t getDeviceConfig(const Device& _device,
                                    uint8_t _configClass,
                                    uint8_t _configIndex) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        return m_pSimulationInterface->getDeviceConfig(_device, _configClass,
                                                       _configIndex);
      } else {
        return m_pInner->getDeviceConfig(_device, _configClass, _configIndex);
      }
    }

    virtual uint16_t getDeviceConfigWord(const Device& _device,
                                    uint8_t _configClass,
                                    uint8_t _configIndex) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        return m_pSimulationInterface->getDeviceConfigWord(_device,
                                                           _configClass,
                                                           _configIndex);
      } else {
        return m_pInner->getDeviceConfigWord(_device, _configClass,
                                             _configIndex);
      }
    }

    virtual void setDeviceConfig(const Device& _device, uint8_t _configClass,
                                 uint8_t _configIndex, uint8_t _value) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        m_pSimulationInterface->setDeviceConfig(_device, _configClass,
                                                _configIndex, _value);
      } else {
        m_pInner->setDeviceConfig(_device, _configClass, _configIndex, _value);
      }
    }

    virtual void setDeviceProgMode(const Device& _device, uint8_t _modeId) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        m_pSimulationInterface->setDeviceProgMode(_device, _modeId);
      } else {
        m_pInner->setDeviceProgMode(_device, _modeId);
      }
    }

    virtual void setValue(const Device& _device, uint8_t _value) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        m_pSimulationInterface->setValue(_device, _value);
      } else {
        m_pInner->setValue(_device, _value);
      }
    }

    virtual void addGroup(const Device& _device, const int _groupId) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        m_pSimulationInterface->addGroup(_device, _groupId);
      } else {
        m_pInner->addGroup(_device, _groupId);
      }
    }

    virtual void removeGroup(const Device& _device, const int _groupId) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        m_pSimulationInterface->removeGroup(_device, _groupId);
      } else {
        m_pInner->removeGroup(_device, _groupId);
      }
    }

    virtual uint32_t getSensorValue(const Device& _device, const int _sensorIndex) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        return m_pSimulationInterface->getSensorValue(_device, _sensorIndex);
      } else {
        return m_pInner->getSensorValue(_device, _sensorIndex);
      }
    }

    virtual uint8_t getSensorType(const Device& _device, const int _sensorIndex) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        return m_pSimulationInterface->getSensorType(_device, _sensorIndex);
      } else {
        return m_pInner->getSensorType(_device, _sensorIndex);
      }
    }

    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        m_pSimulationInterface->lockOrUnlockDevice(_device, _lock);
      } else {
        m_pInner->lockOrUnlockDevice(_device, _lock);
      }
    }

    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality(const Device& _device) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        return m_pSimulationInterface->getTransmissionQuality(_device);
      } else {
        return m_pInner->getTransmissionQuality(_device);
      }
    }

  private:
    DeviceBusInterface* m_pInner;
    DeviceBusInterface* m_pSimulationInterface;
  }; // DeviceAdaptor

  class DeviceActionAdaptor : public AdaptorBase,
                              public ActionRequestInterface {
  public:
    DeviceActionAdaptor(boost::shared_ptr<DSSim> _pSimulation,
                        ActionRequestInterface* _pInner,
                        ActionRequestInterface* _pSimulationInterface,
                        boost::shared_ptr<BusEventSink> _pEventSink)
    : AdaptorBase(_pSimulation),
      m_pInner(_pInner),
      m_pSimulationInterface(_pSimulationInterface),
      m_pEventSink(_pEventSink)
    { }

    virtual void callScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene, const bool _force) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->callScene(pTarget, _origin, scene, _force);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->callScene(pTarget, _origin, scene, _force);
      }
      Group* pGroup = dynamic_cast<Group*>(pTarget);
      if(pGroup != NULL) {
        m_pEventSink->onGroupCallScene(NULL, NullDSID, pGroup->getZoneID(),
                                       pGroup->getID(), _origin, scene, _force);
      }
      Device* pDevice = dynamic_cast<Device*>(pTarget);
      if(pDevice != NULL) {
        m_pEventSink->onDeviceCallScene(NULL, pDevice->getDSMeterDSID(), pDevice->getShortAddress(),
                                       _origin, scene, _force);
      }
    }

    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->saveScene(pTarget, _origin, scene);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->saveScene(pTarget, _origin, scene);
      }
    }

    virtual void undoScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->undoScene(pTarget, _origin, scene);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->undoScene(pTarget, _origin, scene);
      }
      Group* pGroup = dynamic_cast<Group*>(pTarget);
      if(pGroup != NULL) {
        m_pEventSink->onGroupUndoScene(NULL, NullDSID, pGroup->getZoneID(),
                                       pGroup->getID(), _origin, scene, true);
      }
      Device* pDevice = dynamic_cast<Device*>(pTarget);
      if(pDevice != NULL) {
        m_pEventSink->onDeviceUndoScene(NULL, pDevice->getDSMeterDSID(), pDevice->getShortAddress(),
                                       _origin, scene, true);
      }
    }

    virtual void undoSceneLast(AddressableModelItem *pTarget, const uint16_t _origin) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->undoSceneLast(pTarget, _origin);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->undoSceneLast(pTarget, _origin);
      }
      Group* pGroup = dynamic_cast<Group*>(pTarget);
      if(pGroup != NULL) {
        m_pEventSink->onGroupUndoScene(NULL, NullDSID, pGroup->getZoneID(),
                                       pGroup->getID(), _origin, -1, false);
      }
      Device* pDevice = dynamic_cast<Device*>(pTarget);
      if(pDevice != NULL) {
        m_pEventSink->onDeviceUndoScene(NULL, pDevice->getDSMeterDSID(), pDevice->getShortAddress(),
                                        _origin, -1, false);
      }
    }

    virtual void blink(AddressableModelItem *pTarget, const uint16_t _origin) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->blink(pTarget, _origin);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->blink(pTarget, _origin);
      }
    }

    virtual void setValue(AddressableModelItem *pTarget, const uint16_t _origin, const uint8_t _value) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->setValue(pTarget, _origin, _value);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->setValue(pTarget, _origin, _value);
      }
    }

  private:

    bool targetIsSim(AddressableModelItem* _pTarget) {
      Device* pDevice = dynamic_cast<Device*>(_pTarget);
      if(pDevice != NULL) {
        return isHandledBySimulation(pDevice->getDSMeterDSID());
      }
      return true;
    }

    bool targetIsInner(AddressableModelItem* _pTarget) {
      Device* pDevice = dynamic_cast<Device*>(_pTarget);
      if(pDevice != NULL) {
        return !isHandledBySimulation(pDevice->getDSMeterDSID());
      }
      return true;
    }
  private:
    ActionRequestInterface* m_pInner;
    ActionRequestInterface* m_pSimulationInterface;
    boost::shared_ptr<ModelMaintenance> m_pModelMaintenance;
    boost::shared_ptr<BusEventSink> m_pEventSink;
  }; // DeviceActionAdaptor

  class StructureModifyAdaptor : public AdaptorBase,
                                 public StructureModifyingBusInterface {
  public:
    StructureModifyAdaptor(boost::shared_ptr<DSSim> _pSimulation, StructureModifyingBusInterface* _pInner, StructureModifyingBusInterface* _pSimulationInterface)
    : AdaptorBase(_pSimulation),
      m_pInner(_pInner),
      m_pSimulationInterface(_pSimulationInterface)
    { }

    virtual void setZoneID(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        m_pSimulationInterface->setZoneID(_dsMeterID, _deviceID, _zoneID);
      } else {
        m_pInner->setZoneID(_dsMeterID, _deviceID, _zoneID);
      }
    }

    virtual void createZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        m_pSimulationInterface->createZone(_dsMeterID, _zoneID);
      } else {
        m_pInner->createZone(_dsMeterID, _zoneID);
      }
    }

    virtual void removeZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        m_pSimulationInterface->removeZone(_dsMeterID, _zoneID);
      } else {
        m_pInner->removeZone(_dsMeterID, _zoneID);
      }
    }

    virtual void addToGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
      if(isHandledBySimulation(_dsMeterID)) {
        m_pSimulationInterface->addToGroup(_dsMeterID, _groupID, _deviceID);
      } else {
        m_pInner->addToGroup(_dsMeterID, _groupID, _deviceID);
      }
    }

    virtual void removeFromGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
      if(isHandledBySimulation(_dsMeterID)) {
        m_pSimulationInterface->removeFromGroup(_dsMeterID, _groupID, _deviceID);
      } else {
        m_pInner->removeFromGroup(_dsMeterID, _groupID, _deviceID);
      }
    }

    virtual void removeDeviceFromDSMeter(const dss_dsid_t& _dsMeterID, const int _deviceID) {
      if(isHandledBySimulation(_dsMeterID)) {
        m_pSimulationInterface->removeDeviceFromDSMeter(_dsMeterID, _deviceID);
      } else {
        m_pInner->removeDeviceFromDSMeter(_dsMeterID, _deviceID);
      }
    }

    virtual void removeDeviceFromDSMeters(const dss_dsid_t& _deviceDSID) {
      m_pSimulationInterface->removeDeviceFromDSMeters(_deviceDSID);
      m_pInner->removeDeviceFromDSMeters(_deviceDSID);
    }

    virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
      m_pSimulationInterface->sceneSetName(_zoneID, _groupID, _sceneNumber, _name);
      m_pInner->sceneSetName(_zoneID, _groupID, _sceneNumber, _name);
    }

    virtual void deviceSetName(dss_dsid_t _meterDSID, devid_t _deviceID, const std::string& _name) {
      m_pSimulationInterface->deviceSetName(_meterDSID, _deviceID, _name);
      m_pInner->deviceSetName(_meterDSID, _deviceID, _name);
    }

    virtual void meterSetName(dss_dsid_t _meterDSID, const std::string& _name) {
      m_pSimulationInterface->meterSetName(_meterDSID, _name);
      m_pInner->meterSetName(_meterDSID, _name);
    }

    virtual void createGroup(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name) {
      m_pSimulationInterface->createGroup(_zoneID, _groupID, _standardGroupID, _name);
      m_pInner->createGroup(_zoneID, _groupID, _standardGroupID, _name);
    }

    virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID) {
      m_pSimulationInterface->removeGroup(_zoneID, _groupID);
      m_pInner->removeGroup(_zoneID, _groupID);
    }

    virtual void groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name) {
      m_pSimulationInterface->groupSetName(_zoneID, _groupID, _name);
      m_pInner->groupSetName(_zoneID, _groupID, _name);
    }

    virtual void groupSetStandardID(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID) {
      m_pSimulationInterface->groupSetStandardID(_zoneID, _groupID, _standardGroupID);
      m_pInner->groupSetStandardID(_zoneID, _groupID, _standardGroupID);
    }

    virtual void sensorPush(uint16_t _zoneID, dss_dsid_t _sourceID, uint8_t _sensorType, uint16_t _sensorValue) {
      m_pSimulationInterface->sensorPush(_zoneID, _sourceID, _sensorType, _sensorValue);
      m_pInner->sensorPush(_zoneID, _sourceID, _sensorType, _sensorValue);
    }

    virtual void setButtonSetsLocalPriority(const dss_dsid_t& _dsMeterID,
                                            const devid_t _deviceID,
                                            bool _setsPriority) {
      if(isHandledBySimulation(_dsMeterID)) {
        m_pSimulationInterface->setButtonSetsLocalPriority(_dsMeterID,
                                                           _deviceID,
                                                           _setsPriority);
      } else {
        m_pInner->setButtonSetsLocalPriority(_dsMeterID, _deviceID,
                                             _setsPriority);
      }
    }
  private:
    StructureModifyingBusInterface* m_pInner;
    StructureModifyingBusInterface* m_pSimulationInterface;
  }; // StructureModifyAdaptor

  class StructureQueryAdaptor : public AdaptorBase,
                                public StructureQueryBusInterface {
  public:
    StructureQueryAdaptor(boost::shared_ptr<DSSim> _pSimulation, StructureQueryBusInterface* _pInner, StructureQueryBusInterface* _pSimulationInterface)
    : AdaptorBase(_pSimulation),
      m_pInner(_pInner),
      m_pSimulationInterface(_pSimulationInterface)
    { }

    virtual std::vector<DSMeterSpec_t> getDSMeters() {
      std::vector<DSMeterSpec_t> result;
      result = m_pInner->getDSMeters();
      std::vector<DSMeterSpec_t> temp = m_pSimulationInterface->getDSMeters();
      result.insert(result.end(), temp.begin(), temp.end());
      return result;
    }

    virtual DSMeterSpec_t getDSMeterSpec(const dss_dsid_t& _dsMeterID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getDSMeterSpec(_dsMeterID);
      } else {
        return m_pInner->getDSMeterSpec(_dsMeterID);
      }
    }

    virtual std::vector<int> getZones(const dss_dsid_t& _dsMeterID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getZones(_dsMeterID);
      } else {
        return m_pInner->getZones(_dsMeterID);
      }
    }

    virtual std::vector<DeviceSpec_t> getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getDevicesInZone(_dsMeterID, _zoneID);
      } else {
        return m_pInner->getDevicesInZone(_dsMeterID, _zoneID);
      }
    }

    virtual std::vector<DeviceSpec_t> getInactiveDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getInactiveDevicesInZone(_dsMeterID, _zoneID);
      } else {
        return m_pInner->getInactiveDevicesInZone(_dsMeterID, _zoneID);
      }
    }

    virtual std::vector<GroupSpec_t> getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getGroups(_dsMeterID, _zoneID);
      } else {
        return m_pInner->getGroups(_dsMeterID, _zoneID);
      }
    }

    virtual std::vector<std::pair<int, int> > getLastCalledScenes(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getLastCalledScenes(_dsMeterID, _zoneID);
      } else {
        return m_pInner->getLastCalledScenes(_dsMeterID, _zoneID);
      }
    }

    virtual bool getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getEnergyBorder(_dsMeterID, _lower, _upper);
      } else {
        return m_pInner->getEnergyBorder(_dsMeterID, _lower, _upper);
      }
    }

    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->deviceGetSpec(_id, _dsMeterID);
      } else {
        return m_pInner->deviceGetSpec(_id, _dsMeterID);
      }
    }

    virtual std::string getSceneName(dss_dsid_t _dsMeterID,
                                     boost::shared_ptr<Group> _group,
                                     const uint8_t _sceneNumber) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getSceneName(_dsMeterID, _group, _sceneNumber);
      } else {
        return m_pInner->getSceneName(_dsMeterID, _group, _sceneNumber);
      }
    } // getSceneName

    virtual DSMeterHash_t getDSMeterHash(const dss_dsid_t& _dsMeterID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getDSMeterHash(_dsMeterID);
      } else {
        return m_pInner->getDSMeterHash(_dsMeterID);
      }
    }
  private:
    StructureQueryBusInterface* m_pInner;
    StructureQueryBusInterface* m_pSimulationInterface;
  }; // StructureQueryAdaptor

  class MeteringAdaptor : public AdaptorBase,
                          public MeteringBusInterface {
  public:
    MeteringAdaptor(boost::shared_ptr<DSSim> _pSimulation, MeteringBusInterface* _pInner, MeteringBusInterface* _pSimulationInterface)
    : AdaptorBase(_pSimulation),
      m_pInner(_pInner),
      m_pSimulationInterface(_pSimulationInterface)
    { }

    virtual void requestMeterData() {
      m_pInner->requestMeterData();
      m_pSimulationInterface->requestMeterData();
    } // requestMeterData

    virtual unsigned long getPowerConsumption(const dss_dsid_t& _dsMeterID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getPowerConsumption(_dsMeterID);
      } else {
        return m_pInner->getPowerConsumption(_dsMeterID);
      }
    } // getPowerConsumption

    virtual unsigned long getEnergyMeterValue(const dss_dsid_t& _dsMeterID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getEnergyMeterValue(_dsMeterID);
      } else {
        return m_pInner->getEnergyMeterValue(_dsMeterID);
      }
    } // getEnergyMeterValue

  private:
    MeteringBusInterface* m_pInner;
    MeteringBusInterface* m_pSimulationInterface;
  }; // MeteringBusInterface

  class BusEventRelay : public BusEventSink {
  public:

    BusEventRelay(boost::shared_ptr<DSSim> _pSimulation,
                  boost::shared_ptr<BusInterface> _pInnerBusInterface,
                  boost::shared_ptr<BusInterface> _pSimBusInterface,
                  boost::shared_ptr<Apartment> _pApartment,
                  boost::shared_ptr<ModelMaintenance> _pModelMaintenance)
    : m_pSimulation(_pSimulation),
      m_pInnerBusInterface(_pInnerBusInterface),
      m_pSimBusInterface(_pSimBusInterface),
      m_pApartment(_pApartment),
      m_pModelMaintenance(_pModelMaintenance)
    {
      m_pInnerBusInterface->setBusEventSink(this);
      m_pSimBusInterface->setBusEventSink(this);
    }

    virtual ~BusEventRelay() {
      m_pInnerBusInterface->setBusEventSink(NULL);
      m_pSimBusInterface->setBusEventSink(NULL);
    }

    virtual void onGroupCallScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _force) {
      boost::shared_ptr<BusInterface> target;
      if(_source == m_pInnerBusInterface.get()) {
        try {
          boost::shared_ptr<Zone> pZone = m_pApartment->getZone(_zoneID);
          boost::shared_ptr<Group> pGroup = pZone->getGroup(_groupID);
          m_pSimBusInterface->getActionRequestInterface()->callScene(pGroup.get(), _originDeviceId, _sceneID, _force);
        } catch(std::runtime_error& e) {
        }
      }
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneGroup, _dsMeterID);
      pEvent->addParameter(_zoneID);
      pEvent->addParameter(_groupID);
      pEvent->addParameter(_originDeviceId);
      pEvent->addParameter(_sceneID);
      pEvent->addParameter(_force);
      m_pModelMaintenance->addModelEvent(pEvent);
    } // onGroupCallScene

    virtual void onGroupUndoScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _explicit) {
      boost::shared_ptr<BusInterface> target;
      if(_source == m_pInnerBusInterface.get()) {
        try {
          boost::shared_ptr<Zone> pZone = m_pApartment->getZone(_zoneID);
          boost::shared_ptr<Group> pGroup = pZone->getGroup(_groupID);
          if (_explicit) {
            m_pSimBusInterface->getActionRequestInterface()->undoScene(pGroup.get(), _originDeviceId, _sceneID);
          } else {
            m_pSimBusInterface->getActionRequestInterface()->undoSceneLast(pGroup.get(), _originDeviceId);
          }
        } catch(std::runtime_error& e) {
        }
      }
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etUndoSceneGroup, _dsMeterID);
      pEvent->addParameter(_zoneID);
      pEvent->addParameter(_groupID);
      pEvent->addParameter(_originDeviceId);
      if (_explicit) {
        pEvent->addParameter(_sceneID);
      }
      m_pModelMaintenance->addModelEvent(pEvent);
    } // onGroupUndoScene

    virtual void onDeviceCallScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _force) {
      boost::shared_ptr<BusInterface> target;
      if(_source == m_pInnerBusInterface.get()) {
        try {
          DeviceReference devRef =  m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_deviceID, _dsMeterID);
          m_pSimBusInterface->getActionRequestInterface()->callScene(devRef.getDevice().get(), _originDeviceId, _sceneID, _force);
        } catch(std::runtime_error& e) {
        }
      }
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneDevice, _dsMeterID);
      pEvent->addParameter(_deviceID);
      pEvent->addParameter(_originDeviceId);
      pEvent->addParameter(_sceneID);
      pEvent->addParameter(_force);
      m_pModelMaintenance->addModelEvent(pEvent);
    } // onDeviceCallScene

    virtual void onDeviceUndoScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _explicit) {
      boost::shared_ptr<BusInterface> target;
      if(_source == m_pInnerBusInterface.get()) {
        try {
          DeviceReference devRef =  m_pApartment->getDSMeterByDSID(_dsMeterID)->getDevices().getByBusID(_deviceID, _dsMeterID);
          if (_explicit) {
            m_pSimBusInterface->getActionRequestInterface()->undoScene(devRef.getDevice().get(), _originDeviceId, _sceneID);
          } else {
            m_pSimBusInterface->getActionRequestInterface()->undoSceneLast(devRef.getDevice().get(), _originDeviceId);
          }
        } catch(std::runtime_error& e) {
        }
      }
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etUndoSceneDevice, _dsMeterID);
      pEvent->addParameter(_deviceID);
      pEvent->addParameter(_sceneID);
      pEvent->addParameter(_explicit);
      pEvent->addParameter(_originDeviceId);
      m_pModelMaintenance->addModelEvent(pEvent);
    } // onDeviceUndoScene

    virtual void onMeteringEvent(BusInterface* _source,
                                 const dss_dsid_t& _dsMeterID,
                                 const unsigned int _powerW,
                                 const unsigned int _energyWs) {
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etMeteringValues,
                                                  _dsMeterID);
      pEvent->addParameter(_powerW);
      pEvent->addParameter(_energyWs);
      m_pModelMaintenance->addModelEvent(pEvent);
    }

  private:
    boost::shared_ptr<DSSim> m_pSimulation;
    boost::shared_ptr<BusInterface> m_pInnerBusInterface;
    boost::shared_ptr<BusInterface> m_pSimBusInterface;
    boost::shared_ptr<Apartment> m_pApartment;
    boost::shared_ptr<ModelMaintenance> m_pModelMaintenance;
  }; // BusEventRelay

  class BusInterfaceAdaptor::Implementation : public BusInterface {
  public:
    Implementation(boost::shared_ptr<DSSim> _pSimulation,
                   boost::shared_ptr<BusInterface> _pInner,
                   boost::shared_ptr<SimBusInterface> _pSimBusInterface,
                   boost::shared_ptr<ModelMaintenance> _pModelMaintenance,
                   boost::shared_ptr<Apartment> _pApartment)
    : m_pSimulation(_pSimulation),
      m_pInnerBusInterface(_pInner),
      m_pSimBusInterface(_pSimBusInterface)
    {
      assert(_pSimulation != NULL);
      assert(_pInner != NULL);
      assert(_pSimBusInterface != NULL);
      assert(_pModelMaintenance != NULL);
      assert(_pApartment != NULL);
      m_pDeviceBusInterface.reset(
        new DeviceAdaptor(m_pSimulation,
                          m_pInnerBusInterface->getDeviceBusInterface(),
                          m_pSimBusInterface->getDeviceBusInterface()));
      m_pStructureModifyingBusInterface.reset(
        new StructureModifyAdaptor(m_pSimulation,
                                   m_pInnerBusInterface->getStructureModifyingBusInterface(),
                                   m_pSimBusInterface->getStructureModifyingBusInterface()));
      m_pStructureQueryBusInterface.reset(
        new StructureQueryAdaptor(m_pSimulation,
                                   m_pInnerBusInterface->getStructureQueryBusInterface(),
                                   m_pSimBusInterface->getStructureQueryBusInterface()));
      m_pMeteringBusInterface.reset(
        new MeteringAdaptor(m_pSimulation,
                            m_pInnerBusInterface->getMeteringBusInterface(),
                            m_pSimBusInterface->getMeteringBusInterface()));

      m_pBusEventRelay.reset(
        new BusEventRelay(m_pSimulation,
                          m_pInnerBusInterface,
                          m_pSimBusInterface,
                          _pApartment,
                          _pModelMaintenance));
      m_pActionRequestInterface.reset(
        new DeviceActionAdaptor(m_pSimulation,
                                m_pInnerBusInterface->getActionRequestInterface(),
                                m_pSimBusInterface->getActionRequestInterface(),
                                m_pBusEventRelay));
    }

    virtual DeviceBusInterface* getDeviceBusInterface() {
      return m_pDeviceBusInterface.get();
    }

    virtual StructureQueryBusInterface* getStructureQueryBusInterface() {
      return m_pStructureQueryBusInterface.get();
    }

    virtual MeteringBusInterface* getMeteringBusInterface() {
      return m_pMeteringBusInterface.get();
    }

    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() {
      return m_pStructureModifyingBusInterface.get();
    }

    virtual ActionRequestInterface* getActionRequestInterface() {
      return m_pActionRequestInterface.get();
    }

    virtual void setBusEventSink(BusEventSink* _eventSink) { }

  private:
    boost::shared_ptr<DSSim> m_pSimulation;
    boost::shared_ptr<BusInterface> m_pInnerBusInterface;
    boost::shared_ptr<SimBusInterface> m_pSimBusInterface;
    boost::shared_ptr<DeviceBusInterface> m_pDeviceBusInterface;
    boost::shared_ptr<StructureQueryBusInterface> m_pStructureQueryBusInterface;
    boost::shared_ptr<StructureModifyingBusInterface> m_pStructureModifyingBusInterface;
    boost::shared_ptr<MeteringBusInterface> m_pMeteringBusInterface;
    boost::shared_ptr<ActionRequestInterface> m_pActionRequestInterface;
    boost::shared_ptr<BusEventRelay> m_pBusEventRelay;
  }; // Implementation

  //================================================== BusInterfaceAdaptor

  BusInterfaceAdaptor::BusInterfaceAdaptor(boost::shared_ptr<BusInterface> _pInner,
                                           boost::shared_ptr<DSSim> _pSimulation,
                                           boost::shared_ptr<SimBusInterface> _pSimBusInterface,
                                           boost::shared_ptr<ModelMaintenance> _pModelMaintenance,
                                           boost::shared_ptr<Apartment> _pApartment)
  : m_pInnerBusInterface(_pInner),
    m_pSimulation(_pSimulation),
    m_pSimBusInterface(_pSimBusInterface),
    m_pImplementation(new Implementation(m_pSimulation, m_pInnerBusInterface,
                                         m_pSimBusInterface, _pModelMaintenance,
                                         _pApartment))
  { }

  DeviceBusInterface* BusInterfaceAdaptor::getDeviceBusInterface() {
    return m_pImplementation->getDeviceBusInterface();
  } // getDeviceBusInterface

  StructureQueryBusInterface* BusInterfaceAdaptor::getStructureQueryBusInterface() {
    return m_pImplementation->getStructureQueryBusInterface();
  } // getStructureQueryBusInterface

  MeteringBusInterface* BusInterfaceAdaptor::getMeteringBusInterface() {
    return m_pImplementation->getMeteringBusInterface();
  } // getMeteringBusInterface

  StructureModifyingBusInterface* BusInterfaceAdaptor::getStructureModifyingBusInterface() {
    return m_pImplementation->getStructureModifyingBusInterface();
  } // getStructureModifyingBusInterface

  ActionRequestInterface* BusInterfaceAdaptor::getActionRequestInterface() {
    return m_pImplementation->getActionRequestInterface();
  } // getActionRequestInterface

  const std::string BusInterfaceAdaptor::getConnectionURI() {
    return m_pInnerBusInterface->getConnectionURI();
  }

} // namespace dss
