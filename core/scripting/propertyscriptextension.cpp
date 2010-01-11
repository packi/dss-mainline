/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

namespace dss {

  //================================================== PropertyScriptExtension

  const std::string PropertyScriptExtensionName = "propertyextension";

  PropertyScriptExtension::PropertyScriptExtension(PropertySystem& _propertySystem)
  : ScriptExtension(PropertyScriptExtensionName),
    m_PropertySystem(_propertySystem),
    m_NextListenerID(1)
  { } // ctor

  JSBool global_prop_setProperty(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc < 2) {
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
    if(argc < 1) {
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
          *rval = JSVAL_NULL;
        }
      }
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_getProperty

  JSBool global_prop_setListener(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc < 2) {
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
    if(argc < 1) {
      Logger::getInstance()->log("JS: global_prop_removeListener: need one argument: listener-id", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string listenerIdent = ctx->convertTo<std::string>(argv[0]);
      ext->removeListener(listenerIdent);

      *rval = JSVAL_TRUE;
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_removeListener

  JSBool global_prop_setFlag(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc < 3) {
      Logger::getInstance()->log("JS: global_prop_setFlag: need three arguments: prop-path, flag, value", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string propName = ctx->convertTo<std::string>(argv[0]);
      PropertyNodePtr node = ext->getPropertySystem().getProperty(propName);
      std::string flagName = ctx->convertTo<std::string>(argv[1]);
      bool value = ctx->convertTo<bool>(argv[2]);
      PropertyNode::Flag flag;
      if(flagName == "ARCHIVE") {
        flag = PropertyNode::Archive;
      } else if(flagName == "WRITEABLE") {
        flag = PropertyNode::Writeable;
      } else if(flagName == "READABLE") {
        flag = PropertyNode::Readable;
      } else {
        Logger::getInstance()->log("JS: global_prop_setFlag: Invalid value for flag: " + flagName, lsError);
        *rval = JSVAL_TRUE;
        return JS_TRUE;
      }

      node->setFlag(flag, value);
      *rval = JSVAL_TRUE;
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_setFlag

  JSBool global_prop_hasFlag(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc < 2) {
      Logger::getInstance()->log("JS: global_prop_hasFlag: need three arguments: prop-path, flag", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      PropertyScriptExtension* ext = dynamic_cast<PropertyScriptExtension*>(ctx->getEnvironment().getExtension(PropertyScriptExtensionName));
      std::string propName = ctx->convertTo<std::string>(argv[0]);
      PropertyNodePtr node = ext->getPropertySystem().getProperty(propName);
      std::string flagName = ctx->convertTo<std::string>(argv[1]);
      PropertyNode::Flag flag;
      if(flagName == "ARCHIVE") {
        flag = PropertyNode::Archive;
      } else if(flagName == "WRITEABLE") {
        flag = PropertyNode::Writeable;
      } else if(flagName == "READABLE") {
        flag = PropertyNode::Readable;
      } else {
        Logger::getInstance()->log("JS: global_prop_hasFlag: Invalid value for flag: " + flagName, lsError);
        *rval = JSVAL_NULL;
        return JS_TRUE;
      }

      if(node->hasFlag(flag)) {
        *rval = JSVAL_TRUE;
      } else {
        *rval = JSVAL_FALSE;
      }
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_setFlag

  JSFunctionSpec prop_global_methods[] = {
    {"setProperty", global_prop_setProperty, 2, 0, 0},
    {"getProperty", global_prop_getProperty, 1, 0, 0},
    {"setListener", global_prop_setListener, 2, 0, 0},
    {"removeListener", global_prop_removeListener, 1, 0, 0},
    {"setFlag", global_prop_setFlag, 3, 0, 0},
    {"hasFlag", global_prop_hasFlag, 2, 0, 0},
    {NULL, NULL, 0, 0, 0},
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
  { } // ctor

  PropertyScriptListener::~PropertyScriptListener() {
    m_pExtension->removeListener(m_Identifier);
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

}
