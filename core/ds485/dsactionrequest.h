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

#ifndef DSACTIONREQUEST_H_
#define DSACTIONREQUEST_H_

#include "core/businterface.h"

#include <digitalSTROM/dsm-api-v2/dsm-api.h>

namespace dss {

  class DSActionRequest : public ActionRequestInterface {
  public:
    DSActionRequest(DsmApiHandle_t _dsmApiHandle)
    : m_DSMApiHandle(_dsmApiHandle)
    {
      SetBroadcastId(m_BroadcastDSID);
    }

    virtual void callScene(AddressableModelItem *pTarget, const uint16_t scene);
    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t scene);
    virtual void undoScene(AddressableModelItem *pTarget);
    virtual void blink(AddressableModelItem *pTarget);
    virtual void setValue(AddressableModelItem *pTarget, const double _value);
  private:
    DsmApiHandle_t m_DSMApiHandle;
    dsid_t m_BroadcastDSID;
  }; // DSActionRequest

} // namespace dss

#endif
