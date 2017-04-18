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


#include "jshandler.h"

#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include <climits>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "src/logger.h"
#include "src/scripting/scriptobject.h"

#include "src/base.h"
#include "src/dss.h"
#include "src/propertysystem.h"
#include "src/security/user.h"
#include "src/security/security.h"
#include "src/session.h"

namespace dss {

  struct JsDeleter {
    JsDeleter(JSContext* cx) : m_cx(cx) {}
    void operator()(void *x) { JS_free(m_cx, x); }
  private:
    JSContext *m_cx;
  };
  typedef std::unique_ptr<char, JsDeleter> JsCharPtr;

  static std::string jsEncodeString(JSContext *cx, JSString *jsString) {
    JsCharPtr s(JS_EncodeString(cx, jsString), JsDeleter(cx));
    return s.get();
  }

  //============================================= ScriptEnvironment

  ScriptEnvironment::ScriptEnvironment(Security* _pSecurity)
  : m_pRuntime(NULL),
    m_pSecurity(_pSecurity),
    m_RuntimeSize(0),
    m_StackSize(0),
    m_cxOptionSet(0),
    m_cxOptionClear(0),
    m_TimingEnabled(false),
    m_pContext(NULL)
  {
  } // ctor

  ScriptEnvironment::~ScriptEnvironment() {
    if (m_pRuntime != NULL) {
      JS_DestroyRuntime(m_pRuntime);
      m_pRuntime = NULL;
    }
    JS_ShutDown();
  } // dtor

  void ScriptEnvironment::initialize() {
    m_RuntimeSize = 8L * 1024L * 1024L;
    m_StackSize = 8192;
    m_cxOptionClear = m_cxOptionSet = 0;
    m_TimingEnabled = false;

    try {
      if (DSS::hasInstance()) {
        PropertyNodePtr pPtr = DSS::getInstance()->getPropertySystem().getProperty("/config/spidermonkey/runtimesize");
        if (pPtr && (pPtr->getValueType() == vTypeInteger)) {
          m_RuntimeSize = pPtr->getIntegerValue();
        }
        pPtr = DSS::getInstance()->getPropertySystem().getProperty("/config/spidermonkey/stacksize");
        if (pPtr && (pPtr->getValueType() == vTypeInteger)) {
          m_StackSize = pPtr->getIntegerValue();
        }
        pPtr = DSS::getInstance()->getPropertySystem().getProperty("/config/spidermonkey/optionset");
        if (pPtr && (pPtr->getValueType() == vTypeInteger)) {
          m_cxOptionSet = pPtr->getIntegerValue();
        }
        pPtr = DSS::getInstance()->getPropertySystem().getProperty("/config/spidermonkey/optionclear");
        if (pPtr && (pPtr->getValueType() == vTypeInteger)) {
          m_cxOptionClear = pPtr->getIntegerValue();
        }
        pPtr = DSS::getInstance()->getPropertySystem().getProperty("/config/spidermonkey/timing");
        if (pPtr && (pPtr->getValueType() == vTypeBoolean)) {
          m_TimingEnabled = pPtr->getBoolValue();
        }

        m_pPropertyNode = DSS::getInstance()->getPropertySystem().createProperty("/system/js/");
        m_pPropertyNode->createProperty("timings");
        m_pPropertyNode->createProperty("features");
        m_pPropertyNode->createProperty("features/timing")
            ->linkToProxy(PropertyProxyReference<bool>(m_TimingEnabled));
      }
    } catch (PropertyTypeMismatch&) {
    } catch (SecurityException&) {
    }

    m_pRuntime = JS_NewRuntime(m_RuntimeSize);
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
    JSContext* context = JS_NewContext(m_pRuntime, m_StackSize);
    JSContextThread ct(context);

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
    throw ScriptException("Value is not a number");
  }

  template<>
  std::string ScriptContext::convertTo(const jsval& _val) {
    JSString* result;
    result = JS_ValueToString(m_pContext, _val);
    if( result == NULL) {
      throw ScriptException("Could not convert value to JSString");
    }
    return jsEncodeString(m_pContext, result);
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

  template<>
  uint8_t ScriptContext::convertTo(const jsval& _val) {
    if(JSVAL_IS_NUMBER(_val)) {
      uint16 result;
      if(JS_ValueToUint16(m_pContext, _val, &result)) {
        if (result > UCHAR_MAX) {
            throw ScriptException("Value out of range");
        }
        return result;
      }
    }
    throw ScriptException("Value is not of type number");
  }

  template<>
  uint16_t ScriptContext::convertTo(const jsval& _val) {
    if(JSVAL_IS_NUMBER(_val)) {
      uint16 result;
      if(JS_ValueToUint16(m_pContext, _val, &result)) {
        return result;
      }
    }
    throw ScriptException("Value is not of type number");
  }

  template<>
  uint32_t ScriptContext::convertTo(const jsval& _val) {
    if (JSVAL_IS_NUMBER(_val)) {
      uint32_t result;
      if (JS_ValueToECMAUint32(m_pContext, _val, &result)) {
        return result;
      }
    }
    throw ScriptException("Value is not of type number");
  }

  namespace {
    class Stringifier
    {
      std::stringstream m_Stream;
      ScriptContext& m_Context;

    public:

      Stringifier(ScriptContext& context) : m_Context(context) { }

      static int callback(const jschar *buf, uint32 len, void *data)
      {
        auto cx = static_cast<Stringifier*>(data)->m_Context.getJSContext();

        /* Determine how many bytes to allocate. */
        size_t dstlen = 0;
        if (!JS_EncodeCharacters(cx, buf, len, NULL, &dstlen))
            return JS_FALSE;

        /* Create tmp buffer. */
        char dst[dstlen + 1];
        memset(dst, 0, sizeof(dst));

        /* Convert js characters to C chars. */
        JS_EncodeCharacters(cx, buf, len, dst, &dstlen);

        /* Copy the converted characters to output stream. */
        static_cast<Stringifier*>(data)->m_Stream << dst;

        return JS_TRUE;
      }

      std::string getOutput() { return m_Stream.str(); }
    };
  }

  std::string ScriptContext::jsonStringify(jsval& val) {
    Stringifier stringifier(*this);

    if (!JS_Stringify(getJSContext(), &val, nullptr, 0, & Stringifier::callback, &stringifier))
    {
      return std::string();
    }

    return stringifier.getOutput();
  }

  void ScriptContext::jsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er) {
    char *pointer=NULL;
    char *line=NULL;
    int len;

    if (er->linebuf != NULL){
      len = er->tokenptr - er->linebuf + 2;
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
    aLogSeverity logSeverity = lsError;
    logSStream << "JavaScript ";
    if (JSREPORT_IS_WARNING(er->flags)) {
      logSStream << "Warning";
      logSeverity = lsWarning;
    }
    else if (JSREPORT_IS_EXCEPTION(er->flags)) {
      logSStream << "Exception";
    }
    else if (JSREPORT_IS_STRICT(er->flags)) {
      logSStream << "Strict-Mode";
      logSeverity = lsWarning;
    }
    else if (JSREPORT_IS_STRICT_MODE_ERROR(er->flags)) {
      logSStream << "Strict-Error";
    }
    else {
      logSStream << "Error";
    }
    logSStream << "[" << er->errorNumber << "]: ";

    logSStream << "\"" << msg << "\" in file: " << er->filename << ":" << er->lineno;
    if (line[0]){
      logSStream << "\n  script: " << line << "        : " << pointer;
    }

    Logger::getInstance()->log(logSStream.str(), logSeverity);

    free(pointer);
    free(line);
  } // jsErrorHandler


  static JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS, /* use the new resolve hook */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
  };


  JSBool global_print(JSContext *cx, uintN argc, jsval *vp) {
    uint i;
    JSString * unicode_str;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "*"))
      return JS_FALSE;

    std::stringstream sstream;
    sstream << "JS Print: ";

    for (i = 0; i < argc; i++)
    {
      unicode_str = JS_ValueToString(cx, (JS_ARGV(cx, vp))[i]);
      if (unicode_str == NULL)
        return JS_FALSE;

      sstream << jsEncodeString(cx, unicode_str);
    }
    Logger::getInstance()->log(sstream.str(), lsWarning);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
  } // global_print

  JSBool global_keepContext(JSContext *cx, uintN argc, jsval *vp) {
    Logger::getInstance()->log("keepContext() is deprecated", lsWarning);
    return JS_TRUE;
  } // global_keepContext

  class SessionAttachedTimeoutObject : public ScriptContextAttachedObject {
  public:
    SessionAttachedTimeoutObject(ScriptContext* _pContext)
    : ScriptContextAttachedObject(_pContext),
      m_pRunAsUser(NULL)
    {
      if(Security::getCurrentlyLoggedInUser() != NULL) {
        m_pRunAsUser = new User(*Security::getCurrentlyLoggedInUser());
      }
    }

    virtual ~SessionAttachedTimeoutObject() {
      if (m_pRunAsUser) {
        delete m_pRunAsUser;
      }
    }

    void timeout(int _timeoutMS, JSObject* _obj, jsval _function, ScriptFunctionRooter* _rooter) {
      const int kSleepIntervalMS = 500;
      int toSleep = _timeoutMS;
      while(!getIsStopped() && (toSleep > 0)) {
        int toSleepNow = std::min(toSleep, kSleepIntervalMS);
        sleepMS(toSleepNow);
        toSleep -= toSleepNow;
      }

      if(!getIsStopped()) {
        if(m_pRunAsUser != NULL) {
          Security* pSecurity = getContext()->getEnvironment().getSecurity();
          if(pSecurity != NULL) {
            pSecurity->signIn(m_pRunAsUser);
          }
        }

        ScriptLock lock(getContext());
        {
          JSContextThread req(getContext());
          ScriptObject sobj(_obj, *getContext());
          ScriptFunctionParameterList params(*getContext());
          try {
            sobj.callFunctionByReference<void>(_function, params);
          } catch(ScriptException& e) {
            Logger::getInstance()->log("JavaScript: error calling timeout handler: '" +
                std::string(e.what()) + "'", lsError);
          }
          delete _rooter;
        }

      } else {
        ScriptLock lock(getContext());
        JSContextThread req(getContext());
        delete _rooter;
      }

      delete this;
    }
  private:
    User* m_pRunAsUser;
  };

  JSBool global_setTimeout(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    int timeoutMS;
    jsval functionVal;
    JSObject* jsFunction;
    jsdouble jsTimeout;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "oI", &jsFunction, &jsTimeout)) {
      JS_ReportError(cx, "setTimeout(): invalid parameter");
      return JS_FALSE;
    }

    if (!JS_ObjectIsCallable(cx, jsFunction)) {
      JS_ReportError(cx, "setTimeout(): timeout handler is not a function");
      return JS_FALSE;
    }

    functionVal = OBJECT_TO_JSVAL(jsFunction);
    timeoutMS = (int) jsTimeout;

    JSObject* jsRoot = JS_NewObject(cx, NULL, NULL, NULL);

    ScriptFunctionRooter* functionRoot(new ScriptFunctionRooter(ctx, jsRoot, functionVal));
    SessionAttachedTimeoutObject* pTimeoutObj = new SessionAttachedTimeoutObject(ctx);
    boost::thread(boost::bind(&SessionAttachedTimeoutObject::timeout, pTimeoutObj, timeoutMS, jsRoot, functionVal, functionRoot));
    JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    return JS_TRUE;
  } // global_setTimeout

  JSBool global_registerTrigger(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    if(argc != 3) {
      JS_ReportError(cx, "registerTrigger needs 3 parameters");
      return JS_FALSE;
    }
    std::string path = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
    std::string eventName = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
    const jsval& paramValue = JS_ARGV(cx, vp)[2];
    JSObject* paramObj = 0;
    if (!JSVAL_IS_NULL(paramValue)) {
      if (JS_ValueToObject(cx, paramValue, &paramObj) != JS_TRUE) {
        throw ScriptException("paramValue is not of type object");
      }
    }

    Logger::getInstance()->log("global_registerTrigger path:" + path + "eventName:" + eventName, lsDebug);

    PropertySystem& propertySystem = DSS::getInstance()->getPropertySystem();
    PropertyNodePtr matchedNode;
    PropertyNodePtr baseNode = propertySystem.createProperty("/usr/triggers");
    int maxId = 0;
    const auto& baseChildNodes = baseNode->getChildNodes();
    for (auto it = baseChildNodes.begin(); it != baseChildNodes.end(); it++) {
      const auto& baseChildNode = *it;
      maxId = std::max(maxId, strToIntDef(baseChildNode->getName(), 0));
      const auto& triggerPathNode = baseChildNode->getPropertyByName("triggerPath");
      if (!triggerPathNode) {
          continue;
      }
      if (triggerPathNode->getAsString() == path) {
          matchedNode = baseChildNode;
          break;
      }
    }
    if (!matchedNode) {
      int id = maxId + 1;
      std::string matchedNodePath = "/usr/triggers/" + intToString(id);
      matchedNode = propertySystem.createProperty(matchedNodePath);
      matchedNode->createProperty("id")->setIntegerValue(id);
    }
    matchedNode->createProperty("triggerPath")->setStringValue(path);
    matchedNode->createProperty("relayedEventName")->setStringValue(eventName);

    if (paramObj) {
      JSObject* propIter = JS_NewPropertyIterator(cx, paramObj);
      jsid propId;
      std::string paramString;
      while(JS_NextProperty(cx, propIter, &propId) == JS_TRUE) {
        if(JSID_IS_VOID(propId)) {
          break;
        }
        if (!paramString.empty()) {
          paramString += ';';
        }
        {
          jsval val;
          JS_IdToValue(cx, propId, &val);
          paramString += ctx->convertTo<std::string>(val);
        }
        paramString += '=';
        {
          JSObject* obj;
          jsval val;
          JS_GetMethodById(cx, paramObj, propId, &obj, &val);
          paramString += ctx->convertTo<std::string>(val);
        }
      }
      matchedNode->createProperty("additionalRelayingParameter")->setStringValue(paramString);
    }
    return JS_TRUE;
  } // registerTrigger

  JSBool global_unregisterTrigger(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    if(argc != 1) {
      JS_ReportError(cx, "unregisterTrigger needs 1 parameter");
      return JS_FALSE;
    }
    std::string path = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
    Logger::getInstance()->log("global_unregisterTrigger path:" + path, lsDebug);

    PropertySystem& propertySystem = DSS::getInstance()->getPropertySystem();
    PropertyNodePtr baseNode = propertySystem.getProperty("/usr/triggers");
    if (!baseNode) {
        return JS_TRUE;
    }
    PropertyNodePtr matchedNode;
    const auto& baseChildNodes = baseNode->getChildNodes();
    for (auto it = baseChildNodes.begin(); it != baseChildNodes.end(); it++) {
      const auto& baseChildNode = *it;
      const auto& triggerPathNode = baseChildNode->getPropertyByName("triggerPath");
      if (!triggerPathNode) {
        continue;
      }
      if (triggerPathNode->getAsString() == path) {
        matchedNode = baseChildNode;
        break;
      }
    }
    if (matchedNode) {
      baseNode->removeChild(matchedNode);
    }
    return JS_TRUE;
  } // unregisterTrigger

  JSFunctionSpec global_methods[] = {
    JS_FS("print", global_print, 1, 0),
    JS_FS("keepContext", global_keepContext, 0, 0),
    JS_FS("setTimeout", global_setTimeout, 2, 0),
    JS_FS("registerTrigger", global_registerTrigger, 3, 0),
    JS_FS("unregisterTrigger", global_unregisterTrigger, 1, 0),
    JS_FS_END
  };

  ScriptContext::ScriptContext(ScriptEnvironment& _env, JSContext* _pContext)
  : m_Environment(_env),
    m_pContext(_pContext),
    m_Locked(false),
    m_LockedBy(0)
  {
    uint32 cxOptions = _env.getContextFlags((JS_GetOptions(m_pContext) |
        (JSOPTION_VAROBJFIX | JSOPTION_STRICT | JSOPTION_JIT | JSOPTION_METHODJIT)));
    JS_SetOptions(m_pContext, cxOptions);

#ifdef JSVERSION_LATEST
    JS_SetVersion(m_pContext, JSVERSION_LATEST);
#endif

    JS_SetErrorReporter(m_pContext, jsErrorHandler);

    /* Create the global object. */
    m_pRootObject = JS_NewCompartmentAndGlobalObject(m_pContext, &global_class, NULL);
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
    ScriptLock lock(this);
    if (!JS_GetContextThread(m_pContext)) {
      JS_SetContextThread(m_pContext);
    }

    if(!m_AttachedObjects.empty()) {
      Logger::getInstance()->log("Still have some attached objects (" + intToString(m_AttachedObjects.size()) + "). Memory leak?", lsError);
    }
    scrubVector(m_AttachedObjects);

    JS_SetContextPrivate(m_pContext, NULL);
    JS_DestroyContext(m_pContext);
    m_pContext = NULL;
  } // dtor

  void ScriptContext::attachObject(ScriptContextAttachedObject* _pObject) {
    // TODO: check if this lock is really needed
    boost::mutex::scoped_lock lock(m_AttachedObjectsMutex);
    m_AttachedObjects.push_back(_pObject);
  } // attachObject

  void ScriptContext::removeAttachedObject(ScriptContextAttachedObject* _pObject) {
    boost::mutex::scoped_lock lock(m_AttachedObjectsMutex);
    std::vector<ScriptContextAttachedObject*>::iterator it =
       std::find(m_AttachedObjects.begin(), m_AttachedObjects.end(), _pObject);
    if(it != m_AttachedObjects.end()) {
      m_AttachedObjects.erase(it);
    }
  } // removeAttachedObject

  ScriptContextAttachedObject* ScriptContext::getAttachedObjectByName(const std::string& _name) {
    boost::mutex::scoped_lock lock(m_AttachedObjectsMutex);
    typedef std::vector<ScriptContextAttachedObject*>::iterator AttachedObjectIterator;
    for(AttachedObjectIterator iObject = m_AttachedObjects.begin(), e = m_AttachedObjects.end();
        iObject != e; ++iObject) {
      if((*iObject)->getName() == _name) {
        return *iObject;
      }
    }
    return NULL;
  } // getAttachedObjectByName

  void ScriptContext::stop() {
    boost::mutex::scoped_lock lock(m_AttachedObjectsMutex);
    std::vector<ScriptContextAttachedObject*> aObjectList(m_AttachedObjects);
    lock.unlock();
    typedef std::vector<ScriptContextAttachedObject*>::iterator AttachedObjectIterator;
    for(AttachedObjectIterator iObject = aObjectList.begin(), e = aObjectList.end();
        iObject != e; ++iObject) {
      (*iObject)->stop();
    }
  } // stop

  bool ScriptContext::raisePendingExceptions() {
    if(JS_IsExceptionPending(m_pContext)) {
      jsval exval;
      if(JS_GetPendingException(m_pContext, &exval)) {
        JS_ClearPendingException(m_pContext);
        JSString* errstr = JS_ValueToString(m_pContext, exval);
        if(errstr != NULL) {
          std::string errMsg = jsEncodeString(m_pContext, errstr);
          throw ScriptRuntimeException(std::string("Caught Exception while executing script: ") + errMsg, errMsg);
        }
      }
      throw ScriptException("Exception was pending after script execution, but couldnt get it from the vm");
    }
    return false;
  } // raisePendingExceptions

  jsval ScriptContext::doEvaluateScript(const std::string& _fileName) {
    ScriptLock lock(this);
    JSBool ok = JS_FALSE;
    jsval rval;
    JSContextThread req(m_pContext);
    // multiple hanging scriptObj's may exist per context which
    // are all freed with the context destruction
    JSObject* scriptObj = JS_CompileFile(m_pContext, m_pRootObject, _fileName.c_str());
    if (!scriptObj) {
      throw ScriptException("Error compiling script: " + _fileName);
    }
    ok = JS_ExecuteScript(m_pContext, m_pRootObject, scriptObj, &rval);
    if(ok) {
      return rval;
    } else {
      raisePendingExceptions();
      // NOTE:do not throw an exception here
    }
    JS_SET_RVAL(m_pContext, &rval, BOOLEAN_TO_JSVAL(true));
    return rval;
  } // doEvaluateScript

  template <>
  void ScriptContext::evaluateScript(const std::string& _script) {
    doEvaluateScript(_script);
  } // evaluateScript<void>

  template <>
  int ScriptContext::evaluateScript(const std::string& _script) {
    jsval jval = doEvaluateScript(_script);
    JSContextThread thread(this);
    return convertTo<int>(jval);
  } // evaluateScript<int>

  template <>
  double ScriptContext::evaluateScript(const std::string& _script) {
    jsval jval = doEvaluateScript(_script);
    JSContextThread thread(this);
    return convertTo<double>(jval);
  } // evaluateScript<double>

  template <>
  std::string ScriptContext::evaluateScript(const std::string& _script) {
    jsval jval = doEvaluateScript(_script);
    JSContextThread thread(this);
    return convertTo<std::string>(jval);
  } // evaluateScript<std::string>

  template <>
  bool ScriptContext::evaluateScript(const std::string& _script) {
    jsval jval = doEvaluateScript(_script);
    JSContextThread thread(this);
    return convertTo<bool>(jval);
  } // evaluateScript<bool>

  jsval ScriptContext::doEvaluate(const std::string& _script) {
    ScriptLock lock(this);
    JSContextThread thread(this);

    const char* filename = "temporary_script";
    jsval rval;
    JSBool ok = JS_EvaluateScript(m_pContext, m_pRootObject, _script.c_str(), _script.size(),
                       filename, 0, &rval);
    if(ok) {
      return rval;
    } else {
      raisePendingExceptions();
      // NOTE: do not throw an exception here
    }
    JS_SET_RVAL(m_pContext, &rval, BOOLEAN_TO_JSVAL(true));
    return rval;
  } // doEvaluate

  template <>
  void ScriptContext::evaluate(const std::string& _script) {
    doEvaluate(_script);
  } // evaluate<void>

  template <>
  int ScriptContext::evaluate(const std::string& _script) {
    jsval jval = doEvaluate(_script);
    JSContextThread thread(this);
    return convertTo<int>(jval);
  } // evaluate<int>

  template <>
  double ScriptContext::evaluate(const std::string& _script) {
    jsval jval = doEvaluate(_script);
    JSContextThread thread(this);
    return convertTo<double>(jval);
  } // evaluate<double>

  template <>
  std::string ScriptContext::evaluate(const std::string& _script) {
    jsval jval = doEvaluate(_script);
    JSContextThread thread(this);
    return convertTo<std::string>(jval);
  } // evaluate<std::string>

  template <>
  bool ScriptContext::evaluate(const std::string& _script) {
    jsval jval = doEvaluate(_script);
    JSContextThread thread(this);
    return convertTo<bool>(jval);
  } // evaluate<bool>

  void ScriptContext::lock() {
    m_ContextMutex.lock();
    m_LockDataMutex.lock();
    assert(!m_Locked);
    assert(m_LockedBy == 0);
    m_LockedBy = pthread_self();
    m_Locked = true;
    m_LockDataMutex.unlock();
  } // lock

  void ScriptContext::unlock() {
    m_LockDataMutex.lock();
    assert(m_Locked);
    m_LockedBy = 0;
    m_Locked = false;
    m_ContextMutex.unlock();
    m_LockDataMutex.unlock();
  } // unlock

  bool ScriptContext::reentrantLock() {
    bool result = true;
    m_LockDataMutex.lock();
    if(m_Locked && (m_LockedBy == pthread_self())) {
      result = false;
    }
    m_LockDataMutex.unlock();
    if(result == true) {
      lock();
    }
    return result;
  }

  //================================================== ScriptExtension


  //================================================== ScriptFunctionParameterList

  template<>
  void ScriptFunctionParameterList::add(int _value) {
    m_Parameter.push_back(INT_TO_JSVAL(_value));
  } // add<int>

  template<>
  void ScriptFunctionParameterList::add(uint8_t _value) {
    m_Parameter.push_back(INT_TO_JSVAL(_value));
  } // add<uint8_t>

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
    JSString* str = JS_NewStringCopyN(m_Context.getJSContext(), _value.c_str(), _value.size());
    m_Parameter.push_back(STRING_TO_JSVAL(str));
  } // add<const std::string&>

  template<>
  void ScriptFunctionParameterList::add(std::string _value) {
    JSString* str = JS_NewStringCopyN(m_Context.getJSContext(), _value.c_str(), _value.size());
    m_Parameter.push_back(STRING_TO_JSVAL(str));
  } // add<std::string&>

  void ScriptFunctionParameterList::addJSVal(jsval _value) {
    m_Parameter.push_back(_value);
  } // addJSVal


  //=============================================== ScriptFunctionRooter

  ScriptFunctionRooter::ScriptFunctionRooter()
  : m_Function(JSVAL_NULL),
    m_pObject(NULL),
    m_pContext(NULL)
  {} // ctor

  ScriptFunctionRooter::ScriptFunctionRooter(ScriptContext* _pContext, JSObject* _object, jsval _function) {
    rootFunction(_pContext, _object, _function);
  } // ctor

  ScriptFunctionRooter::~ScriptFunctionRooter() {
    if((m_Function != JSVAL_NULL) || (m_pObject != NULL)) {
      assert(m_pContext != NULL);
    }

    JSContext* jsc = m_pContext->getJSContext();
    bool jsThreadSet = false;
    if (!JS_GetContextThread(jsc)) {
      JS_SetContextThread(jsc);
      jsThreadSet = true;
    }
    JS_BeginRequest(jsc);

    if(m_Function != JSVAL_NULL) {
      JS_RemoveValueRoot(jsc, &m_Function);
    }
    if(m_pObject != NULL) {
      JS_RemoveObjectRoot(jsc, &m_pObject);
    }

    JS_EndRequest(jsc);
    if (jsThreadSet) {
      JS_ClearContextThread(jsc);
    }
  } // dtor

  void ScriptFunctionRooter::rootFunction(ScriptContext* _pContext, JSObject* _object, jsval _function) {
    m_pContext = _pContext;
    m_pObject = _object;
    m_Function = _function;

    if(m_Function != JSVAL_NULL) {
      JS_AddValueRoot(m_pContext->getJSContext(), &m_Function);
    }
    if(m_pObject != NULL) {
      JS_AddObjectRoot(m_pContext->getJSContext(), &m_pObject);
    }
  }

} // namespace dss
