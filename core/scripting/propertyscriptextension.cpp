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

#include "propertyscriptextension.h"

#include "core/propertysystem.h"
#include "core/scripting/scriptobject.h"

namespace dss {

  //================================================== PropertyScriptExtension

  const std::string PropertyScriptExtensionName = "propertyextension";

  PropertyScriptExtension::PropertyScriptExtension(PropertySystem& _propertySystem)
  : ScriptExtension(PropertyScriptExtensionName),
    m_PropertySystem(_propertySystem),
    m_NextListenerID(1)
  { } // ctor

  PropertyScriptExtension::~PropertyScriptExtension() {
    scrubVector(m_Listeners);
  } // dtor

  JSBool prop_setProperty(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, obj);
    int argIndex;
    if(node != NULL) {
      if(argc >= 1) {
        argIndex = 0;
      } else {
        Logger::getInstance()->log("JS: Property(obj).setValue: need one argument: value", lsError);
      }
    } else {
      if(argc >= 2) {
        std::string propName = ctx->convertTo<std::string>(argv[0]);
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
        if(JSVAL_IS_STRING(argv[argIndex])) {
          node->setStringValue(ctx->convertTo<std::string>(argv[argIndex]));
        } else if(JSVAL_IS_BOOLEAN(argv[argIndex])) {
          node->setBooleanValue(ctx->convertTo<bool>(argv[argIndex]));
        } else if(JSVAL_IS_INT(argv[argIndex])) {
          node->setIntegerValue(ctx->convertTo<int>(argv[argIndex]));
        } else {
          Logger::getInstance()->log("JS: setProperty: unknown type of argument 2", lsError);
        }
        *rval = JSVAL_TRUE;
        return JS_TRUE;
      } catch(PropertyTypeMismatch&) {
        Logger::getInstance()->log("Error setting value of " + node->getDisplayName(), lsFatal);
      }
    } else {
      Logger::getInstance()->log("Coule not create property " + node->getDisplayName(), lsFatal);
      *rval = JSVAL_FALSE;
      return JS_TRUE;
    }
    return JS_FALSE;
  } // prop_setProperty

  JSBool prop_getProperty(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, obj);
    if(node == NULL) {
      if(argc >= 1) {
        std::string propName = ctx->convertTo<std::string>(argv[0]);
        node = ext->getProperty(ctx, propName);
      } else {
        Logger::getInstance()->log("JS: Property.getProperty: need one argument: property-path", lsError);
      }
    }
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
        *rval = JSVAL_NULL;
      }
    }
    return JS_TRUE;
  } // prop_getProperty

  JSBool prop_setListener(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, obj);
    jsval function;
    if(node != NULL) {
      if(argc >= 1) {
        function = argv[0];
      } else {
        Logger::getInstance()->log("JS: Property(obj).setListener: need one argument callback", lsError);
        return JS_FALSE;
      }
    } else {
      if(argc >= 2) {
        std::string nodePath = ctx->convertTo<std::string>(argv[0]);
        node = ext->getProperty(ctx, nodePath);
        if(node == NULL) {
          *rval = JSVAL_NULL;
          Logger::getInstance()->log("JS: Property.setListener: cannot find node '" + nodePath, lsError);
          return JS_TRUE;
        }
        function = argv[1];
      } else {
        Logger::getInstance()->log("JS: Property.setListener: need two arguments: property-path &  callback", lsError);
        return JS_FALSE;
      }
    }

    std::string ident = ext->produceListenerID();
    PropertyScriptListener* listener =
        new PropertyScriptListener(ext, ctx, obj, function, ident);
    ext->addListener(listener);
    node->addListener(listener);

    JSString* str = JS_NewStringCopyZ(cx, ident.c_str());
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
  } // prop_setListener

  JSBool prop_removeListener(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc < 1) {
      Logger::getInstance()->log("JS: prop_removeListener: need one argument: listener-id", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string listenerIdent = ctx->convertTo<std::string>(argv[0]);
      ext->removeListener(listenerIdent);

      *rval = JSVAL_TRUE;
      return JS_TRUE;
    }
    return JS_FALSE;
  } // prop_removeListener

  JSBool prop_setFlag(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, obj);
    std::string flagName;
    bool value;
    if(node != NULL) {
      if(argc >= 2) {
        flagName = ctx->convertTo<std::string>(argv[0]);
        value = ctx->convertTo<bool>(argv[1]);
      } else {
        Logger::getInstance()->log("JS: Property(obj).setFlag needs two parameter flagName and value", lsError);
        return JS_FALSE;
      }
    } else {
      if(argc >= 3) {
        std::string nodePath = ctx->convertTo<std::string>(argv[0]);
        node = ext->getProperty(ctx, nodePath);
        if(node == NULL) {
          *rval = JSVAL_NULL;
          Logger::getInstance()->log("JS: Property.setFlag: cannot find node '" + nodePath, lsError);
          return JS_TRUE;
        }
        flagName = ctx->convertTo<std::string>(argv[1]);
        value = ctx->convertTo<bool>(argv[2]);
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
      *rval = JSVAL_FALSE;
      return JS_TRUE;
    }

    node->setFlag(flag, value);
    *rval = JSVAL_NULL;
    return JS_TRUE;
  } // prop_setFlag

  JSBool prop_hasFlag(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, obj);
    std::string flagName;
    if(node != NULL) {
      if(argc >= 1) {
        flagName = ctx->convertTo<std::string>(argv[0]);
      } else {
        Logger::getInstance()->log("JS: Property(obj).hasFlag needs at least one parameter flagName", lsError);
        return JS_FALSE;
      }
    } else {
      if(argc >= 2) {
        std::string nodePath = ctx->convertTo<std::string>(argv[0]);
        node = ext->getProperty(ctx, nodePath);
        if(node == NULL) {
          *rval = JSVAL_NULL;
          Logger::getInstance()->log("JS: Property.hasFlag: cannot find node '" + nodePath, lsError);
          return JS_TRUE;
        }
        flagName = ctx->convertTo<std::string>(argv[1]);
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
      *rval = JSVAL_NULL;
      return JS_TRUE;
    }

    if(node->hasFlag(flag)) {
      *rval = JSVAL_TRUE;
    } else {
      *rval = JSVAL_FALSE;
    }
    return JS_TRUE;
  } // prop_setFlag

  JSBool prop_getChild(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr node = ext->getPropertyFromObj(ctx, obj);
    assert(node != NULL);
    std::string nodeName;
    if(argc >= 1) {
      nodeName = ctx->convertTo<std::string>(argv[0]);
    } else {
      Logger::getInstance()->log("JS: Property(obj).getChild needs at least one parameter nodeName", lsError);
      return JS_FALSE;
    }

    PropertyNodePtr childNode = node->getProperty(nodeName);
    if(childNode != NULL) {
      *rval = OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, childNode));
    } else {
      *rval = JSVAL_NULL;
    }
    return JS_TRUE;
  } // prop_getChild

  JSBool prop_getNode(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    PropertyNodePtr pNode;
    if(argc >= 1) {
      pNode = ext->getProperty(ctx, ctx->convertTo<std::string>(argv[0]));
      if(pNode == NULL) {
        Logger::getInstance()->log("JS: Property.getNode: could not find node", lsWarning);
        *rval = JSVAL_NULL;
        return JS_TRUE;
      }
    } else {
      Logger::getInstance()->log("JS: Property.getNode needs at least one parameter nodeName", lsError);
      return JS_FALSE;
    }

    *rval = OBJECT_TO_JSVAL(ext->createJSProperty(*ctx, pNode));
    return JS_TRUE;
  } // prop_getNode

  JSBool prop_store(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    if(ext->store(ctx)) {
      *rval = JSVAL_TRUE;
    } else {
      *rval = JSVAL_FALSE;
    }
    return JS_TRUE;
  } // prop_store

  JSBool prop_load(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));

    if(ext->load(ctx)) {
      *rval = JSVAL_TRUE;
    } else {
      *rval = JSVAL_FALSE;
    }
    return JS_TRUE;
  } // prop_load

  JSFunctionSpec prop_methods[] = {
    {"setValue", prop_setProperty, 1, 0, 0},
    {"getValue", prop_getProperty, 0, 0, 0},
    {"setListener", prop_setListener, 1, 0, 0},
    {"removeListener", prop_removeListener, 1, 0, 0},
    {"getChild", prop_getChild, 1, 0, 0},
    {"setFlag", prop_setFlag, 2, 0, 0},
    {"hasFlag", prop_hasFlag, 1, 0, 0},
    {NULL, NULL, 0, 0, 0},
  };

  typedef struct {
    PropertyNodePtr pNode;
  } prop_wrapper;

  void property_finalize(JSContext *cx, JSObject *obj) {
    prop_wrapper* wrapper = static_cast<prop_wrapper*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete wrapper;
  } // property_finalize

  JSBool property_construct(JSContext *cx, JSObject *obj, uintN argc,
                            jsval *argv, jsval *rval) {
    if(argc >= 1) {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string propName = ctx->convertTo<std::string>(argv[0]);
      PropertyNodePtr pNode = ext->getProperty(ctx, propName);
      if(pNode == NULL) {
        pNode = ext->createProperty(ctx, propName);
      }
      if(pNode != NULL) {
        prop_wrapper* wrapper = new prop_wrapper;
        JS_SetPrivate(cx, obj, wrapper);
        wrapper->pNode = pNode;
        return JS_TRUE;
      } else {
        Logger::getInstance()->log("JS: Property.construct: Could not get/create node '" + propName + "'", lsFatal);
      }
    }
    return JS_FALSE;
  } // property_construct

  JSFunctionSpec prop_static_methods[] = {
    {"getProperty", prop_getProperty, 1, 0, 0},
    {"setProperty", prop_setProperty, 1, 0, 0},
    {"setListener", prop_setListener, 2, 0, 0},
    {"removeListener", prop_removeListener, 1, 0, 0},
    {"setFlag", prop_setFlag, 3, 0, 0},
    {"hasFlag", prop_hasFlag, 2, 0, 0},
    {"getNode", prop_getNode, 1, 0, 0},
    {"store", prop_store, 0, 0, 0},
    {"load", prop_load, 0, 0, 0},
    {NULL, NULL, 0, 0, 0},
  };

  static JSClass prop_class = {
    "Property", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  property_finalize, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // prop_class

  void PropertyScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
                 NULL, &prop_class, property_construct, 0, NULL /* prop_properties */,
                 prop_methods, NULL, prop_static_methods);
  } // extendContext

  JSObject* PropertyScriptExtension::createJSProperty(ScriptContext& _ctx, PropertyNodePtr _node) {
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

  void PropertyScriptExtension::addListener(PropertyScriptListener* _pListener) {
    m_Listeners.push_back(_pListener);
  } // addListener

  void PropertyScriptExtension::removeListener(const std::string& _identifier, bool _destroy) {
    for(std::vector<PropertyScriptListener*>::iterator it = m_Listeners.begin(), e = m_Listeners.end();
        it != e; ++it) {
      if((*it)->getIdentifier() == _identifier) {
        PropertyScriptListener* pListener = *it;
        pListener->unsubscribe();
        m_Listeners.erase(it);
        if(_destroy) {
          delete pListener;
        }
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
    if(JS_InstanceOf(_context->getJSContext(), _obj, &prop_class, NULL) == JS_TRUE) {
      prop_wrapper* pWrapper = static_cast<prop_wrapper*>(JS_GetPrivate(_context->getJSContext(), _obj));
      result = pWrapper->pNode;
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
    m_Identifier(_identifier)
  {
    m_FunctionRoot.rootFunction(getContext(), m_pFunctionObject, m_Function);
  } // ctor

  PropertyScriptListener::~PropertyScriptListener() {
    m_pExtension->removeListener(m_Identifier, false);
  }

  void PropertyScriptListener::createScriptObject() {
    if(m_pScriptObject == NULL) {
      m_pScriptObject.reset(new ScriptObject(m_pFunctionObject, *getContext()));
    }
    assert(m_pScriptObject != NULL);
  } // createScriptObject

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
    ScriptLock lock(getContext(), true);
    boost::shared_ptr<JSContextThread> req;
    if(lock.ownsLock()) {
      req.reset(new JSContextThread(getContext()));
    }
    JSAutoLocalRootScope rootScope(getContext()->getJSContext());
    createScriptObject();
    ScriptFunctionParameterList list(*getContext());
    list.add(_changedNode->getDisplayName());
    try {
      m_pScriptObject->callFunctionByReference<void>(m_Function, list);
    } catch(ScriptException& e) {
      Logger::getInstance()->log("PropertyScriptListener::doOnChange: Caught exception while calling handler: " + std::string(e.what()), lsFatal);
    }
    JS_MaybeGC(getContext()->getJSContext());
  } // doOnChange

  void PropertyScriptListener::stop()  {
    ScriptContextAttachedObject::stop();
    ScriptLock lock(getContext());
    boost::shared_ptr<JSContextThread> req;
    delete this;
  }

} // namespace dss
