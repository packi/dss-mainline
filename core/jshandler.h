/*
 *  jshandler.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "base.h"
#include <js/jsapi.h>

namespace dss {
  
  class ScriptContext;
  
  class ScriptEnvironment {
  private:
    JSRuntime* m_pRuntime;
  public:
    void Initialize();
    
    ScriptContext* GetContext();
  };
  
  class ScriptContext {
  private:
    JSScript* m_pScriptToExecute;
    string m_FileName;
    JSContext* m_pContext;
    JSObject* m_pRootObject;
    JSObject* m_pSourceObject;
    static void JsErrorHandler(JSContext *ctx, const char *msg, JSErrorReport *er);
  public:
    ScriptContext(JSContext* _pContext);
    ~ScriptContext();
    
    void LoadFromFile(const string& _fileName);
    void LoadFromMemory(const char* _script);
    
    template <class t>
    t Evaluate();
  };
  
  class ScriptException : public DSSException {
  public:
    ScriptException(const string& _what) 
    : DSSException(_what)
    {
    }
  };
}