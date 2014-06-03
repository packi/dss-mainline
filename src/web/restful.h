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

#ifndef RESTFUL_H_INCLUDED
#define RESTFUL_H_INCLUDED

#include "src/base.h"

#include <string>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace dss {

  class RestfulAPI;
  class RestfulClass;
  class RestfulMethod;
  class RestfulParameter;

  class RestfulParameter {
  private:
    std::string m_Name;
    std::string m_TypeName;
    bool m_Required;
  public:
    RestfulParameter(const std::string& _name, const std::string& _typeName, bool _required)
    : m_Name(_name), m_TypeName(_typeName), m_Required(_required)
    { }

    const std::string& getName() const { return m_Name; }
    const std::string& getTypeName() const { return m_TypeName; }
    bool isRequired() const { return m_Required; }
  }; // RestfulParameter

  class RestfulMethod {
  private:
    std::string m_Name;
    std::string m_DocumentationShort;
    std::string m_DocumentationLong;
    boost::ptr_vector<RestfulParameter> m_Parameter;
  public:
    RestfulMethod(const std::string& _name)
    : m_Name(_name)
    { }

    RestfulMethod& withParameter(const std::string& _name) {
      m_Parameter.push_back(new RestfulParameter(_name, "string", false));
      return *this;
    }
    RestfulMethod& withParameter(const std::string& _name, const std::string& _typeName) {
      m_Parameter.push_back(new RestfulParameter(_name, _typeName, false));
      return *this;
    }
    RestfulMethod& withParameter(const std::string& _name, const std::string& _typeName, bool _required) {
      m_Parameter.push_back(new RestfulParameter(_name, _typeName, _required));
      return *this;
    }

    RestfulMethod& withDocumentation(const std::string& _short, const std::string& _long = "") {
      m_DocumentationShort = _short;
      m_DocumentationLong = _long;
      return *this;
    }

    bool checkRequest(const std::string& _uri, const Properties& _parameter);

    const std::string& getName() const { return m_Name; }
    const boost::ptr_vector<RestfulParameter>& getParameter() const { return m_Parameter; }
    const std::string& getDocumentationShort() const { return m_DocumentationShort; }
    const std::string& getDocumentationLong() const { return m_DocumentationLong; }
  }; // RestfulMethod

  class RestfulClass {
  private:
    std::string m_Name;
    boost::ptr_vector<RestfulMethod> m_Methods;
    boost::ptr_vector<RestfulMethod> m_StaticMethods;
    boost::ptr_vector<RestfulParameter> m_InstanceParameter;
    std::string m_DocumentationShort;
    std::string m_DocumentationLong;
    //RestfulAPI& m_API;
  public:
    RestfulClass(const std::string& _name, RestfulAPI& _api)
    : m_Name(_name)//, m_API(_api)
    { }

    RestfulMethod& addMethod(const std::string& _name) {
      m_Methods.push_back(new RestfulMethod(_name));
      return m_Methods.back();
    }

    RestfulMethod& addStaticMethod(const std::string& _name) {
      m_StaticMethods.push_back(new RestfulMethod(_name));
      return m_StaticMethods.back();
    }
    RestfulClass& inheritFrom(const std::string& _className);

    RestfulClass& withInstanceParameter(const std::string& _name, const std::string _typeName, bool _required = false) {
      m_InstanceParameter.push_back(new RestfulParameter(_name, _typeName, _required));
      return *this;
    }

    RestfulClass& withDocumentation(const std::string& _short, const std::string& _long = "") {
      m_DocumentationShort = _short;
      m_DocumentationLong = _long;
      return *this;
    }

    RestfulClass& requireOneOf(const std::string& _parameter1, const std::string& _parameter2) {
      return *this;
    }

    bool checkRequest(const std::string& _uri, const Properties& _parameter);

    const std::string& getName() const { return m_Name; }
    const boost::ptr_vector<RestfulParameter>& getInstanceParameter() const { return m_InstanceParameter; }
    const boost::ptr_vector<RestfulMethod>& getMethods() const { return m_Methods; }
    const boost::ptr_vector<RestfulMethod>& getStaticMethods() const { return m_StaticMethods; }
    const std::string& getDocumentationShort() const { return m_DocumentationShort; }
    const std::string& getDocumentationLong() const { return m_DocumentationLong; }
    bool hasMethod(const std::string& _name);
  }; // RestfulClass

  class RestfulAPI {
  private:
    boost::ptr_vector<RestfulClass> m_Classes;
  public:
    bool checkRequest(const std::string& _uri, const Properties& _parameter);

    RestfulClass& addClass(const std::string& _className) {
      RestfulClass* result = new RestfulClass(_className, *this);
      m_Classes.push_back(result);
      return *result;
    }

    const boost::ptr_vector<RestfulClass>& getClasses() const { return m_Classes; }

    bool hasClass(const std::string& _name);
  }; // RestfulAPI

  class RestfulRequest {
  public:
    /**
     * @_request -- /json/system/login -- the /system/login part
     * @_parameter -- the parsed url query string
     */
    RestfulRequest(const std::string& _request, const HashMapStringString& _parameter)
    : m_urlSubPath(_request), m_Parameter(_parameter)
    {
      splitIntoMethodAndClass(_request);
    }

    const std::string& getUrlPath() const {
      return m_urlSubPath;
    }

    const std::string& getClass() const {
      return m_Class;
    } // getClass

    const std::string& getMethod() const {
      return m_Method;
    } // getMethod

    const std::string& getParameter(const std::string& _name) const {
      static const std::string& kEmptyString = "";
      HashMapStringString::const_iterator iEntry = m_Parameter.find(_name);
      if(iEntry != m_Parameter.end()) {
        return iEntry->second;
      } else {
        return kEmptyString;
      }
    } // getParameter

    bool hasParameter(const std::string& _name) const {
      HashMapStringString::const_iterator iEntry = m_Parameter.find(_name);
      return iEntry != m_Parameter.end();
    } // hasParameter

    bool isActive() const {
      if(m_ActiveCallback) {
        return m_ActiveCallback();
      }
      return true;
    }

    void setActiveCallback(boost::function<bool()> _value) {
      m_ActiveCallback = _value;
    }
  private:
    void splitIntoMethodAndClass(const std::string& _request) {
      size_t pos = _request.find('/');
      m_Class = _request.substr(0, pos);
      m_Method = _request.substr(pos+1, std::string::npos);
    } // splitIntoMethodAndClass
  private:
    std::string m_urlSubPath;
    std::string m_Class;
    std::string m_Method;
    HashMapStringString m_Parameter;
    boost::function<bool()> m_ActiveCallback;
  };

} // namespace dss

#endif
