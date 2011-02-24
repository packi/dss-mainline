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

#include "modeljs.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

#include "core/dss.h"
#include "core/logger.h"
#include "core/businterface.h"
#include "core/foreach.h"
#include "core/model/device.h"
#include "core/model/devicereference.h"
#include "core/model/apartment.h"
#include "core/model/set.h"
#include "core/model/modulator.h"
#include "core/model/modelconst.h"
#include "core/metering/metering.h"
#include "core/scripting/scriptobject.h"


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

  JSBool global_get_dsmeterbydsid(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    JSRequest req(ctx);

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      if(argc >= 1) {
        dss_dsid_t dsid = NullDSID;
        try {
          dsid =  dss_dsid_t::fromString(ctx->convertTo<std::string>(argv[0]));
        } catch(std::invalid_argument& e) {
          Logger::getInstance()->log(std::string("Error converting dsid string to dsid") + e.what(), lsError);
          return JS_FALSE;
        }
        try {
          boost::shared_ptr<DSMeter> meter = ext->getApartment().getDSMeterByDSID(dsid);
          JSObject* obj = ext->createJSMeter(*ctx, meter);

          *rval = OBJECT_TO_JSVAL(obj);
        } catch(ItemNotFoundException& e) {
          *rval = JSVAL_NULL;
        }

        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // global_get_dsmeterbydsid

  JSBool global_getEnergyMeterValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      uint32_t result = 0;
      foreach(boost::shared_ptr<DSMeter> pDSMeter, ext->getApartment().getDSMeters()) {
        result += pDSMeter->getEnergyMeterValue();
      }
      *rval = INT_TO_JSVAL(result);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_getEnergyMeterValue

  JSBool global_getConsumption(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      uint32_t result = 0;
      foreach(boost::shared_ptr<DSMeter> pDSMeter, ext->getApartment().getDSMeters()) {
        result += pDSMeter->getPowerConsumption();
      }
      *rval = INT_TO_JSVAL(result);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_getConsumption

  JSFunctionSpec apartment_static_methods[] = {
    {"getName", global_get_name, 0, 0, 0},
    {"setName", global_set_name, 1, 0, 0},
    {"getDevices", global_get_devices, 0, 0, 0},
    {"getDSMeterByDSID", global_get_dsmeterbydsid, 1, 0, 0},
    {"getConsumption", global_getConsumption, 0, 0, 0},
    {"getEnergyMeterValue", global_getEnergyMeterValue, 0, 0, 0},
    {NULL},
  };

  JSBool apartment_construct(JSContext *cx, JSObject *obj, uintN argc,
                        jsval *argv, jsval *rval) {
    return JS_FALSE;
  } // apartment_construct

  static JSClass apartment_class = {
    "Apartment", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // apartment_class

  JSFunctionSpec model_global_methods[] = {
    {"log", global_log, 1, 0, 0},
    {NULL},
  };

  void ModelScriptContextExtension::extendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), model_global_methods);
    JS_DefineFunctions(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), apartment_static_methods);
    JS_InitClass(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), NULL,
                 &apartment_class, &apartment_construct, 0, NULL, NULL, NULL, apartment_static_methods);
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

  class JSDeviceAction : public IDeviceAction {
    private:
      jsval m_Function;
      ScriptContext& m_Context;
      ModelScriptContextExtension& m_Extension;
    public:
      JSDeviceAction(jsval _function, ScriptContext& _ctx, ModelScriptContextExtension& _ext)
      : m_Function(_function), m_Context(_ctx), m_Extension(_ext) {}

      virtual ~JSDeviceAction() {};

      virtual bool perform(boost::shared_ptr<Device> _device) {
        jsval rval;
        JSObject* device = m_Extension.createJSDevice(m_Context, _device);
        jsval dev = OBJECT_TO_JSVAL(device);
        JS_CallFunctionValue(m_Context.getJSContext(), JS_GetGlobalObject(m_Context.getJSContext()), m_Function, 1, &dev, &rval);
        return true;
      }
  };

  JSBool set_perform(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
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
  } // set_perform

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

  JSBool set_by_shortaddress(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 2) {
      try {
        if (!JSVAL_IS_INT(argv[0])) {
            Logger::getInstance()->log("JS: set_by_shortaddress: Could not parse circuit id as integer", lsWarning);
            *rval = JSVAL_NULL;
            return JS_FALSE;
        }

        if (!JSVAL_IS_INT(argv[2])) {
            Logger::getInstance()->log("JS: set_by_shortaddress: Could not parse bus id as integer", lsWarning);
            *rval = JSVAL_NULL;
            return JS_FALSE;
        }

        std::string dsmeterID = ctx->convertTo<std::string>(argv[0]);
        int busid = JSVAL_TO_INT(argv[1]);
        try {
          dss_dsid_t meterDSID = dss_dsid_t::fromString(dsmeterID);
          DeviceReference result = set->getByBusID(busid, meterDSID);
          JSObject* resultObj = ext->createJSDevice(*ctx, result);
          *rval = OBJECT_TO_JSVAL(resultObj);
          return JS_TRUE;
        } catch(std::invalid_argument&) {
          Logger::getInstance()->log("JS: Could not parse dsid '" + dsmeterID + "'", lsWarning);
        }
      } catch(ItemNotFoundException&) {
          Logger::getInstance()->log("JS: set.byShortAddress: Device not found", lsWarning);
      }
        *rval = JSVAL_NULL;
        return JS_TRUE;
      }
    return JS_FALSE;
  } // set_by_shortaddress


  JSBool set_by_dsid(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      JSString* str = JS_ValueToString(cx, argv[0]);
      if(str != NULL) {
        std::string dsid = JS_GetStringBytes(str);
        try {
          DeviceReference result = set->getByDSID(dss_dsid_t::fromString(dsid));
          JSObject* resultObj = ext->createJSDevice(*ctx, result);
          *rval = OBJECT_TO_JSVAL(resultObj);
          return JS_TRUE;
        } catch(std::invalid_argument&) {
          Logger::getInstance()->log("JS: set.byDSID: Could not parse dsid: '" + dsid + "'", lsError);
        } catch(ItemNotFoundException&) {
          Logger::getInstance()->log("JS: set.byDSID: Device with dsid '" + dsid + "' not found", lsWarning);
        }
        *rval = JSVAL_NULL;
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
      } else {
        Logger::getInstance()->log("JS: set_by_functionid: Could not parse parameter1 as integer", lsWarning);
      }
    }
    return JS_FALSE;
  } // set_by_functionid

  JSBool set_by_zone(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if((ext != NULL) && (set != NULL) && (argc >= 1)) {
      Set result;
      try {
        if(JSVAL_IS_INT(argv[0])) {
          result = set->getByZone(JSVAL_TO_INT(argv[0]));
        } else {
          JSString* str = JS_ValueToString(cx, argv[0]);
          if(str != NULL) {
            std::string zonename = JS_GetStringBytes(str);
            result = set->getByZone(zonename);
          }
        }
      } catch(ItemNotFoundException&) {
        // return an empty set if the zone hasn't been found
        Logger::getInstance()->log("JS: set_by_zone: Zone not found", lsWarning);
      }
      JSObject* resultObj = ext->createJSSet(*ctx, result);
      *rval = OBJECT_TO_JSVAL(resultObj);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // set_by_zone

  JSBool set_by_group(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if((ext != NULL) && (set != NULL) && (argc >= 1)) {
      bool ok = false;
      Set result;
      try {
        if(JSVAL_IS_INT(argv[0])) {
          result = set->getByGroup(JSVAL_TO_INT(argv[0]));
          ok = true;
        } else {
          JSString* str = JS_ValueToString(cx, argv[0]);
          if(str != NULL) {
            std::string groupname = JS_GetStringBytes(str);
            result = set->getByGroup(groupname);
            ok = true;
          }
        }
      } catch(ItemNotFoundException&) {
        ok = true; // return an empty set if the group hasn't been found
        Logger::getInstance()->log("JS: set_by_group: Group not found", lsWarning);
      }
      if(ok) {
        JSObject* resultObj = ext->createJSSet(*ctx, result);
        *rval = OBJECT_TO_JSVAL(resultObj);
      } else {
        *rval = JSVAL_NULL;
      }
      return JS_TRUE;
    }
    return JS_FALSE;
  } // set_by_group

  JSBool set_by_dsmeter(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if((ext != NULL) && (set != NULL) && (argc >= 1)) {
      try {
        std::string dsmeterID = ctx->convertTo<std::string>(argv[0]);
        try {
          dss_dsid_t meterDSID = dss_dsid_t::fromString(dsmeterID);
          Set result = set->getByDSMeter(meterDSID);
          JSObject* resultObj = ext->createJSSet(*ctx, result);
          *rval = OBJECT_TO_JSVAL(resultObj);
          return JS_TRUE;
        } catch(std::invalid_argument&) {
          Logger::getInstance()->log("JS: Could not parse dsid '" + dsmeterID + "'", lsWarning);
        }
      } catch(ScriptException& e) {
        Logger::getInstance()->log(std::string("JS: set_by_dsmeter: ") + e.what(), lsWarning);
      }
    }
    return JS_FALSE;
  } // set_by_dsmeter

  JSBool set_by_presence(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if((ext != NULL) && (set != NULL) && (argc >= 1)) {
      try {
        bool presence = ctx->convertTo<bool>(argv[0]);
        Set result = set->getByPresence(presence);
        JSObject* resultObj = ext->createJSSet(*ctx, result);
        *rval = OBJECT_TO_JSVAL(resultObj);
        return JS_TRUE;
      } catch(ScriptException& e) {
        Logger::getInstance()->log(std::string("JS: set_by_presence: ") + e.what(), lsWarning);
      }
    }
    return JS_FALSE;
  } // set_by_presence

  JSBool set_by_tag(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if((ext != NULL) && (set != NULL) && (argc >= 1)) {
      try {
        std::string tagName = ctx->convertTo<std::string>(argv[0]);
        Set result = set->getByTag(tagName);
        JSObject* resultObj = ext->createJSSet(*ctx, result);
        *rval = OBJECT_TO_JSVAL(resultObj);
        return JS_TRUE;
      } catch(ScriptException& e) {
        Logger::getInstance()->log(std::string("JS: set_by_tag: ") + e.what(), lsWarning);
      }
    }
    return JS_FALSE;
  } // set_by_tag

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
    {"perform", set_perform, 1, 0, 0},
    {"byName", set_by_name, 1, 0, 0},
    {"byDSID", set_by_dsid, 1, 0, 0},
    {"byFunctionID", set_by_functionid, 1, 0, 0},
    {"byZone", set_by_zone, 1, 0, 0},
    {"byDSMeter", set_by_dsmeter, 1, 0, 0},
    {"byGroup", set_by_group, 1, 0, 0},
    {"byPresence", set_by_presence, 1, 0, 0},
    {"byTag", set_by_tag, 1, 0, 0},
    {"byShortAddress", set_by_shortaddress, 2, 0, 0},
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

  JSBool dev_blink(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      intf->blink();
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
      intf->increaseValue();
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
      intf->decreaseValue();
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
      if(argc >= 1) {
        uint8_t value = ctx->convertTo<uint8_t>(argv[0]);
        intf->setValue(value);
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
      if(argc == 1) {
        intf->undoScene();
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_undo_scene

  JSBool dev_next_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 1) {
        intf->nextScene();
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_next_scene

  JSBool dev_previous_scene(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("set") || self.is("device")) {
      IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, obj));
      if(argc == 1) {
        intf->previousScene();
      }
      *rval = INT_TO_JSVAL(0);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dev_previous_scene

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

  JSBool dev_get_sensor_value(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ScriptObject self(obj, *ctx);
    if(self.is("device")) {
      DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, obj));
      if(argc == 1) {
        try {
          int sensorValue = ctx->convertTo<int>(argv[0]);
          int retValue= (intf->getDevice()->getSensorValue(sensorValue));
          *rval = INT_TO_JSVAL(retValue);
        } catch(const BusApiError&) {
          *rval = JSVAL_NULL;
        }
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // dev_get_last_called_scene

  JSFunctionSpec device_interface_methods[] = {
    {"turnOn", dev_turn_on, 0, 0, 0},
    {"turnOff", dev_turn_off, 0, 0, 0},
    {"blink", dev_blink, 0, 0, 0},
    {"setValue", dev_set_value, 0, 0, 0},
    {"increaseValue", dev_increase_value, 0, 0, 0},
    {"decreaseValue", dev_decrease_value, 0, 0, 0},
    {"callScene", dev_call_scene, 1, 0, 0},
    {"saveScene", dev_save_scene, 1, 0, 0},
    {"undoScene", dev_undo_scene, 0, 0, 0},
    {"nextScene", dev_next_scene, 0, 0, 0},
    {"previousScene", dev_previous_scene, 0, 0, 0},
    {"getSensorValue", dev_get_sensor_value, 1, 0, 0},
    {NULL, NULL, 0, 0, 0}
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
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dev->getDevice()->getName().c_str()));
          return JS_TRUE;
        case 3:
          *rval = INT_TO_JSVAL(dev->getDevice()->getZoneID());
          return JS_TRUE;
        case 4:
          {
            std::string tmp = dev->getDevice()->getDSMeterDSID().toString();
            *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp.c_str()));
          }
          return JS_TRUE;
        case 5:
          *rval = INT_TO_JSVAL(dev->getDevice()->getFunctionID());
          return JS_TRUE;
        case 6:
          *rval = INT_TO_JSVAL(dev->getDevice()->getLastCalledScene());
          return JS_TRUE;
        case 7:
          *rval = INT_TO_JSVAL(dev->getDevice()->getShortAddress());
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
    {"lastCalledScene", 6, 0, dev_JSGet},
    {"shortAddress", 7, 0, dev_JSGet},
    {NULL, 0, 0, NULL, NULL}
  };

  JSObject* ModelScriptContextExtension::createJSDevice(ScriptContext& _ctx, boost::shared_ptr<Device> _dev) {
    DeviceReference ref(_dev, &m_Apartment);
    return createJSDevice(_ctx, ref);
  } // createJSDevice

  JSObject* ModelScriptContextExtension::createJSDevice(ScriptContext& _ctx, DeviceReference& _ref) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &dev_class, NULL, NULL);
    JS_DefineFunctions(_ctx.getJSContext(), result, device_interface_methods);
    JS_DefineProperties(_ctx.getJSContext(), result, dev_properties);
    DeviceReference* innerObj = new DeviceReference(_ref.getDSID(), &_ref.getDevice()->getApartment());
    // make an explicit copy
    *innerObj = _ref;
    JS_SetPrivate(_ctx.getJSContext(), result, innerObj);
    return result;
  } // createJSDevice

  struct meter_wrapper {
    boost::shared_ptr<DSMeter> pMeter;
  };

  void finalize_meter(JSContext *cx, JSObject *obj) {
    struct meter_wrapper* pDevice = static_cast<meter_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pDevice;
  } // finalize_meter

  static JSClass dsmeter_class = {
    "DSMeter", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub, finalize_meter, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSBool dsmeter_JSGet(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, obj))->pMeter;

    if(meter != NULL) {
      int opt = JSVAL_TO_INT(id);
      switch(opt) {
        case 0:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "dsmeter"));
          return JS_TRUE;
        case 1:
          {
            // make a local reference so the std::string does not go out of scope
            std::string tmp = meter->getDSID().toString();
            *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp.c_str()));
          }
          return JS_TRUE;
        case 2:
          *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, meter->getName().c_str()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  static JSPropertySpec dsmeter_properties[] = {
    {"className", 0, 0, dsmeter_JSGet},
    {"dsid", 1, 0, dsmeter_JSGet},
    {"name", 2, 0, dsmeter_JSGet},
    {NULL, 0, 0, NULL, NULL}
  };

  JSBool dsmeter_getPowerConsumption(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, obj))->pMeter;

    if(meter != NULL) {
      *rval = INT_TO_JSVAL(meter->getPowerConsumption());
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dsmeter_getPowerConsumption

  JSBool dsmeter_getCachedPowerConsumption(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, obj))->pMeter;

    if(meter != NULL) {
      JSAutoLocalRootScope localRoot(cx);
      ScriptObject obj(*ctx, NULL);
      *rval = OBJECT_TO_JSVAL(obj.getJSObject());
      obj.setProperty<std::string>("timestamp", meter->getCachedPowerConsumptionTimeStamp().toString());
      obj.setProperty<int>("value", meter->getCachedPowerConsumption());
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dsmeter_getCachedPowerConsumption

  JSBool dsmeter_getEnergyMeterValue(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, obj))->pMeter;

    if(meter != NULL) {
      *rval = INT_TO_JSVAL(meter->getEnergyMeterValue());
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dsmeter_getEnergyMeterValue

  JSBool dsmeter_getCachedEnergyMeterValue(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, obj))->pMeter;

    if(meter != NULL) {
      JSAutoLocalRootScope localRoot(cx);
      ScriptObject obj(*ctx, NULL);
      *rval = OBJECT_TO_JSVAL(obj.getJSObject());
      obj.setProperty<std::string>("timestamp", meter->getCachedEnergyMeterTimeStamp().toString());
      obj.setProperty<int>("value", meter->getCachedEnergyMeterValue());
      return JS_TRUE;
    }
    return JS_FALSE;
  } // dsmeter_getCachedEnergyMeterValue

  JSFunctionSpec dsmeter_methods[] = {
    {"getPowerConsumption", dsmeter_getPowerConsumption, 0, 0, 0},
    {"getEnergyMeterValue", dsmeter_getEnergyMeterValue, 0, 0, 0},
    {"getCachedPowerConsumption", dsmeter_getCachedPowerConsumption, 0, 0, 0},
    {"getCachedEnergyMeterValue", dsmeter_getCachedEnergyMeterValue, 0, 0, 0},
    {NULL, NULL, 0, 0, 0}
  };

  JSObject* ModelScriptContextExtension::createJSMeter(ScriptContext& _ctx, boost::shared_ptr<DSMeter> _pMeter) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &dsmeter_class, NULL, NULL);
    JS_DefineProperties(_ctx.getJSContext(), result, dsmeter_properties);
    JS_DefineFunctions(_ctx.getJSContext(), result, dsmeter_methods);
    struct meter_wrapper* wrapper = new meter_wrapper;
    wrapper->pMeter = _pMeter;
    JS_SetPrivate(_ctx.getJSContext(), result, wrapper);
    return result;
  } // createJSMeter

  //================================================== EventScriptExtension

  const std::string EventScriptExtensionName = "eventextension";

  EventScriptExtension::EventScriptExtension(EventQueue& _queue, EventInterpreter& _interpreter)
  : ScriptExtension(EventScriptExtensionName),
    m_Queue(_queue),
    m_Interpreter(_interpreter)
  { } // ctor

  struct event_wrapper {
    boost::shared_ptr<Event> event;
  };

  void readEventPropertiesFrom(JSContext *cx, jsval _props, boost::shared_ptr<Event> _pEvent) {
    JSObject* paramObj;
    if(JS_ValueToObject(cx, _props, &paramObj) == JS_TRUE) {
      JSObject* propIter = JS_NewPropertyIterator(cx, paramObj);
      jsid propID;
      while(JS_NextProperty(cx, propIter, &propID) == JS_TRUE) {
        if(propID == JSVAL_VOID) {
          break;
        }
        JSObject* obj;
        jsval vp;
        JS_GetMethodById(cx, paramObj, propID, &obj, &vp);

        JSString *val = JS_ValueToString(cx, vp);
        char* propValue = JS_GetStringBytes(val);

        jsval vp2;
        JS_IdToValue(cx, propID, &vp2);
        val = JS_ValueToString(cx, vp2);
        char* propName = JS_GetStringBytes(val);

        _pEvent->setProperty(propName, propValue);
      }
    }
  }

  JSBool event_construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc < 1) {
      Logger::getInstance()->log("JS: event_construct: (empty name)", lsError);
      return JS_FALSE;
    }

    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    JSAutoLocalRootScope localRoot(cx);

    try {
      std::string name = ctx->convertTo<std::string>(argv[0]);

      boost::shared_ptr<Event> newEvent(new Event(name));

      if(argc >= 2) {
        readEventPropertiesFrom(cx, argv[1], newEvent);
      }

      event_wrapper* evtWrapper = new event_wrapper();
      evtWrapper->event = newEvent;
      JS_SetPrivate(cx, obj, evtWrapper);
      return JS_TRUE;
    } catch(ScriptException& e) {
      Logger::getInstance()->log(std::string("JS: event_construct: error converting string: ") + e.what(), lsError);
    }
    return JS_FALSE;
  } // event_construct

  JSBool timedEvent_construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc < 2) {
      Logger::getInstance()->log("JS: timedEvent_construct: empty name, no time", lsError);
      return JS_FALSE;
    }

    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    JSAutoLocalRootScope localRoot(cx);

    try {
      std::string name = ctx->convertTo<std::string>(argv[0]);

      if(name.empty()) {
        Logger::getInstance()->log("JS: timedEvent_construct: empty name not allowed", lsError);
        return JS_FALSE;
      }
      std::string time = ctx->convertTo<std::string>(argv[1]);
      if(time.empty()) {
        Logger::getInstance()->log("JS: timedEvent_construct: empty time not allowed", lsError);
      }

      boost::shared_ptr<Event> newEvent(new Event(name));

      if(argc >= 3) {
        readEventPropertiesFrom(cx, argv[2], newEvent);
      }

      newEvent->setProperty(EventPropertyTime, time);

      event_wrapper* evtWrapper = new event_wrapper();
      evtWrapper->event = newEvent;
      JS_SetPrivate(cx, obj, evtWrapper);
      return JS_TRUE;
    } catch(ScriptException& e) {
      Logger::getInstance()->log(std::string("JS: event_construct: error converting string: ") + e.what(), lsError);
    }
    return JS_FALSE;
  } // timedEvent_construct

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

  JSBool timedEvent_raise(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(obj, *ctx);
    EventScriptExtension* ext = dynamic_cast<EventScriptExtension*>(ctx->getEnvironment().getExtension(EventScriptExtensionName));
    if(self.is("event")) {
      event_wrapper* eventWrapper = static_cast<event_wrapper*>(JS_GetPrivate(cx, obj));
      std::string id = ext->getEventQueue().pushTimedEvent(eventWrapper->event);
      *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, id.c_str()));
      return JS_TRUE;
    }
    return JS_FALSE;
  } // timedEvent_raise

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
    "Event", JSCLASS_HAS_PRIVATE,
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

  static JSClass timedEvent_class = {
    "TimedEvent", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_event, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSFunctionSpec timedEvent_methods[] = {
    {"raise", timedEvent_raise, 1, 0, 0},
    {NULL}
  };

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
    "Subscription", JSCLASS_HAS_PRIVATE,
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

  JSBool subscription_construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc < 3) {
      Logger::getInstance()->log("JS: subscription_construct: need three arguments: event-name, handler-name & parameter");
      return JS_FALSE;
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
      JSAutoLocalRootScope localRoot(cx);

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
        subscription_wrapper* subscriptionWrapper = new subscription_wrapper();
        subscriptionWrapper->subscription = subscription;
        JS_SetPrivate(cx, obj, subscriptionWrapper);
      }
    }
    return JS_TRUE;
  } // global_subscription

  void EventScriptExtension::extendContext(ScriptContext& _context) {
    JSRequest req(&_context);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
              NULL, &subscription_class, subscription_construct, 0, subscription_properties,
              subscription_methods, NULL, NULL);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
              NULL, &event_class, event_construct, 0, event_properties,
              event_methods, NULL, NULL);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
              NULL, &timedEvent_class, timedEvent_construct, 0, event_properties,
              timedEvent_methods, NULL, NULL);
  } // extendContext

  //================================================== ModelConstantsScriptExtension

  const std::string ModelConstantsScriptExtensionName = "modelconstantsextension";

  ModelConstantsScriptExtension::ModelConstantsScriptExtension()
  : ScriptExtension(ModelConstantsScriptExtensionName)
  { }

  static JSClass scene_class = {
    "Scene", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // scene_class

  JSBool scene_JSGetStatic(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    *rval = id;
    return JS_TRUE;
  } // scene_JSGetStatic

  static JSPropertySpec scene_properties[] = {
    {"User1", Scene1, 0, scene_JSGetStatic},
    {"User2", Scene2, 0, scene_JSGetStatic},
    {"User3", Scene3, 0, scene_JSGetStatic},
    {"User4", Scene4, 0, scene_JSGetStatic},
    {"Min", SceneMin, 0, scene_JSGetStatic},
    {"Max", SceneMax, 0, scene_JSGetStatic},
    {"DeepOff", SceneDeepOff, 0, scene_JSGetStatic},
    {"Off", SceneOff, 0, scene_JSGetStatic},
    {"StandBy", SceneStandBy, 0, scene_JSGetStatic},
    {"Bell", SceneBell, 0, scene_JSGetStatic},
    {"Panic", ScenePanic, 0, scene_JSGetStatic},
    {"Alarm", SceneAlarm, 0, scene_JSGetStatic},
    {"Inc", SceneInc, 0, scene_JSGetStatic},
    {"Dec", SceneDec, 0, scene_JSGetStatic},
    {"Present", ScenePresent, 0, scene_JSGetStatic},
    {"Absent", SceneAbsent, 0, scene_JSGetStatic},
    {"Sleeping", SceneSleeping, 0, scene_JSGetStatic},
    {"WakeUp", SceneWakeUp, 0, scene_JSGetStatic},
    {NULL, 0, 0, NULL, NULL}
  };

  JSBool scene_construct(JSContext *cx, JSObject *obj, uintN argc,
                        jsval *argv, jsval *rval) {
    return JS_FALSE;
  } // scene_construct

  void ModelConstantsScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), NULL,
                 &scene_class, &scene_construct, 0, NULL, NULL, scene_properties, NULL);
  } // extendedJSContext

  //================================================== MeteringScriptExtension

  const std::string MeteringScriptExtensionName = "meteringextension";

  MeteringScriptExtension::MeteringScriptExtension(Apartment& _apartment,
                                                   Metering& _metering)
  : ScriptExtension(MeteringScriptExtensionName),
    m_Apartment(_apartment),
    m_Metering(_metering)
  { }

  static JSClass metering_class = {
    "Metering", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // metering_class

  JSBool metering_construct(JSContext *cx, JSObject *obj, uintN argc,
                        jsval *argv, jsval *rval) {
    return JS_FALSE;
  } // metering_construct

  JSBool metering_getSeries(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    MeteringScriptExtension* ext = dynamic_cast<MeteringScriptExtension*>(ctx->getEnvironment().getExtension(MeteringScriptExtensionName));
    JSAutoLocalRootScope scope(cx);
    JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
    *rval = OBJECT_TO_JSVAL(resultObj);

    int iMeter = 0;
    std::vector<boost::shared_ptr<DSMeter> > dsMeters = ext->getApartment().getDSMeters();
    foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
      ScriptObject objEnergy(*ctx, NULL);
      objEnergy.setProperty<std::string>("dsid", dsMeter->getDSID().toString());
      objEnergy.setProperty<std::string>("type", "energy");
      jsval childJSVal = OBJECT_TO_JSVAL(objEnergy.getJSObject());
      JSBool res = JS_SetElement(cx, resultObj, iMeter, &childJSVal);
      if(!res) {
        return JS_FALSE;
      }
      iMeter++;
      ScriptObject objConsumption(*ctx, NULL);
      objConsumption.setProperty<std::string>("dsid", dsMeter->getDSID().toString());
      objConsumption.setProperty<std::string>("type", "consumption");
      childJSVal = OBJECT_TO_JSVAL(objConsumption.getJSObject());
      res = JS_SetElement(cx, resultObj, iMeter, &childJSVal);
      if(!res) {
        return JS_FALSE;
      }
    }

    return JS_TRUE;
  } // metering_getSeries

  JSBool metering_getResolutions(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    MeteringScriptExtension* ext = dynamic_cast<MeteringScriptExtension*>(ctx->getEnvironment().getExtension(MeteringScriptExtensionName));
    JSAutoLocalRootScope scope(cx);
    JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
    *rval = OBJECT_TO_JSVAL(resultObj);


    std::vector<boost::shared_ptr<MeteringConfigChain> >
      meteringConfig = ext->getMetering().getConfig();
    int iResolution = 0;
    foreach(boost::shared_ptr<MeteringConfigChain> pChain, meteringConfig) {
      for(int iConfig = 0; iConfig < pChain->size(); iConfig++) {
        ScriptObject resolution(*ctx, NULL);

        resolution.setProperty<std::string>("type", pChain->isEnergy() ? "energy" : "consumption");
        resolution.setProperty<std::string>("unit", pChain->getUnit());
        resolution.setProperty<int>("resolution", pChain->getResolution(iConfig));
        jsval childJSVal = OBJECT_TO_JSVAL(resolution.getJSObject());
        JSBool res = JS_SetElement(cx, resultObj, iResolution, &childJSVal);
        if(!res) {
          return JS_FALSE;
        }
        iResolution++;
      }
    }

    return JS_TRUE;
  } // metering_getResolutions

  JSBool metering_getValues(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    MeteringScriptExtension* ext = dynamic_cast<MeteringScriptExtension*>(ctx->getEnvironment().getExtension(MeteringScriptExtensionName));
    JSAutoLocalRootScope scope(cx);

    if(argc < 3) {
      Logger::getInstance()->log("JS: metering_getValues: need three parameters: (dsid, type, resolution)", lsError);
      return JS_FALSE;
    }

    try {
      std::string dsid = ctx->convertTo<std::string>(argv[0]);
      std::string type = ctx->convertTo<std::string>(argv[1]);
      int resolution = ctx->convertTo<int>(argv[2]);


      boost::shared_ptr<DSMeter> pMeter;
      try {
        dss_dsid_t deviceDSID = dss_dsid_t::fromString(dsid);
        pMeter = ext->getApartment().getDSMeterByDSID(deviceDSID);
      } catch(std::runtime_error& e) {
        Logger::getInstance()->log("Error getting device '" + dsid + "'", lsWarning);
        *rval = JSVAL_NULL;
        return JS_TRUE;
      }
      bool energy;
      if(type == "consumption") {
        energy = false;
      } else if(type == "energy") {
        energy = true;
      } else {
        Logger::getInstance()->log("JS: metering_getValues: Invalid type '" +
                                   type + "'", lsError);
        return JS_FALSE;
      }
      std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = ext->getMetering().getConfig();
      boost::shared_ptr<Series<CurrentValue> > pSeries;
      foreach(boost::shared_ptr<MeteringConfigChain> pChain, meteringConfig) {
        if(pChain->isEnergy() != energy) {
          continue;
        }
        for(int iConfig = 0; iConfig < pChain->size(); iConfig++) {
          if(pChain->getResolution(iConfig) == resolution) {
            pSeries = ext->getMetering().getSeries(pChain, iConfig, pMeter);
            break;
          }
        }
      }
      if(pSeries != NULL) {
        boost::shared_ptr<std::deque<CurrentValue> > values = pSeries->getExpandedValues();

        int valueNumber = 0;
        JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
        *rval = OBJECT_TO_JSVAL(resultObj);
        for(std::deque<CurrentValue>::iterator iValue = values->begin(),
            e = values->end();
            iValue != e;
            ++iValue)
        {
          ScriptObject valuePair(*ctx, NULL);
          DateTime tmp_date = iValue->getTimeStamp();
          valuePair.setProperty<std::string>("timestamp", tmp_date.toString());
          valuePair.setProperty<int>("value", iValue->getValue());
          jsval childJSVal = OBJECT_TO_JSVAL(valuePair.getJSObject());
          JSBool res = JS_SetElement(cx, resultObj, valueNumber, &childJSVal);
          if(!res) {
            return JS_FALSE;
          }
          valueNumber++;
        }

      } else {
        Logger::getInstance()->log("JS: metering_getValues: Could not find data for '" + type +
                                   "' and resolution '" + intToString(resolution) + "'", lsWarning);
      }
      return JS_TRUE;
    } catch(ScriptException& e) {
      Logger::getInstance()->log(std::string("JS: metering_getValues: error converting value: ") + e.what(), lsError);
    }
    return JS_FALSE;
  } // metering_getValues


  JSFunctionSpec metering_static_methods[] = {
    {"getSeries", metering_getSeries, 0, 0, 0},
    {"getResolutions", metering_getResolutions, 0, 0, 0},
    {"getValues", metering_getValues, 3, 0, 0},
    {NULL}
  };

  void MeteringScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), NULL,
                 &metering_class, &metering_construct, 0, NULL, NULL, NULL, metering_static_methods);
  } // extendedJSContext

} // namespace
