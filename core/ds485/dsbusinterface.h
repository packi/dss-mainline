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

#ifndef _DSBUSINTERFACE_H_INCLUDED
#define _DSBUSINTERFACE_H_INCLUDED

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

#include <boost/shared_ptr.hpp>

namespace dss {

  class ModelEvent;
  class FrameBucketBase;
  class FrameBucketCollector;
  class ModelMaintenance;
  class DSSim;

  class DSBusInterface : public    Subsystem,
                         public    BusInterface,
                         public    StructureQueryBusInterface,
                         public    StructureModifyingBusInterface {
  private:
    boost::shared_ptr<ActionRequestInterface> m_pActionRequestInterface;
    boost::shared_ptr<DeviceBusInterface> m_pDeviceBusInterface;
    boost::shared_ptr<MeteringBusInterface> m_pMeteringBusInterface;
    bool isSimAddress(const uint8_t _addr);

    ModelMaintenance* m_pModelMaintenance;
    DSSim* m_pDSSim;

    DsmApiHandle_t m_dsmApiHandle;
    bool m_dsmApiReady;
    std::string m_connectionURI;

    dsid_t m_broadcastDSID;

    void busReady();

    static void busChangeCallback(void* _userData, dsid_t *_id, int _flag);
    void handleBusChange(dsid_t *_id, int _flag);

    static void busStateCallback(void* _userData, bus_state_t _state);
    void handleBusState(bus_state_t _state);

    static void eventDeviceAccessibilityOffCallback(uint8_t _errorCode, void* _userData,
                                                   dsid_t _sourceDSMeterID,
                                                   dsid_t _destinationDSMeterID,
                                                    uint16_t _deviceID, uint16_t _zoneID,
                                                    uint32_t _deviceDSID );
    void eventDeviceAccessibilityOff(uint8_t _errorCode, dsid_t _dsMeterID, uint16_t _deviceID, 
                                     uint16_t _zoneID, uint32_t _deviceDSID);
    static void eventDeviceAccessibilityOnCallback(uint8_t _errorCode, void* _userData,
                                                   dsid_t _sourceDSMeterID,
                                                   dsid_t _destinationDSMeterID,
                                                   uint16_t _deviceID, uint16_t _zoneID,
                                                   uint32_t _deviceDSID);
    void eventDeviceAccessibilityOn(uint8_t _errorCode, dsid_t _dsMeterID, uint16_t _deviceID, 
                                    uint16_t _zoneID, uint32_t _deviceDSID);

    void handleBusCallScene(uint8_t _errorCode, dsid_t _sourceID, 
                            uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneID);
    static void handleBusCallSceneCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID,
                                           dsid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                           uint8_t _sceneID);

  protected:
    virtual void doStart();
  public:
    DSBusInterface(DSS* _pDSS, ModelMaintenance* _pModelMaintenance, DSSim* _pDSSim);
    virtual ~DSBusInterface() {};

    virtual void shutdown();

    virtual DeviceBusInterface* getDeviceBusInterface() { return m_pDeviceBusInterface.get(); }
    virtual StructureQueryBusInterface* getStructureQueryBusInterface() { return this; }
    virtual MeteringBusInterface* getMeteringBusInterface() { return m_pMeteringBusInterface.get(); }
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() { return this; }
    virtual ActionRequestInterface* getActionRequestInterface() { return m_pActionRequestInterface.get(); }

    virtual bool isReady();
    static void checkResultCode(const int _resultCode);

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

    virtual bool getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper);

    //------------------------------------------------ Device
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID);
    virtual bool isLocked(boost::shared_ptr<const Device> _device);
  }; // DSBusInterface

} // namespace dss

#endif
