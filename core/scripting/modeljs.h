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
  };
}

#endif