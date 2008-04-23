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
  
  
  JSBool global_get_name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    
    ModelScriptContextExtension* ext = dynamic_cast<ModelScriptContextExtension*>(ctx->GetEnvironment().GetExtension(ModelScriptcontextExtensionName));
    if(ext != NULL) {
      string aptName = ext->GetApartment().GetName();
      JSString* str = JS_NewStringCopyN(cx, aptName.c_str(), aptName.size());
      
      *rval = STRING_TO_JSVAL(str);
      return JS_TRUE;
    }
    return JS_TRUE;
  }
  
  JSFunctionSpec model_global_methods[] = {
    {"getName", global_get_name, 0, 0, 0},
    {NULL},
  };
  
  
  void ModelScriptContextExtension::ExtendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.GetJSContext(), JS_GetGlobalObject(_context.GetJSContext()), model_global_methods);
  } // ExtendedJSContext
  
}
