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

#include <bitset>
#include <vector>

#include <boost/optional.hpp>

#include "dssfwd.h"
#include "logger.h"
#include "model/modelconst.h"
#include "model/stateconst.h"

namespace dss {
class Status;

//TODO(someday): move to shared locate and rename to `StatusSensorBitset`?
typedef std::bitset<SENSOR_VALUE_BIT_MAX + 1> StatusSensorBitset;

/// Status field that is active when any substate is active (`or` function).
/// Removing the last substate does not clear the status field,
/// but the last value persistently stays.
///
/// Status fields are combined into \ref Status.
///
/// Methods `add`, `update` and `remove` take `const State& state`,
/// but the coupling is very weak. State object is accessed only inside the called method.
///
/// TODO(someday): StatusFields should be template by an enum type,
/// there may be more than 2 valid options for one StatusField.
class StatusField : private boost::noncopyable {
public:
  StatusField(Status& status, StatusFieldType type);
  ~StatusField();

  Status& getStatus() { return m_status; }
  const std::string getName() const;
  std::bitset<SENSOR_VALUE_BIT_MAX + 1> m_valueBitset;
  StatusFieldType getType() const { return m_type; }

  void addSubState(const State& subState);
  void updateSubState(const State& subState);
  void removeSubState(const State& subState);

  /// Sets status field value and push it as SensorType::Status.
  /// The value is pushed immediately regardless on whether the value change or not.
  /// The value is lost on first event from any substate
  void setValueAndPush(StatusFieldValue value);
  StatusFieldValue getValue() const;

  /// Get value as bits \ref SensorType::Status value
  StatusSensorBitset getValueAsBitset() const;

private:
  __DECL_LOG_CHANNEL__;
  Status& m_status;
  StatusFieldType m_type;
  boost::shared_ptr<State> m_state; // State uses shared_from_this
  struct SubStateItem;
  std::vector<SubStateItem> m_subStateItems;

  void setValueAndPushImpl(StatusFieldValue value);
  void update();
};

} // namespace dss
