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
#include "model/modelconst.h"
#include "model/deviceinterface.h"

namespace dss {

  namespace StateName {
    extern const std::string Alarm;
    extern const std::string Alarm2;
    extern const std::string Alarm3;
    extern const std::string Alarm4;
    extern const std::string BuildingService;
    extern const std::string Fire;
    extern const std::string Frost;
    extern const std::string Hibernation;
    extern const std::string Panic;
    extern const std::string Motion;
    extern const std::string OperationLock;
    extern const std::string Presence;
    extern const std::string Rain;
    extern const std::string Wind;
    extern const std::string HeatingSystem;
    extern const std::string HeatingSystemMode;
    extern const std::string HeatingModeControl;
  }

  class SystemState : public SystemEvent {
    public:
      SystemState(const Event &event);
      virtual ~SystemState();
      void run() DS_OVERRIDE;
    private:
      std::string formatZoneName(const std::string &_name, int _zoneId);
      std::string formatGroupName(const std::string &_name, int _groupId);
      std::string formatAppartmentStateName(const std::string &_name, int _groupId);

      void callScene(int _zoneId, int _groupId, int _sceneId, callOrigin_t _origin);
      void undoScene(int _zoneId, int _groupId, int _sceneId, callOrigin_t _origin);

      boost::shared_ptr<State> registerState(std::string _name,
                                             bool _persistent);
      boost::shared_ptr<State> getOrRegisterState(std::string _name);
      boost::shared_ptr<State> loadPersistentState(eStateType _type, std::string _name);

      bool lookupState(boost::shared_ptr<State> &_state,
                       const std::string &_name);
      std::string getData(int *zoneId, int *groupId, int *sceneId,
                          callOrigin_t *callOrigin);
      Apartment &m_apartment;

      void bootstrap();
      void startup();
      void callscene();
      void undoscene();
      void stateBinaryInputGeneric(State &_state, int targetGroupId);
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
