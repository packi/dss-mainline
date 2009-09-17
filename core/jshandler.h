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

#ifndef JSHANDLER_H_INCLUDED
#define JSHANDLER_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base.h"

#ifdef HAVE_MOZJS_JSAPI_H
#include <mozjs/jsapi.h>
#else
#include <js/jsapi.h>
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>

namespace dss {

  class ScriptContext;
  class ScriptExtension;
  class ScriptObject;

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
  class ScriptContext {
  private:
    JSScript* m_pScriptToExecute;
    std::string m_FileName;
    JSObject* m_pRootObject;
    boost::scoped_ptr<ScriptObject> m_RootObject;
    JSObject* m_pSourceObject;
    ScriptEnvironment& m_Environment;
    JSContext* m_pContext;
    bool m_KeepContext;
    static void jsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er);
  public:
    ScriptContext(ScriptEnvironment& _env, JSContext* _pContext);
    virtual ~ScriptContext();

    /** Loads the script from File
      * @throw runtime_error if _fileName does not exist
      * @throw ScriptException if there is something wrong with the script
      */
    void loadFromFile(const std::string& _fileName);
    /** Loads the script from Memory.
      * @throw ScriptException if there is something wrong with the script
      */
    void loadFromMemory(const char* _script);

    /** Evaluates the previously loaded script. The result of the script
      * will be casted to typeof t.
      * @throw ScriptRuntimeException if the script raised an exception
      * @throw SciptException
      */
    template <class t>
    t evaluate();

    /** Evaluates the given script */  
    template <class t>
    t evaluateScript(const std::string& _script);

    /** Returns a pointer to the JSContext */
    JSContext* getJSContext() { return m_pContext; }
    /** Returns a const reference to the ScriptEnvironment */
    const ScriptEnvironment& getEnvironment() const { return m_Environment; }
    ScriptEnvironment& getEnvironment() { return m_Environment; }
    ScriptObject& getRootObject() { return *m_RootObject; }
    bool raisePendingExceptions();
    bool getKeepContext() { return m_KeepContext; };
    void setKeepContext(bool _value) { m_KeepContext = _value; }
  public:

    /** Helper function to convert a jsval to a t. */
    template<class t>
    t convertTo(const jsval& _val);
  };

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

    int size() { return m_Parameter.size(); }
    jsval get(const int _index) { return m_Parameter.at(_index); }
  private:
    ScriptContext& m_Context;
    std::vector<jsval> m_Parameter;
  }; // ScriptFunctionParameterList

  /** A ScriptObject is a wrapper for a JavaScript object. */
  class ScriptObject {
  private:
    JSObject* m_pObject;
    ScriptContext& m_Context;
  public:
    ScriptObject(JSObject* _pObject, ScriptContext& _context);
    ScriptObject(ScriptContext& _context, ScriptObject* _pParent);

    /** Returns the objects "classname" property. This property must be
      * present for this call to succeed.
      */
    const std::string getClassName();
    /** Compares the objects classname to _className.
      * @see getClassName
      */
    bool is(const std::string& _className);

    /** Returns the property named \a _name as type \a t */
    template<class t>
    t getProperty(const std::string& _name);

    /** Sets the property named \a _name to \a _value */
    template<class t>
    void setProperty(const std::string& _name, t _value);

    template<class t>
    t callFunctionByName(const std::string& _functionName, ScriptFunctionParameterList& _parameter);

    template<class t>
    t callFunctionByReference(jsval _function, ScriptFunctionParameterList& _parameter);
}; // ScriptObject

} // namespace dss

#endif
