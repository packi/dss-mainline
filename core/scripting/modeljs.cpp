/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "modeljs.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

#include "core/dss.h"
#include "core/logger.h"
#include "core/propertysystem.h"

namespace dss {
  const std::string ModelScriptcontextExtensionName = "modelextension";

  ModelScriptContextExtension::ModelScriptContextExtension(Apartment& _apartment)
  : ScriptExtension(ModelScriptcontextExtensionName),
    m_Apartment(_apartment)
  {
  } // ctor

  template<>
  Set& ModelScriptContextExtension::convertTo(ScriptContext& _context, JSObject* _obj) {
    ScriptObject obj(_obj, _context);
    if(obj.is("set")) {
      return *static_cast<Set*>(JS_GetPrivate(_context.getJSContext(), _obj));
    }
    throw ScriptException("Wrong classname for set");
  } // convertTo<Set>

  template<>
  Set& ModelScriptContextExtension::convertTo(ScriptContext& _context, jsval _val) {
    if(JSVAL_IS_OBJECT(_val)) {
      return convertTo<Set&>(_context, JSVAL_TO_OBJECT(_val));
    }
    throw ScriptException("JSVal is no object");
  } // convertTo<Set>

  JSBool global_get_name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      std::string aptName = ext->getApartment().getName();
      JSString* str = JS_NewStringCopyN(cx, aptName.c_str(), aptName.size());

      *rval = STRING_TO_JSVAL(str);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_get_name

  JSBool global_set_name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && argc >= 1) {
      JSString* str = JS_ValueToString(cx, argv[0]);
      if(str != NULL) {
        std::string aptName = JS_GetStringBytes(str);
        ext->getApartment().setName(aptName);

        *rval = INT_TO_JSVAL(0);
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // global_set_name

  JSBool global_get_devices(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      Set devices = ext->getApartment().getDevices();
      JSObject* obj = ext->createJSSet(*ctx, devices);

      *rval = OBJECT_TO_JSVAL(obj);

      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_get_devices

  JSBool global_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    if (argc < 1) {
      *rval = INT_TO_JSVAL(0); /* Send back a return value of 0. */
      Logger::getInstance()->log("JS: global_log: (empty std::string)");
    } else {
      std::stringstream sstr;
      unsigned int i;
      for (i=0; i<argc; i++){
        JSString *val = JS_ValueToString(cx, argv[i]); /* Convert the value to a javascript std::string. */
        char *str = JS_GetStringBytes(val); /* Then convert it to a C-style std::string. */
        sstr << str;
      }
      Logger::getInstance()->log(sstr.str());
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

  void ModelScriptContextExtension::extendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), model_global_methods);
  } // extendedJSContext

  void finalize_set(JSContext *cx, JSObject *obj) {
    Set* pSet = static_cast<Set*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pSet;
  } // finalize_set

  JSBool set_length(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL) {
      *rval = INT_TO_JSVAL(set->length());
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool set_combine(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      Set& otherSet = ext->convertTo<Set&>(*ctx, argv[0]);
      Set result = set->combine(otherSet);
      JSObject* resultObj = ext->createJSSet(*ctx, result);
       *rval = OBJECT_TO_JSVAL(resultObj);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool set_remove(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      Set& otherSet = ext->convertTo<Set&>(*ctx, argv[0]);
      Set result = set->remove(otherSet);
      JSObject* resultObj = ext->createJSSet(*ctx, result);
      *rval = OBJECT_TO_JSVAL(resultObj);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // set_remove

  JSBool set_by_name(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      JSString* str = JS_ValueToString(cx, argv[0]);
      if(str != NULL) {
        std::string name = JS_GetStringBytes(str);
        DeviceReference result = set->getByName(name);
        JSObject* resultObj = ext->createJSDevice(*ctx, result);
        *rval = OBJECT_TO_JSVAL(resultObj);
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // set_by_name

  JSBool set_by_dsid(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      JSString* str = JS_ValueToString(cx, argv[0]);
      if(str != NULL) {
        std::string dsid = JS_GetStringBytes(str);
        DeviceReference result = set->getByDSID(dsid_t::fromString(dsid));
        JSObject* resultObj = ext->createJSDevice(*ctx, result);
        *rval = OBJECT_TO_JSVAL(resultObj);
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // set_by_dsid

  JSBool set_by_functionid(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      int32_t fid = 0;
      if(JS_ValueToInt32(cx, argv[0], &fid)) {
        Set result = set->getByFunctionID(fid);
        JSObject* resultObj = ext->createJSSet(*ctx, result);
        *rval = OBJECT_TO_JSVAL(resultObj);
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // set_by_functionid

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
    {"byName", set_by_name, 1, 0, 0},
    {"byDSID", set_by_dsid, 1, 0, 0},
    {"byFunctionID", set_by_functionid, 1, 0, 0},
    {NULL},
  };

  static JSPropertySpec set_properties[] = {
    {"className", 0, 0, set_JSGet},
    {NULL}
  };

  JSBool dev_turn_on(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->turnOn();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_turn_off(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->turnOff();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_enable(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->enable();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_disable(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->turnOn();
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_increase_value(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 0) {
        intf->increaseValue();
      } else if(argc >= 1) {
        int parameterNr = ctx->convertTo<int>(argv[0]);
        intf->increaseValue(parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_decrease_value(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 0) {
        intf->decreaseValue();
      } else if(argc >= 1) {
        int parameterNr = ctx->convertTo<int>(argv[0]);
        intf->decreaseValue(parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_start_dim(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      bool up = ctx->convertTo<bool>(argv[0]);
      if(argc == 1) {
        intf->startDim(up);
      } else if(argc >= 2) {
        int parameterNr = ctx->convertTo<int>(argv[1]);
        intf->startDim(up, parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_end_dim(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 0) {
        intf->endDim();
      } else if(argc >= 1) {
        int parameterNr = ctx->convertTo<int>(argv[0]);
        intf->endDim(parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool dev_set_value(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      double value = ctx->convertTo<double>(argv[0]);
      if(argc == 1) {
        intf->setValue(value);
      } else if(argc >= 2) {
        int parameterNr = ctx->convertTo<int>(argv[1]);
        intf->setValue(value, parameterNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_set_value

  JSBool dev_call_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      int sceneNr = ctx->convertTo<int>(argv[0]);
      if(argc == 1) {
        intf->callScene(sceneNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_call_scene

  JSBool dev_undo_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      int sceneNr = ctx->convertTo<int>(argv[0]);
      if(argc == 1) {
        intf->undoScene(sceneNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_undo_scene

  JSBool dev_save_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      int sceneNr = ctx->convertTo<int>(argv[0]);
      if(argc == 1) {
        intf->saveScene(sceneNr);
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_save_scene

  JSBool dev_dslink_send(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("device")) {
      DeviceReference* ref = static_cast<DeviceReference*>(JS_GetPrivate(cx, obj));
      int value = ctx->convertTo<int>(argv[0]);
      bool writeOnly = false;
      bool lastByte = false;
      if(argc == 0) {
        return JS_FALSE;
      }
      if(argc > 1) {
        lastByte = ctx->convertTo<bool>(argv[1]);
      }
      if(argc > 2) {
        writeOnly = ctx->convertTo<bool>(argv[2]);
      }
      try {
        value = ref->getDevice().dsLinkSend(value, lastByte, writeOnly);
        *rval = INT_TO_JSVAL(value);
        return JS_TRUE;
      } catch(std::runtime_error&) {
        JS_ReportError(cx, "Error receiving value");
        return JS_FALSE;
      }
    }
    return JS_FALSE;
  } // dev_dslink_send

  class JSDeviceAction : public IDeviceAction {
  private:
    jsval m_Function;
    ScriptContext& m_Context;
    ModelScriptContextExtension& m_Extension;
  public:
    JSDeviceAction(jsval _function, ScriptContext& _ctx, ModelScriptContextExtension& _ext)
    : m_Function(_function), m_Context(_ctx), m_Extension(_ext) {}

    virtual ~JSDeviceAction() {};

    virtual bool perform(Device& _device) {
      jsval rval;
      JSObject* device = m_Extension.createJSDevice(m_Context, _device);
      jsval dev = OBJECT_TO_JSVAL(device);
      JS_CallFunctionValue(m_Context.getJSContext(), JS_GetGlobalObject(m_Context.getJSContext()), m_Function, 1, &dev, &rval);
      return true;
    }
  };

  JSBool dev_perform(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(obj, *ctx);

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(self.is("set")) {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));
      if(argc == 1) {
        JSDeviceAction act = JSDeviceAction(argv[0], *ctx, *ext);
        set->perform(act);
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
    {"perform", dev_perform, 1, 0, 0},
    {"increaseValue", dev_increase_value, 0, 0, 0},
    {"decreaseValue", dev_decrease_value, 0, 0, 0},
    {"callScene", dev_call_scene, 1, 0, 0},
    {"saveScene", dev_save_scene, 1, 0, 0},
    {"undoScene", dev_undo_scene, 1, 0, 0},
    {"dSLinkSend", dev_dslink_send, 3, 0, 0},
    {NULL}
  };

  JSObject* ModelScriptContextExtension::createJSSet(ScriptContext& _ctx, Set& _set) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &set_class, NULL, NULL);
    JS_DefineFunctions(_ctx.getJSContext(), result, set_methods);
    JS_DefineFunctions(_ctx.getJSContext(), result, device_interface_methods);
    JS_DefineProperties(_ctx.getJSContext(), result, set_properties);
    Set* innerObj = new Set(_set);
    JS_SetPrivate(_ctx.getJSContext(), result, innerObj);
    return result;
  } // createJSSet


  JSBool dev_JSGet(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    DeviceReference* dev = static_cast<DeviceReference*>(JS_GetPrivate(cx, obj));

    if(dev != NULL) {
      int opt = JSVAL_TO_INT(id);
      switch(opt) {
        case 0:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "device"));
          return JS_TRUE;
        case 1:
          {
            // make a local reference so the std::string does not go out of scope
            std::string tmp = dev->getDSID().toString();
            *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp.c_str()));
          }
          return JS_TRUE;
        case 2:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dev->getDevice().getName().c_str()));
          return JS_TRUE;
        case 3: 
          *rval = INT_TO_JSVAL(dev->getDevice().getZoneID());
          return JS_TRUE;
        case 4:
          *rval = INT_TO_JSVAL(dev->getDevice().getModulatorID());
          return JS_TRUE;
        case 5:
          *rval = INT_TO_JSVAL(dev->getDevice().getFunctionID());
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
    {"dsid", 1, 0, dev_JSGet},
    {"name", 2, 0, dev_JSGet},
    {"zoneID", 3, 0, dev_JSGet},
    {"circuitID", 4, 0, dev_JSGet},
    {"functionID", 5, 0, dev_JSGet},
    {NULL}
  };

  JSObject* ModelScriptContextExtension::createJSDevice(ScriptContext& _ctx, Device& _dev) {
    DeviceReference ref(_dev.getDSID(), m_Apartment);
    return createJSDevice(_ctx, ref);
  } // createJSDevice

  JSObject* ModelScriptContextExtension::createJSDevice(ScriptContext& _ctx, DeviceReference& _ref) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &dev_class, NULL, NULL);
    JS_DefineFunctions(_ctx.getJSContext(), result, device_interface_methods);
    JS_DefineProperties(_ctx.getJSContext(), result, dev_properties);
    DeviceReference* innerObj = new DeviceReference(_ref.getDSID(), _ref.getDevice().getApartment());
    // make an explicit copy
    *innerObj = _ref;
    JS_SetPrivate(_ctx.getJSContext(), result, innerObj);
    return result;
  } // createJSDevice

  //================================================== EventScriptExtension

  const std::string EventScriptExtensionName = "eventextension";

  EventScriptExtension::EventScriptExtension(EventQueue& _queue, EventInterpreter& _interpreter)
  : ScriptExtension(EventScriptExtensionName),
    m_Queue(_queue),
    m_Interpreter(_interpreter)
  { } // ctor

  JSBool global_event(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc < 1) {
      *rval = JSVAL_NULL;
      Logger::getInstance()->log("JS: global_event: (empty name)");
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
      if(ext != NULL) {
        JSString *val = JS_ValueToString(cx, argv[0]);
        char* name = JS_GetStringBytes(val);

        boost::shared_ptr<Event> newEvent(new Event(name));

        JSObject* paramObj;
        if((argc >= 2) && (JS_ValueToObject(cx, argv[1], &paramObj) == JS_TRUE)) {
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

            newEvent->setProperty(propName, propValue);
          }
        }

        JSObject* obj = ext->createJSEvent(*ctx, newEvent);

        *rval = OBJECT_TO_JSVAL(obj);
      }
    }
    return JS_TRUE;
  } // global_event

  JSBool global_subscription(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc < 3) {
      *rval = JSVAL_NULL;
      Logger::getInstance()->log("JS: global_subscription: need three arguments: event-name, handler-name & parameter");
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
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

            opts->setParameter(propName, propValue);
          }

        }

        boost::shared_ptr<EventSubscription> subscription(new EventSubscription(eventName, handlerName, ext->getEventInterpreter(), opts));

        JSObject* obj = ext->createJSSubscription(*ctx, subscription);
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

  void EventScriptExtension::extendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), event_global_methods);
  } // extendContext

  struct event_wrapper {
    boost::shared_ptr<Event> event;
  };

  JSBool event_raise(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(obj, *ctx);

    EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
    if(self.is("event")) {
      event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, obj));
      ext->getEventQueue().pushEvent(eventWrapper->event);
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
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, eventWrapper->event->getName().c_str()));
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

  JSObject* EventScriptExtension::createJSEvent(ScriptContext& _ctx, boost::shared_ptr<Event> _event) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &event_class, NULL, NULL);
    JS_DefineFunctions(_ctx.getJSContext(), result, event_methods);
    JS_DefineProperties(_ctx.getJSContext(), result, event_properties);

    event_wrapper* evtWrapper = new event_wrapper();
    evtWrapper->event = _event;
    JS_SetPrivate(_ctx.getJSContext(), result, evtWrapper);
    return result;
  } // createJSEvent

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
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, subscriptionWrapper->subscription->getEventName().c_str()));
          return JS_TRUE;
        case 2:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, subscriptionWrapper->subscription->getHandlerName().c_str()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  JSBool subscription_subscribe(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(obj, *ctx);

    EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
    if(self.is("subscription")) {
      subscription_wrapper* subscriptionWrapper = static_cast<subscription_wrapper*>(JS_GetPrivate(cx, obj));
      ext->getEventInterpreter().subscribe(subscriptionWrapper->subscription);
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


  JSObject* EventScriptExtension::createJSSubscription(ScriptContext& _ctx, boost::shared_ptr<EventSubscription> _subscription) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &subscription_class, NULL, NULL);
    JS_DefineFunctions(_ctx.getJSContext(), result, subscription_methods);
    JS_DefineProperties(_ctx.getJSContext(), result, subscription_properties);

    subscription_wrapper* subscriptionWrapper = new subscription_wrapper();
    subscriptionWrapper->subscription = _subscription;
    JS_SetPrivate(_ctx.getJSContext(), result, subscriptionWrapper);
    return result;
  }

  //================================================== PropertyScriptExtension

  const std::string PropertyScriptExtensionName = "propertyextension";

  PropertyScriptExtension::PropertyScriptExtension(PropertySystem& _propertySystem) 
  : ScriptExtension(PropertyScriptExtensionName),
    m_PropertySystem(_propertySystem),
    m_NextListenerID(1)
  { } // ctor
    
  JSBool global_prop_setProperty(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc > 2) {
      Logger::getInstance()->log("JS: global_prop_setProperty: need two arguments: property-path & value", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string propName = ctx->convertTo<std::string>(argv[0]);
      
      PropertyNodePtr node = ext->getPropertySystem().createProperty(propName);
      if(node != NULL) {
        try {
          if(JSVAL_IS_STRING(argv[1])) {
            node->setStringValue(ctx->convertTo<std::string>(argv[1]));
          } else if(JSVAL_IS_BOOLEAN(argv[1])) {
            node->setBooleanValue(ctx->convertTo<bool>(argv[1]));
          } else if(JSVAL_IS_INT(argv[1])) {
            node->setIntegerValue(ctx->convertTo<int>(argv[1]));
          } else {
            Logger::getInstance()->log("JS: global_prop_setProperty: unknown type of argument 2", lsError);
          }
          *rval = JSVAL_TRUE;
          return JS_TRUE;
        } catch(PropertyTypeMismatch&) {
          Logger::getInstance()->log("Error setting value of " + propName, lsFatal);
        }
      } else {
        Logger::getInstance()->log("Coule not create property " + propName, lsFatal);
        *rval = JSVAL_FALSE;
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // global_prop_setProperty

  JSBool global_prop_getProperty(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc > 1) {
      Logger::getInstance()->log("JS: global_prop_getProperty: need one argument: property-path", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string propName = ctx->convertTo<std::string>(argv[0]);
      
      PropertyNodePtr node = ext->getPropertySystem().getProperty(propName);
      if(node == NULL) {
        *rval = JSVAL_NULL;
      } else {
        switch(node->getValueType()) {
        case vTypeInteger:
          *rval = INT_TO_JSVAL(node->getIntegerValue());
          break;
        case vTypeString: {
            std::string val = node->getStringValue();
            *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, val.c_str()));
          }
          break;
        case vTypeBoolean:
          *rval = BOOLEAN_TO_JSVAL(node->getBoolValue());
          break;
        case vTypeNone:
          *rval = JSVAL_VOID;
        default:
          assert(false);
        }
      }
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_getProperty

  JSBool global_prop_setListener(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc > 2) {
      Logger::getInstance()->log("JS: global_prop_setListener: need two arguments: property-path &  callback", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string propName = ctx->convertTo<std::string>(argv[0]);

      PropertyNodePtr node = ext->getPropertySystem().getProperty(propName);
      if(node == NULL) {
        *rval = JSVAL_NULL;
      } else {
        std::string ident = ext->produceListenerID();
        PropertyScriptListener* listener =
            new PropertyScriptListener(ext, ctx, obj, argv[1], ident);
        ext->addListener(listener);
        node->addListener(listener);
        ctx->attachObject(listener);

        JSString* str = JS_NewStringCopyZ(cx, ident.c_str());
        *rval = STRING_TO_JSVAL(str);
      }
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_setListener

  JSBool global_prop_removeListener(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc > 2) {
      Logger::getInstance()->log("JS: global_prop_removeListener: need two arguments: listener-id", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string listenerIdent = ctx->convertTo<std::string>(argv[0]);
      ext->removeListener(listenerIdent);

      *rval = JSVAL_TRUE;
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_setListener

  JSFunctionSpec prop_global_methods[] = {
    {"setProperty", global_prop_setProperty, 2, 0, 0},
    {"getProperty", global_prop_getProperty, 1, 0, 0},
    {"setListener", global_prop_setListener, 2, 0, 0},
    {"removeListener", global_prop_removeListener, 1, 0, 0},
    {NULL},
  };

  void PropertyScriptExtension::extendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), prop_global_methods);
  } // extendContext

  JSObject* PropertyScriptExtension::createJSProperty(ScriptContext& _ctx, boost::shared_ptr<PropertyNode> _node) {
    return NULL;
  } // createJSProperty

  std::string PropertyScriptExtension::produceListenerID() {
    return "listener_" + intToString(long(this), true) + "_" + intToString(m_NextListenerID++);
  } // produceListenerID

  void PropertyScriptExtension::addListener(PropertyScriptListener* _pListener) {
    m_Listeners.push_back(_pListener);
  } // addListener

  void PropertyScriptExtension::removeListener(const std::string& _identifier) {
    for(std::vector<PropertyScriptListener*>::iterator it = m_Listeners.begin(), e = m_Listeners.end();
        it != e; ++it) {
      if((*it)->getIdentifier() == _identifier) {
        (*it)->unsubscribe();
        m_Listeners.erase(it);
        return;
      }
    }
  } // removeListener


  //================================================== PropertyScriptListener
  
  PropertyScriptListener::PropertyScriptListener(PropertyScriptExtension* _pExtension,
                                                 ScriptContext* _pContext, 
                                                 JSObject* _functionObj, 
                                                 jsval _function, 
                                                 const std::string& _identifier)
  : m_pExtension(_pExtension),
    m_pContext(_pContext),
    m_pFunctionObject(_functionObj),
    m_Function(_function),
    m_Identifier(_identifier)
  {
    JS_AddRoot(_pContext->getJSContext(), &m_Function);
  } // ctor

  PropertyScriptListener::~PropertyScriptListener() {
    m_pExtension->removeListener(m_Identifier);
    JS_RemoveRoot(m_pContext->getJSContext(), &m_Function);
  } // dtor

  void PropertyScriptListener::createScriptObject() {
    if(m_pScriptObject == NULL) {
      m_pScriptObject.reset(new ScriptObject(m_pFunctionObject, *m_pContext));
    }
    assert(m_pScriptObject != NULL);
  } // createScriptObject

  void PropertyScriptListener::propertyChanged(PropertyNodePtr _changedNode) {
    doOnChange(_changedNode);
  } // propertyChanged
  
  void PropertyScriptListener::propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyRemoved
  
  void PropertyScriptListener::propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyAdded

  void PropertyScriptListener::doOnChange(PropertyNodePtr _changedNode) {
    AssertLocked locked(m_pContext);
    createScriptObject();
    ScriptFunctionParameterList list(*m_pContext);
    list.add(_changedNode->getDisplayName());
    try {
      m_pScriptObject->callFunctionByReference<void>(m_Function, list);
    } catch(ScriptException& e) {
      Logger::getInstance()->log("PropertyScriptListener::doOnChange: Caught exception while calling handler: " + std::string(e.what()), lsFatal);
    }
  } // doOnChange

} // namespace
