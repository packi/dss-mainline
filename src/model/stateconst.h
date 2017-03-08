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

namespace dss {
  typedef enum {
    State_Invalid = 0,
    State_Active = 1,
    State_Inactive = 2,
    State_Unknown = 3,
  } eState;

  typedef enum {
    StateWH_Invalid = 0,
    StateWH_Closed = 1,
    StateWH_Open = 2,
    StateWH_Tilted = 3,
    StateWH_Unknown = 4,
  } eStateWindowHandle;

  typedef enum {
    StateType_Apartment = 0,
    StateType_Device = 1,
    StateType_Service = 2,
    StateType_Group = 3,
    StateType_Script = 4,
    StateType_SensorZone = 5,
    StateType_SensorDevice = 6,
    StateType_Circuit = 7,
  } eStateType;
}
