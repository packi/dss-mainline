/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
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

#include "scriptobject.h"

namespace dss {

  //================================================== ScriptObject

  ScriptObject::ScriptObject(JSObject* _pObject, ScriptContext& _context)
  : m_pObject(_pObject),
    m_Context(_context)
  {
    assert(_pObject != NULL);
  } // ctor

  ScriptObject::ScriptObject(ScriptContext& _context, ScriptObject* _pParent)
  : m_pObject(NULL),
    m_Context(_context)
  {
    JSObject* parentObj = NULL;
    if(_pParent != NULL) {
      parentObj = _pParent->m_pObject;
    }
    m_pObject = JS_NewObject(m_Context.getJSContext(), NULL, NULL, parentObj);
  } // ctor

  jsval ScriptObject::doGetProperty(const std::string& _name) {
    JSBool found;
    if(!JS_HasProperty(m_Context.getJSContext(), m_pObject, _name.c_str(), &found)) {
      throw ScriptException("Could not enumerate property");
    }
    if(found) {
      jsval result;
      if(JS_GetProperty(m_Context.getJSContext(), m_pObject, _name.c_str(), &result)) {
        return result;
      } else {
        throw ScriptException(std::string("Could not retrieve value of property ") + _name);
      }
    } else {
      throw ScriptException(std::string("Could not find property ") + _name);
    }
  } // doGetProperty

  template<>
  std::string ScriptObject::getProperty(const std::string& _name) {
    jsval value = doGetProperty(_name);
    return m_Context.convertTo<std::string>(value);
  } // getProperty<string>

  template<>
  int ScriptObject::getProperty(const std::string& _name) {
    jsval value = doGetProperty(_name);
    return m_Context.convertTo<int>(value);
  } // getProperty<string>

  template<>
  double ScriptObject::getProperty(const std::string& _name) {
    jsval value = doGetProperty(_name);
    return m_Context.convertTo<double>(value);
  } // getProperty<double>

  template<>
  bool ScriptObject::getProperty(const std::string& _name) {
    jsval value = doGetProperty(_name);
    return m_Context.convertTo<bool>(value);
  } // getProperty<bool>

  void ScriptObject::doSetProperty(const std::string& _name, jsval _value) {
    JS_SetProperty(m_Context.getJSContext(), m_pObject, _name.c_str(), &_value);
  } // doSetProperty

  template<>
  void ScriptObject::setProperty(const std::string& _name, const std::string& _value) {
    JSRequest req(&m_Context);
    JSString* str = JS_NewStringCopyN(m_Context.getJSContext(), _value.c_str(), _value.size());
    doSetProperty(_name, STRING_TO_JSVAL(str));
  } // setProperty<const std::string>

  template<>
  void ScriptObject::setProperty(const std::string& _name, const char* _value) {
    std::string str(_value);
    setProperty<const std::string&>(_name, str);
  } // setProperty<const char*>

  template<>
  void ScriptObject::setProperty(const std::string& _name, std::string _value) {
    setProperty<const std::string&>(_name, _value);
  } // setProperty<std::string>

  template<>
  void ScriptObject::setProperty(const std::string& _name, bool _value) {
    JSRequest req(&m_Context);
    doSetProperty(_name, _value ? JSVAL_TRUE : JSVAL_FALSE);
  }

  template<>
  void ScriptObject::setProperty(const std::string& _name, int _value) {
    JSRequest req(&m_Context);
    jsval val;
    if(!JS_NewNumberValue(m_Context.getJSContext(), _value, &val)) {
      throw ScriptException("could not allocate number");
    }
    doSetProperty(_name, val);
  } // setProperty<int>

  template<>
  void ScriptObject::setProperty(const std::string& _name, ScriptObject* _value) {
    assert(_value != NULL);
    JSRequest req(&m_Context);
    doSetProperty(_name, OBJECT_TO_JSVAL(_value->m_pObject));
  } // setProperty<ScriptObject>

  bool ScriptObject::is(const std::string& _className) {
    return getClassName() == _className;
  } // is

  const std::string ScriptObject::getClassName() {
    return getProperty<std::string>("className");
  } // getClassName

  jsval ScriptObject::doCallFunctionByName(const std::string& _functionName,
                                         ScriptFunctionParameterList& _parameter) {
    int paramc = _parameter.size();
    jsval* paramv = (jsval*)malloc(sizeof(jsval) * paramc);
    for(int iParam = 0; iParam < paramc; iParam++) {
      paramv[iParam] = _parameter.get(iParam);
    }
    jsval rval;
    JSBool ok = JS_CallFunctionName(m_Context.getJSContext(), m_pObject, _functionName.c_str(), paramc, paramv, &rval);
    free(paramv);
    if(ok) {
      return rval;
    } else {
      m_Context.raisePendingExceptions();
      throw ScriptException("Error running function");
    }
  } // doCallFunctionByName

  template<>
  void ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    doCallFunctionByName(_functionName, _parameter);
  } // callFunctionByName<void>

  template<>
  int ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<int>(doCallFunctionByName(_functionName, _parameter));
  } // callFunctionByName<int>

  template<>
  uint8_t ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<int>(doCallFunctionByName(_functionName, _parameter));
  } // callFunctionByName<uint8_t>

  template<>
  uint16_t ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<int>(doCallFunctionByName(_functionName, _parameter));
  } // callFunctionByName<uint16_t>

  template<>
  uint32_t ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<int>(doCallFunctionByName(_functionName, _parameter));
  } // callFunctionByName<uint32_t>

  template<>
  double ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<double>(doCallFunctionByName(_functionName, _parameter));
  } // callFunctionByName<double>

  template<>
  bool ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<bool>(doCallFunctionByName(_functionName, _parameter));
  } // callFunctionByName<bool>

  template<>
  std::string ScriptObject::callFunctionByName(const std::string& _functionName,
                                                ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<std::string>(doCallFunctionByName(_functionName, _parameter));
  } // callFunctionByName<std::string>

  jsval ScriptObject::doCallFunctionByReference(jsval _function,
                                              ScriptFunctionParameterList& _parameter) {
    int paramc = _parameter.size();
    jsval* paramv = (jsval*)malloc(sizeof(jsval) * paramc);
    for(int iParam = 0; iParam < paramc; iParam++) {
      paramv[iParam] = _parameter.get(iParam);
    }
    jsval rval;
    JSBool ok = JS_CallFunctionValue(m_Context.getJSContext(), m_pObject, _function, paramc, paramv, &rval);
    free(paramv);
    if(ok) {
      return rval;
    } else {
      m_Context.raisePendingExceptions();
      throw ScriptException("Error running function");
    }
  } // doCallFunctionByReference

  template<>
  void ScriptObject::callFunctionByReference(jsval _function,
                                             ScriptFunctionParameterList& _parameter) {
    doCallFunctionByReference(_function, _parameter);
  } // callFunctionByReference<void>

  template<>
  int ScriptObject::callFunctionByReference(jsval _function,
                                            ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<int>(doCallFunctionByReference(_function, _parameter));
  } // callFunctionByReference<int>

  template<>
  double ScriptObject::callFunctionByReference(jsval _function,
                                               ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<double>(doCallFunctionByReference(_function, _parameter));
  } // callFunctionByReference<double>

  template<>
  bool ScriptObject::callFunctionByReference(jsval _function,
                                             ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<bool>(doCallFunctionByReference(_function, _parameter));
  } // callFunctionByReference<bool>

  template<>
  std::string ScriptObject::callFunctionByReference(jsval _function,
                                                    ScriptFunctionParameterList& _parameter) {
    return m_Context.convertTo<std::string>(doCallFunctionByReference(_function, _parameter));
  } // callFunctionByReference<std::string>

  void ScriptObject::addRoot() {
    JS_AddObjectRoot(m_Context.getJSContext(), &m_pObject);
  } // addRoot

  void ScriptObject::removeRoot() {
    JS_RemoveObjectRoot(m_Context.getJSContext(), &m_pObject);
  } // removeRoot


} // namespace dss
