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
// TODO:libdsm
                     public    BusInterface,
                     public    DeviceBusInterface,
                     public    StructureQueryBusInterface,
                     public    MeteringBusInterface,
                     public    StructureModifyingBusInterface {
  private:
    bool isSimAddress(const uint8_t _addr);

    BusInterfaceHandler* m_pBusInterfaceHandler;
    DsmApiHandle_t m_dsmApiController;
    ModelMaintenance* m_pModelMaintenance;
    DSSim* m_pDSSim;
    DsmApiHandle_t m_dsmApiHandle;
    bool m_dsmApiReady;

    bool m_InitializeDS485Controller;

    void checkResultCode(const int _resultCode);
    void busReady();
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

    virtual bool isReady();

    //------------------------------------------------ Handling
    virtual void initialize();

    //------------------------------------------------ Specialized Commands (system)
    virtual std::vector<DSMeterSpec_t> getDSMeters();
    virtual DSMeterSpec_t getDSMeterSpec(dsid_t _dsMeterID);

    virtual std::vector<int> getZones(dsid_t _dsMeterID);
    virtual int getZoneCount(dsid_t _dsMeterID);
    virtual std::vector<int> getDevicesInZone(dsid_t _dsMeterID, const int _zoneID);
    virtual int getDevicesCountInZone(dsid_t _dsMeterID, const int _zoneID);

    virtual void setZoneID(dsid_t _dsMeterID, const devid_t _deviceID, const int _zoneID);
    virtual void createZone(dsid_t _dsMeterID, const int _zoneID);
    virtual void removeZone(dsid_t _dsMeterID, const int _zoneID);

    virtual int getGroupCount(dsid_t _dsMeterID, const int _zoneID);
    virtual std::vector<int> getGroups(dsid_t _dsMeterID, const int _zoneID);

    virtual std::vector<int> getGroupsOfDevice(dsid_t _dsMeterID, const int _deviceID);

    virtual void addToGroup(dsid_t _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeFromGroup(dsid_t _dsMeterID, const int _groupID, const int _deviceID);

    virtual int addUserGroup(const int _dsMeterID);
    virtual void removeUserGroup(const int _dsMeterID, const int _groupID);

    virtual void removeInactiveDevices(dsid_t _dsMeterID);

    virtual dss_dsid_t getDSIDOfDevice(dsid_t, const int _deviceID);

    virtual int getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID);

    virtual unsigned long getPowerConsumption(dsid_t _dsMeterID);
    virtual unsigned long getEnergyMeterValue(dsid_t _dsMeterID);
    virtual void requestEnergyMeterValue();
    virtual void requestPowerConsumption();
    virtual bool getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper);

    //------------------------------------------------ UDI
    virtual uint8_t dSLinkSend(const int _dsMeterID, devid_t _devAdr, uint8_t _value, uint8_t _flags);

    //------------------------------------------------ Device
    virtual uint16_t deviceGetParameterValue(devid_t _id, uint8_t _dsMeterID, int _paramID);
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID);
    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock);
    virtual bool isLocked(boost::shared_ptr<const Device> _device);

    void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size);
    virtual int getSensorValue(const Device& _device, const int _sensorID);
  }; // DS485Proxy

} // namespace dss

#endif
