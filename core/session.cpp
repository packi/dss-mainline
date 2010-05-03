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
    m_UseCountMutex.lock();
    bool result = (m_UsageCount != 0);
    m_UseCountMutex.unlock();
    return result;
  }

  void Session::use() {
    m_UseCountMutex.lock();
    m_UsageCount++;
    m_UseCountMutex.unlock();
  }

  void Session::unuse() {
    m_UseCountMutex.lock();
    m_UsageCount--;
    if(m_UsageCount < 0) {
      m_UsageCount = 0;
    }
    m_UseCountMutex.unlock();
  }
  void Session::addData(const std::string& _key, boost::shared_ptr<boost::any>& _value) {
    m_DataMapMutex.lock();
    dataMap[_key] = _value;
    m_DataMapMutex.unlock();
  }

  boost::shared_ptr<boost::any>& Session::getData(const std::string& _key) {
    m_DataMapMutex.lock();
    boost::shared_ptr<boost::any>& rv = dataMap[_key];
    m_DataMapMutex.unlock();
    return rv;
  }

  bool Session::removeData(const std::string& _key) {
    bool result = false;
    m_DataMapMutex.lock();
    boost::ptr_map<std::string, boost::shared_ptr<boost::any> >::iterator i = dataMap.find(_key);
    if(i != dataMap.end()) {
      dataMap.erase(i);
      result = true;
    }

    m_DataMapMutex.unlock();
    return result;
  }

  int Session::getID() {
    return m_Token;
  }

  void Session::touch() {
    m_LastTouched = DateTime();
  }

} // namespace dss
