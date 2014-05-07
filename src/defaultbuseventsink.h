/*
    Copyright (c) 2010,2014 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>,
             Sergey Bostandzhyna <jin@dev.digitalstrom.org>

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

#ifndef __DSS_DEFAUL_EVENT_SINK_
#define __DSS_DEFAUL_EVENT_SINK_

#include "src/businterface.h"

namespace dss {

  class ModelMaintenance;
  class Apartment;

  class DefaultBusEventSink : public BusEventSink {
  public:
    DefaultBusEventSink(boost::shared_ptr<Apartment> _pApartment,
                     boost::shared_ptr<ModelMaintenance> _pModelMaintenance);
    virtual ~DefaultBusEventSink() {};

    virtual void onGroupCallScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const callOrigin_t _origin,
                                  const std::string _token,
                                  const bool _force);
    virtual void onGroupUndoScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const bool _explicit,
                                  const callOrigin_t _origin,
                                  const std::string _token);
    virtual void onGroupBlink(BusInterface* _source,
                              const dss_dsid_t& _dsMeterID,
                              const int _zoneID,
                              const int _groupID,
                              const int _originDeviceId,
                              const SceneAccessCategory _category,
                              const callOrigin_t _origin,
                              const std::string _token);
    virtual void onDeviceCallScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const callOrigin_t _origin,
                                  const std::string _token,
                                  const bool _force);
    virtual void onDeviceBlink(BusInterface* _source,
                               const dss_dsid_t& _dsMeterID,
                               const int _deviceID,
                               const int _originDeviceId,
                               const SceneAccessCategory _category,
                               const callOrigin_t _origin,
                               const std::string _token);
    virtual void onDeviceUndoScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const bool _explicit,
                                  const callOrigin_t _origin,
                                  const std::string _token);
    virtual void onMeteringEvent(BusInterface* _source,
                                 const dss_dsid_t& _dsMeterID,
                                 const unsigned int _powerW,
                                 const unsigned int _energyWs);
  private:
    boost::shared_ptr<Apartment> m_pApartment;
    boost::shared_ptr<ModelMaintenance> m_pModelMaintenance;
  };
} // namespace dss

#endif
