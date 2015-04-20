/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EVENT_CREATE_H_
#define _EVENT_CREATE_H_

#include "event.h"

#include <boost/shared_ptr.hpp>
#include <digitalSTROM/dsuid.h>

#include "model/deviceinterface.h"
#include "model/zone.h"
#include "businterface.h"

namespace dss {

boost::shared_ptr<Event>
  createDeviceStatusEvent(boost::shared_ptr<DeviceReference> _devRef,
                          int _index, int _value);

boost::shared_ptr<Event>
  createDeviceBinaryInputEvent(boost::shared_ptr<DeviceReference> _devRef,
                               int _index, int _type, int _state);

boost::shared_ptr<Event>
  createDeviceSensorValueEvent(boost::shared_ptr<DeviceReference> _devRef,
                               int _index, int _type, int _value);

boost::shared_ptr<Event>
  createDeviceInvalidSensorEvent(boost::shared_ptr<DeviceReference> _devRef,
                                 int _index, int _type, const DateTime& _ts);

boost::shared_ptr<Event>
  createZoneSensorValueEvent(boost::shared_ptr<Group> _group, int _type,
                             int _value, const dsuid_t &_sourceDevice);

boost::shared_ptr<Event>
  createZoneSensorErrorEvent(boost::shared_ptr<Group> _group, int _type,
                             const DateTime& _ts);

boost::shared_ptr<Event>
  createGroupCallSceneEvent(boost::shared_ptr<Group> _group, int _sceneID,
                            int _groupID, int _zoneID,
                            const callOrigin_t& _callOrigin,
                            const dsuid_t& _originDSUID,
                            const std::string& _originToken,
                            bool _forced);

boost::shared_ptr<Event>
  createGroupUndoSceneEvent(boost::shared_ptr<Group> _group, int _sceneID,
                            int _groupID, int _zoneID,
                            const callOrigin_t& _callOrigin,
                            const dsuid_t& _originDSUID,
                            const std::string& _originToken);

boost::shared_ptr<Event>
  createStateChangeEvent(boost::shared_ptr<State> _state, int _oldstate,
                         callOrigin_t _callOrigin);

boost::shared_ptr<Event>
  createActionDenied(const std::string &_type, const std::string &_name,
                     const std::string &_source, const std::string &_reason);

boost::shared_ptr<Event>
  createHeatingEnabled(int _zoneID, bool _enabled);

boost::shared_ptr<Event>
  createHeatingControllerConfig(int _zoneID, const dsuid_t &_ctrlDsuid,
                              const ZoneHeatingConfigSpec_t &_config);

boost::shared_ptr<Event>
  createHeatingControllerValue(int _zoneID, const dsuid_t &_ctrlDsuid,
                               const ZoneHeatingProperties_t &_properties,
                               const ZoneHeatingOperationModeSpec_t &_mode);

boost::shared_ptr<Event>
  createHeatingControllerValueDsHub(int _zoneID, int _operationMode,
                                    const ZoneHeatingProperties_t &_props,
                                    const ZoneHeatingStatus_t &_stat);

boost::shared_ptr<Event>
  createHeatingControllerState(int _zoneID, const dsuid_t &_ctrlDsuid, int _ctrlState);
}
#endif
