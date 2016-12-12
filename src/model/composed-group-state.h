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

#include <logger.h>
#include "state.h"

namespace dss {

/// Group states calculated for binary inputs within given group
enum class ComposedGroupStateType {
  NONE, // No group state associated with binary input
  OR,   // Group state oring all binary inputs of given binary input type in the group
};

ComposedGroupStateType composedGroupStateTypeForBinaryInputType(BinaryInputType type);

/// Group state that combines all substates by `or` function.
///
/// We inherit State class publicly to make this class registrable into Apartment class.
/// But the non-const State api is not intented for public use and should stay private.
///
/// Methods `add`, `update` and `remove` take `const State& state`,
/// but the coupling is very weak. State object is accessed only inside the called method.
class ComposedGroupState : public State {
public:
  ComposedGroupState(const std::string& name, ComposedGroupStateType type);
  ~ComposedGroupState();

  static constexpr eStateType STATE_TYPE = StateType_Service;
  void addSubState(const State& subState);
  void updateSubState(const State& subState);
  void removeSubState(const State& subState);

private:
  __DECL_LOG_CHANNEL__;
  boost::recursive_mutex m_mutex;
  ComposedGroupStateType m_type;
  struct SubStateItem;
  std::vector<SubStateItem> m_subStateItems;

  void update();
};

} // namespace dss
