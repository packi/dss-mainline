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

#ifndef JSHANDLER_H_INCLUDED
#define JSHANDLER_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <cassert>

#if defined(HAVE_JSAPI_H)
#include <jsapi.h>
#elif defined(HAVE_MOZJS_JSAPI_H)
#include <mozjs/jsapi.h>
#elif defined(HAVE_JS_JSAPI_H)
#include <js/jsapi.h>
#else
#error Could not find spidermonkey
#endif

#ifndef JS_THREADSAFE
#error Need threadsafe build of spidermonkey
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>

#include "base.h"
#include "mutex.h"
#include "logger.h"

namespace dss {

  class ScriptContext;
  class ScriptExtension;
  class ScriptObject;
  class ScriptContextAttachedObject;

  /** Wrapper for a script runtime environment. The ScriptEnvironment
    * is also responsible for creating Contexts and enhancing them with
    * certain ScriptExtensions. */
  class ScriptEnvironment {
  private:
    JSRuntime* m_pRuntime;
    boost::ptr_vector<ScriptExtension> m_Extensions;
  public:
    ScriptEnvironment();
    virtual ~ScriptEnvironment();

    void initialize();

    /** Adds a ScriptExtension to the extension list. The ScriptEnvironment
      * is now responsible to free the object. */
    void addExtension(ScriptExtension* _pExtension);
    /** Returns a pointer to the extension named _name.
      * @return Pointer to the instance or NULL if not found */
    const ScriptExtension* getExtension(const std::string& _name) const;
    ScriptExtension* getExtension(const std::string& _name);

    /** Creates a new ScriptContext with all registered extensions present */
    ScriptContext* getContext();

    bool isInitialized();
  };

  /** ScriptContext is a wrapper for a scripts execution context.
    * A script can either be loaded from a file or from
    * a std::string contained in memory. */
  class ScriptContext : public LockableObject {
  private:
    std::string m_FileName;
    JSObject* m_pRootObject;
    boost::scoped_ptr<ScriptObject> m_RootObject;
    ScriptEnvironment& m_Environment;
    JSContext* m_pContext;
    bool m_KeepContext;
    std::vector<ScriptContextAttachedObject*> m_AttachedObjects;
    static void jsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er);
  public:
    ScriptContext(ScriptEnvironment& _env, JSContext* _pContext);
    virtual ~ScriptContext();

    /** Evaluates the given script */
    template <class t>
    t evaluate(const std::string& _script);
    // FIXME: Workaround a compiler issue that interprets typeof jsval == typeof int
    jsval doEvaluate(const std::string& _script);

    /** Evaluates the given file */
    template <class t>
    t evaluateScript(const std::string& _fileName);
    // FIXME: Workaround a compiler issue that interprets typeof jsval == typeof int
    jsval doEvaluateScript(const std::string& _fileName);

    /** Returns a pointer to the JSContext */
    JSContext* getJSContext() { return m_pContext; }
    /** Returns a const reference to the ScriptEnvironment */
    const ScriptEnvironment& getEnvironment() const { return m_Environment; }
    ScriptEnvironment& getEnvironment() { return m_Environment; }
    ScriptObject& getRootObject() { return *m_RootObject; }
    bool raisePendingExceptions();
    bool getKeepContext() { return m_KeepContext; };
    void setKeepContext(bool _value) { m_KeepContext = _value; }

    void attachObject(ScriptContextAttachedObject* _pObject);
    void removeAttachedObject(ScriptContextAttachedObject* _pObject);
    bool hasAttachedObjects() const { return !m_AttachedObjects.empty(); }
  public:

    /** Helper function to convert a jsval to a t. */
    template<class t>
    t convertTo(const jsval& _val);
  }; // ScriptContext

  /** Exception class that will be raised if anything out of the
    * ordinary should happen. */
  class ScriptException : public DSSException {
  public:
    ScriptException(const std::string& _what)
    : DSSException(_what)
    {
    }

    virtual ~ScriptException() throw() {}
  };

  /** Any runtime exception of a script will result
    * in a ScriptRuntimeException being raised. */
  class ScriptRuntimeException : public ScriptException {
  private:
    const std::string m_ExceptionMessage;
  public:
    ScriptRuntimeException(const std::string& _what, const std::string& _exceptionMessage) throw()
    : ScriptException(_what),
      m_ExceptionMessage(_exceptionMessage)
    { }

    virtual ~ScriptRuntimeException() throw() {}

    /** Holds the original script exception message */
    const std::string& getExceptionMessage() const { return m_ExceptionMessage; }
  };

  /** A ScriptExtension extends a scripts context. This is
    * done by the ScriptRuntime everytime a context is being
    * created.
    */
  class ScriptExtension {
  private:
    const std::string m_Name;
  public:
    ScriptExtension(const std::string& _name) : m_Name(_name) {}
    virtual ~ScriptExtension() {}

    /** Extend a ScriptContext with the provided extension */
    virtual void extendContext(ScriptContext& _context) = 0;

    /** Returns the name of the extension */
    const std::string& getName() const { return m_Name; }
  }; // ScriptExtension


  class ScriptFunctionParameterList {
  public:
    ScriptFunctionParameterList(ScriptContext& _context)
    : m_Context(_context)
    {} // ctor

    template<class t>
    void add(t _value);

    // FIXME: work around a compiler issue (typeof jsval == typeof int)
    void addJSVal(jsval _value);

    int size() { return m_Parameter.size(); }
    jsval get(const int _index) { return m_Parameter.at(_index); }
  private:
    ScriptContext& m_Context;
    std::vector<jsval> m_Parameter;
  }; // ScriptFunctionParameterList

  class ScriptContextAttachedObject {
  public:
    ScriptContextAttachedObject(ScriptContext* _pContext)
    : m_pContext(_pContext)
    {
      assert(m_pContext != NULL);
      m_pContext->attachObject(this);
    }
    ~ScriptContextAttachedObject() {
      Logger::getInstance()->log("destroying attached object " + intToString(int(this), true), lsDebug);
      m_pContext->removeAttachedObject(this);
    }

    ScriptContext* getContext() { return m_pContext; }
  private:
    ScriptContext* m_pContext;
  }; // ScriptContextAttachedObject

  /** Adds a JS-function callback to the root GC set and thus
      save it from the GC. */
  class ScriptFunctionRooter {
  public:
    ScriptFunctionRooter();
    ScriptFunctionRooter(ScriptContext* _pContext, JSObject* _pObject, jsval _function);
    ~ScriptFunctionRooter();

    void rootFunction(ScriptContext* _pContext, JSObject* _pObject, jsval _function);
  private:
    JSObject* m_pObject;
    JSObject* m_pFunction;
    ScriptContext* m_pContext;
  }; // ScriptFunctionRooter

  class JSRequest {
  public:
    JSRequest(ScriptContext* _pContext)
    : m_pContext(_pContext->getJSContext()),
      m_NeedsEndRequest(true)
    {
      Logger::getInstance()->log("^^^ Beginning request for " + intToString(int(m_pContext), true));
      assert(_pContext != NULL);
      JS_BeginRequest(m_pContext);
    }

    JSRequest(JSContext* _pContext)
    : m_pContext(_pContext),
      m_NeedsEndRequest(true)
    {
      Logger::getInstance()->log("^^^ Beginning request for " + intToString(int(m_pContext), true));
      assert(_pContext != NULL);
      JS_BeginRequest(_pContext);
    }

    ~JSRequest() {
      if(m_NeedsEndRequest) {
        JS_EndRequest(m_pContext);
        Logger::getInstance()->log("^^^ Ending request for " + intToString(int(m_pContext), true));
      }
    }

    void endRequest() {
      JS_EndRequest(m_pContext);
      Logger::getInstance()->log("^^^ Ending request for " + intToString(int(m_pContext), true));
      m_NeedsEndRequest = false;
    }
  private:
    JSContext* m_pContext;
    bool m_NeedsEndRequest;
  };


/*
 * Initializer macro for a JSFunctionSpec array element. This is the original
 * kind of native function specifier initializer. Use JS_FN ("fast native", see
 * JSFastNative in jspubtd.h) for all functions that do not need a stack frame
 * when activated.
 */
#ifndef JS_FS
#define JS_FS(name,call,nargs,flags,extra)                                    \
    {name, call, nargs, flags, extra}
#endif

/*
 * "Fast native" initializer macro for a JSFunctionSpec array element. Use this
 * in preference to JS_FS if the native in question does not need its own stack
 * frame when activated.
 */
#ifndef JS_FN
#define JS_FN(name,fastcall,nargs,flags)                                      \
    JS_FS(name, (JSNative)(fastcall), nargs,                                  \
          (flags) | JSFUN_FAST_NATIVE | JSFUN_STUB_GSOPS, 0)
#endif

/*
 * Terminating sentinel initializer to put at the end of a JSFunctionSpec array
 * that's passed to JS_DefineFunctions or JS_InitClass.
 */
#ifndef JS_FS_END
#define JS_FS_END JS_FS(NULL,NULL,0,0,0)
#endif


} // namespace dss

#endif
