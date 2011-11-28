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

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "core/scripting/scriptobject.h"
#include "core/scripting/jsproperty.h"
#include "core/propertysystem.h"
#include "core/security/user.h"
#include "core/security/security.h"

namespace dss {

  //================================================== PropertyScriptExtension

  const std::string PropertyScriptExtensionName = "propertyextension";

  typedef struct {
    PropertyNodePtr pNode;
  } prop_wrapper;

  void property_finalize(JSContext *cx, JSObject *obj) {
    prop_wrapper* wrapper = static_cast<prop_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete wrapper;
  } // property_finalize

  static JSClass prop_class = {
    "Property", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub, property_finalize, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // prop_class


  PropertyScriptExtension::PropertyScriptExtension(PropertySystem& _propertySystem)
  : ScriptExtension(PropertyScriptExtensionName),
    m_PropertySystem(_propertySystem),
    m_NextListenerID(1)
  { } // ctor

  PropertyScriptExtension::~PropertyScriptExtension() {
    for(std::vector<boost::shared_ptr<PropertyScriptListener> >::iterator it = m_Listeners.begin(), e = m_Listeners.end();
        it != e; ++it) {
      (*it)->unsubscribe();
      (*it)->stop();
    }
  } // dtor

  JSBool prop_setProperty(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    int argIndex;
    if(node != NULL) {
      if(argc >= 1) {
        argIndex = 0;
      } else {
        Logger::getInstance()->log("JS: Property(obj).setValue: need one argument: value", lsError);
      }
    } else {
      if(argc >= 2) {
        std::string propName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        node = ext->getProperty(ctx, propName);
        if(node == NULL) {
          node = ext->createProperty(ctx, propName);
        }
        argIndex = 1;
      } else {
        Logger::getInstance()->log("JS: Property.setProperty: need two argument: property-path & value", lsError);
        return JS_FALSE;
      }
    }

    if(node != NULL) {
      try {
        if(JSVAL_IS_STRING(JS_ARGV(cx, vp)[argIndex])) {
          node->setStringValue(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[argIndex]));
        } else if(JSVAL_IS_BOOLEAN(JS_ARGV(cx, vp)[argIndex])) {
          node->setBooleanValue(ctx->convertTo<bool>(JS_ARGV(cx, vp)[argIndex]));
        } else if(JSVAL_IS_INT(JS_ARGV(cx, vp)[argIndex])) {
          node->setIntegerValue(ctx->convertTo<int>(JS_ARGV(cx, vp)[argIndex]));
        } else {
          Logger::getInstance()->log("JS: setProperty: unknown type of argument 2", lsError);
        }
        JS_SET_RVAL(cx, vp, JSVAL_TRUE);
        return JS_TRUE;
      } catch(PropertyTypeMismatch&) {
        Logger::getInstance()->log("Error setting value of " + node->getDisplayName(), lsFatal);
      }
    } else {
      Logger::getInstance()->log("Coule not create property " + node->getDisplayName(), lsFatal);
      JS_SET_RVAL(cx, vp, JSVAL_FALSE);
      return JS_TRUE;
    }
    return JS_FALSE;
  } // prop_setProperty

  JSBool prop_getProperty(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    if(node == NULL) {
      if(argc >= 1) {
        std::string propName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        node = ext->getProperty(ctx, propName);
      } else {
        Logger::getInstance()->log("JS: Property.getProperty: need one argument: property-path", lsError);
      }
    }
    if(node == NULL) {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
    } else {
      switch(node->getValueType()) {
      case vTypeInteger:
        JS_SET_RVAL(cx, vp, INT_TO_JSVAL(node->getIntegerValue()));
        break;
      case vTypeString: {
          std::string val = node->getStringValue();
          JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, val.c_str())));
        }
        break;
      case vTypeBoolean:
        JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(node->getBoolValue()));
        break;
      case vTypeNone:
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
      default:
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
      }
    }
    return JS_TRUE;
  } // prop_getProperty

  JSBool prop_setListener(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(
        ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    jsval function;
    if(node != NULL) {
      if(argc >= 1) {
        function = JS_ARGV(cx,vp)[0];
      } else {
        JS_ReportError(cx, "Property.setListener: need one argument callback", lsError);
        return JS_FALSE;
      }
    } else {
      if(argc >= 2) {
        std::string nodePath;
        try {
          nodePath = ctx->convertTo<std::string>(JS_ARGV(cx,vp)[0]);
        } catch(ScriptException& ex) {
          JS_ReportError(cx, "Property.setListener: cannot convert argument: name");
          return JS_FALSE;
        }
        node = ext->getProperty(ctx, nodePath);
        if(node == NULL) {
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
          JS_ReportWarning(cx, "Property.setListener: cannot find node %s", nodePath.c_str());
          return JS_TRUE;
        }
        function = JS_ARGV(cx,vp)[1];
      } else {
        JS_ReportError(cx, "Property.setListener: need two arguments: property-path & callback");
        return JS_FALSE;
      }
    }

    JSObject* jsRoot = JS_NewObject(cx, NULL, NULL, NULL);
    std::string ident = ext->produceListenerID();
    boost::shared_ptr<PropertyScriptListener> listener (
        new PropertyScriptListener(ext, ctx, jsRoot, function, ident));

    ext->addListener(listener);
    node->addListener(listener.get());

    JSString* str = JS_NewStringCopyZ(cx, ident.c_str());
    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(str));
    return JS_TRUE;
  } // prop_setListener

  JSBool prop_removeListener(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(
        ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    std::string listenerIdent;
    try {
      listenerIdent = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
    } catch(ScriptException& ex) {
      JS_ReportError(cx, "Property.removeListener: cannot convert argument");
      return JS_FALSE;
    }

    ext->removeListener(listenerIdent);

    JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    return JS_TRUE;
  } // prop_removeListener

  JSBool prop_setFlag(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    std::string flagName;
    bool value;
    if(node != NULL) {
      if(argc >= 2) {
        flagName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        value = ctx->convertTo<bool>(JS_ARGV(cx, vp)[1]);
      } else {
        Logger::getInstance()->log("JS: Property(obj).setFlag needs two parameter flagName and value", lsError);
        return JS_FALSE;
      }
    } else {
      if(argc >= 3) {
        std::string nodePath = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        node = ext->getProperty(ctx, nodePath);
        if(node == NULL) {
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
          Logger::getInstance()->log("JS: Property.setFlag: cannot find node '" + nodePath, lsError);
          return JS_TRUE;
        }
        flagName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
        value = ctx->convertTo<bool>(JS_ARGV(cx, vp)[2]);
      } else {
        Logger::getInstance()->log("JS: Property.setFlag needs two parameter path and flagName", lsError);
        return JS_FALSE;
      }
    }

    PropertyNode::Flag flag;
    if(flagName == "ARCHIVE") {
      flag = PropertyNode::Archive;
    } else if(flagName == "WRITEABLE") {
      flag = PropertyNode::Writeable;
    } else if(flagName == "READABLE") {
      flag = PropertyNode::Readable;
    } else {
      Logger::getInstance()->log("JS: prop_setFlag: Invalid value for flag: " + flagName, lsError);
      JS_SET_RVAL(cx, vp, JSVAL_FALSE);
      return JS_TRUE;
    }

    node->setFlag(flag, value);
    JS_SET_RVAL(cx, vp, JSVAL_NULL);
    return JS_TRUE;
  } // prop_setFlag

  JSBool prop_hasFlag(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    std::string flagName;
    if(node != NULL) {
      if(argc >= 1) {
        flagName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
      } else {
        Logger::getInstance()->log("JS: Property(obj).hasFlag needs at least one parameter flagName", lsError);
        return JS_FALSE;
      }
    } else {
      if(argc >= 2) {
        std::string nodePath = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        node = ext->getProperty(ctx, nodePath);
        if(node == NULL) {
          Logger::getInstance()->log("JS: Property.hasFlag: cannot find node '" + nodePath, lsError);
          JS_SET_RVAL(cx, vp, JSVAL_NULL);
          return JS_TRUE;
        }
        flagName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
      } else {
        Logger::getInstance()->log("JS: Property.hasFlag needs two parameter path and flagName", lsError);
        return JS_FALSE;
      }
    }

    PropertyNode::Flag flag;
    if(flagName == "ARCHIVE") {
      flag = PropertyNode::Archive;
    } else if(flagName == "WRITEABLE") {
      flag = PropertyNode::Writeable;
    } else if(flagName == "READABLE") {
      flag = PropertyNode::Readable;
    } else {
      Logger::getInstance()->log("JS: prop_hasFlag: Invalid value for flag: " + flagName, lsError);
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
      return JS_TRUE;
    }

    if(node->hasFlag(flag)) {
      JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    } else {
      JS_SET_RVAL(cx, vp, JSVAL_FALSE);
    }
    return JS_TRUE;
  } // prop_setFlag

  JSBool prop_getChild(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    assert(node != NULL);
    std::string nodeName;
    if(argc >= 1) {
      nodeName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
    } else {
      Logger::getInstance()->log("JS: Property(obj).getChild needs at least one parameter nodeName", lsError);
      return JS_FALSE;
    }

    PropertyNodePtr childNode = node->getProperty(nodeName);
    if(childNode != NULL) {
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, childNode)));
    } else {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
    }
    return JS_TRUE;
  } // prop_getChild

  JSBool prop_getChildren(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    assert(node != NULL);

    JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);

    for(int iChild = 0; iChild < node->getChildCount(); iChild++) {
      jsval childJSVal = OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, node->getChild(iChild)));
      JSBool res = JS_SetElement(cx, resultObj, iChild, &childJSVal);
      if(!res) {
        return JS_FALSE;
      }
    }

    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));
    return JS_TRUE;
  } // prop_getChildren

  JSBool prop_getNode(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr pNode;
    if(argc >= 1) {
      pNode = ext->getProperty(ctx, ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
      if(pNode == NULL) {
        Logger::getInstance()->log("JS: Property.getNode: could not find node", lsWarning);
        JS_SET_RVAL(cx, vp, JSVAL_NULL);
        return JS_TRUE;
      }
    } else {
      Logger::getInstance()->log("JS: Property.getNode needs at least one parameter nodeName", lsError);
      return JS_FALSE;
    }

    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, pNode)));
    return JS_TRUE;
  } // prop_getNode

  JSBool prop_getParent(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    assert(node != NULL);

    PropertyNode* parentNode = node->getParentNode();
    if(parentNode != NULL) {
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, parentNode)));
    } else {
      JS_SET_RVAL(cx, vp, JSVAL_NULL);
    }
    return JS_TRUE;
  } // prop_getChild

  JSBool prop_removeChild(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    assert(node != NULL);

    PropertyNodePtr childNode;
    if(argc >= 1) {
      if(JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[0])) {
        childNode = ext->getPropertyFromObj(ctx, JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[0]));
      }
      if(childNode == NULL) {
        try {
          childNode = node->getProperty(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
        } catch(ScriptException& ex) {
        }
      }
    } else {
      Logger::getInstance()->log("Need one argument (node) in removeChild", lsWarning);
    }

    if(childNode != NULL) {
      node->removeChild(childNode);
      JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    } else {
      JS_SET_RVAL(cx, vp, JSVAL_FALSE);
    }
    return JS_TRUE;
  } // prop_getChild

  JSBool prop_store(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    if(ext->store(ctx)) {
      JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    } else {
      JS_SET_RVAL(cx, vp, JSVAL_FALSE);
    }
    return JS_TRUE;
  } // prop_store

  JSBool prop_load(JSContext* cx, uintN argc, jsval* vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    if(ext->load(ctx)) {
      JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    } else {
      JS_SET_RVAL(cx, vp, JSVAL_FALSE);
    }
    return JS_TRUE;
  } // prop_load

  JSBool prop_get_name(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, node->getDisplayName().c_str())));
    return JS_TRUE;
  } // prop_get_name

  JSBool prop_get_path(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
    PropertyNodePtr node = ext->getPropertyFromObj(ctx, JS_THIS_OBJECT(cx, vp));
    std::string fullName = node->getDisplayName();
    PropertyNode* nextNode = node->getParentNode();
    while(nextNode != NULL) {
      if(nextNode->getParentNode() != NULL) {
        fullName = nextNode->getDisplayName() + "/" + fullName;
      } else {
        fullName = "/" + fullName;
      }
      nextNode = nextNode->getParentNode();
    }
    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, fullName.c_str())));
    return JS_TRUE;
  } // prop_get_path

  JSFunctionSpec prop_methods[] = {
    JS_FS("setValue", prop_setProperty, 1, 0),
    JS_FS("getValue", prop_getProperty, 0, 0),
    JS_FS("setListener", prop_setListener, 1, 0),
    JS_FS("removeListener", prop_removeListener, 1, 0),
    JS_FS("getChild", prop_getChild, 1, 0),
    JS_FS("getChildren", prop_getChildren, 0, 0),
    JS_FS("removeChild", prop_removeChild, 1, 0),
    JS_FS("getParent", prop_getParent, 0, 0),
    JS_FS("setFlag", prop_setFlag, 2, 0),
    JS_FS("hasFlag", prop_hasFlag, 1, 0),
    JS_FS("getName", prop_get_name, 0, 0),
    JS_FS("getPath", prop_get_path, 0, 0),
    JS_FS_END
  };

  JSFunctionSpec prop_static_methods[] = {
    JS_FS("getProperty", prop_getProperty, 1, 0),
    JS_FS("setProperty", prop_setProperty, 1, 0),
    JS_FS("setListener", prop_setListener, 2, 0),
    JS_FS("removeListener", prop_removeListener, 1, 0),
    JS_FS("setFlag", prop_setFlag, 3, 0),
    JS_FS("hasFlag", prop_hasFlag, 2, 0),
    JS_FS("getNode", prop_getNode, 1, 0),
    JS_FS("store", prop_store, 0, 0),
    JS_FS("load", prop_load, 0, 0),
    JS_FS_END
  };

  JSBool property_construct(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    std::string propName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
    PropertyNodePtr pNode = ext->getProperty(ctx, propName);
    if(pNode == NULL) {
      pNode = ext->createProperty(ctx, propName);
    }
    if(pNode != NULL) {
      prop_wrapper* wrapper = new prop_wrapper;

      JSObject *obj = JS_NewObject(cx, &prop_class, NULL, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));

      JS_SetPrivate(cx, obj, wrapper);
      wrapper->pNode = pNode;
      return JS_TRUE;
    }

    JS_ReportError(cx, "Property.construct: could not get/create node %s", propName.c_str());
    JS_SET_RVAL(cx, vp, JSVAL_NULL);
    return JS_FALSE;
  } // property_construct

  JSBool prop_JSGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, obj);
    assert(node != NULL);

    int opt = JSID_TO_INT(id);
    if(opt == 0) {
      std::string propName = node->getDisplayName();
      JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, propName.c_str())));
      return JS_TRUE;
    }
    return JS_FALSE;
  } // prop_JSGet

  static JSPropertySpec prop_properties[] = {
    {"name", 0, 0, prop_JSGet, NULL},
    {NULL}
  };

  void PropertyScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
                 NULL, &prop_class, property_construct, 1, prop_properties,
                 prop_methods, NULL, prop_static_methods);
  } // extendContext

  JSObject* PropertyScriptExtension::createJSProperty(ScriptContext& _ctx, PropertyNodePtr _node) {
    return createJSProperty(_ctx, _node.get());
  }

  JSObject* PropertyScriptExtension::createJSProperty(ScriptContext& _ctx, PropertyNode* _node) {
    std::string fullName = _node->getDisplayName();
    PropertyNode* nextNode = _node->getParentNode();
    while(nextNode != NULL) {
      if(nextNode->getParentNode() != NULL) {
        fullName = nextNode->getDisplayName() + "/" + fullName;
      } else {
        fullName = "/" + fullName;
      }
      nextNode = nextNode->getParentNode();
    }
    jsval propName = STRING_TO_JSVAL(JS_NewStringCopyZ(_ctx.getJSContext(), fullName.c_str()));
    JSObject* result = JS_ConstructObjectWithArguments(_ctx.getJSContext(), &prop_class, NULL, NULL, 1, &propName);
    assert(result != NULL);
    return result;
  } // createJSProperty

  JSObject* PropertyScriptExtension::createJSProperty(ScriptContext& _ctx, const std::string& _path) {
    return createJSProperty(_ctx, getProperty(&_ctx, _path));
  } // createJSProperty

  std::string PropertyScriptExtension::produceListenerID() {
    return "listener_" + intToString(long(this), true) + "_" + intToString(m_NextListenerID++);
  } // produceListenerID

  boost::shared_ptr<PropertyScriptListener> PropertyScriptExtension::getListener(const std::string& _identifier) {
    for(std::vector<boost::shared_ptr<PropertyScriptListener> >::iterator it = m_Listeners.begin(), e = m_Listeners.end();
        it != e; ++it) {
      if((*it)->getIdentifier() == _identifier) {
        return *it;
      }
    }
    return boost::shared_ptr<PropertyScriptListener> ();
  } // removeListener

  void PropertyScriptExtension::addListener(boost::shared_ptr<PropertyScriptListener> _pListener) {
    m_Listeners.push_back(_pListener);
  } // addListener

  void PropertyScriptExtension::removeListener(const std::string& _identifier) {
    for(std::vector<boost::shared_ptr<PropertyScriptListener> >::iterator it = m_Listeners.begin(), e = m_Listeners.end();
        it != e; ++it) {
      if((*it)->getIdentifier() == _identifier) {
        (*it)->unsubscribe();
        m_Listeners.erase(it);
        return;
      }
    }
  } // removeListener

  PropertyNodePtr PropertyScriptExtension::getProperty(ScriptContext* _context, const std::string& _path) {
    return m_PropertySystem.getProperty(_path);
  } // getProperty

  bool PropertyScriptExtension::store(ScriptContext* _ctx) {
    return false;
  } // store

  bool PropertyScriptExtension::load(ScriptContext* _ctx) {
    return false;
  } // load

  PropertyNodePtr PropertyScriptExtension::createProperty(ScriptContext* _context, const std::string& _name) {
    return m_PropertySystem.createProperty(_name);
  } // createProperty

  PropertyNodePtr PropertyScriptExtension::getPropertyFromObj(ScriptContext* _context, JSObject* _obj) {
    PropertyNodePtr result;
    if(_obj != NULL) {
      if(JS_InstanceOf(_context->getJSContext(), _obj, &prop_class, NULL) == JS_TRUE) {
        prop_wrapper* pWrapper = static_cast<prop_wrapper*>(JS_GetPrivate(_context->getJSContext(), _obj));
        result = pWrapper->pNode;
      }
    }
    return result;
  } // getPropertyFromObj


  //================================================== PropertyScriptListener

  PropertyScriptListener::PropertyScriptListener(PropertyScriptExtension* _pExtension,
                                                 ScriptContext* _pContext,
                                                 JSObject* _functionObj,
                                                 jsval _function,
                                                 const std::string& _identifier)
  : ScriptContextAttachedObject(_pContext),
    m_pExtension(_pExtension),
    m_pFunctionObject(_functionObj),
    m_Function(_function),
    m_Identifier(_identifier),
    m_pRunAsUser(NULL)
  {
    m_callbackRunning = false;
    m_FunctionRoot = new ScriptFunctionRooter(getContext(), m_pFunctionObject, m_Function);
    if(Security::getCurrentlyLoggedInUser() != NULL) {
      m_pRunAsUser = new User(*Security::getCurrentlyLoggedInUser());
    }
    Logger::getInstance()->log("JavaScript: registered property-changed callback: " +
        m_Identifier, lsDebug);
  } // ctor

  PropertyScriptListener::~PropertyScriptListener() {
    if (m_callbackRunning) {
      Logger::getInstance()->log("JavaScript: requested property listener release during callback: " +
              m_Identifier, lsError);
    }
    if (m_FunctionRoot) {
      delete m_FunctionRoot;
    }
    if (m_pRunAsUser) {
      delete m_pRunAsUser;
    }
    m_pExtension->removeListener(m_Identifier);
    Logger::getInstance()->log("JavaScript: released property-changed callback: " +
        m_Identifier, lsDebug);
  }

  void PropertyScriptListener::deferredCallback(PropertyNodePtr _changedNode,
      JSObject* _obj, jsval _function, ScriptFunctionRooter* _rooter,
      boost::shared_ptr<PropertyScriptListener> _self) {

    if(getIsStopped()) {
      return;
    }

    if(m_pRunAsUser != NULL) {
      Security* pSecurity = getContext()->getEnvironment().getSecurity();
      if(pSecurity != NULL) {
        pSecurity->signIn(m_pRunAsUser);
      }
    }

    // make a local copy to defer object deletion
    boost::shared_ptr<PropertyScriptListener> myself = _self;

    {
      ScriptLock lock(getContext());
      JSContextThread req(getContext());
      m_pScriptObject.reset(new ScriptObject(m_pFunctionObject, *getContext()));
      ScriptFunctionParameterList list(*getContext());
      list.add(_changedNode->getDisplayName());

      Logger::getInstance()->log("JavaScript: running property-changed callback: " +
          m_Identifier, lsDebug);
      m_callbackRunning = true;

      try {
        m_pScriptObject->callFunctionByReference<void>(m_Function, list);
      } catch(ScriptException& e) {
        Logger::getInstance()->log("JavaScript: error running property-changed callback: " +
            std::string(e.what()), lsFatal);
      }
      m_pScriptObject.reset();
      m_callbackRunning = false;

      Logger::getInstance()->log("JavaScript: exit property-changed callback: " +
          m_Identifier, lsDebug);
    }

    myself.reset();
  }

  void PropertyScriptListener::propertyChanged(PropertyNodePtr _caller, PropertyNodePtr _changedNode) {
    doOnChange(_changedNode);
  } // propertyChanged

  void PropertyScriptListener::propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyRemoved

  void PropertyScriptListener::propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyAdded

  void PropertyScriptListener::doOnChange(PropertyNodePtr _changedNode) {
    if(getIsStopped()) {
      return;
    }
    boost::thread(boost::bind(&PropertyScriptListener::deferredCallback, this, _changedNode,
        m_pFunctionObject, m_Function, m_FunctionRoot, shared_from_this()));
  } // doOnChange

  void PropertyScriptListener::stop()  {
    ScriptContextAttachedObject::stop();
    ScriptLock lock(getContext());
    boost::shared_ptr<JSContextThread> req;
    delete this;
  }

} // namespace dss
