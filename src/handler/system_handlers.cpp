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

SystemEvent::SystemEvent() = default;
SystemEvent::~SystemEvent() = default;

bool SystemEvent::setup(Event& _event) {
  m_evtName = _event.getName();
  m_evtRaiseLocation = _event.getRaiseLocation();
  m_raisedAtGroup = _event.getRaisedAtGroup(DSS::getInstance()->getApartment());
  m_raisedAtDevice = _event.getRaisedAtDevice();
  m_raisedAtState = _event.getRaisedAtState();
  m_properties = _event.getProperties();
  return true;
}

} // namespace dss
