/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Michael Tross, aizo GmbH <michael.tross@aizo.com>
             Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#include "jsmetering.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>
#include <digitalSTROM/dsuid/dsuid.h>

#include "src/dss.h"
#include "src/logger.h"
#include "src/businterface.h"
#include "src/structuremanipulator.h"
#include "src/foreach.h"
#include "src/setbuilder.h"
#include "src/model/device.h"
#include "src/model/devicereference.h"
#include "src/model/apartment.h"
#include "src/model/set.h"
#include "src/model/modulator.h"
#include "src/model/modelconst.h"
#include "src/model/zone.h"
#include "src/metering/metering.h"
#include "src/security/security.h"
#include "src/scripting/scriptobject.h"
#include "src/scripting/jsproperty.h"

namespace dss {

  const std::string MeteringScriptExtensionName = "meteringextension";

  JSBool metering_getSeries(JSContext* cx, uintN argc, jsval *vp);
  JSBool metering_getResolutions(JSContext* cx, uintN argc, jsval *vp);
  JSBool metering_getAggregatedValues(JSContext* cx, uintN argc, jsval *vp);

  JSFunctionSpec metering_static_methods[] = {
    JS_FS("getSeries", metering_getSeries, 0, 0),
    JS_FS("getResolutions", metering_getResolutions, 0, 0),
    JS_FS("getValues", metering_getAggregatedValues, 3, 0),
    JS_FS("getAggregatedValues", metering_getAggregatedValues, 3, 0),
    JS_FS_END
  };

  static JSClass metering_class = {
    "Metering", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // metering_class

  JSBool metering_construct(JSContext *cx, uintN argc, jsval *vp) {
    return JS_FALSE;
  } // metering_construct


  JSBool metering_getSeries(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));

    try {
      MeteringScriptExtension* ext = dynamic_cast<MeteringScriptExtension*>(
          ctx->getEnvironment().getExtension(MeteringScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: metering_getSeries: ext of wrong type", lsFatal);
        return JS_FALSE;
      }

      int iMeter = 0;
      std::vector<boost::shared_ptr<DSMeter> > dsMeters = ext->getApartment().getDSMeters();
      foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
        ScriptObject objEnergy(*ctx, NULL);
        objEnergy.setProperty<std::string>("dsuid", dsuid2str(dsMeter->getDSID()));
        objEnergy.setProperty<std::string>("type", "energy");
        jsval childJSVal = OBJECT_TO_JSVAL(objEnergy.getJSObject());
        JSBool res = JS_SetElement(cx, resultObj, iMeter, &childJSVal);
        if(!res) {
          return JS_FALSE;
        }
        iMeter++;
        ScriptObject objEnergyDelta(*ctx, NULL);
        objEnergyDelta.setProperty<std::string>("dsuid", dsuid2str(dsMeter->getDSID()));
        objEnergyDelta.setProperty<std::string>("type", "energyDelta");
        childJSVal = OBJECT_TO_JSVAL(objEnergyDelta.getJSObject());
        res = JS_SetElement(cx, resultObj, iMeter, &childJSVal);
        if(!res) {
          return JS_FALSE;
        }
        iMeter++;
        ScriptObject objConsumption(*ctx, NULL);
        objConsumption.setProperty<std::string>("dsuid", dsuid2str(dsMeter->getDSID()));
        objConsumption.setProperty<std::string>("type", "consumption");
        childJSVal = OBJECT_TO_JSVAL(objConsumption.getJSObject());
        res = JS_SetElement(cx, resultObj, iMeter, &childJSVal);
        if(!res) {
          return JS_FALSE;
        }
      }
      return JS_TRUE;
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // metering_getSeries

  JSBool metering_getResolutions(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));

    try {
      MeteringScriptExtension* ext = dynamic_cast<MeteringScriptExtension*>(
          ctx->getEnvironment().getExtension(MeteringScriptExtensionName));

      if (ext == NULL) {
        Logger::getInstance()->log("JS: metering_getResolutions: ext of wrong type", lsFatal);
        return JS_FALSE;
      }

      std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = ext->getMetering().getConfig();
      int iResolution = 0;
      foreach(boost::shared_ptr<MeteringConfigChain> pChain, meteringConfig) {
        for(int iConfig = 0; iConfig < pChain->size(); iConfig++) {
          ScriptObject resolution(*ctx, NULL);
          resolution.setProperty<int>("resolution", pChain->getResolution(iConfig));
          resolution.setProperty<std::string>("type", "energy");
          jsval childJSVal = OBJECT_TO_JSVAL(resolution.getJSObject());
          JSBool res = JS_SetElement(cx, resultObj, iResolution, &childJSVal);
          if(!res) {
            return JS_FALSE;
          }
          iResolution++;
          ScriptObject objEnergyDelta(*ctx, NULL);
          objEnergyDelta.setProperty<int>("resolution", pChain->getResolution(iConfig));
          objEnergyDelta.setProperty<std::string>("type", "energyDelta");
          childJSVal = OBJECT_TO_JSVAL(objEnergyDelta.getJSObject());
          res = JS_SetElement(cx, resultObj, iResolution, &childJSVal);
          if(!res) {
            return JS_FALSE;
          }
          iResolution++;
          ScriptObject objConsumption(*ctx, NULL);
          objConsumption.setProperty<int>("resolution", pChain->getResolution(iConfig));
          objConsumption.setProperty<std::string>("type", "consumption");
          childJSVal = OBJECT_TO_JSVAL(objConsumption.getJSObject());
          res = JS_SetElement(cx, resultObj, iResolution, &childJSVal);
          if(!res) {
            return JS_FALSE;
          }
          iResolution++;
        }
      }
      return JS_TRUE;
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // metering_getResolutions

  JSBool metering_getAggregatedValues(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      MeteringScriptExtension* ext = dynamic_cast<MeteringScriptExtension*>(
          ctx->getEnvironment().getExtension(MeteringScriptExtensionName));

      if (ext == NULL) {
        Logger::getInstance()->log("JS: metering_getValues: ext of wrong type", lsFatal);
        return JS_FALSE;
      }

      if(argc < 3) {
        Logger::getInstance()->log("JS: metering_getValues: need three parameters: (dsuid, type, resolution)", lsError);
        return JS_FALSE;
      }
      std::string dsid = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
      std::string type = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
      int resolution = ctx->convertTo<int>(JS_ARGV(cx, vp)[2]);
      std::string unit = "Wh";
      if (argc >= 4) {
        unit = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[3]);
      }
      DateTime startTime(DateTime::NullDate);
      DateTime endTime(DateTime::NullDate);
      int valueCount = 0;
      if (argc >= 5) {
        startTime = DateTime(ctx->convertTo<int>(JS_ARGV(cx, vp)[4]));
      }
      if (argc >= 6) {
        endTime = DateTime(ctx->convertTo<int>(JS_ARGV(cx, vp)[5]));
      }
      if (argc >= 7) {
        valueCount = ctx->convertTo<int>(JS_ARGV(cx, vp)[6]);
      }

      std::vector<boost::shared_ptr<DSMeter> > pMeters;
      if (!beginsWith(dsid, ".meters(")) {
        dsid = ".meters(" + dsid + ")";
      }
      try {
        MeterSetBuilder builder(ext->getApartment());
        pMeters = builder.buildSet(dsid); // setbuilder supports dSID and dSUID
      } catch(std::runtime_error& e) {
        Logger::getInstance()->log("Couldn't parse parameter 'dsuid': '" + std::string(e.what()) + "'", lsWarning);
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
        return JS_TRUE;
      }
      Metering::SeriesTypes energy;
      if(type == "consumption") {
        energy = Metering::etConsumption;
      } else if(type == "energy") {
        energy = Metering::etEnergy;
      } else if(type == "energyDelta") {
        energy = Metering::etEnergyDelta;
      } else {
        Logger::getInstance()->log("JS: metering_getValues: Invalid type '" + type + "'", lsError);
        return JS_FALSE;
      }
      bool energyWh;
      if (unit == "Wh") {
        energyWh = true;
      } else if (unit == "Ws") {
        energyWh = false;
      } else {
        Logger::getInstance()->log("JS: metering_getValues: Invalid unit '" + unit + "'", lsError);
        return JS_FALSE;
      }

      boost::shared_ptr<std::deque<Value> > pSeries = ext->getMetering().getSeries(pMeters,
                                                                                   resolution,
                                                                                   energy,
                                                                                   energyWh,
                                                                                   startTime,
                                                                                   endTime,
                                                                                   valueCount);
      if(NULL == pSeries) {
        JS_ReportWarning(cx, "could not find metering data for %s and resultion %s", type.c_str(), resolution);
        return JS_FALSE;
      }
      int valueNumber = 0;
      JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
      for(std::deque<Value>::iterator iValue = pSeries->begin(),
          e = pSeries->end();
          iValue != e;
          ++iValue)
      {
        ScriptObject valuePair(*ctx, NULL);
        DateTime tmp_date = iValue->getTimeStamp();
        valuePair.setProperty<std::string>("timestamp", tmp_date.toString());
        valuePair.setProperty<double>("value", iValue->getValue());
        jsval childJSVal = OBJECT_TO_JSVAL(valuePair.getJSObject());
        JSBool res = JS_SetElement(cx, resultObj, valueNumber, &childJSVal);
        if(!res) {
          return JS_FALSE;
        }
        valueNumber++;
      }
      return JS_TRUE;
    } catch(ScriptException& e) {
      JS_ReportError(cx, "Scripting failure: %s", e.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // metering_getAggregatedValues


  MeteringScriptExtension::MeteringScriptExtension(Apartment& _apartment,
                                                   Metering& _metering)
  : ScriptExtension(MeteringScriptExtensionName),
    m_Apartment(_apartment),
    m_Metering(_metering)
  { }

  void MeteringScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), NULL,
                 &metering_class, &metering_construct, 0, NULL, NULL, NULL, metering_static_methods);
  } // extendedJSContext

} // namespace
