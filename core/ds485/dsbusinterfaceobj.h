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

#ifndef DSBUSINTERFACEOBJ_H_
#define DSBUSINTERFACEOBJ_H_

#include <digitalSTROM/dsm-api-v2/dsm-api.h>

namespace dss {

  class DSBusInterfaceObj {
  public:
    void setDSMApiHandle(DsmApiHandle_t _value) { m_DSMApiHandle = _value; }
    void setOwnDSID(dsid_t _value) { m_ownDSID = _value; }
  protected:
    DsmApiHandle_t m_DSMApiHandle;
    dsid_t m_ownDSID;
  };

} // namespace dss

#endif
