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

    virtual void ExtendContext(ScriptContext& _context);

    /** Returns a reference to the wrapped apartment */
    Apartment& GetApartment() const { return m_Apartment; }

    /** Creates a JSObject that wrapps a Set.
      * @param _ctx Context in which to create the object
      * @param _set Reference to the \a Set being wrapped
      */
    JSObject* CreateJSSet(ScriptContext& _ctx, Set& _set);
    /** Creates a JSObject that wrapps a Set.
      * @param _ctx Context in which to create the object
      * @param _set Reference to the \a Set being wrapped
      */
    JSObject* CreateJSDevice(ScriptContext& _ctx, Device& _ref);
    JSObject* CreateJSDevice(ScriptContext& _ctx, DeviceReference& _ref);

    template<class t>
    t ConvertTo(ScriptContext& _context, jsval val);

    template<class t>
    t ConvertTo(ScriptContext& _context, JSObject* _obj);
  }; // ModelScriptContextExtension

  class EventScriptExtension : public ScriptExtension {
  private:
    EventQueue& m_Queue;
  public:
    EventScriptExtension(EventQueue& _queue);
    virtual ~EventScriptExtension() {}

    virtual void ExtendContext(ScriptContext& _context);

    EventQueue& GetEventQueue() { return m_Queue; }
    const EventQueue& GetEventQueue() const { return m_Queue; }

    JSObject* CreateJSEvent(ScriptContext& _ctx, boost::shared_ptr<Event> _event);
    JSObject* CreateJSSubscription(ScriptContext& _ctx, boost::shared_ptr<EventSubscription> _subscription);
  };

}

#endif
