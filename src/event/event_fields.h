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

#ifndef __EVENT_FIELDS___
#define __EVENT_FIELDS___

#include <string>

namespace dss {
  extern const std::string ef_callOrigin;
  extern const std::string ef_originDSUID;
  extern const std::string ef_zone;
  extern const std::string ef_group;
  extern const std::string ef_scene;
  extern const std::string ef_sceneID;
  extern const std::string ef_dsuid;
  extern const std::string ef_forced;
  extern const std::string ef_sensorEvent;
  extern const std::string ef_sensorIndex;
  extern const std::string ef_eventid;
  extern const std::string ef_evt;
  extern const std::string ef_triggers;
  extern const std::string ef_type;
}

#endif
