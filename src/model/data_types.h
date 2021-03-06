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
#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>

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

    cd_last = 9, //< keep last
  } CardinalDirection_t;

  bool valid(CardinalDirection_t _direction);
  std::string toString(CardinalDirection_t _direction);
  std::string toUIString(CardinalDirection_t _direction);
  bool parseCardinalDirection(const std::string &direction, CardinalDirection_t *_out);

  typedef enum {
    wpc_none = 0,
    wpc_awning_class_1 = 1,  //< awning 7.8m/s,
    wpc_awning_class_2 = 2,  //< awning 10.6m/s,
    wpc_awning_class_3 = 3,  //< awning 13.6m/s,
    wpc_blind_class_1 = 4,  //< Blind 9.0m/s,
    wpc_blind_class_2 = 5,  //< Blind 10.7m/s,
    wpc_blind_class_3 = 6,  //< Blind 12.8m/s,
    wpc_blind_class_4 = 7,  //< Blind 16.7m/s,
    wpc_blind_class_5 = 8,  //< Blind 21.0m/s,
    wpc_blind_class_6 = 9,  //< Blind 25.6m/s,

    wpc_last,  //< keep last
  } WindProtectionClass_t;

  bool valid(WindProtectionClass_t _class);
  std::string toUIString(WindProtectionClass_t _wpClass);
  bool convertWindProtectionClass(unsigned int _class, WindProtectionClass_t *_out);

  typedef enum {
    ge_none = 0,
    ge_sun = 1,
    ge_frost = 2,
    ge_heating_mode = 3,
    ge_service = 4,
  } GenericEventType_t;

  typedef struct {
    int length;
    unsigned char payload[PAYLOAD_LEN];
  } GenericEventPayload_t;

  unsigned int getOutputChannelSize(const int type);
}

#endif
