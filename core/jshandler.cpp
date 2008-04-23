/*
 *  jshandler.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "jshandler.h"

namespace dss {
  
  //============================================= ScriptEnvironment
  
  ScriptEnvironment::ScriptEnvironment() 
  : m_pRuntime(NULL)
  {
  } // ctor
  
  ScriptEnvironment::~ScriptEnvironment() {
    JS_DestroyRuntime(m_pRuntime);
  } // dtor
  
  
  void ScriptEnvironment::Initialize() {
    m_pRuntime = JS_NewRuntime(8L * 1024L * 1024L);
    if (m_pRuntime == NULL) {
      throw new ScriptException("Error creating environment");
    }
  } // Initialize
  
  void ScriptEnvironment::AddExtension(ScriptExtension* _pExtension) {
    m_Extensions.push_back(_pExtension);
  } // AddExtension
  
  ScriptContext* ScriptEnvironment::GetContext() {
    JSContext* context = JS_NewContext(m_pRuntime, 8192);
    
    ScriptContext* pResult = new ScriptContext(*this, context);
    JS_SetContextPrivate(context, pResult);
    for(vector<ScriptExtension*>::iterator ipExtension = m_Extensions.begin(); ipExtension != m_Extensions.end(); ++ipExtension) {
      (*ipExtension)->ExtendContext(*pResult);
    }
    return pResult;
  } // GetContext

  ScriptExtension* ScriptEnvironment::GetExtension(const string& _name) const {
    for(vector<ScriptExtension*>::const_iterator ipExtension = m_Extensions.begin(); ipExtension != m_Extensions.end(); ++ipExtension) {
      if((*ipExtension)->GetName() == _name) {
        return *ipExtension;
      }
    }
    return NULL;
  } // GetExtension
  
  //============================================= ScriptContext
  
  void ScriptContext::JsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er) {
    char *pointer=NULL;
    char *line=NULL;
    int len;
    
    if (er->linebuf != NULL){
      len = er->tokenptr - er->linebuf + 1;
      pointer = (char*)malloc(len);
      memset(pointer, '-', len);
      pointer[len-1]='\0';
      pointer[len-2]='^';
      
      len = strlen(er->linebuf)+1;
      line = (char*)malloc(len);
      strncpy(line, er->linebuf, len);
      line[len-1] = '\0';
    }
    else {
      len=0;
      pointer = (char*)malloc(1);
      line = (char*)malloc(1);
      pointer[0]='\0';
      line[0] = '\0';
    }
    
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')){ line[len-1]='\0'; len--; }
    
    printf("JS Error: %s\nFile: %s:%u\n", msg, er->filename, er->lineno);
    if (line[0]){
      printf("%s\n%s\n", line, pointer);
    }
    
    free(pointer);
    free(line);
  } // JsErrorHandler
  
  
  JSBool global_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    if (argc < 1){
      /* No arguments passed in, so do nothing. */
      /* We still want to return JS_TRUE though, other wise an exception will be thrown by the engine. */
      *rval = INT_TO_JSVAL(0); /* Send back a return value of 0. */
    } else {
      int i;
      size_t amountWritten=0;
      for (i=0; i<argc; i++){
        JSString *val = JS_ValueToString(cx, argv[i]); /* Convert the value to a javascript string. */
        char *str = JS_GetStringBytes(val); /* Then convert it to a C-style string. */
        size_t length = JS_GetStringLength(val); /* Get the length of the string, # of chars. */
        amountWritten = fwrite(str, sizeof(*str), length, stdout); /* write the string to stdout. */
      }
      *rval = INT_TO_JSVAL(amountWritten); /* Set the return value to be the number of bytes/chars written */
    }
    fwrite("\n", 1, 1, stdout);
    return JS_TRUE;
  }
  
  static JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE, /* use the new resolve hook */
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    /* todo: explain */
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  };
  
  JSFunctionSpec global_methods[] = {
    {"print", global_print, 1, 0, 0},
    {NULL},
  };

  ScriptContext::ScriptContext(ScriptEnvironment& _env, JSContext* _pContext) 
  : m_Environment(_env),
    m_pContext(_pContext)
  {
    JS_SetOptions(m_pContext, JSOPTION_VAROBJFIX | JSOPTION_DONT_REPORT_UNCAUGHT);
    JS_SetErrorReporter(m_pContext, JsErrorHandler);
    
    /* Create the global object. */
    m_pRootObject = JS_NewObject(m_pContext, &global_class, NULL, NULL);
    if (m_pRootObject == NULL) {
      throw new ScriptException("Could not create root-object");
    }
    
    JS_DefineFunctions(m_pContext, m_pRootObject, global_methods);
    
    /* Populate the global object with the standard globals,
     like Object and Array. */
    if (!JS_InitStandardClasses(m_pContext, m_pRootObject)) {
      throw new ScriptException("InitStandardClasses failed");
    }
    
  } // ctor
  
  ScriptContext::~ScriptContext() {
    JS_SetContextPrivate(m_pContext, NULL);
    JS_RemoveRoot(m_pContext, m_pSourceObject);
    JS_DestroyScript(m_pContext, m_pScriptToExecute);
    JS_DestroyContext(m_pContext);
  } // dtor
  
  void ScriptContext::LoadFromFile(const string& _fileName) {
    m_FileName = _fileName;
    if(!FileExists(_fileName)) {
      throw new ScriptException(string("File \"") + _fileName + "\" not found");
    }
    
    m_pScriptToExecute = JS_CompileFile(m_pContext, m_pRootObject, _fileName.c_str());
    if(m_pScriptToExecute == NULL) {
      throw new ScriptException(string("Could not parse file \"") + _fileName + "\"");
    }
    
    // protect newly allocated script from garbage-collection
    m_pSourceObject = JS_NewScriptObject(m_pContext, m_pScriptToExecute);
    if (m_pSourceObject == NULL
        || !JS_AddNamedRoot(m_pContext, &m_pSourceObject, "scrobj")) {
      throw new ScriptException("Could not add named root for script");
    }
    
  } // LoadFromFile
  
  void ScriptContext::LoadFromMemory(const char* _script) {
    m_pScriptToExecute = JS_CompileScript(m_pContext, m_pRootObject, _script, strlen(_script), "memory", 1);
    if(m_pScriptToExecute == NULL) {
      throw new ScriptException(string("Could not parse in-memory script"));
    }
  } // LoadFromMemory

  template <>
  jsval ScriptContext::Evaluate() {
    jsval rval;
    JSBool ok = JS_ExecuteScript(m_pContext, m_pRootObject, m_pScriptToExecute, &rval);
    if(ok) {
      return rval;
    } else {
      if(JS_IsExceptionPending(m_pContext)) {
        jsval exval;
        if(JS_GetPendingException(m_pContext, &exval)) {
          JS_ClearPendingException(m_pContext);
          JSString* errstr = JS_ValueToString(m_pContext, exval);
          if(errstr != NULL) {
            const char* errmsgBytes = JS_GetStringBytes(errstr);
            throw new ScriptRuntimeException(string("Caught Exception while executing script: ") + errmsgBytes, string(errmsgBytes));
          }
        } 
        throw new ScriptException("Exception was pending after script execution, but couldnt get it from the vm");
      } else {
        throw new ScriptException("Error executing script");
      }
    }
  } // Evaluate<jsval>
  
  template <>
  double ScriptContext::Evaluate() {
    jsval rval = Evaluate<jsval>();
    jsdouble result;
    if( JS_ValueToNumber(m_pContext, rval, &result) != JS_TRUE) {
      throw new ScriptException("Could not convert jsval to double");
    }
       
    return result;
  } // Evaluate<double>
  
  template <>
  void ScriptContext::Evaluate() {
    Evaluate<jsval>();
  } // Evaluate<void>
  
  template <>
  string ScriptContext::Evaluate() {
    jsval rval = Evaluate<jsval>();
    JSString* result;
    result = JS_ValueToString(m_pContext, rval);
    if( result == NULL) {
      throw new ScriptException("Could not convert jsval to JSString");
    }
    
    return string(JS_GetStringBytes(result));
  } // Evaluate<string>
  
  //================================================== ScriptExtension
  
}