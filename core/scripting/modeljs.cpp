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
    if(obj.GetProperty<string>("className") == "set") {
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
    {"remove", set_length, 1, 0, 0},    
    {NULL},
  };
  
  static JSPropertySpec set_properties[] = {
    {"className",0, 0,set_JSGet},
    {NULL}
  };
  
  JSObject* ModelScriptContextExtension::CreateJSSet(ScriptContext& _ctx, Set& _set) {
    JSObject* result = JS_NewObject(_ctx.GetJSContext(), &set_class, NULL, NULL);
    JS_DefineFunctions(_ctx.GetJSContext(), result, set_methods);
    JS_DefineProperties(_ctx.GetJSContext(), result, set_properties);
    Set* innerObj = new Set(_set);
    JS_SetPrivate(_ctx.GetJSContext(), result, innerObj);
    return result;
  } // CreateJSSet
  
}
