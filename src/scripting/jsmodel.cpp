/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>,
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#include "jsmodel.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

#include "src/dss.h"
#include "src/logger.h"
#include "src/businterface.h"
#include "src/structuremanipulator.h"
#include "src/foreach.h"
#include "src/model/device.h"
#include "src/model/devicereference.h"
#include "src/model/apartment.h"
#include "src/model/set.h"
#include "src/model/modulator.h"
#include "src/model/modelconst.h"
#include "src/model/zone.h"
#include "src/metering/metering.h"
#include "src/scripting/scriptobject.h"
#include "src/scripting/jsproperty.h"
#include "src/security/security.h"

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
    if(obj.is("Set")) {
      return *static_cast<Set*>(JS_GetPrivate(_context.getJSContext(), _obj));
    }
    throw ScriptException("Wrong classname for Set");
  } // convertTo<Set>

  template<>
  Set& ModelScriptContextExtension::convertTo(ScriptContext& _context, jsval _val) {
    if(JSVAL_IS_OBJECT(_val)) {
      return convertTo<Set&>(_context, JSVAL_TO_OBJECT(_val));
    }
    throw ScriptException("JSVal is no object");
  } // convertTo<Set>

  JSBool global_get_name(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      std::string aptName = ext->getApartment().getName();
      JSString* str = JS_NewStringCopyN(cx, aptName.c_str(), aptName.size());
      JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(str));
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_get_name

  JSBool global_set_name(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if(ext != NULL && argc >= 1) {
        std::string aptName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        ext->getApartment().setName(aptName);
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
        return JS_TRUE;
      }
    } catch (ScriptException& ex) {
      JS_ReportError(cx, ex.what());
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // global_set_name

  JSBool global_get_devices(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

      if(ext != NULL) {
        Set devices = ext->getApartment().getDevices();
        JSObject* obj = ext->createJSSet(*ctx, devices);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
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
  } // global_get_devices

  JSBool global_log(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    if (argc < 1) {
      JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0)); /* Send back a return value of 0. */
      Logger::getInstance()->log("JS: global_log: (empty std::string)");
    } else {
      std::stringstream sstr;
      unsigned int i;
      for (i=0; i<argc; i++){
        std::string line;
        try {
          line = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[i]);
        } catch(ScriptException& ex) {
          JS_ReportError(cx, ex.what());
        }
        sstr << line;
      }
      Logger::getInstance()->log(sstr.str());
      JS_SET_RVAL(cx, vp, INT_TO_JSVAL(sstr.str().size())); /* Set the return value to be the number of bytes/chars written */
    }
    return JS_TRUE;
  } // global_log

  JSBool global_get_dsmeterbydsid(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

      if(ext != NULL) {
        if(argc >= 1) {
          dss_dsid_t dsid = NullDSID;
          try {
            dsid =  dss_dsid_t::fromString(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          } catch(ScriptException& e) {
            JS_ReportError(cx, "Error converting dsid string to dsid");
            return JS_FALSE;
          } catch(std::invalid_argument& e) {
            JS_ReportError(cx, "Error converting dsid string to dsid");
            return JS_FALSE;
          }
          try {
            boost::shared_ptr<DSMeter> meter = ext->getApartment().getDSMeterByDSID(dsid);
            JSObject* obj = ext->createJSMeter(*ctx, meter);
            JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
          } catch(ItemNotFoundException& e) {
            JS_SET_RVAL(cx, vp, JSVAL_NULL);
          }
          return JS_TRUE;
        }
      }
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // global_get_dsmeterbydsid


  JSBool global_getDSMeters(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

      JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));

      std::vector<boost::shared_ptr<DSMeter> > meters = ext->getApartment().getDSMeters();
      for(std::size_t iMeter = 0; iMeter < meters.size(); iMeter++) {
        JSObject* meterObj = ext->createJSMeter(*ctx, meters[iMeter]);
        jsval meterJSVal = OBJECT_TO_JSVAL(meterObj);
        JSBool res = JS_SetElement(cx, resultObj, iMeter, &meterJSVal);
        if(!res) {
          return JS_FALSE;
        }
      }
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_TRUE;
  } // global_getDSMeters

  JSBool global_getZones(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

      JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));

      std::vector<boost::shared_ptr<Zone> > zones = ext->getApartment().getZones();
      for(std::size_t i = 0; i < zones.size(); i++) {
        JSObject* zoneObj = ext->createJSZone(*ctx, zones[i]);
        jsval zoneJSVal = OBJECT_TO_JSVAL(zoneObj);
        JSBool res = JS_SetElement(cx, resultObj, i, &zoneJSVal);
        if(!res) {
          return JS_FALSE;
        }
      }
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_TRUE;
  } // global_getZones

  JSBool global_getZoneByID(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

      int zoneID;
      try {
        zoneID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting zone ID string to number");
        return JS_FALSE;
      }
      try {
        boost::shared_ptr<Zone> zone = ext->getApartment().getZone(zoneID);
        JSObject* obj = ext->createJSZone(*ctx, zone);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
        return JS_TRUE;
      } catch(ItemNotFoundException& e) {
        JS_ReportError(cx, "Zone with ID not found");
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
        return JS_FALSE;
      }
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // global_getZoneByID

  JSBool global_getEnergyMeterValue(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

      if(ext != NULL) {
        uint32_t result = 0;
        foreach(boost::shared_ptr<DSMeter> pDSMeter, ext->getApartment().getDSMeters()) {
          try {
            result += pDSMeter->getEnergyMeterValue();
          } catch(BusApiError& e) {
            Logger::getInstance()->log(std::string("Error requesting energy meter value from dSM: ") + e.what(), lsError);
          }
        }
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(result));
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
  } // global_getEnergyMeterValue

  JSBool global_getConsumption(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

      if(ext != NULL) {
        uint32_t result = 0;
        foreach(boost::shared_ptr<DSMeter> pDSMeter, ext->getApartment().getDSMeters()) {
          try {
            result += pDSMeter->getPowerConsumption();
          } catch(BusApiError& e) {
            Logger::getInstance()->log(std::string("Error requesting energy consumption from dSM: ") + e.what(), lsError);
          }
        }
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(result));
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
  } // global_getConsumption

  JSFunctionSpec apartment_static_methods[] = {
    JS_FS("getName", global_get_name, 0, 0),
    JS_FS("setName", global_set_name, 1, 0),
    JS_FS("getDevices", global_get_devices, 0, 0),
    JS_FS("getDSMeterByDSID", global_get_dsmeterbydsid, 1, 0),
    JS_FS("getConsumption", global_getConsumption, 0, 0),
    JS_FS("getEnergyMeterValue", global_getEnergyMeterValue, 0, 0),
    JS_FS("getDSMeters", global_getDSMeters, 0, 0),
    JS_FS("getZones", global_getZones, 0, 0),
    JS_FS("getZoneByID", global_getZoneByID, 0, 0),
    JS_FS_END
  };

  static JSClass apartment_class = {
    "Apartment", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // apartment_class

  JSFunctionSpec model_global_methods[] = {
    JS_FS("log", global_log, 1, 0),
    JS_FS_END
  };

  JSBool apartment_construct(JSContext *cx, uintN argc, jsval *vp) {
    JSObject *obj = JS_NewObject(cx, &apartment_class, NULL, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
    return JS_TRUE;
  } // apartment_construct

  void ModelScriptContextExtension::extendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.getJSContext(),
        JS_GetGlobalObject(_context.getJSContext()), model_global_methods);
    JS_DefineFunctions(_context.getJSContext(),
        JS_GetGlobalObject(_context.getJSContext()), apartment_static_methods);
    JS_InitClass(_context.getJSContext(),
        JS_GetGlobalObject(_context.getJSContext()),
        NULL,
        &apartment_class,
        &apartment_construct,
        0,
        NULL,
        NULL,
        NULL,
        apartment_static_methods);
  } // extendedJSContext

  void finalize_set(JSContext *cx, JSObject *obj) {
    Set* pSet = static_cast<Set*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pSet;
  } // finalize_set

  JSBool set_length(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));

    if(ext != NULL && set != NULL) {
      JS_SET_RVAL(cx, vp, INT_TO_JSVAL(set->length()));
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool set_combine(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      Set result;
      try {
        Set& otherSet = ext->convertTo<Set&>(*ctx, JS_ARGV(cx, vp)[0]);
        result = set->combine(otherSet);
      } catch (ScriptException& ex) {
        JS_ReportError(cx, ex.what());
        return JS_FALSE;
      }
      JSObject* resultObj = ext->createJSSet(*ctx, result);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
      return JS_TRUE;
    }
    return JS_FALSE;
  }

  JSBool set_remove(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(ext != NULL && set != NULL && argc >= 1) {
      Set result;
      try {
        Set& otherSet = ext->convertTo<Set&>(*ctx, JS_ARGV(cx, vp)[0]);
        result = set->remove(otherSet);
      } catch(ScriptException& ex) {
        JS_ReportError(cx, ex.what());
        return JS_FALSE;
      }
      JSObject* resultObj = ext->createJSSet(*ctx, result);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
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
        return JS_CallFunctionValue(m_Context.getJSContext(),
            JS_GetGlobalObject(m_Context.getJSContext()),
            m_Function,
            1,
            &dev,
            &rval);
      }
  };

  JSBool set_perform(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if(self.is("Set")) {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      if(argc == 1) {

        JSObject* jsFunction = JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[0]);
        if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "o", &jsFunction)) {
          JS_ReportError(cx, "perform(): invalid parameter");
          return JS_FALSE;
        }
        if (!JS_ObjectIsCallable(cx, jsFunction)) {
          JS_ReportError(cx, "perform(): handler is not a function");
          return JS_FALSE;
        }
        jsval functionVal = OBJECT_TO_JSVAL(jsFunction);
        JSDeviceAction act = JSDeviceAction(functionVal, *ctx, *ext);

        try {
          set->perform(act);
          if (JS_IsExceptionPending(cx)) {
            return JS_FALSE;
          }
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (SecurityException& ex) {
          JS_ReportError(cx, "Access denied: %s", ex.what());
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General failure: %s", ex.what());
        }
      }
    }
    return JS_FALSE;
  } // set_perform

  JSBool set_by_name(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if(ext != NULL && set != NULL && argc >= 1) {
        std::string name;
        try {
          name = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        } catch (ScriptException& ex) {
          JS_ReportError(cx, "Convert arguments: %s", ex.what());
          return JS_FALSE;
        }
        DeviceReference result = set->getByName(name);
        JSObject* resultObj = ext->createJSDevice(*ctx, result);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_name

  JSBool set_by_shortaddress(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if(ext != NULL && set != NULL && argc >= 2) {

        std::string dsmeterID;
        int busid;
        try {
          dsmeterID = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
          busid = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
        } catch(ScriptException& ex) {
          JS_ReportError(cx, "Convert argument: %s", ex.what());
          return JS_FALSE;
        }

        dss_dsid_t meterDSID;
        try {
          meterDSID = dss_dsid_t::fromString(dsmeterID);
        } catch(std::invalid_argument& ex) {
          JS_ReportError(cx, "Convert dsid parameter: %s", ex.what());
          return JS_FALSE;
        }

        DeviceReference result = set->getByBusID(busid, meterDSID);
        JSObject* resultObj = ext->createJSDevice(*ctx, result);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_shortaddress


  JSBool set_by_dsid(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if(ext != NULL && set != NULL && argc >= 1) {
        try {
          std::string dsid = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
          DeviceReference result = set->getByDSID(dss_dsid_t::fromString(dsid));
          JSObject* resultObj = ext->createJSDevice(*ctx, result);
          JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
          return JS_TRUE;
        } catch(ScriptException& ex) {
          JS_ReportError(cx, ex.what());
        } catch(std::invalid_argument&) {
          JS_ReportError(cx, "set.byDSID: Could not parse dsid");
        } catch(ItemNotFoundException&) {
          JS_ReportError(cx, "set.byDSID: Device with dsid not found");
        }
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
        return JS_FALSE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_dsid

  JSBool set_by_functionid(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if(ext != NULL && set != NULL && argc >= 1) {
        int32_t fid;
        try {
          fid = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
        } catch(ScriptException& ex) {
          JS_ReportError(cx, ex.what());
          return JS_FALSE;
        }
        Set result = set->getByFunctionID(fid);
        JSObject* resultObj = ext->createJSSet(*ctx, result);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_functionid

  JSBool set_by_zone(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if((ext != NULL) && (set != NULL) && (argc >= 1)) {
        Set result;
        try {
          if(JSVAL_IS_INT(JS_ARGV(cx, vp)[0])) {
            result = set->getByZone(JSVAL_TO_INT(JS_ARGV(cx, vp)[0]));
          } else {
            std::string zonename = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
            result = set->getByZone(zonename);
          }
        } catch(ScriptException& ex) {
          JS_ReportError(cx, "Convert argument: %s", ex.what());
          return JS_FALSE;
        } catch(ItemNotFoundException&) {
          // return an empty set if the zone hasn't been found
          JS_ReportWarning(cx, "set_by_zone: Zone not found");
        }
        JSObject* resultObj = ext->createJSSet(*ctx, result);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_zone

  JSBool set_by_group(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if((ext != NULL) && (set != NULL) && (argc >= 1)) {
        bool ok = false;
        Set result;
        try {
          if(JSVAL_IS_INT(JS_ARGV(cx, vp)[0])) {
            result = set->getByGroup(JSVAL_TO_INT(JS_ARGV(cx, vp)[0]));
            ok = true;
          } else {
            std::string groupname = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
            result = set->getByGroup(groupname);
            ok = true;
          }
        } catch(ScriptException& ex) {
          JS_ReportError(cx, "Convert argument: %s", ex.what());
          return JS_FALSE;
        } catch(ItemNotFoundException&) {
          ok = true; // return an empty set if the group hasn't been found
          JS_ReportWarning(cx, "set_by_group: Group not found");
        }
        if(ok) {
          JSObject* resultObj = ext->createJSSet(*ctx, result);
          JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
        } else {
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
        }
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_group

  JSBool set_by_dsmeter(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if((ext != NULL) && (set != NULL) && (argc >= 1)) {
        try {
          std::string dsmeterID = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
          dss_dsid_t meterDSID = dss_dsid_t::fromString(dsmeterID);
          Set result = set->getByDSMeter(meterDSID);
          JSObject* resultObj = ext->createJSSet(*ctx, result);
          JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
          return JS_TRUE;
        } catch(std::invalid_argument&) {
          JS_ReportError(cx, "Could not parse dsid");
        } catch(ScriptException& ex) {
          JS_ReportError(cx, ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_dsmeter

  JSBool set_by_presence(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if((ext != NULL) && (set != NULL) && (argc >= 1)) {
        try {
          bool presence = ctx->convertTo<bool>(JS_ARGV(cx, vp)[0]);
          Set result = set->getByPresence(presence);
          JSObject* resultObj = ext->createJSSet(*ctx, result);
          JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
          return JS_TRUE;
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_presence

  JSBool set_by_tag(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      Set* set = static_cast<Set*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if((ext != NULL) && (set != NULL) && (argc >= 1)) {
        try {
          std::string tagName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
          Set result = set->getByTag(tagName);
          JSObject* resultObj = ext->createJSSet(*ctx, result);
          JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
          return JS_TRUE;
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // set_by_tag

  JSBool set_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    Set* set = static_cast<Set*>(JS_GetPrivate(cx, obj));

    if(set != NULL) {
      int opt = JSID_TO_INT(id);
      if(opt == 0) {
        JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Set")));
        return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  static JSClass set_class = {
    "Set", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_set, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSFunctionSpec set_methods[] = {
    JS_FS("length", set_length, 0, 0),
    JS_FS("combine", set_combine, 1, 0),
    JS_FS("remove", set_remove, 1, 0),
    JS_FS("perform", set_perform, 1, 0),
    JS_FS("byName", set_by_name, 1, 0),
    JS_FS("byDSID", set_by_dsid, 1, 0),
    JS_FS("byFunctionID", set_by_functionid, 1, 0),
    JS_FS("byZone", set_by_zone, 1, 0),
    JS_FS("byDSMeter", set_by_dsmeter, 1, 0),
    JS_FS("byGroup", set_by_group, 1, 0),
    JS_FS("byPresence", set_by_presence, 1, 0),
    JS_FS("byTag", set_by_tag, 1, 0),
    JS_FS("byShortAddress", set_by_shortaddress, 2, 0),
    JS_FS_END
  };

  static JSPropertySpec set_properties[] = {
    {"className", 0, 0, set_JSGet, NULL},
    {NULL}
  };

  JSBool dev_turn_on(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          intf->turnOn();
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  }

  JSBool dev_turn_off(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          intf->turnOff();
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  }

  JSBool dev_blink(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          intf->blink();
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  }

  JSBool dev_increase_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          intf->increaseValue();
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  }

  JSBool dev_decrease_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));

        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          intf->decreaseValue();
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  }

  JSBool dev_set_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        if(argc >= 1) {
          uint8_t value;
          try {
            value = ctx->convertTo<uint8_t>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            intf->setValue(value);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
            return JS_TRUE;
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_set_value

  JSBool dev_call_scene(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        if(argc == 1) {
          int sceneNr;
          try {
            sceneNr = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert parameter scene number: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            intf->callScene(sceneNr);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
            return JS_TRUE;
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_call_scene

  JSBool dev_undo_scene(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        int sceneNr = -1;
        if(argc > 1) {
          try {
            sceneNr = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert parameter scene number: %s", ex.what());
            return JS_FALSE;
          }
        }
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          if (sceneNr >= 0) {
            intf->undoScene(sceneNr);
          } else {
            intf->undoSceneLast();
          }
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_undo_scene

  JSBool dev_next_scene(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          intf->nextScene();
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_next_scene

  JSBool dev_previous_scene(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          intf->previousScene();
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
        JS_ResumeRequest(cx, ref);
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_previous_scene

  JSBool dev_save_scene(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        int sceneNr;
        if(argc == 1) {

          try {
            sceneNr = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert parameter scene number: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            intf->saveScene(sceneNr);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
            return JS_TRUE;
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_save_scene

  JSBool dev_get_config(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc == 2) {
          int configClass;
          int configIndex;
          try {
            configClass = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
            configIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert arguments: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            uint8_t retValue= pDev->getDeviceConfig(configClass, configIndex);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(retValue));
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_config

  JSBool dev_get_config_word(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc == 2) {
          int configClass;
          int configIndex;
          try {
            configClass = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
            configIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert arguments: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            uint16_t retValue= pDev->getDeviceConfigWord(configClass, configIndex);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(retValue));
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_config_word

  JSBool dev_set_config(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc == 3) {
          int configClass;
          int configIndex;
          int configValue;
          try {
            configClass = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
            configIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
            configValue = ctx->convertTo<int>(JS_ARGV(cx, vp)[2]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert arguments: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            pDev->setDeviceConfig(configClass, configIndex, configValue);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_config

  JSBool dev_get_output_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc == 1) {
          uint8_t offset;
          try {
            offset = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert arguments: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            uint16_t result = pDev->getDeviceOutputValue(offset);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(result));
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_output_value

  JSBool dev_set_output_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc == 1) {
          uint8_t offset;
          uint16_t value;
          try {
            offset = ctx->convertTo<uint8_t>(JS_ARGV(cx, vp)[0]);
            value = ctx->convertTo<uint16_t>(JS_ARGV(cx, vp)[1]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            pDev->setDeviceOutputValue(offset, value);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_set_output_value

  JSBool dev_get_sensor_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc == 1) {
          int sensorIndex;
          try {
            sensorIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert argument: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            int retValue= pDev->getDeviceSensorValue(sensorIndex);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(retValue));
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_sensor_value

  JSBool dev_get_sensor_type(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc == 1) {
          int sensorIndex;
          try {
            sensorIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert argument: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            int retValue = pDev->getDeviceSensorType(sensorIndex);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(retValue));
            JS_ResumeRequest(cx, ref);
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
          JS_ResumeRequest(cx, ref);
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_sensor_type

  JSBool dev_get_property_node(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension("propertyextension"));
    DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, intf->getDevice()->getPropertyNode())));
    return JS_TRUE;
  } // dev_get_property_node

  JSFunctionSpec device_interface_methods[] = {
    JS_FS("turnOn", dev_turn_on, 0, 0),
    JS_FS("turnOff", dev_turn_off, 0, 0),
    JS_FS("blink", dev_blink, 0, 0),
    JS_FS("setValue", dev_set_value, 0, 0),
    JS_FS("increaseValue", dev_increase_value, 0, 0),
    JS_FS("decreaseValue", dev_decrease_value, 0, 0),
    JS_FS("callScene", dev_call_scene, 1, 0),
    JS_FS("saveScene", dev_save_scene, 1, 0),
    JS_FS("undoScene", dev_undo_scene, 0, 0),
    JS_FS("nextScene", dev_next_scene, 0, 0),
    JS_FS("previousScene", dev_previous_scene, 0, 0),
    JS_FS("getConfig", dev_get_config, 2, 0),
    JS_FS("getConfigWord", dev_get_config_word, 2, 0),
    JS_FS("setConfig", dev_set_config, 3, 0),
    JS_FS("getOutputValue", dev_get_output_value, 1, 0),
    JS_FS("setOutputValue", dev_set_output_value, 2, 0),
    JS_FS("getSensorValue", dev_get_sensor_value, 1, 0),
    JS_FS("getSensorType", dev_get_sensor_type, 1, 0),
    JS_FS("getPropertyNode", dev_get_property_node, 0, 0),
    JS_FS_END
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

  JSBool dev_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    DeviceReference* dev = static_cast<DeviceReference*>(JS_GetPrivate(cx, obj));

    if(dev != NULL) {
      int opt = JSID_TO_INT(id);
      switch(opt) {
        case 0:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Device")));
          return JS_TRUE;
        case 1:
          {
            // make a local reference so the std::string does not go out of scope
            std::string tmp = dev->getDSID().toString();
            JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp.c_str())));
          }
          return JS_TRUE;
        case 2:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dev->getDevice()->getName().c_str())));
          return JS_TRUE;
        case 3:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(dev->getDevice()->getZoneID()));
          return JS_TRUE;
        case 4:
          {
            std::string tmp = dev->getDevice()->getDSMeterDSID().toString();
            JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp.c_str())));
          }
          return JS_TRUE;
        case 5:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(dev->getDevice()->getFunctionID()));
          return JS_TRUE;
        case 6:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(dev->getDevice()->getLastCalledScene()));
          return JS_TRUE;
        case 7:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(dev->getDevice()->getRevisionID()));
          return JS_TRUE;
        case 8:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(dev->getDevice()->getProductID()));
          return JS_TRUE;
        case 9:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(dev->getDevice()->isPresent()));
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
    "Device", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  finalize_dev, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSPropertySpec dev_properties[] = {
    {"className", 0, 0, dev_JSGet, NULL},
    {"dsid", 1, 0, dev_JSGet, NULL},
    {"name", 2, 0, dev_JSGet, NULL},
    {"zoneID", 3, 0, dev_JSGet, NULL},
    {"circuitID", 4, 0, dev_JSGet, NULL},
    {"functionID", 5, 0, dev_JSGet, NULL},
    {"lastCalledScene", 6, 0, dev_JSGet, NULL},
    {"revisionID", 7, 0, dev_JSGet, NULL},
    {"productID", 8, 0, dev_JSGet, NULL},
    {"isPresent", 9, 0, dev_JSGet, NULL},
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

  //=== DSMeter ===

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
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub, finalize_meter, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSBool dsmeter_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, obj))->pMeter;

    if(meter != NULL) {
      int opt = JSID_TO_INT(id);
      switch(opt) {
        case 0:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "DSMeter")));
          return JS_TRUE;
        case 1:
          {
            // make a local reference so the std::string does not go out of scope
            std::string tmp = meter->getDSID().toString();
            JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp.c_str())));
          }
          return JS_TRUE;
        case 2:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, meter->getName().c_str())));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  static JSPropertySpec dsmeter_properties[] = {
    {"className", 0, 0, dsmeter_JSGet, NULL},
    {"dsid", 1, 0, dsmeter_JSGet, NULL},
    {"name", 2, 0, dsmeter_JSGet, NULL},
    {NULL, 0, 0, NULL, NULL}
  };

  JSBool dsmeter_getPowerConsumption(JSContext* cx, uintN argc, jsval* vp) {
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;

    JS_SET_RVAL(cx, vp, JSVAL_NULL);
    try {
      if(meter != NULL) {
        try {
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(meter->getPowerConsumption()));
          return JS_TRUE;
        } catch (BusApiError& ex) {
          JS_ReportError(cx, "Bus failure: %s", ex.what());
          return JS_FALSE;
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // dsmeter_getPowerConsumption

  JSBool dsmeter_getCachedPowerConsumption(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;

    try {
      if(meter != NULL) {
        ScriptObject obj(*ctx, NULL);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));
        try {
          obj.setProperty<std::string>("timestamp", meter->getCachedPowerConsumptionTimeStamp().toString());
          obj.setProperty<int>("value", meter->getCachedPowerConsumption());
        } catch (BusApiError& ex) {
          JS_ReportError(cx, "Bus failure: %s", ex.what());
          return JS_FALSE;
        }
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // dsmeter_getCachedPowerConsumption

  JSBool dsmeter_getEnergyMeterValue(JSContext* cx, uintN argc, jsval* vp) {
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;

    JS_SET_RVAL(cx, vp, JSVAL_NULL);
    try {
      if(meter != NULL) {
        try {
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(meter->getEnergyMeterValue()));
        } catch (BusApiError& ex) {
          JS_ReportError(cx, "Bus failure: %s", ex.what());
          return JS_FALSE;
        }
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }

    return JS_FALSE;
  } // dsmeter_getEnergyMeterValue

  JSBool dsmeter_getCachedEnergyMeterValue(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;

    try {
      if(meter != NULL) {
        ScriptObject obj(*ctx, NULL);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));
        try {
          obj.setProperty<std::string>("timestamp", meter->getCachedEnergyMeterTimeStamp().toString());
          obj.setProperty<int>("value", meter->getCachedEnergyMeterValue());
        } catch (BusApiError& ex) {
          JS_ReportError(cx, "Bus failure: %s", ex.what());
          return JS_FALSE;
        }
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }

    return JS_FALSE;
  } // dsmeter_getCachedEnergyMeterValue

  JSBool dsmeter_get_property_node(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(
          ctx->getEnvironment().getExtension("propertyextension"));
      boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;
      if(meter != NULL) {
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, meter->getPropertyNode())));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dsmeter_get_property_node

  JSFunctionSpec dsmeter_methods[] = {
    JS_FS("getPowerConsumption", dsmeter_getPowerConsumption, 0, 0),
    JS_FS("getEnergyMeterValue", dsmeter_getEnergyMeterValue, 0, 0),
    JS_FS("getCachedPowerConsumption", dsmeter_getCachedPowerConsumption, 0, 0),
    JS_FS("getCachedEnergyMeterValue", dsmeter_getCachedEnergyMeterValue, 0, 0),
    JS_FS("getPropertyNode", dsmeter_get_property_node, 0, 0),
    JS_FS_END
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

  //=== JSZone ===

  struct zone_wrapper {
    boost::shared_ptr<Zone> pZone;
  };

  void finalize_zone(JSContext *cx, JSObject *obj) {
    struct zone_wrapper* pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pZone;
  } // finalize_meter

  static JSClass zone_class = {
    "Zone", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub, finalize_meter, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSBool zone_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, obj))->pZone;
    if(pZone != NULL) {
      int opt = JSID_TO_INT(id);
      switch(opt) {
        case 0:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Zone")));
          return JS_TRUE;
        case 1:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(pZone->getID()));
          return JS_TRUE;
        case 2:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, pZone->getName().c_str())));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  static JSPropertySpec zone_properties[] = {
    {"className", 0, 0, zone_JSGet, NULL},
    {"id", 1, 0, zone_JSGet, NULL},
    {"name", 2, 0, zone_JSGet, NULL},
    {NULL, 0, 0, NULL, NULL}
  };

  JSBool zone_getDevices(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      if(pZone != NULL) {
        Set devices = pZone->getDevices();
        JSObject* obj = ext->createJSSet(*ctx, devices);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // zone_getDevices

  JSBool zone_getPowerConsumption(JSContext* cx, uintN argc, jsval* vp) {
    boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;

    JS_SET_RVAL(cx, vp, JSVAL_NULL);
    try {
      if(pZone != NULL) {
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(pZone->getPowerConsumption()));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // zone_getPowerConsumption

  JSBool zone_pushSensorValue(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      dss_dsid_t sourceDSID;
      uint8_t sensorType;
      uint16_t sensorValue;
      if(pZone != NULL) {
        try {
          std::string sDSID = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
          sourceDSID = dsid::fromString(sDSID);
          sensorType = ctx->convertTo<uint8_t>(JS_ARGV(cx, vp)[1]);
          sensorValue = ctx->convertTo<uint16_t>(JS_ARGV(cx, vp)[2]);
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }
        StructureManipulator manipulator(
            *(ext->getApartment().getBusInterface()->getStructureModifyingBusInterface()),
            *(ext->getApartment().getBusInterface()->getStructureQueryBusInterface()),
            ext->getApartment());
        manipulator.sensorPush(pZone, sourceDSID, sensorType, sensorValue);
        //DSS::getInstance()->getBusInterface().getStructureModifyingBusInterface()->sensorPush(pZone->getID(), sourceDSID, sensorType, sensorValue);
        //pZone->sensorPush(sourceDSID, sensorType, sensorValue);
        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // zone_pushSensorValue

  JSBool zone_get_property_node(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(
          ctx->getEnvironment().getExtension("propertyextension"));
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      if(pZone != NULL) {
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, pZone->getPropertyNode())));
        return JS_TRUE;
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // zone_get_property_node

  JSFunctionSpec zone_methods[] = {
    JS_FS("getDevices", zone_getDevices, 0, 0),
    JS_FS("getPowerConsumption", zone_getPowerConsumption, 0, 0),
    JS_FS("pushSensorValue", zone_pushSensorValue, 3, 0),
    JS_FS("getPropertyNode", zone_get_property_node, 0, 0),
    JS_FS_END
  };

  JSObject* ModelScriptContextExtension::createJSZone(ScriptContext& _ctx, boost::shared_ptr<Zone> _pZone) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &zone_class, NULL, NULL);
    JS_DefineProperties(_ctx.getJSContext(), result, zone_properties);
    JS_DefineFunctions(_ctx.getJSContext(), result, zone_methods);
    struct zone_wrapper* wrapper = new zone_wrapper;
    wrapper->pZone = _pZone;
    JS_SetPrivate(_ctx.getJSContext(), result, wrapper);
    return result;
  } // createJSZone


  //================================================== ModelConstantsScriptExtension

  const std::string ModelConstantsScriptExtensionName = "modelconstantsextension";
  ModelConstantsScriptExtension::ModelConstantsScriptExtension()
    : ScriptExtension(ModelConstantsScriptExtensionName)
  { }

  static JSClass scene_class = {
    "Scene", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // scene_class

  JSBool scene_JSGetStatic(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    if (JSID_IS_INT(id)) {
      JS_SET_RVAL(cx, vp, INT_TO_JSVAL(JSID_TO_INT(id)));
      return JS_TRUE;
    }
    return JS_FALSE;
  } // scene_JSGetStatic

  static JSPropertySpec scene_properties[] = {
      {"User1", Scene1, 0, scene_JSGetStatic, NULL},
      {"User2", Scene2, 0, scene_JSGetStatic, NULL},
      {"User3", Scene3, 0, scene_JSGetStatic, NULL},
      {"User4", Scene4, 0, scene_JSGetStatic, NULL},
      {"Min", SceneMin, 0, scene_JSGetStatic, NULL},
      {"Max", SceneMax, 0, scene_JSGetStatic, NULL},
      {"DeepOff", SceneDeepOff, 0, scene_JSGetStatic, NULL},
      {"Off", SceneOff, 0, scene_JSGetStatic, NULL},
      {"StandBy", SceneStandBy, 0, scene_JSGetStatic, NULL},
      {"Bell", SceneBell, 0, scene_JSGetStatic, NULL},
      {"Panic", ScenePanic, 0, scene_JSGetStatic, NULL},
      {"Alarm", SceneAlarm, 0, scene_JSGetStatic, NULL},
      {"Inc", SceneInc, 0, scene_JSGetStatic, NULL},
      {"Dec", SceneDec, 0, scene_JSGetStatic, NULL},
      {"Present", ScenePresent, 0, scene_JSGetStatic, NULL},
      {"Absent", SceneAbsent, 0, scene_JSGetStatic, NULL},
      {"Sleeping", SceneSleeping, 0, scene_JSGetStatic, NULL},
      {"WakeUp", SceneWakeUp, 0, scene_JSGetStatic, NULL},
      {NULL, 0, 0, NULL, NULL}
};

JSBool scene_construct(JSContext *cx, uintN argc, jsval *vp) {
  return JS_FALSE;
} // scene_construct

void ModelConstantsScriptExtension::extendContext(ScriptContext& _context) {
  JS_InitClass(_context.getJSContext(),
      JS_GetGlobalObject(_context.getJSContext()),
      NULL,
      &scene_class,
      &scene_construct,
      0,
      NULL,
      NULL,
      scene_properties,
      NULL);

} // extendedJSContext


} // namespace