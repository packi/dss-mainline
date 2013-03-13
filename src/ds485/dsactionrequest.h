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

#include "src/businterface.h"

#include "dsbusinterfaceobj.h"

namespace dss {

  class DSActionRequest : public DSBusInterfaceObj,
                          public ActionRequestInterface {
  public:
    DSActionRequest()
    : DSBusInterfaceObj()
    {
      SetBroadcastId(m_BroadcastDSID);
    }

    virtual void callScene(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const uint16_t scene, const bool _force);
    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene);
    virtual void undoScene(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const uint16_t scene);
    virtual void undoSceneLast(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category);
    virtual void blink(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category);
    virtual void setValue(AddressableModelItem *pTarget, const uint16_t _origin, const uint8_t _value);
  private:
    dsid_t m_BroadcastDSID;
  }; // DSActionRequest

} // namespace dss

#endif
