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

#include "jshandler.h"

#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "core/logger.h"
#include "core/scripting/scriptobject.h"

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
    } else {
      len=0;
      pointer = (char*)malloc(1);
      line = (char*)malloc(1);
      pointer[0]='\0';
      line[0] = '\0';
    }

    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')){ line[len-1]='\0'; len--; }


    std::ostringstream logSStream;
    logSStream << "JS Error: " << msg << " in file: " << er->filename << ":" << er->lineno;
    if (line[0]){
      logSStream << "; line: " << line << "; pointer: " << pointer;
    }
    Logger::getInstance()->log(logSStream.str(), lsError);

    free(pointer);
    free(line);
  } // jsErrorHandler


  JSBool global_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    JSRequest req(cx);
    if (argc < 1){
      /* No arguments passed in, so do nothing.
        We still want to return JS_TRUE though, other wise an exception will be
        thrown by the engine. */
      *rval = INT_TO_JSVAL(0);
    } else {
      std::stringstream sstream;
      sstream << "JS: ";
      for(unsigned int i = 0; i < argc; i++) {
        JSString *val = JS_ValueToString(cx, argv[i]);
        char *str = JS_GetStringBytes(val);
        size_t length = JS_GetStringLength(val);
        std::string stdStr(str, length);
        sstream << stdStr;
      }
      *rval = BOOLEAN_TO_JSVAL(true);
      Logger::getInstance()->log(sstream.str());
    }
    return JS_TRUE;
  } // global_print

  JSBool global_keepContext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ctx->setKeepContext(true);
    return JS_TRUE;
  } // global_keepContext

  class SessionAttachedTimeoutObject : public ScriptContextAttachedObject {
  public:
    SessionAttachedTimeoutObject(ScriptContext* _pContext)
    : ScriptContextAttachedObject(_pContext)
    { }

    void timeout(int _timeoutMS, jsval _function, JSObject* _obj, boost::shared_ptr<ScriptFunctionRooter> _rooter) {
      sleepMS(_timeoutMS);
      AssertLocked ctxLock(getContext());
      Logger::getInstance()->log("setTimeout: aquiring request");
      JSContext* cx = getContext()->getJSContext();
      JS_SetContextThread(cx);
      JSRequest req(getContext());
      ScriptObject obj(_obj, *getContext());
      ScriptFunctionParameterList params(*getContext());
      obj.callFunctionByReference<void>(_function, params);
      //JS_GC(cx);
      req.endRequest();
      JS_ClearContextThread(cx);
      Logger::getInstance()->log("setTimeout: releasing request");
      delete this;
    }
  };

  JSBool global_setTimeout(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSRequest req(cx);
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    if(argc >= 2) {
      int timeoutMS;
      try {
        timeoutMS = ctx->convertTo<int>(argv[0]);
      } catch(ScriptException& e) {
        Logger::getInstance()->log("Parameter timeoutMS is not of type int");
        return JS_FALSE;
      }
      boost::shared_ptr<ScriptFunctionRooter> functionRoot(new ScriptFunctionRooter(ctx, obj, argv[1]));
      SessionAttachedTimeoutObject* pTimeoutObj = new SessionAttachedTimeoutObject(ctx);
      req.endRequest();
      boost::thread(boost::bind(&SessionAttachedTimeoutObject::timeout, pTimeoutObj, timeoutMS, argv[1], obj, functionRoot));
    }
    return JS_TRUE;
  } // global_setTimeout

  static JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE, /* use the new resolve hook */
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSFunctionSpec global_methods[] = {
    {"print", global_print, 1, 0, 0},
    {"keepContext", global_keepContext, 0, 0, 0},
    {"setTimeout", global_setTimeout, 2, 0, 0},
    JS_FS_END
  };

  ScriptContext::ScriptContext(ScriptEnvironment& _env, JSContext* _pContext)
  : m_Environment(_env),
    m_pContext(_pContext),
    m_KeepContext(false)
  {
    JSRequest req(m_pContext);
    JS_SetOptions(m_pContext, JSOPTION_VAROBJFIX | JSOPTION_DONT_REPORT_UNCAUGHT);
    JS_SetErrorReporter(m_pContext, jsErrorHandler);

    /* Create the global object. */
    m_pRootObject = JS_NewObject(m_pContext, &global_class, NULL, NULL);
    if (m_pRootObject == NULL) {
      throw ScriptException("Could not create root-object");
    }
    m_RootObject.reset(new ScriptObject(m_pRootObject, *this));

    JS_DefineFunctions(m_pContext, m_pRootObject, global_methods);

    /* Populate the global object with the standard globals,
     like Object and Array. */
    if (!JS_InitStandardClasses(m_pContext, m_pRootObject)) {
      throw ScriptException("InitStandardClasses failed");
    }

    JS_SetContextPrivate(m_pContext, this);
  } // ctor

  ScriptContext::~ScriptContext() {
    //scrubVector(m_AttachedObjects);
    Logger::getInstance()->log("destroying script context " + intToString(int(this), true));
    if(!m_AttachedObjects.empty()) {
      Logger::getInstance()->log("Still have some attached objects (" + intToString(m_AttachedObjects.size()) + "). Memory leak?", lsError);
    }
//    JS_GC(m_pContext);
    JS_SetContextPrivate(m_pContext, NULL);
    JS_DestroyContext(m_pContext);
    m_pContext = NULL;
    Logger::getInstance()->log("destroyed script context " + intToString(int(this), true));
  } // dtor

  void ScriptContext::attachObject(ScriptContextAttachedObject* _pObject) {
    Logger::getInstance()->log("Attaching object " + intToString(int(_pObject), true), lsDebug);
    m_AttachedObjects.push_back(_pObject);
  } // attachObject

  void ScriptContext::removeAttachedObject(ScriptContextAttachedObject* _pObject) {
    Logger::getInstance()->log("Removing attached object " + intToString(int(_pObject), true), lsDebug);
    std::vector<ScriptContextAttachedObject*>::iterator it =
       std::find(m_AttachedObjects.begin(), m_AttachedObjects.end(), _pObject);
    if(it != m_AttachedObjects.end()) {
      m_AttachedObjects.erase(it);
    }
  } // removeAttachedObject

  bool ScriptContext::raisePendingExceptions() {
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
    }
    return false;
  } // raisePendingExceptions

  jsval ScriptContext::doEvaluateScript(const std::string& _fileName) {
    AssertLocked lock(this);
    JSRequest req(this);

    std::ifstream in(_fileName.c_str());
    if(!in.is_open()) {
      throw ScriptException("Could not open script-file: '" + _fileName + "'");
    }
    std::string line;
    std::stringstream sstream;
    while(std::getline(in,line)) {
      sstream << line << "\n";
    }
    jsval rval;
    std::string script = sstream.str();
    JSBool ok = JS_EvaluateScript(m_pContext, m_pRootObject, script.c_str(), script.size(),
                       _fileName.c_str(), 0, &rval);
    req.endRequest();
    JS_ClearContextThread(m_pContext);
    if(ok) {
      return rval;
    } else {
      raisePendingExceptions();
      throw ScriptException("Error executing script");
    }
  } // doEvaluateScript

  template <>
  void ScriptContext::evaluateScript(const std::string& _script) {
    doEvaluateScript(_script);
  } // evaluateScript<void>

  template <>
  int ScriptContext::evaluateScript(const std::string& _script) {
    return convertTo<int>(doEvaluateScript(_script));
  } // evaluateScript<int>

  template <>
  double ScriptContext::evaluateScript(const std::string& _script) {
    return convertTo<double>(doEvaluateScript(_script));
  } // evaluateScript<double>

  template <>
  std::string ScriptContext::evaluateScript(const std::string& _script) {
    return convertTo<std::string>(doEvaluateScript(_script));
  } // evaluateScript<std::string>

  template <>
  bool ScriptContext::evaluateScript(const std::string& _script) {
    return convertTo<bool>(doEvaluateScript(_script));
  } // evaluateScript<bool>

  jsval ScriptContext::doEvaluate(const std::string& _script) {
    AssertLocked lock(this);
    JSRequest req(this);

    JS_SetContextThread(m_pContext);
    const char* filename = "temporary_script";
    jsval rval;
    JSBool ok = JS_EvaluateScript(m_pContext, m_pRootObject, _script.c_str(), _script.size(),
                       filename, 0, &rval);
    req.endRequest();
    JS_ClearContextThread(m_pContext);
    if(ok) {
      return rval;
    } else {
      raisePendingExceptions();
      throw ScriptException("Error executing script");
    }
  } // doEvaluate

  template <>
  void ScriptContext::evaluate(const std::string& _script) {
    doEvaluate(_script);
  } // evaluate<void>

  template <>
  int ScriptContext::evaluate(const std::string& _script) {
    return convertTo<int>(doEvaluate(_script));
  } // evaluate<int>

  template <>
  double ScriptContext::evaluate(const std::string& _script) {
    return convertTo<double>(doEvaluate(_script));
  } // evaluate<double>

  template <>
  std::string ScriptContext::evaluate(const std::string& _script) {
    return convertTo<std::string>(doEvaluate(_script));
  } // evaluate<std::string>

  template <>
  bool ScriptContext::evaluate(const std::string& _script) {
    return convertTo<bool>(doEvaluate(_script));
  } // evaluate<bool>


  //================================================== ScriptExtension


  //================================================== ScriptFunctionParameterList

  template<>
  void ScriptFunctionParameterList::add(int _value) {
    m_Parameter.push_back(INT_TO_JSVAL(_value));
  } // add<int>

  template<>
  void ScriptFunctionParameterList::add(unsigned short _value) {
    m_Parameter.push_back(INT_TO_JSVAL(_value));
  } // add<unsigned short>

  template<>
  void ScriptFunctionParameterList::add(double _value) {
    jsval val;
    if(JS_NewNumberValue(m_Context.getJSContext(), _value, &val) == JS_TRUE) {
      m_Parameter.push_back(val);
    }
    throw ScriptException("ScriptFunctionParameterList::add<double>: Could not allocate double value");
  } // add<double>

  template<>
  void ScriptFunctionParameterList::add(bool _value) {
    m_Parameter.push_back(BOOLEAN_TO_JSVAL(_value));
  } // add<bool>

  template<>
  void ScriptFunctionParameterList::add(const std::string& _value) {
    AssertLocked objLock(&m_Context);
    JSString* str = JS_NewStringCopyN(m_Context.getJSContext(), _value.c_str(), _value.size());
    m_Parameter.push_back(STRING_TO_JSVAL(str));
  } // add<const std::string&>

  template<>
  void ScriptFunctionParameterList::add(std::string _value) {
    AssertLocked objLock(&m_Context);
    JSString* str = JS_NewStringCopyN(m_Context.getJSContext(), _value.c_str(), _value.size());
    m_Parameter.push_back(STRING_TO_JSVAL(str));
  } // add<std::string&>

  void ScriptFunctionParameterList::addJSVal(jsval _value) {
    m_Parameter.push_back(_value);
  } // addJSVal


  //=============================================== ScriptFunctionRooter

  ScriptFunctionRooter::ScriptFunctionRooter()
  : m_pObject(NULL),
    m_pFunction(NULL),
    m_pContext(NULL)
  {} // ctor

  ScriptFunctionRooter::ScriptFunctionRooter(ScriptContext* _pContext, JSObject* _pObject, jsval _function)
  : m_pObject(NULL),
    m_pFunction(NULL),
    m_pContext(NULL)
  {
    rootFunction(_pContext, _pObject, _function);
  } // ctor

  ScriptFunctionRooter::~ScriptFunctionRooter() {
    if(m_pFunction != NULL) {
      assert(m_pContext != NULL);
      JS_RemoveRoot(m_pContext->getJSContext(), &m_pFunction);
    }
    if(m_pObject != NULL) {
      assert(m_pContext != NULL);
      JS_RemoveRoot(m_pContext->getJSContext(), &m_pObject);
    }
  } // dtor

  void ScriptFunctionRooter::rootFunction(ScriptContext* _pContext, JSObject* _pObject, jsval _function) {
    assert(_pContext != NULL);
    m_pContext = _pContext;
    m_pObject = _pObject;
    JSContext* cx = m_pContext->getJSContext();
    JSFunction* function = JS_ValueToFunction(cx, _function);
    m_pFunction = JS_GetFunctionObject(function);
    JS_AddRoot(cx, &m_pFunction);
    JS_AddRoot(cx, &m_pObject);
  } // rootFunction

} // namespace dss
