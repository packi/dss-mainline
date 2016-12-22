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

#include <boost/optional.hpp>
#include <logger.h>
#include "state.h"

namespace dss {
class Status;

/// Composed status bit that is active when any substate is active (`or` function).
///
/// Methods `add`, `update` and `remove` take `const State& state`,
/// but the coupling is very weak. State object is accessed only inside the called method.
class StatusBit : private boost::noncopyable {
public:
  StatusBit(Status& status, StatusBitType type, const std::string& name);
  ~StatusBit();

  Status& getStatus() { return m_status; }
  const std::string getName() const { return m_state->getName(); }
  StatusBitType getType() const { return m_type; }

  void addSubState(const State& subState);
  void updateSubState(const State& subState);
  void removeSubState(const State& subState);

private:
  __DECL_LOG_CHANNEL__;
  Status& m_status;
  StatusBitType m_type;
  boost::shared_ptr<State> m_state; // State uses shared_from_this
  struct SubStateItem;
  std::vector<SubStateItem> m_subStateItems;

  void update();
};

} // namespace dss
