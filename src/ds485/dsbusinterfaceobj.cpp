/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dsbusinterfaceobj.h"

namespace dss {

  boost::recursive_mutex DSBusInterfaceObj::m_DSMApiHandleMutex;

  DSBusInterfaceObj::DSBusInterfaceObj() : m_DSMApiHandle(NULL) { }

  void DSBusInterfaceObj::setDSMApiHandle(DsmApiHandle_t _value) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    m_DSMApiHandle = _value;
  }
    
} // namespace dss

