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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "src/defaultbuseventsink.h"

#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"

namespace dss {

  DefaultBusEventSink::DefaultBusEventSink(boost::shared_ptr<Apartment> _pApartment,
                       boost::shared_ptr<ModelMaintenance> _pModelMaintenance) :
      m_pApartment(_pApartment), m_pModelMaintenance(_pModelMaintenance)
  {}

  void DefaultBusEventSink::onGroupCallScene(BusInterface* _source,
                                const dsuid_t& _dsMeterID,
                                const int _zoneID,
                                const int _groupID,
                                const int _originDeviceId,
                                const SceneAccessCategory _category,
                                const int _sceneID,
                                const callOrigin_t _origin,
                                const std::string _token,
                                const bool _force) {
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneGroup, _dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_groupID);
    pEvent->addParameter(_originDeviceId);
    pEvent->addParameter(_sceneID);
    pEvent->addParameter(_origin);
    pEvent->addParameter(_force);
    pEvent->setSingleStringParameter(_token);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // onGroupCallScene

  void DefaultBusEventSink::onGroupUndoScene(BusInterface* _source,
                                const dsuid_t& _dsMeterID,
                                const int _zoneID,
                                const int _groupID,
                                const int _originDeviceId,
                                const SceneAccessCategory _category,
                                const int _sceneID,
                                const bool _explicit,
                                const callOrigin_t _origin,
                                const std::string _token) {
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etUndoSceneGroup, _dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_groupID);
    pEvent->addParameter(_originDeviceId);
    if (_explicit) {
      pEvent->addParameter(_sceneID);
    } else {
      pEvent->addParameter(-1);
    }
    pEvent->addParameter(_origin);
    pEvent->setSingleStringParameter(_token);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // onGroupUndoScene

  void DefaultBusEventSink::onGroupBlink(BusInterface* _source,
                            const dsuid_t& _dsMeterID,
                            const int _zoneID,
                            const int _groupID,
                            const int _originDeviceId,
                            const SceneAccessCategory _category,
                            const callOrigin_t _origin,
                            const std::string _token) {
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etBlinkGroup, _dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_groupID);
    pEvent->addParameter(_originDeviceId);
    pEvent->addParameter(_origin);
    pEvent->setSingleStringParameter(_token);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // onGroupCallScene

  void DefaultBusEventSink::onDeviceCallScene(BusInterface* _source,
                                const dsuid_t& _dsMeterID,
                                const int _deviceID,
                                const int _originDeviceId,
                                const SceneAccessCategory _category,
                                const int _sceneID,
                                const callOrigin_t _origin,
                                const std::string _token,
                                const bool _force) {
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneDevice, _dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_originDeviceId);
    pEvent->addParameter(_sceneID);
    pEvent->addParameter(_origin);
    pEvent->addParameter(_force);
    pEvent->setSingleStringParameter(_token);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // onDeviceCallScene

  void DefaultBusEventSink::onDeviceUndoScene(BusInterface* _source,
                                const dsuid_t& _dsMeterID,
                                const int _deviceID,
                                const int _originDeviceId,
                                const SceneAccessCategory _category,
                                const int _sceneID,
                                const bool _explicit,
                                const callOrigin_t _origin,
                                const std::string _token) {
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etUndoSceneDevice, _dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_sceneID);
    pEvent->addParameter(_explicit);
    pEvent->addParameter(_originDeviceId);
    pEvent->addParameter(_origin);
    pEvent->setSingleStringParameter(_token);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // onDeviceUndoScene

  void DefaultBusEventSink::onDeviceBlink(BusInterface* _source,
                             const dsuid_t& _dsMeterID,
                             const int _deviceID,
                             const int _originDeviceId,
                             const SceneAccessCategory _category,
                             const callOrigin_t _origin,
                             const std::string _token) {
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etBlinkDevice, _dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_originDeviceId);
    pEvent->addParameter(_origin);
    pEvent->setSingleStringParameter(_token);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // onDeviceCallScene

  void DefaultBusEventSink::onMeteringEvent(BusInterface* _source,
                               const dsuid_t& _dsMeterID,
                               const unsigned int _powerW,
                               const unsigned int _energyWs) {
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etMeteringValues,
                                                _dsMeterID);
    pEvent->addParameter(_powerW);
    pEvent->addParameter(_energyWs);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DefaultBusEventSink::onZoneSensorValue(BusInterface* _source,
      const dsuid_t &_dsMeterID,
      const dsuid_t &_sourceDevice,
      const int& _zoneID,
      const int& _groupID,
      const int& _sensorType,
      const int& _sensorValue,
      const int& _precision,
      const SceneAccessCategory _category,
      const callOrigin_t _origin) {
    ModelEvent* pEvent = new ModelEventWithStrings(ModelEvent::etZoneSensorValue, _dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_groupID);
    pEvent->addParameter(_sensorType);
    pEvent->addParameter(_sensorValue);
    pEvent->addParameter(_precision);
    pEvent->setSingleStringParameter(dsuid2str(_sourceDevice));
    m_pModelMaintenance->addModelEvent(pEvent);
  } // onZoneSensorValue

} // namespace dss
