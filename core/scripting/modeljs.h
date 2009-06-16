/*
 *  modeljs.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/22/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _MODEL_JS_INCLUDED
#define _MODEL_JS_INCLUDED

#include <bitset>

#include "../jshandler.h"
#include "../model.h"
#include "../event.h"

namespace dss {

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

    /** Creates a JSObject that wrapps a Set.
      * @param _ctx Context in which to create the object
      * @param _set Reference to the \a Set being wrapped
      */
    JSObject* createJSSet(ScriptContext& _ctx, Set& _set);
    /** Creates a JSObject that wrapps a Set.
      * @param _ctx Context in which to create the object
      * @param _set Reference to the \a Set being wrapped
      */
    JSObject* createJSDevice(ScriptContext& _ctx, Device& _ref);
    JSObject* createJSDevice(ScriptContext& _ctx, DeviceReference& _ref);

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

    JSObject* createJSEvent(ScriptContext& _ctx, boost::shared_ptr<Event> _event);
    JSObject* createJSSubscription(ScriptContext& _ctx, boost::shared_ptr<EventSubscription> _subscription);
  };

}

#endif
