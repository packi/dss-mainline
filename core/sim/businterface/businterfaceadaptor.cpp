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

#include "core/event.h"

#include "core/model/device.h"
#include "core/model/group.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"

#include "core/sim/dssim.h"
#include "core/sim/dsmetersim.h"
#include "core/sim/businterface/simbusinterface.h"

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

    virtual void setValue(const Device& _device, uint8_t _value) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        m_pSimulationInterface->setValue(_device, _value);
      } else {
        m_pInner->setValue(_device, _value);
      }
    }

    virtual int getSensorValue(const Device& _device, const int _sensorID) {
      if(isHandledBySimulation(_device.getDSMeterDSID())) {
        return m_pSimulationInterface->getSensorValue(_device, _sensorID);
      } else {
        return m_pInner->getSensorValue(_device, _sensorID);
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

    virtual void callScene(AddressableModelItem *pTarget, const uint16_t scene) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->callScene(pTarget, scene);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->callScene(pTarget, scene);
      }
      Group* pGroup = dynamic_cast<Group*>(pTarget);
      if(pGroup != NULL) {
        m_pEventSink->onGroupCallScene(NULL, NullDSID, pGroup->getZoneID(),
                                       pGroup->getID(), scene);
      }
    }

    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t scene) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->saveScene(pTarget, scene);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->saveScene(pTarget, scene);
      }
    }

    virtual void undoScene(AddressableModelItem *pTarget) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->undoScene(pTarget);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->undoScene(pTarget);
      }
    }

    virtual void blink(AddressableModelItem *pTarget) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->blink(pTarget);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->blink(pTarget);
      }
    }

    virtual void setValue(AddressableModelItem *pTarget, const uint8_t _value) {
      if(targetIsSim(pTarget)) {
        m_pSimulationInterface->setValue(pTarget, _value);
      }
      if(targetIsInner(pTarget)) {
        m_pInner->setValue(pTarget, _value);
      }
    }

  private:

    bool targetIsSim(AddressableModelItem* _pTarget) {
      Device* pDevice = static_cast<Device*>(_pTarget);
      if(pDevice != NULL) {
        return isHandledBySimulation(pDevice->getDSMeterDSID());
      }
      return true;
    }

    bool targetIsInner(AddressableModelItem* _pTarget) {
      Device* pDevice = static_cast<Device*>(_pTarget);
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

    virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
      m_pSimulationInterface->sceneSetName(_zoneID, _groupID, _sceneNumber, _name);
      m_pInner->sceneSetName(_zoneID, _groupID, _sceneNumber, _name);
    }

    virtual void createGroup(uint16_t _zoneID, uint8_t _groupID) {
      m_pSimulationInterface->createGroup(_zoneID, _groupID);
      m_pInner->createGroup(_zoneID, _groupID);
    }

    virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID) {
      m_pSimulationInterface->removeGroup(_zoneID, _groupID);
      m_pInner->removeGroup(_zoneID, _groupID);
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

    virtual std::vector<int> getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getDevicesInZone(_dsMeterID, _zoneID);
      } else {
        return m_pInner->getDevicesInZone(_dsMeterID, _zoneID);
      }
    }

    virtual std::vector<int> getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getGroups(_dsMeterID, _zoneID);
      } else {
        return m_pInner->getGroups(_dsMeterID, _zoneID);
      }
    }

    virtual std::vector<int> getGroupsOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getGroupsOfDevice(_dsMeterID, _deviceID);
      } else {
        return m_pInner->getGroupsOfDevice(_dsMeterID, _deviceID);
      }
    }

    virtual dss_dsid_t getDSIDOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getDSIDOfDevice(_dsMeterID, _deviceID);
      } else {
        return m_pInner->getDSIDOfDevice(_dsMeterID, _deviceID);
      }
    }

    virtual int getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID) {
      if(isHandledBySimulation(_dsMeterID)) {
        return m_pSimulationInterface->getLastCalledScene(_dsMeterID, _zoneID, _groupID);
      } else {
        return m_pInner->getLastCalledScene(_dsMeterID, _zoneID, _groupID);
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

    virtual bool isLocked(boost::shared_ptr<const Device> _device) {
      if(isHandledBySimulation(_device->getDSMeterDSID())) {
        return m_pSimulationInterface->isLocked(_device);
      } else {
        return m_pInner->isLocked(_device);
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
        return m_pSimulationInterface->getPowerConsumption(_dsMeterID);
      } else {
        return m_pInner->getPowerConsumption(_dsMeterID);
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
                  boost::shared_ptr<EventInterpreter> _pEventInterpreter,
                  boost::shared_ptr<ModelMaintenance> _pModelMaintenance)
    : m_pSimulation(_pSimulation),
      m_pInnerBusInterface(_pInnerBusInterface),
      m_pSimBusInterface(_pSimBusInterface),
      m_pApartment(_pApartment),
      m_pEventInterpreter(_pEventInterpreter),
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
                                  const int _sceneID) {
      boost::shared_ptr<BusInterface> target;
      if(_source == m_pInnerBusInterface.get()) {
        boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
        if(pMeter != NULL) {
          pMeter->groupCallScene(_zoneID, _groupID, _sceneID);
        }
      }
      ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneGroup);
      pEvent->addParameter(_zoneID);
      pEvent->addParameter(_groupID);
      pEvent->addParameter(_sceneID);
      m_pModelMaintenance->addModelEvent(pEvent);
      boost::shared_ptr<Event> pHEvent;
      try {
        pHEvent.reset(new Event("callScene", m_pApartment->getZone(_zoneID)));
      } catch(ItemNotFoundException&) {
        pHEvent.reset(new Event("callScene"));
      }
      pHEvent->setProperty("sceneID", intToString(_sceneID));
      m_pEventInterpreter->getQueue().pushEvent(pHEvent);
    } // onGroupCallScene

    virtual void onMeteringEvent(BusInterface* _source,
                                 const dss_dsid_t& _dsMeterID,
                                 const int _powerW,
                                 const int _energyWh) {
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etMeteringValues,
                                                  _dsMeterID);
      pEvent->addParameter(_powerW);
      pEvent->addParameter(_energyWh);
      m_pModelMaintenance->addModelEvent(pEvent);
    }

  private:
    boost::shared_ptr<DSSim> m_pSimulation;
    boost::shared_ptr<BusInterface> m_pInnerBusInterface;
    boost::shared_ptr<BusInterface> m_pSimBusInterface;
    boost::shared_ptr<Apartment> m_pApartment;
    boost::shared_ptr<EventInterpreter> m_pEventInterpreter;
    boost::shared_ptr<ModelMaintenance> m_pModelMaintenance;
  }; // BusEventRelay

  class BusInterfaceAdaptor::Implementation : public BusInterface {
  public:
    Implementation(boost::shared_ptr<DSSim> _pSimulation,
                   boost::shared_ptr<BusInterface> _pInner,
                   boost::shared_ptr<SimBusInterface> _pSimBusInterface,
                   boost::shared_ptr<ModelMaintenance> _pModelMaintenance,
                   boost::shared_ptr<Apartment> _pApartment,
                   boost::shared_ptr<EventInterpreter> _pEventInterpreter)
    : m_pSimulation(_pSimulation),
      m_pInnerBusInterface(_pInner),
      m_pSimBusInterface(_pSimBusInterface)
    {
      assert(_pSimulation != NULL);
      assert(_pInner != NULL);
      assert(_pSimBusInterface != NULL);
      assert(_pModelMaintenance != NULL);
      assert(_pApartment != NULL);
      assert(_pEventInterpreter != NULL);
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
                          _pEventInterpreter,
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
                                           boost::shared_ptr<Apartment> _pApartment,
                                           boost::shared_ptr<EventInterpreter> _pEventInterpreter)
  : m_pInnerBusInterface(_pInner),
    m_pSimulation(_pSimulation),
    m_pSimBusInterface(_pSimBusInterface),
    m_pImplementation(new Implementation(m_pSimulation, m_pInnerBusInterface,
                                         m_pSimBusInterface, _pModelMaintenance,
                                         _pApartment, _pEventInterpreter))
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


} // namespace dss
