/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef PHYSICALMODELITEM_H
#define PHYSICALMODELITEM_H

#include "datetools.h"

namespace dss {

  class PhysicalModelItem {
  private:
    bool m_IsPresent;
    bool m_IsConnected;
    DateTime m_InactiveSince;
  public:
    PhysicalModelItem()
    : m_IsPresent(false), m_IsConnected(false),
      m_InactiveSince(DateTime::NullDate)
    { }

    virtual ~PhysicalModelItem() {}; // please the compiler (virtual dtor)

    bool isPresent() const { return m_IsPresent & m_IsConnected; }

    virtual void setIsPresent(const bool _value) {
      m_IsPresent = _value;
      if (m_IsPresent) {
        m_InactiveSince = DateTime::NullDate;
      } else {
        m_InactiveSince = DateTime();
      }
    }

    void setInactiveSince(DateTime _value) { m_InactiveSince = _value; }

    DateTime getInactiveSince() const { return m_InactiveSince; }
    std::string getInactiveSinceStr() const {
      return m_InactiveSince.toString();
    }

    bool isConnected() const { return m_IsConnected; }
    virtual void setIsConnected(const bool _value) { m_IsConnected = _value; }
  };

}

#endif // PHYSICALMODELITEM_H
