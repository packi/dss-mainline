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

    Last change $Date: 2009-06-30 13:56:46 +0200 (Tue, 30 Jun 2009) $
    by $Author: pstaehlin $
*/

#ifndef RESTFUL_H_INCLUDED
#define RESTFUL_H_INCLUDED

#include "core/base.h"

#include <string>
#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>

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
  }; // RestfulParameter

  class RestfulMethod {
  private:
    std::string m_Name;
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

    bool checkRequest(const std::string& _uri, const Properties& _parameter);

    const std::string& getName() const { return m_Name; }
    const boost::ptr_vector<RestfulParameter>& getParameter() const { return m_Parameter; }
  }; // RestfulMethod

  class RestfulClass {
  private:
    std::string m_Name;
    boost::ptr_vector<RestfulMethod> m_Methods;
    boost::ptr_vector<RestfulMethod> m_StaticMethods;
    boost::ptr_vector<RestfulParameter> m_InstanceParameter;
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

    RestfulClass& requireOneOf(const std::string& _parameter1, const std::string& _parameter2) {
      return *this;
    }

    bool checkRequest(const std::string& _uri, const Properties& _parameter);

    const std::string& getName() const { return m_Name; }
    const boost::ptr_vector<RestfulParameter>& getInstanceParameter() const { return m_InstanceParameter; }
    const boost::ptr_vector<RestfulMethod>& getMethods() const { return m_Methods; }
    const boost::ptr_vector<RestfulMethod>& getStaticMethods() const { return m_StaticMethods; }
  }; // RestfulClass

  class RestfulAPI {
  private:
    boost::ptr_vector<RestfulClass> m_Classes;
  public:
    bool checkRequest(const std::string& _uri, const Properties& _parameter);
    RestfulClass& addClass(const std::string& _className) {
      m_Classes.push_back(new RestfulClass(_className, *this));
      return m_Classes.back();
    }

    const boost::ptr_vector<RestfulClass>& getClasses() const { return m_Classes; }
  }; // RestfulAPI

  class RestfulParameterType {
  private:
    std::string m_Name;
  public:
    RestfulParameterType(const std::string& _name)
    : m_Name(_name)
    { }

    virtual ~RestfulParameterType() {}

    const std::string& getName() { return m_Name; }

    virtual bool checkValue(const std::string& _value) = 0;
  }; // RestfulParameterType

  class RestfulParameterTypeString : public RestfulParameterType {
  public:
    virtual ~RestfulParameterTypeString() {}

    virtual bool checkValue(const std::string& _value) { return true; }
  }; // RestfulParameterTypeString

  class RestfulParameterTypeInteger : public RestfulParameterType {
  public:
    virtual ~RestfulParameterTypeInteger() {}

    virtual bool checkValue(const std::string& _value) {
      try {
        strToInt(_value);
      } catch(const std::invalid_argument& _arg) {
        return false;
      }
      return true;
    }
  };

  class RestfulRequest {
  private:
  }; // RestfulRequest

} // namespace dss

#endif
