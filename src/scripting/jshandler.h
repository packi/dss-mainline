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

#ifndef JSHANDLER_H_INCLUDED
#define JSHANDLER_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <cassert>

#include <boost/thread/mutex.hpp>
#include <jsapi.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>

#include "src/base.h"
#include "src/datetools.h"
#include "src/logger.h"
#include "src/propertysystem.h"

namespace dss {

  class ScriptContext;
  class ScriptContextWrapper;
  class ScriptExtension;
  class ScriptObject;
  class ScriptContextAttachedObject;
  class Security;

  /** Wrapper for a script runtime environment. The ScriptEnvironment
    * is also responsible for creating Contexts and enhancing them with
    * certain ScriptExtensions. */
  class ScriptEnvironment {
  private:
    JSRuntime* m_pRuntime;
    boost::ptr_vector<ScriptExtension> m_Extensions;
    Security* m_pSecurity;
    size_t m_RuntimeSize;
    size_t m_StackSize;
    uint32 m_cxOptionSet;
    uint32 m_cxOptionClear;
  public:
    ScriptEnvironment(Security* _pSecurity = NULL);
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
    Security* getSecurity() { return m_pSecurity; }

    /** Get configuration flags for JSContext */
    uint32 getContextFlags(uint32 _originalFlags) {
      return ((_originalFlags & ~m_cxOptionClear) | m_cxOptionSet);
    }

    bool isInitialized();
  };

  /** ScriptContext is a wrapper for a scripts execution context.
    * A script can either be loaded from a file or from
    * a std::string contained in memory. */
  class ScriptContext {
  private:
    std::string m_FileName;
    JSObject* m_pRootObject;
    boost::scoped_ptr<ScriptObject> m_RootObject;
    ScriptEnvironment& m_Environment;
    boost::shared_ptr<ScriptContextWrapper> m_pWrapper;
    JSContext* m_pContext;
    std::vector<ScriptContextAttachedObject*> m_AttachedObjects;
    mutable boost::mutex m_AttachedObjectsMutex;
    static void jsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er);
    mutable boost::mutex m_ContextMutex;
    mutable boost::mutex m_LockDataMutex;
    bool m_Locked;
    pthread_t m_LockedBy;
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
    /** Returns a ptr to an optional wrapper class */
    boost::shared_ptr<ScriptContextWrapper> getWrapper() { return m_pWrapper; }
    void attachWrapper(boost::shared_ptr<ScriptContextWrapper> _wrapper) {
      m_pWrapper = _wrapper;
    }
    void detachWrapper() { m_pWrapper.reset(); }
    ScriptObject& getRootObject() { return *m_RootObject; }
    bool raisePendingExceptions();

    void attachObject(ScriptContextAttachedObject* _pObject);
    void removeAttachedObject(ScriptContextAttachedObject* _pObject);
    bool hasAttachedObjects() const { return !m_AttachedObjects.empty(); }
    int getAttachedObjectsCount() const { return m_AttachedObjects.size(); }
    ScriptContextAttachedObject* getAttachedObjectByName(const std::string& _name);

    void lock();
    void unlock();
    bool reentrantLock();

    void stop();
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

  /** Wrap the script context and add script_id and property node
   * data around */
  class ScriptContextWrapper {
  public:
    ScriptContextWrapper(boost::shared_ptr<ScriptContext> _pContext,
                         PropertyNodePtr _pRootNode,
                         const std::string& _identifier,
                         bool _uniqueNode
                        );
    ~ScriptContextWrapper();
    boost::shared_ptr<ScriptContext> get();
    void addFile(const std::string& _name);
    PropertyNodePtr getPropertyNode();
    const std::string& getIdentifier() const;
  private:
    void stopScript(bool _value);
  private:
    boost::shared_ptr<ScriptContext> m_pContext;
    DateTime m_StartTime;
    std::vector<std::string> m_LoadedFiles;
    PropertyNodePtr m_pPropertyNode;
    PropertyNodePtr m_StopNode;
    PropertyNodePtr m_StartedAtNode;
    PropertyNodePtr m_FilesNode;
    PropertyNodePtr m_AttachedObjectsNode;
    std::string m_Identifier;
    bool m_UniqueNode;
  }; // ScriptContextWrapper

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
    : m_pContext(_pContext),
      m_IsStopped(false)
    {
      assert(m_pContext != NULL);
      m_pContext->attachObject(this);
    }

    ScriptContextAttachedObject(ScriptContext* _pContext, const std::string& _name)
    : m_pContext(_pContext),
      m_Name(_name),
      m_IsStopped(false)
    {
      assert(m_pContext != NULL);
      m_pContext->attachObject(this);
    }

    virtual ~ScriptContextAttachedObject() {
      m_pContext->removeAttachedObject(this);
    }

    virtual void stop() {
      m_IsStopped = true;
    }

    ScriptContext* getContext() { return m_pContext; }
    const std::string& getName() const { return m_Name; }
  protected:
    bool getIsStopped() const { return m_IsStopped; }
  private:
    ScriptContext* m_pContext;
    std::string m_Name;
    bool m_IsStopped;
  }; // ScriptContextAttachedObject

  /** Adds a JS-function callback to the root GC set and thus
      save it from the GC. */
  class ScriptFunctionRooter {
  public:
    ScriptFunctionRooter();
    ScriptFunctionRooter(ScriptContext* _pContext, JSObject* _object, jsval _function);
    ~ScriptFunctionRooter();

    void rootFunction(ScriptContext* _pContext, JSObject* _object, jsval _function);
  private:
    jsval m_Function;
    JSObject* m_pObject;
    ScriptContext* m_pContext;
  }; // ScriptFunctionRooter


  class JSRequest {
  public:
    JSRequest(ScriptContext* _pContext)
    : m_pContext(_pContext->getJSContext()),
      m_NeedsEndRequest(true)
    {
      assert(_pContext != NULL);
      JS_BeginRequest(m_pContext);
    }

    JSRequest(JSContext* _pContext)
    : m_pContext(_pContext),
      m_NeedsEndRequest(true)
    {
      assert(_pContext != NULL);
      JS_BeginRequest(_pContext);
    }

    ~JSRequest() {
      if(m_NeedsEndRequest) {
        JS_EndRequest(m_pContext);
      }
    }

    void endRequest() {
      JS_EndRequest(m_pContext);
      m_NeedsEndRequest = false;
    }
  private:
    JSContext* m_pContext;
    bool m_NeedsEndRequest;
  };

  class JSContextThread {
  public:
    JSContextThread(boost::shared_ptr<ScriptContext> _pContext)
    : m_pContext(_pContext->getJSContext()),
      m_NeedsEndRequest(true)
    {
      assert(m_pContext != NULL);
      enter();
    }

    JSContextThread(ScriptContext* _pContext)
    : m_pContext(_pContext->getJSContext()),
      m_NeedsEndRequest(true)
    {
      assert(m_pContext != NULL);
      enter();
    }

    JSContextThread(JSContext* _pContext)
    : m_pContext(_pContext),
      m_NeedsEndRequest(true)
    {
      assert(m_pContext != NULL);
      enter();
    }

    ~JSContextThread() {
      if(m_NeedsEndRequest) {
        m_pRequest.reset();
        JS_ClearContextThread(m_pContext);
      }
    }

    void endRequest() {
      m_pRequest.reset();
      JS_ClearContextThread(m_pContext);
      m_NeedsEndRequest = false;
    }

  private:
    void enter() {
      JS_SetContextThread(m_pContext);
      m_pRequest.reset(new JSRequest(m_pContext));
    }
  private:
    JSContext* m_pContext;
    bool m_NeedsEndRequest;
    boost::shared_ptr<JSRequest> m_pRequest;
  }; // JSContextThread

  class ScriptLock {
  public:
    ScriptLock(ScriptContext* _pContext, bool _reentrant = false)
    : m_pContext(_pContext)
    {
      enter(_reentrant);
    }

    ScriptLock(boost::shared_ptr<ScriptContext> _pContext, bool _reentrant = false)
    : m_pContext(_pContext.get())
    {
      enter(_reentrant);
    }

    ~ScriptLock() {
      leave();
    }

    bool ownsLock() const { return m_OwnsLock; }
  private:
    void enter(bool _reentrant) {
      if(!_reentrant) {
        m_pContext->lock();
        m_OwnsLock = true;
      } else {
        m_OwnsLock = m_pContext->reentrantLock();
      }
    }

    void leave() {
      if(m_OwnsLock) {
        m_pContext->unlock();
      }
    }
  private:
    ScriptContext* m_pContext;
    bool m_OwnsLock;
  }; // ScriptLock

} // namespace dss

#endif
