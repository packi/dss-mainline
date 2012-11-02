/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

    Based on system_conditions.js, authors:
            Roman Köhler <roman.koehler@aizo.com>
            Michael Troß <michael.tross@aizo.com>

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

#ifndef __SYSTEM_CONDITIONS_H__
#define __SYSTEM_CONDITIONS_H__

#include <string>
#include "dss.h"
#include "propertysystem.h"

namespace dss {

  bool checkTimeCondition(PropertyNodePtr timeStartNode, PropertyNodePtr timeEndNode, int secNow);
  bool checkSystemCondition(std::string _path);

};

#endif//__SYSTEM_CONDITIONS_H__
