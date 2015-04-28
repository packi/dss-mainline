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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "system_handlers.h"

namespace dss {

SystemEvent::SystemEvent() : Task() {
}
SystemEvent::~SystemEvent() {
}

std::string SystemEvent::getActionName(PropertyNodePtr _actionNode) {
  std::string action_name;
  PropertyNode *parent1 = _actionNode->getParentNode();
  if (parent1 != NULL) {
    PropertyNode *parent2 = parent1->getParentNode();
    if (parent2 != NULL) {
      PropertyNodePtr name = parent2->getPropertyByName("name");
      if (name != NULL) {
        action_name = name->getAsString();
      }
    }
  }
  return action_name;
}

bool SystemEvent::setup(Event& _event) {
  m_properties = _event.getProperties();
  return true;
}

} // namespace dss
