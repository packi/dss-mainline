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

#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "datetools.h"
#include "mutex.h"

#include <string>
#include <map>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>

namespace dss {
  class Session {
  public:
    Session(const int _tokenID = -1);

    void setTimeout(const int _timeout);
    int getID();
    virtual bool isStillValid();
    bool isUsed();
    void use();
    void unuse();
    void touch();
    void addData(const std::string& _key, boost::shared_ptr<boost::any> _value);
    boost::shared_ptr<boost::any> getData(const std::string& _key);
    bool removeData(const std::string& _key);
    Session& operator=(const Session& _other);

  protected:
    int m_Token;
    Mutex m_UseCountMutex;
    int m_UsageCount;

    DateTime m_LastTouched;

    Mutex m_DataMapMutex;
    std::map<std::string, boost::shared_ptr<boost::any> > dataMap;

    int m_SessionTimeoutSec;
  }; // Session


}

#endif // defined SESSION_H_INCLUDED
