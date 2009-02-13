/*
 *  modeljs.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/22/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "modeljs.h"
#include "core/dss.h"
#include "core/logger.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

namespace dss {
  const string ModelScriptcontextExtensionName = "modelextension";

  ModelScriptContextExtension::ModelScriptContextExtension(Apartment& _apartment)
  : ScriptExtension(ModelScriptcontextExtensionName),
    m_Apartment(_apartment)
  {
  } // ctor

  template<>
  Set& ModelScriptContextExtension::ConvertTo(ScriptContext& _context, JSObject* _obj) {
    ScriptObject obj(_obj, _context);
    if(obj.Is("set")) {
      return *static_cast<Set*>(JS_GetPrivate(_context.GetJSContext(), _obj));
    }
    throw ScriptException("Wrong classname for set");
  } // ConvertTo<Set>

  template<>
  Set& ModelScriptContextExtension::ConvertTo(ScriptContext& _context, jsval _val) {
    if(JSVAL_IS_OBJECT(_val)) {
      return ConvertTo<Set&>(_context, JSVAL_TO_OBJECT(_val));
    }
    throw ScriptException("JSVal is no object");
  } // ConvertTo<Set>

  JSBool global_get_name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      string aptName = ext->GetApartment().GetName();
      JSString* str = JS_NewStringCopyN(cx, aptName.c_str(), aptName.size());

      *rval = STRING_TO_JSVAL(str);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_get_name

  JSBool global_set_name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && argc >= 1) {
      JSString* str = JS_ValueToString(cx, argv[0]);
      if(str != NULL) {
        string aptName = JS_GetStringBytes(str);
        ext->GetApartment().SetName(aptName);

        *rval = INT_TO_JSVAL(0);
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // global_set_name

  JSBool global_get_devices(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      Set devices = ext->GetApartment().GetDevices();
      JSObject* obj = ext->CreateJSSet(*ctx, devices);

      *rval = OBJECT_TO_JSVAL(obj);

      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_get_devices

  JSBool global_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    if (argc < 1) {
      *rval = INT_TO_JSVAL(0); /* Send back a return value of 0. */
      Logger::GetInstance()->Log("JS: global_log: (empty string)");
    } else {
      stringstream sstr;
      unsigned int i;
      for (i=0; i<argc; i++){
        JSString *val = JS_ValueToString(cx, argv[i]); /* Convert the value to a javascript string. */
        char *str = JS_GetStringBytes(val); /* Then convert it to a C-style string. */
        sstr << str;
      }
      Logger::GetInstance()->Log(sstr.str());
      *rval = INT_TO_JSVAL(0); /* Set the return value to be the number of bytes/chars written */
    }
    return JS_TRUE;
  } // global_log

  JSFunctionSpec model_global_methods[] = {
    {"getName", global_get_name, 0, 0, 0},
    {"setName", global_set_name, 1, 0, 0},
    {"getDevices", global_get_devices, 0, 0, 0},
    {"log", global_log, 1, 0, 0},
    {NULL},
  };

  void ModelScriptContextExtension::ExtendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.GetJSContext(), JS_GetGlobalObject(_context.GetJSContext()), model_global_methods);
  } // ExtendedJSContext

  void finalize_set(JSContext *cx, JSObject *obj) {
    Set* pSet = static_cast<Set*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pSet;
  } // finalize_set

  JSBool set_length(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL) {
      *rval = INT_TO_JSVAL(set->Length());
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool set_combine(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      Set& otherSet = ext->ConvertTo<Set&>(*ctx, argv[0]);
      Set result = set->Combine(otherSet);
      JSObject* resultObj = ext->CreateJSSet(*ctx, result);
       *rval = OBJECT_TO_JSVAL(resultObj);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool set_remove(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      Set& otherSet = ext->ConvertTo<Set&>(*ctx, argv[0]);
      Set result = set->Remove(otherSet);
      JSObject* resultObj = ext->CreateJSSet(*ctx, result);
      *rval = OBJECT_TO_JSVAL(resultObj);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool set_JSGet(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    if(set != NULL) {
      int opt = JSVAL_TO_INT(id);
      if(opt == 0) {
        *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "set"));
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  static JSClass set_class = {
    "set", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_set, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSFunctionSpec set_methods[] = {
    {"length", set_length, 0, 0, 0},
    {"combine", set_combine, 1, 0, 0},
    {"remove", set_remove, 1, 0, 0},
    {NULL},
  };

  static JSPropertySpec set_properties[] = {
    {"className", 0, 0, set_JSGet},
    {NULL}
  };

  JSBool dev_turn_on(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->TurnOn();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_turn_off(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->TurnOff();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_enable(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->Enable();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_disable(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->TurnOn();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_increase_value(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 0) {
        intf->IncreaseValue();
      } else if(argc >= 1) {
        int parameterNr = ctx->ConvertTo<int>(argv[0]);
        intf->IncreaseValue(parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_decrease_value(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 0) {
        intf->DecreaseValue();
      } else if(argc >= 1) {
        int parameterNr = ctx->ConvertTo<int>(argv[0]);
        intf->DecreaseValue(parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_start_dim(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      bool up = ctx->ConvertTo<bool>(argv[0]);
      if(argc == 1) {
        intf->StartDim(up);
      } else if(argc >= 2) {
        int parameterNr = ctx->ConvertTo<int>(argv[1]);
        intf->StartDim(up, parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_end_dim(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 0) {
        intf->EndDim();
      } else if(argc >= 1) {
        int parameterNr = ctx->ConvertTo<int>(argv[0]);
        intf->EndDim(parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_set_value(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      double value = ctx->ConvertTo<double>(argv[0]);
      if(argc == 1) {
        intf->SetValue(value);
      } else if(argc >= 2) {
        int parameterNr = ctx->ConvertTo<int>(argv[1]);
        intf->SetValue(value, parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_set_value

  JSBool dev_call_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      int sceneNr = ctx->ConvertTo<int>(argv[0]);
      if(argc == 1) {
        intf->CallScene(sceneNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_call_scene

  JSBool dev_undo_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      int sceneNr = ctx->ConvertTo<int>(argv[0]);
      if(argc == 1) {
        intf->UndoScene(sceneNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_undo_scene

  JSBool dev_save_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.Is("set") || self.Is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      int sceneNr = ctx->ConvertTo<int>(argv[0]);
      if(argc == 1) {
        intf->SaveScene(sceneNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_save_scene

  class JSDeviceAction : public IDeviceAction {
  private:
    jsval m_Function;
    ScriptContext& m_Context;
    ModelScriptContextExtension& m_Extension;
  public:
    JSDeviceAction(jsval _function, ScriptContext& _ctx, ModelScriptContextExtension& _ext)
    : m_Function(_function), m_Context(_ctx), m_Extension(_ext) {}

    virtual ~JSDeviceAction() {};

    virtual bool Perform(Device& _device) {
      jsval rval;
      JSObject* device = m_Extension.CreateJSDevice(m_Context, _device);
      jsval dev = OBJECT_TO_JSVAL(device);
      JS_CallFunctionValue(m_Context.GetJSContext(), JS_GetGlobalObject(m_Context.GetJSContext()), m_Function, 1, &dev, &rval);
      return true;
    }
  };

  JSBool dev_perform(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(obj, *ctx);

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(self.Is("set")) {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));
      if(argc == 1) {
        JSDeviceAction act = JSDeviceAction(argv[0], *ctx, *ext);
        set->Perform(act);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSFunctionSpec device_interface_methods[] = {
    {"turnOn", dev_turn_on, 0, 0, 0},
    {"turnOff", dev_turn_on, 0, 0, 0},
    {"enable", dev_enable, 0, 0, 0},
    {"disable", dev_disable, 0, 0, 0},
    {"startDim", dev_start_dim, 1, 0, 0},
    {"endDim", dev_end_dim, 0, 0, 0},
    {"setValue", dev_set_value, 0, 0, 0},
//    {"getValue", dev_turn_on, 0, 0, 0},
    {"perform", dev_perform, 1, 0, 0},
    {"increaseValue", dev_increase_value, 0, 0, 0},
    {"decreaseValue", dev_decrease_value, 0, 0, 0},
    {"callScene", dev_call_scene, 1, 0, 0},
    {"saveScene", dev_save_scene, 1, 0, 0},
    {"undoScene", dev_undo_scene, 1, 0, 0},
    {NULL}
  };

  JSObject* ModelScriptContextExtension::CreateJSSet(ScriptContext& _ctx, Set& _set) {
    JSObject* result = JS_NewObject(_ctx.GetJSContext(), &set_class, NULL, NULL);
    JS_DefineFunctions(_ctx.GetJSContext(), result, set_methods);
    JS_DefineFunctions(_ctx.GetJSContext(), result, device_interface_methods);
    JS_DefineProperties(_ctx.GetJSContext(), result, set_properties);
    Set* innerObj = new Set(_set);
    JS_SetPrivate(_ctx.GetJSContext(), result, innerObj);
    return result;
  } // CreateJSSet


  JSBool dev_JSGet(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    DeviceReference* dev = static_cast<DeviceReference*>(JS_GetPrivate(cx, obj));

    if(dev != NULL) {
      int opt = JSVAL_TO_INT(id);
      switch(opt) {
        case 0:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "device"));
          return JS_TRUE;
        case 1:
          *rval = INT_TO_JSVAL(dev->GetDSID());
          return JS_TRUE;
        case 2:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dev->GetDevice().GetName().c_str()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  void finalize_dev(JSContext *cx, JSObject *obj) {
    DeviceReference* pDevice = static_cast<DeviceReference*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pDevice;
  } // finalize_dev

  static JSClass dev_class = {
    "dev", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_dev, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSPropertySpec dev_properties[] = {
    {"className", 0, 0, dev_JSGet},
    {"id", 1, 0, dev_JSGet},
    {"name", 2, 0, dev_JSGet},
    {NULL}
  };

  JSObject* ModelScriptContextExtension::CreateJSDevice(ScriptContext& _ctx, Device& _dev) {
    DeviceReference ref(_dev.GetShortAddress(), m_Apartment);
    return CreateJSDevice(_ctx, ref);
  } // CreateJSDevice

  JSObject* ModelScriptContextExtension::CreateJSDevice(ScriptContext& _ctx, DeviceReference& _ref) {
    JSObject* result = JS_NewObject(_ctx.GetJSContext(), &dev_class, NULL, NULL);
    JS_DefineFunctions(_ctx.GetJSContext(), result, device_interface_methods);
    JS_DefineProperties(_ctx.GetJSContext(), result, dev_properties);
    DeviceReference* innerObj = new DeviceReference(_ref.GetDSID(), _ref.GetDevice().GetApartment());
    // make an explicit copy
    *innerObj = _ref;
    JS_SetPrivate(_ctx.GetJSContext(), result, innerObj);
    return result;
  } // CreateJSDevice

  //================================================== EventScriptExtension

  const string EventScriptExtensionName = "eventextension";

  EventScriptExtension::EventScriptExtension(EventQueue& _queue, EventInterpreter& _interpreter)
  : ScriptExtension(EventScriptExtensionName),
    m_Queue(_queue),
    m_Interpreter(_interpreter)
  { } // ctor

  JSBool global_event(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc < 1) {
      *rval = JSVAL_NULL;
      Logger::GetInstance()->Log("JS: global_event: (empty name)");
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->GetEnvironment().GetExtension(EventScriptExtensionName));
      if(ext != NULL) {
        JSString *val = JS_ValueToString(cx, argv[0]);
        char* name = JS_GetStringBytes(val);

        boost::shared_ptr<Event> newEvent(new Event(name));
        JSObject* obj = ext->CreateJSEvent(*ctx, newEvent);

        *rval = OBJECT_TO_JSVAL(obj);
      }
    }
    return JS_TRUE;
  } // global_event

  JSBool global_subscription(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc < 3) {
      *rval = JSVAL_NULL;
      Logger::GetInstance()->Log("JS: global_subscription: need three arguments: event-name, handler-name & parameter");
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->GetEnvironment().GetExtension(EventScriptExtensionName));
      if(ext != NULL) {
        JSString *val = JS_ValueToString(cx, argv[0]);
        char* eventName = JS_GetStringBytes(val);

        val = JS_ValueToString(cx, argv[1]);
        char* handlerName = JS_GetStringBytes(val);

        JSObject* paramObj;

        boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());

        if(JS_ValueToObject(cx, argv[2], &paramObj) == JS_TRUE) {
          JSObject* propIter = JS_NewPropertyIterator(cx, paramObj);
          jsid propID;
          while(JS_NextProperty(cx, propIter, &propID) == JS_TRUE) {
            if(propID == JSVAL_VOID) {
              break;
            }
            JSObject* obj;
            jsval vp;
            JS_GetMethodById(cx, paramObj, propID, &obj, &vp);

            jsval vp2;

            val = JS_ValueToString(cx, vp);
            char* propValue = JS_GetStringBytes(val);

            JS_IdToValue(cx, propID, &vp2);
            val = JS_ValueToString(cx, vp2);
            char* propName = JS_GetStringBytes(val);

            opts->SetParameter(propName, propValue);
          }

        }

        boost::shared_ptr<EventSubscription> subscription(new EventSubscription(eventName, handlerName, opts));

        JSObject* obj = ext->CreateJSSubscription(*ctx, subscription);
        *rval = OBJECT_TO_JSVAL(obj);
      }
    }
    return JS_TRUE;
  } // global_subscription

  JSFunctionSpec event_global_methods[] = {
    {"event", global_event, 1, 0, 0},
    {"subscription", global_subscription},
    {NULL},
  };

  void EventScriptExtension::ExtendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.GetJSContext(), JS_GetGlobalObject(_context.GetJSContext()), event_global_methods);
  } // ExtendContext

  struct event_wrapper {
    boost::shared_ptr<Event> event;
  };

  JSBool event_raise(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(obj, *ctx);

    EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->GetEnvironment().GetExtension(EventScriptExtensionName));
    if(self.Is("event")) {
      event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, obj));
      ext->GetEventQueue().PushEvent(eventWrapper->event);
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // event_raise

  JSBool event_JSGet(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, obj));

    if(eventWrapper != NULL) {
      int opt = JSVAL_TO_INT(id);
      switch(opt) {
        case 0:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "event"));
          return JS_TRUE;
        case 1:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, eventWrapper->event->GetName().c_str()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  void finalize_event(JSContext *cx, JSObject *obj) {
    event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete eventWrapper;
  } // finalize_dev

  static JSClass event_class = {
    "event", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_event, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSPropertySpec event_properties[] = {
    {"className", 0, 0, event_JSGet},
    {"name", 1, 0, event_JSGet},
    {NULL}
  };

  JSFunctionSpec event_methods[] = {
    {"raise", event_raise, 1, 0, 0},
    {NULL}
  };

  JSObject* EventScriptExtension::CreateJSEvent(ScriptContext& _ctx, boost::shared_ptr<Event> _event) {
    JSObject* result = JS_NewObject(_ctx.GetJSContext(), &event_class, NULL, NULL);
    JS_DefineFunctions(_ctx.GetJSContext(), result, event_methods);
    JS_DefineProperties(_ctx.GetJSContext(), result, event_properties);

    event_wrapper* evtWrapper = new event_wrapper();
    evtWrapper->event = _event;
    JS_SetPrivate(_ctx.GetJSContext(), result, evtWrapper);
    return result;
  } // CreateJSEvent

  struct subscription_wrapper {
    boost::shared_ptr<EventSubscription> subscription;
  };

  JSBool subscription_JSGet(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    subscription_wrapper* subscriptionWrapper = static_cast<subscription_wrapper*>(JS_GetPrivate(cx, obj));

    if(subscriptionWrapper != NULL) {
      int opt = JSVAL_TO_INT(id);
      switch(opt) {
        case 0:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "subscription"));
          return JS_TRUE;
        case 1:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, subscriptionWrapper->subscription->GetEventName().c_str()));
          return JS_TRUE;
        case 2:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, subscriptionWrapper->subscription->GetHandlerName().c_str()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  JSBool subscription_subscribe(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(obj, *ctx);

    EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->GetEnvironment().GetExtension(EventScriptExtensionName));
    if(self.Is("subscription")) {
      subscription_wrapper* subscriptionWrapper = static_cast<subscription_wrapper*>(JS_GetPrivate(cx, obj));
      ext->GetEventInterpreter().Subscribe(subscriptionWrapper->subscription);
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // subscription_subscribe

  void finalize_subscription(JSContext *cx, JSObject *obj) {
    subscription_wrapper* subscriptionWrapper = static_cast<subscription_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete subscriptionWrapper;
  } // finalize_dev

  static JSClass subscription_class = {
    "subscription", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_subscription, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSPropertySpec subscription_properties[] = {
    {"className", 0, 0, subscription_JSGet},
    {"eventName", 1, 0, subscription_JSGet},
    {"handlerName", 2, 0, subscription_JSGet},
    {NULL}
  };

  JSFunctionSpec subscription_methods[] = {
    {"subscribe", subscription_subscribe, 0, 0, 0},
    {NULL}
  };


  JSObject* EventScriptExtension::CreateJSSubscription(ScriptContext& _ctx, boost::shared_ptr<EventSubscription> _subscription) {
    JSObject* result = JS_NewObject(_ctx.GetJSContext(), &subscription_class, NULL, NULL);
    JS_DefineFunctions(_ctx.GetJSContext(), result, subscription_methods);
    JS_DefineProperties(_ctx.GetJSContext(), result, subscription_properties);

    subscription_wrapper* subscriptionWrapper = new subscription_wrapper();
    subscriptionWrapper->subscription = _subscription;
    JS_SetPrivate(_ctx.GetJSContext(), result, subscriptionWrapper);
    return result;
  }

} // namespace
