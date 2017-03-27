/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

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

#include "action-execute.h"

#include <boost/make_shared.hpp>

#include "event/event_create.h"
#include "messages/vdc-messages.pb.h"
#include "model/apartment.h"
#include "model/group.h"
#include "model/state.h"
#include "model/zone.h"
#include "protobufjson.h"
#include "sceneaccess.h"
#include "util.h"
#include "systemcondition.h"

namespace dss {

ActionExecute::ActionExecute(const Properties &properties) : m_properties(properties) {}

} // namespace dss
