/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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

#include "core/businterface.h"
#include "core/subsystem.h"

#include <digitalSTROM/dsm-api-v2/dsm-api.h>

#include <boost/shared_ptr.hpp>

namespace dss {

  class ModelEvent;
  class ModelMaintenance;
  class DSActionRequest;
  class DSDeviceBusInterface;
  class DSMeteringBusInterface;
  class DSStructureQueryBusInterface;
  class DSStructureModifyingBusInterface;

  class DSBusInterface : public Subsystem,
                         public BusInterface {
  private:
    boost::shared_ptr<DSActionRequest> m_pActionRequestInterface;
    boost::shared_ptr<DSDeviceBusInterface> m_pDeviceBusInterface;
    boost::shared_ptr<DSMeteringBusInterface> m_pMeteringBusInterface;
    boost::shared_ptr<DSStructureQueryBusInterface> m_pStructureQueryBusInterface;
    boost::shared_ptr<DSStructureModifyingBusInterface> m_pStructureModifyingBusInterface;

    ModelMaintenance* m_pModelMaintenance;

    DsmApiHandle_t m_dsmApiHandle;
    bool m_dsmApiReady;
    std::string m_connectionURI;
    BusEventSink* m_pBusEventSink;

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
    DSBusInterface(DSS* _pDSS, ModelMaintenance* _pModelMaintenance);
    virtual ~DSBusInterface() {};

    virtual void shutdown();

    virtual DeviceBusInterface* getDeviceBusInterface();
    virtual StructureQueryBusInterface* getStructureQueryBusInterface();
    virtual MeteringBusInterface* getMeteringBusInterface();
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface();
    virtual ActionRequestInterface* getActionRequestInterface();

    virtual void setBusEventSink(BusEventSink* _eventSink) {
      m_pBusEventSink = _eventSink;
    }
    static void checkResultCode(const int _resultCode);
    static void checkBroadcastResultCode(const int _resultCode);

    virtual void initialize();
  }; // DSBusInterface

} // namespace dss

#endif
