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

#include "restful.h"

#include "src/foreach.h"

namespace dss {

  bool RestfulAPI::hasClass(const std::string& _name) {
    foreach(RestfulClass& cls, m_Classes) {
      if(cls.getName() == _name) {
        return true;
      }
    }
    return false;
  } // hasClass

  bool RestfulClass::hasMethod(const std::string& _name) {
    foreach(RestfulMethod& method, m_Methods) {
      if(method.getName() == _name) {
        return true;
      }
    }
    return false;
  } // hasMethod
  
} // namespace dss