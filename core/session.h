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

#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "datetools.h"
#include "model.h"

#include <map>

namespace dss {

  typedef std::map<const int, Set> SetsByID;

  class Session {
  private:
    int m_Token;

    int m_LastSetNr;
    DateTime m_LastTouched;
    SetsByID m_SetsByID;
  public:
    Session() {}
    Session(const int _tokenID);

    bool isStillValid();
    bool hasSetWithID(const int _id);
    Set& getSetByID(const int _id);
    Set& allocateSet(int& _id);
    void freeSet(const int id);
    Set& addSet(Set _set, int& id);

    Session& operator=(const Session& _other);
  }; // Session


}

#endif // defined SESSION_H_INCLUDED
