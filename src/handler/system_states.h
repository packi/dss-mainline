/*
 *  Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SYSTEM_STATE_HANDLER__
#define __SYSTEM_STATE_HANDLER__

#include "event.h"
#include "handler/system_handlers.h"
#include "model/deviceinterface.h"

namespace dss {
  class SystemState : public SystemEvent {
    public:
      SystemState();
      virtual ~SystemState();
      virtual void run();
      virtual bool setup(Event& _event);
    private:
      std::string m_evtName;

      EventRaiseLocation m_evtRaiseLocation;
      boost::shared_ptr<const Group> m_raisedAtGroup;
      boost::shared_ptr<const DeviceReference> m_raisedAtDevice;
      boost::shared_ptr<const State> m_raisedAtState;

      std::string formatZoneName(const std::string &_name, int _zoneId);
      std::string formatGroupName(const std::string &_name, int _groupId);
      std::string formatGroupName2(const std::string &_name, int _groupId);

      void callScene(int _zoneId, int _groupId, int _sceneId, callOrigin_t _origin);
      void undoScene(int _zoneId, int _groupId, int _sceneId, callOrigin_t _origin);

      boost::shared_ptr<State> registerState(std::string _name,
                                             bool _persistent);
      boost::shared_ptr<State> getOrRegisterState(std::string _name);
      bool lookupState(boost::shared_ptr<State> &_state,
                       const std::string &_name);
      std::string getData(int *zoneId, int *groupId, int *sceneId,
                          callOrigin_t *callOrigin);
      Apartment &m_apartment;

      void bootstrap();
      void startup();
      void callscene();
      void undoscene();
      void stateBinaryInputGeneric(State &_state,
                                   int targetGroupType,
                                   int targetGroupId);
      void stateBinaryinput();
      void stateApartment();
  };

  class EventInterpreterPluginSystemState : public TaskProcessor,
                                            public EventInterpreterPlugin {
    public:
      EventInterpreterPluginSystemState(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterPluginSystemState();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };
}
#endif
