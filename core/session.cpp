/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "session.h"

namespace dss {

  Session::Session(const int _tokenID)
  : m_Token(_tokenID)
  {
    m_LastTouched = DateTime();
  } // ctor

  bool Session::isStillValid() {
    //const int TheSessionTimeout = 5;
    return true;//m_LastTouched.addMinute(TheSessionTimeout).after(DateTime());
  } // isStillValid

  bool Session::hasSetWithID(const int _id) {
    SetsByID::iterator iKey = m_SetsByID.find(_id);
    return iKey != m_SetsByID.end();
  } // hasSetWithID

  Set& Session::getSetByID(const int _id) {
    SetsByID::iterator iEntry = m_SetsByID.find(_id);
    return iEntry->second;
  } // getSetByID

  Set& Session::allocateSet(int& id) {
    id = ++m_LastSetNr;
    m_SetsByID[id] = Set();
    return m_SetsByID[id];
  } // allocateSet

  Set& Session::addSet(Set _set, int& id) {
    id = ++m_LastSetNr;
    m_SetsByID[id] = _set;
    return m_SetsByID[id];
  } // addSet

  void Session::freeSet(const int _id) {
    m_SetsByID.erase(m_SetsByID.find(_id));
  } // freeSet

  Session& Session::operator=(const Session& _other) {
    m_LastSetNr = _other.m_LastSetNr;
    m_Token = _other.m_Token;
    m_LastTouched = _other.m_LastTouched;

    for(SetsByID::const_iterator iEntry = m_SetsByID.begin(), e = m_SetsByID.end();
        iEntry != e; ++iEntry)
    {
      m_SetsByID[iEntry->first] = iEntry->second;
    }
    return *this;
  }

} // namespace dss
