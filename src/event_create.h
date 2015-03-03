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

namespace dss {

boost::shared_ptr<Event>
  createDeviceStatusEvent(boost::shared_ptr<DeviceReference> pDevRev,
                          int statusIndex, int statusValue);

boost::shared_ptr<Event>
  createDeviceBinaryInputEvent(boost::shared_ptr<DeviceReference> pDevRev,
                               int inputIndex, int inputType, int inputState);

boost::shared_ptr<Event>
  createDeviceSensorValueEvent(boost::shared_ptr<DeviceReference> pDevRev,
                               int sensorIndex, int sensorType,
                               int sensorValue);

}
#endif
