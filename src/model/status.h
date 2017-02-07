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

namespace dss {

class Group;
class StatusField;

/// Status with each value bit managed by StatusField.
/// Status value is propagated as sensor value to parent group
/// periodically and on value change.
class Status : boost::noncopyable {
public:
  Status(Group& group);
  ~Status();

  Group& getGroup() { return m_group; }
  const Group& getGroup() const { return m_group; }

  StatusField* tryGetBit(StatusFieldType statusType);
  void insertBit(StatusFieldType statusType, std::unique_ptr<StatusField> state);
  unsigned int getValue() const { return m_valueBitset.to_ulong(); }

  static boost::chrono::seconds PUSH_SENSOR_PERIOD;

private:
  __DECL_LOG_CHANNEL__;
  Group& m_group;
  ds::asio::Timer m_periodicPushTimer;
  std::bitset<SENSOR_VALUE_BIT_MAX + 1> m_valueBitset;
  boost::container::map<StatusFieldType, std::unique_ptr<StatusField>> m_bits;

  friend class StatusField;
  void setBitValue(StatusFieldType type, bool value);
  void push();
  void asyncPeriodicPush();
};

std::ostream& operator<<(std::ostream &, const Status &);

} // namespace dss
