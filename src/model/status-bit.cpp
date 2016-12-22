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
#include "status-bit.h"

#include "ds/log.h"

#include "base.h"
#include "foreach.h"
#include "status.h"

namespace dss {

__DEFINE_LOG_CHANNEL__(StatusBit, lsNotice);

struct StatusBit::SubStateItem {
  const void* m_ptr;
  std::string m_name;
  eState m_value;
  explicit SubStateItem(const State& subState)
      : m_ptr(&subState), m_name(subState.getName()), m_value(static_cast<eState>(subState.getState())) {}
  bool operator==(const State& subState) { return m_ptr == &subState; }
};

StatusBit::StatusBit(Status& status, StatusBitType type, const std::string& name)
    : m_status(status), m_type(type),
    m_state(boost::make_shared<State>(StateType_Service, name)) {
  log(std::string("StatusBit this:") + getName(), lsInfo);
}

StatusBit::~StatusBit() = default;

void StatusBit::addSubState(const State& subState) {
  auto&& value = subState.getState();
  log(std::string("addSubState this:") + getName() + " state:" + subState.getName() + " value:" + intToString(value),
      lsNotice);
  auto&& it = std::find(m_subStateItems.begin(), m_subStateItems.end(), subState);
  DS_ASSUME(it == m_subStateItems.end());
  m_subStateItems.push_back(SubStateItem(subState));
  update();
}

void StatusBit::updateSubState(const State& subState) {
  auto&& value = static_cast<eState>(subState.getState());
  auto&& it = std::find(m_subStateItems.begin(), m_subStateItems.end(), subState);
  DS_ASSUME(it != m_subStateItems.end());
  if (it->m_value == value) {
    return;
  }
  log(std::string("updateSubState this:") + getName() + " state:" + subState.getName() + " value:" + intToString(value),
      lsNotice);
  it->m_value = value;
  update();
}

void StatusBit::removeSubState(const State& subState) {
  log(std::string("removeSubState this:") + getName() + " state:" + subState.getName(), lsNotice);
  auto&& it = std::find(m_subStateItems.begin(), m_subStateItems.end(), subState);
  DS_ASSUME(it != m_subStateItems.end());
  m_subStateItems.erase(it);
  update();
}

void StatusBit::update() {
  // Requirements: The composed state is:
  // * `active` if at least one sub state is `active`
  // * `inactive` otherwise
  int groupValue = State_Inactive;
  foreach (auto&& x, m_subStateItems) {
    log(std::string("update this:") + getName() + " state:" + x.m_name + " value:" + intToString(x.m_value), lsDebug);
    if (x.m_value == State_Active) {
      groupValue = State_Active;
    }
  }
  auto oldGroupValue = m_state->getState();
  if (oldGroupValue == groupValue) {
    return;
  }
  log(std::string("update this:") + getName() + " groupValue:" + intToString(groupValue), lsNotice);
  m_state->setState(coSystem, groupValue);
  m_status.setBitValue(m_type, groupValue == State_Active);
}

} // namespace dss
