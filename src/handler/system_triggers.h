/*
 *  Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland
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

#pragma once

#include <string>

namespace dss {

  /**
   * property node structure to declare trigger
   * ptn = path trigger node
   */

  //< trigger conditions, what needs to match, zone, group, etc
  extern const std::string ptn_triggers;
  extern const std::string  ptn_type;

  //< should every match raise a relay event?
  extern const std::string ptn_damping;
  extern const std::string  ptn_damp_interval;
  extern const std::string  ptn_damp_rewind;
  extern const std::string  ptn_damp_start_ts; //< runtime

  //< delay the execution of the action?
  extern const std::string ptn_action_lag;
  extern const std::string  ptn_action_delay;
  extern const std::string  ptn_action_reschedule;
  extern const std::string  ptn_action_ts;  //< runtime
  extern const std::string  ptn_action_eventid; //< runtime, already scheduled event
}
