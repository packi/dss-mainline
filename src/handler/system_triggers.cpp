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

#include "src/handler/system_triggers.h"

namespace dss {
  const std::string pn_triggers = "triggers";
  const std::string  pn_type = "type";

  const std::string pn_damping = "damping";
  const std::string  pn_last_matched = "lastTrigger";
  const std::string  pn_delay = "delay";
  const std::string  pn_rewind_timer = "rewindTimer";
}
