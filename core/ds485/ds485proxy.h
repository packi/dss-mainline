/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef _DS485_PROXY_H_INCLUDED
#define _DS485_PROXY_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "core/ds485types.h"

// TODO_ libdsm
#include "core/businterface.h"
#include "core/subsystem.h"
#include "core/mutex.h"

#include <map>
#include <vector>

#include <digitalSTROM/dsm-api-v2/dsm-api.h>

#ifndef WIN32
  #include <ext/hash_map>
#else
  #include <hash_map>
#endif

#ifndef WIN32
using namespace __gnu_cxx;
#else
using namespace stdext;
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

namespace dss {

  class ModelEvent;
  class FrameBucketBase;
  class FrameBucketCollector;
  class BusInterfaceHandler;
  class ModelMaintenance;
  class DSSim;

  class DS485Proxy : public    Subsystem,
                     public    BusInterface,
                     public    DeviceBusInterface,
                     public    StructureQueryBusInterface,
                     public    MeteringBusInterface,
                     public    StructureModifyingBusInterface,
                     public    ActionRequestInterface {
  private:
    bool isSimAddress(const uint8_t _addr);

    BusInterfaceHandler* m_pBusInterfaceHandler;
    DsmApiHandle_t m_dsmApiController;
    ModelMaintenance* m_pModelMaintenance;
    DSSim* m_pDSSim;

    DsmApiHandle_t m_dsmApiHandle;
    bool m_dsmApiReady;
    std::string m_connection;

    bool m_InitializeDS485Controller;

    void checkResultCode(const int _resultCode);
    void busReady();
    
    void setConnection(const std::string& _connection) { m_connection = _connection; }
    const std::string& getConnection() const { return m_connection; }
    
    static void busChangeCallback(void* _userData, dsid_t *_id, int _flag);
    void handleBusChange(dsid_t *_id, int _flag);

    static void busStateCallback(void* _userData, bus_state_t _state);
    void handleBusState(bus_state_t _state);
    
    static void eventDeviceAccessibilityOffCallback(uint8_t _errorCode, void* _userData,
                                                    uint16_t _deviceID, uint16_t _zoneID, 
                                                    uint32_t _deviceDSID );
    void eventDeviceAccessibilityOff(uint8_t _errorCode, uint16_t _deviceID, uint16_t _zoneID,
                                     uint32_t _deviceDSID);
    static void eventDeviceAccessibilityOnCallback(uint8_t _errorCode, void* _userData, 
                                                   uint16_t _deviceID, uint16_t _zoneID,
                                                   uint32_t _deviceDSID);
    void eventDeviceAccessibilityOn(uint8_t _errorCode, uint16_t _deviceID, uint16_t _zoneID,
                                    uint32_t _deviceDSID);

  protected:
    virtual void doStart();
  public:
    DS485Proxy(DSS* _pDSS, ModelMaintenance* _pModelMaintenance, DSSim* _pDSSim);
    virtual ~DS485Proxy() {};

    virtual void shutdown();

    void setBusInterfaceHandler(BusInterfaceHandler* _value) { m_pBusInterfaceHandler = _value; }

    virtual DeviceBusInterface* getDeviceBusInterface() { return this; }
    virtual StructureQueryBusInterface* getStructureQueryBusInterface() { return this; }
    virtual MeteringBusInterface* getMeteringBusInterface() { return this; }
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() { return this; }
    virtual ActionRequestInterface* getActionRequestInterface() { return this; }

    virtual bool isReady();

    //------------------------------------------------ Handling
    virtual void initialize();

    //------------------------------------------------ Specialized Commands (system)
    virtual std::vector<DSMeterSpec_t> getDSMeters();
    virtual DSMeterSpec_t getDSMeterSpec(const dsid_t& _dsMeterID);

    virtual std::vector<int> getZones(const dsid_t& _dsMeterID);
    virtual int getZoneCount(const dsid_t& _dsMeterID);
    virtual std::vector<int> getDevicesInZone(const dsid_t& _dsMeterID, const int _zoneID);
    virtual int getDevicesCountInZone(const dsid_t& _dsMeterID, const int _zoneID);

    virtual void setZoneID(const dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID);
    virtual void createZone(const dsid_t& _dsMeterID, const int _zoneID);
    virtual void removeZone(const dsid_t& _dsMeterID, const int _zoneID);

    virtual int getGroupCount(const dsid_t& _dsMeterID, const int _zoneID);
    virtual std::vector<int> getGroups(const dsid_t& _dsMeterID, const int _zoneID);

    virtual std::vector<int> getGroupsOfDevice(const dsid_t& _dsMeterID, const int _deviceID);

    virtual void addToGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeFromGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID);

    virtual int addUserGroup(const int _dsMeterID);
    virtual void removeUserGroup(const int _dsMeterID, const int _groupID);

    virtual void removeInactiveDevices(const dsid_t& _dsMeterID);

    virtual dss_dsid_t getDSIDOfDevice(const dsid_t&, const int _deviceID);

    virtual int getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID);

    virtual unsigned long getPowerConsumption(const dsid_t& _dsMeterID);
    virtual unsigned long getEnergyMeterValue(const dsid_t& _dsMeterID);
    virtual void requestEnergyMeterValue();
    virtual void requestPowerConsumption();
    virtual bool getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper);
    
    virtual void callScene(AddressableModelItem *pTarget, const uint16_t scene);
    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t scene);
    virtual void undoScene(AddressableModelItem *pTarget);
    virtual void blink(AddressableModelItem *pTarget);
    virtual void increaseValue(AddressableModelItem *pTarget);
    virtual void decreaseValue(AddressableModelItem *pTarget);
    virtual void startDim(AddressableModelItem *pTarget, const bool _directionUp);
    virtual void endDim(AddressableModelItem *pTarget);
    virtual void setValue(AddressableModelItem *pTarget, const double _value);

    //------------------------------------------------ Device
    virtual uint16_t deviceGetParameterValue(devid_t _id, const dss_dsid_t& _dsMeterID, int _paramID);
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID);
    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock);
    virtual bool isLocked(boost::shared_ptr<const Device> _device);

    void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size);
    virtual int getSensorValue(const Device& _device, const int _sensorID);
  }; // DS485Proxy

} // namespace dss

#endif
