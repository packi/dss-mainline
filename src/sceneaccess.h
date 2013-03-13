/*
    Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland

    Author: Christian Hitz, aizo AG <christian.hitz@aizo.com>

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


#ifndef SCENE_ACCESS_H_INCLUDED
#define SCENE_ACCESS_H_INCLUDED

#include <string>

namespace dss {

typedef enum {
  SAC_MANUAL,
  SAC_TIMER,
  SAC_ALGORITHM,
  SAC_UNKNOWN,
} SceneAccessCategory;

class AddressableModelItem;

class SceneAccess
{
public:
  static bool access(const AddressableModelItem *_pTarget, const SceneAccessCategory _category);
  static SceneAccessCategory stringToCategory(const std::string& _categoryString);
  static std::string categoryToString(const SceneAccessCategory _category);
};

} // namespace dss

#endif
