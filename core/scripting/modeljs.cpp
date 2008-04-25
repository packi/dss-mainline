/*
 *  modeljs.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/22/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "modeljs.h"

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
    throw new ScriptException("Wrong classname for set");
  }
  
  template<>
  Set& ModelScriptContextExtension::ConvertTo(ScriptContext& _context, jsval _val) {
    if(JSVAL_IS_OBJECT(_val)) {
      return ConvertTo<Set&>(_context, JSVAL_TO_OBJECT(_val));
    }
    throw new ScriptException("JSVal is no object");
  }
  
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
  }
  
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
  }
  
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
  }

  JSFunctionSpec model_global_methods[] = {
    {"getName", global_get_name, 0, 0, 0},
    {"setName", global_set_name, 1, 0, 0},
    {"getDevices", global_get_devices, 0, 0, 0},
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
  }
  
  class JSDeviceAction : public IDeviceAction {
  private:
    jsval m_Function;
    ModelScriptContextExtension& m_Extension;
    ScriptContext& m_Context;
  public:
    JSDeviceAction(jsval _function, ScriptContext& _ctx, ModelScriptContextExtension& _ext)
    : m_Function(_function), m_Context(_ctx), m_Extension(_ext) {}
    
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
          *rval = INT_TO_JSVAL(dev->GetID());
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
    DeviceReference ref(_dev.GetID(), m_Apartment);
    return CreateJSDevice(_ctx, ref);
  } // CreateJSDevice
  
  JSObject* ModelScriptContextExtension::CreateJSDevice(ScriptContext& _ctx, DeviceReference& _ref) {
    JSObject* result = JS_NewObject(_ctx.GetJSContext(), &dev_class, NULL, NULL);
    JS_DefineFunctions(_ctx.GetJSContext(), result, device_interface_methods);
    JS_DefineProperties(_ctx.GetJSContext(), result, dev_properties);
    DeviceReference* innerObj = new DeviceReference(_ref.GetID(), _ref.GetDevice().GetApartment());
    // make an explicit copy
    *innerObj = _ref;
    JS_SetPrivate(_ctx.GetJSContext(), result, innerObj);
    return result;
  } // CreateJSDevice

  
  
  
}
