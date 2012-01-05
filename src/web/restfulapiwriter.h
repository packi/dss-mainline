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

#ifndef RESTFULAPIWRITER_H_
#define RESTFULAPIWRITER_H_

#include "restful.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

namespace dss {

  class RestfulAPIWriter {
  private:
    static Poco::XML::AutoPtr<Poco::XML::Element> writeToXML(const RestfulParameter& _parameter, Poco::XML::AutoPtr<Poco::XML::Document>& _document);
  public:
    static void writeToXML(const RestfulAPI& api, const std::string& _location);
  }; // RestfulAPIWriter

} // namespace dss

#endif /* RESTFULAPIWRITER_H_ */
