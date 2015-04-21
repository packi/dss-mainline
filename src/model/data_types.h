/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MODEL_DATA_TYPES__
#define __MODEL_DATA_TYPES__

#include <string>

namespace dss {

  typedef enum {
    cd_none = 0,
    cd_north = 1,
    cd_north_east = 2,
    cd_east = 3,
    cd_south_east = 4,
    cd_south = 5,
    cd_south_west = 6,
    cd_west = 7,
    cd_north_west = 8,
    cd_last = 9,
  } CardinalDirection_t;

  bool valid(CardinalDirection_t _direction);
  std::string toString(CardinalDirection_t _direction);
  bool parseCardinalDirection(const std::string &direction, CardinalDirection_t *_out);

  typedef enum {
    wpc_none = 0,
    wpc_class_1 = 1,  //<  7.8m/s,
    wpc_class_2 = 2,  //< 10.6m/s,
    wpc_class_3 = 3,  //< 13.6m/s,
  } WindProtectionClass_t;

  bool valid(WindProtectionClass_t _class);
  bool convertWindProtectionClass(unsigned int _class, WindProtectionClass_t *_out);
}

#endif
