/*
    Copyright (c) 2009,2012 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#ifndef RESTFULAPIWRITER_H_
#define RESTFULAPIWRITER_H_

#include "restful.h"

namespace dss {

  class RestfulAPIWriter {
  private:
    static void writeToXML(const RestfulParameter& _parameter, std::string& _document, const int _indent);
  public:
    static void writeToXML(const RestfulAPI& api, const std::string& _location);
  }; // RestfulAPIWriter

} // namespace dss

#endif /* RESTFULAPIWRITER_H_ */
