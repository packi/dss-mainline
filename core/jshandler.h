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
  
  class ScriptEnvironment {
  private:
    JSRuntime* m_pRuntime;
    vector<ScriptExtension*> m_Extensions;
  public:
    ScriptEnvironment();
    virtual ~ScriptEnvironment();
    
    void Initialize();
    
    void AddExtension(ScriptExtension* _pExtension);
    ScriptExtension* GetExtension(const string& _name) const;
    
    ScriptContext* GetContext();
    
    bool IsInitialized();
  };
  
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
    
    void LoadFromFile(const string& _fileName);
    void LoadFromMemory(const char* _script);
    
    template <class t>
    t Evaluate();
    
    JSContext* GetJSContext() { return m_pContext; };
    const ScriptEnvironment& GetEnvironment() { return m_Environment; };    
  public:
    
    template<class t>
    t ConvertTo(const jsval& _val);
  };
  
  class ScriptException : public DSSException {
  public:
    ScriptException(const string& _what) 
    : DSSException(_what)
    {
    }
    
    virtual ~ScriptException() throw() {}
  };
  
  class ScriptRuntimeException : public ScriptException {
  private:
    const string m_ExceptionMessage;
  public:
    ScriptRuntimeException(const string& _what, const string& _exceptionMessage) throw()
    : ScriptException(_what),
      m_ExceptionMessage(_exceptionMessage)
    { }
    
    virtual ~ScriptRuntimeException() throw() {};
    
    const string& GetExceptionMessage() const { return m_ExceptionMessage; };
  };
  
  class ScriptExtension {
  private:
    const string m_Name;
  public:
    ScriptExtension(const string& _name) : m_Name(_name) {};
    virtual ~ScriptExtension() {};
    
    virtual void ExtendContext(ScriptContext& _context) = 0;
    
    const string& GetName() const { return m_Name; };
  }; // ScriptExtension
  
  class ScriptObject {
  private:
    JSObject* m_pObject;
    ScriptContext& m_Context;
  public:
    ScriptObject(JSObject* _pObject, ScriptContext& _context);
    
    const string GetClassName();
    bool Is(const string& _className);
    
    template<class t>
    t GetProperty(const string& _name);
  }; // ScriptObject
}

#endif
