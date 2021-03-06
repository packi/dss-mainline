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

    Last change $Date: 2009-06-30 13:56:46 +0200 (Tue, 30 Jun 2009) $
    by $Author: pstaehlin $
*/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "restful.h"

#include <boost/optional/optional.hpp>
#include <ds/log.h>
#include "base.h"

namespace dss {

  RestfulRequest::RestfulRequest(const std::string& _request,
                                 const std::string& params)
    : m_urlSubPath(_request), m_queryString("&" + params)
  {
    splitIntoMethodAndClass(_request);
  }

  boost::optional<std::string> RestfulRequest::tryGetParameter(const std::string& name) const {
    size_t end, offset;
    std::string tmp;

    // we added leading "&" upon init
    offset = m_queryString.find("&" + name + "=");
    if (offset == std::string::npos) {
      return boost::none;
    }

    offset += name.length() + 2;
    end = m_queryString.find('&', offset);
    tmp = (end == std::string::npos) ?
      m_queryString.substr(offset) :
      m_queryString.substr(offset, end - offset);

    return urlDecode(tmp);
  }

  std::string RestfulRequest::getRequiredParameter(const std::string& name) const {
    if (auto x = tryGetParameter(name)) {
      return *x;
    }
    DS_FAIL_REQUIRE("Missing parameter", name);
  }

  std::string RestfulRequest::getParameter(const std::string& name) const {
    if (auto x = tryGetParameter(name)) {
      return *x;
    }
    return std::string();
  }

  template <>
  bool RestfulRequest::getParameter<std::string>(const std::string& _name,
                                                 std::string &out) const {
    std::string tmp = getParameter(_name);
    if (tmp.empty()) {
      return false;
    }
    out = tmp;
    return true;
  }

  template <>
  bool RestfulRequest::getParameter<int>(const std::string& _name,
                                         int &out) const
  {
    std::string tmp = getParameter(_name);
    if (tmp.empty()) {
      return false;
    }
    try {
      out = strToInt(tmp);
      return true;
    } catch (...) {
      return false;
    }
  }

  template <>
  bool RestfulRequest::getParameter<unsigned int>(const std::string& _name,
                                                  unsigned int &out) const
  {
    std::string tmp = getParameter(_name);
    if (tmp.empty()) {
      return false;
    }
    try {
      out = strToUInt(tmp);
      return true;
    } catch (...) {
      return false;
    }
  }

  template <>
  bool RestfulRequest::getParameter<uint8_t>(const std::string& _name,
                                             uint8_t &out) const
  {
    std::string tmp = getParameter(_name);
    if (tmp.empty()) {
      return false;
    }
    try {
      out = strToUInt(tmp);
      return true;
    } catch (...) {
      return false;
    }
  }

  template <>
  bool RestfulRequest::getParameter<uint16_t>(const std::string& _name,
                                              uint16_t &out) const
  {
    std::string tmp = getParameter(_name);
    if (tmp.empty()) {
      return false;
    }
    try {
      out = strToUInt(tmp);
      return true;
    } catch (...) {
      return false;
    }
  }

  template <>
  bool RestfulRequest::getParameter<double>(const std::string& _name,
                                            double &out) const
  {
    std::string tmp = getParameter(_name);
    if (tmp.empty()) {
      return false;
    }
    try {
      out = strToDouble(tmp);
      return true;
    } catch (...) {
      return false;
    }
  }

  template <>
  bool RestfulRequest::getParameter<bool>(const std::string& _name,
                                          bool &out) const
  {
    std::string tmp = getParameter(_name);
    if (tmp.empty()) {
      return false;
    }
    try {
      int value;
      if ((value = strToIntDef(tmp, -1)) >= 0) {
        switch (value) {
          case 0: out = false; break;
          case 1: out = true; break;
          default: break;
        }
      } else if (tmp == "true") {
        out = true;
      } else if (tmp == "false") {
        out = false;
      } else {
        return false;
      }
      return true;
    } catch (...) {
      return false;
    }
  }

  /**
   * @_request -- '/system/login'
   * class -> 'system', method -> login everything till EOS
   */
  void RestfulRequest::splitIntoMethodAndClass(const std::string& _request) {
    if (_request.empty()) {
      return;
    }

    // TODO explicitly ensure leading '/', see toplevel/sublevel split
    size_t offset = _request.find('/', 1);
    if (std::string::npos == offset) {
      m_Class = _request.substr(1);
    } else {
      m_Class = _request.substr(1, offset - 1);
      m_Method = _request.substr(offset + 1);
    }
  }
} // namespace dss
