/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef _MODEL_JS_INCLUDED
#define _MODEL_JS_INCLUDED

#include <bitset>

#include <boost/ptr_container/ptr_vector.hpp>

#include "core/jshandler.h"
#include "core/event.h"
#include "core/propertysystem.h"

namespace dss {

  class Set;
  class Device;
  class Apartment;
  class DSMeter;

  /** This class extends a ScriptContext to contain the JS-API of the Apartment */
  class ModelScriptContextExtension : public ScriptExtension {
  private:
    Apartment& m_Apartment;
  public:
    /** Creates an instance that interfaces with \a _apartment */
    ModelScriptContextExtension(Apartment& _apartment);
    virtual ~ModelScriptContextExtension() {}

    virtual void extendContext(ScriptContext& _context);

    /** Returns a reference to the wrapped apartment */
    Apartment& getApartment() const { return m_Apartment; }

    /** Creates a JSObject that wraps a Set.
      * @param _ctx Context in which to create the object
      * @param _set Reference to the \a Set being wrapped
      */
    JSObject* createJSSet(ScriptContext& _ctx, Set& _set);
    /** Creates a JSObject that wraps a Device.
      * @param _ctx Context in which to create the object
      * @param _device Reference to the \a Device being wrapped
      */
    JSObject* createJSDevice(ScriptContext& _ctx, boost::shared_ptr<Device> _ref);
    /** @copydoc createJSDevice */
    JSObject* createJSDevice(ScriptContext& _ctx, DeviceReference& _ref);

    /** Creates a JSObject that wraps a DSMeter */
    JSObject* createJSMeter(ScriptContext& _ctx, boost::shared_ptr<DSMeter> _pMeter);

    template<class t>
    t convertTo(ScriptContext& _context, jsval val);

    template<class t>
    t convertTo(ScriptContext& _context, JSObject* _obj);
  }; // ModelScriptContextExtension

  class EventScriptExtension : public ScriptExtension {
  private:
    EventQueue& m_Queue;
    EventInterpreter& m_Interpreter;
  public:
    EventScriptExtension(EventQueue& _queue, EventInterpreter& _interpreter);
    virtual ~EventScriptExtension() {}

    virtual void extendContext(ScriptContext& _context);

    EventQueue& getEventQueue() { return m_Queue; }
    const EventQueue& getEventQueue() const { return m_Queue; }

    EventInterpreter& getEventInterpreter() { return m_Interpreter; }
    const EventInterpreter& getEventInterpreter() const { return m_Interpreter; }
  }; // EventScriptExtension

  class ModelConstantsScriptExtension : public ScriptExtension {
  public:
    ModelConstantsScriptExtension();
    virtual ~ModelConstantsScriptExtension() {}

    virtual void extendContext(ScriptContext& _context);
  }; // ModelConstantsScriptExtension

}

#endif
