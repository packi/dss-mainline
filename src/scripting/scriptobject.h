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

#ifndef SCRIPTOBJECT_H_INCLUDED
#define SCRIPTOBJECT_H_INCLUDED

#include "src/scripting/jshandler.h"

namespace dss {

  /** A ScriptObject is a wrapper for a JavaScript object. */
  class ScriptObject {
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
    // FIXME: work around a compiler issue (typeof jsval == typeof int)
    jsval doGetProperty(const std::string& _name);

    /** Sets the property named \a _name to \a _value */
    template<class t>
    void setProperty(const std::string& _name, t _value);

    template<class t>
    t callFunctionByName(const std::string& _functionName, ScriptFunctionParameterList& _parameter);
    // FIXME: work around a compiler issue (typeof jsval == typeof int)
    jsval doCallFunctionByName(const std::string& _functionName, ScriptFunctionParameterList& _parameter);

    template<class t>
    t callFunctionByReference(jsval _function, ScriptFunctionParameterList& _parameter);

    void addRoot();
    void removeRoot();

    JSObject* getJSObject() { return m_pObject; }
    ScriptContext& getContext() { return m_Context; }
  private:
    JSObject* m_pObject;
    ScriptContext& m_Context;
    void doSetProperty(const std::string& _name, jsval _value);
    jsval doCallFunctionByReference(jsval _function, ScriptFunctionParameterList& _parameter);
  }; // ScriptObject

} // namespace dss

#endif // SCRIPTOBJECT_H_INCLUDED
