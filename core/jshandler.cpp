/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "jshandler.h"

#include <cstring>

namespace dss {

  //============================================= ScriptEnvironment

  ScriptEnvironment::ScriptEnvironment()
  : m_pRuntime(NULL)
  {
  } // ctor

  ScriptEnvironment::~ScriptEnvironment() {
    JS_DestroyRuntime(m_pRuntime);
    m_pRuntime = NULL;
  } // dtor


  void ScriptEnvironment::initialize() {
    m_pRuntime = JS_NewRuntime(8L * 1024L * 1024L);
    if (m_pRuntime == NULL) {
      throw ScriptException("Error creating environment");
    }
  } // initialize

  bool ScriptEnvironment::isInitialized() {
    return m_pRuntime != NULL;
  } // isInitialized

  void ScriptEnvironment::addExtension(ScriptExtension* _pExtension) {
    m_Extensions.push_back(_pExtension);
  } // addExtension

  ScriptContext* ScriptEnvironment::getContext() {
    JSContext* context = JS_NewContext(m_pRuntime, 8192);

    ScriptContext* pResult = new ScriptContext(*this, context);
    JS_SetContextPrivate(context, pResult);
    for(boost::ptr_vector<ScriptExtension>::iterator ipExtension = m_Extensions.begin(); ipExtension != m_Extensions.end(); ++ipExtension) {
      ipExtension->extendContext(*pResult);
    }
    return pResult;
  } // getContext

  ScriptExtension* ScriptEnvironment::getExtension(const std::string& _name) {
    for(boost::ptr_vector<ScriptExtension>::iterator ipExtension = m_Extensions.begin(); ipExtension != m_Extensions.end(); ++ipExtension) {
      if(ipExtension->getName() == _name) {
        return &*ipExtension;
      }
    }
    return NULL;
  } // getExtension

  const ScriptExtension* ScriptEnvironment::getExtension(const std::string& _name) const {
    for(boost::ptr_vector<ScriptExtension>::const_iterator ipExtension = m_Extensions.begin(); ipExtension != m_Extensions.end(); ++ipExtension) {
      if(ipExtension->getName() == _name) {
        return &*ipExtension;
      }
    }
    return NULL;
  } // getExtension

  //============================================= ScriptContext

  void ScriptContext::jsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er) {
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
  } // jsErrorHandler


  JSBool global_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    if (argc < 1){
      /* No arguments passed in, so do nothing. */
      /* We still want to return JS_TRUE though, other wise an exception will be thrown by the engine. */
      *rval = INT_TO_JSVAL(0); /* Send back a return value of 0. */
    } else {
      unsigned int i;
      size_t amountWritten=0;
      for (i=0; i<argc; i++){
        JSString *val = JS_ValueToString(cx, argv[i]); /* Convert the value to a javascript std::string. */
        char *str = JS_GetStringBytes(val); /* Then convert it to a C-style std::string. */
        size_t length = JS_GetStringLength(val); /* Get the length of the std::string, # of chars. */
        amountWritten = fwrite(str, sizeof(*str), length, stdout); /* write the std::string to stdout. */
      }
      *rval = INT_TO_JSVAL(amountWritten); /* Set the return value to be the number of bytes/chars written */
    }
    fwrite("\n", 1, 1, stdout);
    return JS_TRUE;
  }

  static JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE, /* use the new resolve hook */
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
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
    JS_SetErrorReporter(m_pContext, jsErrorHandler);

    /* Create the global object. */
    m_pRootObject = JS_NewObject(m_pContext, &global_class, NULL, NULL);
    if (m_pRootObject == NULL) {
      throw ScriptException("Could not create root-object");
    }

    JS_DefineFunctions(m_pContext, m_pRootObject, global_methods);

    /* Populate the global object with the standard globals,
     like Object and Array. */
    if (!JS_InitStandardClasses(m_pContext, m_pRootObject)) {
      throw ScriptException("InitStandardClasses failed");
    }

  } // ctor

  ScriptContext::~ScriptContext() {
    JS_SetContextPrivate(m_pContext, NULL);
//    JS_RemoveRoot(m_pContext, m_pSourceObject);
    JS_DestroyScript(m_pContext, m_pScriptToExecute);
    JS_DestroyContext(m_pContext);
  } // dtor

  void ScriptContext::loadFromFile(const std::string& _fileName) {
    m_FileName = _fileName;
    if(!fileExists(_fileName)) {
      throw ScriptException(std::string("File \"") + _fileName + "\" not found");
    }

    m_pScriptToExecute = JS_CompileFile(m_pContext, m_pRootObject, _fileName.c_str());
    if(m_pScriptToExecute == NULL) {
      throw ScriptException(std::string("Could not parse file \"") + _fileName + "\"");
    }
    /*
    // protect newly allocated script from garbage-collection
    m_pSourceObject = JS_NewScriptObject(m_pContext, m_pScriptToExecute);
    if (m_pSourceObject == NULL
        || !JS_AddNamedRoot(m_pContext, &m_pSourceObject, "scrobj")) {
      throw ScriptException("Could not add named root for script");
    }
    */
  } // loadFromFile

  void ScriptContext::loadFromMemory(const char* _script) {
    m_pScriptToExecute = JS_CompileScript(m_pContext, m_pRootObject, _script, strlen(_script), "memory", 1);
    if(m_pScriptToExecute == NULL) {
      throw ScriptException(std::string("Could not parse in-memory script"));
    }
  } // loadFromMemory

  template<>
  int ScriptContext::convertTo(const jsval& _val) {
    if(JSVAL_IS_NUMBER(_val)) {
      int result;
      if(JS_ValueToInt32(m_pContext, _val, &result)) {
        return result;
      }
    }
    throw ScriptException("Value is not of type int");
  }

  template<>
  std::string ScriptContext::convertTo(const jsval& _val) {
    JSString* result;
    result = JS_ValueToString(m_pContext, _val);
    if( result == NULL) {
      throw ScriptException("Could not convert jsval to JSString");
    }

    return std::string(JS_GetStringBytes(result));
  }

  template<>
  bool ScriptContext::convertTo(const jsval& _val) {
    if(JSVAL_IS_BOOLEAN(_val)) {
      JSBool result;
      if(JS_ValueToBoolean(m_pContext, _val, &result)) {
        return result;
      }
    }
    throw ScriptException("Value is not of type boolean");
  }

  template<>
  double ScriptContext::convertTo(const jsval& _val) {
    if(JSVAL_IS_NUMBER(_val)) {
      jsdouble result;
      if(JS_ValueToNumber(m_pContext, _val, &result)) {
        return result;
      }
    }
    throw ScriptException("Value is not of type double");
  }

  template <>
  jsval ScriptContext::evaluate() {
    jsval rval;
    JSBool ok = JS_ExecuteScript(m_pContext, m_pRootObject, m_pScriptToExecute, &rval);
    if(ok) {
      //JS_GC(m_pContext);
      return rval;
    } else {
      if(JS_IsExceptionPending(m_pContext)) {
        jsval exval;
        if(JS_GetPendingException(m_pContext, &exval)) {
          JS_ClearPendingException(m_pContext);
          JSString* errstr = JS_ValueToString(m_pContext, exval);
          if(errstr != NULL) {
            const char* errmsgBytes = JS_GetStringBytes(errstr);
            throw ScriptRuntimeException(std::string("Caught Exception while executing script: ") + errmsgBytes, std::string(errmsgBytes));
          }
        }
        throw ScriptException("Exception was pending after script execution, but couldnt get it from the vm");
      } else {
        throw ScriptException("Error executing script");
      }
    }
  } // evaluate<jsval>

  template <>
  double ScriptContext::evaluate() {
    return convertTo<double>(evaluate<jsval>());
  } // evaluate<double>


  template <>
  int ScriptContext::evaluate() {
    return convertTo<int>(evaluate<jsval>());
  } // evaluate<int>


  template <>
  void ScriptContext::evaluate() {
    evaluate<jsval>();
  } // evaluate<void>

  template <>
  std::string ScriptContext::evaluate() {
    return convertTo<std::string>(evaluate<jsval>());
  } // evaluate<string>

  //================================================== ScriptExtension

  //================================================== ScriptObject

  ScriptObject::ScriptObject(JSObject* _pObject, ScriptContext& _context)
  : m_pObject(_pObject),
    m_Context(_context)
  {
  } // ctor

  template<>
  jsval ScriptObject::getProperty(const std::string& _name) {
    JSBool found;
    if(!JS_HasProperty(m_Context.getJSContext(), m_pObject, _name.c_str(), &found)) {
      throw ScriptException("Could not enumerate property");
    }
    if(found) {
      jsval result;
      if(JS_GetProperty(m_Context.getJSContext(), m_pObject, _name.c_str(), &result)) {
        return result;
      } else {
        throw ScriptException(std::string("Could not retrieve value of property ") + _name);
      }
    } else {
      throw ScriptException(std::string("Could not find property ") + _name);
    }
  } // getProperty<jsval>

  template<>
  std::string ScriptObject::getProperty(const std::string& _name) {
    jsval value = getProperty<jsval>(_name);
    if(JSVAL_IS_STRING(value)) {
      return std::string(JS_GetStringBytes(JSVAL_TO_STRING(value)));
    }
    throw ScriptException(std::string("Property is not of std::string type: ") + _name);
  } // getProperty<string>

  bool ScriptObject::is(const std::string& _className) {
    return getClassName() == _className;
  }

  const std::string ScriptObject::getClassName() {
    return getProperty<std::string>("className");
  } // getClassName
}
