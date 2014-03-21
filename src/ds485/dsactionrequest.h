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
    : DSBusInterfaceObj(), m_pBusEventSink(NULL)
    {
      SetBroadcastId(m_BroadcastDSID);
    }

    virtual void callScene(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token, const bool _force);
    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene, const std::string _token);
    virtual void undoScene(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token);
    virtual void undoSceneLast(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const std::string _token);
    virtual void blink(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const std::string _token);
    virtual void setValue(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token);
    virtual void increaseOutputChannelValue(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token);
    virtual void decreaseOutputChannelValue(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token);
    virtual void stopOutputChannelValue(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token);
    void setBusEventSink(BusEventSink* _eventSink);
  private:
    dsid_t m_BroadcastDSID;
    BusEventSink* m_pBusEventSink;
  }; // DSActionRequest

} // namespace dss

#endif
