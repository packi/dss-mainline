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

#include <boost/function.hpp>

#include "src/base.h"

namespace dss {

  class RestfulRequest {
  public:
    /**
     * @_request -- in '/json/system/login' -- the '/system/login' part
     * @_parameter -- the query part of the url
     */
    RestfulRequest(const std::string& _request, const std::string& params);

    const std::string& getUrlPath() const {
      return m_urlSubPath;
    }

    /**
     * @ret /[json|icon|browse]/class/method
     */
    const std::string& getClass() const {
      return m_Class;
    } // getClass

    /**
     * @ret /[json|icon|browse]/class/method
     */
    const std::string& getMethod() const {
      return m_Method;
    } // getMethod

    bool hasParameter(const std::string& _name) const {
      return (m_queryString.find("&" + _name + "=") != std::string::npos);
    }

    const std::string getParameter(const std::string& _name) const;

    template <typename T>
    bool getParameter(const std::string& _name, T &out) const;

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
    /**
     * @_request -- '/system/login'
     * class -> 'system', method -> login everything till EOS
     */
    void splitIntoMethodAndClass(const std::string& _request);
  private:
    std::string m_urlSubPath;
    std::string m_queryString;
    std::string m_Class;
    std::string m_Method;
    boost::function<bool()> m_ActiveCallback;
  };
} // namespace dss

#endif
