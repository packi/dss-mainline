/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Author: Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#include "jsevent.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

#include "src/dss.h"
#include "src/logger.h"
#include "src/event.h"
#include "src/scripting/scriptobject.h"
#include "src/security/security.h"

namespace dss {

  const std::string EventScriptExtensionName = "eventextension";

  JSBool event_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *rval);
  JSBool event_raise(JSContext* cx, uintN argc, jsval *vp);
  JSBool timedEvent_raise(JSContext* cx, uintN argc, jsval *vp);

  JSBool subscription_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *rval);
  JSBool subscription_subscribe(JSContext* cx, uintN argc, jsval *vp);

  struct event_wrapper {
    boost::shared_ptr<Event> event;
  };

  struct subscription_wrapper {
    boost::shared_ptr<EventSubscription> subscription;
  };

  void finalize_event(JSContext *cx, JSObject *obj) {
    event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);

    // FIXME: possible memory leak in eventWrapper->event

    delete eventWrapper;
  } // finalize_dev

  static JSClass event_class = {
    "Event", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_event, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSPropertySpec event_properties[] = {
    {"className", 0, 0, event_JSGet, NULL},
    {"name", 1, 0, event_JSGet, NULL},
    {NULL}
  };

  JSFunctionSpec event_methods[] = {
    JS_FS("raise", event_raise, 1, 0),
    JS_FS_END
  };

  static JSClass timedEvent_class = {
    "TimedEvent", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_event, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSFunctionSpec timedEvent_methods[] = {
    JS_FS("raise", timedEvent_raise, 1, 0),
    JS_FS_END
  };

  static JSClass timedICalEvent_class = {
    "TimedICalEvent", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_event, JSCLASS_NO_OPTIONAL_MEMBERS
  };


  void finalize_subscription(JSContext *cx, JSObject *obj) {
    subscription_wrapper* subscriptionWrapper = static_cast<subscription_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete subscriptionWrapper;
  } // finalize_dev

  static JSClass subscription_class = {
    "Subscription", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_subscription, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSPropertySpec subscription_properties[] = {
    {"className", 0, 0, subscription_JSGet, NULL},
    {"eventName", 1, 0, subscription_JSGet, NULL},
    {"handlerName", 2, 0, subscription_JSGet, NULL},
    {NULL}
  };

  JSFunctionSpec subscription_methods[] = {
    JS_FS("subscribe", subscription_subscribe, 0, 0),
    JS_FS_END
  };


  EventScriptExtension::EventScriptExtension(EventQueue& _queue, EventInterpreter& _interpreter)
  : ScriptExtension(EventScriptExtensionName),
    m_Queue(_queue),
    m_Interpreter(_interpreter)
  { } // ctor

  void readEventPropertiesFrom(JSContext *cx, jsval _vp, boost::shared_ptr<Event> _pEvent) {
    JSObject* paramObj;
    if(JS_ValueToObject(cx, _vp, &paramObj) == JS_TRUE) {
      JSObject* propIter = JS_NewPropertyIterator(cx, paramObj);
      jsid propID;
      while(JS_NextProperty(cx, propIter, &propID) == JS_TRUE) {
        if(JSID_IS_VOID(propID)) {
          break;
        }
        JSObject* obj;

        jsval arg1;
        JS_GetMethodById(cx, paramObj, propID, &obj, &arg1);
        JSString *val = JS_ValueToString(cx, arg1);
        char* propValue = JS_EncodeString(cx, val);

        jsval arg2;
        JS_IdToValue(cx, propID, &arg2);
        val = JS_ValueToString(cx, arg2);
        char* propName = JS_EncodeString(cx, val);

        _pEvent->setProperty(propName, propValue);

        JS_free(cx, propValue);
        JS_free(cx, propName);
      }
    }
  }

  JSBool event_construct(JSContext *cx, uintN argc, jsval *vp) {
    JSString *jsEName = JS_ValueToString(cx, argc ? JS_ARGV(cx, vp)[0] : JSVAL_VOID);
    if (!jsEName) {
      Logger::getInstance()->log("JS: event_construct: (empty name)", lsError);
      return JS_FALSE;
    }

    char *str = JS_EncodeString(cx, jsEName);
    std::string eName(str);
    JS_free(cx, str);

    try {
      boost::shared_ptr<Event> newEvent(new Event(eName));

      if(argc >= 2) {
        readEventPropertiesFrom(cx, JS_ARGV(cx, vp)[1], newEvent);
      }

      event_wrapper* evtWrapper = new event_wrapper();
      evtWrapper->event = newEvent;

      JSObject *obj = JS_NewObject(cx, &event_class, NULL, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));

      JS_SetPrivate(cx, obj, evtWrapper);
      return JS_TRUE;
    } catch(ScriptException& e) {
      Logger::getInstance()->log(std::string("JS: event_construct: error converting string: ") + e.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // event_construct

  JSBool timedEvent_construct(JSContext *cx, uintN argc, jsval *vp) {
    JSString *s1, *s2;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "SS", &s1, &s2)) {
      return JS_FALSE;
    }

    try {
      char *s = JS_EncodeString(cx, s1);
      std::string name(s);
      JS_free(cx, s);

      s = JS_EncodeString(cx, s2);
      std::string time(s);
      JS_free(cx, s);

      if(name.empty()) {
        Logger::getInstance()->log("JS: timedEvent_construct: empty name not allowed", lsError);
        return JS_FALSE;
      }
      if(time.empty()) {
        Logger::getInstance()->log("JS: timedEvent_construct: empty time not allowed", lsError);
        return JS_FALSE;
      }

      boost::shared_ptr<Event> newEvent(new Event(name));

      if(argc >= 3) {
        readEventPropertiesFrom(cx, JS_ARGV(cx, vp)[2], newEvent);
      }

      newEvent->setProperty(EventPropertyTime, time);

      event_wrapper* evtWrapper = new event_wrapper();
      evtWrapper->event = newEvent;

      JSObject *obj = JS_NewObject(cx, &timedEvent_class, NULL, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));

      JS_SetPrivate(cx, obj, evtWrapper);
      return JS_TRUE;
    } catch(ScriptException& e) {
      Logger::getInstance()->log(std::string("JS: event_construct: error converting string: ") + e.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // timedEvent_construct

  JSBool timedICalEvent_construct(JSContext *cx, uintN argc, jsval *vp) {
    JSString *s1, *s2, *s3;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "SSS", &s1, &s2, &s3)) {
      return JS_FALSE;
    }

    try {
      char *s = JS_EncodeString(cx, s1);
      std::string name(s);
      JS_free(cx, s);

      s = JS_EncodeString(cx, s2);
      std::string time(s);
      JS_free(cx, s);

      s = JS_EncodeString(cx, s3);
      std::string rrule(s);
      JS_free(cx, s);

      if(name.empty()) {
        Logger::getInstance()->log("JS: timedICalEvent_construct: empty name not allowed", lsError);
        return JS_FALSE;
      }
      if(time.empty()) {
        Logger::getInstance()->log("JS: timedICalEvent_construct: empty start-time not allowed", lsError);
        return JS_FALSE;
      }
      // TODO: validate rrule
      if(rrule.empty()) {
        Logger::getInstance()->log("JS: timedICalEvent_construct: empty rrule not allowed", lsError);
        return JS_FALSE;
      }

      boost::shared_ptr<Event> newEvent(new Event(name));

      if(argc >= 4) {
        readEventPropertiesFrom(cx, JS_ARGV(cx, vp)[3], newEvent);
      }

      newEvent->setProperty(EventPropertyICalStartTime, time);
      newEvent->setProperty(EventPropertyICalRRule, rrule);

      event_wrapper* evtWrapper = new event_wrapper();
      evtWrapper->event = newEvent;

      JSObject *obj = JS_NewObject(cx, &timedICalEvent_class, NULL, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));

      JS_SetPrivate(cx, obj, evtWrapper);
      return JS_TRUE;
    } catch(ScriptException& e) {
      Logger::getInstance()->log(std::string("JS: timedICalEvent_construct: error converting string: ") + e.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // timedICalEvent_construct

  JSBool event_raise(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
      if(self.is("Event")) {
        event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        ext->getEventQueue().pushEvent(eventWrapper->event);
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
        return JS_TRUE;
      }
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // event_raise

  JSBool timedEvent_raise(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
      if(self.is("Event")) {
        event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        std::string id = ext->getEventQueue().pushTimedEvent(eventWrapper->event);
        JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, id.c_str())));
        return JS_TRUE;
      }
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // timedEvent_raise


  JSBool event_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, obj));

    if(eventWrapper != NULL) {
      int opt = JSID_TO_INT(id);
      switch(opt) {
        case 0:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Event")));
          return JS_TRUE;
        case 1:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, eventWrapper->event->getName().c_str())));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }


  JSBool subscription_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    subscription_wrapper* subscriptionWrapper = static_cast<subscription_wrapper*>(JS_GetPrivate(cx, obj));

    if(subscriptionWrapper != NULL) {
      int opt = JSID_TO_INT(id);
      switch(opt) {
        case 0:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Subscription")));
          return JS_TRUE;
        case 1:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, subscriptionWrapper->subscription->getEventName().c_str())));
          return JS_TRUE;
        case 2:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, subscriptionWrapper->subscription->getHandlerName().c_str())));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  JSBool subscription_subscribe(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
      if(self.is("Subscription")) {
        subscription_wrapper* subscriptionWrapper = static_cast<subscription_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        ext->getEventInterpreter().subscribe(subscriptionWrapper->subscription);
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
        return JS_TRUE;
      }
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // subscription_subscribe

  JSBool subscription_construct(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
    if(ext == NULL) {
      return JS_FALSE;
    }

    JSString *s1, *s2;
    JSObject *paramObj;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "SSo", &s1, &s2, &paramObj)) {
      return JS_FALSE;
    }

    char* eventName = JS_EncodeString(cx, s1);
    char* handlerName = JS_EncodeString(cx, s2);

    boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());

    JSObject* propIter = JS_NewPropertyIterator(cx, paramObj);
    jsid propID;
    while(JS_NextProperty(cx, propIter, &propID) == JS_TRUE) {
      if(JSID_IS_VOID(propID)) {
        break;
      }
      JSObject* obj;
      jsval vp1, vp2;

      JS_GetMethodById(cx, paramObj, propID, &obj, &vp1);
      JS_IdToValue(cx, propID, &vp2);

      char* propValue = JS_EncodeString(cx, JS_ValueToString(cx, vp1));
      char* propName = JS_EncodeString(cx, JS_ValueToString(cx, vp2));

      opts->setParameter(propName, propValue);

      JS_free(cx, propValue);
      JS_free(cx, propName);
    }

    boost::shared_ptr<EventSubscription> subscription(new EventSubscription(eventName, handlerName, ext->getEventInterpreter(), opts));
    subscription_wrapper* subscriptionWrapper = new subscription_wrapper();
    subscriptionWrapper->subscription = subscription;

    JSObject *obj = JS_NewObject(cx, &subscription_class, NULL, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));

    JS_SetPrivate(cx, obj, subscriptionWrapper);
    return JS_TRUE;
  } // global_subscription

  void EventScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
              NULL, &event_class, event_construct, 0, event_properties,
              event_methods, NULL, NULL);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
              NULL, &timedEvent_class, timedEvent_construct, 0, event_properties,
              timedEvent_methods, NULL, NULL);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
              NULL, &timedICalEvent_class, timedICalEvent_construct, 0, event_properties,
              timedEvent_methods, NULL, NULL);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
              NULL, &subscription_class, subscription_construct, 0, subscription_properties,
              subscription_methods, NULL, NULL);
  } // extendContext

} // namespace
