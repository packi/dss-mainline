/*
    Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland

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
#pragma once

#include <boost/container/map.hpp>
#include <bitset>
#include <ds/asio/timer.h>
#include "logger.h"
#include "modelconst.h"
#include "status-field.h"

namespace dss {

class Group;

/// Status with each value bit managed by StatusField.
/// Status value is propagated as sensor value to parent group
/// periodically and on value change.
class Status : boost::noncopyable {
public:
  Status(Group& group);
  ~Status();

  Group& getGroup() { return m_group; }
  const Group& getGroup() const { return m_group; }

  StatusField& getMalfunctionField() { return m_malfunctionField; }
  StatusField& getServiceField() { return m_serviceField; }
  StatusField& getField(StatusFieldType type);

  /// Get all fields composed together into status sensor bitset
  StatusSensorBitset getValueAsBitset() const;

  /// Push current state as \ref SensorType::Status value
  void push();

  static boost::chrono::seconds PUSH_PERIOD;

private:
  __DECL_LOG_CHANNEL__;
  Group& m_group;
  ds::asio::Timer m_periodicPushTimer;
  std::bitset<SENSOR_VALUE_BIT_MAX + 1> m_valueBitset;
  StatusField m_malfunctionField;
  StatusField m_serviceField;

  friend class StatusField;
  void asyncPushLoopRestart(ds::asio::Timer::Duration delay);
};

std::ostream& operator<<(std::ostream &, const Status &);

} // namespace dss
