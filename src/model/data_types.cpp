
#include <cassert>

#include "src/model/data_types.h"

namespace dss {

static const char cds_none[] = "none";
static const char cds_n[] = "north";
static const char cds_ne[] = "north east";
static const char cds_e[] = "east";
static const char cds_se[] = "south east";
static const char cds_s[] = "south";
static const char cds_sw[] = "south west";
static const char cds_w[] = "west";
static const char cds_nw[] = "north west";

bool valid(CardinalDirection_t _direction)
{
  return (cd_none <= _direction && _direction < cd_last);
}

std::string toString(CardinalDirection_t _direction)
{
  switch (_direction) {
  case cd_north:
    return "north";
  case cd_north_east:
    return "north east";
  case cd_east:
    return "east";
  case cd_south_east:
    return "south east";
  case cd_south:
    return "south";
  case cd_south_west:
    return "south west";
  case cd_west:
    return "west";
  case cd_north_west:
    return "north west";
  default:
    return "none";
  }
}

std::string toUIString(CardinalDirection_t _direction)
{
  switch (_direction) {
  case cd_north:
    return "North";
  case cd_north_east:
    return "North-East";
  case cd_east:
    return "East";
  case cd_south_east:
    return "South-East";
  case cd_south:
    return "South";
  case cd_south_west:
    return "South-West";
  case cd_west:
    return "West";
  case cd_north_west:
    return "North-West";
  default:
    return "none";
  }
}

bool parseCardinalDirection(const std::string &_direction, CardinalDirection_t *_out)
{
  assert(_out != NULL);
  if (_direction == cds_none) {
    *_out =  cd_none;
    return true;
  } else if (_direction == cds_n) {
    *_out =  cd_north;
    return true;
  } else if (_direction == cds_ne) {
    *_out =  cd_north_east;
    return true;
  } else if (_direction == cds_e) {
    *_out =  cd_east;
    return true;
  } else if (_direction == cds_se) {
    *_out =  cd_south_east;
    return true;
  } else if (_direction == cds_s) {
    *_out =  cd_south;
    return true;
  } else if (_direction == cds_sw) {
    *_out =  cd_south_west;
    return true;
  } else if (_direction == cds_w) {
    *_out =  cd_west;
    return true;
  } else if (_direction == cds_nw) {
    *_out =  cd_north_west;
    return true;
  } else {
    return false;
  }
}

bool valid(WindProtectionClass_t _class)
{
  return (wpc_none <= _class && _class < wpc_last);
}

std::string toUIString(WindProtectionClass_t _wpClass)
{
  switch (_wpClass) {
  case wpc_awning_class_1:
    return "Class 1 - 7.8m/s";
  case wpc_awning_class_2:
    return "Class 2 - 10.6m/s";
  case wpc_awning_class_3:
    return "Class 3 - 13.6m/s";
  case wpc_blind_class_1:
    return "Class 1 - 9.0m/s";
  case wpc_blind_class_2:
    return "Class 2 - 10.7m/s";
  case wpc_blind_class_3:
    return "Class 3 - 12.8m/s";
  case wpc_blind_class_4:
    return "Class 4 - 16.7/s";
  case wpc_blind_class_5:
    return "Class 5 - 21.0m/s";
  case wpc_blind_class_6:
    return "Class 6 - 25.6m/s";
  default:
    return "none";
  }
}


bool convertWindProtectionClass(unsigned int _class, WindProtectionClass_t *_out)
{
  assert(_out);
  if (wpc_last <= _class) {
      return false;
  }
  *_out = static_cast<WindProtectionClass_t>(_class);
  return true;
}

unsigned int getOutputChannelSize(const int type) {
  return (type == 7 || type == 8) ? 1 : 0;
}

}
