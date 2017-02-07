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
#include "status-field.h"

#include "ds/log.h"

#include "base.h"
#include "foreach.h"
#include "status.h"

namespace dss {

__DEFINE_LOG_CHANNEL__(StatusField, lsNotice);

struct StatusField::SubStateItem {
  const void* m_ptr;
  std::string m_name;
  eState m_value;
  explicit SubStateItem(const State& subState)
      : m_ptr(&subState), m_name(subState.getName()), m_value(static_cast<eState>(subState.getState())) {}
  bool operator==(const State& subState) { return m_ptr == &subState; }
};

StatusField::StatusField(Status& status, StatusFieldType type, const std::string& name)
    : m_status(status), m_type(type),
    m_state(boost::make_shared<State>(StateType_Service, name)) {
  log(std::string("StatusField this:") + getName(), lsInfo);
  DS_REQUIRE(static_cast<std::size_t>(type) <= SENSOR_VALUE_BIT_MAX);
}

StatusField::~StatusField() = default;

void StatusField::addSubState(const State& subState) {
  auto&& value = subState.getState();
  log(std::string("addSubState this:") + getName() + " state:" + subState.getName() + " value:" + intToString(value),
      lsNotice);
  auto&& it = std::find(m_subStateItems.begin(), m_subStateItems.end(), subState);
  DS_REQUIRE(it == m_subStateItems.end());
  m_subStateItems.push_back(SubStateItem(subState));
  update();
}

void StatusField::updateSubState(const State& subState) {
  auto&& value = static_cast<eState>(subState.getState());
  auto&& it = std::find(m_subStateItems.begin(), m_subStateItems.end(), subState);
  DS_REQUIRE(it != m_subStateItems.end());
  if (it->m_value == value) {
    return;
  }
  log(std::string("updateSubState this:") + getName() + " state:" + subState.getName() + " value:" + intToString(value),
      lsNotice);
  it->m_value = value;
  update();
}

void StatusField::removeSubState(const State& subState) {
  log(std::string("removeSubState this:") + getName() + " state:" + subState.getName(), lsNotice);
  auto&& it = std::find(m_subStateItems.begin(), m_subStateItems.end(), subState);
  DS_REQUIRE(it != m_subStateItems.end());
  m_subStateItems.erase(it);
  update();
}

void StatusField::setValue(StatusFieldValue value) {
  log(ds::str("setValue this:", getName()," value:", value), lsNotice);
  setValueImpl(value);
}

namespace {
eState statusFieldValueToState(StatusFieldValue value) {
  switch(value) {
    case StatusFieldValue::INACTIVE: return State_Inactive;
    case StatusFieldValue::ACTIVE: return State_Active;
  }
  return State_Inactive;
}
} // namespace

void StatusField::setValueImpl(StatusFieldValue value) {
  log(ds::str("setValueImpl this:", getName()," value:", value), lsDebug);
  m_state->setState(coSystem, statusFieldValueToState(value));
  m_status.setBitValue(m_type, [&] {
    switch(value) {
      case StatusFieldValue::INACTIVE: return 0;
      case StatusFieldValue::ACTIVE: return 1;
    }
    return 1;
  }());
}

void StatusField::update() {
  // Requirements: The composed state is:
  // * `active` if at least one sub state is `active`
  // * `inactive` otherwise
  StatusFieldValue value = StatusFieldValue::INACTIVE;
  foreach (auto&& x, m_subStateItems) {
    log(std::string("update this:") + getName() + " state:" + x.m_name + " value:" + intToString(x.m_value), lsDebug);
    if (x.m_value == State_Active) {
      value = StatusFieldValue::ACTIVE;
    }
  }
  auto&& oldStateValue = m_state->getState();
  auto&& stateValue = statusFieldValueToState(value);
  if (oldStateValue == stateValue) {
    return;
  }
  log(ds::str("update this:", getName(), " composed value:", value), lsNotice);
  setValueImpl(value);
}

} // namespace dss
