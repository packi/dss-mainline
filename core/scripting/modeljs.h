/*
 *  modeljs.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/22/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _MODEL_JS_INCLUDED
#define _MODEL_JS_INCLUDED

#include <bitset>

#include "../jshandler.h"
#include "../model.h"

namespace dss {
  
  class ModelScriptContextExtension : public ScriptExtension {
  private:
    Apartment& m_Apartment;
  public:
    ModelScriptContextExtension(Apartment& _apartment);
    
    virtual void ExtendContext(ScriptContext& _context);
    
    Apartment& GetApartment() const { return m_Apartment; };
    
    JSObject* CreateJSSet(ScriptContext& _ctx, Set& _set);
    JSObject* CreateJSDevice(ScriptContext& _ctx, Device& _ref);
    JSObject* CreateJSDevice(ScriptContext& _ctx, DeviceReference& _ref);
    
    template<class t>
    t ConvertTo(ScriptContext& _context, jsval val);
    
    template<class t>
    t ConvertTo(ScriptContext& _context, JSObject* _obj);
  };
  
  class ActionJS : public Action {
  private:
    ScriptEnvironment m_Environment;
  public:
   
    virtual void Perform(const Arguments& _args);
  };
}

#endif