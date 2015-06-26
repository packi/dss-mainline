/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Author: Remy Mahler, digitalstrom AG <remy.mahler@digitalstrom.com>

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

#include "jscluster.h"

#include "security/security.h"

#include "src/model/apartment.h"
#include "src/model/cluster.h"

namespace dss {

  const std::string ClusterScriptExtensionName = "clusterextension";

  // Forward declaration
  JSBool cluster_create(JSContext* cx, uintN argc, jsval *vp);
  JSBool cluster_remove(JSContext* cx, uintN argc, jsval *vp);
  JSBool cluster_add_device(JSContext* cx, uintN argc, jsval *vp);
  JSBool cluster_remove_device(JSContext* cx, uintN argc, jsval *vp);
  JSBool cluster_configuration_lock(JSContext* cx, uintN argc, jsval *vp);
  JSBool cluster_set_name(JSContext* cx, uintN argc, jsval *vp);
  JSBool cluster_get_name(JSContext* cx, uintN argc, jsval *vp);

  JSFunctionSpec cluster_static_methods[] = {
    JS_FS("createCluster", cluster_create, 2, 0),
    JS_FS("removeCluster", cluster_remove, 1, 0),
    JS_FS("clusterAddDevice", cluster_add_device, 3, 0),
    JS_FS("clusterRemoveDevice", cluster_remove_device, 2, 0),
    JS_FS("clusterConfigurationLock", cluster_configuration_lock, 2, 0),
    JS_FS("clusterSetName", cluster_set_name, 2, 0),
    JS_FS("clusterGetName", cluster_get_name, 1, 0),
    JS_FS_END
  }; // cluster_static_methods

  static JSClass cluster_class = {
    "Cluster", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // cluster_class

  JSBool cluster_construct(JSContext *cx, uintN argc, jsval *vp) {
    return JS_FALSE;
  } // cluster_construct

  JSBool cluster_create(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ClusterScriptExtension* ext =
          dynamic_cast<ClusterScriptExtension*>(ctx->getEnvironment().getExtension(ClusterScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: cluster_create: ext of wrong type", lsFatal);
        return JS_FALSE;
      }
      if (argc != 2) {
        Logger::getInstance()->log("JS: cluster_create: need two parameters: (DeviceClass, name)", lsError);
        return JS_FALSE;
      }
      if (!JSVAL_IS_INT(JS_ARGV(cx, vp)[0]) || !JSVAL_IS_STRING(JS_ARGV(cx, vp)[1])) {
        Logger::getInstance()->log("JS: cluster_create: need two parameters of type int, string: (DeviceClass, name)", lsError);
        return JS_FALSE;
      }
      boost::shared_ptr<Cluster> cluster = ext->getApartment().getEmptyCluster();
      if (cluster == NULL) {
        Logger::getInstance()->log("JS: cluster_create: no cluster creation possible", lsWarning);
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(-1));
        return JS_TRUE;
      }
      int deviceClass = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      std::string name = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
      cluster->setLocation(cd_none);
      cluster->setProtectionClass(wpc_none);
      cluster->setStandardGroupID(deviceClass);
      cluster->setName(name);
      if (ext->busUpdateCluster(cx, cluster)) {
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(cluster->getID()));
      } else {
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(-1));
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
    JS_SET_RVAL(cx, vp, JSVAL_FALSE);
    return JS_FALSE;
  } // cluster_create

  JSBool cluster_remove(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ClusterScriptExtension* ext =
          dynamic_cast<ClusterScriptExtension*>(ctx->getEnvironment().getExtension(ClusterScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: cluster_remove: ext of wrong type", lsFatal);
        return JS_FALSE;
      }
      if (argc != 1) {
        Logger::getInstance()->log("JS: cluster_remove: need one parameter: (clusterId)", lsError);
        return JS_FALSE;
      }
      if (!JSVAL_IS_INT(JS_ARGV(cx, vp)[0])) {
        Logger::getInstance()->log("JS: cluster_remove: need one parameter of type int (clusterId)", lsError);
        return JS_FALSE;
      }
      int clusterId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      boost::shared_ptr<Cluster> cluster = ext->getApartment().getCluster(clusterId);
      if (cluster == NULL) {
        Logger::getInstance()->log("JS: cluster_remove: no cluster found", lsFatal);
        return JS_FALSE;
      }
      if (!cluster->releaseCluster()) {
        Logger::getInstance()->log("JS: cluster_remove: cluster not empty", lsWarning);
        JS_SET_RVAL(cx, vp, JSVAL_FALSE);
        return JS_TRUE;
      }
      JSBool retVal = ext->busUpdateCluster(cx, cluster);
      JS_SET_RVAL(cx, vp, (retVal == JS_TRUE) ? JSVAL_TRUE : JSVAL_FALSE);
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
  } // cluster_remove

  JSBool cluster_add_device(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ClusterScriptExtension* ext =
          dynamic_cast<ClusterScriptExtension*>(ctx->getEnvironment().getExtension(ClusterScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: cluster_create: ext of wrong type", lsFatal);
        return JS_FALSE;
      }
      if (argc != 2) {
        Logger::getInstance()->log("JS: cluster_add_device: need two parameters: (clusterId, device dsuid)", lsError);
        return JS_FALSE;
      }
      if (!JSVAL_IS_INT(JS_ARGV(cx, vp)[0]) || !JSVAL_IS_STRING(JS_ARGV(cx, vp)[1])) {
        Logger::getInstance()->log("JS: cluster_add_device: need two parameters of type int, string: (clusterId, device dsuid)", lsError);
        return JS_FALSE;
      }
      int clusterId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      dsuid_t deviceDsuid = str2dsuid(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]));
      boost::shared_ptr<Device> device = ext->getApartment().getDeviceByDSID(deviceDsuid);
      device->addToGroup(clusterId);
      return ext->busAddToGroup(cx, device, clusterId);
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
  } // cluster_add_device

  JSBool cluster_remove_device(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ClusterScriptExtension* ext =
          dynamic_cast<ClusterScriptExtension*>(ctx->getEnvironment().getExtension(ClusterScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: cluster_create: ext of wrong type", lsFatal);
        return JS_FALSE;
      }
      if (argc != 2) {
        Logger::getInstance()->log("JS: cluster_add_device: need two parameters: (clusterId, device dsuid)", lsError);
        return JS_FALSE;
      }
      if (!JSVAL_IS_INT(JS_ARGV(cx, vp)[0]) || !JSVAL_IS_STRING(JS_ARGV(cx, vp)[1])) {
        Logger::getInstance()->log("JS: cluster_add_device: need two parameters of type int, string: (clusterId, device dsuid)", lsError);
        return JS_FALSE;
      }
      int clusterId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      dsuid_t deviceDsuid = str2dsuid(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]));
      boost::shared_ptr<Device> device = ext->getApartment().getDeviceByDSID(deviceDsuid);
      device->removeFromGroup(clusterId);
      return ext->busRemoveFromGroup(cx, device, clusterId);
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
  } // cluster_remove_device

  JSBool cluster_configuration_lock(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ClusterScriptExtension* ext =
          dynamic_cast<ClusterScriptExtension*>(ctx->getEnvironment().getExtension(ClusterScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: cluster_create: ext of wrong type", lsFatal);
        return JS_FALSE;
      }
      if (argc != 2) {
        Logger::getInstance()->log("JS: cluster_configuration_lock: need two parameters: (clusterId, lock)", lsError);
        return JS_FALSE;
      }
      if (!JSVAL_IS_INT(JS_ARGV(cx, vp)[0]) || !JSVAL_IS_BOOLEAN(JS_ARGV(cx, vp)[1])) {
        Logger::getInstance()->log("JS: cluster_set_name: need two parameters of type int, string: (clusterId, name)", lsError);
        return JS_FALSE;
      }
      int clusterId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      bool lock = ctx->convertTo<bool>(JS_ARGV(cx, vp)[1]);
      boost::shared_ptr<Cluster> cluster = ext->getApartment().getCluster(clusterId);
      if (cluster == NULL) {
        Logger::getInstance()->log("JS: cluster_set_name: no cluster found", lsFatal);
        return JS_FALSE;
      }
      cluster->setConfigurationLocked(lock);
      return ext->busUpdateClusterLock(cx, cluster);
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
  } // cluster_configuration_lock

  JSBool cluster_set_name(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ClusterScriptExtension* ext =
          dynamic_cast<ClusterScriptExtension*>(ctx->getEnvironment().getExtension(ClusterScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: cluster_create: ext of wrong type", lsFatal);
        return JS_FALSE;
      }
      if (argc != 2) {
        Logger::getInstance()->log("JS: cluster_set_name: need two parameters: (clusterId, name)", lsError);
        return JS_FALSE;
      }
      if (!JSVAL_IS_INT(JS_ARGV(cx, vp)[0]) || !JSVAL_IS_STRING(JS_ARGV(cx, vp)[1])) {
        Logger::getInstance()->log("JS: cluster_set_name: need two parameters of type int, string: (clusterId, name)", lsError);
        return JS_FALSE;
      }
      int clusterId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      std::string name = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);

      boost::shared_ptr<Cluster> cluster = ext->getApartment().getCluster(clusterId);
      if (cluster == NULL) {
        Logger::getInstance()->log("JS: cluster_set_name: no cluster found", lsFatal);
        return JS_FALSE;
      }
      cluster->setName(name);
      return ext->busUpdateCluster(cx, cluster);
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
  } // cluster_set_name

  JSBool cluster_get_name(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    try {
      ClusterScriptExtension* ext =
          dynamic_cast<ClusterScriptExtension*>(ctx->getEnvironment().getExtension(ClusterScriptExtensionName));
      if (ext == NULL) {
        Logger::getInstance()->log("JS: cluster_create: ext of wrong type", lsFatal);
        return JS_FALSE;
      }
      if (argc != 1) {
        Logger::getInstance()->log("JS: cluster_get_name: need one parameter: (clusterId)", lsError);
        return JS_FALSE;
      }
      if (!JSVAL_IS_INT(JS_ARGV(cx, vp)[0])) {
        Logger::getInstance()->log("JS: cluster_get_name: need one parameter of type int (clusterId)", lsError);
        return JS_FALSE;
      }
      int clusterId = ctx->convertTo<int>(JS_ARGV(cx, vp)[0]);
      boost::shared_ptr<Cluster> cluster = ext->getApartment().getCluster(clusterId);
      if (cluster == NULL) {
        Logger::getInstance()->log("JS: cluster_get_name: no cluster found", lsFatal);
        return JS_FALSE;
      }
      JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, cluster->getName().c_str())));
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
  } // cluster_get_name

  ClusterScriptExtension::ClusterScriptExtension(Apartment& _apartment)
    : ScriptExtension(ClusterScriptExtensionName),
      m_Apartment(_apartment)
  { } // ctor

  ClusterScriptExtension::~ClusterScriptExtension()
  { } // dtor

  void ClusterScriptExtension::extendContext(ScriptContext& _context)
  {
    JS_InitClass(_context.getJSContext(),
                 JS_GetGlobalObject(_context.getJSContext()),
                 NULL,
                 &cluster_class,
                 &cluster_construct,
                 0,
                 NULL,
                 NULL,
                 NULL,
                 cluster_static_methods);
  } // extendContext

  Apartment& ClusterScriptExtension::getApartment() {
    return m_Apartment;
  } // getApartment

  JSBool ClusterScriptExtension::busAddToGroup(JSContext* cx, boost::shared_ptr<Device> _device, int _clusterId) {
    jsrefcount ref = JS_SuspendRequest(cx);
    try {
      m_Apartment.getBusInterface()->getStructureModifyingBusInterface()->addToGroup(
            _device->getDSMeterDSID(),
            _clusterId,
            _device->getShortAddress());
      JS_ResumeRequest(cx, ref);
      return JS_TRUE;
    } catch (DSSException& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "General Failure: %s", ex.what());
    }
    return JS_FALSE;
  } // busAddToGroup

  JSBool ClusterScriptExtension::busRemoveFromGroup(JSContext* cx, boost::shared_ptr<Device> _device, int _clusterId) {
    jsrefcount ref = JS_SuspendRequest(cx);
    try {
      m_Apartment.getBusInterface()->getStructureModifyingBusInterface()->removeFromGroup(
            _device->getDSMeterDSID(),
            _clusterId,
            _device->getShortAddress());
      JS_ResumeRequest(cx, ref);
      return JS_TRUE;
    } catch (DSSException& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "General Failure: %s", ex.what());
    }
    return JS_FALSE;
  } // busRemoveFromGroup

  JSBool ClusterScriptExtension::busUpdateCluster(JSContext* cx, boost::shared_ptr<Cluster> _cluster) {
    jsrefcount ref = JS_SuspendRequest(cx);
    try {
      StructureModifyingBusInterface* itf = m_Apartment.getBusInterface()->getStructureModifyingBusInterface();
      itf->clusterSetName(_cluster->getID(), _cluster->getName());
      itf->clusterSetStandardID(_cluster->getID(), _cluster->getStandardGroupID());
      itf->clusterSetProperties(_cluster->getID(), _cluster->getLocation(),
                                _cluster->getFloor(), _cluster->getProtectionClass());
      itf->clusterSetLockedScenes(_cluster->getID(), _cluster->getLockedScenes());
      JS_ResumeRequest(cx, ref);
      return JS_TRUE;
    } catch (DSSException& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "General Failure: %s", ex.what());
    }
    return JS_FALSE;
  } // busUpdateCluster

  JSBool ClusterScriptExtension::busUpdateClusterLock(JSContext* cx, boost::shared_ptr<Cluster> _cluster) {
    jsrefcount ref = JS_SuspendRequest(cx);
    try {
      StructureModifyingBusInterface* itf = m_Apartment.getBusInterface()->getStructureModifyingBusInterface();
      itf->clusterSetConfigurationLock(_cluster->getID(), _cluster->isConfigurationLocked());
      JS_ResumeRequest(cx, ref);
      return JS_TRUE;
    } catch (DSSException& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ResumeRequest(cx, ref);
      JS_ReportError(cx, "General Failure: %s", ex.what());
    }
    return JS_FALSE;
  } // busUpdateClusterLock

} // namespace dss
