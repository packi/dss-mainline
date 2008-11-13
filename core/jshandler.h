/*
 *  jshandler.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef JSHANDLER_H_INCLUDED
#define JSHANDLER_H_INCLUDED

#include "base.h"
#include <js/jsapi.h>

#include <vector>

using namespace std;

namespace dss {

  class ScriptContext;
  class ScriptExtension;

  /** Wrapper for a script runtime environment. The ScriptEnvironment
    * is also responsible for creating Contexts and enhancing them with
    * certain ScriptExtensions. */
  class ScriptEnvironment {
  private:
    JSRuntime* m_pRuntime;
    vector<ScriptExtension*> m_Extensions;
  public:
    ScriptEnvironment();
    virtual ~ScriptEnvironment();

    void Initialize();

    /** Adds a ScriptExtension to the extension list. The ScriptEnvironment
      * is now responsible to free the object. */
    void AddExtension(ScriptExtension* _pExtension);
    /** Returns a pointer to the extension named _name.
      * @return Pointer to the instance or NULL if not found */
    ScriptExtension* GetExtension(const string& _name) const;

    /** Creates a new ScriptContext with all registered extensions present */
    ScriptContext* GetContext();

    bool IsInitialized();
  };

  /** ScriptContext is a wrapper for a scripts execution context.
    * A script can either be loaded from a file or from
    * a string contained in memory. */
  class ScriptContext {
  private:
    JSScript* m_pScriptToExecute;
    string m_FileName;
    JSObject* m_pRootObject;
    JSObject* m_pSourceObject;
    ScriptEnvironment& m_Environment;
    JSContext* m_pContext;
    static void JsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er);
  public:
    ScriptContext(ScriptEnvironment& _env, JSContext* _pContext);
    virtual ~ScriptContext();

    /** Loads the script from File
      * @throw runtime_error if _fileName does not exist
      * @throw ScriptException if there is something wrong with the script
      */
    void LoadFromFile(const string& _fileName);
    /** Loads the script from Memory.
      * @throw ScriptException if there is something wrong with the script
      */
    void LoadFromMemory(const char* _script);

    /** Evaluates the previously loaded script. The result of the script
      * will be casted to typeof t.
      * @throw ScriptRuntimeException if the script raised an exception
      * @throw SciptException
      */
    template <class t>
    t Evaluate();

    /** Returns a pointer to the JSContext */
    JSContext* GetJSContext() { return m_pContext; }
    /** Returns a const reference to the ScriptEnvironment */
    const ScriptEnvironment& GetEnvironment() { return m_Environment; }
  public:

    /** Helper function to convert a jsval to a t. */
    template<class t>
    t ConvertTo(const jsval& _val);
  };

  /** Exception class that will be raised if anything out of the
    * ordinary should happen. */
  class ScriptException : public DSSException {
  public:
    ScriptException(const string& _what)
    : DSSException(_what)
    {
    }

    virtual ~ScriptException() throw() {}
  };

  /** Any runtime exception of a script will result
    * in a ScriptRuntimeException being raised. */
  class ScriptRuntimeException : public ScriptException {
  private:
    const string m_ExceptionMessage;
  public:
    ScriptRuntimeException(const string& _what, const string& _exceptionMessage) throw()
    : ScriptException(_what),
      m_ExceptionMessage(_exceptionMessage)
    { }

    virtual ~ScriptRuntimeException() throw() {}

    /** Holds the original script exception message */
    const string& GetExceptionMessage() const { return m_ExceptionMessage; }
  };

  /** A ScriptExtension extends a scripts context. This is
    * done by the ScriptRuntime everytime a context is being
    * created.
    */
  class ScriptExtension {
  private:
    const string m_Name;
  public:
    ScriptExtension(const string& _name) : m_Name(_name) {}
    virtual ~ScriptExtension() {}

    /** Extend a ScriptContext with the provided extension */
    virtual void ExtendContext(ScriptContext& _context) = 0;

    /** Returns the name of the extension */
    const string& GetName() const { return m_Name; }
  }; // ScriptExtension


  /** A ScriptObject is a wrapper for a JavaScript object. */
  class ScriptObject {
  private:
    JSObject* m_pObject;
    ScriptContext& m_Context;
  public:
    ScriptObject(JSObject* _pObject, ScriptContext& _context);

    /** Returns the objects "classname" property. This property must be
      * present for this call to succeed.
      */
    const string GetClassName();
    /** Compares the objects classname to _className.
      * @see GetClassName
      */
    bool Is(const string& _className);

    /** Returns the property named _name as type t */
    template<class t>
    t GetProperty(const string& _name);
  }; // ScriptObject
}

#endif
