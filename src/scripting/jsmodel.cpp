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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "jsmodel.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

#include <digitalSTROM/dsuid.h>
#include <ds/log.h>

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
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/zone.h"
#include "src/model/state.h"
#include "src/metering/metering.h"
#include "src/scripting/scriptobject.h"
#include "src/scripting/jsproperty.h"
#include "src/security/security.h"
#include "src/stringconverter.h"
#include "src/sceneaccess.h"
#include "src/util.h"
#include "src/messages/vdc-messages.pb.h"
#include "src/vdc-element-reader.h"
#include "src/web/webrequests.h"
#include "src/protobufjson.h"
#include "src/stringconverter.h"
#include "src/vdc-connection.h"
#include "comm-channel.h"


namespace dss {
  struct meter_wrapper {
    boost::shared_ptr<DSMeter> pMeter;
  };

  const std::string ModelScriptcontextExtensionName = "modelextension";

  Set filterOutInvisibleDevices(Set set) {
    Set invisible = Set();
    for (int i = 0; i < set.length(); i++) {
      const DeviceReference& d = set.get(i);
      if (!d.getDevice()->isVisible() && d.getDevice()->isPresent()) {
        invisible.addDevice(d);
      }
    }

    if (!invisible.isEmpty()) {
      return set.remove(invisible);
    }

    return set;
  }

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
        aptName = escapeHTML(aptName);

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
        Set devices = filterOutInvisibleDevices(ext->getApartment().getDevices());
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
          dsuid_t dsuid;
          try {
            std::string id = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
            if (id.length() <= 24) {
              dsid_t dsid = str2dsid(id);
              dsuid = dsuid_from_dsid(&dsid);
            } else {
              dsuid =  str2dsuid(id);
            }
          } catch(ScriptException& e) {
            JS_ReportError(cx, "Error converting dsuid string to dsuid");
            return JS_FALSE;
          } catch(std::invalid_argument& e) {
            JS_ReportError(cx, "Error converting dsuid string to dsuid");
            return JS_FALSE;
          }
          try {
            boost::shared_ptr<DSMeter> meter = ext->getApartment().getDSMeterByDSID(dsuid);
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
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_getDSMeters: ext of wrong type");
        return JS_FALSE;
      }

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
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_getZones: ext of wrong type");
        return JS_FALSE;
      }

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

  JSBool global_getClusters(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_getClusters: ext of wrong type");
        return JS_FALSE;
      }

      JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));

      std::size_t i = 0;
      foreach (boost::shared_ptr<Cluster> cluster, ext->getApartment().getClusters()) {
        if (cluster->getApplicationType() == ApplicationType::None) {
          continue;
        }
        JSObject* clusterObj = ext->createJSCluster(*ctx, cluster);
        jsval clusterJSVal = OBJECT_TO_JSVAL(clusterObj);
        JSBool res = JS_SetElement(cx, resultObj, i, &clusterJSVal);
        ++i;
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
  } // global_getClusters

  JSBool global_getZoneByID(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_getZoneByID: ext of wrong type");
        return JS_FALSE;
      }

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

  JSBool global_getStateByName(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_getStateByName: ext of wrong type");
        return JS_FALSE;
      }

      std::string stateName;
      try {
        stateName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: state name");
        return JS_FALSE;
      }

      eStateType stateType = StateType_Script;
      try {
        if (argc >= 2) {
          if (!ctx->convertTo<bool>(JS_ARGV(cx, vp)[1])) { // private param false
            stateType = StateType_Service;
          }
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: persistence");
        return JS_FALSE;
      }

      boost::shared_ptr<State> state;
      try {
        if (stateType == StateType_Script) {
          state = ext->getApartment().getState(StateType_Script, ctx->getWrapper()->getIdentifier(), stateName);
        } else {
          state = ext->getApartment().getState(StateType_Service, stateName);
        }
        JSObject* obj = ext->createJSState(*ctx, state);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
        return JS_TRUE;
      } catch(ItemNotFoundException& e) {
        JS_ReportError(cx, "State with given name not found");
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
  } // global_getStateByName

  JSBool global_registerState(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_registerState: ext of wrong type");
        return JS_FALSE;
      }

      std::string stateName;
      try {
        stateName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: state name");
        return JS_FALSE;
      }

      bool isPersistent = false;
      try {
        if (argc >= 2) {
          isPersistent = ctx->convertTo<bool>(JS_ARGV(cx, vp)[1]);
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: persistence");
        return JS_FALSE;
      }

      eStateType stateType = StateType_Script;
      try {
        if (argc >= 3) {
          if (!ctx->convertTo<bool>(JS_ARGV(cx, vp)[2])) {
            stateType = StateType_Service;
          }
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: private");
        return JS_FALSE;
      }

      std::string identifier = ctx->getWrapper()->getIdentifier();
      try {
        boost::shared_ptr<State> state = ext->getApartment().allocateState(stateType, stateName, identifier);
        state->setPersistence(isPersistent);
        JSObject* obj = ext->createJSState(*ctx, state);
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
        return JS_TRUE;
      } catch(ItemNotFoundException& e) {
        JS_ReportError(cx, "State with given name not found");
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
        return JS_FALSE;
      } catch(ItemDuplicateException& e) {
        JS_ReportError(cx, "State with given name already existing");
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
  } // global_registerState

  JSBool global_unregisterState(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_unregisterState: ext of wrong type");
        return JS_FALSE;
      }

      std::string stateName;
      try {
        stateName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: state name");
        return JS_FALSE;
      }

      eStateType stateType = StateType_Script;
      try {
        if (argc >= 2) {
          if (!ctx->convertTo<bool>(JS_ARGV(cx, vp)[1])) {
            stateType = StateType_Service;
          }
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: script type");
        return JS_FALSE;
      }

      std::string identifier = ctx->getWrapper()->getIdentifier();
      try {
        boost::shared_ptr<State> state;
        state = ext->getApartment().getState(stateType, identifier, stateName);
        ext->getApartment().removeState(state);
        state.reset();
        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
        return JS_TRUE;
      } catch(ItemNotFoundException& e) {
        JS_ReportError(cx, "State with given name not found");
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
  } // global_unregisterState

  JSBool global_unregisterStateSensor(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_unregisterStateSensor: ext of wrong type");
        return JS_FALSE;
      }

      std::string stateName;
      try {
        stateName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: state name");
        return JS_FALSE;
      }

      boost::shared_ptr<State> state;
      std::string identifier = ctx->getWrapper()->getIdentifier();
      try {
        eStateType stateType = StateType_SensorDevice;
        state = ext->getApartment().getState(stateType, identifier, stateName);
        ext->getApartment().removeState(state);
        state.reset();
        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
        return JS_TRUE;
      } catch(ItemNotFoundException& e) {}
      try {
        eStateType stateType = StateType_SensorZone;
        state = ext->getApartment().getState(stateType, identifier, stateName);
        ext->getApartment().removeState(state);
        state.reset();
        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
        return JS_TRUE;
      } catch(ItemNotFoundException& e) {}

      JS_ReportError(cx, "StateSensor with given name not found");
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      return JS_FALSE;
    } catch (SecurityException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: security exception: ") + ex.what(), lsError);
    } catch (DSSException& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: dss exception: ") + ex.what(), lsError);
    } catch (std::exception& ex) {
      Logger::getInstance()->log(std::string("JS: scripting failure: general exception: ") + ex.what(), lsError);
    }

    return JS_FALSE;
  } // global_unregisterStateSensor

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
        unsigned long result = 0;
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

  JSBool global_get_weatherInformation(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if (ext != NULL) {
      ApartmentSensorStatus_t aStatus = ext->getApartment().getSensorStatus();

      ScriptObject obj(*ctx, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));
      obj.setProperty<std::string>("WeatherIconId", aStatus.m_WeatherIconId);
      obj.setProperty<std::string>("WeatherConditionId", aStatus.m_WeatherConditionId);
      obj.setProperty<std::string>("WeatherServiceId", aStatus.m_WeatherServiceId);
      obj.setProperty<std::string>("WeatherServiceTime", aStatus.m_WeatherTS.toISO8601_ms());
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_get_weatherInformation

  JSBool global_set_weatherInformation(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext != NULL && argc >= 1) {
        StringConverter st("UTF-8", "UTF-8");
        std::string iconId, condId, serviceName;
        if (argc >= 1) {
          iconId = st.convert(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          iconId = escapeHTML(iconId);
        }
        if (argc >= 2) {
          condId = st.convert(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]));
          condId = escapeHTML(condId);
        }
        if (argc >= 3) {
          serviceName = st.convert(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]));
          serviceName = escapeHTML(serviceName);
        }
        DateTime now;
        ext->getApartment().setWeatherInformation(iconId, condId, serviceName, now);
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
  } // global_set_weatherInformation

  JSBool global_set_deviceVisibility(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_setDeviceVisibility: ext of wrong type");
        return JS_FALSE;
      }

      if (argc < 2)
      {
          JS_ReportError(cx, "Model.global_setDeviceVisibility: wrong number of arguments");
          return JS_FALSE;
      }
      dsuid_t dsuid;
      try {
        std::string id = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        if (id.length() <= 24) {
          dsid_t dsid = str2dsid(id);
          dsuid = dsuid_from_dsid(&dsid);
        } else {
          dsuid =  str2dsuid(id);
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      } catch(std::invalid_argument& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      }

      bool visibility;
      try {
        visibility = ctx->convertTo<bool>(JS_ARGV(cx, vp)[1]);
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting parameter: visibility");
        return JS_FALSE;
      }

      try {
        boost::shared_ptr<Device> device = ext->getApartment().getDeviceByDSID(dsuid);
        bool wasVisible = device->isVisible();
        device->setDeviceVisibility(visibility);
        bool isVisible = device->isVisible();

        if (wasVisible && !isVisible) {
          dsuid_t main = device->getMainDeviceDSUID();
          if (!dsuid_equal(&dsuid, &main)) {
            boost::shared_ptr<Device> main_device;
            try {
              main_device = ext->getApartment().getDeviceByDSID(main);
              StructureManipulator manipulator(
                  *ext->getApartment().getBusInterface()->getStructureModifyingBusInterface(),
                  *ext->getApartment().getBusInterface()->getStructureQueryBusInterface(),
                  ext->getApartment());
              boost::shared_ptr<Zone> zone = ext->getApartment().getZone(
                                                    main_device->getZoneID());
              manipulator.addDeviceToZone(device, zone);
            } catch(ItemNotFoundException& e) {
               Logger::getInstance()->log("JS: could not determine main device", lsWarning);
            }
          }
        }
        return JS_TRUE;
      } catch(ItemNotFoundException& e) {
        JS_ReportError(cx, "Device with given dSUID not found");
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
  } // global_set_deviceVisibility

  JSBool global_getDeviceConfig(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);

      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_getDeviceConfig: ext of wrong type");
        return JS_FALSE;
      }

      if (argc < 3) {
          JS_ReportError(cx, "Model.global_getDeviceConfig: wrong number of arguments");
          return JS_FALSE;
      }

      dsuid_t dsuid;
      try {
        std::string id = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        if (id.length() <= 24) {
          dsid_t dsid = str2dsid(id);
          dsuid = dsuid_from_dsid(&dsid);
        } else {
          dsuid = str2dsuid(id);
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      } catch(std::invalid_argument& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      }

      int configClass;
      int configIndex;
      try {
        configClass = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
        configIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[2]);
      } catch (ScriptException& ex) {
        JS_ReportError(cx, "Convert arguments: %s", ex.what());
        return JS_FALSE;
      }

      jsrefcount ref = JS_SuspendRequest(cx);
      try {
        boost::shared_ptr<Device> device = ext->getApartment().getDeviceByDSID(dsuid);
        uint8_t retValue = device->getDeviceConfig(configClass, configIndex);
        JS_ResumeRequest(cx, ref);
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(retValue));
        return JS_TRUE;
      } catch(const BusApiError& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "Bus failure: %s", ex.what());
      } catch (DSSException& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "Failure: %s", ex.what());
      } catch (std::exception& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "General Failure: %s", ex.what());
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // global_getDeviceConfig

  JSBool global_getDeviceConfigWord(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);

      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_getDeviceConfigWord: ext of wrong type");
        return JS_FALSE;
      }

      if (argc < 3) {
          JS_ReportError(cx, "Model.global_getDeviceConfigWord: wrong number of arguments");
          return JS_FALSE;
      }

      dsuid_t dsuid;
      try {
        std::string id = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        if (id.length() <= 24) {
          dsid_t dsid = str2dsid(id);
          dsuid = dsuid_from_dsid(&dsid);
        } else {
          dsuid = str2dsuid(id);
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      } catch(std::invalid_argument& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      }

      int configClass;
      int configIndex;
      try {
        configClass = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
        configIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[2]);
      } catch (ScriptException& ex) {
        JS_ReportError(cx, "Convert arguments: %s", ex.what());
        return JS_FALSE;
      }

      jsrefcount ref = JS_SuspendRequest(cx);
      try {
        boost::shared_ptr<Device> device = ext->getApartment().getDeviceByDSID(dsuid);
        uint16_t retValue= device->getDeviceConfigWord(configClass, configIndex);
        JS_ResumeRequest(cx, ref);
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(retValue));
        return JS_TRUE;
      } catch(const BusApiError& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "Bus failure: %s", ex.what());
      } catch (DSSException& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "Failure: %s", ex.what());
      } catch (std::exception& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "General Failure: %s", ex.what());
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // global_getDeviceConfig

  JSBool global_setDeviceConfig(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);

      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.global_setDeviceConfig: ext of wrong type");
        return JS_FALSE;
      }

      if (argc < 4) {
          JS_ReportError(cx, "Model.global_setDeviceConfig: wrong number of arguments");
          return JS_FALSE;
      }

      dsuid_t dsuid;
      try {
        std::string id = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        if (id.length() <= 24) {
          dsid_t dsid = str2dsid(id);
          dsuid = dsuid_from_dsid(&dsid);
        } else {
          dsuid = str2dsuid(id);
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      } catch(std::invalid_argument& e) {
        JS_ReportError(cx, "Error converting dsuid string to dsuid");
        return JS_FALSE;
      }

      int configClass;
      int configIndex;
      int configValue;

      try {
        configClass = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
        configIndex = ctx->convertTo<int>(JS_ARGV(cx, vp)[2]);
        configValue = ctx->convertTo<int>(JS_ARGV(cx, vp)[3]);
      } catch (ScriptException& ex) {
        JS_ReportError(cx, "Convert arguments: %s", ex.what());
        return JS_FALSE;
      }
      jsrefcount ref = JS_SuspendRequest(cx);

      try {
        boost::shared_ptr<Device> device = ext->getApartment().getDeviceByDSID(dsuid);
        device->setDeviceConfig(configClass, configIndex, configValue);
        JS_ResumeRequest(cx, ref);
        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
        return JS_TRUE;
      } catch(const BusApiError& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "Bus failure: %s", ex.what());
      } catch (DSSException& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "Failure: %s", ex.what());
      } catch (std::exception& ex) {
        JS_ResumeRequest(cx, ref);
        JS_ReportError(cx, "General Failure: %s", ex.what());
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // global_setDeviceConfig

  JSFunctionSpec apartment_static_methods[] = {
    JS_FS("getName", global_get_name, 0, 0),
    JS_FS("setName", global_set_name, 1, 0),
    JS_FS("getDevices", global_get_devices, 0, 0),
    JS_FS("getDSMeterByDSID", global_get_dsmeterbydsid, 1, 0),
    JS_FS("getDSMeterByDSUID", global_get_dsmeterbydsid, 1, 0),
    JS_FS("getConsumption", global_getConsumption, 0, 0),
    JS_FS("getEnergyMeterValue", global_getEnergyMeterValue, 0, 0),
    JS_FS("getDSMeters", global_getDSMeters, 0, 0),
    JS_FS("getZones", global_getZones, 0, 0),
    JS_FS("getClusters", global_getClusters, 0, 0),
    JS_FS("getZoneByID", global_getZoneByID, 0, 0),
    JS_FS("getState", global_getStateByName, 1, 0),
    JS_FS("registerState", global_registerState, 1, 0),
    JS_FS("unregisterState", global_unregisterState, 2, 0),
    JS_FS("unregisterStateSensor", global_unregisterStateSensor, 1, 0),
    JS_FS("getWeatherInformation", global_get_weatherInformation, 0, 0),
    JS_FS("setWeatherInformation", global_set_weatherInformation, 3, 0),
    JS_FS("setDeviceVisibility", global_set_deviceVisibility, 2, 0),
    JS_FS("getDeviceConfig", global_getDeviceConfig, 3, 0),
    JS_FS("getDeviceConfigWord", global_getDeviceConfigWord, 3, 0),
    JS_FS("setDeviceConfig", global_setDeviceConfig, 4, 0),
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
  } // extendContext

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
  } // set_length

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
  } // set_combine

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
    if (ext == NULL) {
      JS_ReportError(cx, "Model.set_perform: ext of wrong type");
      return JS_FALSE;
    }
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

        dsuid_t meterDSID;
        try {
          if (dsmeterID.length() <= 24) {
            dsid_t dsid = str2dsid(dsmeterID);
            meterDSID = dsuid_from_dsid(&dsid);
          } else {
            meterDSID = str2dsuid(dsmeterID);
          }
        } catch(std::invalid_argument& ex) {
          JS_ReportError(cx, "Convert dsuid parameter: %s", ex.what());
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
          std::string dsidStr = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
          dsuid_t dsuid;
          if (dsidStr.length() <= 24) {
            dsid_t dsid = str2dsid(dsidStr);
            dsuid = dsuid_from_dsid(&dsid);
          } else {
            dsuid = str2dsuid(dsidStr);
          }
          DeviceReference result = set->getByDSID(dsuid);
          JSObject* resultObj = ext->createJSDevice(*ctx, result);
          JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
          return JS_TRUE;
        } catch(ScriptException& ex) {
          JS_ReportError(cx, ex.what());
        } catch(std::invalid_argument&) {
          JS_ReportError(cx, "set.byDSUID: Could not parse dsuid");
        } catch(ItemNotFoundException&) {
          JS_ReportError(cx, "set.byDSUID: Device with dsuid not found");
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
          dsuid_t meterDSID;
          if (dsmeterID.length() <= 24) {
            dsid_t dsid = str2dsid(dsmeterID);
            meterDSID = dsuid_from_dsid(&dsid);
          } else {
            meterDSID = str2dsuid(dsmeterID);
          }
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
  } // set_JSGet

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
    JS_FS("byDSUID", set_by_dsid, 1, 0),
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
    {}
  };

  JSBool dev_turn_on(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          SceneAccessCategory category = SAC_UNKNOWN;
          if (argc >= 1) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          }
          intf->turnOn(coJSScripting, category);
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_turn_on

  JSBool dev_turn_off(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          SceneAccessCategory category = SAC_UNKNOWN;
          if (argc >= 1) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          }
          intf->turnOff(coJSScripting, category);
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_turn_off

  JSBool dev_blink(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          SceneAccessCategory category = SAC_UNKNOWN;
          if (argc >= 1) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          }
          intf->blink(coJSScripting, category, "");
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_blink

  JSBool dev_increase_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          SceneAccessCategory category = SAC_UNKNOWN;
          if (argc >= 1) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          }
          intf->increaseValue(coJSScripting, category);
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_increase_value

  JSBool dev_decrease_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));

        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          SceneAccessCategory category = SAC_UNKNOWN;
          if (argc >= 1) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          }
          intf->decreaseValue(coJSScripting, category);
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_decrease_value

  JSBool dev_set_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Set") || self.is("Device")) {
        IDeviceInterface* intf = static_cast<IDeviceInterface*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        if(argc >= 1) {
          uint8_t value = 0;
          SceneAccessCategory category = SAC_UNKNOWN;
          try {
            value = ctx->convertTo<uint8_t>(JS_ARGV(cx, vp)[0]);

            if(argc >= 2) {
              category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]));
            }
          } catch (ScriptException& ex) {
            JS_ReportError(cx, ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            intf->setValue(coJSScripting, category, value, "");
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
            return JS_TRUE;
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
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
        if(argc >= 1) {
          int sceneNr;
          bool forceFlag = false;
          SceneAccessCategory category = SAC_UNKNOWN;
          try {
            sceneNr = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
            if (argc >= 2) {
              forceFlag = ctx->convertTo<bool>(JS_ARGV(cx, vp)[1]);
            }
            if (argc >= 3) {
              category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]));
            }
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Device.callScene: cannot convert parameters: %s", ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            intf->callScene(coJSScripting, category, sceneNr, "", forceFlag);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
            return JS_TRUE;
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
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
        SceneAccessCategory category = SAC_UNKNOWN;
        if(argc > 1) {
          try {
            sceneNr = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, "Convert parameter scene number: %s", ex.what());
            return JS_FALSE;
          }
          if (argc >= 2) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]));
          }
        }
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          if (sceneNr >= 0) {
            intf->undoScene(coJSScripting, category, sceneNr, "");
          } else {
            intf->undoSceneLast(coJSScripting, category, "");
          }
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
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
          SceneAccessCategory category = SAC_UNKNOWN;
          if (argc >= 1) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          }
          intf->nextScene(coJSScripting, category);
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
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
          SceneAccessCategory category = SAC_UNKNOWN;
          if (argc >= 1) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
          }
          intf->previousScene(coJSScripting, category);
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
          return JS_TRUE;
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General Failure: %s", ex.what());
        }
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
          try {
            if (CommChannel::getInstance()->isSceneLocked((uint32_t)sceneNr)) {
              JS_ReportError(cx, "Device settings are being updated for selected activity, please try again later");
              return JS_FALSE;
            }
          } catch (std::runtime_error &err) {
            JS_ReportError(cx, "Can't save scene: %s", err.what());
            return JS_FALSE;
          }

          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            intf->saveScene(coJSScripting, sceneNr, "");
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(0));
            return JS_TRUE;
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
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
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
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
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General Failure: %s", ex.what());
          }
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
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_set_config

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
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_output_value

  JSBool dev_get_vdc_property(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if (ext == NULL) {
      JS_ReportError(cx, "ext of wrong type");
      return JS_FALSE;
    }

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      if(argc == 1) {
        std::string path;
        try {
          path = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        } catch (ScriptException& ex) {
          JS_ReportError(cx, "Convert arguments: %s", ex.what());
          return JS_FALSE;
        }
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          std::vector<std::string> parts = splitString(path, '/');
          google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
          vdcapi::PropertyElement* currentElement;
          bool firstElement = true;
          foreach(std::string part, parts) {
            if (firstElement) {
              currentElement = query.Add();
              firstElement = false;
            } else {
              currentElement = currentElement->add_elements();
            }
            currentElement->set_name(part);
          }
          ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
          vdcapi::Message message;
          if(self.is("Device")) {
            DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
            boost::shared_ptr<Device> pDev(intf->getDevice());

            if (!pDev->isVdcDevice()) {
              JS_ReportError(cx, "Device is not a vdc device");
              return JS_FALSE;
            }
            message = pDev->getVdcProperty(query);
          } else if (self.is("DSMeter")) {
            boost::shared_ptr<DSMeter> pMeter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;
            StructureManipulator manipulator(
                *ext->getApartment().getBusInterface()->getStructureModifyingBusInterface(),
                *ext->getApartment().getBusInterface()->getStructureQueryBusInterface(),
                ext->getApartment());
            message = manipulator.getProperty(pMeter, query);
          }
          VdcElementReader reader(message.vdc_response_get_property().properties());
          JSONWriter json(JSONWriter::jsonNoneResult);
          json.add("result");
          ProtobufToJSon::processElementsPretty(reader.childElements(), json);

          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, json.successJSON().c_str())));
          return JS_TRUE;
        } catch(const BusApiError& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Bus failure: %s", ex.what());
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_vdc_property

  JSBool dev_set_vdc_property(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if (ext == NULL) {
      JS_ReportError(cx, "ext of wrong type");
      return JS_FALSE;
    }

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      if (argc == 2) {
        std::string path;
        std::string value;
        try {
          path = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        } catch (ScriptException& ex) {
          JS_ReportError(cx, ex.what());
          return JS_FALSE;
        }
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          std::vector<std::string> parts = splitString(path, '/');
          google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
          vdcapi::PropertyElement* currentElement = 0;
          bool firstElement = true;
          foreach(std::string part, parts) {
            if (firstElement) {
              currentElement = query.Add();
              firstElement = false;
            } else {
              currentElement = currentElement->add_elements();
            }
            currentElement->set_name(part);
          }

          if (currentElement == 0) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "path failed to parse");
            return JS_FALSE;
          }

          if(JSVAL_IS_STRING(JS_ARGV(cx, vp)[1])) {
            StringConverter st("UTF-8", "UTF-8");
            std::string propValue = st.convert(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]));
            currentElement->mutable_value()->set_v_string(propValue);
          } else if(JSVAL_IS_BOOLEAN(JS_ARGV(cx, vp)[1])) {
            currentElement->mutable_value()->set_v_bool(ctx->convertTo<bool>(JS_ARGV(cx, vp)[1]));
          } else if (JSVAL_IS_INT(JS_ARGV(cx, vp)[1])) {
            currentElement->mutable_value()->set_v_int64(ctx->convertTo<int>(JS_ARGV(cx, vp)[1]));
          } else if(JSVAL_IS_DOUBLE(JS_ARGV(cx, vp)[1])) {
            currentElement->mutable_value()->set_v_double(ctx->convertTo<double>(JS_ARGV(cx, vp)[1]));
          } else {
            JS_ReportWarning(cx, "Property.setVdcProperty: unknown type of argument 2");
            JS_ResumeRequest(cx, ref);
            return JS_FALSE;
          }
          ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
          if(self.is("Device")) {
            DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
            boost::shared_ptr<Device> pDev(intf->getDevice());
            if (!pDev->isVdcDevice()) {
              JS_ReportError(cx, "Device is not a vdc device");
              return JS_FALSE;
            }
            pDev->setVdcProperty(query);
          } else if (self.is("DSMeter")) {
            boost::shared_ptr<DSMeter> pMeter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;
            StructureManipulator manipulator(
                *ext->getApartment().getBusInterface()->getStructureModifyingBusInterface(),
                *ext->getApartment().getBusInterface()->getStructureQueryBusInterface(),
                ext->getApartment());
            manipulator.setProperty(pMeter, query);
          }

          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
          return JS_TRUE;
        } catch(const BusApiError& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Bus failure: %s", ex.what());
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General failure: %s", ex.what());
        }
      }

    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_set_output_value

  JSBool dev_set_output_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if (argc == 2) {
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
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
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
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
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
            boost::shared_ptr<DeviceSensor_t> sensor = pDev->getSensor(sensorIndex);
            int retValue = static_cast<int>(sensor->m_sensorType);
            JS_SET_RVAL(cx, vp, INT_TO_JSVAL(retValue));
            JS_ResumeRequest(cx, ref);
            return JS_TRUE;
          } catch(const BusApiError& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_get_sensor_type

  // dev.addStateSensor(int sensorType, string 1-Condition, string 0-Condition)
  JSBool dev_addStateSensor(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      boost::shared_ptr<Device> pDev;
      SensorType sensorType;
      std::string activateCondition;
      std::string deactivateCondition;
      std::string creatorId = "default";

      if (ext == NULL) {
        JS_ReportError(cx, "Model.dev_addStateSensor: ext of wrong type");
        return JS_FALSE;
      }
      if (argc < 3) {
        JS_ReportError(cx, "Model.dev_addStateSensor: missing arguments");
        return JS_FALSE;
      }
      if (self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        pDev = intf->getDevice();
      }
      if (NULL == pDev) {
        JS_ReportError(cx, "Model.dev_addStateSensor: device object error");
        return JS_FALSE;
      }

      try {
        sensorType = static_cast<SensorType>(ctx->convertTo<int>(JS_ARGV(cx, vp)[0]));
        activateCondition = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
        deactivateCondition = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]);
        if (argc >= 4) {
          creatorId = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[3]);
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, e.what());
        return JS_FALSE;
      } catch(std::invalid_argument& e) {
        JS_ReportError(cx, e.what());
        return JS_FALSE;
      }

      boost::shared_ptr<DeviceSensor_t> pSensor;
      try {
        pSensor = pDev->getSensorByType(sensorType);
      } catch (ItemNotFoundException& ex) {
        JS_ReportError(cx, "Model.dev_addStateSensor: invalid sensor type");
        return JS_TRUE;
      }

      std::string identifier = ctx->getWrapper() ? ctx->getWrapper()->getIdentifier() : "";
      boost::shared_ptr<StateSensor> state = boost::make_shared<StateSensor> (
          creatorId, identifier, pDev, sensorType, activateCondition, deactivateCondition);
      std::string sName = state->getName();
      state->setPersistence(true);
      try {
        ext->getApartment().allocateState(state);
      } catch(ItemDuplicateException& e) {
        JS_ReportWarning(cx, "Device sensor state %s already exists: %s", sName.c_str(), e.what());
      }
      JSString* str = JS_NewStringCopyN(cx, sName.c_str(), sName.size());
      JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(str));
      return JS_TRUE;

    } catch(ItemDuplicateException& ex) {
      JS_ReportWarning(cx, "Item duplicate: %s", ex.what());
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
  } // dev_addStateSensor

  JSBool dev_get_property_node(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension("propertyextension"));
    if (ext == NULL) {
      JS_ReportError(cx, "Model.dev_get_property_node: ext of wrong type");
      return JS_FALSE;
    }
    DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
    PropertyNodePtr pnode = intf->getDevice()->getPropertyNode();
    if (NULL != pnode) {
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, pnode)));
    } else {
      JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(false));
    }
    return JS_TRUE;
  } // dev_get_property_node

  JSBool dev_set_joker_group(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);

      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "ext of wrong type");
        return JS_FALSE;
      }

      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDev(intf->getDevice());
        if(argc < 1) {
          JS_ReportError(cx, "missing group id parameter");
          return JS_FALSE;
        }

        int newGroupId = -1;
        try {
          newGroupId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
        } catch (ScriptException& ex) {
          JS_ReportError(cx, "could not convert group id argument: %s",
                         ex.what());
          return JS_FALSE;
        }

        if (!isDefaultGroup(newGroupId)) {
          JS_ReportError(cx, "invalid group id parameter");
          return JS_FALSE;
        }

        StructureManipulator manipulator(
            *ext->getApartment().getBusInterface()->getStructureModifyingBusInterface(),
            *ext->getApartment().getBusInterface()->getStructureQueryBusInterface(),
            ext->getApartment());


        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          manipulator.setJokerGroup(pDev, newGroupId);
          if (pDev->is2WayMaster()) {
            dsuid_t next;
            dsuid_get_next_dsuid(pDev->getDSID(), &next);
            boost::shared_ptr<Device> pPartnerDevice;
            try {
              pPartnerDevice = DSS::getInstance()->getApartment().getDeviceByDSID(next);
            } catch(ItemNotFoundException& e) {
              JS_ResumeRequest(cx, ref);
              JS_ReportError(cx, "could not find partner device");
              return JS_FALSE;
            }

            manipulator.setJokerGroup(pPartnerDevice, newGroupId);
          }
          JS_ResumeRequest(cx, ref);
          return JS_TRUE;

        } catch(const BusApiError& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Bus failure: %s", ex.what());
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_set_joker_group

  JSBool dev_set_button_id(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);

      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "ext of wrong type");
        return JS_FALSE;
      }

      if(self.is("Device")) {
        DeviceReference* intf = static_cast<DeviceReference*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));
        boost::shared_ptr<Device> pDevice(intf->getDevice());
        if(argc < 1) {
          JS_ReportError(cx, "missing button id parameter");
          return JS_FALSE;
        }

        int buttonId = -1;
        try {
          buttonId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
        } catch (ScriptException& ex) {
          JS_ReportError(cx, "could not convert button id argument: %s",
                         ex.what());
          return JS_FALSE;
        }

        if ((buttonId  < 0) || (buttonId > 15)) {
          JS_ReportError(cx, "invalid button id parameter");
          return JS_FALSE;
        }

        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          pDevice->setDeviceButtonID(buttonId);
          if (pDevice->is2WayMaster()) {
            DeviceFeatures_t features = pDevice->getDeviceFeatures();
            if (!features.syncButtonID) {
              JS_ResumeRequest(cx, ref);
              return JS_TRUE;
            }

            dsuid_t next;
            dsuid_get_next_dsuid(pDevice->getDSID(), &next);
            try {
              boost::shared_ptr<Device> pPartnerDevice;
              pPartnerDevice = DSS::getInstance()->getApartment().getDeviceByDSID(next);
              pPartnerDevice->setDeviceButtonID(buttonId);
            } catch(ItemNotFoundException& e) {
              JS_ResumeRequest(cx, ref);
              JS_ReportError(cx, "could not find partner device");
              return JS_FALSE;
            }
          }

          JS_ResumeRequest(cx, ref);
          return JS_TRUE;

        } catch(const BusApiError& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Bus failure: %s", ex.what());
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dev_set_button_id

  JSFunctionSpec device_interface_methods[] = {
    JS_FS("turnOn", dev_turn_on, 0, 0),
    JS_FS("turnOff", dev_turn_off, 0, 0),
    JS_FS("blink", dev_blink, 1, 0),
    JS_FS("setValue", dev_set_value, 0, 0),
    JS_FS("increaseValue", dev_increase_value, 1, 0),
    JS_FS("decreaseValue", dev_decrease_value, 1, 0),
    JS_FS("callScene", dev_call_scene, 2, 0),
    JS_FS("saveScene", dev_save_scene, 1, 0),
    JS_FS("undoScene", dev_undo_scene, 1, 0),
    JS_FS("nextScene", dev_next_scene, 1, 0),
    JS_FS("previousScene", dev_previous_scene, 1, 0),
    JS_FS("getConfig", dev_get_config, 2, 0),
    JS_FS("getConfigWord", dev_get_config_word, 2, 0),
    JS_FS("setConfig", dev_set_config, 3, 0),
    JS_FS("getOutputValue", dev_get_output_value, 1, 0),
    JS_FS("setOutputValue", dev_set_output_value, 2, 0),
    JS_FS("getSensorValue", dev_get_sensor_value, 1, 0),
    JS_FS("getSensorType", dev_get_sensor_type, 1, 0),
    JS_FS("addStateSensor", dev_addStateSensor, 3, 0),
    JS_FS("getPropertyNode", dev_get_property_node, 0, 0),
    JS_FS("setJokerGroup", dev_set_joker_group, 1, 0),
    JS_FS("setButtonID", dev_set_button_id, 1, 0),
    JS_FS("setVdcProperty", dev_set_vdc_property, 2, 0),
    JS_FS("getVdcProperty", dev_get_vdc_property, 1, 0),
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
            std::string tmp = dsuid2str(dev->getDSID());
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
            std::string tmp = dsuid2str(dev->getDevice()->getDSMeterDSID());
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
        case 10:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, dev->getDevice()->getOemEanAsString().c_str())));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // dev_JSGet

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
    {"dsuid", 1, 0, dev_JSGet, NULL},
    {"name", 2, 0, dev_JSGet, NULL},
    {"zoneID", 3, 0, dev_JSGet, NULL},
    {"circuitID", 4, 0, dev_JSGet, NULL},
    {"functionID", 5, 0, dev_JSGet, NULL},
    {"lastCalledScene", 6, 0, dev_JSGet, NULL},
    {"revisionID", 7, 0, dev_JSGet, NULL},
    {"productID", 8, 0, dev_JSGet, NULL},
    {"isPresent", 9, 0, dev_JSGet, NULL},
    {"productEAN", 10, 0, dev_JSGet, NULL},
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
            std::string tmp = dsuid2str(meter->getDSID());
            JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, tmp.c_str())));
          }
          return JS_TRUE;
        case 2:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, meter->getName().c_str())));
          return JS_TRUE;
        case 3:
          JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(meter->isPresent()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // dsmeter_JSGet

  static JSPropertySpec dsmeter_properties[] = {
    {"className", 0, 0, dsmeter_JSGet, NULL},
    {"dsuid", 1, 0, dsmeter_JSGet, NULL},
    {"name", 2, 0, dsmeter_JSGet, NULL},
    {"present", 3, 0, dsmeter_JSGet, NULL},
    {NULL, 0, 0, NULL, NULL}
  };

  JSBool dsmeter_getPowerConsumption(JSContext* cx, uintN argc, jsval* vp) {
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;

    JS_SET_RVAL(cx, vp, JSVAL_NULL);
    try {
      if(meter != NULL) {
        try {
          if (!meter->getCapability_HasMetering()) {
            JS_ReportWarning(cx, "DSMeter does not support energy metering: %s",
                dsuid2str(meter->getDSID()).c_str());
            return JS_TRUE;
          }
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
          obj.setProperty<double>("value", meter->getCachedEnergyMeterValue());
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
      if (ext == NULL) {
        JS_ReportError(cx, "Model.dsmeter_get_property_node: ext of wrong type");
        return JS_FALSE;
      }
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

  JSBool dsmeter_setPowerStateConfig(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    boost::shared_ptr<DSMeter> meter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      ScriptObject self(JS_THIS_OBJECT(cx, vp), *ctx);
      if (self.is("DSMeter")) {
        if (argc == 3) {
          uint8_t index;
          uint32_t setThreshold;
          uint32_t resetThreshold;
          try {
            index = ctx->convertTo<uint8_t>(JS_ARGV(cx, vp)[0]);
            setThreshold = ctx->convertTo<uint32_t>(JS_ARGV(cx, vp)[1]);
            resetThreshold = ctx->convertTo<uint32_t>(JS_ARGV(cx, vp)[2]);
          } catch (ScriptException& ex) {
            JS_ReportError(cx, ex.what());
            return JS_FALSE;
          }
          jsrefcount ref = JS_SuspendRequest(cx);
          try {
            ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
                ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
            if (ext == NULL) {
              JS_ReportError(cx, "Model.dsmeter_setPowerStateConfig: ext of wrong type");
              return JS_FALSE;
            }
            StructureModifyingBusInterface *modInterface = ext->getApartment().getBusInterface()->getStructureModifyingBusInterface();
            modInterface->setCircuitPowerStateConfig(meter->getDSID(), index, setThreshold, resetThreshold);
            meter->setPowerState(index, setThreshold, resetThreshold);
            JS_ResumeRequest(cx, ref);
            JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
            return JS_TRUE;
          } catch (const BusApiError& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Bus failure: %s", ex.what());
          } catch (DSSException& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "Failure: %s", ex.what());
          } catch (std::exception& ex) {
            JS_ResumeRequest(cx, ref);
            JS_ReportError(cx, "General failure: %s", ex.what());
          }
        }
      }
    } catch (ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dsmeter_setPowerStateConfig

  JSBool dsmeter_vdc_callMethod(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
        ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
    if (ext == NULL) {
      JS_ReportError(cx, "ext of wrong type");
      return JS_FALSE;
    }

    try {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      if(argc == 2) {
        std::string method;
        std::string params;
        try {
          method = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
          params = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
        } catch (ScriptException& ex) {
          JS_ReportError(cx, "Convert arguments: %s", ex.what());
          return JS_FALSE;
        }
        jsrefcount ref = JS_SuspendRequest(cx);
        try {
          vdcapi::PropertyElement parsedParamsElement;
          const vdcapi::PropertyElement* paramsElement = &vdcapi::PropertyElement::default_instance();
          if (!params.empty()) {
            parsedParamsElement = ProtobufToJSon::jsonToElement(params);
            paramsElement = &parsedParamsElement;
          }

          boost::shared_ptr<DSMeter> pMeter = static_cast<meter_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pMeter;
          vdcapi::Message res = VdcConnection::genericRequest(pMeter->getDSID(), pMeter->getDSID(), method, paramsElement->elements());
          JSONWriter json(JSONWriter::jsonNoneResult);
          ProtobufToJSon::protoPropertyToJson(res, json);
          JS_ResumeRequest(cx, ref);
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, json.successJSON().c_str())));
          return JS_TRUE;
        } catch(const BusApiError& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Bus failure: %s", ex.what());
        } catch (DSSException& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "Failure: %s", ex.what());
        } catch (std::exception& ex) {
          JS_ResumeRequest(cx, ref);
          JS_ReportError(cx, "General failure: %s", ex.what());
        }
      }
    } catch(ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    }
    return JS_FALSE;
  } // dsmeter_vdc_callMethod

  JSFunctionSpec dsmeter_methods[] = {
    JS_FS("getPowerConsumption", dsmeter_getPowerConsumption, 0, 0),
    JS_FS("getEnergyMeterValue", dsmeter_getEnergyMeterValue, 0, 0),
    JS_FS("getCachedPowerConsumption", dsmeter_getCachedPowerConsumption, 0, 0),
    JS_FS("getCachedEnergyMeterValue", dsmeter_getCachedEnergyMeterValue, 0, 0),
    JS_FS("getPropertyNode", dsmeter_get_property_node, 0, 0),
    JS_FS("setPowerStateConfig", dsmeter_setPowerStateConfig, 0, 0),
    JS_FS("setVdcProperty", dev_set_vdc_property, 2, 0),
    JS_FS("getVdcProperty", dev_get_vdc_property, 1, 0),
    JS_FS("callVdcMethod", dsmeter_vdc_callMethod, 1, 0),
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

  //=== JSCluster ===

  struct cluster_wrapper {
    boost::shared_ptr<Cluster> pCluster;
  };

  void finalize_cluster(JSContext *cx, JSObject *obj) {
    struct cluster_wrapper* pCluster = static_cast<cluster_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pCluster;
  } // finalize_cluster

  static JSClass cluster_class = {
    "Cluster", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub, finalize_cluster, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSBool cluster_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    boost::shared_ptr<Cluster> pCluster = static_cast<cluster_wrapper*>(JS_GetPrivate(cx, obj))->pCluster;
    if(pCluster != NULL) {
      int opt = JSID_TO_INT(id);
      switch(opt) {
        case 0:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Cluster")));
          return JS_TRUE;
        case 1:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(pCluster->getID()));
          return JS_TRUE;
        case 2:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, pCluster->getName().c_str())));
          return JS_TRUE;
        case 3:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(static_cast<int>(pCluster->getApplicationType())));
          return JS_TRUE;
        case 4:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(pCluster->getLocation()));
          return JS_TRUE;
        case 5:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(pCluster->getProtectionClass()));
          return JS_TRUE;
        case 6:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL(pCluster->getFloor()));
          return JS_TRUE;
        case 7:
          JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(pCluster->isAutomatic()));
          return JS_TRUE;
        case 8:
          JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(pCluster->isConfigurationLocked()));
          return JS_TRUE;
        case 9:
          JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(pCluster->isOperationLock()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // cluster_JSGet

  static JSPropertySpec cluster_properties[] = {
    {"className",         0, 0, cluster_JSGet, NULL},
    {"id",                1, 0, cluster_JSGet, NULL},
    {"name",              2, 0, cluster_JSGet, NULL},
    {"standardGroup",     3, 0, cluster_JSGet, NULL},
    {"location",          4, 0, cluster_JSGet, NULL},
    {"protectionClass",   5, 0, cluster_JSGet, NULL},
    {"floor",             6, 0, cluster_JSGet, NULL},
    {"automatic",         7, 0, cluster_JSGet, NULL},
    {"configurationLock", 8, 0, cluster_JSGet, NULL},
    {"operationLock",     9, 0, cluster_JSGet, NULL},
    {NULL, 0, 0, NULL, NULL}
  };

  JSObject* ModelScriptContextExtension::createJSCluster(ScriptContext& _ctx, boost::shared_ptr<Cluster> _pCluster) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &cluster_class, NULL, NULL);
    JS_DefineProperties(_ctx.getJSContext(), result, cluster_properties);
    //JS_DefineFunctions(_ctx.getJSContext(), result, cluster_methods);
    struct cluster_wrapper* wrapper = new cluster_wrapper;
    wrapper->pCluster = _pCluster;
    JS_SetPrivate(_ctx.getJSContext(), result, wrapper);
    return result;
  } // createJSCluster

  //=== JSZone ===

  struct zone_wrapper {
    boost::shared_ptr<Zone> pZone;
  };

  void finalize_zone(JSContext *cx, JSObject *obj) {
    struct zone_wrapper* pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pZone;
  } // finalize_zone

  static JSClass zone_class = {
    "Zone", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub, finalize_zone, JSCLASS_NO_OPTIONAL_MEMBERS
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
        case 3:
          JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(pZone->isPresent()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  }

  static JSPropertySpec zone_properties[] = {
    {"className", 0, 0, zone_JSGet, NULL},
    {"id", 1, 0, zone_JSGet, NULL},
    {"name", 2, 0, zone_JSGet, NULL},
    {"present", 3, 0, zone_JSGet, NULL},
    {NULL, 0, 0, NULL, NULL}
  };

  JSBool zone_getDevices(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_getDevices: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      if(pZone != NULL) {
        Set devices = filterOutInvisibleDevices(pZone->getDevices());
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

  JSBool zone_callScene(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      int groupID;
      int sceneID;
      bool forcedCall = false;
      SceneAccessCategory category = SAC_UNKNOWN;
      if(pZone != NULL) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          sceneID = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
          if (argc >= 3) {
            forcedCall = ctx->convertTo<bool>(JS_ARGV(cx, vp)[2]);
          }
          if (argc >= 4) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[3]));
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        pGroup->callScene(coJSScripting, category, sceneID, "", forcedCall);

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
  } // zone_callScene

  JSBool  zone_callSceneMin(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      int groupID;
      int sceneID;
      SceneAccessCategory category = SAC_UNKNOWN;
      if(pZone != NULL) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          sceneID = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
          if (argc >= 3) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]));
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        pGroup->callSceneMin(coJSScripting, category, sceneID, "");
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
  } // zone_callSceneMin

  JSBool zone_callSceneSys(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      int groupID;
      int sceneID;
      bool forcedCall = false;
      SceneAccessCategory category = SAC_UNKNOWN;
      if(pZone != NULL) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          sceneID = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
          if (argc >= 3) {
            forcedCall = ctx->convertTo<bool>(JS_ARGV(cx, vp)[2]);
          }
          if (argc >= 4) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[3]));
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        pGroup->callScene(coSystem, category, sceneID, "", forcedCall);

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
  } // zone_callSceneSys

  JSBool zone_undoScene(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      int groupID;
      int sceneID;
      SceneAccessCategory category = SAC_UNKNOWN;
      if(pZone != NULL) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          sceneID = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
          if (argc >= 3) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]));
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        pGroup->undoScene(coJSScripting, category, sceneID, "");

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
  } // zone_undoScene

  JSBool zone_undoSceneSys(JSContext* cx, uintN argc, jsval* vp) {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      try {
        boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
        int groupID;
        int sceneID;
        SceneAccessCategory category = SAC_UNKNOWN;
        if(pZone != NULL) {
          try {
            groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
            sceneID = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
            if (argc >= 3) {
              category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]));
            }
          } catch(ScriptException& e) {
            JS_ReportError(cx, e.what());
            return JS_FALSE;
          } catch(std::invalid_argument& e) {
            JS_ReportError(cx, e.what());
            return JS_FALSE;
          }

          boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
          pGroup->undoScene(coSystem, category, sceneID, "");

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
    } // zone_undoSceneSys

  JSBool zone_blink(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      int groupID = 0;
      SceneAccessCategory category = SAC_UNKNOWN;
      if(pZone != NULL) {
        if (argc >= 1) {
          try {
            groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
            if (argc >= 2) {
              category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]));
            }
          } catch(ScriptException& e) {
            JS_ReportError(cx, e.what());
            return JS_FALSE;
          } catch(std::invalid_argument& e) {
            JS_ReportError(cx, e.what());
            return JS_FALSE;
          }
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        if (!pGroup) {
          JS_ReportWarning(cx, "Zone.blink: group with id \"%d\" not found", groupID);
          return JS_FALSE;
        }
        pGroup->blink(coJSScripting, category, "");

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
  } // zone_blink

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
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_pushSensorValue: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      uint8_t groupID;
      dsuid_t sourceDSID;
      SensorType sensorType;
      uint16_t sensorValue;
      SceneAccessCategory category = SAC_UNKNOWN;
      if (pZone != NULL && argc >= 4) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          std::string sDSID = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
          if (sDSID.length() == 0) {
            sourceDSID = DSUID_NULL;
          } else if (sDSID.length() == 24) {
            dsid_t dsid = str2dsid(sDSID);
            sourceDSID = dsuid_from_dsid(&dsid);
          } else {
            sourceDSID = str2dsuid(sDSID);
          }
          sensorType = static_cast<SensorType>(ctx->convertTo<int>(JS_ARGV(cx, vp)[2]));
          sensorValue = ctx->convertTo<uint16_t>(JS_ARGV(cx, vp)[3]);
          if (argc >= 5) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[4]));
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }
        if (sensorValue >= (1 << 10)) {
          JS_ReportWarning(cx, "sensor value too large: %d", sensorValue);
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        double sensorValueFloat = sensorValueToDouble(sensorType, sensorValue);
        pGroup->pushSensor(coJSScripting, category, sourceDSID, sensorType, sensorValueFloat, "");

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

  JSBool zone_pushSensorValueFloat(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_pushSensorValueFloat: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      uint8_t groupID;
      dsuid_t sourceDSID;
      SensorType sensorType;
      double sensorValue;
      SceneAccessCategory category = SAC_UNKNOWN;
      if (pZone != NULL && argc >= 4) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
          std::string sDSID = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
          if (sDSID.length() == 0) {
            sourceDSID = DSUID_NULL;
          } else if (sDSID.length() == 24) {
            dsid_t dsid = str2dsid(sDSID);
            sourceDSID = dsuid_from_dsid(&dsid);
          } else {
            sourceDSID = str2dsuid(sDSID);
          }
          sensorType = static_cast<SensorType>(ctx->convertTo<uint8_t>(JS_ARGV(cx, vp)[2]));
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        try {
          if (JSVAL_IS_DOUBLE(JS_ARGV(cx, vp)[3])) {
            sensorValue = ctx->convertTo<double>(JS_ARGV(cx, vp)[3]);
          } else {
            std::string svalue = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[3]);
            sensorValue = ::strtod(svalue.c_str(), 0);
          }
          if (argc >= 5) {
            category = SceneAccess::stringToCategory(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[4]));
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        pGroup->pushSensor(coJSScripting, category, sourceDSID, sensorType, sensorValue, "");

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
  } // zone_pushSensorValueFloat

  JSBool zone_get_property_node(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(
          ctx->getEnvironment().getExtension("propertyextension"));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_get_property_node: ext of wrong type");
        return JS_FALSE;
      }
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

  JSBool zone_addConnectedDevice(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_addConnectedDevice: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      uint8_t groupID;
      if (pZone != NULL && argc >= 1) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        if (!pGroup) {
          JS_ReportWarning(cx, "Model.zone_addConnectedDevice: group with id \"%d\" not found", groupID);
          return JS_FALSE;
        }
        pGroup->addConnectedDevice();
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
  } // zone_addConnectedDevice

  JSBool zone_removeConnectedDevice(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_removeConnectedDevice: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      uint8_t groupID;
      if (pZone != NULL && argc >= 1) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        if (!pGroup) {
          JS_ReportWarning(cx, "Model.zone_removeConnectedDevice: group with id \"%d\" not found", groupID);
          return JS_FALSE;
        }
        pGroup->removeConnectedDevice();
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
  } // zone_removeConnectedDevice

  JSBool zone_getTemperatureControlStatus(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_getTemperatureControlStatus: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      const ZoneHeatingProperties_t& hProp = pZone->getHeatingProperties();
      ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
      ZoneSensorStatus_t hSensors = pZone->getSensorStatus();

      ScriptObject obj(*ctx, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));

      obj.setProperty<int>("ControlMode", static_cast<uint8_t>(hProp.m_mode));
      switch (hProp.m_mode) {
        case HeatingControlMode::OFF:
          break;
        case HeatingControlMode::PID:
          obj.setProperty<int>("OperationMode", pZone->getHeatingOperationMode());
          if (hSensors.m_TemperatureValue) {
            obj.setProperty<double>("TemperatureValue", *hSensors.m_TemperatureValue);
          } else {
            obj.setPropertyNull("TemperatureValue");
          }
          obj.setProperty<std::string>("TemperatureValueTime", hSensors.m_TemperatureValueTS.toISO8601());
          if (hStatus.m_NominalValue) {
            obj.setProperty<double>("NominalValue", *hStatus.m_NominalValue);
          } else {
            obj.setPropertyNull("NominalValue");
          }
          obj.setProperty<std::string>("NominalValueTime", hStatus.m_NominalValueTS.toISO8601());
          if (hStatus.m_ControlValue) {
            obj.setProperty<double>("ControlValue", *hStatus.m_ControlValue);
          } else {
            obj.setPropertyNull("ControlValue");
          }
          obj.setProperty<std::string>("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
          break;
        case HeatingControlMode::ZONE_FOLLOWER:
          if (hStatus.m_ControlValue) {
            obj.setProperty<double>("ControlValue", *hStatus.m_ControlValue);
          } else {
            obj.setPropertyNull("ControlValue");
          }
          obj.setProperty<std::string>("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
          break;
        case HeatingControlMode::FIXED:
        case HeatingControlMode::MANUAL:
          obj.setProperty<int>("OperationMode", pZone->getHeatingOperationMode());
          if (hStatus.m_ControlValue) {
            obj.setProperty<double>("ControlValue", *hStatus.m_ControlValue);
          } else {
            obj.setPropertyNull("ControlValue");
          }
          break;
      }
      return JS_TRUE;

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
  } // zone_getTemperatureControlStatus

  JSBool zone_getTemperatureControlConfiguration(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_getTemperatureControlConfiguration: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      const ZoneHeatingProperties_t& hProp = pZone->getHeatingProperties();

      ScriptObject obj(*ctx, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));

      obj.setProperty<int>("ControlMode", static_cast<uint8_t>(hProp.m_mode));
      switch (hProp.m_mode) {
        case HeatingControlMode::OFF:
          break;
        case HeatingControlMode::PID:
          obj.setProperty<int>("EmergencyValue", hProp.m_EmergencyValue - 100);
          obj.setProperty<double>("CtrlKp", (double)hProp.m_Kp * 0.025);
          obj.setProperty<int>("CtrlTs", hProp.m_Ts);
          obj.setProperty<int>("CtrlTi", hProp.m_Ti);
          obj.setProperty<int>("CtrlKd", hProp.m_Kd);
          obj.setProperty<double>("CtrlImin", (double)hProp.m_Imin * 0.025);
          obj.setProperty<double>("CtrlImax", (double)hProp.m_Imax * 0.025);
          obj.setProperty<int>("CtrlYmin", hProp.m_Ymin - 100);
          obj.setProperty<int>("CtrlYmax", hProp.m_Ymax - 100);
          obj.setProperty<bool>("CtrlAntiWindUp", (hProp.m_AntiWindUp > 0));
          obj.setProperty<bool>("CtrlKeepFloorWarm", (hProp.m_KeepFloorWarm > 0));
          break;
        case HeatingControlMode::ZONE_FOLLOWER:
          obj.setProperty<int>("ReferenceZone", hProp.m_HeatingMasterZone);
          obj.setProperty<int>("CtrlOffset", hProp.m_CtrlOffset);
          break;
        case HeatingControlMode::MANUAL:
          obj.setProperty<int>("ManualValue", hProp.m_ManualValue - 100);
          break;
        case HeatingControlMode::FIXED:
          break;
      }
      return JS_TRUE;

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
  } // zone_getTemperatureControlConfiguration

  JSBool zone_getTemperatureControlConfiguration2(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_getTemperatureControlConfiguration2: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      const ZoneHeatingProperties_t& hProp = pZone->getHeatingProperties();

      ScriptObject obj(*ctx, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));

      if (auto heatingName = heatingControlModeName(hProp.m_mode)) {
        obj.setProperty<std::string>("mode", *heatingName);
      } else {
        obj.setProperty<std::string>("mode", std::string("unknown"));
      }

      ScriptObject controlModeObj(*ctx, NULL);
      controlModeObj.setProperty<int>("emergencyValue", hProp.m_EmergencyValue - 100);
      controlModeObj.setProperty<double>("ctrlKp", (double)hProp.m_Kp * 0.025);
      controlModeObj.setProperty<int>("ctrlTs", hProp.m_Ts);
      controlModeObj.setProperty<int>("ctrlTi", hProp.m_Ti);
      controlModeObj.setProperty<int>("ctrlKd", hProp.m_Kd);
      controlModeObj.setProperty<double>("ctrlImin", (double)hProp.m_Imin * 0.025);
      controlModeObj.setProperty<double>("ctrlImax", (double)hProp.m_Imax * 0.025);
      controlModeObj.setProperty<int>("ctrlYmin", hProp.m_Ymin - 100);
      controlModeObj.setProperty<int>("ctrlYmax", hProp.m_Ymax - 100);
      controlModeObj.setProperty<bool>("ctrlAntiWindUp", (hProp.m_AntiWindUp > 0));
      obj.setProperty("controlMode", &controlModeObj);

      ScriptObject zoneFollowerModeObj(*ctx, NULL);
      zoneFollowerModeObj.setProperty<int>("referenceZone", hProp.m_HeatingMasterZone);
      zoneFollowerModeObj.setProperty<int>("ctrlOffset", hProp.m_CtrlOffset);
      obj.setProperty("zoneFollowerMode", &zoneFollowerModeObj);

      ScriptObject manualModeObj(*ctx, NULL);
      manualModeObj.setProperty<int>("controlValue", hProp.m_ManualValue - 100);
      obj.setProperty("manualMode", &manualModeObj);

      switch (hProp.m_mode) {
        case HeatingControlMode::OFF:
          break;
        case HeatingControlMode::PID: {
          ScriptObject targetTemperaturesObj(*ctx, NULL);
          for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
            targetTemperaturesObj.setProperty<double>(ds::str(i), hProp.m_TeperatureSetpoints[i]);
          }
          obj.setProperty("targetTemperatures", &targetTemperaturesObj);
        } break;
        case HeatingControlMode::ZONE_FOLLOWER:
          break;
        case HeatingControlMode::MANUAL:
          break;
        case HeatingControlMode::FIXED: {
          ScriptObject fixedValuesObj(*ctx, NULL);
          for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
            fixedValuesObj.setProperty<double>(ds::str(i), hProp.m_FixedControlValues[i]);
          }
          obj.setProperty("fixedValues", &fixedValuesObj);
        } break;
      }

      return JS_TRUE;

    } catch (ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }

    return JS_FALSE;
  } // zone_getTemperatureControlConfiguration2

  JSBool zone_getTemperatureControlValues(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_getTemperatureControlValues: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;

      ScriptObject obj(*ctx, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));

      const ZoneHeatingProperties_t& hProp = pZone->getHeatingProperties();

      switch (hProp.m_mode) {
        case HeatingControlMode::OFF:
          break;
        case HeatingControlMode::PID:
					obj.setProperty<double>("Off", hProp.m_TeperatureSetpoints[0]);
          obj.setProperty<double>("Comfort", hProp.m_TeperatureSetpoints[1]);
          obj.setProperty<double>("Economy", hProp.m_TeperatureSetpoints[2]);
          obj.setProperty<double>("NotUsed", hProp.m_TeperatureSetpoints[3]);
          obj.setProperty<double>("Night", hProp.m_TeperatureSetpoints[4]);
          obj.setProperty<double>("Holiday", hProp.m_TeperatureSetpoints[5]);
          obj.setProperty<double>("Cooling", hProp.m_TeperatureSetpoints[6]);
          obj.setProperty<double>("CoolingOff", hProp.m_TeperatureSetpoints[7]);
          break;
        case HeatingControlMode::ZONE_FOLLOWER:
          break;
        case HeatingControlMode::FIXED:
					obj.setProperty<double>("Off", hProp.m_FixedControlValues[0]);
          obj.setProperty<double>("Comfort", hProp.m_FixedControlValues[1]);
          obj.setProperty<double>("Economy", hProp.m_FixedControlValues[2]);
          obj.setProperty<double>("NotUsed", hProp.m_FixedControlValues[3]);
          obj.setProperty<double>("Night", hProp.m_FixedControlValues[4]);
          obj.setProperty<double>("Holiday", hProp.m_FixedControlValues[5]);
          obj.setProperty<double>("Cooling", hProp.m_FixedControlValues[6]);
          obj.setProperty<double>("CoolingOff", hProp.m_FixedControlValues[7]);
          break;
        case HeatingControlMode::MANUAL:
          break;
      }
      return JS_TRUE;

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
  } // zone_getTemperatureControlValues

  JSBool zone_setTemperatureControlConfiguration(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_setTemperatureControlConfiguration: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      ZoneHeatingConfigSpec_t hConfig = pZone->getHeatingConfig();

      StructureManipulator manipulator(
          *ext->getApartment().getBusInterface()->getStructureModifyingBusInterface(),
          *ext->getApartment().getBusInterface()->getStructureQueryBusInterface(),
          ext->getApartment());

      // make sure that the input parameter is valid
      DS_REQUIRE((argc == 1) && (JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[0])));

      JSObject* configObj = JSVAL_TO_OBJECT(JS_ARGV(cx, vp) [0]);
      JSObject* propIter = JS_NewPropertyIterator(cx, configObj);
      jsid propID;
      while(JS_NextProperty(cx, propIter, &propID) == JS_TRUE) {
        if (JSID_IS_VOID(propID)) {
          break;
        }
        JSObject* obj;
        jsval arg1, arg2;
        JS_GetMethodById(cx, configObj, propID, &obj, &arg1);
        JSString *val = JS_ValueToString(cx, arg1);
        char* propValue = JS_EncodeString(cx, val);
        JS_IdToValue(cx, propID, &arg2);
        val = JS_ValueToString(cx, arg2);
        char* propKey = JS_EncodeString(cx, val);

        int intValue = strtol(propValue, NULL, 10);
        if (strcmp(propKey, "ControlMode") == 0) {
          hConfig.mode = static_cast<HeatingControlMode>(intValue);
        } else if (strcmp(propKey, "ReferenceZone") == 0) {
          hConfig.SourceZoneId = intValue;
        } else if (strcmp(propKey, "CtrlOffset") == 0) {
          hConfig.Offset = intValue;
        } else if (strcmp(propKey, "EmergencyValue") == 0) {
          hConfig.EmergencyValue = intValue;
          hConfig.EmergencyValue += 100;
        } else if (strcmp(propKey, "ManualValue") == 0) {
          hConfig.ManualValue = intValue;
          hConfig.ManualValue += 100;
        } else if (strcmp(propKey, "CtrlYmin") == 0) {
          hConfig.Ymin = intValue;
          hConfig.Ymin += 100;
        } else if (strcmp(propKey, "CtrlYmax") == 0) {
          hConfig.Ymax = intValue;
          hConfig.Ymax += 100;
        } else if (strcmp(propKey, "CtrlKp") == 0) {
          double doubleValue = strToDouble(propValue, 0);
          hConfig.Kp = doubleValue * 40;
        } else if (strcmp(propKey, "CtrlTi") == 0) {
          hConfig.Ti = intValue;
        } else if (strcmp(propKey, "CtrlTs") == 0) {
          hConfig.Ts = intValue;
        } else if (strcmp(propKey, "CtrlKd") == 0) {
          hConfig.Kd = intValue;
        } else if (strcmp(propKey, "CtrlImin") == 0) {
          double doubleValue = strToDouble(propValue, 0);
          hConfig.Imin = doubleValue * 40;
        } else if (strcmp(propKey, "CtrlImax") == 0) {
          double doubleValue = strToDouble(propValue, 0);
          hConfig.Imax = doubleValue * 40;
        } else if (strcmp(propKey, "CtrlAntiWindUp") == 0) {
          hConfig.AntiWindUp = ctx->convertTo<bool>(arg1) ? 1 : 0;;
        } else if (strcmp(propKey, "CtrlKeepFloorWarm") == 0) {
          hConfig.KeepFloorWarm = ctx->convertTo<bool>(arg1) ? 1 : 0;
        } else {
          JS_ReportWarning(cx, "Model.zone_setTemperatureControlConfiguration: unknown configuration \"%s\"", propKey);
        }
        JS_free(cx, propValue);
        JS_free(cx, propKey);
      }

      manipulator.setZoneHeatingConfig(pZone, hConfig);

      JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(false));
      return JS_TRUE;

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
  } // zone_setTemperatureControlConfiguration

  JSBool zone_setTemperatureControlConfiguration2(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_setTemperatureControlConfiguration2: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      ZoneHeatingConfigSpec_t hConfig = pZone->getHeatingConfig();

      auto structureModifyingBusInterface = ext->getApartment().getBusInterface()->getStructureModifyingBusInterface();
      auto structureQueryBusInterface = ext->getApartment().getBusInterface()->getStructureQueryBusInterface();
      StructureManipulator manipulator(
          *structureModifyingBusInterface, *structureQueryBusInterface, ext->getApartment());

      JSObject* configObj = JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[0]);
      JSObject* propIter = JS_NewPropertyIterator(cx, configObj);
      jsid propID;
      while (JS_NextProperty(cx, propIter, &propID) == JS_TRUE) {
        if (JSID_IS_VOID(propID)) {
          break;
        }
        JSObject* obj;
        jsval jsvPropValue, jsvPropKey;

        JS_GetMethodById(cx, configObj, propID, &obj, &jsvPropValue);
        JS_IdToValue(cx, propID, &jsvPropKey);

        std::string key = ctx->convertTo<std::string>(jsvPropKey);

        if (key == "mode") {
          if (auto heatingMode = heatingControlModeFromName(ctx->convertTo<std::string>(jsvPropValue))) {
            hConfig.mode = *heatingMode;
          }
        } else if (key == "targetTemperatures") {
          ZoneHeatingOperationModeSpec_t hOpValues = pZone->getHeatingControlOperationModeValues();

          // update the temperatures
          std::string jsonObj = ctx->jsonStringify(jsvPropValue);
          ZoneHeatingProperties::parseTargetTemperatures(jsonObj, hOpValues);

          // set data in model
          pZone->setHeatingControlOperationMode(hOpValues);
        } else if (key == "fixedValues") {
          ZoneHeatingOperationModeSpec_t hOpValues = pZone->getHeatingFixedOperationModeValues();

          // update the temperatures
          std::string jsonObj = ctx->jsonStringify(jsvPropValue);
          ZoneHeatingProperties::parseFixedValues(jsonObj, hOpValues);

          // set data in model
          pZone->setHeatingFixedOperationMode(hOpValues);
        } else if (key == "controlMode") {
          // update the control values
          std::string jsonObj = ctx->jsonStringify(jsvPropValue);
          ZoneHeatingProperties::parseControlMode(jsonObj, hConfig);
        } else if (key == "zoneFollowerMode") {
          // update the control values
          std::string jsonObj = ctx->jsonStringify(jsvPropValue);
          ZoneHeatingProperties::parseFollowerMode(jsonObj, hConfig);
        } else if (key == "manualMode") {
          // update the control values
          std::string jsonObj = ctx->jsonStringify(jsvPropValue);
          ZoneHeatingProperties::parseManualMode(jsonObj, hConfig);
        } else {
          JS_ReportWarning(
              cx, "Model.zone_setTemperatureControlConfiguration2: unknown configuration \"%s\"", key.c_str());
        }
      }

      // set data in model and dsms
      manipulator.setZoneHeatingConfig(pZone, hConfig);

      JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(false));
      return JS_TRUE;

    } catch (ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }

    return JS_FALSE;
  } // zone_setTemperatureControlConfiguration2

  JSBool zone_setTemperatureControlValues(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_setTemperatureControlValues: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      const ZoneHeatingProperties_t& hProp = pZone->getHeatingProperties();
      ZoneHeatingOperationModeSpec_t hOpValues = pZone->getHeatingOperationModeValues();
      SensorType SensorConversion;

      if (hProp.m_mode == HeatingControlMode::PID) {
        SensorConversion = SensorType::RoomTemperatureSetpoint;
      } else if (hProp.m_mode == HeatingControlMode::FIXED) {
        SensorConversion = SensorType::RoomTemperatureControlVariable;
      } else {
        JS_ReportError(cx, "Model.zone_setTemperatureControlValues: cannot set control values in current mode");
        return JS_FALSE;
      }

      JSObject* configObj = JSVAL_TO_OBJECT(JS_ARGV(cx, vp) [0]);
      if (!configObj) {
        JS_ReportWarning(cx, "Model.zone_setTemperatureControlValues: invalid parameter data");
        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(false));
        return JS_TRUE;
      }

      JSObject* propIter = JS_NewPropertyIterator(cx, configObj);
      jsid propID;
      while(JS_NextProperty(cx, propIter, &propID) == JS_TRUE) {
        if (JSID_IS_VOID(propID)) {
          break;
        }
        JSObject* obj;
        jsval arg1, arg2;
        JS_GetMethodById(cx, configObj, propID, &obj, &arg1);
        JSString *val = JS_ValueToString(cx, arg1);
        char* sValue = JS_EncodeString(cx, val);
        JS_IdToValue(cx, propID, &arg2);
        val = JS_ValueToString(cx, arg2);
        char* sKey = JS_EncodeString(cx, val);

        std::string propKey(sKey);
        std::string propValue(sValue);
        JS_free(cx, sKey);
        JS_free(cx, sValue);

        double fValue = strToDouble(propValue);
        if (propKey == "Off") {
          hOpValues.opModes[0] = doubleToSensorValue(SensorConversion, fValue);
        } else if (propKey == "Comfort") {
          hOpValues.opModes[1] = doubleToSensorValue(SensorConversion, fValue);
        } else if (propKey == "Economy") {
          hOpValues.opModes[2] = doubleToSensorValue(SensorConversion, fValue);
        } else if (propKey == "NotUsed") {
          hOpValues.opModes[3] = doubleToSensorValue(SensorConversion, fValue);
        } else if (propKey == "Night") {
          hOpValues.opModes[4] = doubleToSensorValue(SensorConversion, fValue);
        } else if (propKey == "Holiday") {
          hOpValues.opModes[5] = doubleToSensorValue(SensorConversion, fValue);
        } else if (propKey == "Cooling") {
          hOpValues.opModes[6] = doubleToSensorValue(SensorConversion, fValue);
        } else if (propKey == "CoolingOff") {
          hOpValues.opModes[7] = doubleToSensorValue(SensorConversion, fValue);
        } else {
          JS_ReportWarning(cx, "Model.zone_setTemperatureControlValues: unknown opmode \"%s\"", propKey.c_str());
        }
      }

      // set the requested data in the zone model
      pZone->setHeatingOperationMode(hOpValues);

      // send current operation mode to all dsms
      ext->getApartment().getBusInterface()->getStructureModifyingBusInterface()->setZoneHeatingOperationModes(
          DSUID_BROADCAST, pZone->getID(), hOpValues);

      JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
      return JS_TRUE;

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
  } // zone_setTemperatureControlValues

  JSBool zone_getAssignedSensor(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_getAssignedSensor: ext of wrong type");
        return JS_FALSE;
      }
      auto sensorType = SensorType::UnknownType;
      if (argc >= 1) {
        try {
          sensorType = static_cast<SensorType>(ctx->convertTo<int>(JS_ARGV(cx, vp)[0]));
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }
      } else {
        JS_ReportError(cx, "Model.zone_getAssignedSensor: parameter sensortype missing");
        return JS_FALSE;
      }

      ScriptObject obj(*ctx, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj.getJSObject()));
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      boost::shared_ptr<Device> pSensor = pZone->getAssignedSensorDevice(sensorType);

      obj.setProperty<int>("type", static_cast<int>(sensorType));
      if (!pSensor) {
        obj.setProperty<bool>("dsuid", false);
      } else {
        obj.setProperty<std::string>("dsuid", dsuid2str(pSensor->getDSID()));
      }
      return JS_TRUE;

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
  } // zone_getAssignedSensor

  // zone.addStateSensor(int gropNumber, int sensorType, string Active-Condition, string Inactive-Condition)
  JSBool zone_addStateSensor(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_addStateSensor: ext of wrong type");
        return JS_FALSE;
      }
      if (argc < 4) {
        JS_ReportError(cx, "Model.zone_addStateSensor: missing arguments");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      if (NULL == pZone) {
        JS_ReportError(cx, "Model.zone_addStateSensor: zone object error");
        return JS_FALSE;
      }

      int groupNumber;
      SensorType sensorType;
      std::string activateCondition;
      std::string deactivateCondition;
      std::string creatorId = "default";

      try {
        groupNumber = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
        sensorType = static_cast<SensorType>(ctx->convertTo<int>(JS_ARGV(cx, vp)[1]));
        activateCondition = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]);
        deactivateCondition = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[3]);
        if (argc >= 5) {
          creatorId = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[4]);
        }
      } catch(ScriptException& e) {
        JS_ReportError(cx, e.what());
        return JS_FALSE;
      } catch(std::invalid_argument& e) {
        JS_ReportError(cx, e.what());
        return JS_FALSE;
      }

      boost::shared_ptr<Group> pGroup = pZone->getGroup(groupNumber);
      std::string identifier = ctx->getWrapper() ? ctx->getWrapper()->getIdentifier() : "";
      boost::shared_ptr<StateSensor> state = boost::make_shared<StateSensor> (
          creatorId, identifier, pGroup, sensorType, activateCondition, deactivateCondition);
      std::string sName = state->getName();
      state->setPersistence(true);
      try {
        ext->getApartment().allocateState(state);
      } catch(ItemDuplicateException& e) {
        JS_ReportWarning(cx, "Zone sensor state %s already exists: %s", sName.c_str(), e.what());
      }
      JSString* str = JS_NewStringCopyN(cx, sName.c_str(), sName.size());
      JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(str));
      return JS_TRUE;

    } catch(ItemDuplicateException& ex) {
      JS_ReportWarning(cx, "Item duplicate: %s", ex.what());
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
  } // zone_addStateSensor

  // zone.addDevice(string dsuid)
  JSBool zone_addDevice(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_addDevice: ext of wrong type");
        return JS_FALSE;
      }
      if (argc < 1) {
        JS_ReportError(cx, "Model.zone_addDevice: missing arguments");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      if (NULL == pZone) {
        JS_ReportError(cx, "Model.zone_addDevice: zone object error");
        return JS_FALSE;
      }

      StructureManipulator manipulator(
          *ext->getApartment().getBusInterface()->getStructureModifyingBusInterface(),
          *ext->getApartment().getBusInterface()->getStructureQueryBusInterface(),
          ext->getApartment());

      dsuid_t dsuid;
      try {
        dsuid = str2dsuid(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
      } catch(ScriptException& e) {
        JS_ReportError(cx, e.what());
        return JS_FALSE;
      } catch(std::invalid_argument& e) {
        JS_ReportError(cx, e.what());
        return JS_FALSE;
      } catch (std::runtime_error& e) {
        JS_ReportError(cx, e.what());
        return JS_FALSE;
      }

      boost::shared_ptr<Device> dev = DSS::getInstance()->getApartment().getDeviceByDSID(dsuid);
      if(!dev->isPresent()) {
        JS_ReportError(cx, "cannot add nonexisting device to a zone");
        return JS_FALSE;
      }

      if (dev->getZoneID() == pZone->getID()) {
        JS_ReportError(cx, "device is already in zone");
        return JS_FALSE;
      }

      try {
        manipulator.addDeviceToZone(dev, pZone);
        if (dev->is2WayMaster()) {
          dsuid_t next;
          dsuid_get_next_dsuid(dev->getDSID(), &next);
          try {
            boost::shared_ptr<Device> pPartnerDevice;
            pPartnerDevice = DSS::getInstance()->getApartment().getDeviceByDSID(next);
            manipulator.addDeviceToZone(pPartnerDevice, pZone);
          } catch(std::runtime_error& e) {
            JS_ReportError(cx, "could not find partner device");
            return JS_FALSE;
          }
        } else if (dev->getOemInfoState() == DEVICE_OEM_VALID) {
          uint16_t serialNr = dev->getOemSerialNumber();
          if ((serialNr > 0) & !dev->getOemIsIndependent()) {
            std::vector<boost::shared_ptr<Device> > devices = DSS::getInstance()->getApartment().getDevicesVector();
            foreach (const boost::shared_ptr<Device>& device, devices) {
              if (dev->isOemCoupledWith(device)) {
                manipulator.addDeviceToZone(device, pZone);
              }
            } // foreach
          } // if serial
        } else if (dev->isMainDevice() && (dev->getPairedDevices() > 0)) {
           bool doSleep = false;
           dsuid_t next;
           dsuid_t current = dev->getDSID();
           for (int p = 0; p < dev->getPairedDevices(); p++) {
             dsuid_get_next_dsuid(current, &next);
             current = next;
             try {
               boost::shared_ptr<Device> pPartnerDevice;
               pPartnerDevice = DSS::getInstance()->getApartment().getDeviceByDSID(next);
               if (!pPartnerDevice->isVisible()) {
                 if (doSleep) {
                   usleep(500 * 1000); // 500ms
                 }
                 manipulator.addDeviceToZone(pPartnerDevice, pZone);
               }
             } catch(std::runtime_error& e) {
               Logger::getInstance()->log(std::string("JS: coult not find partner device widh dsuid ") + dsuid2str(next), lsError);
             }
           }
         }
      } catch(ItemNotFoundException& ex) {
        JS_ReportError(cx, ex.what());
        return JS_FALSE;
      }
      return JS_TRUE;
    } catch(ItemDuplicateException& ex) {
      JS_ReportWarning(cx, "Item duplicate: %s", ex.what());
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
  } // zone_addDevice

  // setStatusField(int groupID, string field, string value)
  JSBool zone_setStatusField(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      DS_REQUIRE(ext, "Model.zone_setGroupConfiguration: ext of wrong type");
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      DS_REQUIRE(argc >= 3);
      auto&& groupId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      auto&& group = pZone->getGroup(groupId);
      DS_REQUIRE(group, "Failed to find group", groupId);
      group->setStatusField(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]),
          ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]));
      return JS_TRUE;
    } catch (std::exception& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    }
    return JS_FALSE;
  }

  JSBool zone_getGroupConfiguration(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_getGroupConfiguration: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      uint8_t groupID;
      if (pZone != NULL && argc >= 1) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
        } catch (ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch (std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        if (!pGroup) {
          JS_ReportWarning(cx, "Model.zone_getGroupConfiguration: group with id \"%d\" not found", groupID);
          return JS_FALSE;
        }

        // Currently configuration for zone related groups in zone 0 are undefined
        if ((pZone->getID() == 0) && (isDefaultGroup(pGroup->getID()))) {
          JS_ReportWarning(cx,
              "Model.zone_getGroupConfiguration: Configuration for group with id \"%d\" in zone 0 is not defined",
              groupID);
          return JS_FALSE;
        }

        // read configuration from group and serialize it to JSON object
        std::string configJson = pGroup->serializeApplicationConfiguration(pGroup->getApplicationConfiguration());
        JSString* str = JS_NewStringCopyN(cx, configJson.c_str(), configJson.size());
        JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(str));
        return JS_TRUE;
      }
    } catch (ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // zone_getGroupConfiguration

  // this function provide 3 overloads:
  // setGroupConfiguration(int groupID) - sets default configuration for group
  // setGroupConfiguration(int groupID, string jsonConfiguration) - sets the configuration written in JSON format without changing the application Type.
  //                                                                Cannot be used for cluster and user groups.
  // setGroupConfiguration(int groupID, int applicationType, string jsonConfiguration) - sets the configuration for specified application type.
  JSBool zone_setGroupConfiguration(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(
          ctx->getEnvironment().getExtension(ModelScriptcontextExtensionName));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.zone_setGroupConfiguration: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<Zone> pZone = static_cast<zone_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pZone;
      uint8_t groupID;
      std::string jsonConfiguration = "{}";
      int applicationType = -1;
      if (pZone != NULL && argc >= 1) {
        try {
          groupID = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);

          if (argc == 2) {
            jsonConfiguration = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
          } else if (argc > 2) {
            applicationType = ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
            jsonConfiguration = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[2]);
          }
        } catch (ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch (std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        }

        boost::shared_ptr<Group> pGroup = pZone->getGroup(groupID);
        if (!pGroup) {
          JS_ReportWarning(cx, "Model.zone_setGroupConfiguration: group with id \"%d\" not found", groupID);
          return JS_FALSE;
        }

        // Currently configuration for zone related groups in zone 0 are undefined
        if ((pZone->getID() == 0) && (isDefaultGroup(pGroup->getID()))) {
          JS_ReportWarning(cx,
              "Model.zone_setGroupConfiguration: Configuration for group with id \"%d\" in zone 0 is not defined",
              groupID);
          return JS_FALSE;
        }

        // if the applicationType was not provided we assume current group application type, but only for dS defined
        // groups
        if (applicationType < 0) {
          if (isDefaultGroup(pGroup->getID()) || isGlobalAppDsGroup(pGroup->getID())) {
            applicationType = static_cast<int>(pGroup->getApplicationType());
          } else {
            JS_ReportWarning(cx, "Model.zone_setGroupConfiguration: Invalid application Type \"%d\"", applicationType);
            return JS_FALSE;
          }
        }

        StructureManipulator manipulator(*ext->getApartment().getBusInterface()->getStructureModifyingBusInterface(),
            *ext->getApartment().getBusInterface()->getStructureQueryBusInterface(), ext->getApartment());

        // set the configuration in selected group
        manipulator.groupSetApplication(pGroup, static_cast<ApplicationType>(applicationType),
            pGroup->deserializeApplicationConfiguration(jsonConfiguration));

        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(true));
        return JS_TRUE;
      }
    } catch (ItemNotFoundException& ex) {
      JS_ReportWarning(cx, "Item not found: %s", ex.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // zone_setGroupConfiguration

  JSFunctionSpec zone_methods[] = {
    JS_FS("getDevices", zone_getDevices, 0, 0),
    JS_FS("callScene", zone_callScene, 4, 0),
    JS_FS("undoScene", zone_undoScene, 3, 0),
    JS_FS("callSceneMin", zone_callSceneMin, 3, 0),
    JS_FS("callSceneSys", zone_callSceneSys, 4, 0),
    JS_FS("undoSceneSys", zone_undoSceneSys, 3, 0),
    JS_FS("blink", zone_blink, 2, 0),
    JS_FS("getPowerConsumption", zone_getPowerConsumption, 0, 0),
    JS_FS("pushSensorValue", zone_pushSensorValue, 4, 0),
    JS_FS("pushSensorValueFloat", zone_pushSensorValueFloat, 4, 0),
    JS_FS("getPropertyNode", zone_get_property_node, 0, 0),
    JS_FS("addConnectedDevice", zone_addConnectedDevice, 1, 0),
    JS_FS("removeConnectedDevice", zone_removeConnectedDevice, 1, 0),
    JS_FS("getTemperatureControlStatus", zone_getTemperatureControlStatus, 0, 0),
    JS_FS("getTemperatureControlConfiguration", zone_getTemperatureControlConfiguration, 0, 0),
    JS_FS("getTemperatureControlConfiguration2", zone_getTemperatureControlConfiguration2, 0, 0),
    JS_FS("getTemperatureControlValues", zone_getTemperatureControlValues, 0, 0),
    JS_FS("setTemperatureControlConfiguration", zone_setTemperatureControlConfiguration, 1, 0),
    JS_FS("setTemperatureControlConfiguration2", zone_setTemperatureControlConfiguration2, 1, 0),
    JS_FS("setTemperatureControlValues", zone_setTemperatureControlValues, 1, 0),
    JS_FS("getAssignedSensor", zone_getAssignedSensor, 1, 0),
    JS_FS("addStateSensor", zone_addStateSensor, 3, 0),
    JS_FS("addDevice", zone_addDevice, 1, 0),
    JS_FS("setStatusField", zone_setStatusField, 3, 0),
    JS_FS("getGroupConfiguration", zone_getGroupConfiguration, 1, 0),
    JS_FS("setGroupConfiguration", zone_setGroupConfiguration, 1, 0),
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

  //=== JSState ===

  struct state_wrapper {
    boost::shared_ptr<State> pState;
  };

  void finalize_state(JSContext *cx, JSObject *obj) {
    struct state_wrapper* pState = static_cast<state_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pState;
  } // finalize_state

  static JSClass state_class = {
    "State", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub, finalize_state, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSBool state_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    boost::shared_ptr<State> pState = static_cast<state_wrapper*>(JS_GetPrivate(cx, obj))->pState;
    if (pState != NULL) {
      int opt = JSID_TO_INT(id);
      switch(opt) {
        case 0:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "State")));
          return JS_TRUE;
        case 1:
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, pState->getName().c_str())));
          return JS_TRUE;
        case 2:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL((int) pState->getType()));
          return JS_TRUE;
        case 3:
          JS_SET_RVAL(cx, vp, INT_TO_JSVAL((int) pState->getState()));
          return JS_TRUE;
      }
    }
    return JS_FALSE;
  } // state_JSGet

  static JSPropertySpec state_properties[] = {
    {"className", 0, 0, state_JSGet, NULL},
    {"name", 1, 0, state_JSGet, NULL},
    {"type", 2, 0, state_JSGet, NULL},
    {"value", 3, 0, state_JSGet, NULL},
    {NULL, 0, 0, NULL, NULL}
  };

  JSBool state_get_value(JSContext* cx, uintN argc, jsval* vp) {
    try {
      boost::shared_ptr<State> pState = static_cast<state_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pState;
      if (pState != NULL) {
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL((int) pState->getState()));
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
  } // state_get_value

  JSBool state_set_value(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      boost::shared_ptr<State> pState = static_cast<state_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pState;
      if (pState != NULL) {
        if ((pState->getType() != StateType_Service) && (pState->getType() != StateType_Script)) {
          JS_ReportError(cx, "State type is not allowed to be set by scripting");
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
          return JS_FALSE;
        }
        if (pState->getProviderService() != ctx->getWrapper()->getIdentifier()) {
          JS_ReportError(cx, "State is owned by %s", pState->getProviderService().c_str());
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
          return JS_FALSE;
        }
        try {
          callOrigin_t origin = coJSScripting;
          if (argc >= 2) {
            origin = (callOrigin_t) ctx->convertTo<int>(JS_ARGV(cx, vp)[1]);
          }
          if (argc >= 1) {
            if(JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
              std::string value = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
              pState->setState(origin, value);
            } else if(JSVAL_IS_INT(JS_ARGV(cx, vp)[0])) {
              int svalue = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
              pState->setState(origin, svalue);
            }
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
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
  } // state_set_value

  JSBool state_get_property_node(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(
          ctx->getEnvironment().getExtension("propertyextension"));
      if (ext == NULL) {
        JS_ReportError(cx, "Model.state_get_property_node: ext of wrong type");
        return JS_FALSE;
      }
      boost::shared_ptr<State> pState = static_cast<state_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pState;
      if (pState != NULL) {
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, pState->getPropertyNode())));
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
  } // state_get_property_node

  JSBool state_set_value_range(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      boost::shared_ptr<State> pState = static_cast<state_wrapper*>(JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)))->pState;
      if (pState != NULL) {
        if ((pState->getType() != StateType_Service) &&
            (pState->getType() != StateType_Script)) {
          JS_ReportError(cx, "State type is not allowed to be set by scripting");
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
          return JS_FALSE;
        }
        if (pState->getProviderService() != ctx->getWrapper()->getIdentifier()) {
          JS_ReportError(cx, "State is owned by %s", pState->getProviderService().c_str());
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
          return JS_FALSE;
        }
        try {
          if (argc >= 1) {
            if (JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[0]) &&
              !JSVAL_IS_NULL(JS_ARGV(cx, vp)[0])) {
              JSObject* configObj = JSVAL_TO_OBJECT(JS_ARGV(cx, vp) [0]);
              if (JS_IsArrayObject(cx, configObj)) {
                uint32_t lengthp = 0;
                if (JS_GetArrayLength(cx, configObj, &lengthp)) {
                  State::ValueRange_t vecValue;
                  for (uint32_t indx = 0; indx < lengthp; ++indx) {
                    jsval output;
                    if (JS_LookupElement(cx, configObj, indx, &output)) {
                      std::string value = ctx->convertTo<std::string>(output);
                      vecValue.push_back(value);
                    }
                  }
                  pState->setValueRange(vecValue);
                }
              }
            }
          }
        } catch(ScriptException& e) {
          JS_ReportError(cx, e.what());
          return JS_FALSE;
        } catch(std::invalid_argument& e) {
          JS_ReportError(cx, e.what());
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
  } //state_set_value_range

  JSFunctionSpec state_methods[] = {
    JS_FS("getValue", state_get_value, 0, 0),
    JS_FS("setValue", state_set_value, 0, 0),
    JS_FS("getPropertyNode", state_get_property_node, 0, 0),
    JS_FS("setValueRange", state_set_value_range, 0, 0),
    JS_FS_END
  };

  JSObject* ModelScriptContextExtension::createJSState(ScriptContext& _ctx, boost::shared_ptr<State> _pState) {
    JSObject* result = JS_NewObject(_ctx.getJSContext(), &state_class, NULL, NULL);
    JS_DefineProperties(_ctx.getJSContext(), result, state_properties);
    JS_DefineFunctions(_ctx.getJSContext(), result, state_methods);
    struct state_wrapper* wrapper = new state_wrapper;
    wrapper->pState = _pState;
    JS_SetPrivate(_ctx.getJSContext(), result, wrapper);
    return result;
  } // createJSState

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
      {"Off", SceneOff, 0, scene_JSGetStatic, NULL},
      {"User1", Scene1, 0, scene_JSGetStatic, NULL},
      {"User2", Scene2, 0, scene_JSGetStatic, NULL},
      {"User3", Scene3, 0, scene_JSGetStatic, NULL},
      {"User4", Scene4, 0, scene_JSGetStatic, NULL},
      {"Min", SceneMin, 0, scene_JSGetStatic, NULL},
      {"Max", SceneMax, 0, scene_JSGetStatic, NULL},
      {"Inc", SceneInc, 0, scene_JSGetStatic, NULL},
      {"Dec", SceneDec, 0, scene_JSGetStatic, NULL},
      {"Stop", SceneStop, 0, scene_JSGetStatic, NULL},
      {"DeepOff", SceneDeepOff, 0, scene_JSGetStatic, NULL},
      {"StandBy", SceneStandBy, 0, scene_JSGetStatic, NULL},
      {"Bell", SceneBell, 0, scene_JSGetStatic, NULL},
      {"Panic", ScenePanic, 0, scene_JSGetStatic, NULL},
      {"Alarm", SceneAlarm, 0, scene_JSGetStatic, NULL},
      {"Present", ScenePresent, 0, scene_JSGetStatic, NULL},
      {"Absent", SceneAbsent, 0, scene_JSGetStatic, NULL},
      {"Sleeping", SceneSleeping, 0, scene_JSGetStatic, NULL},
      {"WakeUp", SceneWakeUp, 0, scene_JSGetStatic, NULL},
      {"RoomActivate", SceneRoomActivate, 0, scene_JSGetStatic, NULL},
      {"Fire", SceneFire, 0, scene_JSGetStatic, NULL},
      {"Smoke", SceneSmoke, 0, scene_JSGetStatic, NULL},
      {"Water", SceneWater, 0, scene_JSGetStatic, NULL},
      {"Gas", SceneGas, 0, scene_JSGetStatic, NULL},
      {"Bell2", SceneBell2, 0, scene_JSGetStatic, NULL},
      {"Bell3", SceneBell3, 0, scene_JSGetStatic, NULL},
      {"Bell4", SceneBell4, 0, scene_JSGetStatic, NULL},
      {"Alarm2", SceneAlarm2, 0, scene_JSGetStatic, NULL},
      {"Alarm3", SceneAlarm3, 0, scene_JSGetStatic, NULL},
      {"Alarm4", SceneAlarm4, 0, scene_JSGetStatic, NULL},
      {"WindActive", SceneWindActive, 0, scene_JSGetStatic, NULL},
      {"WindInactive", SceneWindInactive, 0, scene_JSGetStatic, NULL},
      {"RainActive", SceneRainActive, 0, scene_JSGetStatic, NULL},
      {"RainInactive", SceneRainInactive, 0, scene_JSGetStatic, NULL},
      {"HailActive", SceneHailActive, 0, scene_JSGetStatic, NULL},
      {"HailInactive", SceneHailInactive, 0, scene_JSGetStatic, NULL},
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
