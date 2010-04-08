/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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
    m_UsageCount = 0;
  } // ctor

  bool Session::isStillValid() {
    //const int TheSessionTimeout = 5;
    return true;//m_LastTouched.addMinute(TheSessionTimeout).after(DateTime());
  } // isStillValid

  Session& Session::operator=(const Session& _other) {
    m_Token = _other.m_Token;
    m_LastTouched = _other.m_LastTouched;

    return *this;
  }

  bool Session::isUsed() {
    return (m_UsageCount != 0);
  }

  void Session::use() {
    m_UsageCount++;
  }

  void Session::unuse() {
    m_UsageCount--;
    if(m_UsageCount < 0) {
      m_UsageCount = 0;
    }
  }
} // namespace dss
