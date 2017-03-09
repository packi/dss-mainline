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

#pragma once

#include <string>

#include "system_handlers.h"

namespace dss {

  /**
   * property node structure to declare trigger
   * ptn = path trigger node
   */

  //< trigger conditions, what needs to match, zone, group, etc
  extern const std::string ptn_triggers;
  extern const std::string  ptn_type;

  //< should every match raise a relay event?
  extern const std::string ptn_damping;
  extern const std::string  ptn_damp_interval;
  extern const std::string  ptn_damp_rewind;
  extern const std::string  ptn_damp_start_ts; //< runtime

  //< delay the execution of the action?
  extern const std::string ptn_action_lag;
  extern const std::string  ptn_action_delay;
  extern const std::string  ptn_action_reschedule;
  extern const std::string  ptn_action_ts;  //< runtime
  extern const std::string  ptn_action_eventid; //< runtime, already scheduled event

  class SystemTrigger : public SystemEvent {
    public:
      SystemTrigger();
      virtual ~SystemTrigger();
      virtual void run();
      virtual bool setup(Event& _event);

    protected:
      std::string m_evtName;
      bool m_evtSrcIsGroup;
      bool m_evtSrcIsDevice;
      int m_evtSrcZone;
      int m_evtSrcGroup;
      dsuid_t m_evtSrcDSID;

      /**
       * checkTrigger() - visit the subtree, each child is a trigger
       * @_triggerProp - node with known structure containing parameters
       * @return true if match, false otherwise
       */
      bool checkTrigger(PropertyNodePtr _triggerProp);

      /**
       * checkTriggerNode() - inspect single trigger node
       * @_triggerProp - node with known structure
       * @return true if match, false otherwise
       */
      bool checkTriggerNode(PropertyNodePtr _triggerProp);

      /* subclasses of trigger type */
      bool checkSceneZone(PropertyNodePtr _triggerProp);
      bool checkUndoSceneZone(PropertyNodePtr _triggerProp);
      bool checkDeviceScene(PropertyNodePtr _triggerProp);
      bool checkDeviceSensor(PropertyNodePtr _triggerProp);
      bool checkDeviceBinaryInput(PropertyNodePtr _triggerProp);
      bool checkDevice(PropertyNodePtr _tiggerProp);
      bool checkDirectDeviceAction(PropertyNodePtr _triggerProp);
      bool checkDeviceNamedAction(PropertyNodePtr _triggerProp);
      bool checkDeviceNamedEvent(PropertyNodePtr _triggerProp);
      bool checkHighlevel(PropertyNodePtr _triggerProp);
      bool checkState(PropertyNodePtr _triggerProp);
      bool checkSensorValue(PropertyNodePtr _triggerProp);
      bool checkEvent(PropertyNodePtr _triggerProp);

      /**
       * damping() - aggregate triggers do not raise an action each time
       * @_dampProp property subtree with timeout/last_execution arguments
       * @return true if event shall be damped, false if no damping is applied
       */
      bool damping(PropertyNodePtr _dampProp);

      /**
       * rescheduleAction() - reschedule event if trigger fired again (optional)
       * @_triggerNode - node in global lookup table (/usr/triggers)
       * @_triggerParamNode - node with complete trigger parameters
       * @return true if event was rescheduled
       */
      bool rescheduleAction(PropertyNodePtr _triggerNode, PropertyNodePtr _triggerParamNode);

      /**
       * relayTrigger() - If a trigger matches this will raise an event
       * @_triggerNode - node in global lookup table (/usr/triggers)
       * @_triggerParamNode - node with complete trigger parameters
       */
      void relayTrigger(PropertyNodePtr _triggerNode, PropertyNodePtr _triggerParamNode);
  };

}
